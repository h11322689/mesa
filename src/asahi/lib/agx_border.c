   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "util/format/format_utils.h"
   #include "util/format/u_format.h"
   #include "util/half_float.h"
   #include "agx_formats.h"
   #include "agx_pack.h"
      /*
   * AGX allows the sampler descriptor to specify a custom border colour. The
   * packing depends on the texture format (i.e. no
   * customBorderColorWithoutFormat).
   *
   * Each channel is packed separately into 32-bit words. Pure integers are stored
   * as-is. Pure floats are extended to 16-bit/32-bit as appropriate. Normalized
   * formats are encoded as usual, except sRGB gets 4 extra bits.
   *
   * The texture descriptor swizzle is applied to the border colour. That swizzle
   * includes the format swizzle. In effect, we want to encode the border colour
   * like it would be encoded in memory, and then the swizzles work out
   * for Vulkan.
   */
      struct channel {
      enum util_format_type type;
   bool normalized;
      };
      static struct channel
   get_channel_info(enum pipe_format format, unsigned channel)
   {
      /* Compressed formats may have packing with no PIPE equivalent, handle
   * specially.
   */
   switch (format) {
   case PIPE_FORMAT_ETC2_R11_UNORM:
   case PIPE_FORMAT_ETC2_RG11_UNORM:
            case PIPE_FORMAT_ETC2_R11_SNORM:
   case PIPE_FORMAT_ETC2_RG11_SNORM:
            case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_RGTC2_UNORM:
         case PIPE_FORMAT_RGTC1_SNORM:
   case PIPE_FORMAT_RGTC2_SNORM:
            case PIPE_FORMAT_ETC1_RGB8:
   case PIPE_FORMAT_ETC2_RGB8:
   case PIPE_FORMAT_ETC2_RGBA8:
   case PIPE_FORMAT_ETC2_RGB8A1:
   case PIPE_FORMAT_BPTC_RGBA_UNORM:
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
            case PIPE_FORMAT_ETC2_SRGB8:
   case PIPE_FORMAT_ETC2_SRGBA8:
   case PIPE_FORMAT_ETC2_SRGB8A1:
   case PIPE_FORMAT_BPTC_SRGBA:
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
      return (struct channel){
      UTIL_FORMAT_TYPE_UNSIGNED,
   true,
            case PIPE_FORMAT_BPTC_RGB_FLOAT:
   case PIPE_FORMAT_BPTC_RGB_UFLOAT:
            default:
      assert(
      !util_format_is_compressed(format) &&
                           const struct util_format_description *desc = util_format_description(format);
   struct util_format_channel_description chan_desc = desc->channel[channel];
   bool srgb = (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) &&
            if (chan_desc.type == UTIL_FORMAT_TYPE_UNSIGNED ||
               assert((chan_desc.normalized ^ chan_desc.pure_integer) &&
               if (srgb && chan_desc.type != UTIL_FORMAT_TYPE_VOID) {
      assert(chan_desc.normalized && chan_desc.size == 8 &&
                     return (struct channel){
      .type = chan_desc.type,
   .normalized = chan_desc.normalized,
         }
      static uint32_t
   pack_channel(uint32_t value, enum pipe_format format, unsigned channel)
   {
               switch (chan.type) {
   case UTIL_FORMAT_TYPE_VOID:
            case UTIL_FORMAT_TYPE_UNSIGNED:
      if (chan.normalized)
         else
         case UTIL_FORMAT_TYPE_SIGNED:
      if (chan.normalized)
         else
         case UTIL_FORMAT_TYPE_FLOAT:
      assert(chan.size == 32 || chan.size <= 16);
         case UTIL_FORMAT_TYPE_FIXED:
                     }
      void
   agx_pack_border(struct agx_border_packed *out, const uint32_t in[4],
         {
               const struct util_format_description *desc = util_format_description(format);
            /* Determine the in-memory order of the format. That is the inverse of the
   * format swizzle. If a component is replicated, we use the first component,
   * by looping backwards and overwriting.
   */
   for (int i = 3; i >= 0; --i) {
      static_assert(PIPE_SWIZZLE_X == 0, "known ordering");
            if (desc->swizzle[i] <= PIPE_SWIZZLE_W)
               agx_pack(out, BORDER, cfg) {
      cfg.channel_0 = pack_channel(in[channel_map[0]], format, 0);
   cfg.channel_1 = pack_channel(in[channel_map[1]], format, 1);
   cfg.channel_2 = pack_channel(in[channel_map[2]], format, 2);
         }
