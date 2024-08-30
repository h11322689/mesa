   /**************************************************************************
   *
   * Copyright 2010 Luca Barbieri
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial
   * portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
   * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "pipe/p_state.h"
   #include "util/format/u_format.h"
   #include "util/u_debug_describe.h"
   #include "util/u_string.h"
      void
   debug_describe_reference(char* buf, UNUSED const struct pipe_reference*ptr)
   {
         }
      void
   debug_describe_resource(char* buf, const struct pipe_resource *ptr)
   {
      switch(ptr->target)
   {
   case PIPE_BUFFER:
      sprintf(buf, "pipe_buffer<%u>", (unsigned)util_format_get_stride(ptr->format, ptr->width0));
      case PIPE_TEXTURE_1D:
      sprintf(buf, "pipe_texture1d<%u,%s,%u>", ptr->width0, util_format_short_name(ptr->format), ptr->last_level);
      case PIPE_TEXTURE_2D:
      sprintf(buf, "pipe_texture2d<%u,%u,%s,%u>", ptr->width0, ptr->height0, util_format_short_name(ptr->format), ptr->last_level);
      case PIPE_TEXTURE_RECT:
      sprintf(buf, "pipe_texture_rect<%u,%u,%s>", ptr->width0, ptr->height0, util_format_short_name(ptr->format));
      case PIPE_TEXTURE_CUBE:
      sprintf(buf, "pipe_texture_cube<%u,%u,%s,%u>", ptr->width0, ptr->height0, util_format_short_name(ptr->format), ptr->last_level);
      case PIPE_TEXTURE_3D:
      sprintf(buf, "pipe_texture3d<%u,%u,%u,%s,%u>", ptr->width0, ptr->height0, ptr->depth0, util_format_short_name(ptr->format), ptr->last_level);
      case PIPE_TEXTURE_1D_ARRAY:
      sprintf(buf, "pipe_texture_1darray<%u,%u,%s,%u>", ptr->width0, ptr->array_size, util_format_short_name(ptr->format), ptr->last_level);
      case PIPE_TEXTURE_2D_ARRAY:
      sprintf(buf, "pipe_texture_2darray<%u,%u,%u,%s,%u>", ptr->width0, ptr->height0, ptr->array_size, util_format_short_name(ptr->format), ptr->last_level);
      case PIPE_TEXTURE_CUBE_ARRAY:
      sprintf(buf, "pipe_texture_cubearray<%u,%u,%u,%s,%u>", ptr->width0, ptr->height0, ptr->array_size, util_format_short_name(ptr->format), ptr->last_level);
      default:
      sprintf(buf, "pipe_martian_resource<%u>", ptr->target);
         }
      void
   debug_describe_surface(char* buf, const struct pipe_surface *ptr)
   {
      char res[128];
   debug_describe_resource(res, ptr->texture);
      }
      void
   debug_describe_sampler_view(char* buf, const struct pipe_sampler_view *ptr)
   {
      char res[128];
   debug_describe_resource(res, ptr->texture);
      }
      void
   debug_describe_image_view(char* buf, const struct pipe_image_view *ptr)
   {
      char res[128];
   debug_describe_resource(res, ptr->resource);
   sprintf(buf, "pipe_image_view<%s,%s>", res,
      }
      void
   debug_describe_so_target(char* buf,
         {
      char res[128];
   debug_describe_resource(res, ptr->buffer);
   sprintf(buf, "pipe_stream_output_target<%s,%u,%u>", res,
      }
