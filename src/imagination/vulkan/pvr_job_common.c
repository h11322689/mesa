   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <stdbool.h>
   #include <stdint.h>
      #include "hwdef/rogue_hw_defs.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_device_info.h"
   #include "pvr_job_common.h"
   #include "pvr_private.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_alloc.h"
   #include "vk_format.h"
   #include "vk_object.h"
      void pvr_pbe_get_src_format_and_gamma(VkFormat vk_format,
                           {
                        if (vk_format_has_32bit_component(vk_format) ||
      vk_format_is_int(vk_format)) {
      } else if (vk_format_is_float(vk_format)) {
         } else if (vk_format_is_srgb(vk_format)) {
               /* F16 source for gamma'd formats. */
      } else if (vk_format_has_depth(vk_format) &&
            vk_format_get_component_bits(vk_format,
            } else if (vk_format_has_stencil(vk_format) &&
            vk_format_get_component_bits(vk_format,
            } else if (chan_0_width > 16) {
         } else if (chan_0_width > 8) {
         } else if (!with_packed_usc_channel) {
         } else {
            }
      void pvr_pbe_pack_state(
      const struct pvr_device_info *dev_info,
   const struct pvr_pbe_surf_params *surface_params,
   const struct pvr_pbe_render_params *render_params,
   uint32_t pbe_cs_words[static const ROGUE_NUM_PBESTATE_STATE_WORDS],
      {
      /* This function needs updating if the value of
   * ROGUE_NUM_PBESTATE_STATE_WORDS changes, so check that it's the expected
   * value.
   */
            /* This function needs updating if the value of ROGUE_NUM_PBESTATE_REG_WORDS
   * changes, so check that it's the expected value.
   */
                     if (surface_params->z_only_render) {
               pvr_csb_pack (&pbe_cs_words[1], PBESTATE_STATE_WORD1, state) {
                  pbe_reg_words[0] = 0;
                        pvr_csb_pack (&pbe_cs_words[0], PBESTATE_STATE_WORD0, state) {
                  pvr_csb_pack (&pbe_cs_words[1], PBESTATE_STATE_WORD1, state) {
                        state.source_pos = pvr_pbestate_source_pos(render_params->source_start);
   if (PVR_HAS_FEATURE(dev_info, eight_output_registers)) {
      state.source_pos_offset_128 = render_params->source_start >=
      } else {
                  /* MRT index (Use 0 for a single render target)/ */
            /* Normalization flag based on output format. */
                        pvr_csb_pack (&pbe_reg_words[0], PBESTATE_REG_WORD0, reg) {
               switch (surface_params->mem_layout) {
   case PVR_MEMLAYOUT_TWIDDLED:
                  case PVR_MEMLAYOUT_3DTWIDDLED:
                  case PVR_MEMLAYOUT_LINEAR:
   default:
      reg.memlayout = PVRX(PBESTATE_MEMLAYOUT_LINEAR);
               /* FIXME: Remove rotation and y_flip hardcoding if needed. */
   reg.rotation = PVRX(PBESTATE_ROTATION_TYPE_0_DEG);
            /* Note: Due to gamma being overridden above, anything other than
   * ENABLED/NONE is ignored.
   */
   if (surface_params->gamma == PVR_PBE_GAMMA_ENABLED) {
               if (surface_params->nr_components == 2)
      reg.twocomp_gamma =
            reg.linestride = (surface_params->stride - 1) /
                  /* r, y or depth*/
   switch (surface_params->swizzle[0]) {
   case PIPE_SWIZZLE_X:
      reg.swiz_chan0 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN0;
      case PIPE_SWIZZLE_Y:
      reg.swiz_chan1 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN0;
      case PIPE_SWIZZLE_Z:
      reg.swiz_chan2 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN0;
      case PIPE_SWIZZLE_W:
      reg.swiz_chan3 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN0;
      case PIPE_SWIZZLE_0:
   case PIPE_SWIZZLE_NONE:
      reg.swiz_chan0 = ROGUE_PBESTATE_SWIZ_ZERO;
      case PIPE_SWIZZLE_1:
      reg.swiz_chan0 = ROGUE_PBESTATE_SWIZ_ONE;
      default:
      unreachable("Unknown enum pipe_swizzle");
      }
   /* g, u or stencil*/
   switch (surface_params->swizzle[1]) {
   case PIPE_SWIZZLE_X:
      reg.swiz_chan0 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN1;
      case PIPE_SWIZZLE_Y:
      reg.swiz_chan1 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN1;
      case PIPE_SWIZZLE_Z:
      reg.swiz_chan2 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN1;
      case PIPE_SWIZZLE_W:
      reg.swiz_chan3 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN1;
      case PIPE_SWIZZLE_0:
   case PIPE_SWIZZLE_NONE:
      reg.swiz_chan1 = ROGUE_PBESTATE_SWIZ_ZERO;
      case PIPE_SWIZZLE_1:
      reg.swiz_chan1 = ROGUE_PBESTATE_SWIZ_ONE;
      default:
      unreachable("Unknown enum pipe_swizzle");
      }
   /* b or v*/
   switch (surface_params->swizzle[2]) {
   case PIPE_SWIZZLE_X:
      reg.swiz_chan0 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN2;
      case PIPE_SWIZZLE_Y:
      reg.swiz_chan1 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN2;
      case PIPE_SWIZZLE_Z:
      reg.swiz_chan2 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN2;
      case PIPE_SWIZZLE_W:
      reg.swiz_chan3 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN2;
      case PIPE_SWIZZLE_0:
   case PIPE_SWIZZLE_NONE:
      reg.swiz_chan2 = ROGUE_PBESTATE_SWIZ_ZERO;
      case PIPE_SWIZZLE_1:
      reg.swiz_chan2 = ROGUE_PBESTATE_SWIZ_ONE;
      default:
      unreachable("Unknown enum pipe_swizzle");
      }
   /* a */
   switch (surface_params->swizzle[3]) {
   case PIPE_SWIZZLE_X:
      reg.swiz_chan0 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN3;
      case PIPE_SWIZZLE_Y:
      reg.swiz_chan1 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN3;
      case PIPE_SWIZZLE_Z:
      reg.swiz_chan2 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN3;
      case PIPE_SWIZZLE_W:
      reg.swiz_chan3 = ROGUE_PBESTATE_SWIZ_SOURCE_CHAN3;
      case PIPE_SWIZZLE_0:
   case PIPE_SWIZZLE_NONE:
      reg.swiz_chan3 = ROGUE_PBESTATE_SWIZ_ZERO;
      case PIPE_SWIZZLE_1:
      reg.swiz_chan3 = ROGUE_PBESTATE_SWIZ_ONE;
      default:
      unreachable("Unknown enum pipe_swizzle");
               if (surface_params->mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
                        pvr_csb_pack (&pbe_reg_words[1], PBESTATE_REG_WORD1, reg) {
      if (surface_params->mem_layout == PVR_MEMLAYOUT_TWIDDLED ||
      surface_params->mem_layout == PVR_MEMLAYOUT_3DTWIDDLED) {
   reg.size_x = util_logbase2_ceil(surface_params->width);
               reg.minclip_y = render_params->min_y_clip;
   reg.maxclip_x = render_params->max_x_clip;
   reg.zslice = render_params->slice;
         }
      /* TODO: Split this into smaller functions to make it easier to follow. When
   * doing this, it would be nice to have a function that returns
   * total_tiles_in_flight so that CR_ISP_CTL can be fully packed in
   * pvr_render_job_ws_fragment_state_init().
   */
   void pvr_setup_tiles_in_flight(
      const struct pvr_device_info *dev_info,
   const struct pvr_device_runtime_info *dev_runtime_info,
   uint32_t msaa_mode,
   uint32_t pixel_width,
   bool paired_tiles,
   uint32_t max_tiles_in_flight,
   uint32_t *const isp_ctl_out,
      {
      uint32_t total_tiles_in_flight = 0;
   uint32_t usable_partition_size;
   uint32_t partitions_available;
   uint32_t usc_min_output_regs;
   uint32_t max_partitions;
   uint32_t partition_size;
   uint32_t max_phantoms;
   uint32_t tile_size_x;
   uint32_t tile_size_y;
            /* Round up the pixel width to the next allocation granularity. */
   usc_min_output_regs =
         pixel_width = MAX2(pixel_width, usc_min_output_regs);
                              isp_samples = PVR_GET_FEATURE_VALUE(dev_info, isp_samples_per_pixel, 1);
   if (isp_samples == 2) {
      if (msaa_mode != PVRX(CR_ISP_AA_MODE_TYPE_AA_NONE))
      } else if (isp_samples == 4) {
      if (msaa_mode == PVRX(CR_ISP_AA_MODE_TYPE_AA_4X) ||
      msaa_mode == PVRX(CR_ISP_AA_MODE_TYPE_AA_8X))
      else if (msaa_mode == PVRX(CR_ISP_AA_MODE_TYPE_AA_2X))
               /* Cores with a tile size of 16x16 don't have quadrant affinity. Hence the
   * partition size is the same as for a 32x32 tile quadrant (with no MSAA).
   * When MSAA is enabled, the USC has to process half the tile (16x8 pixels).
   */
   tile_size_x = PVR_GET_FEATURE_VALUE(dev_info, tile_size_x, 0);
            /* We only support square tiles. */
            if (tile_size_x == 16U) {
      /* Cores with 16x16 tiles does not use tile quadrants. */
      } else {
      /* Size of a tile quadrant (in dwords). */
               /* Maximum available partition space for partitions of this size. */
   max_partitions = PVR_GET_FEATURE_VALUE(dev_info, max_partitions, 0);
   usable_partition_size = MIN2(dev_runtime_info->total_reserved_partition_size,
            if (PVR_GET_FEATURE_VALUE(dev_info, common_store_size_in_dwords, 0) <
      (1024 * 4 * 4)) {
   /* Do not apply the limit for cores with 16x16 tile size (no quadrant
   * affinity). */
   if (tile_size_x != 16) {
      /* This is to counter the extremely limited CS size on some cores.
   */
   /* Available partition space is limited to 8 tile quadrants. */
   usable_partition_size =
                  /* Ensure that maximum number of partitions in use is not greater
   * than the total number of partitions available.
   */
   partitions_available =
            if (PVR_HAS_FEATURE(dev_info, xt_top_infrastructure))
         else if (PVR_HAS_FEATURE(dev_info, roguexe))
         else
            for (uint32_t i = 0; i < max_phantoms; i++) {
      uint32_t usc_tiles_in_flight = partitions_available;
            /* Cores with tiles size other than 16x16 use tile quadrants. */
   if (tile_size_x != 16) {
      uint32_t num_clusters =
         usc_tiles_in_flight =
                        isp_tiles_in_flight =
         /* Ensure that maximum number of ISP tiles in flight is not greater
   * than the maximum number of USC tiles in flight.
   */
   if (!PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format) ||
      PVR_GET_FEATURE_VALUE(dev_info, simple_parameter_format_version, 0) !=
                              /* Limit the number of tiles in flight if the shaders have
   * requested a large allocation of local memory.
   */
   if (max_tiles_in_flight > 0U) {
               if (PVR_HAS_FEATURE(dev_info, roguexe)) {
      if (tile_size_x == 16) {
      /* The FW infers the tiles in flight value from the
   * partitions setting.
   */
   /* Partitions per tile. */
      } else {
      /* Partitions per tile quadrant. */
                     /* Due to limitations of ISP_CTL_PIPE there can only be a difference of
   * 1 between Phantoms.
   */
   if (total_tiles_in_flight > (isp_tiles_in_flight + 1U))
                        if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format) &&
      PVR_GET_FEATURE_VALUE(dev_info, simple_parameter_format_version, 0) ==
         /* Limit the ISP tiles in flight to fit into the available USC partition
   * store.
   */
               if (PVR_HAS_FEATURE(dev_info, paired_tiles) && paired_tiles) {
      total_tiles_in_flight =
               pvr_csb_pack (pixel_ctl_out, CR_USC_PIXEL_OUTPUT_CTRL, reg) {
      if (pixel_width == 1 && usc_min_output_regs == 1) {
         } else if (pixel_width == 2) {
         } else if (pixel_width == 4) {
         } else if (pixel_width == 8 &&
               } else if (usc_min_output_regs == 1) {
         } else {
                  if (PVR_HAS_FEATURE(dev_info, usc_pixel_partition_mask)) {
      /* Setup the partition mask based on the maximum number of
   * partitions available.
   */
      } else {
               /* Setup the partition mask based on the number of partitions
   * available.
   */
                  pvr_csb_pack (isp_ctl_out, CR_ISP_CTL, reg) {
      if (PVR_HAS_FEATURE(dev_info, xt_top_infrastructure))
         else
         }
