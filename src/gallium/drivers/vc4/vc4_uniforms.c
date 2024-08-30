   /*
   * Copyright © 2014-2015 Broadcom
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
   #include "util/format_srgb.h"
      #include "vc4_context.h"
   #include "vc4_qir.h"
      static void
   write_texture_p0(struct vc4_job *job,
                     {
         struct vc4_sampler_view *sview =
                  }
      static void
   write_texture_p1(struct vc4_job *job,
                     {
         struct vc4_sampler_view *sview =
         struct vc4_sampler_state *sampler =
            }
      static void
   write_texture_p2(struct vc4_job *job,
                     {
         uint32_t unit = data & 0xffff;
   struct pipe_sampler_view *texture = texstate->textures[unit];
            cl_aligned_u32(uniforms,
            VC4_SET_FIELD(VC4_TEX_P2_PTYPE_CUBE_MAP_STRIDE,
      }
      static void
   write_texture_first_level(struct vc4_job *job,
                     {
         uint32_t unit = data & 0xffff;
            }
      static void
   write_texture_msaa_addr(struct vc4_job *job,
                     {
         struct pipe_sampler_view *texture = texstate->textures[unit];
            }
         #define SWIZ(x,y,z,w) {          \
         PIPE_SWIZZLE_##x, \
   PIPE_SWIZZLE_##y, \
   PIPE_SWIZZLE_##z, \
   }
      static void
   write_texture_border_color(struct vc4_job *job,
                     {
         struct pipe_sampler_state *sampler = texstate->samplers[unit];
   struct pipe_sampler_view *texture = texstate->textures[unit];
   struct vc4_resource *rsc = vc4_resource(texture->texture);
            const struct util_format_description *tex_format_desc =
            float border_color[4];
   for (int i = 0; i < 4; i++)
         if (util_format_is_srgb(texture->format)) {
            for (int i = 0; i < 3; i++)
               /* Turn the border color into the layout of channels that it would
      * have when stored as texture contents.
      float storage_color[4];
   util_format_unswizzle_4f(storage_color,
                  /* Now, pack so that when the vc4_format-sampled texture contents are
      * replaced with our border color, the vc4_get_format_swizzle()
   * swizzling will get the right channels.
      if (util_format_is_depth_or_stencil(texture->format)) {
               } else {
            switch (rsc->vc4_format) {
   default:
   case VC4_TEXTURE_TYPE_RGBA8888:
            util_pack_color(storage_color,
      case VC4_TEXTURE_TYPE_RGBA4444:
   case VC4_TEXTURE_TYPE_RGBA5551:
            util_pack_color(storage_color,
      case VC4_TEXTURE_TYPE_RGB565:
            util_pack_color(storage_color,
      case VC4_TEXTURE_TYPE_ALPHA:
               case VC4_TEXTURE_TYPE_LUMALPHA:
            uc.ui[0] = ((float_to_ubyte(storage_color[1]) << 24) |
            }
      static uint32_t
   get_texrect_scale(struct vc4_texture_stateobj *texstate,
               {
         struct pipe_sampler_view *texture = texstate->textures[data];
            if (contents == QUNIFORM_TEXRECT_SCALE_X)
         else
            }
      void
   vc4_write_uniforms(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
               {
         struct vc4_shader_uniform_info *uinfo = &shader->uniforms;
   struct vc4_job *job = vc4->job;
            cl_ensure_space(&job->uniforms, (uinfo->count +
            struct vc4_cl_out *uniforms =
                  for (int i = 0; i < uinfo->count; i++) {
                           switch (contents) {
   case QUNIFORM_CONSTANT:
               case QUNIFORM_UNIFORM:
            cl_aligned_u32(&uniforms,
      case QUNIFORM_VIEWPORT_X_SCALE:
                                    case QUNIFORM_VIEWPORT_Z_OFFSET:
                                    case QUNIFORM_USER_CLIP_PLANE:
                                                                                       case QUNIFORM_TEXTURE_FIRST_LEVEL:
                        case QUNIFORM_UBO0_ADDR:
            /* Constant buffer 0 may be a system memory pointer,
   * in which case we want to upload a shadow copy to
   * the GPU.
   */
   if (!cb->cb[0].buffer) {
         u_upload_data(vc4->uploader, 0,
                              cl_aligned_reloc(job, &job->uniforms,
                                                         cl_aligned_reloc(job, &job->uniforms,
                                          case QUNIFORM_TEXTURE_BORDER_COLOR:
                        case QUNIFORM_TEXRECT_SCALE_X:
   case QUNIFORM_TEXRECT_SCALE_Y:
            cl_aligned_u32(&uniforms,
                     case QUNIFORM_BLEND_CONST_COLOR_X:
   case QUNIFORM_BLEND_CONST_COLOR_Y:
   case QUNIFORM_BLEND_CONST_COLOR_Z:
   case QUNIFORM_BLEND_CONST_COLOR_W:
            cl_aligned_f(&uniforms,
                     case QUNIFORM_BLEND_CONST_COLOR_RGBA: {
            const uint8_t *format_swiz =
                                          color |= (vc4->blend_color.ub[format_swiz[i]] <<
                     case QUNIFORM_BLEND_CONST_COLOR_AAAA: {
            uint8_t a = vc4->blend_color.ub[3];
   cl_aligned_u32(&uniforms, ((a) |
                           case QUNIFORM_STENCIL:
            cl_aligned_u32(&uniforms,
                                                case QUNIFORM_UNIFORMS_ADDRESS:
                              if (false) {
                                                         }
      void
   vc4_set_shader_uniform_dirty_flags(struct vc4_compiled_shader *shader)
   {
                  for (int i = 0; i < shader->uniforms.count; i++) {
            switch (shader->uniforms.contents[i]) {
   case QUNIFORM_CONSTANT:
   case QUNIFORM_UNIFORMS_ADDRESS:
         case QUNIFORM_UNIFORM:
   case QUNIFORM_UBO0_ADDR:
                        case QUNIFORM_VIEWPORT_X_SCALE:
   case QUNIFORM_VIEWPORT_Y_SCALE:
   case QUNIFORM_VIEWPORT_Z_OFFSET:
                                             case QUNIFORM_TEXTURE_CONFIG_P0:
   case QUNIFORM_TEXTURE_CONFIG_P1:
   case QUNIFORM_TEXTURE_CONFIG_P2:
   case QUNIFORM_TEXTURE_BORDER_COLOR:
   case QUNIFORM_TEXTURE_FIRST_LEVEL:
   case QUNIFORM_TEXTURE_MSAA_ADDR:
   case QUNIFORM_TEXRECT_SCALE_X:
   case QUNIFORM_TEXRECT_SCALE_Y:
            /* We could flag this on just the stage we're
                     case QUNIFORM_BLEND_CONST_COLOR_X:
   case QUNIFORM_BLEND_CONST_COLOR_Y:
   case QUNIFORM_BLEND_CONST_COLOR_Z:
   case QUNIFORM_BLEND_CONST_COLOR_W:
   case QUNIFORM_BLEND_CONST_COLOR_RGBA:
                                             case QUNIFORM_SAMPLE_MASK:
                     }
