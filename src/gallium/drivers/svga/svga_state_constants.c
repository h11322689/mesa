   /**********************************************************
   * Copyright 2008-2022 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "util/format/u_format.h"
   #include "util/u_bitmask.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "pipe/p_defines.h"
   #include "util/u_upload_mgr.h"
      #include "svga_screen.h"
   #include "svga_context.h"
   #include "svga_state.h"
   #include "svga_cmd.h"
   #include "svga_tgsi.h"
   #include "svga_debug.h"
   #include "svga_resource_buffer.h"
   #include "svga_shader.h"
      #include "svga_hw_reg.h"
         static unsigned
   svga_get_image_size_constant(const struct svga_context *svga, float **dest,
                     {
               for (int i = 0; i < num_image_views; i++) {
      if (images[shader][i].desc.resource) {
      if (images[shader][i].desc.resource->target == PIPE_BUFFER) {
      unsigned bytes_per_element = util_format_get_blocksize(images[shader][i].desc.format);
      }
                  if (images[shader][i].desc.resource->target == PIPE_TEXTURE_1D_ARRAY)
                        if (images[shader][i].desc.resource->target == PIPE_TEXTURE_2D_ARRAY)
         else if (images[shader][i].desc.resource->target == PIPE_TEXTURE_CUBE_ARRAY)
         else
            }
   else {
            }
      }
         /*
   * Don't try to send more than 4kb of successive constants.
   */
   #define MAX_CONST_REG_COUNT 256  /**< number of float[4] constants */
      /**
   * Extra space for svga-specific VS/PS constants (such as texcoord
   * scale factors, vertex transformation scale/translation).
   */
   #define MAX_EXTRA_CONSTS 32
      /** Guest-backed surface constant buffers must be this size */
   #define GB_CONSTBUF_SIZE (SVGA3D_CONSTREG_MAX)
         /**
   * Emit any extra shader-type-independent shader constants into the buffer
   * pointed to by 'dest'.
   * \return number of float[4] constants put into the 'dest' buffer
   */
   static unsigned
   svga_get_extra_constants_common(const struct svga_context *svga,
               {
      uint32_t *dest_u = (uint32_t *) dest;  // uint version of dest
   unsigned i;
            for (i = 0; i < variant->key.num_textures; i++) {
      const struct pipe_sampler_view *sv = svga->curr.sampler_views[shader][i];
   if (sv) {
      const struct pipe_resource *tex = sv->texture;
   /* Scaling factors needed for handling unnormalized texture coordinates
   * for texture rectangles.
   */
   if (variant->key.tex[i].unnormalized) {
                     *dest++ = 1.0f / (float) tex->width0;
   *dest++ = 1.0f / (float) tex->height0;
                              /* Store the sizes for texture buffers.
   */
   if (tex->target == PIPE_BUFFER) {
      unsigned bytes_per_element = util_format_get_blocksize(sv->format);
   *dest_u++ = tex->width0 / bytes_per_element;
   *dest_u++ = 1;
                                    /* image_size */
   if (variant->key.image_size_used) {
      count += svga_get_image_size_constant(svga, &dest, shader,
                           }
         /**
   * Emit any extra fragment shader constants into the buffer pointed
   * to by 'dest'.
   * \return number of float[4] constants put into the dest buffer
   */
   static unsigned
   svga_get_extra_fs_constants(const struct svga_context *svga, float *dest)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.fs;
            count += svga_get_extra_constants_common(svga, variant,
                        }
      /**
   * Emit extra constants needed for prescale computation into the
   * the buffer pointed to by '*dest'. The updated buffer pointer
   * will be returned in 'dest'.
   */
   static unsigned
   svga_get_prescale_constants(const struct svga_context *svga, float **dest,
         {
      memcpy(*dest, prescale->scale, 4 * sizeof(float));
            memcpy(*dest, prescale->translate, 4 * sizeof(float));
               }
      /**
   * Emit extra constants needed for point sprite emulation.
   */
   static unsigned
   svga_get_pt_sprite_constants(const struct svga_context *svga, float **dest)
   {
      const struct svga_screen *screen = svga_screen(svga->pipe.screen);
            dst[0] = 1.0 / (svga->curr.viewport[0].scale[0] * 2);
   dst[1] = 1.0 / (svga->curr.viewport[0].scale[1] * 2);
   dst[2] = svga->curr.rast->pointsize;
   dst[3] = screen->maxPointSize;
   *dest = *dest + 4;
      }
      /**
   * Emit user-defined clip plane coefficients into the buffer pointed to
   * by '*dest'. The updated buffer pointer will be returned in 'dest'.
   */
   static unsigned
   svga_get_clip_plane_constants(const struct svga_context *svga,
               {
               /* SVGA_NEW_CLIP */
   if (svga_have_vgpu10(svga)) {
      /* append user-defined clip plane coefficients onto constant buffer */
   unsigned clip_planes = variant->key.clip_plane_enable;
   while (clip_planes) {
      int i = u_bit_scan(&clip_planes);
   COPY_4V(*dest, svga->curr.clip.ucp[i]);
   *dest += 4;
         }
      }
         /**
   * Emit any extra vertex shader constants into the buffer pointed
   * to by 'dest'.
   * In particular, these would be the scale and bias factors computed
   * from the framebuffer size which are used to copy with differences in
   * GL vs D3D coordinate spaces.  See svga_tgsi_insn.c for more info.
   * \return number of float[4] constants put into the dest buffer
   */
   static unsigned
   svga_get_extra_vs_constants(const struct svga_context *svga, float *dest)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.vs;
            /* SVGA_NEW_VS_VARIANT
   */
   if (variant->key.vs.need_prescale) {
      count += svga_get_prescale_constants(svga, &dest,
               if (variant->key.vs.undo_viewport) {
      /* Used to convert window coords back to NDC coords */
   dest[0] = 1.0f / svga->curr.viewport[0].scale[0];
   dest[1] = 1.0f / svga->curr.viewport[0].scale[1];
   dest[2] = -svga->curr.viewport[0].translate[0];
   dest[3] = -svga->curr.viewport[0].translate[1];
   dest += 4;
               /* Bias to be added to VertexID */
   if (variant->key.vs.need_vertex_id_bias) {
      uint32_t *dest_u = (uint32_t *) dest;  // uint version of dest
   dest_u[0] = svga->curr.vertex_id_bias;
   dest_u[1] = 1;
   dest_u[2] = 1;
   dest_u[3] = 1;
   dest+=4;
               /* SVGA_NEW_CLIP */
            /* common constants */
   count += svga_get_extra_constants_common(svga, variant,
                        }
      /**
   * Emit any extra geometry shader constants into the buffer pointed
   * to by 'dest'.
   */
   static unsigned
   svga_get_extra_gs_constants(const struct svga_context *svga, float *dest)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.gs;
            /* SVGA_NEW_GS_VARIANT
            /* Constants for point sprite
   * These are used in the transformed gs that supports point sprite.
   * They need to be added before the prescale constants.
   */
   if (variant->key.gs.wide_point) {
                  if (variant->key.gs.need_prescale) {
               /* If prescale is needed and the geometry shader writes to viewport
   * index, then prescale for all viewports will be added to the
   * constant buffer.
   */
   if (variant->key.gs.writes_viewport_index)
            for (i = 0; i < num_prescale; i++) {
      count +=
      svga_get_prescale_constants(svga, &dest,
               /* SVGA_NEW_CLIP */
            /* common constants */
   count += svga_get_extra_constants_common(svga, variant,
            assert(count <= MAX_EXTRA_CONSTS);
      }
         /**
   * Emit any extra tessellation control shader constants into the
   * buffer pointed to by 'dest'.
   */
   static unsigned
   svga_get_extra_tcs_constants(struct svga_context *svga, float *dest)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.tcs;
            /* SVGA_NEW_CLIP */
            /* common constants */
   count += svga_get_extra_constants_common(svga, variant,
                  assert(count <= MAX_EXTRA_CONSTS);
      }
         /**
   * Emit any extra tessellation evaluation shader constants into
   * the buffer pointed to by 'dest'.
   */
   static unsigned
   svga_get_extra_tes_constants(struct svga_context *svga, float *dest)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.tes;
            if (variant->key.tes.need_prescale) {
      count += svga_get_prescale_constants(svga, &dest,
               /* SVGA_NEW_CLIP */
            /* common constants */
   count += svga_get_extra_constants_common(svga, variant,
                  assert(count <= MAX_EXTRA_CONSTS);
      }
         /**
   * Emit any extra compute shader constants into
   * the buffer pointed to by 'dest'.
   */
   static unsigned
   svga_get_extra_cs_constants(struct svga_context *svga, float *dest)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.cs;
            /* common constants */
   count += svga_get_extra_constants_common(svga, variant,
                  assert(count <= MAX_EXTRA_CONSTS);
      }
         /*
   * Check and emit a range of shader constant registers, trying to coalesce
   * successive shader constant updates in a single command in order to save
   * space on the command buffer.  This is a HWv8 feature.
   */
   static enum pipe_error
   emit_const_range(struct svga_context *svga,
                  enum pipe_shader_type shader,
      {
      unsigned i, j;
            assert(shader == PIPE_SHADER_VERTEX ||
               #ifdef DEBUG
      if (offset + count > SVGA3D_CONSTREG_MAX) {
      debug_printf("svga: too many constants (offset %u + count %u = %u (max = %u))\n",
         #endif
         if (offset > SVGA3D_CONSTREG_MAX) {
      /* This isn't OK, but if we propagate an error all the way up we'll
   * just get into more trouble.
   * XXX note that offset is always zero at this time so this is moot.
   */
               if (offset + count > SVGA3D_CONSTREG_MAX) {
      /* Just drop the extra constants for now.
   * Ideally we should not have allowed the app to create a shader
   * that exceeds our constant buffer size but there's no way to
   * express that in gallium at this time.
   */
               i = 0;
   while (i < count) {
      if (memcmp(svga->state.hw_draw.cb[shader][offset + i],
            values[i],
   /* Found one dirty constant
   */
   if (SVGA_DEBUG & DEBUG_CONSTS)
      debug_printf("%s %s %d: %f %f %f %f\n",
               __func__,
   shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
   offset + i,
               /* Look for more consecutive dirty constants.
   */
   j = i + 1;
   while (j < count &&
         j < i + MAX_CONST_REG_COUNT &&
                     if (SVGA_DEBUG & DEBUG_CONSTS)
      debug_printf("%s %s %d: %f %f %f %f\n",
               __func__,
   shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                                          /* Send them all together.
   */
   if (svga_have_gb_objects(svga)) {
      ret = SVGA3D_SetGBShaderConstsInline(svga->swc,
                              }
   else {
      ret = SVGA3D_SetShaderConsts(svga->swc,
                        }
   if (ret != PIPE_OK) {
                  /*
   * Local copy of the hardware state.
   */
   memcpy(svga->state.hw_draw.cb[shader][offset + i],
                                 } else {
                        }
         /**
   * Emit all the constants in a constant buffer for a shader stage.
   * On VGPU10, emit_consts_vgpu10 is used instead.
   */
   static enum pipe_error
   emit_consts_vgpu9(struct svga_context *svga, enum pipe_shader_type shader)
   {
      const struct pipe_constant_buffer *cbuf;
   struct pipe_transfer *transfer = NULL;
   unsigned count;
   const float (*data)[4] = NULL;
   enum pipe_error ret = PIPE_OK;
            assert(shader < PIPE_SHADER_TYPES);
   assert(!svga_have_vgpu10(svga));
   /* Only one constant buffer per shader is supported before VGPU10.
   * This is only an approximate check against that.
   */
                     if (svga->curr.constbufs[shader][0].buffer) {
      /* emit user-provided constants */
   data = (const float (*)[4])
      pipe_buffer_map(&svga->pipe, svga->curr.constbufs[shader][0].buffer,
      if (!data) {
                  /* sanity check */
            /* Use/apply the constant buffer size and offsets here */
   count = cbuf->buffer_size / (4 * sizeof(float));
                              if (ret != PIPE_OK) {
                     /* emit extra shader constants */
   {
      const struct svga_shader_variant *variant = NULL;
   unsigned offset;
   float extras[MAX_EXTRA_CONSTS][4];
            switch (shader) {
   case PIPE_SHADER_VERTEX:
      variant = svga->state.hw_draw.vs;
   count = svga_get_extra_vs_constants(svga, (float *) extras);
      case PIPE_SHADER_FRAGMENT:
      variant = svga->state.hw_draw.fs;
   count = svga_get_extra_fs_constants(svga, (float *) extras);
      default:
      assert(!"Unexpected shader type");
               assert(variant);
   offset = variant->shader->info.constbuf0_num_uniforms;
            if (count > 0) {
      ret = emit_const_range(svga, shader, offset, count,
                     }
         /**
   * A helper function to destroy any pending unused srv.
   */
   void
   svga_destroy_rawbuf_srv(struct svga_context *svga)
   {
               while ((index = util_bitmask_get_next_index(
                     SVGA_RETRY(svga, SVGA3D_vgpu10_DestroyShaderResourceView(svga->swc,
         util_bitmask_clear(svga->sampler_view_id_bm, index);
         }
      /**
   * A helper function to emit constant buffer as srv raw buffer.
   */
   enum pipe_error
   svga_emit_rawbuf(struct svga_context *svga,
                  unsigned slot,
   enum pipe_shader_type shader,
      {
                        struct svga_raw_buffer *rawbuf = &svga->state.hw_draw.rawbufs[shader][slot];
   struct svga_winsys_surface *buf_handle = NULL;
   unsigned srvid = SVGA3D_INVALID_ID;
                     if (buffer == NULL) {
      if ((svga->state.hw_draw.enabled_rawbufs[shader] & (1 << slot)) == 0) {
         }
      }
   else {
      if ((rawbuf->buffer_offset != buffer_offset) ||
                     /* Add the current srvid to the delete list */
   if (rawbuf->srvid != SVGA3D_INVALID_ID) {
      util_bitmask_set(svga->sampler_view_to_free_id_bm, rawbuf->srvid);
               buf_handle = svga_buffer_handle(svga, buffer,
         if (!buf_handle) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
                              SVGA3dShaderResourceViewDesc viewDesc;
   viewDesc.bufferex.firstElement = buffer_offset / 4;
                  ret = SVGA3D_vgpu10_DefineShaderResourceView(svga->swc,
                  if (ret != PIPE_OK) {
      util_bitmask_clear(svga->sampler_view_id_bm, srvid);
               /* Save the current raw buffer attributes in the slot */
   rawbuf->srvid = srvid;
   rawbuf->buffer_size = buffer_size;
                     }
   else {
      /* Same buffer attributes in the slot. Can use the same SRV. */
   assert(rawbuf->srvid != SVGA3D_INVALID_ID);
   srvid = rawbuf->srvid;
      }
               ret = SVGA3D_vgpu10_SetShaderResources(svga->swc,
                                 if (ret != PIPE_OK) {
                  /* Save the enabled rawbuf state */
         done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         /**
   * A helper function to emit a constant buffer binding at the
   * specified slot for the specified shader type
   */
   static enum pipe_error
   emit_constbuf(struct svga_context *svga,
               unsigned slot,
   enum pipe_shader_type shader,
   unsigned buffer_offset,
   unsigned buffer_size,
   const void *buffer,
   unsigned extra_buffer_offset,
   {
      struct svga_buffer *sbuf = svga_buffer((struct pipe_resource *)buffer);
   struct pipe_resource *dst_buffer = NULL;
   enum pipe_error ret = PIPE_OK;
   struct pipe_transfer *src_transfer;
   struct svga_winsys_surface *dst_handle = NULL;
   unsigned new_buf_size = 0;
   unsigned alloc_buf_size;
   unsigned offset = 0;;
                        /* buffer here is a user-space buffer so mapping it is really cheap. */
   if (buffer_size > 0) {
      src_map = pipe_buffer_map_range(&svga->pipe,
                     assert(src_map);
   if (!src_map) {
                              /* According to the DX10 spec, the constant buffer size must be
   * in multiples of 16.
   */
            /* Constant buffer size in the upload buffer must be in multiples of 256.
   * In order to maximize the chance of merging the upload buffer chunks
   * when svga_buffer_add_range() is called,
   * the allocate buffer size needs to be in multiples of 256 as well.
   * Otherwise, since there is gap between each dirty range of the upload buffer,
   * each dirty range will end up in its own UPDATE_GB_IMAGE command.
   */
            u_upload_alloc(svga->const0_upload, 0, alloc_buf_size,
                  if (!dst_map) {
      if (src_map)
                     /* Initialize the allocated buffer slot to 0 to ensure the padding is
   * filled with 0.
   */
            if (src_map) {
      memcpy(dst_map, src_map, buffer_size);
               if (extra_buffer_size) {
      assert(extra_buffer_offset + extra_buffer_size <= new_buf_size);
   memcpy((char *) dst_map + extra_buffer_offset, extra_buffer,
               /* Get winsys handle for the constant buffer */
   if (svga->state.hw_draw.const0_buffer == dst_buffer &&
      svga->state.hw_draw.const0_handle) {
   /* re-reference already mapped buffer */
      }
   else {
                     dst_handle = svga_buffer_handle(svga, dst_buffer,
         if (!dst_handle) {
      pipe_resource_reference(&dst_buffer, NULL);
            }
   else if (sbuf) {
      dst_handle = svga_buffer_handle(svga, &sbuf->b, PIPE_BIND_CONSTANT_BUFFER);
   new_buf_size = align(buffer_size, 16);
                        /* clamp the buf size before sending the command */
            const struct svga_screen *screen = svga_screen(svga->pipe.screen);
            /* Issue the SetSingleConstantBuffer command */
   if (!sws->have_constant_buffer_offset_cmd ||
      svga->state.hw_draw.constbufoffsets[shader][slot].handle != dst_handle ||
   svga->state.hw_draw.constbufoffsets[shader][slot].size != new_buf_size) {
   ret = SVGA3D_vgpu10_SetSingleConstantBuffer(svga->swc,
                              }
   else if (dst_handle){
      unsigned command = SVGA_3D_CMD_DX_SET_VS_CONSTANT_BUFFER_OFFSET +
         ret = SVGA3D_vgpu10_SetConstantBufferOffset(svga->swc,
                           if (ret != PIPE_OK) {
      pipe_resource_reference(&dst_buffer, NULL);
               /* save the upload buffer / handle for next time */
   if (dst_buffer != buffer && dst_buffer) {
      pipe_resource_reference(&svga->state.hw_draw.const0_buffer, dst_buffer);
               /* Save this const buffer until it's replaced in the future.
   * Otherwise, all references to the buffer will go away after the
   * command buffer is submitted, it'll get recycled and we will have
   * incorrect constant buffer bindings.
   */
   pipe_resource_reference(&svga->state.hw_draw.constbuf[shader][slot], dst_buffer);
   svga->state.hw_draw.constbufoffsets[shader][slot].handle = dst_handle;
                        }
         /* For constbuf 0 */
   static enum pipe_error
   emit_consts_vgpu10(struct svga_context *svga, enum pipe_shader_type shader)
   {
      const struct pipe_constant_buffer *cbuf;
   enum pipe_error ret = PIPE_OK;
   float extras[MAX_EXTRA_CONSTS][4];
   unsigned extra_count, extra_size, extra_offset;
            assert(shader == PIPE_SHADER_VERTEX ||
         shader == PIPE_SHADER_GEOMETRY ||
   shader == PIPE_SHADER_FRAGMENT ||
   shader == PIPE_SHADER_TESS_CTRL ||
                     switch (shader) {
   case PIPE_SHADER_VERTEX:
      variant = svga->state.hw_draw.vs;
   extra_count = svga_get_extra_vs_constants(svga, (float *) extras);
      case PIPE_SHADER_FRAGMENT:
      variant = svga->state.hw_draw.fs;
   extra_count = svga_get_extra_fs_constants(svga, (float *) extras);
      case PIPE_SHADER_GEOMETRY:
      variant = svga->state.hw_draw.gs;
   extra_count = svga_get_extra_gs_constants(svga, (float *) extras);
      case PIPE_SHADER_TESS_CTRL:
      variant = svga->state.hw_draw.tcs;
   extra_count = svga_get_extra_tcs_constants(svga, (float *) extras);
      case PIPE_SHADER_TESS_EVAL:
      variant = svga->state.hw_draw.tes;
   extra_count = svga_get_extra_tes_constants(svga, (float *) extras);
      case PIPE_SHADER_COMPUTE:
      variant = svga->state.hw_draw.cs;
   extra_count = svga_get_extra_cs_constants(svga, (float *) extras);
      default:
      assert(!"Unexpected shader type");
   /* Don't return an error code since we don't want to keep re-trying
   * this function and getting stuck in an infinite loop.
   */
                                 /* Compute extra constants size and offset in bytes */
   extra_size = extra_count * 4 * sizeof(float);
            if (cbuf->buffer_size + extra_size == 0)
            ret = emit_constbuf(svga, 0, shader,
               if (ret != PIPE_OK)
            svga->state.hw_draw.default_constbuf_size[shader] =
                        }
         static enum pipe_error
   emit_constbuf_vgpu10(struct svga_context *svga, enum pipe_shader_type shader)
   {
      enum pipe_error ret = PIPE_OK;
   unsigned dirty_constbufs;
            enabled_constbufs = svga->state.hw_draw.enabled_constbufs[shader] | 1u;
            while (dirty_constbufs) {
      unsigned index = u_bit_scan(&dirty_constbufs);
   unsigned offset = svga->curr.constbufs[shader][index].buffer_offset;
   unsigned size = svga->curr.constbufs[shader][index].buffer_size;
   struct svga_buffer *buffer =
            if (buffer) {
         }
   else {
      enabled_constbufs &= ~(1 << index);
   assert(offset == 0);
               if (size % 16 != 0) {
      /* GL's buffer range sizes can be any number of bytes but the
   * SVGA3D device requires a multiple of 16 bytes.
                  if (offset + align(size, 16) <= total_size) {
      /* round up size to multiple of 16 */
      }
   else {
      /* round down to multiple of 16 (this may cause rendering problems
   * but should avoid a device error).
   */
                           /**
   * If the buffer has been bound as an uav buffer, it will
   * need to be bound as a shader resource raw buffer.
   */
   if (svga->state.raw_constbufs[shader] & (1 << index)) {
      ret = svga_emit_rawbuf(svga, index, shader, offset, size, buffer);
   if (ret != PIPE_OK) {
                  ret = emit_constbuf(svga, index, shader, 0, 0, NULL,
         if (ret != PIPE_OK) {
                  /* Remove the rawbuf from the to-be-enabled constbuf list
   * so the buffer will not be referenced again as constant buffer
   * at resource validation time.
   */
      }
   else {
      if (svga->state.hw_draw.enabled_rawbufs[shader] & (1 << index)) {
      ret = svga_emit_rawbuf(svga, index, shader, offset, size, NULL);
   if (ret != PIPE_OK) {
                     ret = emit_constbuf(svga, index, shader, offset, size, buffer,
         if (ret != PIPE_OK) {
            }
               svga->state.hw_draw.enabled_constbufs[shader] = enabled_constbufs;
               }
      static enum pipe_error
   emit_fs_consts(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.fs;
            /* SVGA_NEW_FS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_FS_CONSTS
   */
   if (svga_have_vgpu10(svga)) {
         }
   else {
                     }
      static enum pipe_error
   emit_fs_constbuf(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.fs;
            /* SVGA_NEW_FS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_FS_CONSTBUF
   */
   assert(svga_have_vgpu10(svga));
               }
      struct svga_tracked_state svga_hw_fs_constants =
   {
      "hw fs params",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_FS_CONSTS |
   SVGA_NEW_FS_VARIANT |
   SVGA_NEW_TEXTURE_CONSTS),
      };
         struct svga_tracked_state svga_hw_fs_constbufs =
   {
      "hw fs params",
   SVGA_NEW_FS_CONST_BUFFER,
      };
         static enum pipe_error
   emit_vs_consts(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.vs;
            /* SVGA_NEW_VS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_VS_CONST_BUFFER
   */
   if (svga_have_vgpu10(svga)) {
         }
   else {
                     }
         static enum pipe_error
   emit_vs_constbuf(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.vs;
            /* SVGA_NEW_FS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_FS_CONSTBUF
   */
   assert(svga_have_vgpu10(svga));
               }
         struct svga_tracked_state svga_hw_vs_constants =
   {
      "hw vs params",
   (SVGA_NEW_PRESCALE |
   SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_VS_CONSTS |
   SVGA_NEW_VS_VARIANT |
   SVGA_NEW_TEXTURE_CONSTS),
      };
         struct svga_tracked_state svga_hw_vs_constbufs =
   {
      "hw vs params",
   SVGA_NEW_VS_CONST_BUFFER,
      };
         static enum pipe_error
   emit_gs_consts(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.gs;
            /* SVGA_NEW_GS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_GS_CONST_BUFFER
   */
            /**
   * If only the rasterizer state has changed and the current geometry
   * shader does not emit wide points, then there is no reason to
   * re-emit the GS constants, so skip it.
   */
   if (dirty == SVGA_NEW_RAST && !variant->key.gs.wide_point)
                        }
         static enum pipe_error
   emit_gs_constbuf(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.gs;
            /* SVGA_NEW_GS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_GS_CONSTBUF
   */
   assert(svga_have_vgpu10(svga));
               }
         struct svga_tracked_state svga_hw_gs_constants =
   {
      "hw gs params",
   (SVGA_NEW_PRESCALE |
   SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_GS_CONSTS |
   SVGA_NEW_RAST |
   SVGA_NEW_GS_VARIANT |
   SVGA_NEW_TEXTURE_CONSTS),
      };
         struct svga_tracked_state svga_hw_gs_constbufs =
   {
      "hw gs params",
   SVGA_NEW_GS_CONST_BUFFER,
      };
         /**
   * Emit constant buffer for tessellation control shader
   */
   static enum pipe_error
   emit_tcs_consts(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.tcs;
                     /* SVGA_NEW_TCS_VARIANT */
   if (!variant)
                                 }
         static enum pipe_error
   emit_tcs_constbuf(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.tcs;
            /* SVGA_NEW_TCS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_TCS_CONSTBUF
   */
   assert(svga_have_vgpu10(svga));
               }
         struct svga_tracked_state svga_hw_tcs_constants =
   {
      "hw tcs params",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_TCS_CONSTS |
   SVGA_NEW_TCS_VARIANT),
      };
         struct svga_tracked_state svga_hw_tcs_constbufs =
   {
      "hw tcs params",
   SVGA_NEW_TCS_CONST_BUFFER,
      };
         /**
   * Emit constant buffer for tessellation evaluation shader
   */
   static enum pipe_error
   emit_tes_consts(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.tes;
                     /* SVGA_NEW_TES_VARIANT */
   if (!variant)
                        }
         static enum pipe_error
   emit_tes_constbuf(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.tes;
            /* SVGA_NEW_TES_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_TES_CONSTBUF
   */
   assert(svga_have_vgpu10(svga));
               }
         struct svga_tracked_state svga_hw_tes_constants =
   {
      "hw tes params",
   (SVGA_NEW_PRESCALE |
   SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_TES_CONSTS |
   SVGA_NEW_TES_VARIANT),
      };
         struct svga_tracked_state svga_hw_tes_constbufs =
   {
      "hw gs params",
   SVGA_NEW_TES_CONST_BUFFER,
      };
         /**
   * Emit constant buffer for compute shader
   */
   static enum pipe_error
   emit_cs_consts(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.cs;
                     /* SVGA_NEW_CS_VARIANT */
   if (!variant)
            /* SVGA_NEW_CS_CONST_BUFFER */
               }
         static enum pipe_error
   emit_cs_constbuf(struct svga_context *svga, uint64_t dirty)
   {
      const struct svga_shader_variant *variant = svga->state.hw_draw.cs;
            /* SVGA_NEW_CS_VARIANT
   */
   if (!variant)
            /* SVGA_NEW_CS_CONSTBUF
   */
   assert(svga_have_vgpu10(svga));
               }
         struct svga_tracked_state svga_hw_cs_constants =
   {
      "hw cs params",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_CS_CONSTS |
   SVGA_NEW_CS_VARIANT |
   SVGA_NEW_TEXTURE_CONSTS),
      };
         struct svga_tracked_state svga_hw_cs_constbufs =
   {
      "hw cs params",
   SVGA_NEW_CS_CONST_BUFFER,
      };
         /**
   * A helper function to update the rawbuf for constbuf mask
   */
   static void
   update_rawbuf_mask(struct svga_context *svga, enum pipe_shader_type shader)
   {
      unsigned dirty_constbufs;
            enabled_constbufs = svga->state.hw_draw.enabled_constbufs[shader] | 1u;
            while (dirty_constbufs) {
      unsigned index = u_bit_scan(&dirty_constbufs);
   struct svga_buffer *sbuf =
            if (sbuf && svga_has_raw_buffer_view(sbuf)) {
         } else {
               }
         /**
   * update_rawbuf is called at hw state update time to determine
   * if any of the bound constant buffers need to be bound as
   * raw buffer srv. This function is called after uav state is
   * updated and before shader variants are bound.
   */
   static enum pipe_error
   update_rawbuf(struct svga_context *svga, uint64 dirty)
   {
      uint64_t rawbuf_dirtybit[] = {
      SVGA_NEW_VS_RAW_BUFFER,       /* PIPE_SHADER_VERTEX */
   SVGA_NEW_FS_RAW_BUFFER,       /* PIPE_SHADER_FRAGMENT */
   SVGA_NEW_GS_RAW_BUFFER,       /* PIPE_SHADER_GEOMETRY */
   SVGA_NEW_TCS_RAW_BUFFER,      /* PIPE_SHADER_TESS_CTRL */
               for (enum pipe_shader_type shader = PIPE_SHADER_VERTEX;
      shader < PIPE_SHADER_COMPUTE; shader++) {
   unsigned rawbuf_mask = svga->state.raw_constbufs[shader];
                     /* If the rawbuf state is different for the shader stage,
   * send SVGA_NEW_XX_RAW_BUFFER to trigger a new shader
   * variant that will use srv for ubo access.
   */
   if ((svga->state.raw_constbufs[shader] != rawbuf_mask) ||
      (svga->state.raw_shaderbufs[shader] != rawbuf_sbuf_mask))
               }
         struct svga_tracked_state svga_need_rawbuf_srv =
   {
      "raw buffer srv",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_SHADER_BUFFER |
   SVGA_NEW_CONST_BUFFER),
      };
         /**
   * update_cs_rawbuf is called at compute dispatch time to determine
   * if any of the bound constant buffers need to be bound as
   * raw buffer srv. This function is called after uav state is
   * updated and before a compute shader variant is bound.
   */
   static enum pipe_error
   update_cs_rawbuf(struct svga_context *svga, uint64 dirty)
   {
                        /* if the rawbuf state is different for the shader stage,
   * send SVGA_NEW_RAW_BUFFER to trigger a new shader
   * variant to use srv for ubo access.
   */
   if (svga->state.raw_constbufs[PIPE_SHADER_COMPUTE] != rawbuf_mask)
               }
         struct svga_tracked_state svga_cs_need_rawbuf_srv =
   {
      "raw buffer srv",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_SHADER_BUFFER |
   SVGA_NEW_CONST_BUFFER),
      };
