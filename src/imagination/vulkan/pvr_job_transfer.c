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
      #include <assert.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "pvr_csb.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_formats.h"
   #include "pvr_job_common.h"
   #include "pvr_job_context.h"
   #include "pvr_job_transfer.h"
   #include "pvr_private.h"
   #include "pvr_tex_state.h"
   #include "pvr_transfer_frag_store.h"
   #include "pvr_types.h"
   #include "pvr_uscgen.h"
   #include "pvr_util.h"
   #include "pvr_winsys.h"
   #include "util/bitscan.h"
   #include "util/list.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "util/xxhash.h"
   #include "vk_format.h"
   #include "vk_log.h"
   #include "vk_sync.h"
      #define PVR_TRANSFER_MAX_PASSES 10U
   #define PVR_TRANSFER_MAX_CLIP_RECTS 4U
   #define PVR_TRANSFER_MAX_PREPARES_PER_SUBMIT 16U
   #define PVR_TRANSFER_MAX_CUSTOM_RECTS 3U
      /* Number of triangles sent to the TSP per raster. */
   #define PVR_TRANSFER_NUM_LAYERS 1U
      #define PVR_MAX_WIDTH 16384
   #define PVR_MAX_HEIGHT 16384
      #define PVR_MAX_CLIP_SIZE(dev_info) \
            enum pvr_paired_tiles {
      PVR_PAIRED_TILES_NONE,
   PVR_PAIRED_TILES_X,
      };
      struct pvr_transfer_wa_source {
      uint32_t src_offset;
   uint32_t mapping_count;
   struct pvr_rect_mapping mappings[PVR_TRANSFER_MAX_CUSTOM_MAPPINGS];
      };
      struct pvr_transfer_pass {
               uint32_t source_count;
            uint32_t clip_rects_count;
      };
      /* Structure representing a layer iteration. */
   struct pvr_transfer_custom_mapping {
      bool double_stride;
   uint32_t texel_unwind_src;
   uint32_t texel_unwind_dst;
   uint32_t texel_extend_src;
   uint32_t texel_extend_dst;
   uint32_t pass_count;
   struct pvr_transfer_pass passes[PVR_TRANSFER_MAX_PASSES];
   uint32_t max_clip_rects;
      };
      struct pvr_transfer_3d_iteration {
         };
      struct pvr_transfer_3d_state {
               bool empty_dst;
   bool down_scale;
   /* Write all channels present in the dst from the USC even if those are
   * constants.
   */
            /* The rate of the shader. */
   uint32_t msaa_multiplier;
   /* Top left corner of the render in ISP tiles. */
   uint32_t origin_x_in_tiles;
   /* Top left corner of the render in ISP tiles. */
   uint32_t origin_y_in_tiles;
   /* Width of the render in ISP tiles. */
   uint32_t width_in_tiles;
   /* Height of the render in ISP tiles. */
            /* Width of a sample in registers (pixel partition width). */
            /* Properties of the USC shader. */
            /* TODO: Use pvr_dev_addr_t of an offset type for these. */
   uint32_t pds_shader_task_offset;
   uint32_t tex_state_data_offset;
            uint32_t uniform_data_size;
   uint32_t tex_state_data_size;
            /* Pointer into the common store. */
   uint32_t common_ptr;
   /* Pointer into the dynamic constant reg buffer. */
   uint32_t dynamic_const_reg_ptr;
   /* Pointer into the USC constant reg buffer. */
            uint32_t pds_coeff_task_offset;
            /* Number of temporary 32bit registers used by PDS. */
            struct pvr_transfer_custom_mapping custom_mapping;
            enum pvr_filter filter[PVR_TRANSFER_MAX_SOURCES];
               };
      struct pvr_transfer_prep_data {
      struct pvr_winsys_transfer_cmd_flags flags;
      };
      struct pvr_transfer_submit {
      uint32_t prep_count;
   struct pvr_transfer_prep_data
      };
      static enum pvr_transfer_pbe_pixel_src pvr_pbe_src_format_raw(VkFormat format)
   {
               if (bpp <= 32U)
         else if (bpp <= 64U)
               }
      static VkResult pvr_pbe_src_format_pick_depth(
      const VkFormat src_format,
   const VkFormat dst_format,
      {
      if (dst_format != VK_FORMAT_D24_UNORM_S8_UINT)
            switch (src_format) {
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
      *src_format_out = PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D24S8_D24S8;
         case VK_FORMAT_D32_SFLOAT:
      *src_format_out = PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32_D24S8;
         default:
                     }
      static VkResult pvr_pbe_src_format_pick_stencil(
      const VkFormat src_format,
   const VkFormat dst_format,
      {
      if ((src_format != VK_FORMAT_D24_UNORM_S8_UINT &&
      src_format != VK_FORMAT_S8_UINT) ||
   dst_format != VK_FORMAT_D24_UNORM_S8_UINT) {
               if (src_format == VK_FORMAT_S8_UINT)
         else
               }
      static VkResult
   pvr_pbe_src_format_ds(const struct pvr_transfer_cmd_surface *src,
                        const enum pvr_filter filter,
      {
               const bool src_depth = vk_format_has_depth(src_format);
   const bool dst_depth = vk_format_has_depth(dst_format);
   const bool src_stencil = vk_format_has_stencil(src_format);
            if (flags & PVR_TRANSFER_CMD_FLAGS_DSMERGE) {
      /* Merging, so destination should always have both. */
   if (!dst_depth || !dst_stencil)
            if (flags & PVR_TRANSFER_CMD_FLAGS_PICKD) {
      return pvr_pbe_src_format_pick_depth(src_format,
            } else {
      return pvr_pbe_src_format_pick_stencil(src_format,
                        /* We can't invent channels out of nowhere. */
   if ((dst_depth && !src_depth) || (dst_stencil && !src_stencil))
            switch (dst_format) {
   case VK_FORMAT_D16_UNORM:
      if (src_format == VK_FORMAT_D24_UNORM_S8_UINT)
            if (!down_scale)
         else
               case VK_FORMAT_D24_UNORM_S8_UINT:
      switch (src_format) {
   case VK_FORMAT_D24_UNORM_S8_UINT:
      if (filter == PVR_FILTER_LINEAR)
                              /* D16_UNORM results in a 0.0->1.0 float from the TPU, the same as D32 */
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_D32_SFLOAT:
                  default:
      if (filter == PVR_FILTER_LINEAR)
         else
                     case VK_FORMAT_D32_SFLOAT:
      if (src_format == VK_FORMAT_D24_UNORM_S8_UINT)
         else
                  default:
      if (src_format == VK_FORMAT_D24_UNORM_S8_UINT)
         else
                  }
      /**
   * How the PBE expects the output buffer for an RGBA space conversion.
   */
   static VkResult
   pvr_pbe_src_format_normal(VkFormat src_format,
                           {
      bool dst_signed = vk_format_is_sint(dst_format) ||
            if (vk_format_is_int(dst_format)) {
      uint32_t red_width;
   bool src_signed;
            if (!vk_format_is_int(src_format))
                     red_width = vk_format_get_component_bits(dst_format,
                  switch (red_width) {
   case 8:
      if (!src_signed && !dst_signed)
         else if (src_signed && !dst_signed)
         else if (!src_signed && dst_signed)
                              case 10:
      switch (dst_format) {
   case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      *src_format_out = src_signed ? PVR_TRANSFER_PBE_PIXEL_SRC_SU1010102
               case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      *src_format_out = src_signed
                     default:
                     case 16:
      if (!src_signed && !dst_signed)
         else if (src_signed && !dst_signed)
         else if (!src_signed && dst_signed)
                              case 32:
      if (dont_force_pbe) {
         } else {
      count =
               if (!src_signed && !dst_signed) {
      *src_format_out = (count > 2U) ? PVR_TRANSFER_PBE_PIXEL_SRC_RAW128
      } else if (src_signed && !dst_signed) {
      *src_format_out = (count > 2U) ? PVR_TRANSFER_PBE_PIXEL_SRC_S4XU32
      } else if (!src_signed && dst_signed) {
      *src_format_out = (count > 2U) ? PVR_TRANSFER_PBE_PIXEL_SRC_U4XS32
      } else {
      *src_format_out = (count > 2U) ? PVR_TRANSFER_PBE_PIXEL_SRC_RAW128
                  default:
               } else if (vk_format_is_float(dst_format) ||
                     if (!vk_format_is_float(src_format) &&
      !vk_format_is_normalized(src_format) &&
   !vk_format_is_block_compressed(src_format)) {
               if (vk_format_is_normalized(dst_format)) {
                        /* Alpha only. */
   switch (dst_format) {
   case VK_FORMAT_D16_UNORM:
                  default:
      chan_width =
      vk_format_get_component_bits(dst_format,
                        if (src_format == dst_format) {
      switch (chan_width) {
   case 16U:
      if (down_scale) {
      *src_format_out = dst_signed
            } else {
      *src_format_out = dst_signed
                        case 32U:
      *src_format_out = pvr_pbe_src_format_raw(dst_format);
      default:
      is_float = true;
         } else {
      switch (chan_width) {
   case 16U:
      *src_format_out = dst_signed
                  default:
      is_float = true;
                     if (is_float) {
                        if (dont_force_pbe) {
         } else {
                        switch (count) {
   case 1U:
      *src_format_out = PVR_TRANSFER_PBE_PIXEL_SRC_F32;
      case 2U:
      *src_format_out = PVR_TRANSFER_PBE_PIXEL_SRC_F32X2;
      default:
      *src_format_out = PVR_TRANSFER_PBE_PIXEL_SRC_F32X4;
         } else {
      if (dst_format == VK_FORMAT_B8G8R8A8_UNORM ||
      dst_format == VK_FORMAT_R8G8B8A8_UNORM ||
   dst_format == VK_FORMAT_A8B8G8R8_UNORM_PACK32) {
      } else {
                  } else {
                     }
      static inline uint32_t
   pvr_get_blit_flags(const struct pvr_transfer_cmd *transfer_cmd)
   {
      return transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_FAST2D
            }
      static VkResult pvr_pbe_src_format(struct pvr_transfer_cmd *transfer_cmd,
               {
      struct pvr_tq_layer_properties *layer = &prop->layer_props;
   const enum pvr_filter filter = transfer_cmd->source_count
               const uint32_t flags = transfer_cmd->flags;
   VkFormat dst_format = transfer_cmd->dst.vk_format;
   const struct pvr_transfer_cmd_surface *src;
   VkFormat src_format;
            if (transfer_cmd->source_count > 0) {
      src = &transfer_cmd->sources[0].surface;
   down_scale = transfer_cmd->sources[0].resolve_op == PVR_RESOLVE_BLEND &&
            } else {
      src = &transfer_cmd->dst;
                        /* This has to come before the rest as S8 for instance is integer and
   * signedness check fails on D24S8.
   */
   if (vk_format_is_depth_or_stencil(src_format) ||
      vk_format_is_depth_or_stencil(dst_format) ||
   flags & PVR_TRANSFER_CMD_FLAGS_DSMERGE) {
   return pvr_pbe_src_format_ds(src,
                                       return pvr_pbe_src_format_normal(src_format,
                        }
      static inline void pvr_setup_hwbg_object(const struct pvr_device_info *dev_info,
         {
               pvr_csb_pack (&regs->pds_bgnd0_base, CR_PDS_BGRND0_BASE, reg) {
      reg.shader_addr = PVR_DEV_ADDR(state->pds_shader_task_offset);
   assert(pvr_dev_addr_is_aligned(
      reg.shader_addr,
      reg.texunicode_addr = PVR_DEV_ADDR(state->uni_tex_code_offset);
   assert(pvr_dev_addr_is_aligned(
      reg.texunicode_addr,
            pvr_csb_pack (&regs->pds_bgnd1_base, CR_PDS_BGRND1_BASE, reg) {
      reg.texturedata_addr = PVR_DEV_ADDR(state->tex_state_data_offset);
   assert(pvr_dev_addr_is_aligned(
      reg.texturedata_addr,
                     pvr_csb_pack (&regs->pds_bgnd3_sizeinfo, CR_PDS_BGRND3_SIZEINFO, reg) {
      reg.usc_sharedsize =
                  assert(!(state->uniform_data_size &
         reg.pds_uniformsize =
                  assert(
      !(state->tex_state_data_size &
      reg.pds_texturestatesize =
                  reg.pds_tempsize =
      DIV_ROUND_UP(state->pds_temps,
      }
      static inline bool
   pvr_is_surface_aligned(pvr_dev_addr_t dev_addr, bool is_input, uint32_t bpp)
   {
      /* 96 bpp is 32 bit granular. */
   if (bpp == 64U || bpp == 128U) {
               if ((dev_addr.addr & mask) != 0ULL)
               if (is_input) {
      if ((dev_addr.addr &
      (PVRX(TEXSTATE_STRIDE_IMAGE_WORD1_TEXADDR_ALIGNMENT) - 1U)) !=
   0ULL) {
         } else {
      if ((dev_addr.addr &
      (PVRX(PBESTATE_STATE_WORD0_ADDRESS_LOW_ALIGNMENT) - 1U)) != 0ULL) {
                     }
      static inline VkResult
   pvr_mem_layout_spec(const struct pvr_transfer_cmd_surface *surface,
                     uint32_t load,
   bool is_input,
   uint32_t *width_out,
   uint32_t *height_out,
   {
      const uint32_t bpp = vk_format_get_blocksizebits(surface->vk_format);
            *mem_layout_out = surface->mem_layout;
   *height_out = surface->height;
   *width_out = surface->width;
   *stride_out = surface->stride;
            if (surface->mem_layout != PVR_MEMLAYOUT_LINEAR &&
      !pvr_is_surface_aligned(*dev_addr_out, is_input, bpp)) {
               switch (surface->mem_layout) {
   case PVR_MEMLAYOUT_LINEAR:
      if (surface->stride == 0U)
                     if (!pvr_is_surface_aligned(*dev_addr_out, is_input, bpp))
            if (unsigned_stride < *width_out)
            if (!is_input) {
      if (unsigned_stride == 1U) {
      /* Change the setup to twiddling as that doesn't hit the stride
   * limit and twiddled == strided when 1px stride.
   */
                  *stride_out = unsigned_stride;
         case PVR_MEMLAYOUT_TWIDDLED:
   case PVR_MEMLAYOUT_3DTWIDDLED:
      /* Ignoring stride value for twiddled/tiled surface. */
   *stride_out = *width_out;
         default:
                     }
      static VkResult
   pvr_pbe_setup_codegen_defaults(const struct pvr_device_info *dev_info,
                           {
      const struct pvr_transfer_cmd_surface *dst = &transfer_cmd->dst;
   const uint8_t *swizzle;
   VkFormat format;
            switch (dst->vk_format) {
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
      format = VK_FORMAT_R32_UINT;
         default:
      format = dst->vk_format;
               swizzle = pvr_get_format_swizzle(format);
            pvr_pbe_get_src_format_and_gamma(format,
                              surface_params->is_normalized = vk_format_is_normalized(format);
   surface_params->pbe_packmode = pvr_get_pbe_packmode(format);
            result = pvr_mem_layout_spec(dst,
                              0U,
   false,
      if (result != VK_SUCCESS)
            surface_params->z_only_render = false;
   surface_params->depth = dst->depth;
            if (surface_params->mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
         else
            uint32_t tile_size_x = PVR_GET_FEATURE_VALUE(dev_info, tile_size_x, 0U);
            /* If the rectangle happens to be empty / off-screen we clip away
   * everything.
   */
   if (state->empty_dst) {
      render_params->min_x_clip = 2U * tile_size_x;
   render_params->max_x_clip = 3U * tile_size_x;
   render_params->min_y_clip = 2U * tile_size_y;
   render_params->max_y_clip = 3U * tile_size_y;
   state->origin_x_in_tiles = 0U;
   state->origin_y_in_tiles = 0U;
   state->height_in_tiles = 1U;
      } else {
               /* Clamp */
   render_params->min_x_clip =
         render_params->max_x_clip =
      MAX2(MIN2(scissor->offset.x + scissor->extent.width,
                     render_params->min_y_clip =
         render_params->max_y_clip =
      MAX2(MIN2(scissor->offset.y + scissor->extent.height,
                     if (state->custom_mapping.pass_count > 0U) {
                     render_params->min_x_clip = (uint32_t)pass->clip_rects[0U].offset.x;
   render_params->max_x_clip =
      (uint32_t)(pass->clip_rects[0U].offset.x +
            render_params->min_y_clip = (uint32_t)pass->clip_rects[0U].offset.y;
   render_params->max_y_clip =
      (uint32_t)(pass->clip_rects[0U].offset.y +
                  state->origin_x_in_tiles = render_params->min_x_clip / tile_size_x;
   state->origin_y_in_tiles = render_params->min_y_clip / tile_size_y;
   state->width_in_tiles =
         state->height_in_tiles =
            /* Be careful here as this isn't the same as ((max_x_clip -
   * min_x_clip) + tile_size_x) >> tile_size_x.
   */
   state->width_in_tiles -= state->origin_x_in_tiles;
               render_params->source_start = PVR_PBE_STARTPOS_BIT0;
               }
      static VkResult
   pvr_pbe_setup_modify_defaults(const struct pvr_transfer_cmd_surface *dst,
                           {
      struct pvr_transfer_pass *pass;
                              if (state->custom_mapping.pass_count == 0)
                                       render_params->min_x_clip = (uint32_t)clip_rect->offset.x;
   render_params->max_x_clip =
         render_params->min_y_clip = (uint32_t)clip_rect->offset.y;
   render_params->max_y_clip =
               }
      static uint32_t
   pvr_pbe_get_pixel_size(enum pvr_transfer_pbe_pixel_src pixel_format)
   {
      switch (pixel_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_CONV_D24_D32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_CONV_D32_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_CONV_S8D24_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D24S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F16_U8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RBSWAP_SU1010102:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RBSWAP_UU1010102:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D24S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SS8888:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SU1010102:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SU8888:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SWAP_LMSB:
   case PVR_TRANSFER_PBE_PIXEL_SRC_US8888:
   case PVR_TRANSFER_PBE_PIXEL_SRC_UU1010102:
   case PVR_TRANSFER_PBE_PIXEL_SRC_UU8888:
            case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F16F16:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32X2:
   case PVR_TRANSFER_PBE_PIXEL_SRC_MOV_BY45:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW64:
   case PVR_TRANSFER_PBE_PIXEL_SRC_S16NORM:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D24S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D32S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SS16S16:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SU16U16:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SU32U32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_U16NORM:
   case PVR_TRANSFER_PBE_PIXEL_SRC_US16S16:
   case PVR_TRANSFER_PBE_PIXEL_SRC_US32S32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_UU16U16:
            case PVR_TRANSFER_PBE_PIXEL_SRC_F32X4:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW128:
   case PVR_TRANSFER_PBE_PIXEL_SRC_S4XU32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_U4XS32:
            case PVR_TRANSFER_PBE_PIXEL_SRC_NUM:
   default:
                     }
      static void pvr_pbe_setup_swizzle(const struct pvr_transfer_cmd *transfer_cmd,
               {
      bool color_fill = !!(transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_FILL);
            const uint32_t pixel_size =
                     switch (dst->vk_format) {
   case VK_FORMAT_X8_D24_UNORM_PACK32:
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_S8_UINT:
      surf_params->swizzle[0U] = PIPE_SWIZZLE_X;
   surf_params->swizzle[1U] = PIPE_SWIZZLE_0;
   surf_params->swizzle[2U] = PIPE_SWIZZLE_0;
   surf_params->swizzle[3U] = PIPE_SWIZZLE_0;
         default: {
      const uint32_t red_width =
      vk_format_get_component_bits(dst->vk_format,
               if (transfer_cmd->source_count > 0 &&
      vk_format_is_alpha(dst->vk_format)) {
   if (vk_format_has_alpha(transfer_cmd->sources[0].surface.vk_format)) {
      /* Modify the destination format swizzle to always source from
   * src0.
   */
   surf_params->swizzle[0U] = PIPE_SWIZZLE_X;
   surf_params->swizzle[1U] = PIPE_SWIZZLE_0;
   surf_params->swizzle[2U] = PIPE_SWIZZLE_0;
   surf_params->swizzle[3U] = PIPE_SWIZZLE_1;
               /* Source format having no alpha channel still allocates 4 output
   * buffer registers.
               if (vk_format_is_normalized(dst->vk_format)) {
      if (color_fill &&
      (dst->vk_format == VK_FORMAT_B8G8R8A8_UNORM ||
   dst->vk_format == VK_FORMAT_R8G8B8A8_UNORM ||
   dst->vk_format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)) {
   surf_params->source_format =
      } else if (state->shader_props.layer_props.pbe_format ==
            surf_params->source_format =
      } else if (red_width <= 8U) {
      surf_params->source_format =
         } else if (red_width == 32U && !state->dont_force_pbe) {
               for (uint32_t i = 0; i < transfer_cmd->source_count; i++) {
                                                switch (count) {
   case 1U:
      surf_params->swizzle[1U] = PIPE_SWIZZLE_0;
      case 2U:
      surf_params->swizzle[2U] = PIPE_SWIZZLE_0;
      case 3U:
                  case 4U:
   default:
            }
      }
      }
      /**
   * Calculates the required PBE byte mask based on the incoming transfer command.
   *
   * @param transfer_cmd  the transfer command
   * @return the bytemask (active high disable mask)
   */
      static uint64_t pvr_pbe_byte_mask(const struct pvr_device_info *dev_info,
         {
                        if (flags & PVR_TRANSFER_CMD_FLAGS_DSMERGE) {
               switch (transfer_cmd->dst.vk_format) {
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
      mask = 0xF0F0F0F0U;
      case VK_FORMAT_D24_UNORM_S8_UINT:
      mask = 0x88888888U;
      default:
                  if ((flags & PVR_TRANSFER_CMD_FLAGS_PICKD) == 0U)
                        /* The mask is as it was inactive on cores without the ERN. This keeps the
   * firmware agnostic to the feature.
   */
      }
      static VkResult pvr_pbe_setup_emit(const struct pvr_transfer_cmd *transfer_cmd,
                           {
      struct pvr_device *const device = ctx->device;
            struct pvr_winsys_transfer_regs *regs = &state->regs;
   struct pvr_pds_event_program program = {
      .emit_words = pbe_setup_words,
      };
   struct pvr_pds_upload pds_upload;
   uint32_t staging_buffer_size;
   uint32_t *staging_buffer;
   pvr_dev_addr_t addr;
            /* Precondition, make sure to use a valid index for ctx->usc_eot_bos. */
   assert(rt_count <= ARRAY_SIZE(ctx->usc_eot_bos));
            addr.addr = ctx->usc_eot_bos[rt_count - 1U]->dev_addr.addr -
            pvr_pds_setup_doutu(&program.task_control,
                                                staging_buffer = vk_alloc(&device->vk.alloc,
                     if (!staging_buffer)
            pvr_pds_generate_pixel_event_data_segment(&program,
                  /* TODO: We can save some memory by generating a code segment for each
   * rt_count, which at the time of writing is a maximum of 3, in
   * pvr_setup_transfer_eot_shaders() when we setup the corresponding EOT
   * USC programs.
   */
   pvr_pds_generate_pixel_event_code_segment(&program,
                  result =
      pvr_cmd_buffer_upload_pds(transfer_cmd->cmd_buffer,
                           staging_buffer,
   program.data_size,
   PVRX(CR_EVENT_PIXEL_PDS_DATA_ADDR_ALIGNMENT),
      vk_free(&device->vk.alloc, staging_buffer);
   if (result != VK_SUCCESS)
            pvr_csb_pack (&regs->event_pixel_pds_info, CR_EVENT_PIXEL_PDS_INFO, reg) {
      reg.temp_stride = 0U;
   reg.const_size =
      DIV_ROUND_UP(program.data_size,
      reg.usc_sr_size =
      DIV_ROUND_UP(rt_count * PVR_STATE_PBE_DWORDS,
            pvr_csb_pack (&regs->event_pixel_pds_data, CR_EVENT_PIXEL_PDS_DATA, reg) {
                  pvr_csb_pack (&regs->event_pixel_pds_code, CR_EVENT_PIXEL_PDS_CODE, reg) {
                     }
      static VkResult pvr_pbe_setup(const struct pvr_transfer_cmd *transfer_cmd,
               {
      struct pvr_device *const device = ctx->device;
            const struct pvr_transfer_cmd_surface *dst = &transfer_cmd->dst;
   uint32_t num_rts = vk_format_get_plane_count(dst->vk_format);
   uint32_t pbe_setup_words[PVR_TRANSFER_MAX_RENDER_TARGETS *
         struct pvr_pbe_render_params render_params;
   struct pvr_pbe_surf_params surf_params;
            if (state->custom_mapping.pass_count > 0U)
            if (PVR_HAS_FEATURE(dev_info, paired_tiles))
            for (uint32_t i = 0U; i < num_rts; i++) {
      uint64_t *pbe_regs;
            /* Ensure the access into the pbe_wordx_mrty is made within its bounds. */
   assert(i * ROGUE_NUM_PBESTATE_REG_WORDS_FOR_TRANSFER <
         /* Ensure the access into pbe_setup_words is made within its bounds. */
            pbe_regs =
      &state->regs
               if (PVR_HAS_ERN(dev_info, 42064))
            if (i == 0U) {
      result = pvr_pbe_setup_codegen_defaults(dev_info,
                           if (result != VK_SUCCESS)
      } else {
      result = pvr_pbe_setup_modify_defaults(dst,
                           if (result != VK_SUCCESS)
                        pvr_pbe_pack_state(dev_info,
                              if (PVR_HAS_ERN(dev_info, 42064)) {
               pvr_csb_pack (&temp_reg, PBESTATE_REG_WORD2, reg) {
                              if (PVR_HAS_FEATURE(dev_info, paired_tiles)) {
      if (pbe_regs[2U] &
      (1ULL << PVRX(PBESTATE_REG_WORD2_PAIR_TILES_SHIFT))) {
   if (transfer_cmd->dst.mem_layout == PVR_MEMLAYOUT_TWIDDLED)
         else
                     result =
         if (result != VK_SUCCESS)
            /* Adjust tile origin and width to include all emits. */
   if (state->custom_mapping.pass_count > 0U) {
      const uint32_t tile_size_x =
         const uint32_t tile_size_y =
         struct pvr_transfer_pass *pass =
         VkOffset2D offset = { 0U, 0U };
            for (uint32_t i = 0U; i < pass->clip_rects_count; i++) {
               offset.x = MIN2(offset.x, rect->offset.x);
   offset.y = MIN2(offset.y, rect->offset.y);
   end.x = MAX2(end.x, rect->offset.x + rect->extent.width);
               state->origin_x_in_tiles = (uint32_t)offset.x / tile_size_x;
   state->origin_y_in_tiles = (uint32_t)offset.y / tile_size_y;
   state->width_in_tiles =
         state->height_in_tiles =
                  }
      /**
   * Writes the ISP tile registers according to the MSAA state. Sets up the USC
   * pixel partition allocations and the number of tiles in flight.
   */
   static VkResult pvr_isp_tiles(const struct pvr_device *device,
         {
      const struct pvr_device_runtime_info *dev_runtime_info =
         const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t isp_samples =
         uint32_t origin_x = state->origin_x_in_tiles;
   uint32_t origin_y = state->origin_y_in_tiles;
   uint32_t width = state->width_in_tiles;
   uint32_t height = state->height_in_tiles;
            /* msaa_multiplier is calculated by sample_count & ~1U. Given sample
   * count is always in powers of two, we can get the sample count from
   * msaa_multiplier using the following logic.
   */
            /* isp_samples_per_pixel feature is also know as "2x/4x for free", when
   * this is present SAMPLES_PER_PIXEL is 2/4, otherwise 1. The following
   * logic should end up with these numbers:
   *
   * |---------------------------------|
   * | 4 SAMPLES / ISP PIXEL           |
   * |-----------------------+----+----|
   * |                  MSAA | X* | Y* |
   * |                    2X |  1 |  1 |
   * |                    4X |  1 |  1 |
   * |---------------------------------|
   * | 2 SAMPLES / ISP PIXEL           |
   * |-----------------------+----+----|
   * |                  MSAA | X* | Y* |
   * |                    2X |  1 |  1 |
   * |                    4X |  1 |  2 |
   * |                    8X |  2 |  2 |
   * |-----------------------+----+----|
   * |  1 SAMPLE / ISP PIXEL           |
   * |-----------------------+----+----|
   * |                  MSAA | X* | Y* |
   * |                    2X |  1 |  2 |
   * |                    4X |  2 |  2 |
   * |-----------------------+----+----|
            origin_x <<= (state->msaa_multiplier >> (isp_samples + 1U)) & 1U;
   origin_y <<= ((state->msaa_multiplier >> (isp_samples + 1U)) |
               width <<= (state->msaa_multiplier >> (isp_samples + 1U)) & 1U;
   height <<= ((state->msaa_multiplier >> (isp_samples + 1U)) |
                  if (PVR_HAS_FEATURE(dev_info, paired_tiles) &&
      state->pair_tiles != PVR_PAIRED_TILES_NONE) {
   width = ALIGN_POT(width, 2U);
               pvr_csb_pack (&state->regs.isp_mtile_size, CR_ISP_MTILE_SIZE, reg) {
      reg.x = width;
               pvr_csb_pack (&state->regs.isp_render_origin, CR_ISP_RENDER_ORIGIN, reg) {
      reg.x = origin_x;
               pvr_setup_tiles_in_flight(dev_info,
                           dev_runtime_info,
   pvr_cr_isp_aa_mode_type(samples),
            pvr_csb_pack (&state->regs.isp_ctl, CR_ISP_CTL, reg) {
               if (PVR_HAS_FEATURE(dev_info, paired_tiles)) {
      if (state->pair_tiles == PVR_PAIRED_TILES_X) {
         } else if (state->pair_tiles == PVR_PAIRED_TILES_Y) {
      reg.pair_tiles = true;
                                 }
      static bool
   pvr_int_pbe_pixel_changes_dst_rate(const struct pvr_device_info *dev_info,
         {
      /* We don't emulate rate change from the USC with the pbe_yuv feature. */
   if (!PVR_HAS_FEATURE(dev_info, pbe_yuv) &&
      (pbe_format == PVR_TRANSFER_PBE_PIXEL_SRC_Y_UV_INTERLEAVED ||
   pbe_format == PVR_TRANSFER_PBE_PIXEL_SRC_Y_U_V)) {
                  }
      /**
   * Number of DWORDs from the unified store that floating texture coefficients
   * take up.
   */
   static void pvr_uv_space(const struct pvr_device_info *dev_info,
               {
      const struct pvr_transfer_cmd_surface *dst = &transfer_cmd->dst;
            /* This also avoids division by 0 in pvr_dma_texture_floats(). */
   if (state->custom_mapping.pass_count == 0U &&
      (dst_rect->extent.width == 0U || dst_rect->extent.height == 0U ||
   MAX2(dst_rect->offset.x, dst_rect->offset.x + dst_rect->extent.width) <
         MIN2(dst_rect->offset.x, dst_rect->offset.x + dst_rect->extent.width) >
         MAX2(dst_rect->offset.y, dst_rect->offset.y + dst_rect->extent.height) <
         MIN2(dst_rect->offset.y, dst_rect->offset.y + dst_rect->extent.height) >
            } else {
               if (transfer_cmd->source_count > 0) {
                     const VkRect2D *src_rect =
         const VkRect2D *dst_rect =
         int32_t dst_x1 = dst_rect->offset.x + dst_rect->extent.width;
   int32_t dst_y1 = dst_rect->offset.y + dst_rect->extent.height;
                           if (state->filter[0U] > PVR_FILTER_POINT) {
         } else if (src_rect->extent.width == 0U ||
               } else if ((src_rect->offset.x * dst_x1 !=
                  (src_rect->offset.y * dst_y1 !=
         (src_rect->extent.width != dst_rect->extent.width) ||
   (src_rect->extent.height != dst_rect->extent.height) ||
   transfer_cmd->sources[0U].mappings[0U].flip_x ||
      } else {
                  /* We have to adjust the rate. */
   if (layer->layer_floats != PVR_INT_COORD_SET_FLOATS_0 &&
      pvr_int_pbe_pixel_changes_dst_rate(dev_info, layer->pbe_format)) {
               }
      static uint32_t pvr_int_pbe_pixel_num_sampler_and_image_states(
         {
      switch (pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_Y_UV_INTERLEAVED:
   case PVR_TRANSFER_PBE_PIXEL_SRC_Y_U_V:
         default:
            }
      static VkResult pvr_sampler_state_for_surface(
      const struct pvr_device_info *dev_info,
   const struct pvr_transfer_cmd_surface *surface,
   enum pvr_filter filter,
   const struct pvr_tq_frag_sh_reg_layout *sh_reg_layout,
   uint32_t sampler,
      {
               pvr_csb_pack (&sampler_state[0U], TEXSTATE_SAMPLER, reg) {
      reg.anisoctl = PVRX(TEXSTATE_ANISOCTL_DISABLED);
   reg.minlod = PVRX(TEXSTATE_CLAMP_MIN);
   reg.maxlod = PVRX(TEXSTATE_CLAMP_MIN);
            if (filter == PVR_FILTER_DONTCARE || filter == PVR_FILTER_POINT) {
      reg.minfilter = PVRX(TEXSTATE_FILTER_POINT);
      } else if (filter == PVR_FILTER_LINEAR) {
      reg.minfilter = PVRX(TEXSTATE_FILTER_LINEAR);
      } else {
      assert(PVR_HAS_FEATURE(dev_info, tf_bicubic_filter));
   reg.minfilter = PVRX(TEXSTATE_FILTER_BICUBIC);
               reg.addrmode_u = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
            if (surface->mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
                        assert(sampler <= sh_reg_layout->combined_image_samplers.count);
                        }
      static inline VkResult pvr_image_state_set_codegen_defaults(
      struct pvr_device *device,
   struct pvr_transfer_3d_state *state,
   const struct pvr_transfer_cmd_surface *surface,
   uint32_t load,
      {
      struct pvr_tq_layer_properties *layer = &state->shader_props.layer_props;
   struct pvr_texture_state_info info = { 0U };
            switch (surface->vk_format) {
   /* ERN 46863 */
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
      switch (layer->pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW64:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D32S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_CONV_D32_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32_D24S8:
      info.format = VK_FORMAT_R32G32_UINT;
      default:
         }
         case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
      info.format = VK_FORMAT_R32_UINT;
         default:
      info.format = surface->vk_format;
               info.flags = 0U;
   info.base_level = 0U;
   info.mip_levels = 1U;
   info.mipmaps_present = false;
            if (surface->mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
         else
            if (PVR_HAS_FEATURE(&device->pdevice->dev_info, tpu_array_textures))
            result = pvr_mem_layout_spec(surface,
                              load,
   true,
      if (result != VK_SUCCESS)
            if (state->custom_mapping.texel_extend_dst > 1U) {
      info.extent.width /= state->custom_mapping.texel_extend_dst;
               info.tex_state_type = PVR_TEXTURE_STATE_SAMPLE;
   memcpy(info.swizzle,
                  if (surface->vk_format == VK_FORMAT_S8_UINT) {
      info.swizzle[0U] = PIPE_SWIZZLE_X;
   info.swizzle[1U] = PIPE_SWIZZLE_0;
   info.swizzle[2U] = PIPE_SWIZZLE_0;
               if (info.extent.depth > 0U)
         else if (info.extent.height > 1U)
         else
            result = pvr_pack_tex_state(device, &info, mem_ptr);
   if (result != VK_SUCCESS)
               }
      static VkResult pvr_image_state_for_surface(
      const struct pvr_transfer_ctx *ctx,
   const struct pvr_transfer_cmd *transfer_cmd,
   const struct pvr_transfer_cmd_surface *surface,
   uint32_t load,
   uint32_t source,
   const struct pvr_tq_frag_sh_reg_layout *sh_reg_layout,
   struct pvr_transfer_3d_state *state,
   uint32_t uf_image,
      {
      uint32_t tex_state[ROGUE_MAXIMUM_IMAGE_STATE_SIZE] = { 0U };
   VkResult result;
            result = pvr_image_state_set_codegen_defaults(ctx->device,
                           if (result != VK_SUCCESS)
                     /* Offset of the shared registers containing the hardware image state. */
   assert(uf_image < sh_reg_layout->combined_image_samplers.count);
            /* Copy the image state to the buffer which is loaded into the shared
   * registers.
   */
               }
      /* Writes the texture state/sampler state into DMAed memory. */
   static VkResult
   pvr_sampler_image_state(struct pvr_transfer_ctx *ctx,
                           {
      if (!state->empty_dst) {
      uint32_t uf_sampler = 0U;
            for (uint32_t source = 0; source < transfer_cmd->source_count; source++) {
      struct pvr_tq_layer_properties *layer =
                  for (uint32_t load = 0U; load < max_load; load++) {
      const struct pvr_transfer_cmd_surface *surface;
                  switch (layer->pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D24S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D32S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32S8_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D24S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D24S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D32_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F16F16:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F16_U8:
      if (load > 0U) {
      surface = &transfer_cmd->dst;
      } else {
      surface = &transfer_cmd->sources[source].surface;
                  case PVR_TRANSFER_PBE_PIXEL_SRC_Y_UV_INTERLEAVED:
   case PVR_TRANSFER_PBE_PIXEL_SRC_Y_U_V:
                        default:
      surface = &transfer_cmd->sources[source + load].surface;
                     if (load < pvr_int_pbe_pixel_num_sampler_and_image_states(
                           result = pvr_sampler_state_for_surface(dev_info,
                                                         result = pvr_image_state_for_surface(ctx,
                                       transfer_cmd,
                                                }
      /* The returned offset is in dwords. */
   static inline uint32_t pvr_dynamic_const_reg_advance(
      const struct pvr_tq_frag_sh_reg_layout *sh_reg_layout,
      {
                           }
      /** Scales coefficients for sampling. (non normalized). */
   static inline void
   pvr_dma_texture_floats(const struct pvr_transfer_cmd *transfer_cmd,
                        {
      if (transfer_cmd->source_count > 0) {
      struct pvr_tq_layer_properties *layer = &state->shader_props.layer_props;
   const struct pvr_rect_mapping *mapping =
         VkRect2D src_rect = mapping->src_rect;
            switch (layer->layer_floats) {
   case PVR_INT_COORD_SET_FLOATS_0:
            case PVR_INT_COORD_SET_FLOATS_6:
   case PVR_INT_COORD_SET_FLOATS_4: {
      int32_t consts[2U] = { 0U, 0U };
   int32_t denom[2U] = { 0U, 0U };
   int32_t nums[2U] = { 0U, 0U };
   int32_t src_x, dst_x;
   int32_t src_y, dst_y;
                  dst_x = mapping->flip_x ? -(int32_t)dst_rect.extent.width
         dst_y = mapping->flip_y ? -(int32_t)dst_rect.extent.height
                        nums[0U] = src_x;
   denom[0U] = dst_x;
   consts[0U] =
      mapping->flip_x
      ? src_rect.offset.x * dst_x -
         nums[1U] = src_y;
   denom[1U] = dst_y;
   consts[1U] =
      mapping->flip_y
                     for (uint32_t i = 0U; i < 2U; i++) {
      tmp = (float)(nums[i]) / (float)(denom[i]);
                  tmp = ((float)(consts[i]) + (i == 1U ? offset : 0.0f)) /
         mem_ptr[pvr_dynamic_const_reg_advance(sh_reg_layout, state)] =
               if (layer->layer_floats == PVR_INT_COORD_SET_FLOATS_6) {
      tmp = (float)MIN2(dst_rect.offset.x, dst_rect.offset.x + dst_x);
                  tmp = (float)MIN2(dst_rect.offset.y, dst_rect.offset.y + dst_y);
   mem_ptr[pvr_dynamic_const_reg_advance(sh_reg_layout, state)] =
      }
               default:
      unreachable("Unknown COORD_SET_FLOATS.");
            }
      static bool pvr_int_pbe_pixel_requires_usc_filter(
      const struct pvr_device_info *dev_info,
      {
      switch (pixel_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_SMRG_D24S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_DMRG_D24S8_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_U16NORM:
   case PVR_TRANSFER_PBE_PIXEL_SRC_S16NORM:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32X2:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32X4:
         case PVR_TRANSFER_PBE_PIXEL_SRC_F16F16:
         default:
            }
      /**
   * Sets up the MSAA related bits in the operation
   *
   * TPU sample count is read directly from transfer_cmd in the TPU code. An MSAA
   * src can be read from sample rate or instance rate shaders as long as the
   * sample count is set on the TPU. If a layer is single sample we expect the
   * same sample replicated in full rate shaders. If the layer is multi sample,
   * instance rate shaders are used to emulate the filter or to select the
   * specified sample. The sample number is static in the programs.
   */
   static VkResult pvr_msaa_state(const struct pvr_device_info *dev_info,
                     {
      struct pvr_tq_shader_properties *shader_props = &state->shader_props;
   struct pvr_tq_layer_properties *layer = &shader_props->layer_props;
   struct pvr_winsys_transfer_regs *const regs = &state->regs;
   uint32_t src_sample_count =
         uint32_t dst_sample_count = transfer_cmd->dst.sample_count & ~1U;
            shader_props->full_rate = false;
   state->msaa_multiplier = 1U;
            /* clang-format off */
   pvr_csb_pack (&regs->isp_aa, CR_ISP_AA, reg);
            layer->sample_count = 1U;
                     if (bsample_count > PVR_GET_FEATURE_VALUE(dev_info, max_multisample, 0U))
            /* Shouldn't get two distinct bits set (implies different sample counts).
   * The reason being the rate at which the shader runs has to match.
   */
   if ((bsample_count & (bsample_count - 1U)) != 0U)
            if (src_sample_count == 0U && dst_sample_count == 0U) {
      /* S -> S (no MSAA involved). */
      } else if (src_sample_count != 0U && dst_sample_count == 0U) {
      /* M -> S (resolve). */
            if ((uint32_t)layer->resolve_op >=
      (src_sample_count + (uint32_t)PVR_RESOLVE_SAMPLE0)) {
   return vk_error(transfer_cmd->cmd_buffer,
                        switch (layer->resolve_op) {
   case PVR_RESOLVE_MIN:
   case PVR_RESOLVE_MAX:
      switch (transfer_cmd->sources[source].surface.vk_format) {
   case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_S8_UINT:
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
      if (transfer_cmd->sources[source].surface.vk_format !=
      transfer_cmd->dst.vk_format) {
   return vk_error(transfer_cmd->cmd_buffer,
                  default:
      return vk_error(transfer_cmd->cmd_buffer,
               /* Instance rate. */
   layer->sample_count = src_sample_count;
               case PVR_RESOLVE_BLEND:
      if (pvr_int_pbe_pixel_requires_usc_filter(dev_info,
            /* Instance rate. */
   layer->sample_count = src_sample_count;
      } else {
      /* Sample rate. */
   state->shader_props.full_rate = true;
                  pvr_csb_pack (&regs->isp_aa, CR_ISP_AA, reg) {
                        default:
      /* Shader doesn't have to know the number of samples. It's enough
   * if the TPU knows, and the shader sets the right sno (given to the
   * shader in resolve_op).
   */
   state->shader_props.full_rate = false;
         } else {
               pvr_csb_pack (&regs->isp_aa, CR_ISP_AA, reg) {
                  if (src_sample_count == 0U && dst_sample_count != 0U) {
      /* S -> M (replicate samples) */
   layer->msaa = false;
      } else {
      /* M -> M (sample to sample) */
   layer->msaa = true;
                     }
      static bool pvr_requires_usc_linear_filter(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_R32_SFLOAT:
   case VK_FORMAT_R32G32_SFLOAT:
   case VK_FORMAT_R32G32B32_SFLOAT:
   case VK_FORMAT_R32G32B32A32_SFLOAT:
   case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
         default:
            }
      static inline bool
   pvr_int_pbe_usc_linear_filter(enum pvr_transfer_pbe_pixel_src pbe_format,
                     {
      if (sample || msaa || full_rate)
            switch (pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_D24S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_S8D24:
   case PVR_TRANSFER_PBE_PIXEL_SRC_D32S8:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32X2:
   case PVR_TRANSFER_PBE_PIXEL_SRC_F32X4:
         default:
            }
      static inline bool pvr_pick_component_needed(
         {
      return custom_mapping->pass_count > 0U &&
            }
      /** Writes the shader related constants into the DMA space. */
   static void
   pvr_write_usc_constants(const struct pvr_tq_frag_sh_reg_layout *sh_reg_layout,
         {
      const uint32_t reg = sh_reg_layout->driver_total;
   const uint32_t consts_count =
            /* If not we likely need to write more consts. */
            /* Append the usc consts after the driver allocated regs. */
   for (uint32_t i = 0U; i < consts_count; i++)
      }
      static inline void
   pvr_dma_texel_unwind(struct pvr_transfer_3d_state *state,
                  {
      const uint32_t coord_sample_mask =
            mem_ptr[pvr_dynamic_const_reg_advance(sh_reg_layout, state)] =
         mem_ptr[pvr_dynamic_const_reg_advance(sh_reg_layout, state)] =
      }
      /** Writes the Uniform/Texture state data segments + the UniTex code. */
   static inline VkResult
   pvr_pds_unitex(const struct pvr_device_info *dev_info,
                  struct pvr_transfer_ctx *ctx,
      {
      struct pvr_pds_upload *unitex_code =
      &ctx->pds_unitex_code[program->num_texture_dma_kicks]
      struct pvr_transfer_3d_state *state = &prep_data->state;
   struct pvr_suballoc_bo *pvr_bo;
   VkResult result;
            /* Uniform program is not used. */
            if (program->num_texture_dma_kicks == 0U) {
      state->uniform_data_size = 0U;
   state->tex_state_data_size = 0U;
   state->tex_state_data_offset = 0U;
                        pvr_pds_set_sizes_pixel_shader_sa_uniform_data(program, dev_info);
   assert(program->data_size == 0U);
            pvr_pds_set_sizes_pixel_shader_sa_texture_data(program, dev_info);
   state->tex_state_data_size =
      ALIGN_POT(program->data_size,
         result =
      pvr_cmd_buffer_alloc_mem(transfer_cmd->cmd_buffer,
                  if (result != VK_SUCCESS)
            state->tex_state_data_offset =
            map = pvr_bo_suballoc_get_map_addr(pvr_bo);
            /* Save the dev_addr and size in the 3D state. */
   state->uni_tex_code_offset = unitex_code->code_offset;
               }
      /** Converts a float in range 0 to 1 to an N-bit fixed-point integer. */
   static uint32_t pvr_float_to_ufixed(float value, uint32_t bits)
   {
               /* NaN and Inf and overflow. */
   if (util_is_inf_or_nan(value) || value >= 1.0f)
         else if (value < 0.0f)
            /* Normalise. */
            /* Cast to double so that we can accurately represent the sum for N > 23. */
      }
      /** Converts a float in range -1 to 1 to a signed N-bit fixed-point integer. */
   static uint32_t pvr_float_to_sfixed(float value, uint32_t N)
   {
      int32_t max = (1 << (N - 1)) - 1;
   int32_t min = 0 - (1 << (N - 1));
            /* NaN and Inf and overflow. */
   if (util_is_inf_or_nan(value) || value >= 1.0f)
         else if (value == 0.0f)
         else if (value <= -1.0f)
            /* Normalise. */
            /* Cast to double so that we can accurately represent the sum for N > 23. */
   if (value > 0.0f)
         else
               }
      /** Convert a value in IEEE single precision format to 16-bit floating point
   * format.
   */
   /* TODO: See if we can use _mesa_float_to_float16_rtz_slow() instead. */
   static uint16_t pvr_float_to_f16(float value, bool round_to_even)
   {
      uint32_t input_value;
   uint32_t exponent;
   uint32_t mantissa;
            /* 0.0f can be exactly expressed in binary using IEEE float format. */
   if (value == 0.0f)
            if (value < 0U) {
      output = 0x8000;
      } else {
                  /* 2^16 * (2 - 1/1024) = highest f16 representable value. */
   value = MIN2(value, 131008);
            /* Extract the exponent and mantissa. */
   exponent = util_get_float32_exponent(value) + 15;
            /* If the exponent is outside the supported range then denormalise the
   * mantissa.
   */
   if ((int32_t)exponent <= 0) {
               mantissa |= (1 << 23);
   exponent = input_value >> 23;
            if (shift < 24)
         else
      } else {
                           if (round_to_even) {
      /* Round to nearest even. */
   if ((((int)value) % 2 != 0) && (((1 << 13) - 1) & mantissa))
      } else {
      /* Round to nearest. */
   if (mantissa & (1 << 12))
                  }
      static VkResult pvr_pack_clear_color(VkFormat format,
               {
      const uint32_t red_width =
         uint32_t pbe_pack_mode = pvr_get_pbe_packmode(format);
            if (pbe_pack_mode == PVRX(PBESTATE_PACKMODE_INVALID))
            /* Set packed color based on PBE pack mode and PBE norm. */
   switch (pbe_pack_mode) {
   case PVRX(PBESTATE_PACKMODE_U8U8U8U8):
   case PVRX(PBESTATE_PACKMODE_A8R3G3B2):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 8) & 0xFFU;
   pkd_color[0] |= (pvr_float_to_ufixed(color[1].f, 8) & 0xFFU) << 8;
   pkd_color[0] |= (pvr_float_to_ufixed(color[2].f, 8) & 0xFFU) << 16;
      } else {
      pkd_color[0] = color[0].ui & 0xFFU;
   pkd_color[0] |= (color[1].ui & 0xFFU) << 8;
   pkd_color[0] |= (color[2].ui & 0xFFU) << 16;
      }
         case PVRX(PBESTATE_PACKMODE_S8S8S8S8):
   case PVRX(PBESTATE_PACKMODE_X8U8S8S8):
   case PVRX(PBESTATE_PACKMODE_X8S8S8U8):
      if (pbe_norm) {
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, false);
   pkd_color[0] |= (uint32_t)pvr_float_to_f16(color[1].f, false) << 16;
   pkd_color[1] = (uint32_t)pvr_float_to_f16(color[2].f, false);
      } else {
      pkd_color[0] = color[0].ui & 0xFFU;
   pkd_color[0] |= (color[1].ui & 0xFFU) << 8;
   pkd_color[0] |= (color[2].ui & 0xFFU) << 16;
      }
         case PVRX(PBESTATE_PACKMODE_U16U16U16U16):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 16) & 0xFFFFU;
   pkd_color[0] |= (pvr_float_to_ufixed(color[1].f, 16) & 0xFFFFU) << 16;
   pkd_color[1] = pvr_float_to_ufixed(color[2].f, 16) & 0xFFFFU;
      } else {
      pkd_color[0] = color[0].ui & 0xFFFFU;
   pkd_color[0] |= (color[1].ui & 0xFFFFU) << 16;
   pkd_color[1] = color[2].ui & 0xFFFFU;
      }
         case PVRX(PBESTATE_PACKMODE_S16S16S16S16):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_sfixed(color[0].f, 16) & 0xFFFFU;
   pkd_color[0] |= (pvr_float_to_sfixed(color[1].f, 16) & 0xFFFFU) << 16;
   pkd_color[1] = (pvr_float_to_sfixed(color[2].f, 16) & 0xFFFFU);
      } else {
      pkd_color[0] = color[0].ui & 0xFFFFU;
   pkd_color[0] |= (color[1].ui & 0xFFFFU) << 16;
   pkd_color[1] = color[2].ui & 0xFFFFU;
      }
         case PVRX(PBESTATE_PACKMODE_A2_XRBIAS_U10U10U10):
   case PVRX(PBESTATE_PACKMODE_ARGBV16_XR10):
   case PVRX(PBESTATE_PACKMODE_F16F16F16F16):
   case PVRX(PBESTATE_PACKMODE_A2R10B10G10):
   case PVRX(PBESTATE_PACKMODE_A4R4G4B4):
   case PVRX(PBESTATE_PACKMODE_A1R5G5B5):
   case PVRX(PBESTATE_PACKMODE_R5G5B5A1):
   case PVRX(PBESTATE_PACKMODE_R5G6B5):
      if (red_width > 0) {
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, false);
   pkd_color[0] |= (uint32_t)pvr_float_to_f16(color[1].f, false) << 16;
   pkd_color[1] = (uint32_t)pvr_float_to_f16(color[2].f, false);
      } else {
      /* Swizzle only uses first channel for alpha formats. */
      }
         case PVRX(PBESTATE_PACKMODE_U32U32U32U32):
      pkd_color[0] = color[0].ui;
   pkd_color[1] = color[1].ui;
   pkd_color[2] = color[2].ui;
   pkd_color[3] = color[3].ui;
         case PVRX(PBESTATE_PACKMODE_S32S32S32S32):
      pkd_color[0] = (uint32_t)color[0].i;
   pkd_color[1] = (uint32_t)color[1].i;
   pkd_color[2] = (uint32_t)color[2].i;
   pkd_color[3] = (uint32_t)color[3].i;
         case PVRX(PBESTATE_PACKMODE_F32F32F32F32):
      memcpy(pkd_color, &color[0].f, 4U * sizeof(float));
         case PVRX(PBESTATE_PACKMODE_R10B10G10A2):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 10) & 0xFFU;
   pkd_color[0] |= (pvr_float_to_ufixed(color[1].f, 10) & 0xFFU) << 10;
   pkd_color[0] |= (pvr_float_to_ufixed(color[2].f, 10) & 0xFFU) << 20;
      } else if (format == VK_FORMAT_A2R10G10B10_UINT_PACK32) {
      pkd_color[0] = color[2].ui & 0x3FFU;
   pkd_color[0] |= (color[1].ui & 0x3FFU) << 10;
   pkd_color[0] |= (color[0].ui & 0x3FFU) << 20;
      } else {
      pkd_color[0] = color[0].ui & 0x3FFU;
   pkd_color[0] |= (color[1].ui & 0x3FFU) << 10;
   pkd_color[0] |= (color[2].ui & 0x3FFU) << 20;
                     case PVRX(PBESTATE_PACKMODE_A2F10F10F10):
   case PVRX(PBESTATE_PACKMODE_F10F10F10A2):
      pkd_color[0] = pvr_float_to_sfixed(color[0].f, 10) & 0xFFU;
   pkd_color[0] |= (pvr_float_to_sfixed(color[1].f, 10) & 0xFFU) << 10;
   pkd_color[0] |= (pvr_float_to_sfixed(color[2].f, 10) & 0xFFU) << 20;
   pkd_color[0] |= (pvr_float_to_sfixed(color[3].f, 2) & 0xFFU) << 30;
         case PVRX(PBESTATE_PACKMODE_U8U8U8):
   case PVRX(PBESTATE_PACKMODE_R5SG5SB6):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 8) & 0xFFU;
   pkd_color[0] |= (pvr_float_to_ufixed(color[1].f, 8) & 0xFFU) << 8;
      } else {
      pkd_color[0] = color[0].ui & 0xFFU;
   pkd_color[0] |= (color[1].ui & 0xFFU) << 8;
      }
         case PVRX(PBESTATE_PACKMODE_S8S8S8):
   case PVRX(PBESTATE_PACKMODE_B6G5SR5S):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_sfixed(color[0].f, 8) & 0xFFU;
   pkd_color[0] |= (pvr_float_to_sfixed(color[1].f, 8) & 0xFFU) << 8;
      } else {
      pkd_color[0] = color[0].ui & 0xFFU;
   pkd_color[0] |= (color[1].ui & 0xFFU) << 8;
      }
         case PVRX(PBESTATE_PACKMODE_U16U16U16):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 16) & 0xFFFFU;
   pkd_color[0] |= (pvr_float_to_ufixed(color[1].f, 16) & 0xFFFFU) << 16;
      } else {
      pkd_color[0] = color[0].ui & 0xFFFFU;
   pkd_color[0] |= (color[1].ui & 0xFFFFU) << 16;
      }
         case PVRX(PBESTATE_PACKMODE_S16S16S16):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_sfixed(color[0].f, 16) & 0xFFFFU;
   pkd_color[0] |= (pvr_float_to_sfixed(color[1].f, 16) & 0xFFFFU) << 16;
      } else {
      pkd_color[0] = color[0].ui & 0xFFFFU;
   pkd_color[0] |= (color[1].ui & 0xFFFFU) << 16;
      }
         case PVRX(PBESTATE_PACKMODE_F16F16F16):
   case PVRX(PBESTATE_PACKMODE_F11F11F10):
   case PVRX(PBESTATE_PACKMODE_F10F11F11):
   case PVRX(PBESTATE_PACKMODE_SE9995):
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, true);
   pkd_color[0] |= (uint32_t)pvr_float_to_f16(color[1].f, true) << 16;
   pkd_color[1] = (uint32_t)pvr_float_to_f16(color[2].f, true);
         case PVRX(PBESTATE_PACKMODE_U32U32U32):
      pkd_color[0] = color[0].ui;
   pkd_color[1] = color[1].ui;
   pkd_color[2] = color[2].ui;
         case PVRX(PBESTATE_PACKMODE_S32S32S32):
      pkd_color[0] = (uint32_t)color[0].i;
   pkd_color[1] = (uint32_t)color[1].i;
   pkd_color[2] = (uint32_t)color[2].i;
         case PVRX(PBESTATE_PACKMODE_X24G8X32):
   case PVRX(PBESTATE_PACKMODE_U8X24):
      pkd_color[1] = (color[1].ui & 0xFFU) << 24;
         case PVRX(PBESTATE_PACKMODE_F32F32F32):
      memcpy(pkd_color, &color[0].f, 3U * sizeof(float));
         case PVRX(PBESTATE_PACKMODE_U8U8):
      if (pbe_norm) {
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, false);
      } else {
      pkd_color[0] = color[0].ui & 0xFFU;
      }
         case PVRX(PBESTATE_PACKMODE_S8S8):
      if (pbe_norm) {
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, false);
      } else {
      pkd_color[0] = color[0].ui & 0xFFU;
   pkd_color[0] |= (color[1].ui & 0xFFU) << 8;
   pkd_color[0] |= (color[2].ui & 0xFFU) << 16;
      }
         case PVRX(PBESTATE_PACKMODE_U16U16):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 16) & 0xFFFFU;
      } else {
      pkd_color[0] = color[0].ui & 0xFFFFU;
      }
         case PVRX(PBESTATE_PACKMODE_S16S16):
      if (pbe_norm) {
      pkd_color[0] = pvr_float_to_sfixed(color[0].f, 16) & 0xFFFFU;
      } else {
      pkd_color[0] = color[0].ui & 0xFFFFU;
      }
         case PVRX(PBESTATE_PACKMODE_F16F16):
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, true);
   pkd_color[0] |= (uint32_t)pvr_float_to_f16(color[1].f, true) << 16;
         case PVRX(PBESTATE_PACKMODE_U32U32):
      pkd_color[0] = color[0].ui;
   pkd_color[1] = color[1].ui;
         case PVRX(PBESTATE_PACKMODE_S32S32):
      pkd_color[0] = (uint32_t)color[0].i;
   pkd_color[1] = (uint32_t)color[1].i;
         case PVRX(PBESTATE_PACKMODE_X24U8F32):
   case PVRX(PBESTATE_PACKMODE_X24X8F32):
      memcpy(pkd_color, &color[0].f, 1U * sizeof(float));
   pkd_color[1] = color[1].ui & 0xFFU;
         case PVRX(PBESTATE_PACKMODE_F32F32):
      memcpy(pkd_color, &color[0].f, 2U * sizeof(float));
         case PVRX(PBESTATE_PACKMODE_ST8U24):
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 24) & 0xFFFFFFU;
   pkd_color[0] |= color[1].ui << 24;
         case PVRX(PBESTATE_PACKMODE_U8):
      if (format == VK_FORMAT_S8_UINT)
         else if (pbe_norm)
         else
                  case PVRX(PBESTATE_PACKMODE_S8):
      if (pbe_norm)
         else
               case PVRX(PBESTATE_PACKMODE_U16):
      if (pbe_norm)
         else
               case PVRX(PBESTATE_PACKMODE_S16):
      if (pbe_norm)
         else
               case PVRX(PBESTATE_PACKMODE_F16):
      pkd_color[0] = (uint32_t)pvr_float_to_f16(color[0].f, true);
         /* U32 */
   case PVRX(PBESTATE_PACKMODE_U32):
      if (format == VK_FORMAT_X8_D24_UNORM_PACK32) {
         } else if (format == VK_FORMAT_D24_UNORM_S8_UINT) {
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 24) & 0xFFFFFFU;
      } else if (format == VK_FORMAT_A2B10G10R10_UINT_PACK32) {
      pkd_color[0] = color[0].ui & 0x3FFU;
   pkd_color[0] |= (color[1].ui & 0x3FFU) << 10;
   pkd_color[0] |= (color[2].ui & 0x3FFU) << 20;
      } else {
         }
         /* U24ST8 */
   case PVRX(PBESTATE_PACKMODE_U24ST8):
      pkd_color[1] = (color[1].ui & 0xFFU) << 24;
   pkd_color[1] |= pvr_float_to_ufixed(color[0].f, 24) & 0xFFFFFFU;
         /* S32 */
   case PVRX(PBESTATE_PACKMODE_S32):
      pkd_color[0] = (uint32_t)color[0].i;
         /* F32 */
   case PVRX(PBESTATE_PACKMODE_F32):
      memcpy(pkd_color, &color[0].f, sizeof(float));
         /* X8U24 */
   case PVRX(PBESTATE_PACKMODE_X8U24):
      pkd_color[0] = pvr_float_to_ufixed(color[0].f, 24) & 0xFFFFFFU;
         default:
                     }
      static VkResult
   pvr_isp_scan_direction(struct pvr_transfer_cmd *transfer_cmd,
               {
      pvr_dev_addr_t dst_dev_addr = transfer_cmd->dst.dev_addr;
   bool backwards_in_x = false;
   bool backwards_in_y = false;
   bool done_dest_rect = false;
   VkRect2D dst_rect;
   int32_t dst_x1;
            for (uint32_t i = 0; i < transfer_cmd->source_count; i++) {
      struct pvr_transfer_cmd_source *src = &transfer_cmd->sources[i];
            if (src_dev_addr.addr == dst_dev_addr.addr && !custom_mapping) {
      VkRect2D *src_rect = &src->mappings[0].src_rect;
                                                               if ((dst_rect.offset.x < src_x1 && dst_x1 > src_rect->offset.x) &&
      (dst_rect.offset.y < src_y1 && dst_y1 > src_rect->offset.y)) {
   if (src_rect->extent.width != dst_rect.extent.width ||
      src_rect->extent.height != dst_rect.extent.height) {
                                    /* Direction is to the bottom. */
                     if (backwards_in_x) {
      if (backwards_in_y)
         else
      } else {
      if (backwards_in_y)
         else
                  }
      static VkResult pvr_3d_copy_blit_core(struct pvr_transfer_ctx *ctx,
                           {
      struct pvr_transfer_3d_state *const state = &prep_data->state;
   struct pvr_winsys_transfer_regs *const regs = &state->regs;
   struct pvr_device *const device = ctx->device;
                              state->common_ptr = 0U;
   state->dynamic_const_reg_ptr = 0U;
            if ((transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_FILL) != 0U) {
               if (transfer_cmd->source_count != 0U)
            if (vk_format_is_compressed(transfer_cmd->dst.vk_format))
            /* No shader. */
   state->pds_temps = 0U;
   state->uniform_data_size = 0U;
            /* No background enabled. */
   /* clang-format off */
   pvr_csb_pack (&regs->isp_bgobjvals, CR_ISP_BGOBJVALS, reg);
   /* clang-format on */
   pvr_csb_pack (&regs->isp_aa, CR_ISP_AA, reg) {
                  result = pvr_pack_clear_color(transfer_cmd->dst.vk_format,
               if (result != VK_SUCCESS)
            pvr_csb_pack (&regs->usc_clear_register0, CR_USC_CLEAR_REGISTER, reg) {
                  pvr_csb_pack (&regs->usc_clear_register1, CR_USC_CLEAR_REGISTER, reg) {
                  pvr_csb_pack (&regs->usc_clear_register2, CR_USC_CLEAR_REGISTER, reg) {
                  pvr_csb_pack (&regs->usc_clear_register3, CR_USC_CLEAR_REGISTER, reg) {
                  state->msaa_multiplier = transfer_cmd->dst.sample_count & ~1U;
   state->pds_shader_task_offset = 0U;
   state->uni_tex_code_offset = 0U;
      } else if (transfer_cmd->source_count > 0U) {
      const struct pvr_tq_frag_sh_reg_layout nop_sh_reg_layout = {
      /* TODO: Setting this to 1 so that we don't try to pvr_bo_alloc() with
   * zero size. The device will ignore the PDS program if USC_SHAREDSIZE
   * is zero and in the case of the nop shader we're expecting it to be
   * zero. See if we can safely pass PVR_DEV_ADDR_INVALID for the unitex
   * program.
   */
      };
   const struct pvr_tq_frag_sh_reg_layout *sh_reg_layout;
   struct pvr_pds_pixel_shader_sa_program unitex_prog = { 0U };
   uint32_t tex_state_dma_size_dw;
   struct pvr_suballoc_bo *pvr_bo;
            result = pvr_pbe_src_format(transfer_cmd, state, &state->shader_props);
   if (result != VK_SUCCESS)
                              state->shader_props.layer_props.sample =
                  result = pvr_msaa_state(dev_info, transfer_cmd, state, 0);
   if (result != VK_SUCCESS)
            state->shader_props.pick_component =
            if (state->filter[0] == PVR_FILTER_LINEAR &&
      pvr_requires_usc_linear_filter(
         if (pvr_int_pbe_usc_linear_filter(
         state->shader_props.layer_props.pbe_format,
   state->shader_props.layer_props.sample,
   state->shader_props.layer_props.msaa,
         } else {
                     if (state->empty_dst) {
      sh_reg_layout = &nop_sh_reg_layout;
      } else {
               result =
      pvr_transfer_frag_store_get_shader_info(device,
                                       assert(kick_usc_pds_dev_addr.addr <= UINT32_MAX);
               unitex_prog.kick_usc = false;
            tex_state_dma_size_dw =
            unitex_prog.num_texture_dma_kicks = 1U;
            result = pvr_cmd_buffer_alloc_mem(transfer_cmd->cmd_buffer,
                     if (result != VK_SUCCESS)
                     result = pvr_sampler_image_state(ctx,
                           if (result != VK_SUCCESS)
                     if (transfer_cmd->sources[0].surface.mem_layout ==
      PVR_MEMLAYOUT_3DTWIDDLED) {
   dma_space[pvr_dynamic_const_reg_advance(sh_reg_layout, state)] =
                        if (pvr_pick_component_needed(&state->custom_mapping))
            pvr_pds_encode_dma_burst(unitex_prog.texture_dma_control,
                           unitex_prog.texture_dma_address,
                     result =
         if (result != VK_SUCCESS)
            pvr_csb_pack (&regs->isp_bgobjvals, CR_ISP_BGOBJVALS, reg) {
                  /* clang-format off */
   pvr_csb_pack (&regs->isp_aa, CR_ISP_AA, reg);
      } else {
      /* No shader. */
   state->pds_temps = 0U;
   state->uniform_data_size = 0U;
            /* No background enabled. */
   /* clang-format off */
   pvr_csb_pack (&regs->isp_bgobjvals, CR_ISP_BGOBJVALS, reg);
   /* clang-format on */
   pvr_csb_pack (&regs->isp_aa, CR_ISP_AA, reg) {
         }
   state->msaa_multiplier = transfer_cmd->dst.sample_count & ~1U;
   state->pds_shader_task_offset = 0U;
   state->uni_tex_code_offset = 0U;
            result = pvr_pbe_src_format(transfer_cmd, state, &state->shader_props);
   if (result != VK_SUCCESS)
                        pvr_csb_pack (&regs->isp_render, CR_ISP_RENDER, reg) {
               result = pvr_isp_scan_direction(transfer_cmd,
               if (result != VK_SUCCESS)
               /* Set up pixel event handling. */
   result = pvr_pbe_setup(transfer_cmd, ctx, state);
   if (result != VK_SUCCESS)
            result = pvr_isp_tiles(device, state);
   if (result != VK_SUCCESS)
            if (PVR_HAS_FEATURE(&device->pdevice->dev_info, gpu_multicore_support)) {
      pvr_csb_pack (&regs->frag_screen, CR_FRAG_SCREEN, reg) {
      reg.xmax = transfer_cmd->dst.width - 1;
                  if ((pass_idx + 1U) < state->custom_mapping.pass_count)
               }
      static VkResult
   pvr_pbe_src_format_f2d(uint32_t merge_flags,
                        struct pvr_transfer_cmd_source *src,
      {
               /* This has to come before the rest as S8 for instance is integer and
   * signedsess check fails on D24S8.
   */
   if (vk_format_is_depth_or_stencil(src_format) ||
      vk_format_is_depth_or_stencil(dst_format) ||
   merge_flags & PVR_TRANSFER_CMD_FLAGS_DSMERGE) {
   return pvr_pbe_src_format_ds(&src->surface,
                                       return pvr_pbe_src_format_normal(src_format,
                        }
      /** Writes the coefficient loading PDS task. */
   static inline VkResult
   pvr_pds_coeff_task(struct pvr_transfer_ctx *ctx,
                     {
      struct pvr_transfer_3d_state *state = &prep_data->state;
   struct pvr_pds_coeff_loading_program program = { 0U };
   struct pvr_suballoc_bo *pvr_bo;
                     pvr_csb_pack (&program.FPU_iterators[0U],
                  if (sample_3d)
         else
                     /* Varying wrap on the TSP means that the TSP chooses the shorter path
   * out of the normal and the wrapping path i.e. chooses between u0->u1
   * and u1->1.0 == 0.0 -> u0. We don't need this behavior.
   */
   /*
   * if RHW ever needed offset SRC_F32 to the first U in 16 bit units
   * l0 U    <= offs 0
   * l0 V
   * l1 U    <= offs 4
   * ...
   */
   reg.shademodel = PVRX(PDSINST_DOUTI_SHADEMODEL_GOURUAD);
               if (sample_3d)
         else
                     result = pvr_cmd_buffer_alloc_mem(
      transfer_cmd->cmd_buffer,
   ctx->device->heaps.pds_heap,
   PVR_DW_TO_BYTES(program.data_size + program.code_size),
      if (result != VK_SUCCESS)
            state->pds_coeff_task_offset =
            pvr_pds_generate_coeff_loading_program(&program,
            state->coeff_data_size = program.data_size;
               }
      #define X 0U
   #define Y 1U
   #define Z 2U
      static void pvr_tsp_floats(const struct pvr_device_info *dev_info,
                              VkRect2D *rect,
      {
   #define U0 0U
   #define U1 1U
   #define V0 2U
   #define V1 3U
         const uint32_t indices[8U] = { U0, V0, U0, V1, U1, V1, U1, V0 };
   float delta[2U] = { 0.0f, 0.0f };
   int32_t non_normalized[4U];
   uint32_t src_flipped[2U];
   uint32_t normalized[4U];
            non_normalized[U0] = rect->offset.x;
   non_normalized[U1] = rect->offset.x + rect->extent.width;
   non_normalized[V0] = rect->offset.y;
            /* Filter adjust. */
   src_span[X] = rect->extent.width;
   src_flipped[X] = src_span[X] > 0U ? 0U : 1U;
   src_span[Y] = rect->extent.height;
   src_flipped[Y] = src_span[Y] > 0U ? 0U : 1U;
   /*
   * | X  | Y  | srcFlipX | srcFlipY |
   * +----+----+----------+----------|
   * | X  | Y  | 0        | 0        |
   * | -X | Y  | 1        | 0        |
   * | X  | -Y | 0        | 1        |
   * | -X | -Y | 1        | 1        |
   */
   for (uint32_t i = X; i <= Y; i++) {
      if (custom_filter) {
      if (src_flipped[i] != 0U)
         else
                  /* Normalize. */
   for (uint32_t i = 0U; i < ARRAY_SIZE(normalized); i++) {
      uint32_t tmp;
            ftmp = (float)non_normalized[i] + delta[i >> 1U];
            tmp = fui(ftmp);
   if (!PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format))
                        /* Apply indices. */
   for (uint32_t i = 0U; i < 8U; i++)
            if (z_present) {
               if (!PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format))
            for (uint32_t i = 8U; i < 12U; i++)
            #undef U0
   #undef U1
   #undef V0
   #undef V1
   }
      static void
   pvr_isp_prim_block_tsp_vertex_block(const struct pvr_device_info *dev_info,
                                       const struct pvr_transfer_cmd_source *src,
   {
      struct pvr_transfer_3d_iteration layer;
            /*  |<-32b->|
   *  +-------+-----
   *  |  RHW  |    | X num_isp_vertices
   *  +-------+--  |
   *  |  U    | |  |
   *  |  V    | | X PVR_TRANSFER_NUM_LAYERS
   *  +-------+-----
   *
   * RHW is not there any more in the Transfer. The comment still explains
   * where it should go if ever needed.
   */
   for (uint32_t i = mapping_offset; i < mapping_offset + num_mappings; i++) {
      bool z_present = src->surface.mem_layout == PVR_MEMLAYOUT_3DTWIDDLED;
   const float recips[3U] = {
      [X] = 1.0f / (float)src->surface.width,
   [Y] = 1.0f / (float)src->surface.height,
      };
   float z_pos = (src->filter < PVR_FILTER_LINEAR)
                  pvr_tsp_floats(dev_info,
                  &mappings[i].src_rect,
   recips,
               /* We request UVs from TSP for ISP triangle:
   *  0 u 1
   *  +---,
   * v|  /|
   *  | / |
   * 2'/--'3
   */
   for (uint32_t j = 0U; j < PVR_TRANSFER_NUM_LAYERS; j++) {
      *cs_ptr++ = layer.texture_coords[0U];
               if (z_present) {
      *cs_ptr++ = layer.texture_coords[8U];
               for (uint32_t j = 0U; j < PVR_TRANSFER_NUM_LAYERS; j++) {
      *cs_ptr++ = layer.texture_coords[6U];
               if (z_present) {
      *cs_ptr++ = layer.texture_coords[11U];
               for (uint32_t j = 0U; j < PVR_TRANSFER_NUM_LAYERS; j++) {
      *cs_ptr++ = layer.texture_coords[2U];
               if (z_present) {
      *cs_ptr++ = layer.texture_coords[9U];
               for (uint32_t j = 0U; j < PVR_TRANSFER_NUM_LAYERS; j++) {
      *cs_ptr++ = layer.texture_coords[4U];
               if (z_present) {
      *cs_ptr++ = layer.texture_coords[10U];
                  if (!PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      /* Skipped optional primitive id. */
   for (uint32_t i = 0U; i < tsp_comp_format_in_dw; i++)
      } else {
      /* Align back to 64 bits. */
   if (((uintptr_t)cs_ptr & 7U) != 0U)
                  }
      #undef X
   #undef Y
   #undef Z
      static void pvr_isp_prim_block_pds_state(const struct pvr_device_info *dev_info,
                     {
               pvr_csb_pack (cs_ptr, TA_STATE_PDS_SHADERBASE, shader_base) {
         }
            pvr_csb_pack (cs_ptr, TA_STATE_PDS_TEXUNICODEBASE, tex_base) {
         }
            pvr_csb_pack (cs_ptr, TA_STATE_PDS_SIZEINFO1, info1) {
      info1.pds_uniformsize =
                  info1.pds_texturestatesize =
                  info1.pds_varyingsize =
                  info1.usc_varyingsize =
      ALIGN_POT(state->usc_coeff_regs,
               info1.pds_tempsize =
      ALIGN_POT(state->pds_temps,
         }
            pvr_csb_pack (cs_ptr, TA_STATE_PDS_VARYINGBASE, base) {
         }
            pvr_csb_pack (cs_ptr, TA_STATE_PDS_TEXTUREDATABASE, base) {
         }
            /* PDS uniform program not used. */
   pvr_csb_pack (cs_ptr, TA_STATE_PDS_UNIFORMDATABASE, base) {
         }
            pvr_csb_pack (cs_ptr, TA_STATE_PDS_SIZEINFO2, info) {
      info.usc_sharedsize =
      ALIGN_POT(state->common_ptr,
            info.pds_tri_merge_disable = !PVR_HAS_ERN(dev_info, 42307);
      }
            /* Get back to 64 bits boundary. */
   if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format))
               }
      static void pvr_isp_prim_block_isp_state(const struct pvr_device_info *dev_info,
                                 {
      const bool has_simple_internal_parameter_format_v2 =
                  if (has_simple_internal_parameter_format_v2) {
      const uint32_t tsp_data_per_vrx_in_bytes =
            pvr_csb_pack ((uint64_t *)cs_ptr,
                                                vert_fmt.vf_varying_vertex_bits = tsp_data_per_vrx_in_bytes * 8U;
   vert_fmt.vf_primitive_total = (num_isp_vertices / 2U) - 1U;
      }
                        /* clang-format off */
   pvr_csb_pack (cs_ptr, TA_STATE_ISPCTL, ispctl);
   /* clang-format on */
            pvr_csb_pack (cs_ptr, TA_STATE_ISPA, ispa) {
      ispa.objtype = PVRX(TA_OBJTYPE_TRIANGLE);
   ispa.passtype = read_bgnd ? PVRX(TA_PASSTYPE_TRANSLUCENT)
         ispa.dcmpmode = PVRX(TA_CMPMODE_ALWAYS);
      }
            if (has_simple_internal_parameter_format_v2) {
      *cs_ptr_out = cs_ptr;
               /* How many bytes the TSP compression format needs? */
   pvr_csb_pack (cs_ptr, IPF_COMPRESSION_SIZE_WORD, word) {
      word.cs_isp_comp_table_size = 0U;
   word.cs_tsp_comp_format_size = tsp_comp_format_in_dw;
   word.cs_tsp_comp_table_size = 0U;
      }
            /* ISP vertex compression. */
   pvr_csb_pack (cs_ptr, IPF_ISP_COMPRESSION_WORD_0, word0) {
      word0.cf_isp_comp_fmt_x0 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
   word0.cf_isp_comp_fmt_x1 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
   word0.cf_isp_comp_fmt_x2 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
   word0.cf_isp_comp_fmt_y0 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
   word0.cf_isp_comp_fmt_y1 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
   word0.cf_isp_comp_fmt_y2 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
   word0.cf_isp_comp_fmt_z0 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_COMPRESSION_WORD_1, word1) {
      word1.vf_prim_msaa = 0U;
   word1.vf_prim_id_pres = 0U;
   word1.vf_vertex_clipped = 0U;
   word1.vf_vertex_total = num_isp_vertices - 1U;
   word1.cf_isp_comp_fmt_z3 = PVRX(IPF_COMPRESSION_FORMAT_RAW_BYTE);
      }
               }
      static void
   pvr_isp_prim_block_index_block(const struct pvr_device_info *dev_info,
               {
               if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      for (uint32_t i = 0U; i < DIV_ROUND_UP(num_mappings, 2U); i++) {
               pvr_csb_pack ((uint64_t *)cs_ptr,
                  idx_data_word.ix_triangle3_index_2 = idx + 5U;
                  idx_data_word.ix_triangle2_index_2 = idx + 6U;
                  idx_data_word.ix_triangle1_index_2 = idx + 1U;
                  idx_data_word.ix_triangle0_index_2 = idx + 2U;
   idx_data_word.ix_triangle0_index_1 = idx + 1U;
      }
               *cs_ptr_out = cs_ptr;
               for (uint32_t i = 0U, j = 0U; i < num_mappings; i++, j += 4U) {
      if ((i & 1U) == 0U) {
      pvr_csb_pack (cs_ptr, IPF_INDEX_DATA, word) {
      word.ix_index0_0 = j;
   word.ix_index0_1 = j + 1U;
   word.ix_index0_2 = j + 2U;
                     /* Don't increment cs_ptr here. IPF_INDEX_DATA is patched in the
   * else part and then cs_ptr is incremented.
   */
   pvr_csb_pack (cs_ptr, IPF_INDEX_DATA, word) {
      word.ix_index0_0 = j + 2U;
         } else {
               pvr_csb_pack (&tmp, IPF_INDEX_DATA, word) {
      word.ix_index0_2 = j;
      }
                  pvr_csb_pack (cs_ptr, IPF_INDEX_DATA, word) {
      word.ix_index0_0 = j + 2U;
   word.ix_index0_1 = j + 3U;
   word.ix_index0_2 = j + 2U;
      }
                  /* The last pass didn't ++. */
   if ((num_mappings & 1U) != 0U)
               }
      /* Calculates a 24 bit fixed point (biased) representation of a signed integer.
   */
   static inline VkResult
   pvr_int32_to_isp_xy_vtx(const struct pvr_device_info *dev_info,
                     {
      if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      const uint32_t max_fractional = PVRX(IPF_ISP_VERTEX_XY_SIPF_FRAC_MAX_VAL);
            uint32_t fractional;
            if (bias)
            if (val < 0 || val > max_integer + 1) {
      mesa_loge("ISP vertex xy value out of range.");
               if (val <= max_integer) {
      integer = val;
      } else if (val == max_integer + 1) {
      /* The integer field is 13 bits long so the max value is
   * 2 ^ 13 - 1 = 8191. For 8k support we need to handle 8192 so we set
   * all fractional bits to get as close as possible. The best we can do
   * is: 0x1FFF.F = 8191.9375 ≈ 8192 .
   */
   integer = max_integer;
               pvr_csb_pack (word_out, IPF_ISP_VERTEX_XY_SIPF, word) {
      word.integer = integer;
                                    if (((uint32_t)val & 0x7fff8000U) != 0U)
            pvr_csb_pack (word_out, IPF_ISP_VERTEX_XY, word) {
      word.sign = val < 0;
                  }
      static VkResult
   pvr_isp_prim_block_isp_vertices(const struct pvr_device_info *dev_info,
                                 {
      uint32_t *cs_ptr = *cs_ptr_out;
   bool bias = true;
            if (PVR_HAS_FEATURE(dev_info, screen_size8K))
            for (i = mapping_offset; i < mapping_offset + num_mappings; i++) {
      uint32_t bottom = 0U;
   uint32_t right = 0U;
   uint32_t left = 0U;
   uint32_t top = 0U;
            /* ISP vertex data (X, Y, Z). */
   result = pvr_int32_to_isp_xy_vtx(dev_info,
                     if (result != VK_SUCCESS)
            result = pvr_int32_to_isp_xy_vtx(dev_info,
                           if (result != VK_SUCCESS)
            result = pvr_int32_to_isp_xy_vtx(dev_info,
                     if (result != VK_SUCCESS)
            result = pvr_int32_to_isp_xy_vtx(dev_info,
                           if (result != VK_SUCCESS)
            if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      pvr_csb_pack ((uint64_t *)cs_ptr, IPF_ISP_VERTEX_WORD_SIPF, word) {
      word.y = top;
                     pvr_csb_pack ((uint64_t *)cs_ptr, IPF_ISP_VERTEX_WORD_SIPF, word) {
      word.y = top;
                     pvr_csb_pack ((uint64_t *)cs_ptr, IPF_ISP_VERTEX_WORD_SIPF, word) {
      word.y = bottom;
                     pvr_csb_pack ((uint64_t *)cs_ptr, IPF_ISP_VERTEX_WORD_SIPF, word) {
      word.y = bottom;
                                 /* ISP vertices 0 and 1. */
   pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_0, word0) {
      word0.x0 = left;
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_1, word1) {
         }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_2, word2) {
      word2.x1 = right & 0xFFFF;
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_3, word3) {
      word3.x1 = right >> PVRX(IPF_ISP_VERTEX_WORD_3_X1_SHIFT);
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_4, word4) {
         }
            /* ISP vertices 2 and 3. */
   pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_0, word0) {
      word0.x0 = left;
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_1, word1) {
         }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_2, word2) {
      word2.x1 = right & 0xFFFF;
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_3, word3) {
      word3.x1 = right >> PVRX(IPF_ISP_VERTEX_WORD_3_X1_SHIFT);
      }
            pvr_csb_pack (cs_ptr, IPF_ISP_VERTEX_WORD_4, word4) {
         }
      }
               }
      static uint32_t
   pvr_isp_primitive_block_size(const struct pvr_device_info *dev_info,
               {
      uint32_t num_isp_vertices = num_mappings * 4U;
   uint32_t num_tsp_vertices_per_isp_vertex;
   uint32_t isp_vertex_data_size_dw;
   bool color_fill = (src == NULL);
   uint32_t tsp_comp_format_dw;
   uint32_t isp_state_size_dw;
   uint32_t pds_state_size_dw;
   uint32_t idx_data_size_dw;
   uint32_t tsp_data_size;
            if (color_fill) {
         } else {
      num_tsp_vertices_per_isp_vertex =
               tsp_data_size = PVR_DW_TO_BYTES(num_isp_vertices * PVR_TRANSFER_NUM_LAYERS *
            if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      /* An XYZ vertex is 16/16/32 bits => 8 bytes. */
            /* Round to even for 64 bit boundary. */
   idx_data_size_dw = ALIGN_POT(num_mappings, 2U);
   tsp_comp_format_dw = 0U;
   isp_state_size_dw = 4U;
      } else {
               if (!color_fill) {
      if (src->surface.mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
               /* An XYZ vertex is 24/24/32 bits => 10 bytes with last padded to 4 byte
   * burst align.
   */
            /* 4 triangles fit in 3 dw: t0t0t0t1_t1t1t2t2_t2t3t3t3. */
   idx_data_size_dw = num_mappings + DIV_ROUND_UP(num_mappings, 2U);
   isp_state_size_dw = 5U;
               stream_size =
      tsp_data_size + PVR_DW_TO_BYTES(idx_data_size_dw + tsp_comp_format_dw +
                  }
      static VkResult
   pvr_isp_primitive_block(const struct pvr_device_info *dev_info,
                           struct pvr_transfer_ctx *ctx,
   const struct pvr_transfer_cmd *transfer_cmd,
   struct pvr_transfer_prep_data *prep_data,
   const struct pvr_transfer_cmd_source *src,
   bool custom_filter,
   struct pvr_rect_mapping *mappings,
   uint32_t num_mappings,
   {
      struct pvr_transfer_3d_state *state = &prep_data->state;
   uint32_t num_isp_vertices = num_mappings * 4U;
   uint32_t num_tsp_vertices_per_isp_vert;
   uint32_t tsp_data_size_in_bytes;
   uint32_t tsp_comp_format_in_dw;
   bool color_fill = src == NULL;
   uint32_t stream_size_in_bytes;
   uint32_t *cs_ptr_start;
            if (color_fill) {
         } else {
      num_tsp_vertices_per_isp_vert =
               tsp_data_size_in_bytes =
      PVR_DW_TO_BYTES(num_isp_vertices * PVR_TRANSFER_NUM_LAYERS *
         if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
         } else {
               if (!color_fill && src->surface.mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
               stream_size_in_bytes =
                     if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      /* This includes:
   *    Vertex formats.
   *    ISP state words.
   */
   pvr_isp_prim_block_isp_state(dev_info,
                                    /* This include:
   *    Index data / point pitch.
   */
            result = pvr_isp_prim_block_isp_vertices(dev_info,
                                 if (result != VK_SUCCESS)
                     if (!color_fill) {
      /* This includes:
   *    TSP vertex formats.
   */
   pvr_isp_prim_block_tsp_vertex_block(dev_info,
                                                      } else {
      if (!color_fill) {
      /* This includes:
   *    Compressed TSP vertex data & tables.
   *    Primitive id.
   *    TSP compression formats.
   */
   pvr_isp_prim_block_tsp_vertex_block(dev_info,
                                                            /* Point the CS_PRIM_BASE here. */
            /* This includes:
   *    ISP state words.
   *    Compression size word.
   *    ISP compression and vertex formats.
   */
   pvr_isp_prim_block_isp_state(dev_info,
                                             result = pvr_isp_prim_block_isp_vertices(dev_info,
                                 if (result != VK_SUCCESS)
               assert((*cs_ptr_out - cs_ptr_start) * sizeof(cs_ptr_start[0]) ==
               }
      static inline uint32_t
   pvr_transfer_prim_blocks_per_alloc(const struct pvr_device_info *dev_info)
   {
               if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format))
               }
      static inline uint32_t
   pvr_transfer_max_quads_per_pb(const struct pvr_device_info *dev_info)
   {
      return PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format) ? 4U
      }
      static inline uint8_t *pvr_isp_ctrl_stream_sipf_write_aligned(uint8_t *stream,
               {
      const uint32_t offset = (uintptr_t)stream & 0x3U;
   uint32_t *aligned_stream = (uint32_t *)(stream - offset);
                              if (offset + size > 4U) {
      aligned_stream++;
                  }
      /**
   * Writes ISP ctrl stream.
   *
   * We change sampler/texture state when we process a new TQ source. The
   * primitive block contains the shader pointers, but we supply the primitive
   * blocks with shaders from here.
   */
   static VkResult pvr_isp_ctrl_stream(const struct pvr_device_info *dev_info,
                     {
      const uint32_t max_mappings_per_pb = pvr_transfer_max_quads_per_pb(dev_info);
   bool fill_blit = (transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_FILL) != 0U;
   uint32_t free_ctrl_stream_words = PVRX(IPF_CONTROL_STREAM_SIZE_DWORDS);
   struct pvr_transfer_3d_state *const state = &prep_data->state;
   struct pvr_winsys_transfer_regs *const regs = &state->regs;
   struct pvr_transfer_pass *pass = NULL;
   uint32_t flags = transfer_cmd->flags;
   struct pvr_suballoc_bo *pvr_cs_bo;
   pvr_dev_addr_t stream_base_vaddr;
   uint32_t num_prim_blks = 0U;
   uint32_t prim_blk_size = 0U;
   uint32_t region_arrays_size;
   uint32_t num_region_arrays;
   uint32_t total_stream_size;
   bool was_linked = false;
   uint32_t rem_mappings;
   uint32_t num_sources;
   uint32_t *blk_cs_ptr;
   uint32_t *cs_ptr;
   uint32_t source;
            if (state->custom_mapping.pass_count > 0U) {
                        for (source = 0; source < num_sources; source++) {
               while (num_mappings > 0U) {
      if (fill_blit) {
      prim_blk_size += pvr_isp_primitive_block_size(
      dev_info,
                  if (transfer_cmd->source_count > 0) {
      prim_blk_size += pvr_isp_primitive_block_size(
      dev_info,
                  num_mappings -= MIN2(max_mappings_per_pb, num_mappings);
            } else {
               if (fill_blit) {
      num_prim_blks = 1U;
   prim_blk_size +=
      pvr_isp_primitive_block_size(dev_info,
                           for (source = 0; source < transfer_cmd->source_count; source++) {
               while (num_mappings > 0U) {
      prim_blk_size += pvr_isp_primitive_block_size(
                        num_mappings -= MIN2(max_mappings_per_pb, num_mappings);
                     num_region_arrays =
      (num_prim_blks + (pvr_transfer_prim_blocks_per_alloc(dev_info) - 1U)) /
      region_arrays_size = PVRX(IPF_CONTROL_STREAM_SIZE_DWORDS) *
                  /* Allocate space for IPF control stream. */
   result = pvr_cmd_buffer_alloc_mem(transfer_cmd->cmd_buffer,
                     if (result != VK_SUCCESS)
            stream_base_vaddr =
      PVR_DEV_ADDR(pvr_cs_bo->dev_addr.addr -
         cs_ptr = pvr_bo_suballoc_get_map_addr(pvr_cs_bo);
            source = 0;
   while (source < num_sources) {
      if (fill_blit)
         else
            if ((transfer_cmd->source_count > 0 || fill_blit) && rem_mappings != 0U) {
      struct pvr_pds_pixel_shader_sa_program unitex_pds_prog = { 0U };
   struct pvr_transfer_cmd_source *src = &transfer_cmd->sources[source];
   struct pvr_rect_mapping fill_mapping;
                                    if (vk_format_is_compressed(transfer_cmd->dst.vk_format)) {
                        state->pds_shader_task_offset = 0U;
   state->uni_tex_code_offset = 0U;
                  result = pvr_pack_clear_color(transfer_cmd->dst.vk_format,
                                       pvr_csb_pack (&regs->usc_clear_register0,
                              pvr_csb_pack (&regs->usc_clear_register1,
                              pvr_csb_pack (&regs->usc_clear_register2,
                              pvr_csb_pack (&regs->usc_clear_register3,
                                             unitex_pds_prog.kick_usc = false;
      } else {
      const bool down_scale = transfer_cmd->sources[source].resolve_op ==
                     struct pvr_tq_shader_properties *shader_props =
         struct pvr_tq_layer_properties *layer = &shader_props->layer_props;
   const struct pvr_tq_frag_sh_reg_layout *sh_reg_layout;
   enum pvr_transfer_pbe_pixel_src pbe_src_format;
   struct pvr_suballoc_bo *pvr_bo;
                  /* Reset the shared register bank ptrs each src implies new texture
   * state (Note that we don't change texture state per prim block).
   */
   state->common_ptr = 0U;
                                 result = pvr_pbe_src_format_f2d(flags,
                                                         layer->pbe_format = pbe_src_format;
   layer->sample =
                                 result = pvr_msaa_state(dev_info, transfer_cmd, state, source);
                  if (state->filter[source] == PVR_FILTER_LINEAR &&
      pvr_requires_usc_linear_filter(src->surface.vk_format)) {
   if (pvr_int_pbe_usc_linear_filter(layer->pbe_format,
                           } else {
                     result = pvr_transfer_frag_store_get_shader_info(
      transfer_cmd->cmd_buffer->device,
   &ctx->frag_store,
   shader_props,
   &dev_offset,
                                    result =
                                                                     /* Allocate memory for DMA. */
   result = pvr_cmd_buffer_alloc_mem(transfer_cmd->cmd_buffer,
                                    result = pvr_sampler_state_for_surface(
      dev_info,
   &transfer_cmd->sources[source].surface,
   state->filter[source],
   sh_reg_layout,
   0U,
                     result = pvr_image_state_for_surface(
      ctx,
   transfer_cmd,
   &transfer_cmd->sources[source].surface,
   0U,
   source,
   sh_reg_layout,
   state,
   0U,
                     pvr_pds_encode_dma_burst(unitex_pds_prog.texture_dma_control,
                                                                  if (pvr_pick_component_needed(&state->custom_mapping)) {
      pvr_dma_texel_unwind(state,
                        result = pvr_pds_unitex(dev_info,
                                          while (rem_mappings > 0U) {
      const uint32_t min_free_ctrl_stream_words =
      PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format) ? 2
      const uint32_t num_mappings =
         struct pvr_rect_mapping *mappings = NULL;
                                    num_region_arrays++;
                                                pvr_csb_pack (&link_addr,
                                          pvr_isp_ctrl_stream_sipf_write_aligned(
      (uint8_t *)cs_ptr,
   link_addr,
   PVR_DW_TO_BYTES(
   } else {
      pvr_csb_pack (cs_ptr, IPF_CONTROL_STREAM, control_stream) {
                           cs_ptr =
                                    if (fill_blit)
                        prim_blk_addr = stream_base_vaddr;
   prim_blk_addr.addr +=
                  result = pvr_isp_primitive_block(dev_info,
                                    ctx,
   transfer_cmd,
   prep_data,
   fill_blit ? NULL : src,
   state->custom_filter,
                              if (PVR_HAS_FEATURE(dev_info,
                                    pvr_csb_pack (&tmp, IPF_PRIMITIVE_HEADER_SIPF2, prim_header) {
      prim_header.cs_prim_base_size = 1;
   prim_header.cs_mask_num_bytes = 1;
                           pvr_csb_pack (&tmp, IPF_PRIMITIVE_BASE_SIPF2, word) {
                              /* IPF_BYTE_BASED_MASK_ONE_BYTE_WORD_0_SIPF2 since
   * IPF_PRIMITIVE_HEADER_SIPF2.cs_mask_num_bytes == 1.
   */
   pvr_csb_pack (&tmp,
                  switch (num_mappings) {
   case 4:
      mask.cs_mask_one_byte_tile0_7 = true;
   mask.cs_mask_one_byte_tile0_6 = true;
      case 3:
      mask.cs_mask_one_byte_tile0_5 = true;
   mask.cs_mask_one_byte_tile0_4 = true;
      case 2:
      mask.cs_mask_one_byte_tile0_3 = true;
   mask.cs_mask_one_byte_tile0_2 = true;
      case 1:
      mask.cs_mask_one_byte_tile0_1 = true;
   mask.cs_mask_one_byte_tile0_0 = true;
      default:
      /* Unreachable since we clamped the value earlier so
   * reaching this is an implementation error.
   */
   unreachable("num_mapping exceeded max_mappings_per_pb");
         }
   /* Only 1 byte since there's only 1 valid tile within the single
   * IPF_BYTE_BASED_MASK_ONE_BYTE_WORD_0_SIPF2 mask.
   * ROGUE_IPF_PRIMITIVE_HEADER_SIPF2.cs_valid_tile0 == true.
                                    } else {
      pvr_csb_pack (cs_ptr, IPF_PRIMITIVE_FORMAT, word) {
      word.cs_type = PVRX(IPF_CS_TYPE_PRIM);
   word.cs_isp_state_read = true;
   word.cs_isp_state_size = 2U;
   word.cs_prim_total = 2U * num_mappings - 1U;
   word.cs_mask_fmt = PVRX(IPF_CS_MASK_FMT_FULL);
                     pvr_csb_pack (cs_ptr, IPF_PRIMITIVE_BASE, word) {
                                    rem_mappings -= num_mappings;
                           /* A fill blit may also have sources for normal blits. */
   if (fill_blit && transfer_cmd->source_count > 0) {
      /* Fill blit count for custom mapping equals source blit count. While
   * normal blits use only one fill blit.
   */
   if (state->custom_mapping.pass_count == 0 && source > num_sources) {
      fill_blit = false;
                     if (PVR_HAS_FEATURE(dev_info, ipf_creq_pf))
            if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format_v2)) {
      uint8_t *cs_byte_ptr = (uint8_t *)cs_ptr;
            /* clang-format off */
   pvr_csb_pack (&tmp, IPF_CONTROL_STREAM_TERMINATE_SIPF2, term);
                        } else {
      pvr_csb_pack (cs_ptr, IPF_CONTROL_STREAM, word) {
         }
               pvr_csb_pack (&regs->isp_mtile_base, CR_ISP_MTILE_BASE, reg) {
      reg.addr =
      PVR_DEV_ADDR(pvr_cs_bo->dev_addr.addr -
            pvr_csb_pack (&regs->isp_render, CR_ISP_RENDER, reg) {
                  if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format_v2) &&
      PVR_HAS_FEATURE(dev_info, ipf_creq_pf)) {
   pvr_csb_pack (&regs->isp_rgn, CR_ISP_RGN_SIPF, isp_rgn) {
      /* Bit 0 in CR_ISP_RGN.cs_size_ipf_creq_pf is used to indicate the
   * presence of a link.
   */
         } else {
      /* clang-format off */
   pvr_csb_pack(&regs->isp_rgn, CR_ISP_RGN, isp_rgn);
                  }
      static void pvr_transfer_set_filter(struct pvr_transfer_cmd *transfer_cmd,
         {
      for (uint32_t i = 0; i < transfer_cmd->source_count; i++) {
      VkRect2D *src = &transfer_cmd->sources[i].mappings[0U].src_rect;
            /* If no scaling is applied to the copy region, we can use point
   * filtering.
   */
   if (!state->custom_filter && (src->extent.width == dst->extent.width) &&
      (src->extent.height == dst->extent.height))
      else
         }
      /** Generates hw resources to kick a 3D clip blit. */
   static VkResult pvr_3d_clip_blit(struct pvr_transfer_ctx *ctx,
                           {
      struct pvr_transfer_3d_state *state = &prep_data->state;
   uint32_t texel_unwind_src = state->custom_mapping.texel_unwind_src;
   struct pvr_transfer_cmd bg_cmd = { 0U };
   uint32_t control_reg;
            state->dont_force_pbe = false;
   bg_cmd.scissor = transfer_cmd->scissor;
   bg_cmd.cmd_buffer = transfer_cmd->cmd_buffer;
   bg_cmd.flags = transfer_cmd->flags;
   bg_cmd.flags &=
      ~(PVR_TRANSFER_CMD_FLAGS_FAST2D | PVR_TRANSFER_CMD_FLAGS_FILL |
         bg_cmd.source_count = state->custom_mapping.pass_count > 0U ? 0 : 1;
   if (bg_cmd.source_count > 0) {
               src->mappings[0U].src_rect = transfer_cmd->scissor;
   src->mappings[0U].dst_rect = transfer_cmd->scissor;
   src->resolve_op = PVR_RESOLVE_BLEND;
               state->filter[0] = PVR_FILTER_DONTCARE;
   bg_cmd.dst = transfer_cmd->dst;
   state->custom_mapping.texel_unwind_src =
            result =
         if (result != VK_SUCCESS)
            /* If the destination has 4 channels and the source has at most 2, we still
   * need all 4 channels from the USC into the PBE.
   */
   state->dont_force_pbe = true;
            /* We need the viewport mask, otherwise all pixels would be disabled. */
   pvr_csb_pack (&control_reg, CR_ISP_BGOBJVALS, reg) {
         }
            pvr_transfer_set_filter(transfer_cmd, state);
   result = pvr_isp_ctrl_stream(&ctx->device->pdevice->dev_info,
                     if (result != VK_SUCCESS)
            /* In case of resolve M -> S, the accumulation is read from and written to a
   * single sampled surface. Make sure that we are resolving and we have the
   * right number of tiles.
   */
   if (state->down_scale) {
               pvr_csb_pack (&tmp, CR_PBE_WORD0_MRT0, reg) {
         }
            result = pvr_isp_tiles(ctx->device, state);
   if (result != VK_SUCCESS)
                  }
      static bool pvr_texel_unwind(uint32_t bpp,
                           {
               for (uint32_t i = 0U; i < 16U; i++) {
      if (pvr_is_surface_aligned(dev_addr, is_input, bpp)) {
         } else {
      if (i == 15U) {
         } else {
      dev_addr.addr -= (bpp / texel_extend) / 8U;
                                 }
      static bool pvr_is_identity_mapping(const struct pvr_rect_mapping *mapping)
   {
      return (mapping->src_rect.offset.x == mapping->dst_rect.offset.x &&
         mapping->src_rect.offset.y == mapping->dst_rect.offset.y &&
      }
      static inline bool pvr_is_pbe_stride_aligned(const uint32_t stride)
   {
      if (stride == 1U)
            return ((stride & (PVRX(PBESTATE_REG_WORD0_LINESTRIDE_UNIT_SIZE) - 1U)) ==
      }
      static struct pvr_transfer_pass *
   pvr_create_pass(struct pvr_transfer_custom_mapping *custom_mapping,
         {
                        pass = &custom_mapping->passes[custom_mapping->pass_count];
   pass->clip_rects_count = 0U;
   pass->dst_offset = dst_offset;
                        }
      /* Acquire pass with given offset. If one doesn't exist, create new. */
   static struct pvr_transfer_pass *
   pvr_acquire_pass(struct pvr_transfer_custom_mapping *custom_mapping,
         {
      for (uint32_t i = 0U; i < custom_mapping->pass_count; i++) {
      if (custom_mapping->passes[i].dst_offset == dst_offset)
                  }
      static struct pvr_transfer_wa_source *
   pvr_create_source(struct pvr_transfer_pass *pass,
               {
                        src = &pass->sources[pass->source_count];
   src->mapping_count = 0U;
                        }
      /* Acquire source with given offset. If one doesn't exist, create new. */
   static struct pvr_transfer_wa_source *
   pvr_acquire_source(struct pvr_transfer_pass *pass,
               {
      for (uint32_t i = 0U; i < pass->source_count; i++) {
      if (pass->sources[i].src_offset == src_offset &&
      pass->sources[i].extend_height == extend_height)
               }
      static void pvr_remove_source(struct pvr_transfer_pass *pass, uint32_t idx)
   {
               for (uint32_t i = idx; i < (pass->source_count - 1U); i++)
               }
      static void pvr_remove_mapping(struct pvr_transfer_wa_source *src, uint32_t idx)
   {
               for (uint32_t i = idx; i < (src->mapping_count - 1U); i++)
               }
      static struct pvr_rect_mapping *
   pvr_create_mapping(struct pvr_transfer_wa_source *src)
   {
                  }
      /**
   * If PBE can't write to surfaces with odd stride, the stride of
   * destination surface is doubled to make it even. Height of the surface is
   * halved. The source surface is not resized. Each half of the modified
   * destination surface samples every second row from the source surface. This
   * only works with nearest filtering.
   */
   static bool pvr_double_stride(struct pvr_transfer_pass *pass, uint32_t stride)
   {
      struct pvr_rect_mapping *mappings = pass->sources[0].mappings;
            if (stride == 1U)
            if (mappings[0U].dst_rect.extent.height == 1U &&
      pass->sources[0].mapping_count == 1U) {
   /* Only one mapping required if height is 1. */
   if ((mappings[0U].dst_rect.offset.y & 1U) != 0U) {
      mappings[0U].dst_rect.offset.x += (int32_t)stride;
   mappings[0U].dst_rect.offset.y /= 2U;
   mappings[0U].dst_rect.extent.height =
      } else {
      mappings[0U].dst_rect.extent.height =
      (mappings[0U].dst_rect.offset.y +
   mappings[0U].dst_rect.extent.height + 1U) /
                                    for (uint32_t i = 0; i < pass->sources[0].mapping_count; i++) {
      struct pvr_rect_mapping *mapping_a = &mappings[i];
   struct pvr_rect_mapping *mapping_b =
         int32_t mapping_a_src_rect_y1 =
         int32_t mapping_b_src_rect_y1 = mapping_a_src_rect_y1;
   const bool dst_starts_odd_row = !!(mapping_a->dst_rect.offset.y & 1);
   const bool dst_ends_odd_row =
      !!((mapping_a->dst_rect.offset.y + mapping_a->dst_rect.extent.height) &
      const bool src_starts_odd_row = !!(mapping_a->src_rect.offset.y & 1);
   const bool src_ends_odd_row =
                  assert(pass->sources[0].mapping_count + new_mapping <
                  mapping_a->src_rect.offset.y = ALIGN_POT(mapping_a->src_rect.offset.y, 2);
   if (dst_starts_odd_row && !src_starts_odd_row)
         else if (!dst_starts_odd_row && src_starts_odd_row)
            mapping_a_src_rect_y1 = ALIGN_POT(mapping_a_src_rect_y1, 2);
   if (dst_ends_odd_row && !src_ends_odd_row)
         else if (!dst_ends_odd_row && src_ends_odd_row)
            mapping_a->src_rect.extent.height =
            mapping_b->src_rect.offset.y = ALIGN_POT(mapping_b->src_rect.offset.y, 2);
   if (dst_starts_odd_row && src_starts_odd_row)
         else if (!dst_starts_odd_row && !src_starts_odd_row)
            mapping_b_src_rect_y1 = ALIGN_POT(mapping_b_src_rect_y1, 2);
   if (dst_ends_odd_row && src_ends_odd_row)
         else if (!dst_ends_odd_row && !src_ends_odd_row)
            mapping_b->src_rect.extent.height =
            /* Destination rectangles. */
            if (dst_starts_odd_row)
            mapping_b->dst_rect.offset.x += stride;
   mapping_b->dst_rect.offset.y /= 2;
   mapping_b->dst_rect.extent.height /= 2;
            if (!mapping_a->src_rect.extent.width ||
      !mapping_a->src_rect.extent.height) {
      } else if (mapping_b->src_rect.extent.width &&
                                       }
      static void pvr_split_rect(uint32_t stride,
                           {
      rect_a->offset.x = 0;
   rect_a->extent.width = stride - texel_unwind;
   rect_a->offset.y = 0;
            rect_b->offset.x = (int32_t)stride - texel_unwind;
   rect_b->extent.width = texel_unwind;
   rect_b->offset.y = 0;
      }
      static bool pvr_rect_width_covered_by(const VkRect2D *rect_a,
         {
      return (rect_b->offset.x <= rect_a->offset.x &&
            }
      static void pvr_unwind_rects(uint32_t width,
                           {
      struct pvr_transfer_wa_source *const source = &pass->sources[0];
   struct pvr_rect_mapping *const mappings = source->mappings;
   const uint32_t num_mappings = source->mapping_count;
            if (texel_unwind == 0)
                     for (uint32_t i = 0; i < num_mappings; i++) {
      VkRect2D *const old_rect = input ? &mappings[i].src_rect
            if (height == 1) {
         } else if (width == 1) {
         } else if (pvr_rect_width_covered_by(old_rect, &rect_a)) {
         } else if (pvr_rect_width_covered_by(old_rect, &rect_b)) {
      old_rect->offset.x = texel_unwind - width + old_rect->offset.x;
      } else {
                                    VkRect2D *const new_rect_opp = input ? &mappings[new_mapping].dst_rect
                        const uint32_t split_point = width - texel_unwind;
                                 old_rect_opp->extent.width -= split_width;
   new_rect_opp->extent.width = split_width;
                                 new_rect->offset.x = 0;
   new_rect->offset.y++;
            }
      /**
   * Assign clip rects to rectangle mappings. TDM can only do two PBE clip
   * rects per screen.
   */
   static void
   pvr_map_clip_rects(struct pvr_transfer_custom_mapping *custom_mapping)
   {
      for (uint32_t i = 0U; i < custom_mapping->pass_count; i++) {
                        for (uint32_t s = 0U; s < pass->source_count; s++) {
               for (uint32_t j = 0U; j < src->mapping_count; j++) {
      struct pvr_rect_mapping *mappings = src->mappings;
                  /* Try merge adjacent clip rects. */
   for (uint32_t k = 0U; k < pass->clip_rects_count; k++) {
      if (clip_rects[k].offset.y == mappings[j].dst_rect.offset.y &&
      clip_rects[k].extent.height ==
         clip_rects[k].offset.x + clip_rects[k].extent.width ==
         clip_rects[k].extent.width +=
                           if (clip_rects[k].offset.y == mappings[j].dst_rect.offset.y &&
      clip_rects[k].extent.height ==
         clip_rects[k].offset.x ==
      mappings[j].dst_rect.offset.x +
      clip_rects[k].offset.x = mappings[j].dst_rect.offset.x;
   clip_rects[k].extent.width +=
                           if (clip_rects[k].offset.x == mappings[j].dst_rect.offset.x &&
      clip_rects[k].extent.width ==
         clip_rects[k].offset.y + clip_rects[k].extent.height ==
         clip_rects[k].extent.height +=
                           if (clip_rects[k].offset.x == mappings[j].dst_rect.offset.x &&
      clip_rects[k].extent.width ==
         clip_rects[k].offset.y ==
      mappings[j].dst_rect.offset.y +
      clip_rects[k].extent.height +=
         clip_rects[k].offset.y = mappings[j].dst_rect.offset.y;
   merged = true;
                                 /* Create new pass if needed, TDM can only have 2 clip rects. */
   if (pass->clip_rects_count >= custom_mapping->max_clip_rects) {
      struct pvr_transfer_pass *new_pass =
         struct pvr_transfer_wa_source *new_source =
      pvr_create_source(new_pass,
                                                   if (src->mapping_count == 0) {
      pvr_remove_source(pass, s);
      } else {
      /* Redo - mapping was replaced. */
         } else {
                                             }
      static bool pvr_extend_height(const VkRect2D *rect,
               {
      if (rect->offset.x >= (int32_t)unwind_src)
            return (rect->offset.y > (int32_t)height) ||
      }
      static void
   pvr_generate_custom_mapping(uint32_t src_stride,
                              uint32_t src_width,
   uint32_t src_height,
      {
      src_stride *= custom_mapping->texel_extend_src;
   src_width *= custom_mapping->texel_extend_src;
   dst_stride *= custom_mapping->texel_extend_dst;
            if (custom_mapping->texel_unwind_src > 0U) {
      pvr_unwind_rects(src_stride,
                  src_height,
            if (custom_mapping->double_stride) {
      custom_mapping->double_stride =
                        pvr_unwind_rects(dst_stride,
                  dst_height,
                  /* If the last row of the source mapping is sampled, height of the surface
   * can only be increased if the new area contains a valid region. Some blits
   * are split to two sources.
   */
   if (custom_mapping->texel_unwind_src > 0U) {
      for (uint32_t i = 0; i < custom_mapping->pass_count; i++) {
                                 for (uint32_t k = 0; k < src->mapping_count; k++) {
      VkRect2D *src_rect = &src->mappings[k].src_rect;
   bool extend_height =
                        if (src->mapping_count == 1) {
                                                               new_src->mapping_count++;
   src->mapping_count--;
                     }
      static bool
   pvr_get_custom_mapping(const struct pvr_device_info *dev_info,
                     {
      const uint32_t dst_bpp =
         const struct pvr_transfer_cmd_source *src = NULL;
   struct pvr_transfer_pass *pass;
            custom_mapping->max_clip_rects = max_clip_rects;
   custom_mapping->texel_unwind_src = 0U;
   custom_mapping->texel_unwind_dst = 0U;
   custom_mapping->texel_extend_src = 1U;
   custom_mapping->texel_extend_dst = 1U;
            if (transfer_cmd->source_count > 1)
                     ret = pvr_texel_unwind(dst_bpp,
                           if (!ret) {
      custom_mapping->texel_extend_dst = dst_bpp / 8U;
   if (transfer_cmd->source_count > 0) {
      if (transfer_cmd->sources[0].surface.mem_layout ==
      PVR_MEMLAYOUT_LINEAR) {
      } else if (transfer_cmd->sources[0].surface.mem_layout ==
                                 ret = pvr_texel_unwind(dst_bpp,
                           if (!ret)
               if (transfer_cmd->source_count > 0) {
      src = &transfer_cmd->sources[0];
   const uint32_t src_bpp =
                     if (!ret && (src->surface.mem_layout == PVR_MEMLAYOUT_LINEAR ||
            ret = pvr_texel_unwind(src_bpp,
                                 if (!ret) {
                     ret = pvr_texel_unwind(src_bpp,
                                 if (!ret)
               VkRect2D rect = transfer_cmd->scissor;
   assert(
      (rect.offset.x + rect.extent.width) <= custom_mapping->max_clip_size &&
         /* Texel extend only works with strided memory layout, because pixel width is
   * changed. Texel unwind only works with strided memory layout. 1D blits are
   * allowed.
   */
   if (src && src->surface.height > 1U &&
      (custom_mapping->texel_extend_src > 1U ||
   custom_mapping->texel_unwind_src > 0U) &&
   src->surface.mem_layout != PVR_MEMLAYOUT_LINEAR) {
               /* Texel extend only works with strided memory layout, because pixel width is
   * changed. Texel unwind only works with strided memory layout. 1D blits are
   * allowed.
   */
   if ((custom_mapping->texel_extend_dst > 1U ||
      custom_mapping->texel_unwind_dst > 0U) &&
   transfer_cmd->dst.mem_layout != PVR_MEMLAYOUT_LINEAR &&
   transfer_cmd->dst.height > 1U) {
               if (transfer_cmd->dst.mem_layout == PVR_MEMLAYOUT_LINEAR) {
      custom_mapping->double_stride = !pvr_is_pbe_stride_aligned(
               if (custom_mapping->texel_unwind_src > 0U ||
      custom_mapping->texel_unwind_dst > 0U || custom_mapping->double_stride) {
   struct pvr_transfer_wa_source *wa_src;
            pass = pvr_acquire_pass(custom_mapping, 0U);
   wa_src = pvr_create_source(pass, 0U, false);
            if (transfer_cmd->source_count > 0) {
         } else {
      mapping->src_rect = transfer_cmd->scissor;
         } else {
                  if (custom_mapping->texel_extend_src > 1U ||
      custom_mapping->texel_extend_dst > 1U) {
   pass->sources[0].mappings[0U].src_rect.offset.x *=
         pass->sources[0].mappings[0U].src_rect.extent.width *=
         pass->sources[0].mappings[0U].dst_rect.offset.x *=
         pass->sources[0].mappings[0U].dst_rect.extent.width *=
               if (transfer_cmd->source_count > 0) {
      pvr_generate_custom_mapping(transfer_cmd->sources[0].surface.stride,
                              transfer_cmd->sources[0].surface.width,
   transfer_cmd->sources[0].surface.height,
   } else {
      pvr_generate_custom_mapping(0U,
                              0U,
   0U,
               }
      static void pvr_pbe_extend_rect(uint32_t texel_extend, VkRect2D *rect)
   {
      rect->offset.x *= texel_extend;
      }
      static void pvr_pbe_rect_intersect(VkRect2D *rect_a, VkRect2D *rect_b)
   {
      rect_a->extent.width = MIN2(rect_a->offset.x + rect_a->extent.width,
               rect_a->offset.x = MAX2(rect_a->offset.x, rect_b->offset.x);
   rect_a->extent.height = MIN2(rect_a->offset.y + rect_a->extent.height,
                  }
      static VkFormat pvr_texel_extend_src_format(VkFormat vk_format)
   {
      uint32_t bpp = vk_format_get_blocksizebits(vk_format);
            switch (bpp) {
   case 16:
      ext_format = VK_FORMAT_R8G8_UINT;
      case 32:
      ext_format = VK_FORMAT_R8G8B8A8_UINT;
      case 48:
      ext_format = VK_FORMAT_R16G16B16_UINT;
      default:
      ext_format = VK_FORMAT_R8_UINT;
                  }
      static void
   pvr_modify_command(struct pvr_transfer_custom_mapping *custom_mapping,
               {
      struct pvr_transfer_pass *pass = &custom_mapping->passes[pass_idx];
            if (custom_mapping->texel_extend_src > 1U) {
               pvr_pbe_extend_rect(custom_mapping->texel_extend_src, &mapping->dst_rect);
            transfer_cmd->dst.vk_format = VK_FORMAT_R8_UINT;
   transfer_cmd->dst.width *= custom_mapping->texel_extend_src;
   transfer_cmd->dst.stride *= custom_mapping->texel_extend_src;
   transfer_cmd->sources[0].surface.vk_format = VK_FORMAT_R8_UINT;
   transfer_cmd->sources[0].surface.width *=
         transfer_cmd->sources[0].surface.stride *=
      } else if (custom_mapping->texel_extend_dst > 1U) {
      VkRect2D max_clip = {
      .offset = { 0, 0 },
   .extent = { custom_mapping->max_clip_size,
               pvr_pbe_extend_rect(custom_mapping->texel_extend_dst,
                     if (transfer_cmd->source_count > 0) {
      transfer_cmd->sources[0].surface.width *=
                        transfer_cmd->sources[0].surface.vk_format =
      pvr_texel_extend_src_format(
            transfer_cmd->dst.vk_format = VK_FORMAT_R8_UINT;
   transfer_cmd->dst.width *= custom_mapping->texel_extend_dst;
               if (custom_mapping->double_stride) {
      transfer_cmd->dst.width *= 2U;
               if (custom_mapping->texel_unwind_src > 0U) {
      if (transfer_cmd->sources[0].surface.height == 1U) {
      transfer_cmd->sources[0].surface.width +=
         transfer_cmd->sources[0].surface.stride +=
      } else if (transfer_cmd->sources[0].surface.stride == 1U) {
      transfer_cmd->sources[0].surface.height +=
      } else {
      /* Increase source width by texel unwind. If texel unwind is less than
   * the distance between width and stride. The blit can be done with one
   * rectangle mapping, but the width of the surface needs be to
   * increased in case we sample from the area between width and stride.
   */
   transfer_cmd->sources[0].surface.width =
      MIN2(transfer_cmd->sources[0].surface.width +
                     for (uint32_t i = 0U; i < pass->source_count; i++) {
               if (i > 0)
            transfer_cmd->sources[i].mapping_count = src->mapping_count;
   for (uint32_t j = 0U; j < transfer_cmd->sources[i].mapping_count; j++)
            if (src->extend_height)
            transfer_cmd->sources[i].surface.width =
         transfer_cmd->sources[i].surface.height =
         transfer_cmd->sources[i].surface.stride =
               if (transfer_cmd->dst.height == 1U) {
      transfer_cmd->dst.width =
                     if (transfer_cmd->dst.mem_layout == PVR_MEMLAYOUT_TWIDDLED) {
      transfer_cmd->dst.width =
         transfer_cmd->dst.height = MIN2((uint32_t)custom_mapping->max_clip_size,
      } else {
                  if (transfer_cmd->source_count > 0) {
      for (uint32_t i = 0; i < pass->source_count; i++) {
                        src->surface.dev_addr.addr -=
         src->surface.dev_addr.addr += MAX2(src->surface.sample_count, 1U) *
                  bpp = vk_format_get_blocksizebits(transfer_cmd->dst.vk_format);
   transfer_cmd->dst.dev_addr.addr -=
         transfer_cmd->dst.dev_addr.addr +=
            if (transfer_cmd->source_count > 0)
      }
      /* Route a copy_blit (FastScale HW) to a clip_blit (Fast2D HW).
   * Destination rectangle can be specified in dst_rect, or NULL to use existing.
   */
   static VkResult pvr_reroute_to_clip(struct pvr_transfer_ctx *ctx,
                                 {
               clip_transfer_cmd = *transfer_cmd;
            if (transfer_cmd->source_count <= 1U) {
      if (dst_rect)
            return pvr_3d_clip_blit(ctx,
                                    }
      static VkResult pvr_3d_copy_blit(struct pvr_transfer_ctx *ctx,
                           {
      const struct pvr_device_info *const dev_info =
            struct pvr_transfer_3d_state *state = &prep_data->state;
   struct pvr_transfer_cmd *active_cmd = transfer_cmd;
   struct pvr_transfer_cmd int_cmd;
            state->dont_force_pbe = false;
                     if (transfer_cmd->source_count == 1U) {
               /* Try to work out a condition to map pixel formats to RAW. That is only
   * possible if we don't perform any kind of 2D operation on the blit as we
   * don't know the actual pixel values - i.e. it has to be point sampled -
   * scaling doesn't matter as long as point sampled.
   */
   if (src->surface.vk_format == transfer_cmd->dst.vk_format &&
      state->filter[0] == PVR_FILTER_POINT &&
   src->surface.sample_count <= transfer_cmd->dst.sample_count &&
                  int_cmd = *transfer_cmd;
                  if (bpp > 0U) {
      switch (bpp) {
   case 8U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R8_UINT;
      case 16U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R8G8_UINT;
      case 24U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R8G8B8_UINT;
      case 32U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R32_UINT;
      case 48U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R16G16B16_UINT;
      case 64U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R32G32_UINT;
      case 96U:
      int_cmd.sources[0].surface.vk_format = VK_FORMAT_R32G32B32_UINT;
      case 128U:
      int_cmd.sources[0].surface.vk_format =
            default:
      active_cmd = transfer_cmd;
                                 if (pass_idx == 0U) {
               if (state->custom_mapping.texel_extend_src > 1U)
               if (state->custom_mapping.pass_count > 0U) {
               if (active_cmd != &int_cmd) {
      int_cmd = *active_cmd;
                                 if (state->custom_mapping.double_stride ||
      pass->sources[0].mapping_count > 1U || pass->source_count > 1U) {
   result =
      } else {
               mappings[0U].src_rect.offset.x /=
                        if (int_cmd.source_count > 0) {
      for (uint32_t i = 0U; i < pass->sources[0].mapping_count; i++)
                        result = pvr_3d_copy_blit_core(ctx,
                                             /* Route DS merge blits to Clip blit. Background object is used to preserve
   * the unmerged channel.
   */
   if ((transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_DSMERGE) != 0U) {
      /* PBE byte mask could be used for DS merge with FastScale. Clearing the
   * other channel on a DS merge requires Clip blit.
   */
   if (!PVR_HAS_ERN(dev_info, 42064) ||
      ((transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_FILL) != 0U)) {
   return pvr_reroute_to_clip(ctx,
                                          return pvr_3d_copy_blit_core(ctx,
                        }
      /* TODO: This should be generated in csbgen. */
   #define TEXSTATE_STRIDE_IMAGE_WORD1_TEXADDR_MASK \
            static bool pvr_validate_source_addr(pvr_dev_addr_t addr)
   {
      if (!pvr_dev_addr_is_aligned(
         addr,
                  if (addr.addr & ~TEXSTATE_STRIDE_IMAGE_WORD1_TEXADDR_MASK)
               }
      static bool pvr_supports_texel_unwind(struct pvr_transfer_cmd *transfer_cmd)
   {
               if (transfer_cmd->source_count > 1)
            if (transfer_cmd->source_count) {
               if (src->height == 1) {
      if (src->mem_layout != PVR_MEMLAYOUT_LINEAR &&
      src->mem_layout != PVR_MEMLAYOUT_TWIDDLED &&
   src->mem_layout != PVR_MEMLAYOUT_3DTWIDDLED) {
         } else if (src->mem_layout == PVR_MEMLAYOUT_TWIDDLED ||
            if (!pvr_validate_source_addr(src->dev_addr))
      } else {
      if (src->mem_layout != PVR_MEMLAYOUT_LINEAR)
                  if (dst->mem_layout != PVR_MEMLAYOUT_LINEAR &&
      dst->mem_layout != PVR_MEMLAYOUT_TWIDDLED) {
                  }
      static bool pvr_3d_validate_addr(struct pvr_transfer_cmd *transfer_cmd)
   {
      if (!pvr_supports_texel_unwind(transfer_cmd)) {
      return pvr_dev_addr_is_aligned(
      transfer_cmd->dst.dev_addr,
               }
      static void
   pvr_submit_info_stream_init(struct pvr_transfer_ctx *ctx,
               {
      const struct pvr_winsys_transfer_regs *const regs = &prep_data->state.regs;
   const struct pvr_physical_device *const pdevice = ctx->device->pdevice;
            uint32_t *stream_ptr = (uint32_t *)cmd->fw_stream;
            /* Leave space for stream header. */
            *(uint64_t *)stream_ptr = regs->pds_bgnd0_base;
            *(uint64_t *)stream_ptr = regs->pds_bgnd1_base;
            *(uint64_t *)stream_ptr = regs->pds_bgnd3_sizeinfo;
            *(uint64_t *)stream_ptr = regs->isp_mtile_base;
            STATIC_ASSERT(ARRAY_SIZE(regs->pbe_wordx_mrty) == 9U);
   STATIC_ASSERT(sizeof(regs->pbe_wordx_mrty[0]) == sizeof(uint64_t));
   memcpy(stream_ptr, regs->pbe_wordx_mrty, sizeof(regs->pbe_wordx_mrty));
            *stream_ptr = regs->isp_bgobjvals;
            *stream_ptr = regs->usc_pixel_output_ctrl;
            *stream_ptr = regs->usc_clear_register0;
            *stream_ptr = regs->usc_clear_register1;
            *stream_ptr = regs->usc_clear_register2;
            *stream_ptr = regs->usc_clear_register3;
            *stream_ptr = regs->isp_mtile_size;
            *stream_ptr = regs->isp_render_origin;
            *stream_ptr = regs->isp_ctl;
            *stream_ptr = regs->isp_aa;
            *stream_ptr = regs->event_pixel_pds_info;
            *stream_ptr = regs->event_pixel_pds_code;
            *stream_ptr = regs->event_pixel_pds_data;
            *stream_ptr = regs->isp_render;
            *stream_ptr = regs->isp_rgn;
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      *stream_ptr = regs->frag_screen;
               cmd->fw_stream_len = (uint8_t *)stream_ptr - (uint8_t *)cmd->fw_stream;
            pvr_csb_pack ((uint64_t *)stream_len_ptr, KMD_STREAM_HDR, value) {
            }
      static void
   pvr_submit_info_flags_init(const struct pvr_device_info *const dev_info,
               {
      *flags = prep_data->flags;
      }
      static void pvr_transfer_job_ws_submit_info_init(
      struct pvr_transfer_ctx *ctx,
   struct pvr_transfer_submit *submit,
   struct vk_sync *wait,
      {
      const struct pvr_device *const device = ctx->device;
            submit_info->frame_num = device->global_queue_present_count;
   submit_info->job_num = device->global_cmd_buffer_submit_count;
   submit_info->wait = wait;
            for (uint32_t i = 0U; i < submit->prep_count; i++) {
      struct pvr_winsys_transfer_cmd *const cmd = &submit_info->cmds[i];
            pvr_submit_info_stream_init(ctx, prep_data, cmd);
         }
      static VkResult pvr_submit_transfer(struct pvr_transfer_ctx *ctx,
                     {
                        return ctx->device->ws->ops->transfer_submit(ctx->ws_ctx,
                  }
      static VkResult pvr_queue_transfer(struct pvr_transfer_ctx *ctx,
                     {
      struct pvr_transfer_prep_data *prep_data = NULL;
   struct pvr_transfer_prep_data *prev_prep_data;
   struct pvr_transfer_submit submit = { 0U };
   bool finished = false;
   uint32_t pass = 0U;
            /* Transfer queue might decide to do a blit in multiple passes. When the
   * prepare doesn't set the finished flag this code will keep calling the
   * prepare with increasing pass. If queued transfers are submitted from
   * here we submit them straight away. That's why we only need a single
   * prepare for the blit rather then one for each pass. Otherwise we insert
   * each prepare into the prepare array. When the client does blit batching
   * and we split the blit into multiple passes each pass in each queued
   * transfer adds one more prepare. Thus the prepare array after 2
   * pvr_queue_transfer calls might look like:
   *
   * +------+------++-------+-------+-------+
   * |B0/P0 |B0/P1 || B1/P0 | B1/P1 | B1/P2 |
   * +------+------++-------+-------+-------+
   * F           S/U F                    S/U
   *
   * Bn/Pm : nth blit (queue transfer call) / mth prepare
   * F     : fence point
   * S/U   : update / server sync update point
            while (!finished) {
      prev_prep_data = prep_data;
            /* Clear down the memory before we write to this prep. */
            if (pass == 0U) {
      if (!pvr_3d_validate_addr(transfer_cmd))
      } else {
      /* Transfer queue workarounds could use more than one pass with 3D
   * path.
   */
               if (transfer_cmd->flags & PVR_TRANSFER_CMD_FLAGS_FAST2D) {
      result =
      } else {
      result =
      }
   if (result != VK_SUCCESS)
            /* Submit if we have finished the blit or if we are out of prepares. */
   if (finished || submit.prep_count == ARRAY_SIZE(submit.prep_array)) {
      result = pvr_submit_transfer(ctx,
                                    /* Check if we need to reset prep_count. */
   if (submit.prep_count == ARRAY_SIZE(submit.prep_array))
                              }
      VkResult pvr_transfer_job_submit(struct pvr_transfer_ctx *ctx,
                     {
      list_for_each_entry_safe (struct pvr_transfer_cmd,
                        /* The fw guarantees that any kick on the same context will be
   * synchronized in submission order. This means only the first kick must
   * wait, and only the last kick need signal.
   */
   struct vk_sync *first_cmd_wait_sync = NULL;
   struct vk_sync *last_cmd_signal_sync = NULL;
            if (list_first_entry(sub_cmd->transfer_cmds,
                              if (list_last_entry(sub_cmd->transfer_cmds,
                              result = pvr_queue_transfer(ctx,
                     if (result != VK_SUCCESS)
                  }
