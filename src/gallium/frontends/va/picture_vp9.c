   /**************************************************************************
   *
   * Copyright 2018 Advanced Micro Devices, Inc.
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
      #include "util/vl_vlc.h"
   #include "va_private.h"
      #define NUM_VP9_REFS 8
      void vlVaHandlePictureParameterBufferVP9(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VADecPictureParameterBufferVP9 *vp9 = buf->data;
                     context->desc.vp9.picture_parameter.prev_frame_width = context->desc.vp9.picture_parameter.frame_width;
   context->desc.vp9.picture_parameter.prev_frame_height = context->desc.vp9.picture_parameter.frame_height;
   context->desc.vp9.picture_parameter.frame_width = vp9->frame_width;
            context->desc.vp9.picture_parameter.pic_fields.subsampling_x = vp9->pic_fields.bits.subsampling_x;
   context->desc.vp9.picture_parameter.pic_fields.subsampling_y = vp9->pic_fields.bits.subsampling_y;
   context->desc.vp9.picture_parameter.pic_fields.frame_type = vp9->pic_fields.bits.frame_type;
   context->desc.vp9.picture_parameter.pic_fields.prev_show_frame = context->desc.vp9.picture_parameter.pic_fields.show_frame;
   context->desc.vp9.picture_parameter.pic_fields.show_frame = vp9->pic_fields.bits.show_frame;
   context->desc.vp9.picture_parameter.pic_fields.error_resilient_mode = vp9->pic_fields.bits.error_resilient_mode;
   context->desc.vp9.picture_parameter.pic_fields.intra_only = vp9->pic_fields.bits.intra_only;
   context->desc.vp9.picture_parameter.pic_fields.allow_high_precision_mv = vp9->pic_fields.bits.allow_high_precision_mv;
   context->desc.vp9.picture_parameter.pic_fields.mcomp_filter_type = vp9->pic_fields.bits.mcomp_filter_type;
   context->desc.vp9.picture_parameter.pic_fields.frame_parallel_decoding_mode = vp9->pic_fields.bits.frame_parallel_decoding_mode;
   context->desc.vp9.picture_parameter.pic_fields.reset_frame_context = vp9->pic_fields.bits.reset_frame_context;
   context->desc.vp9.picture_parameter.pic_fields.refresh_frame_context = vp9->pic_fields.bits.refresh_frame_context;
   context->desc.vp9.picture_parameter.pic_fields.frame_context_idx = vp9->pic_fields.bits.frame_context_idx;
   context->desc.vp9.picture_parameter.pic_fields.segmentation_enabled = vp9->pic_fields.bits.segmentation_enabled;
   context->desc.vp9.picture_parameter.pic_fields.segmentation_temporal_update = vp9->pic_fields.bits.segmentation_temporal_update;
   context->desc.vp9.picture_parameter.pic_fields.segmentation_update_map = vp9->pic_fields.bits.segmentation_update_map;
   context->desc.vp9.picture_parameter.pic_fields.last_ref_frame = vp9->pic_fields.bits.last_ref_frame;
   context->desc.vp9.picture_parameter.pic_fields.last_ref_frame_sign_bias = vp9->pic_fields.bits.last_ref_frame_sign_bias;
   context->desc.vp9.picture_parameter.pic_fields.golden_ref_frame = vp9->pic_fields.bits.golden_ref_frame;
   context->desc.vp9.picture_parameter.pic_fields.golden_ref_frame_sign_bias = vp9->pic_fields.bits.golden_ref_frame_sign_bias;
   context->desc.vp9.picture_parameter.pic_fields.alt_ref_frame = vp9->pic_fields.bits.alt_ref_frame;
   context->desc.vp9.picture_parameter.pic_fields.alt_ref_frame_sign_bias = vp9->pic_fields.bits.alt_ref_frame_sign_bias;
            context->desc.vp9.picture_parameter.filter_level = vp9->filter_level;
            context->desc.vp9.picture_parameter.log2_tile_rows = vp9->log2_tile_rows;
            context->desc.vp9.picture_parameter.frame_header_length_in_bytes = vp9->frame_header_length_in_bytes;
            for (i = 0; i < 7; ++i)
         for (i = 0; i < 3; ++i)
                              for (i = 0 ; i < NUM_VP9_REFS ; i++) {
      if (vp9->pic_fields.bits.frame_type == 0)
         else
               if (!context->decoder && !context->templat.max_references)
            context->desc.vp9.slice_parameter.slice_count = 0;
   context->desc.vp9.slice_parameter.slice_info_present = false;
   memset(context->desc.vp9.slice_parameter.slice_data_flag, 0,
         memset(context->desc.vp9.slice_parameter.slice_data_offset, 0,
         memset(context->desc.vp9.slice_parameter.slice_data_size, 0,
      }
      void vlVaHandleSliceParameterBufferVP9(vlVaContext *context, vlVaBuffer *buf)
   {
      VASliceParameterBufferVP9 *vp9 = buf->data;
                     ASSERTED const size_t max_pipe_vp9_slices = ARRAY_SIZE(context->desc.vp9.slice_parameter.slice_data_offset);
            context->desc.vp9.slice_parameter.slice_info_present = true;
   context->desc.vp9.slice_parameter.slice_data_size[context->desc.vp9.slice_parameter.slice_count] =
         context->desc.vp9.slice_parameter.slice_data_offset[context->desc.vp9.slice_parameter.slice_count] =
            switch (vp9->slice_data_flag) {
   case VA_SLICE_DATA_FLAG_ALL:
      context->desc.vp9.slice_parameter.slice_data_flag[context->desc.vp9.slice_parameter.slice_count] =
            case VA_SLICE_DATA_FLAG_BEGIN:
      context->desc.vp9.slice_parameter.slice_data_flag[context->desc.vp9.slice_parameter.slice_count] =
            case VA_SLICE_DATA_FLAG_MIDDLE:
      context->desc.vp9.slice_parameter.slice_data_flag[context->desc.vp9.slice_parameter.slice_count] =
            case VA_SLICE_DATA_FLAG_END:
      context->desc.vp9.slice_parameter.slice_data_flag[context->desc.vp9.slice_parameter.slice_count] =
            default:
                  /* assert(buf->num_elements == 1) above; */
            for (i = 0; i < 8; ++i) {
      context->desc.vp9.slice_parameter.seg_param[i].segment_flags.segment_reference_enabled =
         context->desc.vp9.slice_parameter.seg_param[i].segment_flags.segment_reference =
         context->desc.vp9.slice_parameter.seg_param[i].segment_flags.segment_reference_skipped =
                     context->desc.vp9.slice_parameter.seg_param[i].luma_ac_quant_scale = vp9->seg_param[i].luma_ac_quant_scale;
   context->desc.vp9.slice_parameter.seg_param[i].luma_dc_quant_scale = vp9->seg_param[i].luma_dc_quant_scale;
   context->desc.vp9.slice_parameter.seg_param[i].chroma_ac_quant_scale = vp9->seg_param[i].chroma_ac_quant_scale;
         }
      static unsigned vp9_u(struct vl_vlc *vlc, unsigned n)
   {
               if (n == 0)
            if (valid < 32)
               }
      static signed vp9_s(struct vl_vlc *vlc, unsigned n)
   {
      unsigned v;
            v = vp9_u(vlc, n);
               }
      static void bitdepth_colorspace_sampling(struct vl_vlc *vlc, unsigned profile)
   {
               if (profile == 2)
      /* bit_depth */
         cs = vp9_u(vlc, 3);
   if (cs != 7)
      /* yuv_range_flag */
   }
      static void frame_size(struct vl_vlc *vlc)
   {
         /* width_minus_one */
   vp9_u(vlc, 16);
   /* height_minus_one */
            /* has_scaling */
   if (vp9_u(vlc, 1)) {
      /* render_width_minus_one */
   vp9_u(vlc, 16);
   /* render_height_minus_one */
      }
      void vlVaDecoderVP9BitstreamHeader(vlVaContext *context, vlVaBuffer *buf)
   {
      struct vl_vlc vlc;
   unsigned profile;
   bool frame_type, show_frame, error_resilient_mode;
   bool mode_ref_delta_enabled, mode_ref_delta_update = false;
            vl_vlc_init(&vlc, 1, (const void * const*)&buf->data,
            /* frame_marker */
   if (vp9_u(&vlc, 2) != 0x2)
                     if (profile == 3)
            if (profile != 0 && profile != 2)
            /* show_existing_frame */
   if (vp9_u(&vlc, 1))
            frame_type = vp9_u(&vlc, 1);
   show_frame = vp9_u(&vlc, 1);
            if (frame_type == 0) {
      /* sync_code */
   if (vp9_u(&vlc, 24) != 0x498342)
            bitdepth_colorspace_sampling(&vlc, profile);
      } else {
               intra_only = show_frame ? 0 : vp9_u(&vlc, 1);
   if (!error_resilient_mode)
                  if (intra_only) {
      /* sync_code */
                  bitdepth_colorspace_sampling(&vlc, profile);
   /* refresh_frame_flags */
   vp9_u(&vlc, 8);
      } else {
                     for (i = 0; i < 3; ++i) {
      /* frame refs */
   vp9_u(&vlc, 3);
               for (i = 0; i < 3; ++i) {
      size_in_refs = vp9_u(&vlc, 1);
   if (size_in_refs)
               if (!size_in_refs) {
      /* width/height_minus_one */
   vp9_u(&vlc, 16);
               if (vp9_u(&vlc, 1)) {
      /* render_width/height_minus_one */
   vp9_u(&vlc, 16);
               /* high_precision_mv */
   vp9_u(&vlc, 1);
   /* filter_switchable */
   if (!vp9_u(&vlc, 1))
      /* filter_index */
      }
   if (!error_resilient_mode) {
      /* refresh_frame_context */
   vp9_u(&vlc, 1);
   /* frame_parallel_decoding_mode */
      }
   /* frame_context_index */
                     /* filter_level */
   vp9_u(&vlc, 6);
   /* sharpness_level */
            mode_ref_delta_enabled = vp9_u(&vlc, 1);
   if (mode_ref_delta_enabled) {
      mode_ref_delta_update = vp9_u(&vlc, 1);
   if (mode_ref_delta_update) {
      for (i = 0; i < 4; ++i) {
      /* update_ref_delta */
   if (vp9_u(&vlc, 1))
      /* ref_deltas */
   }
   for (i = 0; i < 2; ++i) {
      /* update_mode_delta */
   if (vp9_u(&vlc, 1))
      /* mode_deltas */
         }
   context->desc.vp9.picture_parameter.mode_ref_delta_enabled = mode_ref_delta_enabled;
                     context->desc.vp9.picture_parameter.base_qindex = vp9_u(&vlc, 8);
   context->desc.vp9.picture_parameter.y_dc_delta_q = vp9_u(&vlc, 1) ? vp9_s(&vlc, 4) : 0;
   context->desc.vp9.picture_parameter.uv_ac_delta_q = vp9_u(&vlc, 1) ? vp9_s(&vlc, 4) : 0;
                     /* enabled */
   if (!vp9_u(&vlc, 1))
            /* update_map */
   if (vp9_u(&vlc, 1)) {
      for (i = 0; i < 7; ++i) {
      /* tree_probs_set */
   if (vp9_u(&vlc, 1)) {
      /* tree_probs */
                  /* temporal_update */
   if (vp9_u(&vlc, 1)) {
      for (i = 0; i < 3; ++i) {
      /* pred_probs_set */
   if (vp9_u(&vlc, 1))
      /* pred_probs */
                  /* update_data */
   if (vp9_u(&vlc, 1)) {
      /* abs_delta */
   context->desc.vp9.picture_parameter.abs_delta = vp9_u(&vlc, 1);
   for (i = 0; i < 8; ++i) {
      /* Use alternate quantizer */
   if ((context->desc.vp9.slice_parameter.seg_param[i].alt_quant_enabled = vp9_u(&vlc, 1)))
         /* Use alternate loop filter value */
   if ((context->desc.vp9.slice_parameter.seg_param[i].alt_lf_enabled = vp9_u(&vlc, 1)))
         /* Optional Segment reference frame */
   if (vp9_u(&vlc, 1))
         /* Optional Segment skip mode */
            }
