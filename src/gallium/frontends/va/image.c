   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
   * Copyright 2014 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "pipe/p_screen.h"
      #include "util/u_memory.h"
   #include "util/u_handle_table.h"
   #include "util/u_surface.h"
   #include "util/u_video.h"
   #include "util/u_process.h"
      #include "vl/vl_winsys.h"
   #include "vl/vl_video_buffer.h"
      #include "va_private.h"
      static const VAImageFormat formats[] =
   {
      {VA_FOURCC('N','V','1','2')},
   {VA_FOURCC('P','0','1','0')},
   {VA_FOURCC('P','0','1','6')},
   {VA_FOURCC('I','4','2','0')},
   {VA_FOURCC('Y','V','1','2')},
   {VA_FOURCC('Y','U','Y','V')},
   {VA_FOURCC('Y','U','Y','2')},
   {VA_FOURCC('U','Y','V','Y')},
   {VA_FOURCC('Y','8','0','0')},
   {VA_FOURCC('4','4','4','P')},
   {VA_FOURCC('R','G','B','P')},
   {.fourcc = VA_FOURCC('B','G','R','A'), .byte_order = VA_LSB_FIRST, 32, 32,
   0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000},
   {.fourcc = VA_FOURCC('R','G','B','A'), .byte_order = VA_LSB_FIRST, 32, 32,
   0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000},
   {.fourcc = VA_FOURCC('A','R','G','B'), .byte_order = VA_LSB_FIRST, 32, 32,
   0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000},
   {.fourcc = VA_FOURCC('B','G','R','X'), .byte_order = VA_LSB_FIRST, 32, 24,
   0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000},
   {.fourcc = VA_FOURCC('R','G','B','X'), .byte_order = VA_LSB_FIRST, 32, 24,
      };
      static void
   vlVaVideoSurfaceSize(vlVaSurface *p_surf, int component,
         {
      *width = p_surf->templat.width;
            vl_video_buffer_adjust_size(width, height, component,
            }
      VAStatus
   vlVaQueryImageFormats(VADriverContextP ctx, VAImageFormat *format_list, int *num_formats)
   {
      struct pipe_screen *pscreen;
   enum pipe_format format;
                     if (!ctx)
            if (!(format_list && num_formats))
            *num_formats = 0;
   pscreen = VL_VA_PSCREEN(ctx);
   for (i = 0; i < ARRAY_SIZE(formats); ++i) {
      format = VaFourccToPipeFormat(formats[i].fourcc);
   if (pscreen->is_video_format_supported(pscreen, format,
      PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM))
               }
      VAStatus
   vlVaCreateImage(VADriverContextP ctx, VAImageFormat *format, int width, int height, VAImage *image)
   {
      VAStatus status;
   vlVaDriver *drv;
   VAImage *img;
            if (!ctx)
            if (!(format && image && width && height))
                     img = CALLOC(1, sizeof(VAImage));
   if (!img)
         mtx_lock(&drv->mutex);
   img->image_id = handle_table_add(drv->htab, img);
            img->format = *format;
   img->width = width;
   img->height = height;
   w = align(width, 2);
            switch (format->fourcc) {
   case VA_FOURCC('N','V','1','2'):
      img->num_planes = 2;
   img->pitches[0] = w;
   img->offsets[0] = 0;
   img->pitches[1] = w;
   img->offsets[1] = w * h;
   img->data_size  = w * h * 3 / 2;
         case VA_FOURCC('P','0','1','0'):
   case VA_FOURCC('P','0','1','6'):
      img->num_planes = 2;
   img->pitches[0] = w * 2;
   img->offsets[0] = 0;
   img->pitches[1] = w * 2;
   img->offsets[1] = w * h * 2;
   img->data_size  = w * h * 3;
         case VA_FOURCC('I','4','2','0'):
   case VA_FOURCC('Y','V','1','2'):
      img->num_planes = 3;
   img->pitches[0] = w;
   img->offsets[0] = 0;
   img->pitches[1] = w / 2;
   img->offsets[1] = w * h;
   img->pitches[2] = w / 2;
   img->offsets[2] = w * h * 5 / 4;
   img->data_size  = w * h * 3 / 2;
         case VA_FOURCC('U','Y','V','Y'):
   case VA_FOURCC('Y','U','Y','V'):
   case VA_FOURCC('Y','U','Y','2'):
      img->num_planes = 1;
   img->pitches[0] = w * 2;
   img->offsets[0] = 0;
   img->data_size  = w * h * 2;
         case VA_FOURCC('B','G','R','A'):
   case VA_FOURCC('R','G','B','A'):
   case VA_FOURCC('A','R','G','B'):
   case VA_FOURCC('B','G','R','X'):
   case VA_FOURCC('R','G','B','X'):
      img->num_planes = 1;
   img->pitches[0] = w * 4;
   img->offsets[0] = 0;
   img->data_size  = w * h * 4;
         case VA_FOURCC('Y','8','0','0'):
      img->num_planes = 1;
   img->pitches[0] = w;
   img->offsets[0] = 0;
   img->data_size  = w * h;
         case VA_FOURCC('4','4','4', 'P'):
   case VA_FOURCC('R','G','B', 'P'):
      img->num_planes = 3;
   img->offsets[0] = 0;
   img->offsets[1] = w * h;
   img->offsets[2] = w * h * 2;
   img->pitches[0] = w;
   img->pitches[1] = w;
   img->pitches[2] = w;
   img->data_size  = w * h * 3;
         default:
                  status =  vlVaCreateBuffer(ctx, 0, VAImageBufferType,
               if (status != VA_STATUS_SUCCESS)
                     }
      VAStatus
   vlVaDeriveImage(VADriverContextP ctx, VASurfaceID surface, VAImage *image)
   {
      vlVaDriver *drv;
   vlVaSurface *surf;
   vlVaBuffer *img_buf;
   VAImage *img;
   VAStatus status;
   struct pipe_screen *screen;
   struct pipe_surface **surfaces;
   struct pipe_video_buffer *new_buffer = NULL;
   int w;
   int h;
   int i;
   unsigned stride = 0;
            /* This function is used by some programs to test for hardware decoding, but on
   * AMD devices, the buffers default to interlaced, which causes this function to fail.
   * Some programs expect this function to fail, while others, assume this means
   * hardware acceleration is not available and give up without trying the fall-back
   * vaCreateImage + vaPutImage
   */
   const char *proc = util_get_process_name();
   const char *derive_interlaced_allowlist[] = {
         "vlc",
   "h264encode",
            if (!ctx)
                     if (!drv)
                     if (!screen)
                     if (!surf || !surf->buffer)
            if (surf->buffer->interlaced) {
      for (i = 0; i < ARRAY_SIZE(derive_interlaced_allowlist); i++)
                  if (i >= ARRAY_SIZE(derive_interlaced_allowlist) ||
      !screen->get_video_param(screen, PIPE_VIDEO_PROFILE_UNKNOWN,
               } else if (util_format_get_num_planes(surf->buffer->buffer_format) >= 2 &&
            !screen->get_video_param(screen, PIPE_VIDEO_PROFILE_UNKNOWN,
                     surfaces = surf->buffer->get_surfaces(surf->buffer);
   if (!surfaces || !surfaces[0]->texture)
            img = CALLOC(1, sizeof(VAImage));
   if (!img)
            img->format.fourcc = PipeFormatToVaFourcc(surf->buffer->buffer_format);
   img->buf = VA_INVALID_ID;
   /* Use the visible dimensions. */
   img->width = surf->templat.width;
   img->height = surf->templat.height;
   img->num_palette_entries = 0;
   img->entry_bytes = 0;
   /* Image data size is computed using internal dimensions. */
   w = align(surf->buffer->width, 2);
            for (i = 0; i < ARRAY_SIZE(formats); ++i) {
      if (img->format.fourcc == formats[i].fourcc) {
      img->format = formats[i];
                  mtx_lock(&drv->mutex);
   if (screen->resource_get_info) {
      screen->resource_get_info(screen, surfaces[0]->texture, &stride,
         if (!stride)
               img->num_planes = 1;
            switch (img->format.fourcc) {
   case VA_FOURCC('U','Y','V','Y'):
   case VA_FOURCC('Y','U','Y','V'):
      img->pitches[0] = stride > 0 ? stride : w * 2;
   assert(img->pitches[0] >= (w * 2));
   img->data_size  = img->pitches[0] * h;
         case VA_FOURCC('B','G','R','A'):
   case VA_FOURCC('R','G','B','A'):
   case VA_FOURCC('B','G','R','X'):
   case VA_FOURCC('R','G','B','X'):
      img->pitches[0] = stride > 0 ? stride : w * 4;
   assert(img->pitches[0] >= (w * 4));
   img->data_size  = img->pitches[0] * h;
         case VA_FOURCC('N','V','1','2'):
   case VA_FOURCC('P','0','1','0'):
   case VA_FOURCC('P','0','1','6'):
   {
      /* In some gallium platforms, the stride and offset are different*/
   /* for the Y and UV planes, query them independently.*/
   if (screen->resource_get_info) {
      /* resource_get_info is called above for surfaces[0]->texture and */
   /* saved results in stride, offset, reuse those values to avoid a new call to: */
   /* screen->resource_get_info(screen, surfaces[0]->texture, &img->pitches[0],*/
   /*                         &img->offsets[0]);*/
                  screen->resource_get_info(screen, surfaces[1]->texture, &img->pitches[1],
         if (!img->pitches[1])
               if (surf->buffer->interlaced) {
                     new_template = surf->templat;
                  /* not all devices support non-interlaced buffers */
   if (!new_buffer) {
      status = VA_STATUS_ERROR_OPERATION_FAILED;
               /* convert the interlaced to the progressive */
   src_rect.x0 = dst_rect.x0 = 0;
   src_rect.x1 = dst_rect.x1 = surf->templat.width;
                  vl_compositor_yuv_deint_full(&drv->cstate, &drv->compositor,
                        /* recalculate the values now that we have a new surface */
   surfaces = surf->buffer->get_surfaces(new_buffer);
   if (screen->resource_get_info) {
      screen->resource_get_info(screen, surfaces[0]->texture, &img->pitches[0],
                        screen->resource_get_info(screen, surfaces[1]->texture, &img->pitches[1],
         if (!img->pitches[1])
               w = align(new_buffer->width, 2);
               img->num_planes = 2;
   if(screen->resource_get_info) {
      /* Note this block might use h and w from the recalculated size if it entered
            } else {
      /* Use stride = w as default if screen->resource_get_info was not available */
   img->pitches[0] = w;
   img->pitches[1] = w;
   img->offsets[1] = w * h;
         } break;
   default:
      /* VaDeriveImage only supports contiguous planes. But there is now a
         status = VA_STATUS_ERROR_OPERATION_FAILED;
               img_buf = CALLOC(1, sizeof(vlVaBuffer));
   if (!img_buf) {
      status = VA_STATUS_ERROR_ALLOCATION_FAILED;
                        img_buf->type = VAImageBufferType;
   img_buf->size = img->data_size;
            pipe_resource_reference(&img_buf->derived_surface.resource, surfaces[0]->texture);
            if (surf->ctx)
            img->buf = handle_table_add(VL_VA_DRIVER(ctx)->htab, img_buf);
                           exit_on_error:
      FREE(img);
   mtx_unlock(&drv->mutex);
      }
      VAStatus
   vlVaDestroyImage(VADriverContextP ctx, VAImageID image)
   {
      vlVaDriver *drv;
   VAImage  *vaimage;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   vaimage = handle_table_get(drv->htab, image);
   if (!vaimage) {
      mtx_unlock(&drv->mutex);
               handle_table_remove(VL_VA_DRIVER(ctx)->htab, image);
   mtx_unlock(&drv->mutex);
   status = vlVaDestroyBuffer(ctx, vaimage->buf);
   FREE(vaimage);
      }
      VAStatus
   vlVaSetImagePalette(VADriverContextP ctx, VAImageID image, unsigned char *palette)
   {
      if (!ctx)
               }
      VAStatus
   vlVaGetImage(VADriverContextP ctx, VASurfaceID surface, int x, int y,
         {
      vlVaDriver *drv;
   vlVaSurface *surf;
   vlVaBuffer *img_buf;
   VAImage *vaimage;
   struct pipe_resource *view_resources[VL_NUM_COMPONENTS];
   enum pipe_format format;
   bool convert = false;
   uint8_t *data[3];
            if (!ctx)
                     mtx_lock(&drv->mutex);
   surf = handle_table_get(drv->htab, surface);
   if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
               vaimage = handle_table_get(drv->htab, image);
   if (!vaimage) {
      mtx_unlock(&drv->mutex);
               if (x < 0 || y < 0) {
      mtx_unlock(&drv->mutex);
               if (x + width > surf->templat.width ||
      y + height > surf->templat.height) {
   mtx_unlock(&drv->mutex);
               if (width > vaimage->width ||
      height > vaimage->height) {
   mtx_unlock(&drv->mutex);
               img_buf = handle_table_get(drv->htab, vaimage->buf);
   if (!img_buf) {
      mtx_unlock(&drv->mutex);
               format = VaFourccToPipeFormat(vaimage->format.fourcc);
   if (format == PIPE_FORMAT_NONE) {
      mtx_unlock(&drv->mutex);
                  if (format != surf->buffer->buffer_format) {
      /* support NV12 to YV12 and IYUV conversion now only */
   if ((format == PIPE_FORMAT_YV12 &&
      surf->buffer->buffer_format == PIPE_FORMAT_NV12) ||
   (format == PIPE_FORMAT_IYUV &&
   surf->buffer->buffer_format == PIPE_FORMAT_NV12))
      else if (format == PIPE_FORMAT_NV12 &&
      (surf->buffer->buffer_format == PIPE_FORMAT_P010 ||
   surf->buffer->buffer_format == PIPE_FORMAT_P016)) {
   mtx_unlock(&drv->mutex);
      }
   else {
      mtx_unlock(&drv->mutex);
                  memset(view_resources, 0, sizeof(view_resources));
            for (i = 0; i < MIN2(vaimage->num_planes, 3); i++) {
      data[i] = ((uint8_t*)img_buf->data) + vaimage->offsets[i];
      }
   if (vaimage->format.fourcc == VA_FOURCC('I','4','2','0')) {
      void *tmp_d;
   unsigned tmp_p;
   tmp_d  = data[1];
   data[1] = data[2];
   data[2] = tmp_d;
   tmp_p = pitches[1];
   pitches[1] = pitches[2];
               for (i = 0; i < vaimage->num_planes; i++) {
      unsigned box_w = align(width, 2);
   unsigned box_h = align(height, 2);
   unsigned box_x = x & ~1;
   unsigned box_y = y & ~1;
   if (!view_resources[i]) continue;
   vl_video_buffer_adjust_size(&box_w, &box_h, i,
               vl_video_buffer_adjust_size(&box_x, &box_y, i,
               for (j = 0; j < view_resources[i]->array_size; ++j) {
      struct pipe_box box = {box_x, box_y, j, box_w, box_h, 1};
   struct pipe_transfer *transfer;
   uint8_t *map;
   map = drv->pipe->texture_map(drv->pipe, view_resources[i], 0,
         if (!map) {
      mtx_unlock(&drv->mutex);
               if (i == 1 && convert) {
      u_copy_nv12_to_yv12((void *const *)data, pitches, i, j,
      transfer->stride, view_resources[i]->array_size,
   } else {
      util_copy_rect((uint8_t*)(data[i] + pitches[i] * j),
      view_resources[i]->format,
   pitches[i] * view_resources[i]->array_size, 0, 0,
   }
         }
               }
      VAStatus
   vlVaPutImage(VADriverContextP ctx, VASurfaceID surface, VAImageID image,
               {
      vlVaDriver *drv;
   vlVaSurface *surf;
   vlVaBuffer *img_buf;
   VAImage *vaimage;
   struct pipe_resource *view_resources[VL_NUM_COMPONENTS];
   enum pipe_format format;
   uint8_t *data[3];
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
            surf = handle_table_get(drv->htab, surface);
   if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
               vaimage = handle_table_get(drv->htab, image);
   if (!vaimage) {
      mtx_unlock(&drv->mutex);
               img_buf = handle_table_get(drv->htab, vaimage->buf);
   if (!img_buf) {
      mtx_unlock(&drv->mutex);
               if (img_buf->derived_surface.resource) {
      /* Attempting to transfer derived image to surface */
   mtx_unlock(&drv->mutex);
                        if (format == PIPE_FORMAT_NONE) {
      mtx_unlock(&drv->mutex);
               if ((format != surf->buffer->buffer_format) &&
         ((format != PIPE_FORMAT_YV12) || (surf->buffer->buffer_format != PIPE_FORMAT_NV12)) &&
               surf->templat.buffer_format = format;
   if (format == PIPE_FORMAT_YUYV || format == PIPE_FORMAT_UYVY ||
      format == PIPE_FORMAT_B8G8R8A8_UNORM || format == PIPE_FORMAT_B8G8R8X8_UNORM ||
   format == PIPE_FORMAT_R8G8B8A8_UNORM || format == PIPE_FORMAT_R8G8B8X8_UNORM)
               if (!tmp_buf) {
      mtx_unlock(&drv->mutex);
               surf->buffer->destroy(surf->buffer);
               memset(view_resources, 0, sizeof(view_resources));
            for (i = 0; i < MIN2(vaimage->num_planes, 3); i++) {
      data[i] = ((uint8_t*)img_buf->data) + vaimage->offsets[i];
      }
   if (vaimage->format.fourcc == VA_FOURCC('I','4','2','0')) {
      void *tmp_d;
   unsigned tmp_p;
   tmp_d  = data[1];
   data[1] = data[2];
   data[2] = tmp_d;
   tmp_p = pitches[1];
   pitches[1] = pitches[2];
               for (i = 0; i < vaimage->num_planes; ++i) {
      unsigned width, height;
            if (!view_resources[i]) continue;
            vlVaVideoSurfaceSize(surf, i, &width, &height);
   for (j = 0; j < tex->array_size; ++j) {
               if (((format == PIPE_FORMAT_YV12) || (format == PIPE_FORMAT_IYUV))
      && (surf->buffer->buffer_format == PIPE_FORMAT_NV12)
   && i == 1) {
                  map = drv->pipe->texture_map(drv->pipe,
                                 if (map == NULL) {
                        u_copy_nv12_from_yv12((const void * const*) data, pitches, i, j,
                  } else {
      drv->pipe->texture_subdata(drv->pipe, tex, 0,
                        }
   drv->pipe->flush(drv->pipe, NULL, 0);
               }
