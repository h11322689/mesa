   #include "util/format/u_format.h"
   #include "zink_format.h"
   #include "util/u_math.h"
      enum pipe_format
   zink_decompose_vertex_format(enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
   unsigned first_non_void = util_format_get_first_non_void_channel(format);
   enum pipe_format new_format;
   assert(first_non_void == 0);
   if (!desc->is_array)
         if (desc->is_unorm) {
      enum pipe_format unorm_formats[] = {
      PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R16_UNORM,
      };
      } else if (desc->is_snorm) {
      enum pipe_format snorm_formats[] = {
      PIPE_FORMAT_R8_SNORM,
   PIPE_FORMAT_R16_SNORM,
      };
      } else {
      enum pipe_format uint_formats[][3] = {
      {PIPE_FORMAT_R8_USCALED, PIPE_FORMAT_R16_USCALED, PIPE_FORMAT_R32_USCALED},
      };
   enum pipe_format sint_formats[][3] = {
      {PIPE_FORMAT_R8_SSCALED, PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R32_SSCALED},
      };
   switch (desc->channel[first_non_void].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
         case UTIL_FORMAT_TYPE_SIGNED:
         case UTIL_FORMAT_TYPE_FLOAT:
      return desc->channel[first_non_void].size == 16 ? PIPE_FORMAT_R16_FLOAT : PIPE_FORMAT_R32_FLOAT;
      default:
            }
      }
      bool
   zink_format_is_red_alpha(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_R4A4_UNORM:
   case PIPE_FORMAT_R8A8_SINT:
   case PIPE_FORMAT_R8A8_SNORM:
   case PIPE_FORMAT_R8A8_UINT:
   case PIPE_FORMAT_R8A8_UNORM:
   case PIPE_FORMAT_R16A16_SINT:
   case PIPE_FORMAT_R16A16_SNORM:
   case PIPE_FORMAT_R16A16_UINT:
   case PIPE_FORMAT_R16A16_UNORM:
   case PIPE_FORMAT_R16A16_FLOAT:
   case PIPE_FORMAT_R32A32_SINT:
   case PIPE_FORMAT_R32A32_UINT:
   case PIPE_FORMAT_R32A32_FLOAT:
         default: break;
   }
      }
      bool
   zink_format_is_emulated_alpha(enum pipe_format format)
   {
      return util_format_is_alpha(format) ||
         util_format_is_luminance(format) ||
      }
      static enum pipe_format
   emulate_alpha(enum pipe_format format)
   {
      if (format == PIPE_FORMAT_A8_UNORM)
         if (format == PIPE_FORMAT_A8_UINT)
         if (format == PIPE_FORMAT_A8_SNORM)
         if (format == PIPE_FORMAT_A8_SINT)
         if (format == PIPE_FORMAT_A16_UNORM)
         if (format == PIPE_FORMAT_A16_UINT)
         if (format == PIPE_FORMAT_A16_SNORM)
         if (format == PIPE_FORMAT_A16_SINT)
         if (format == PIPE_FORMAT_A16_FLOAT)
         if (format == PIPE_FORMAT_A32_UINT)
         if (format == PIPE_FORMAT_A32_SINT)
         if (format == PIPE_FORMAT_A32_FLOAT)
            }
      static enum pipe_format
   emulate_red_alpha(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_R8A8_SINT:
         case PIPE_FORMAT_R8A8_SNORM:
         case PIPE_FORMAT_R8A8_UINT:
         case PIPE_FORMAT_R8A8_UNORM:
         case PIPE_FORMAT_R16A16_SINT:
         case PIPE_FORMAT_R16A16_SNORM:
         case PIPE_FORMAT_R16A16_UINT:
         case PIPE_FORMAT_R16A16_UNORM:
         case PIPE_FORMAT_R16A16_FLOAT:
         case PIPE_FORMAT_R32A32_SINT:
         case PIPE_FORMAT_R32A32_UINT:
         case PIPE_FORMAT_R32A32_FLOAT:
         default: break;
   }
      }
      enum pipe_format
   zink_format_get_emulated_alpha(enum pipe_format format)
   {
      if (util_format_is_alpha(format))
         if (util_format_is_luminance(format))
         if (util_format_is_luminance_alpha(format)) {
      if (format == PIPE_FORMAT_LATC2_UNORM)
         if (format == PIPE_FORMAT_LATC2_SNORM)
                           }
      bool
   zink_format_is_voidable_rgba_variant(enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
            if(desc->block.width != 1 ||
      desc->block.height != 1 ||
   (desc->block.bits != 32 && desc->block.bits != 64 &&
   desc->block.bits != 128))
         if (desc->nr_channels != 4)
            unsigned size = desc->channel[0].size;
   for(chan = 0; chan < 4; ++chan) {
      if(desc->channel[chan].size != size)
                  }
      void
   zink_format_clamp_channel_color(const struct util_format_description *desc, union pipe_color_union *dst, const union pipe_color_union *src, unsigned i)
   {
      int non_void = util_format_get_first_non_void_channel(desc->format);
            if (channel > PIPE_SWIZZLE_W || desc->channel[channel].type == UTIL_FORMAT_TYPE_VOID) {
      if (non_void != -1) {
      if (desc->channel[non_void].type == UTIL_FORMAT_TYPE_FLOAT) {
         } else {
      if (desc->channel[non_void].normalized)
         else if (desc->channel[non_void].type == UTIL_FORMAT_TYPE_SIGNED)
         else
         } else {
         }
               switch (desc->channel[channel].type) {
   case UTIL_FORMAT_TYPE_VOID:
      unreachable("handled above");
      case UTIL_FORMAT_TYPE_SIGNED:
      if (desc->channel[channel].normalized)
         else {
      dst->i[i] = MAX2(src->i[i], -(1<<(desc->channel[channel].size - 1)));
      }
      case UTIL_FORMAT_TYPE_UNSIGNED:
      if (desc->channel[channel].normalized)
         else
            case UTIL_FORMAT_TYPE_FIXED:
   case UTIL_FORMAT_TYPE_FLOAT:
      dst->ui[i] = src->ui[i];
         }
      void
   zink_format_clamp_channel_srgb(const struct util_format_description *desc, union pipe_color_union *dst, const union pipe_color_union *src, unsigned i)
   {
      unsigned channel = desc->swizzle[i];
   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB &&
      channel <= PIPE_SWIZZLE_W) {
   switch (desc->channel[channel].type) {
   case UTIL_FORMAT_TYPE_SIGNED:
   case UTIL_FORMAT_TYPE_UNSIGNED:
      dst->f[i] = CLAMP(src->f[i], 0.0, 1.0);
      default:
                        }
