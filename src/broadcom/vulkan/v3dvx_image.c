   /*
   * Copyright © 2021 Raspberry Pi Ltd
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
      #include "v3dv_private.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/cle/v3dx_pack.h"
   #include "broadcom/compiler/v3d_compiler.h"
      /*
   * Packs and ensure bo for the shader state (the latter can be temporal).
   */
   static void
   pack_texture_shader_state_helper(struct v3dv_device *device,
               {
      assert(!for_cube_map_array_storage ||
                  assert(image_view->vk.image);
            assert(image->vk.samples == VK_SAMPLE_COUNT_1_BIT ||
                  for (uint8_t plane = 0; plane < image_view->plane_count; plane++) {
      uint8_t iplane = image_view->planes[plane].image_plane;
               tex.level_0_is_strictly_uif =
                                          /* FIXME: v3d never sets uif_xor_disable, but uses it on the following
   * check so let's set the default value
   */
   tex.uif_xor_disable = false;
   if (tex.uif_xor_disable ||
      tex.level_0_is_strictly_uif) {
               tex.base_level = image_view->vk.base_mip_level;
                  tex.swizzle_r = v3d_translate_pipe_swizzle(image_view->planes[plane].swizzle[0]);
   tex.swizzle_g = v3d_translate_pipe_swizzle(image_view->planes[plane].swizzle[1]);
                           if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
         } else {
                  /* Empirical testing with CTS shows that when we are sampling from cube
   * arrays we want to set image depth to layers / 6, but not when doing
   * image load/store.
   */
   if (image_view->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY &&
      !for_cube_map_array_storage) {
   assert(tex.image_depth % 6 == 0);
                              /* On 4.x, the height of a 1D texture is redefined to be the
   * upper 14 bits of the width (which is only usable with txf).
   */
                                          /* At this point we don't have the job. That's the reason the first
   * parameter is NULL, to avoid a crash when cl_pack_emit_reloc tries to
   * add the bo to the job. This also means that we need to add manually
   * the image bo to the job using the texture.
   */
   const uint32_t base_offset =
      image->planes[iplane].mem->bo->offset +
   v3dv_layer_offset(image, 0, image_view->vk.base_array_layer,
                        /* V3D 4.x doesn't have the reverse and swap_r/b bits, so we compose
   * the reverse and/or swap_r/b swizzle from the format table with the
   * image view swizzle. This, however, doesn't work for border colors,
   * for that there is the reverse_standard_border_color.
   *
   * In v3d 7.x, however, there is no reverse_standard_border_color bit,
   * since the reverse and swap_r/b bits also affect border colors. It is
   * because of this that we absolutely need to use these bits with
   * reversed and swpaped formats, since that's the only way to ensure
   * correct border colors. In that case we don't want to program the
   * swizzle to the composition of the format swizzle and the view
   * swizzle like we do in v3d 4.x, since the format swizzle is applied
      #if V3D_VERSION == 42
            tex.srgb = is_srgb;
      #endif
   #if V3D_VERSION >= 71
                                    if (tex.reverse || tex.r_b_swap) {
      tex.swizzle_r =
         tex.swizzle_g =
         tex.swizzle_b =
         tex.swizzle_a =
               tex.chroma_offset_x = 1;
   tex.chroma_offset_y = 1;
   /* See comment in XML field definition for rationale of the shifts */
      #endif
               }
      void
   v3dX(pack_texture_shader_state)(struct v3dv_device *device,
         {
      pack_texture_shader_state_helper(device, iview, false);
   if (iview->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
      }
      void
   v3dX(pack_texture_shader_state_from_buffer_view)(struct v3dv_device *device,
         {
      assert(buffer_view->buffer);
            v3dvx_pack(buffer_view->texture_shader_state, TEXTURE_SHADER_STATE, tex) {
      tex.swizzle_r =
         tex.swizzle_g =
         tex.swizzle_b =
         tex.swizzle_a =
                     /* On 4.x, the height of a 1D texture is redefined to be the upper 14
   * bits of the width (which is only usable with txf) (or in other words,
   * we are providing a 28 bit field for size, but split on the usual
   * 14bit height/width).
   */
   tex.image_width = buffer_view->num_elements;
   tex.image_height = tex.image_width >> 14;
   tex.image_width &= (1 << 14) - 1;
            assert(buffer_view->format->plane_count == 1);
            #if V3D_VERSION == 42
         #endif
   #if V3D_VERSION >= 71
         #endif
            /* At this point we don't have the job. That's the reason the first
   * parameter is NULL, to avoid a crash when cl_pack_emit_reloc tries to
   * add the bo to the job. This also means that we need to add manually
   * the image bo to the job using the texture.
   */
   const uint32_t base_offset =
      buffer->mem->bo->offset +
                  #if V3D_VERSION >= 71
         tex.chroma_offset_x = 1;
   tex.chroma_offset_y = 1;
   /* See comment in XML field definition for rationale of the shifts */
   tex.texture_base_pointer_cb = base_offset >> 6;
   #endif
         }
