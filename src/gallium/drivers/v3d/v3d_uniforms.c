   /*
   * Copyright © 2014-2017 Broadcom
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "util/u_pack_color.h"
   #include "util/u_upload_mgr.h"
      #include "v3d_context.h"
   #include "compiler/v3d_compiler.h"
      /* We don't expect that the packets we use in this file change across across
   * hw versions, so we just include directly the v33 header
   */
   #include "broadcom/cle/v3d_packet_v33_pack.h"
      static uint32_t
   get_texrect_scale(struct v3d_texture_stateobj *texstate,
               {
         struct pipe_sampler_view *texture = texstate->textures[data];
            if (contents == QUNIFORM_TEXRECT_SCALE_X)
         else
            }
      static uint32_t
   get_texture_size(struct v3d_texture_stateobj *texstate,
               {
         struct pipe_sampler_view *texture = texstate->textures[data];
   switch (contents) {
   case QUNIFORM_TEXTURE_WIDTH:
            if (texture->target == PIPE_BUFFER) {
               } else {
            case QUNIFORM_TEXTURE_HEIGHT:
               case QUNIFORM_TEXTURE_DEPTH:
            assert(texture->target != PIPE_BUFFER);
      case QUNIFORM_TEXTURE_ARRAY_SIZE:
            assert(texture->target != PIPE_BUFFER);
   if (texture->target != PIPE_TEXTURE_CUBE_ARRAY) {
         } else {
            case QUNIFORM_TEXTURE_LEVELS:
            assert(texture->target != PIPE_BUFFER);
      default:
         }
      static uint32_t
   get_image_size(struct v3d_shaderimg_stateobj *shaderimg,
               {
                  switch (contents) {
   case QUNIFORM_IMAGE_WIDTH:
            if (image->base.resource->target == PIPE_BUFFER) {
               } else {
            case QUNIFORM_IMAGE_HEIGHT:
            assert(image->base.resource->target != PIPE_BUFFER);
      case QUNIFORM_IMAGE_DEPTH:
            assert(image->base.resource->target != PIPE_BUFFER);
      case QUNIFORM_IMAGE_ARRAY_SIZE:
            assert(image->base.resource->target != PIPE_BUFFER);
   if (image->base.resource->target != PIPE_TEXTURE_CUBE_ARRAY) {
         } else {
            default:
         }
      /**
   *  Writes the V3D 3.x P0 (CFG_MODE=1) texture parameter.
   *
   * Some bits of this field are dependent on the type of sample being done by
   * the shader, while other bits are dependent on the sampler state.  We OR the
   * two together here.
   */
   static void
   write_texture_p0(struct v3d_job *job,
                  struct v3d_cl_out **uniforms,
      {
         struct pipe_sampler_state *psampler = texstate->samplers[unit];
            }
      /** Writes the V3D 3.x P1 (CFG_MODE=1) texture parameter. */
   static void
   write_texture_p1(struct v3d_job *job,
                     {
         /* Extract the texture unit from the top bits, and the compiler's
      * packed p1 from the bottom.
      uint32_t unit = data >> 5;
            struct pipe_sampler_view *psview = texstate->textures[unit];
            struct V3D33_TEXTURE_UNIFORM_PARAMETER_1_CFG_MODE1 unpacked = {
                  uint32_t packed;
   V3D33_TEXTURE_UNIFORM_PARAMETER_1_CFG_MODE1_pack(&job->indirect,
                  }
      /** Writes the V3D 4.x TMU configuration parameter 0. */
   static void
   write_tmu_p0(struct v3d_job *job,
               struct v3d_cl_out **uniforms,
   {
         int unit = v3d_unit_data_get_unit(data);
   struct pipe_sampler_view *psview = texstate->textures[unit];
   struct v3d_sampler_view *sview = v3d_sampler_view(psview);
   /* GL_OES_texture_buffer spec:
      *     "If no buffer object is bound to the buffer texture, the
   *      results of the texel access are undefined."
   *
   * This can be interpreted as allowing any result to come back, but
   * not terminate the program (and some tests interpret that).
   *
   * FIXME: just return is not a full valid solution, as it could still
   * try to get a wrong address for the shader state address. Perhaps we
   * would need to set up a BO with a "default texture state"
      if (sview == NULL)
                     cl_aligned_reloc(&job->indirect, uniforms, sview->bo,
         }
      static void
   write_image_tmu_p0(struct v3d_job *job,
                     {
         /* Extract the image unit from the top bits, and the compiler's
      * packed p0 from the bottom.
      uint32_t unit = data >> 24;
            struct v3d_image_view *iview = &img->si[unit];
            cl_aligned_reloc(&job->indirect, uniforms,
               }
      /** Writes the V3D 4.x TMU configuration parameter 1. */
   static void
   write_tmu_p1(struct v3d_job *job,
               struct v3d_cl_out **uniforms,
   {
         uint32_t unit = v3d_unit_data_get_unit(data);
   struct pipe_sampler_state *psampler = texstate->samplers[unit];
   struct v3d_sampler_state *sampler = v3d_sampler_state(psampler);
   struct pipe_sampler_view *psview = texstate->textures[unit];
   struct v3d_sampler_view *sview = v3d_sampler_view(psview);
            /* If we are being asked by the compiler to write parameter 1, then we
      * need that. So if we are at this point, we should expect to have a
   * sampler and psampler. As an additional assert, we can check that we
   * are not on a texel buffer case, as these don't have a sampler.
      assert(psview->target != PIPE_BUFFER);
   assert(sampler);
            if (sampler->border_color_variants)
            cl_aligned_reloc(&job->indirect, uniforms,
               }
      struct v3d_cl_reloc
   v3d_write_uniforms(struct v3d_context *v3d, struct v3d_job *job,
               {
         struct v3d_device_info *devinfo = &v3d->screen->devinfo;
   struct v3d_constbuf_stateobj *cb = &v3d->constbuf[stage];
   struct v3d_texture_stateobj *texstate = &v3d->tex[stage];
   struct v3d_uniform_list *uinfo = &shader->prog_data.base->uniforms;
            /* The hardware always pre-fetches the next uniform (also when there
      * aren't any), so we always allocate space for an extra slot. This
   * fixes MMU exceptions reported since Linux kernel 5.4 when the
   * uniforms fill up the tail bytes of a page in the indirect
   * BO. In that scenario, when the hardware pre-fetches after reading
   * the last uniform it will read beyond the end of the page and trigger
   * the MMU exception.
               struct v3d_cl_reloc uniform_stream = cl_get_address(&job->indirect);
            struct v3d_cl_out *uniforms =
            for (int i = 0; i < uinfo->count; i++) {
                     switch (uinfo->contents[i]) {
   case QUNIFORM_CONSTANT:
               case QUNIFORM_UNIFORM:
               case QUNIFORM_VIEWPORT_X_SCALE: {
            float clipper_xy_granularity = V3DV_X(devinfo, CLIPPER_XY_GRANULARITY);
      }
   case QUNIFORM_VIEWPORT_Y_SCALE: {
            float clipper_xy_granularity = V3DV_X(devinfo, CLIPPER_XY_GRANULARITY);
      }
   case QUNIFORM_VIEWPORT_Z_OFFSET:
                                    case QUNIFORM_USER_CLIP_PLANE:
                                                                  case QUNIFORM_IMAGE_TMU_CONFIG_P0:
                        case QUNIFORM_TEXTURE_CONFIG_P1:
                        case QUNIFORM_TEXRECT_SCALE_X:
   case QUNIFORM_TEXRECT_SCALE_Y:
            cl_aligned_u32(&uniforms,
                     case QUNIFORM_TEXTURE_WIDTH:
   case QUNIFORM_TEXTURE_HEIGHT:
   case QUNIFORM_TEXTURE_DEPTH:
   case QUNIFORM_TEXTURE_ARRAY_SIZE:
   case QUNIFORM_TEXTURE_LEVELS:
            cl_aligned_u32(&uniforms,
                     case QUNIFORM_IMAGE_WIDTH:
   case QUNIFORM_IMAGE_HEIGHT:
   case QUNIFORM_IMAGE_DEPTH:
   case QUNIFORM_IMAGE_ARRAY_SIZE:
            cl_aligned_u32(&uniforms,
                     case QUNIFORM_LINE_WIDTH:
                                             case QUNIFORM_UBO_ADDR: {
            uint32_t unit = v3d_unit_data_get_unit(data);
   /* Constant buffer 0 may be a system memory pointer,
   * in which case we want to upload a shadow copy to
   * the GPU.
   */
   if (!cb->cb[unit].buffer) {
         u_upload_data(v3d->uploader, 0,
                              cl_aligned_reloc(&job->indirect, &uniforms,
                                                         cl_aligned_reloc(&job->indirect, &uniforms,
                     case QUNIFORM_GET_SSBO_SIZE:
                        case QUNIFORM_TEXTURE_FIRST_LEVEL:
                        case QUNIFORM_SPILL_OFFSET:
                        case QUNIFORM_SPILL_SIZE_PER_THREAD:
                        case QUNIFORM_NUM_WORK_GROUPS:
                        case QUNIFORM_SHARED_OFFSET:
                                                                     write_texture_p0(job, &uniforms, texstate,
            #if 0
                  uint32_t written_val = *((uint32_t *)uniforms - 1);
   fprintf(stderr, "shader %p[%d]: 0x%08x / 0x%08x (%f) ",
            #endif
                           }
      void
   v3d_set_shader_uniform_dirty_flags(struct v3d_compiled_shader *shader)
   {
                  for (int i = 0; i < shader->prog_data.base->uniforms.count; i++) {
            switch (shader->prog_data.base->uniforms.contents[i]) {
   case QUNIFORM_CONSTANT:
         case QUNIFORM_UNIFORM:
                        case QUNIFORM_VIEWPORT_X_SCALE:
   case QUNIFORM_VIEWPORT_Y_SCALE:
   case QUNIFORM_VIEWPORT_Z_OFFSET:
                                             case QUNIFORM_TMU_CONFIG_P0:
   case QUNIFORM_TMU_CONFIG_P1:
   case QUNIFORM_TEXTURE_CONFIG_P1:
   case QUNIFORM_TEXTURE_FIRST_LEVEL:
   case QUNIFORM_TEXRECT_SCALE_X:
   case QUNIFORM_TEXRECT_SCALE_Y:
   case QUNIFORM_TEXTURE_WIDTH:
   case QUNIFORM_TEXTURE_HEIGHT:
   case QUNIFORM_TEXTURE_DEPTH:
   case QUNIFORM_TEXTURE_ARRAY_SIZE:
   case QUNIFORM_TEXTURE_LEVELS:
   case QUNIFORM_SPILL_OFFSET:
   case QUNIFORM_SPILL_SIZE_PER_THREAD:
            /* We could flag this on just the stage we're
   * compiling for, but it's not passed in.
                     case QUNIFORM_SSBO_OFFSET:
                        case QUNIFORM_IMAGE_TMU_CONFIG_P0:
   case QUNIFORM_IMAGE_WIDTH:
   case QUNIFORM_IMAGE_HEIGHT:
   case QUNIFORM_IMAGE_DEPTH:
                        case QUNIFORM_LINE_WIDTH:
                        case QUNIFORM_NUM_WORK_GROUPS:
                                             default:
            assert(quniform_contents_is_texture_p0(shader->prog_data.base->uniforms.contents[i]));
   dirty |= V3D_DIRTY_FRAGTEX | V3D_DIRTY_VERTTEX |
            }
