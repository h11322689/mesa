   /**************************************************************************
   *
   * Copyright 2017 Advanced Micro Devices, Inc.
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
      #include "va_private.h"
      void vlVaHandlePictureParameterBufferMJPEG(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VAPictureParameterBufferJPEGBaseline *mjpeg = buf->data;
   unsigned sf;
                     context->desc.mjpeg.picture_parameter.picture_width = mjpeg->picture_width;
            STATIC_ASSERT(sizeof(mjpeg->components) ==
         for (i = 0; i < MIN2(mjpeg->num_components, ARRAY_SIZE(mjpeg->components)); ++i) {
      context->desc.mjpeg.picture_parameter.components[i].component_id =
         context->desc.mjpeg.picture_parameter.components[i].h_sampling_factor =
         context->desc.mjpeg.picture_parameter.components[i].v_sampling_factor =
         context->desc.mjpeg.picture_parameter.components[i].quantiser_table_selector =
            sf = mjpeg->components[i].h_sampling_factor << 4 | mjpeg->components[i].v_sampling_factor;
   context->mjpeg.sampling_factor <<= 8;
                        context->desc.mjpeg.picture_parameter.crop_x = mjpeg->va_reserved[0] & 0xffff;
   context->desc.mjpeg.picture_parameter.crop_y = (mjpeg->va_reserved[0] >> 16) & 0xffff;
   context->desc.mjpeg.picture_parameter.crop_width = mjpeg->va_reserved[1] & 0xffff;
         }
      void vlVaHandleIQMatrixBufferMJPEG(vlVaContext *context, vlVaBuffer *buf)
   {
                        memcpy(&context->desc.mjpeg.quantization_table.load_quantiser_table, mjpeg->load_quantiser_table, 4);
      }
      void vlVaHandleHuffmanTableBufferType(vlVaContext *context, vlVaBuffer *buf)
   {
      VAHuffmanTableBufferJPEGBaseline *mjpeg = buf->data;
                     STATIC_ASSERT(sizeof(mjpeg->load_huffman_table) ==
         for (i = 0; i < 2; ++i) {
               memcpy(&context->desc.mjpeg.huffman_table.table[i].num_dc_codes,
         memcpy(&context->desc.mjpeg.huffman_table.table[i].dc_values,
         memcpy(&context->desc.mjpeg.huffman_table.table[i].num_ac_codes,
         memcpy(&context->desc.mjpeg.huffman_table.table[i].ac_values,
               }
      void vlVaHandleSliceParameterBufferMJPEG(vlVaContext *context, vlVaBuffer *buf)
   {
      VASliceParameterBufferJPEGBaseline *mjpeg = buf->data;
                     context->desc.mjpeg.slice_parameter.slice_data_size = mjpeg->slice_data_size;
   context->desc.mjpeg.slice_parameter.slice_data_offset = mjpeg->slice_data_offset;
   context->desc.mjpeg.slice_parameter.slice_data_flag = mjpeg->slice_data_flag;
   context->desc.mjpeg.slice_parameter.slice_horizontal_position = mjpeg->slice_horizontal_position;
            STATIC_ASSERT(sizeof(mjpeg->components) ==
         for (i = 0; i < MIN2(mjpeg->num_components, ARRAY_SIZE(mjpeg->components)); ++i) {
      context->desc.mjpeg.slice_parameter.components[i].component_selector =
         context->desc.mjpeg.slice_parameter.components[i].dc_table_selector =
         context->desc.mjpeg.slice_parameter.components[i].ac_table_selector =
               context->desc.mjpeg.slice_parameter.num_components = mjpeg->num_components;
   context->desc.mjpeg.slice_parameter.restart_interval = mjpeg->restart_interval;
      }
      void vlVaGetJpegSliceHeader(vlVaContext *context)
   {
      int size = 0, saved_size, len_pos, i;
   uint16_t *bs;
            /* SOI */
   p[size++] = 0xff;
            /* DQT */
   p[size++] = 0xff;
            len_pos = size++;
            for (i = 0; i < 4; ++i) {
      if (context->desc.mjpeg.quantization_table.load_quantiser_table[i] == 0)
            p[size++] = i;
   memcpy((p + size), &context->desc.mjpeg.quantization_table.quantiser_table[i], 64);
               bs = (uint16_t*)&p[len_pos];
                     /* DHT */
   p[size++] = 0xff;
            len_pos = size++;
            for (i = 0; i < 2; ++i) {
               if (context->desc.mjpeg.huffman_table.load_huffman_table[i] == 0)
            p[size++] = 0x00 | i;
   memcpy((p + size), &context->desc.mjpeg.huffman_table.table[i].num_dc_codes, 16);
   size += 16;
   for (j = 0; j < 16; ++j)
         assert(num <= 12);
   memcpy((p + size), &context->desc.mjpeg.huffman_table.table[i].dc_values, num);
               for (i = 0; i < 2; ++i) {
               if (context->desc.mjpeg.huffman_table.load_huffman_table[i] == 0)
            p[size++] = 0x10 | i;
   memcpy((p + size), &context->desc.mjpeg.huffman_table.table[i].num_ac_codes, 16);
   size += 16;
   for (j = 0; j < 16; ++j)
         assert(num <= 162);
   memcpy((p + size), &context->desc.mjpeg.huffman_table.table[i].ac_values, num);
               bs = (uint16_t*)&p[len_pos];
                     /* DRI */
   if (context->desc.mjpeg.slice_parameter.restart_interval) {
      p[size++] = 0xff;
   p[size++] = 0xdd;
   p[size++] = 0x00;
   p[size++] = 0x04;
   bs = (uint16_t*)&p[size++];
   *bs = util_bswap16(context->desc.mjpeg.slice_parameter.restart_interval);
               /* SOF */
   p[size++] = 0xff;
            len_pos = size++;
                     bs = (uint16_t*)&p[size++];
   *bs = util_bswap16(context->desc.mjpeg.picture_parameter.picture_height);
            bs = (uint16_t*)&p[size++];
   *bs = util_bswap16(context->desc.mjpeg.picture_parameter.picture_width);
                     for (i = 0; i < context->desc.mjpeg.picture_parameter.num_components; ++i) {
      p[size++] = context->desc.mjpeg.picture_parameter.components[i].component_id;
   p[size++] = context->desc.mjpeg.picture_parameter.components[i].h_sampling_factor << 4 |
                     bs = (uint16_t*)&p[len_pos];
                     /* SOS */
   p[size++] = 0xff;
            len_pos = size++;
                     for (i = 0; i < context->desc.mjpeg.slice_parameter.num_components; ++i) {
      p[size++] = context->desc.mjpeg.slice_parameter.components[i].component_selector;
   p[size++] = context->desc.mjpeg.slice_parameter.components[i].dc_table_selector << 4 |
               p[size++] = 0x00;
   p[size++] = 0x3f;
            bs = (uint16_t*)&p[len_pos];
               }
