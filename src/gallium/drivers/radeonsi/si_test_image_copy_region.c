   /*
   * Copyright 2016 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /* This file implements randomized texture blit tests. */
      #include "si_pipe.h"
   #include "util/rand_xor.h"
   #include "util/u_surface.h"
   #include "amd/addrlib/inc/addrtypes.h"
      static uint64_t seed_xorshift128plus[2];
      #define RAND_NUM_SIZE 8
      /* The GPU blits are emulated on the CPU using these CPU textures. */
      struct cpu_texture {
      uint8_t *ptr;
   uint64_t size;
   uint64_t layer_stride;
      };
      static void alloc_cpu_texture(struct cpu_texture *tex, struct pipe_resource *templ, unsigned level)
   {
      unsigned width = u_minify(templ->width0, level);
            tex->stride = align(util_format_get_stride(templ->format, width), RAND_NUM_SIZE);
   tex->layer_stride = util_format_get_2d_size(templ->format, tex->stride, height);
   tex->size = tex->layer_stride * util_num_layers(templ, level);
   tex->ptr = malloc(tex->size);
      }
      static void set_random_pixels(struct pipe_context *ctx, struct pipe_resource *tex,
         {
      struct pipe_transfer *t;
   uint8_t *map;
   int x, y, z;
   unsigned width = u_minify(tex->width0, level);
   unsigned height = u_minify(tex->height0, level);
   unsigned num_y_blocks = util_format_get_nblocksy(tex->format, height);
            map = pipe_texture_map_3d(ctx, tex, level, PIPE_MAP_WRITE, 0, 0, 0, width, height,
                  for (z = 0; z < num_layers; z++) {
      for (y = 0; y < num_y_blocks; y++) {
      uint64_t *ptr = (uint64_t *)(map + t->layer_stride * z + t->stride * y);
                                 for (x = 0; x < size; x++) {
                           }
      static void set_random_pixels_for_2_textures(struct pipe_context *ctx, struct pipe_resource *tex1,
         {
      /* tex1 and tex2 are assumed to be the same size, format, and layout */
   for (unsigned level = 0; level <= tex1->last_level; level++) {
      for (unsigned sample = 0; sample < MAX2(tex1->nr_samples, 1); sample++) {
      struct pipe_transfer *t1, *t2;
   uint8_t *map1, *map2;
   int x, y, z;
   unsigned width = align(u_minify(tex1->width0, level), util_format_get_blockwidth(tex1->format));
   unsigned height = align(u_minify(tex1->height0, level), util_format_get_blockheight(tex1->format));
   unsigned num_y_blocks = util_format_get_nblocksy(tex1->format, height);
   unsigned num_layers = util_num_layers(tex1, level);
   /* If we set level to sample + 1, we will only upload that sample instead of
   * overwriting all samples.
                  map1 = pipe_texture_map_3d(ctx, tex1, level_or_sample, PIPE_MAP_WRITE, 0, 0, 0, width, height,
         map2 = pipe_texture_map_3d(ctx, tex2, level_or_sample, PIPE_MAP_WRITE, 0, 0, 0, width, height,
                        for (z = 0; z < num_layers; z++) {
      for (y = 0; y < num_y_blocks; y++) {
                                          for (x = 0; x < size; x++) {
                        pipe_texture_unmap(ctx, t1);
            }
      static bool compare_textures(struct pipe_context *ctx, struct pipe_resource *tex,
         {
      struct pipe_transfer *t;
   uint8_t *map;
   int y, z;
   bool pass = true;
   unsigned width = u_minify(tex->width0, level);
   unsigned height = u_minify(tex->height0, level);
   unsigned stride = util_format_get_stride(tex->format, width);
   unsigned num_y_blocks = util_format_get_nblocksy(tex->format, height);
            map = pipe_texture_map_3d(ctx, tex, level, PIPE_MAP_READ, 0, 0, 0, width, height,
                  for (z = 0; z < num_layers; z++) {
      for (y = 0; y < num_y_blocks; y++) {
                     if (memcmp(ptr, cpu_ptr, stride)) {
      pass = false;
               done:
      pipe_texture_unmap(ctx, t);
      }
      static bool compare_gpu_textures(struct pipe_context *ctx, struct pipe_resource *tex1,
         {
      /* tex1 and tex2 are assumed to be the same size, format, and layout */
   for (unsigned level = 0; level <= tex1->last_level; level++) {
      struct pipe_transfer *t1, *t2;
   uint8_t *map1, *map2;
   unsigned width = u_minify(tex1->width0, level);
   unsigned height = u_minify(tex1->height0, level);
   unsigned stride = util_format_get_stride(tex1->format, width);
   unsigned num_y_blocks = util_format_get_nblocksy(tex1->format, height);
            map1 = pipe_texture_map_3d(ctx, tex1, level, PIPE_MAP_READ, 0, 0, 0, width, height,
         map2 = pipe_texture_map_3d(ctx, tex2, level, PIPE_MAP_READ, 0, 0, 0, width, height,
         assert(map1 && map2);
            for (unsigned z = 0; z < num_layers; z++) {
      for (unsigned y = 0; y < num_y_blocks; y++) {
                                    if (memcmp(ptr1, ptr2, stride)) {
      pipe_texture_unmap(ctx, t1);
   pipe_texture_unmap(ctx, t2);
                     pipe_texture_unmap(ctx, t1);
                  }
      struct si_format_options {
      bool only_resolve;
   bool allow_float;
   bool allow_unorm16;
   bool allow_srgb;
   bool allow_x_channels;
   bool allow_subsampled;
      };
      static enum pipe_format get_random_format(struct si_screen *sscreen, bool render_target,
                           {
      /* Depth/stencil formats can only select Z/S using the blit mask, not via the view format. */
   if (res_format != PIPE_FORMAT_NONE && util_format_is_depth_or_stencil(res_format))
            /* Keep generating formats until we get a supported one. */
   while (1) {
      /* Skip one format: PIPE_FORMAT_NONE */
   enum pipe_format format = (rand() % (PIPE_FORMAT_COUNT - 1)) + 1;
            if (desc->colorspace == UTIL_FORMAT_COLORSPACE_YUV ||
                  if (!options->allow_srgb && desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
            if (!options->allow_subsampled && desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
            if (!options->allow_compressed && util_format_get_blockwidth(format) >= 4)
            if (color_or_zs != PIPE_FORMAT_NONE &&
      (util_format_is_depth_or_stencil(color_or_zs) !=
               if (desc->layout == UTIL_FORMAT_LAYOUT_PLAIN) {
      if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      /* Don't select stencil-only formats - we don't support them for rendering. */
   if (util_format_has_stencil(desc) && !util_format_has_depth(desc))
                                 /* Don't test formats with X channels because cpu_texture doesn't emulate them. */
   for (i = 0; i < desc->nr_channels; i++) {
      if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID)
      }
   if (i != desc->nr_channels)
                  if (res_format != PIPE_FORMAT_NONE) {
      /* If the resource format is Z/S, we handle it at the beginning of this function,
   * so here res_format can only be a color format.
   */
                  if (util_format_get_blocksize(res_format) != util_format_get_blocksize(format) ||
      util_format_get_blockwidth(res_format) != util_format_get_blockwidth(format) ||
   util_format_get_blockheight(res_format) != util_format_get_blockheight(format))
            if (integer_or_not != PIPE_FORMAT_NONE) {
      /* The integer property must match between blit src/dst. */
   if (util_format_is_pure_integer(integer_or_not) != util_format_is_pure_integer(format))
               if (options->only_resolve &&
                  if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS) {
      /* Every integer format should have an equivalent non-integer format, but 128-bit integer
   * formats don't have that if floats are disallowed, which can cause an infinite loop later
   * if compat_type is non-integer.
   */
   if (!options->allow_float &&
                  if (!options->allow_unorm16 &&
      desc->channel[0].size == 16 && desc->channel[0].normalized &&
   desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED)
            unsigned bind = PIPE_BIND_SAMPLER_VIEW;
   if (render_target) {
      if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS)
         else
               if (sscreen->b.is_format_supported(&sscreen->b, format, PIPE_TEXTURE_2D, 1, 1, bind))
         }
      #define MAX_ALLOC_SIZE (64 * 1024 * 1024)
      static void set_random_image_attrs(struct pipe_resource *templ, bool allow_msaa,
         {
               if (only_cb_resolve) {
         } else {
                  switch (target_index) {
   case 0:
      templ->target = PIPE_TEXTURE_1D;
      case 1:
      templ->target = PIPE_TEXTURE_2D;
      case 2:
      if (util_format_is_depth_or_stencil(templ->format))
         else
            case 3:
      templ->target = PIPE_TEXTURE_RECT;
      case 4:
      templ->target = PIPE_TEXTURE_1D_ARRAY;
      case 5:
      templ->target = PIPE_TEXTURE_2D_ARRAY;
      case 6:
      templ->target = PIPE_TEXTURE_2D;
   templ->nr_samples = 2 << (rand() % 3);
      case 7:
      templ->target = PIPE_TEXTURE_2D_ARRAY;
   templ->nr_samples = 2 << (rand() % 3);
      default:
                           templ->height0 = 1;
   templ->depth0 = 1;
   templ->array_size = 1;
            /* Try to hit microtiling in 1/2 of the cases. */
                     if (templ->target != PIPE_TEXTURE_1D &&
      templ->target != PIPE_TEXTURE_1D_ARRAY)
         if (templ->target == PIPE_TEXTURE_3D)
            if (templ->target == PIPE_TEXTURE_1D_ARRAY ||
      templ->target == PIPE_TEXTURE_2D_ARRAY)
         /* Keep reducing the size until it we get a small enough size. */
   while (util_format_get_nblocks(templ->format, templ->width0, templ->height0) *
         templ->depth0 * templ->array_size * util_format_get_blocksize(templ->format) >
      switch (rand() % 3) {
   case 0:
      if (templ->width0 > 1)
            case 1:
      if (templ->height0 > 1)
            case 2:
      if (templ->depth0 > 1)
         else if (templ->array_size > 1)
                        if (util_format_get_blockwidth(templ->format) == 2)
            if (templ->target != PIPE_TEXTURE_RECT &&
      util_format_description(templ->format)->layout != UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
            if (templ->nr_samples <= 1)
         }
      static void print_image_attrs(struct si_screen *sscreen, struct si_texture *tex)
   {
               if (sscreen->info.gfx_level >= GFX9) {
      static const char *modes[32] = {
      [ADDR_SW_LINEAR] = "LINEAR",
   [ADDR_SW_4KB_S_X] = "4KB_S_X",
   [ADDR_SW_4KB_D_X] = "4KB_D_X",
   [ADDR_SW_64KB_Z_X] = "64KB_Z_X",
   [ADDR_SW_64KB_S_X] = "64KB_S_X",
   [ADDR_SW_64KB_D_X] = "64KB_D_X",
      };
      } else {
      static const char *modes[32] = {
      [RADEON_SURF_MODE_LINEAR_ALIGNED] = "LINEAR",
   [RADEON_SURF_MODE_1D] = "1D_TILED",
      };
               if (!mode)
            static const char *targets[PIPE_MAX_TEXTURE_TYPES] = {
      [PIPE_TEXTURE_1D] = "1D",
   [PIPE_TEXTURE_2D] = "2D",
   [PIPE_TEXTURE_3D] = "3D",
   [PIPE_TEXTURE_RECT] = "RECT",
   [PIPE_TEXTURE_1D_ARRAY] = "1D_ARRAY",
               char size[64];
   if (tex->buffer.b.b.target == PIPE_TEXTURE_1D)
         else if (tex->buffer.b.b.target == PIPE_TEXTURE_2D ||
               else
      snprintf(size, sizeof(size), "%ux%ux%u", tex->buffer.b.b.width0, tex->buffer.b.b.height0,
         printf("%8s, %14s, %2u %7s, %8s", targets[tex->buffer.b.b.target], size,
            }
      void si_test_image_copy_region(struct si_screen *sscreen)
   {
      struct pipe_screen *screen = &sscreen->b;
   struct pipe_context *ctx = screen->context_create(screen, NULL, 0);
   struct si_context *sctx = (struct si_context *)ctx;
   unsigned i, iterations, num_partial_copies;
            /* the seed for random test parameters */
   srand(0x9b47d95b);
   /* the seed for random pixel data */
            iterations = 1000000000; /* just kill it when you are bored */
            /* These parameters are randomly generated per test:
   * - which texture dimensions to use
   * - random initial pixels in src
   * - execute multiple subrectangle copies for partial blits
   */
   for (i = 0; i < iterations; i++) {
      struct pipe_resource tsrc = {}, tdst = {}, *src, *dst;
   struct si_texture *sdst;
   struct si_texture *ssrc;
   struct cpu_texture src_cpu[RADEON_SURF_MAX_LEVELS], dst_cpu[RADEON_SURF_MAX_LEVELS];
   unsigned max_width, max_height, max_depth, j;
   unsigned gfx_blits = 0, cs_blits = 0;
            /* generate a random test case */
   struct si_format_options format_options = {
      .only_resolve = false,
   .allow_float = true,
   .allow_unorm16 = true,
   .allow_x_channels = false, /* cpu_texture doesn't implement X channels */
   .allow_subsampled = false, /* TODO: fix subsampled formats */
                        /* MSAA copy testing not implemented and might be too difficult because of how
   * cpu_texture works.
   */
   set_random_image_attrs(&tsrc, false, false);
            /* Allocate textures (both the GPU and CPU copies).
   * The CPU will emulate what the GPU should be doing.
   */
   src = screen->resource_create(screen, &tsrc);
   dst = screen->resource_create(screen, &tdst);
   assert(src);
   assert(dst);
   sdst = (struct si_texture *)dst;
            printf("%4u: dst = (", i);
   print_image_attrs(sscreen, sdst);
   printf("), src = (");
   print_image_attrs(sscreen, ssrc);
   printf("), format = %20s, ", util_format_description(tsrc.format)->short_name);
            for (unsigned level = 0; level <= tsrc.last_level; level++) {
      alloc_cpu_texture(&src_cpu[level], &tsrc, level);
      }
   for (unsigned level = 0; level <= tdst.last_level; level++) {
      alloc_cpu_texture(&dst_cpu[level], &tdst, level);
               /* clear dst pixels */
   uint32_t zero = 0;
   si_clear_buffer(sctx, dst, 0, sdst->surface.surf_size, &zero, 4, SI_OP_SYNC_BEFORE_AFTER,
            for (j = 0; j < num_partial_copies; j++) {
      int width, height, depth;
   int srcx, srcy, srcz, dstx, dsty, dstz;
   struct pipe_box box;
                                 max_width = MIN2(u_minify(tsrc.width0, src_level), u_minify(tdst.width0, dst_level));
                  /* random sub-rectangle copies from src to dst */
   depth = (rand() % max_depth) + 1;
                                                                              /* Align the box to the format block size. */
                                                /* GPU copy */
                  /* See which engine was used. */
                  /* CPU copy */
   util_copy_box(dst_cpu[dst_level].ptr, tdst.format, dst_cpu[dst_level].stride,
                           pass = true;
   for (unsigned level = 0; level <= tdst.last_level; level++)
            if (pass)
         else
            printf("BLITs: GFX = %2u, CS = %2u, %s [%u/%u]\n", gfx_blits, cs_blits,
            /* cleanup */
   pipe_resource_reference(&src, NULL);
   pipe_resource_reference(&dst, NULL);
   for (unsigned level = 0; level <= tsrc.last_level; level++)
         for (unsigned level = 0; level <= tdst.last_level; level++)
               ctx->destroy(ctx);
      }
      void si_test_blit(struct si_screen *sscreen, unsigned test_flags)
   {
      struct pipe_screen *screen = &sscreen->b;
   struct pipe_context *ctx = screen->context_create(screen, NULL, 0);
   struct si_context *sctx = (struct si_context *)ctx;
   unsigned iterations;
   unsigned num_pass = 0, num_fail = 0;
            bool allow_float = false;
   bool allow_unorm16_dst = false;
   bool allow_srgb_dst = false;
   bool allow_filter = false;
   bool allow_scaled_min = false;
   bool allow_scaled_mag = false;
   bool allow_out_of_bounds_dst = false;
   bool allow_out_of_bounds_src = false;
   bool allow_scissor = false;
            /* The following tests always compare the tested operation with the gfx blit (u_blitter). */
   switch (test_flags) {
   case DBG(TEST_CB_RESOLVE):
      /* This is mostly failing because the precision of CB_RESOLVE is very different
   * from the gfx blit. FP32 and FP16 are the only formats that mostly pass.
   */
   allow_float = true;
   allow_unorm16_dst = true;
   allow_srgb_dst = true;
         case DBG(TEST_COMPUTE_BLIT):
      //allow_float = true;      /* precision difference: NaNs not preserved by CB (u_blitter) */
   allow_unorm16_dst = true;
   //allow_srgb_dst = true;   /* precision difference: sRGB is less precise in CB (u_blitter) */
   //allow_filter = true;     /* not implemented by compute blits, lots of precision differences */
   //allow_scaled_min = true; /* not implemented by compute blits, lots of precision differences */
   //allow_scaled_mag = true; /* not implemented by compute blits, lots of precision differences */
   allow_out_of_bounds_dst = true;
   allow_out_of_bounds_src = true;
   //allow_scissor = true;    /* not implemented by compute blits */
   allow_flip = true;
         default:
                  /* the seed for random test parameters */
   srand(0x9b47d95b);
   /* the seed for random pixel data */
                     /* These parameters are randomly generated per test:
   * - which texture dimensions to use
   * - random initial pixels in src
   * - random pipe_blit_info
   */
   for (unsigned i = 0; i < iterations; i++) {
               /* Generate a random test case. */
   {
      struct si_format_options format_options = {
      .only_resolve = only_cb_resolve,
   .allow_float = allow_float,
   .allow_unorm16 = true,
   .allow_srgb = true,
   .allow_x_channels = true,
   .allow_subsampled = false, /* TODO: fix subsampled formats */
               tsrc.format = get_random_format(sscreen, false, 0, 0, 0, &format_options);
               set_random_image_attrs(&tsrc, true, only_cb_resolve);
            /* MSAA blits must have matching sample counts. */
   if (tsrc.nr_samples > 1 && tdst.nr_samples > 1)
            /* Allocate textures. */
   gfx_src = screen->resource_create(screen, &tsrc);
   gfx_dst = screen->resource_create(screen, &tdst);
   comp_src = screen->resource_create(screen, &tsrc);
            /* clear dst pixels */
   uint32_t zero = 0;
   si_clear_buffer(sctx, gfx_dst, 0, ((struct si_texture *)gfx_dst)->surface.surf_size, &zero,
         si_clear_buffer(sctx, comp_dst, 0, ((struct si_texture *)comp_dst)->surface.surf_size, &zero,
            /* TODO: These two fix quite a lot of BCn cases. */
   /*si_clear_buffer(sctx, gfx_src, 0, ((struct si_texture *)gfx_src)->surface.surf_size, &zero,
         si_clear_buffer(sctx, comp_src, 0, ((struct si_texture *)comp_src)->surface.surf_size, &zero,
                     struct pipe_blit_info info;
            {
      struct si_format_options format_options = {
      .only_resolve = only_cb_resolve,
   .allow_float = allow_float,
   .allow_unorm16 = true,
   .allow_srgb = true,
   .allow_x_channels = true,
   .allow_subsampled = false, /* TODO: fix subsampled formats */
               info.src.format = get_random_format(sscreen, false, 0, tsrc.format, 0, &format_options);
   format_options.allow_unorm16 = allow_unorm16_dst;
   format_options.allow_srgb = allow_srgb_dst;
   info.dst.format = get_random_format(sscreen, true, 0, tdst.format, info.src.format,
               printf("%4u: dst = (", i);
   print_image_attrs(sscreen, (struct si_texture *)gfx_dst);
   printf(", %20s), src = (", util_format_short_name(info.dst.format));
   print_image_attrs(sscreen, (struct si_texture *)gfx_src);
   printf(", %20s)", util_format_short_name(info.src.format));
            int src_width, src_height, src_depth, dst_width, dst_height, dst_depth;
            unsigned src_level = rand() % (tsrc.last_level + 1);
            unsigned max_src_width = u_minify(tsrc.width0, src_level);
   unsigned max_src_height = u_minify(tsrc.height0, src_level);
            unsigned max_dst_width = u_minify(tdst.width0, dst_level);
   unsigned max_dst_height = u_minify(tdst.height0, dst_level);
            /* make sure that it doesn't divide by zero */
   assert(max_src_width && max_src_height && max_src_depth &&
            /* random sub-rectangle copies from src to dst */
   src_width = (rand() % max_src_width) + 1;
   src_height = (rand() % max_src_height) + 1;
            dst_width = (rand() % max_dst_width) + 1;
   dst_height = (rand() % max_dst_height) + 1;
            srcx = rand() % (u_minify(tsrc.width0, src_level) - src_width + 1);
   srcy = rand() % (u_minify(tsrc.height0, src_level) - src_height + 1);
            dstx = rand() % (u_minify(tdst.width0, dst_level) - dst_width + 1);
   dsty = rand() % (u_minify(tdst.height0, dst_level) - dst_height + 1);
            /* Test out-of-bounds boxes. Add -dim/10 .. +dim/10 */
   if (allow_out_of_bounds_src) {
      if (max_src_width / 5 >= 2)
         if (max_src_height / 5 >= 2)
               if (allow_out_of_bounds_dst) {
      if (max_dst_width / 5 >= 2)
         if (max_dst_height / 5 >= 2)
               /* Align the box to the format block size. */
   srcx &= ~(util_format_get_blockwidth(tsrc.format) - 1);
            dstx &= ~(util_format_get_blockwidth(tdst.format) - 1);
            src_width = align(src_width, util_format_get_blockwidth(tsrc.format));
            dst_width = align(dst_width, util_format_get_blockwidth(tdst.format));
            if (!allow_scaled_min) {
      if (src_width > dst_width)
         if (src_height > dst_height)
         if (src_depth > dst_depth)
               if (!allow_scaled_mag) {
      if (src_width < dst_width)
         if (src_height < dst_height)
         if (src_depth < dst_depth)
               /* Flips */
   if (allow_flip) {
      if (rand() % 2) {
      srcx += src_width;
      }
   if (rand() % 2) {
      srcy += src_height;
                  info.src.level = src_level;
            u_box_3d(srcx, srcy, srcz, src_width, src_height, src_depth, &info.src.box);
            if (util_format_is_depth_and_stencil(tsrc.format)) {
      switch (rand() % 3) {
   case 0:
      info.mask = PIPE_MASK_ZS;
      case 1:
      info.mask = PIPE_MASK_Z;
      case 2:
      info.mask = PIPE_MASK_S;
         } else {
      /* RGBA, Z, or S */
               /* Don't filter MSAA and integer sources. */
   if (allow_filter && tsrc.nr_samples <= 1 &&
      !util_format_is_pure_integer(info.src.format) && rand() % 2)
      else
                     if (info.scissor_enable) {
      info.scissor.minx = MAX2(MIN2(info.dst.box.x, info.dst.box.x + info.dst.box.width), 0);
   info.scissor.miny = MAX2(MIN2(info.dst.box.y, info.dst.box.y + info.dst.box.height), 0);
                  if (abs(info.dst.box.width) / 2 >= 2) {
      info.scissor.minx += rand() % (abs(info.dst.box.width) / 2);
      }
   if (abs(info.dst.box.height) / 2 >= 2) {
      info.scissor.miny += rand() % (abs(info.dst.box.height) / 2);
                           snprintf(dstbox_s, sizeof(dstbox_s), "{%ix%ix%i .. %ix%ix%i}",
               snprintf(srcbox_s, sizeof(srcbox_s), "{%ix%ix%i .. %ix%ix%i}",
               if (info.scissor_enable) {
      snprintf(scissor, sizeof(scissor), "(%u..%u, %u..%u)",
      } else {
                  printf(", filter %u, mask 0x%02x, ", info.filter, info.mask);
   printf("dst(level %u, box = %-28s), ", info.dst.level, dstbox_s);
   printf("src(level %u, box = %-28s), ", info.src.level, srcbox_s);
            /* Blits. */
   info.src.resource = gfx_src;
   info.dst.resource = gfx_dst;
            info.src.resource = comp_src;
            bool success;
   if (only_cb_resolve)
         else
            if (success) {
         } else {
      si_gfx_blit(ctx, &info);
               bool pass = compare_gpu_textures(ctx, gfx_dst, comp_dst);
   if (pass)
         else
                     /* cleanup */
   pipe_resource_reference(&gfx_src, NULL);
   pipe_resource_reference(&gfx_dst, NULL);
   pipe_resource_reference(&comp_src, NULL);
               ctx->destroy(ctx);
      }
