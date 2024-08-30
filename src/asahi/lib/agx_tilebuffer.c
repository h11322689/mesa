   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_tilebuffer.h"
   #include <assert.h>
   #include "compiler/agx_internal_formats.h"
   #include "util/bitscan.h"
   #include "util/format/u_format.h"
   #include "agx_formats.h"
   #include "agx_usc.h"
      /* Maximum number of bytes per tile on G13G. This may change in future versions
   * of the architecture.
   */
   #define MAX_BYTES_PER_TILE (32768 - 1)
      /* Maximum bytes per sample in the tilebuffer. Greater allocations require
   * spilling render targets to memory.
   */
   #define MAX_BYTES_PER_SAMPLE (64)
      /* Minimum tile size in pixels, architectural. */
   #define MIN_TILE_SIZE_PX (16 * 16)
      /* Select the largest tile size that fits */
   static struct agx_tile_size
   agx_select_tile_size(unsigned bytes_per_pixel)
   {
      /* clang-format off */
   struct agx_tile_size sizes[] = {
      { 32, 32 },
   { 32, 16 },
      };
            for (unsigned i = 0; i < ARRAY_SIZE(sizes); ++i) {
               if ((bytes_per_pixel * size.width * size.height) <= MAX_BYTES_PER_TILE)
                  }
      struct agx_tilebuffer_layout
   agx_build_tilebuffer_layout(enum pipe_format *formats, uint8_t nr_cbufs,
         {
      struct agx_tilebuffer_layout tib = {
      .nr_samples = nr_samples,
                        for (unsigned rt = 0; rt < nr_cbufs; ++rt) {
               /* Require natural alignment for tilebuffer allocations. This could be
   * optimized, but this shouldn't be a problem in practice.
   */
   enum pipe_format physical_fmt = agx_tilebuffer_physical_format(&tib, rt);
   unsigned align_B = util_format_get_blocksize(physical_fmt);
   assert(util_is_power_of_two_nonzero(align_B) &&
         util_is_power_of_two_nonzero(MAX_BYTES_PER_SAMPLE) &&
            offset_B = ALIGN_POT(offset_B, align_B);
            /* Determine the size, if we were to allocate this render target to the
   * tilebuffer as desired.
   */
   unsigned nr = util_format_get_nr_components(physical_fmt) == 1
                  unsigned size_B = align_B * nr;
            /* If allocating this render target would exceed any tilebuffer limits, we
   * need to spill it to memory. We continue processing in case there are
   * smaller render targets after that would still fit. Otherwise, we
   * allocate it to the tilebuffer.
   *
   * TODO: Suboptimal, we might be able to reorder render targets to
   * avoid fragmentation causing spilling.
   */
   bool fits =
                  if (fits) {
      tib._offset_B[rt] = offset_B;
      } else {
                              /* Multisampling needs a nonempty allocation.
   * XXX: Check this against hw
   */
   if (nr_samples > 1)
                     tib.tile_size = agx_select_tile_size(tib.sample_size_B * nr_samples);
      }
      enum pipe_format
   agx_tilebuffer_physical_format(struct agx_tilebuffer_layout *tib, unsigned rt)
   {
         }
      bool
   agx_tilebuffer_supports_mask(struct agx_tilebuffer_layout *tib, unsigned rt)
   {
      /* We don't bother support masking with spilled render targets. This might be
   * optimized in the future but spilling is so rare anyway it's not worth it.
   */
   if (tib->spilled[rt])
            enum pipe_format fmt = agx_tilebuffer_physical_format(tib, rt);
      }
      static unsigned
   agx_shared_layout_from_tile_size(struct agx_tile_size t)
   {
      if (t.width == 32 && t.height == 32)
         else if (t.width == 32 && t.height == 16)
         else if (t.width == 16 && t.height == 16)
         else
      }
      uint32_t
   agx_tilebuffer_total_size(struct agx_tilebuffer_layout *tib)
   {
      return tib->sample_size_B * tib->nr_samples * tib->tile_size.width *
      }
      void
   agx_usc_tilebuffer(struct agx_usc_builder *b, struct agx_tilebuffer_layout *tib)
   {
      agx_usc_pack(b, SHARED, cfg) {
      cfg.uses_shared_memory = true;
   cfg.layout = agx_shared_layout_from_tile_size(tib->tile_size);
   cfg.sample_stride_in_8_bytes = tib->sample_size_B / 8;
   cfg.sample_count = tib->nr_samples;
         }
