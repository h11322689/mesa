   /* Make the test not meaningless when asserts are disabled. */
   #undef NDEBUG
      #include <assert.h>
   #include <inttypes.h>
   #include <stdio.h>
   #include <stdlib.h>
      #include <amdgpu.h>
   #include "drm-uapi/amdgpu_drm.h"
   #include "drm-uapi/drm_fourcc.h"
      #include "ac_surface.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "util/u_vector.h"
   #include "util/mesa-sha1.h"
   #include "addrlib/inc/addrinterface.h"
      #include "ac_surface_test_common.h"
      /*
   * The main goal of this test is making sure that we do
   * not change the meaning of existing modifiers.
   */
      struct test_entry {
      /* key part */
   uint64_t modifier;
   unsigned w;
   unsigned h;
            /* debug info */
   const char *name;
   uint8_t pipes;
   uint8_t rb;
   uint8_t banks_or_pkrs;
            /* value to determine uniqueness */
            /* u_vector requires power of two sizing */
      };
      static uint64_t
   block_count(unsigned w, unsigned h, unsigned elem_bits, unsigned block_bits,
         {
      unsigned align_bits = block_bits - elem_bits;
   unsigned w_align = 1 << (align_bits / 2 + align_bits % 2);
            w = align(w, w_align);
            if (aligned_pitch)
            if (aligned_height)
            }
         static ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT
   get_addr_from_coord_base(ADDR_HANDLE addrlib, const struct radeon_surf *surf,
               {
      ADDR2_COMPUTE_DCCINFO_INPUT din = {0};
   ADDR2_COMPUTE_DCCINFO_OUTPUT dout = {0};
   din.size = sizeof(ADDR2_COMPUTE_DCCINFO_INPUT);
            din.swizzleMode = surf->u.gfx9.swizzle_mode;
   din.resourceType = ADDR_RSRC_TEX_2D;
   din.bpp = util_format_get_blocksizebits(format);
   din.unalignedWidth = w;
   din.unalignedHeight = h;
   din.numSlices = 1;
   din.numMipLevels = 1;
   din.numFrags = 1;
   din.dccKeyFlags.pipeAligned = surf->u.gfx9.color.dcc.pipe_aligned;
   din.dccKeyFlags.rbAligned = surf->u.gfx9.color.dcc.rb_aligned;
            ADDR_E_RETURNCODE ret = Addr2ComputeDccInfo(addrlib, &din, &dout);
            ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT dcc_input = {0};
   dcc_input.size = sizeof(dcc_input);
   dcc_input.swizzleMode = surf->u.gfx9.swizzle_mode;
   dcc_input.resourceType = ADDR_RSRC_TEX_2D;
   dcc_input.bpp = din.bpp;
   dcc_input.numSlices = 1;
   dcc_input.numMipLevels = 1;
   dcc_input.numFrags = 1;
   dcc_input.dccKeyFlags.pipeAligned = pipe_aligned;
   dcc_input.dccKeyFlags.rbAligned = rb_aligned;
   dcc_input.pitch = dout.pitch;
   dcc_input.height = dout.height;
   dcc_input.compressBlkWidth = dout.compressBlkWidth;
   dcc_input.compressBlkHeight = dout.compressBlkHeight;
   dcc_input.compressBlkDepth = dout.compressBlkDepth;
   dcc_input.metaBlkWidth     = dout.metaBlkWidth;
   dcc_input.metaBlkHeight    = dout.metaBlkHeight;
   dcc_input.metaBlkDepth     = dout.metaBlkDepth;
      }
      static
   void generate_hash(struct ac_addrlib *ac_addrlib,
               {
               srandom(53);
   struct mesa_sha1 ctx;
            _mesa_sha1_update(&ctx, &surf->total_size, sizeof(surf->total_size));
   _mesa_sha1_update(&ctx, &surf->meta_offset, sizeof(surf->meta_offset));
   _mesa_sha1_update(&ctx, &surf->display_dcc_offset, sizeof(surf->display_dcc_offset));
   _mesa_sha1_update(&ctx, &surf->u.gfx9.color.display_dcc_pitch_max,
            ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT input = {0};
   input.size = sizeof(input);
   input.swizzleMode = surf->u.gfx9.swizzle_mode;
   input.resourceType = ADDR_RSRC_TEX_2D;
   input.bpp = util_format_get_blocksizebits(entry->format);
   input.unalignedWidth = entry->w;
   input.unalignedHeight = entry->h;
   input.numSlices = 1;
   input.numMipLevels = 1;
   input.numSamples = 1;
   input.numFrags = 1;
            ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT dcc_input = {0};
   if (surf->meta_offset) {
      dcc_input = get_addr_from_coord_base(addrlib, surf, entry->w,
                           ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT display_dcc_input = {0};
   if (surf->display_dcc_offset) {
      display_dcc_input = get_addr_from_coord_base(addrlib, surf, entry->w,
                     for (unsigned i = 0; i < 1000; ++i) {
      int32_t x, y;
   x = random();
            input.x = (x & INT_MAX) % entry->w;
            ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT output = {0};
            ADDR_E_RETURNCODE ret = Addr2ComputeSurfaceAddrFromCoord(addrlib, &input, &output);
                     if (surf->meta_offset) {
                                                               if (surf->display_dcc_offset) {
                                                                     }
      static void test_modifier(const struct radeon_info *info,
            const char *name,
                  {
      unsigned elem_bits = util_logbase2(util_format_get_blocksize(format));
   const unsigned dims[][2] = {
      {1, 1},
   {1920, 1080},
   {1366, 768},
   {3840, 2160},
      };
   for (unsigned i = 0; i < ARRAY_SIZE(dims); ++i) {
      struct ac_surf_config config = (struct ac_surf_config) {
      .info = (struct ac_surf_info) {
      .width = dims[i][0],
   .height = dims[i][1],
   .depth = 1,
   .samples = 1,
   .storage_samples = 1,
   .levels = 1,
   .num_channels = 3,
                  struct test_entry entry = {
      .modifier = modifier,
   .w = config.info.width,
   .h = config.info.height,
   .format = format,
   .name = name,
   .pipes = G_0098F8_NUM_PIPES(info->gb_addr_config),
   .rb = G_0098F8_NUM_RB_PER_SE(info->gb_addr_config) +
         .se = G_0098F8_NUM_SHADER_ENGINES_GFX9(info->gb_addr_config),
   .banks_or_pkrs = info->gfx_level >= GFX10 ?
               struct radeon_surf surf = (struct radeon_surf) {
      .blk_w = 1,
   .blk_h = 1,
   .bpe = util_format_get_blocksize(format),
               int r = ac_compute_surface(addrlib, info, &config, RADEON_SURF_MODE_2D, &surf);
            assert(surf.cmask_offset == 0);
                     uint64_t surf_size;
   unsigned aligned_pitch, aligned_height;
   if (modifier != DRM_FORMAT_MOD_LINEAR) {
      surf_size = block_count(dims[i][0], dims[i][1],
            } else {
      aligned_pitch = align(dims[i][0], 256 / util_format_get_blocksize(format));
   aligned_height = dims[i][1];
                  assert(surf.u.gfx9.surf_pitch == aligned_pitch);
   assert(surf.u.gfx9.surf_height == aligned_height);
   assert(surf.surf_size == surf_size);
            if (ac_modifier_has_dcc_retile(modifier)) {
      unsigned dcc_align = info->gfx_level >= GFX10 ? 4096 : 65536;
   unsigned dcc_pitch;
   uint64_t dcc_size = block_count(dims[i][0], dims[i][1],
                  assert(surf.u.gfx9.color.display_dcc_size == align(dcc_size, dcc_align));
                     } else
            if (ac_modifier_has_dcc(modifier)) {
      uint64_t dcc_align = 1;
   unsigned block_bits;
   if (info->gfx_level >= GFX10) {
      unsigned num_pipes = G_0098F8_NUM_PIPES(info->gb_addr_config);
   if (info->gfx_level >= GFX10_3 &&
      G_0098F8_NUM_PKRS(info->gb_addr_config) == num_pipes && num_pipes > 1)
      block_bits = 16 +
      num_pipes +
      block_bits = MAX2(block_bits, 20);
   dcc_align = MAX2(4096, 256 <<
            } else {
      block_bits = 18 +
      G_0098F8_NUM_RB_PER_SE(info->gb_addr_config) +
      block_bits = MAX2(block_bits, 20);
                              uint64_t dcc_size = block_count(dims[i][0], dims[i][1],
                                 } else
                     generate_hash(addrlib, &entry, &surf);
            }
      static void run_modifier_test(struct u_vector *test_entries, const char *name,
         {
      struct ac_addrlib *addrlib = ac_addrlib_create(info, NULL);
            const struct ac_modifier_options options = {
      .dcc = true,
               enum pipe_format formats[] = {
      PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R16_UNORM,
   PIPE_FORMAT_R32_FLOAT,
   PIPE_FORMAT_R32G32_FLOAT,
      };
   for (unsigned j = 0; j < ARRAY_SIZE(formats); ++j) {
      unsigned mod_count = 0;
            uint64_t *modifiers = malloc(sizeof(uint64_t) * mod_count);
            for (unsigned i = 0; i < mod_count; ++i) {
                     }
      }
      static int compare_test_entry(const void *a, const void *b)
   {
         }
      static bool test_entry_key_equal(const struct test_entry *a, const struct test_entry *b)
   {
         }
      static bool test_entry_value_equal(const struct test_entry *a, const struct test_entry *b)
   {
      if (memcmp(a->hash, b->hash, sizeof(a->hash)))
            }
      static void print_test_entry(const struct test_entry *e)
   {
      printf("%.16" PRIx64 " %.4d %.4d %.2d %s %d %d %d %d\n", e->modifier, e->w, e->h,
      }
      int main()
   {
               struct u_vector test_entries;
            for (unsigned i = 0; i < ARRAY_SIZE(testcases); ++i) {
                           qsort(u_vector_tail(&test_entries),
         u_vector_length(&test_entries),
            struct test_entry *cur, *prev = NULL, *prevprev = NULL;
   bool mismatched_duplicates = false;
   u_vector_foreach(cur, &test_entries) {
      if (prev && test_entry_key_equal(cur, prev) &&
      !test_entry_value_equal(cur, prev)) {
   if (!prevprev || !test_entry_key_equal(prev, prevprev)) {
         }
   print_test_entry(cur);
      }
   prevprev = prev;
      }
                        }
