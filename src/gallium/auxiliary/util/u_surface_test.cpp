   /* SPDX-License-Identifier: MIT */
      #include "u_box.h"
   #include "u_surface.h"
   #include <gtest/gtest.h>
      struct pipe_resource
   test_resource_with_format(pipe_format f)
   {
      struct pipe_resource rsc;
            rsc.width0 = 1;
   rsc.height0 = 1;
   rsc.depth0 = 1;
               }
      static struct pipe_blit_info
   test_blit_with_formats(struct pipe_resource *src, pipe_format src_format,
         {
      struct pipe_blit_info info;
            info.dst.resource = dst;
            info.src.resource = src;
            info.mask = PIPE_MASK_RGBA;
   info.filter = PIPE_TEX_FILTER_NEAREST;
   info.scissor_enable = false;
            u_box_2d(0, 0, src->width0, src->height0, &info.src.box);
               }
      TEST(util_can_blit_via_copy_region, formats)
   {
      struct pipe_resource src_rgba8_unorm = test_resource_with_format(PIPE_FORMAT_R8G8B8A8_UNORM);
   struct pipe_resource dst_rgba8_unorm = test_resource_with_format(PIPE_FORMAT_R8G8B8A8_UNORM);
   struct pipe_resource src_rgbx8_unorm = test_resource_with_format(PIPE_FORMAT_R8G8B8X8_UNORM);
   struct pipe_resource dst_rgbx8_unorm = test_resource_with_format(PIPE_FORMAT_R8G8B8X8_UNORM);
            /* Same-format blit should pass */
   struct pipe_blit_info rgba8_unorm_rgba8_unorm_blit = test_blit_with_formats(&src_rgba8_unorm, PIPE_FORMAT_R8G8B8A8_UNORM,
                  /* Blit that should do sRGB encoding should fail. */
   struct pipe_blit_info rgba8_unorm_rgba8_srgb_blit = test_blit_with_formats(&src_rgba8_unorm, PIPE_FORMAT_R8G8B8A8_UNORM,
                  /* RGBA->RGBX is fine, since A is ignored. */
   struct pipe_blit_info rgba8_unorm_rgbx8_unorm_blit = test_blit_with_formats(&src_rgba8_unorm, PIPE_FORMAT_R8G8B8A8_UNORM,
                  /* RGBX->RGBA is invalid, since src A is undefined. */
   struct pipe_blit_info rgbx8_unorm_rgba8_unorm_blit = test_blit_with_formats(&src_rgbx8_unorm, PIPE_FORMAT_R8G8B8X8_UNORM,
                  /* If the RGBA8_UNORM resources are both viewed as sRGB, it's still a memcpy.  */
   struct pipe_blit_info rgba8_srgb_rgba8_srgb_blit = test_blit_with_formats(&src_rgba8_unorm, PIPE_FORMAT_R8G8B8A8_SRGB,
                  /* A memcpy blit can't be lowered to copy_region if copy_region would have mismatched resource formats. */
   struct pipe_blit_info non_memcpy_copy_region = test_blit_with_formats(&src_rgba8_unorm, PIPE_FORMAT_R8G8B8A8_UNORM,
            }
