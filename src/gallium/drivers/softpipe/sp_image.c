   /*
   * Copyright 2016 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "sp_context.h"
   #include "sp_image.h"
   #include "sp_texture.h"
      #include "util/format/u_format.h"
      /*
   * Get the offset into the base image
   * first element for a buffer or layer/level for texture.
   */
   static uint32_t
   get_image_offset(const struct softpipe_resource *spr,
               {
               if (spr->base.target == PIPE_BUFFER)
            if (spr->base.target == PIPE_TEXTURE_1D_ARRAY ||
      spr->base.target == PIPE_TEXTURE_2D_ARRAY ||
   spr->base.target == PIPE_TEXTURE_CUBE_ARRAY ||
   spr->base.target == PIPE_TEXTURE_CUBE ||
   spr->base.target == PIPE_TEXTURE_3D)
         }
      /*
   * Does this texture instruction have a layer or depth parameter.
   */
   static inline bool
   has_layer_or_depth(unsigned tgsi_tex_instr)
   {
      return (tgsi_tex_instr == TGSI_TEXTURE_3D ||
         tgsi_tex_instr == TGSI_TEXTURE_CUBE ||
   tgsi_tex_instr == TGSI_TEXTURE_1D_ARRAY ||
   tgsi_tex_instr == TGSI_TEXTURE_2D_ARRAY ||
      }
      /*
   * Is this texture instruction a single non-array coordinate.
   */
   static inline bool
   has_1coord(unsigned tgsi_tex_instr)
   {
      return (tgsi_tex_instr == TGSI_TEXTURE_BUFFER ||
            }
      /*
   * check the bounds vs w/h/d
   */
   static inline bool
   bounds_check(int width, int height, int depth,
         {
      if (s < 0 || s >= width)
         if (t < 0 || t >= height)
         if (r < 0 || r >= depth)
            }
      /*
   * Checks if the texture target compatible with the image resource
   * pipe target.
   */
   static inline bool
   has_compat_target(unsigned pipe_target, unsigned tgsi_target)
   {
      switch (pipe_target) {
   case PIPE_TEXTURE_1D:
      if (tgsi_target == TGSI_TEXTURE_1D)
            case PIPE_TEXTURE_2D:
      if (tgsi_target == TGSI_TEXTURE_2D)
            case PIPE_TEXTURE_RECT:
      if (tgsi_target == TGSI_TEXTURE_RECT)
            case PIPE_TEXTURE_3D:
      if (tgsi_target == TGSI_TEXTURE_3D ||
      tgsi_target == TGSI_TEXTURE_2D)
         case PIPE_TEXTURE_CUBE:
      if (tgsi_target == TGSI_TEXTURE_CUBE ||
      tgsi_target == TGSI_TEXTURE_2D)
         case PIPE_TEXTURE_1D_ARRAY:
      if (tgsi_target == TGSI_TEXTURE_1D ||
      tgsi_target == TGSI_TEXTURE_1D_ARRAY)
         case PIPE_TEXTURE_2D_ARRAY:
      if (tgsi_target == TGSI_TEXTURE_2D ||
      tgsi_target == TGSI_TEXTURE_2D_ARRAY)
         case PIPE_TEXTURE_CUBE_ARRAY:
      if (tgsi_target == TGSI_TEXTURE_CUBE ||
      tgsi_target == TGSI_TEXTURE_CUBE_ARRAY ||
   tgsi_target == TGSI_TEXTURE_2D)
         case PIPE_BUFFER:
         }
      }
      static bool
   get_dimensions(const struct pipe_image_view *iview,
                  const struct softpipe_resource *spr,
   unsigned tgsi_tex_instr,
   enum pipe_format pformat,
      {
      if (tgsi_tex_instr == TGSI_TEXTURE_BUFFER) {
      *width = iview->u.buf.size / util_format_get_blocksize(pformat);
   *height = 1;
   *depth = 1;
   /*
   * Bounds check the buffer size from the view
   * and the buffer size from the underlying buffer.
   */
   if (util_format_get_stride(pformat, *width) >
      util_format_get_stride(spr->base.format, spr->base.width0))
   } else {
               level = spr->base.target == PIPE_BUFFER ? 0 : iview->u.tex.level;
   *width = u_minify(spr->base.width0, level);
            if (spr->base.target == PIPE_TEXTURE_3D)
         else
            /* Make sure the resource and view have compatible formats */
   if (util_format_get_blocksize(pformat) >
      util_format_get_blocksize(spr->base.format))
   }
      }
      static void
   fill_coords(const struct tgsi_image_params *params,
               unsigned index,
   const int s[TGSI_QUAD_SIZE],
   const int t[TGSI_QUAD_SIZE],
   {
      *s_coord = s[index];
   *t_coord = has_1coord(params->tgsi_tex_instr) ? 0 : t[index];
   *r_coord = has_layer_or_depth(params->tgsi_tex_instr) ?
      }
   /*
   * Implement the image LOAD operation.
   */
   static void
   sp_tgsi_load(const struct tgsi_image *image,
               const struct tgsi_image_params *params,
   const int s[TGSI_QUAD_SIZE],
   const int t[TGSI_QUAD_SIZE],
   const int r[TGSI_QUAD_SIZE],
   {
      struct sp_tgsi_image *sp_img = (struct sp_tgsi_image *)image;
   struct pipe_image_view *iview;
   struct softpipe_resource *spr;
   unsigned width, height, depth;
   unsigned stride;
   int c, j;
   char *data_ptr;
            if (params->unit >= PIPE_MAX_SHADER_IMAGES)
         iview = &sp_img->sp_iview[params->unit];
   spr = (struct softpipe_resource *)iview->resource;
   if (!spr)
            if (!has_compat_target(spr->base.target, params->tgsi_tex_instr))
            if (!get_dimensions(iview, spr, params->tgsi_tex_instr,
                           for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      int s_coord, t_coord, r_coord;
            if (!(params->execmask & (1 << j)))
            fill_coords(params, j, s, t, r, &s_coord, &t_coord, &r_coord);
   if (!bounds_check(width, height, depth,
                  if (fill_zero) {
      int nc = util_format_get_nr_components(params->format);
   int ival = util_format_is_pure_integer(params->format);
   for (c = 0; c < 4; c++) {
      rgba[c][j] = 0;
   if (c == 3 && nc < 4) {
      if (ival)
         else
         }
      }
   offset = get_image_offset(spr, iview, params->format, r_coord);
            uint32_t sdata[4];
   util_format_read_4(params->format,
                     for (c = 0; c < 4; c++)
      }
      fail_write_all_zero:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      for (c = 0; c < 4; c++)
      }
      }
      /*
   * Implement the image STORE operation.
   */
   static void
   sp_tgsi_store(const struct tgsi_image *image,
               const struct tgsi_image_params *params,
   const int s[TGSI_QUAD_SIZE],
   const int t[TGSI_QUAD_SIZE],
   const int r[TGSI_QUAD_SIZE],
   {
      struct sp_tgsi_image *sp_img = (struct sp_tgsi_image *)image;
   struct pipe_image_view *iview;
   struct softpipe_resource *spr;
   unsigned width, height, depth;
   unsigned stride;
   char *data_ptr;
   int j, c;
   unsigned offset = 0;
            if (params->unit >= PIPE_MAX_SHADER_IMAGES)
         iview = &sp_img->sp_iview[params->unit];
   spr = (struct softpipe_resource *)iview->resource;
   if (!spr)
         if (!has_compat_target(spr->base.target, params->tgsi_tex_instr))
            if (params->format == PIPE_FORMAT_NONE)
            if (!get_dimensions(iview, spr, params->tgsi_tex_instr,
                           for (j = 0; j < TGSI_QUAD_SIZE; j++) {
               if (!(params->execmask & (1 << j)))
            fill_coords(params, j, s, t, r, &s_coord, &t_coord, &r_coord);
   if (!bounds_check(width, height, depth,
                  offset = get_image_offset(spr, iview, pformat, r_coord);
            uint32_t sdata[4];
   for (c = 0; c < 4; c++)
         util_format_write_4(pformat, sdata, 0, data_ptr, stride,
         }
      /*
   * Implement atomic operations on unsigned integers.
   */
   static void
   handle_op_uint(const struct pipe_image_view *iview,
                  const struct tgsi_image_params *params,
   bool just_read,
   char *data_ptr,
   uint qi,
   unsigned stride,
   enum tgsi_opcode opcode,
   int s,
      {
      uint c;
   int nc = util_format_get_nr_components(params->format);
            util_format_read_4(params->format,
                        if (just_read) {
      for (c = 0; c < nc; c++) {
         }
      }
   switch (opcode) {
   case TGSI_OPCODE_ATOMUADD:
      for (c = 0; c < nc; c++) {
      unsigned temp = sdata[c];
   sdata[c] += ((uint32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMXCHG:
      for (c = 0; c < nc; c++) {
      unsigned temp = sdata[c];
   sdata[c] = ((uint32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMCAS:
      for (c = 0; c < nc; c++) {
      unsigned dst_x = sdata[c];
   unsigned cmp_x = ((uint32_t *)rgba[c])[qi];
   unsigned src_x = ((uint32_t *)rgba2[c])[qi];
   unsigned temp = sdata[c];
   sdata[c] = (dst_x == cmp_x) ? src_x : dst_x;
      }
      case TGSI_OPCODE_ATOMAND:
      for (c = 0; c < nc; c++) {
      unsigned temp = sdata[c];
   sdata[c] &= ((uint32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMOR:
      for (c = 0; c < nc; c++) {
      unsigned temp = sdata[c];
   sdata[c] |= ((uint32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMXOR:
      for (c = 0; c < nc; c++) {
      unsigned temp = sdata[c];
   sdata[c] ^= ((uint32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMUMIN:
      for (c = 0; c < nc; c++) {
      unsigned dst_x = sdata[c];
   unsigned src_x = ((uint32_t *)rgba[c])[qi];
   sdata[c] = MIN2(dst_x, src_x);
      }
      case TGSI_OPCODE_ATOMUMAX:
      for (c = 0; c < nc; c++) {
      unsigned dst_x = sdata[c];
   unsigned src_x = ((uint32_t *)rgba[c])[qi];
   sdata[c] = MAX2(dst_x, src_x);
      }
      case TGSI_OPCODE_ATOMIMIN:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int src_x = ((uint32_t *)rgba[c])[qi];
   sdata[c] = MIN2(dst_x, src_x);
      }
      case TGSI_OPCODE_ATOMIMAX:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int src_x = ((uint32_t *)rgba[c])[qi];
   sdata[c] = MAX2(dst_x, src_x);
      }
      default:
      assert(!"Unexpected TGSI opcode in sp_tgsi_op");
      }
   util_format_write_4(params->format, sdata, 0, data_ptr, stride,
      }
      /*
   * Implement atomic operations on signed integers.
   */
   static void
   handle_op_int(const struct pipe_image_view *iview,
               const struct tgsi_image_params *params,
   bool just_read,
   char *data_ptr,
   uint qi,
   unsigned stride,
   enum tgsi_opcode opcode,
   int s,
   int t,
   {
      uint c;
   int nc = util_format_get_nr_components(params->format);
   int sdata[4];
   util_format_read_4(params->format,
                        if (just_read) {
      for (c = 0; c < nc; c++) {
         }
      }
   switch (opcode) {
   case TGSI_OPCODE_ATOMUADD:
      for (c = 0; c < nc; c++) {
      int temp = sdata[c];
   sdata[c] += ((int32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMXCHG:
      for (c = 0; c < nc; c++) {
      int temp = sdata[c];
   sdata[c] = ((int32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMCAS:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int cmp_x = ((int32_t *)rgba[c])[qi];
   int src_x = ((int32_t *)rgba2[c])[qi];
   int temp = sdata[c];
   sdata[c] = (dst_x == cmp_x) ? src_x : dst_x;
      }
      case TGSI_OPCODE_ATOMAND:
      for (c = 0; c < nc; c++) {
      int temp = sdata[c];
   sdata[c] &= ((int32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMOR:
      for (c = 0; c < nc; c++) {
      int temp = sdata[c];
   sdata[c] |= ((int32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMXOR:
      for (c = 0; c < nc; c++) {
      int temp = sdata[c];
   sdata[c] ^= ((int32_t *)rgba[c])[qi];
      }
      case TGSI_OPCODE_ATOMUMIN:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int src_x = ((int32_t *)rgba[c])[qi];
   sdata[c] = MIN2(dst_x, src_x);
      }
      case TGSI_OPCODE_ATOMUMAX:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int src_x = ((int32_t *)rgba[c])[qi];
   sdata[c] = MAX2(dst_x, src_x);
      }
      case TGSI_OPCODE_ATOMIMIN:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int src_x = ((int32_t *)rgba[c])[qi];
   sdata[c] = MIN2(dst_x, src_x);
      }
      case TGSI_OPCODE_ATOMIMAX:
      for (c = 0; c < nc; c++) {
      int dst_x = sdata[c];
   int src_x = ((int32_t *)rgba[c])[qi];
   sdata[c] = MAX2(dst_x, src_x);
      }
      default:
      assert(!"Unexpected TGSI opcode in sp_tgsi_op");
      }
   util_format_write_4(params->format, sdata, 0, data_ptr, stride,
      }
      /* GLES OES_shader_image_atomic.txt allows XCHG on R32F */
   static void
   handle_op_r32f_xchg(const struct pipe_image_view *iview,
                     const struct tgsi_image_params *params,
   bool just_read,
   char *data_ptr,
   uint qi,
   unsigned stride,
   enum tgsi_opcode opcode,
   {
      float sdata[4];
   uint c;
   int nc = 1;
   util_format_read_4(params->format,
                     if (just_read) {
      for (c = 0; c < nc; c++) {
         }
               for (c = 0; c < nc; c++) {
      int temp = sdata[c];
   sdata[c] = ((float *)rgba[c])[qi];
      }
   util_format_write_4(params->format, sdata, 0, data_ptr, stride,
      }
      /*
   * Implement atomic image operations.
   */
   static void
   sp_tgsi_op(const struct tgsi_image *image,
            const struct tgsi_image_params *params,
   enum tgsi_opcode opcode,
   const int s[TGSI_QUAD_SIZE],
   const int t[TGSI_QUAD_SIZE],
   const int r[TGSI_QUAD_SIZE],
   const int sample[TGSI_QUAD_SIZE],
      {
      struct sp_tgsi_image *sp_img = (struct sp_tgsi_image *)image;
   struct pipe_image_view *iview;
   struct softpipe_resource *spr;
   unsigned width, height, depth;
   unsigned stride;
   int j, c;
   unsigned offset;
            if (params->unit >= PIPE_MAX_SHADER_IMAGES)
         iview = &sp_img->sp_iview[params->unit];
   spr = (struct softpipe_resource *)iview->resource;
   if (!spr)
         if (!has_compat_target(spr->base.target, params->tgsi_tex_instr))
            if (!get_dimensions(iview, spr, params->tgsi_tex_instr,
                           for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      int s_coord, t_coord, r_coord;
            fill_coords(params, j, s, t, r, &s_coord, &t_coord, &r_coord);
   if (!bounds_check(width, height, depth,
            int nc = util_format_get_nr_components(params->format);
   int ival = util_format_is_pure_integer(params->format);
   int c;
   for (c = 0; c < 4; c++) {
      rgba[c][j] = 0;
   if (c == 3 && nc < 4) {
      if (ival)
         else
         }
               /* just readback the value for atomic if execmask isn't set */
   if (!(params->execmask & (1 << j))) {
                  offset = get_image_offset(spr, iview, params->format, r_coord);
            /* we should see atomic operations on r32 formats */
   if (util_format_is_pure_uint(params->format))
      handle_op_uint(iview, params, just_read, data_ptr, j, stride,
      else if (util_format_is_pure_sint(params->format))
      handle_op_int(iview, params, just_read, data_ptr, j, stride,
      else if (params->format == PIPE_FORMAT_R32_FLOAT &&
            handle_op_r32f_xchg(iview, params, just_read, data_ptr, j, stride,
      else
      }
      fail_write_all_zero:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      for (c = 0; c < 4; c++)
      }
      }
      static void
   sp_tgsi_get_dims(const struct tgsi_image *image,
               {
      struct sp_tgsi_image *sp_img = (struct sp_tgsi_image *)image;
   struct pipe_image_view *iview;
   struct softpipe_resource *spr;
            if (params->unit >= PIPE_MAX_SHADER_IMAGES)
         iview = &sp_img->sp_iview[params->unit];
   spr = (struct softpipe_resource *)iview->resource;
   if (!spr)
            if (params->tgsi_tex_instr == TGSI_TEXTURE_BUFFER) {
      dims[0] = iview->u.buf.size / util_format_get_blocksize(iview->format);
   dims[1] = dims[2] = dims[3] = 0;
               level = iview->u.tex.level;
   dims[0] = u_minify(spr->base.width0, level);
   switch (params->tgsi_tex_instr) {
   case TGSI_TEXTURE_1D_ARRAY:
      dims[1] = iview->u.tex.last_layer - iview->u.tex.first_layer + 1;
      case TGSI_TEXTURE_1D:
         case TGSI_TEXTURE_2D_ARRAY:
      dims[2] = iview->u.tex.last_layer - iview->u.tex.first_layer + 1;
      case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_RECT:
      dims[1] = u_minify(spr->base.height0, level);
      case TGSI_TEXTURE_3D:
      dims[1] = u_minify(spr->base.height0, level);
   dims[2] = u_minify(spr->base.depth0, level);
      case TGSI_TEXTURE_CUBE_ARRAY:
      dims[1] = u_minify(spr->base.height0, level);
   dims[2] = (iview->u.tex.last_layer - iview->u.tex.first_layer + 1) / 6;
      default:
      assert(!"unexpected texture target in sp_get_dims()");
         }
      struct sp_tgsi_image *
   sp_create_tgsi_image(void)
   {
      struct sp_tgsi_image *img = CALLOC_STRUCT(sp_tgsi_image);
   if (!img)
            img->base.load = sp_tgsi_load;
   img->base.store = sp_tgsi_store;
   img->base.op = sp_tgsi_op;
   img->base.get_dims = sp_tgsi_get_dims;
      };
