   /**************************************************************************
   *
   * Copyright 2013 Advanced Micro Devices, Inc.
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
      #include "vid_enc_common.h"
      #include "vl/vl_video_buffer.h"
   #include "tgsi/tgsi_text.h"
      void enc_ReleaseTasks(struct list_head *head)
   {
               if (!head || !list_is_linked(head))
            LIST_FOR_EACH_ENTRY_SAFE(i, next, head, list) {
      pipe_resource_reference(&i->bitstream, NULL);
   i->buf->destroy(i->buf);
         }
      void enc_MoveTasks(struct list_head *from, struct list_head *to)
   {
      to->prev->next = from->next;
   from->next->prev = to->prev;
   from->prev->next = to;
   to->prev = from->prev;
      }
      static void enc_GetPictureParamPreset(struct pipe_h264_enc_picture_desc *picture)
   {
      picture->motion_est.enc_disable_sub_mode = 0x000000fe;
   picture->motion_est.enc_ime2_search_range_x = 0x00000001;
   picture->motion_est.enc_ime2_search_range_y = 0x00000001;
      }
      enum pipe_video_profile enc_TranslateOMXProfileToPipe(unsigned omx_profile)
   {
      switch (omx_profile) {
   case OMX_VIDEO_AVCProfileBaseline:
         case OMX_VIDEO_AVCProfileMain:
         case OMX_VIDEO_AVCProfileExtended:
         case OMX_VIDEO_AVCProfileHigh:
         case OMX_VIDEO_AVCProfileHigh10:
         case OMX_VIDEO_AVCProfileHigh422:
         case OMX_VIDEO_AVCProfileHigh444:
         default:
            }
      unsigned enc_TranslateOMXLevelToPipe(unsigned omx_level)
   {
      switch (omx_level) {
   case OMX_VIDEO_AVCLevel1:
   case OMX_VIDEO_AVCLevel1b:
         case OMX_VIDEO_AVCLevel11:
         case OMX_VIDEO_AVCLevel12:
         case OMX_VIDEO_AVCLevel13:
         case OMX_VIDEO_AVCLevel2:
         case OMX_VIDEO_AVCLevel21:
         case OMX_VIDEO_AVCLevel22:
         case OMX_VIDEO_AVCLevel3:
         case OMX_VIDEO_AVCLevel31:
         case OMX_VIDEO_AVCLevel32:
         case OMX_VIDEO_AVCLevel4:
         case OMX_VIDEO_AVCLevel41:
         default:
   case OMX_VIDEO_AVCLevel42:
         case OMX_VIDEO_AVCLevel5:
         case OMX_VIDEO_AVCLevel51:
            }
      void vid_enc_BufferEncoded_common(vid_enc_PrivateType * priv, OMX_BUFFERHEADERTYPE* input, OMX_BUFFERHEADERTYPE* output)
   {
      struct output_buf_private *outp = output->pOutputPortPrivate;
   struct input_buf_private *inp = input->pInputPortPrivate;
   struct encode_task *task;
   struct pipe_box box = {};
         #if ENABLE_ST_OMX_BELLAGIO
      if (!inp || list_is_empty(&inp->tasks)) {
      input->nFilledLen = 0; /* mark buffer as empty */
   enc_MoveTasks(&priv->used_tasks, &inp->tasks);
         #endif
         task = list_entry(inp->tasks.next, struct encode_task, list);
   list_del(&task->list);
            if (!task->bitstream)
                     if (outp->transfer)
            pipe_resource_reference(&outp->bitstream, task->bitstream);
            box.width = outp->bitstream->width0;
   box.height = outp->bitstream->height0;
            output->pBuffer = priv->t_pipe->buffer_map(priv->t_pipe, outp->bitstream, 0,
                                    output->nOffset = 0;
            /* all output buffers contain exactly one frame */
         #if ENABLE_ST_OMX_TIZONIA
      input->nFilledLen = 0; /* mark buffer as empty */
      #endif
   }
         struct encode_task *enc_NeedTask_common(vid_enc_PrivateType * priv, OMX_VIDEO_PORTDEFINITIONTYPE *def)
   {
      struct pipe_video_buffer templat = {};
            if (!list_is_empty(&priv->free_tasks)) {
      task = list_entry(priv->free_tasks.next, struct encode_task, list);
   list_del(&task->list);
               /* allocate a new one */
   task = CALLOC_STRUCT(encode_task);
   if (!task)
            templat.buffer_format = PIPE_FORMAT_NV12;
   templat.width = def->nFrameWidth;
   templat.height = def->nFrameHeight;
            task->buf = priv->s_pipe->create_video_buffer(priv->s_pipe, &templat);
   if (!task->buf) {
      FREE(task);
                  }
      void enc_ScaleInput_common(vid_enc_PrivateType * priv, OMX_VIDEO_PORTDEFINITIONTYPE *def,
         {
      struct pipe_video_buffer *src_buf = *vbuf;
   struct vl_compositor *compositor = &priv->compositor;
   struct vl_compositor_state *s = &priv->cstate;
   struct pipe_sampler_view **views;
   struct pipe_surface **dst_surface;
            if (!priv->scale_buffer[priv->current_scale_buffer])
            views = src_buf->get_sampler_view_planes(src_buf);
   dst_surface = priv->scale_buffer[priv->current_scale_buffer]->get_surfaces
                  for (i = 0; i < VL_MAX_SURFACES; ++i) {
      struct u_rect src_rect;
   if (!views[i] || !dst_surface[i])
         src_rect.x0 = 0;
   src_rect.y0 = 0;
   src_rect.x1 = def->nFrameWidth;
   src_rect.y1 = def->nFrameHeight;
   if (i > 0) {
      src_rect.x1 /= 2;
      }
   vl_compositor_set_rgba_layer(s, compositor, 0, views[i], &src_rect, NULL, NULL);
      }
   *size  = priv->scale.xWidth * priv->scale.xHeight * 2;
   *vbuf = priv->scale_buffer[priv->current_scale_buffer++];
      }
      void enc_ControlPicture_common(vid_enc_PrivateType * priv, struct pipe_h264_enc_picture_desc *picture)
   {
               /* Get bitrate from port */
   switch (priv->bitrate.eControlRate) {
   case OMX_Video_ControlRateVariable:
      rate_ctrl->rate_ctrl_method = PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE;
      case OMX_Video_ControlRateConstant:
      rate_ctrl->rate_ctrl_method = PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT;
      case OMX_Video_ControlRateVariableSkipFrames:
      rate_ctrl->rate_ctrl_method = PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP;
      case OMX_Video_ControlRateConstantSkipFrames:
      rate_ctrl->rate_ctrl_method = PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP;
      default:
      rate_ctrl->rate_ctrl_method = PIPE_H2645_ENC_RATE_CONTROL_METHOD_DISABLE;
               rate_ctrl->frame_rate_den = OMX_VID_ENC_CONTROL_FRAME_RATE_DEN_DEFAULT;
            if (rate_ctrl->rate_ctrl_method != PIPE_H2645_ENC_RATE_CONTROL_METHOD_DISABLE) {
      if (priv->bitrate.nTargetBitrate < OMX_VID_ENC_BITRATE_MIN)
         else if (priv->bitrate.nTargetBitrate < OMX_VID_ENC_BITRATE_MAX)
         else
         rate_ctrl->peak_bitrate = rate_ctrl->target_bitrate;
   if (rate_ctrl->target_bitrate < OMX_VID_ENC_BITRATE_MEDIAN)
         else
            if (rate_ctrl->frame_rate_num) {
      unsigned long long t = rate_ctrl->target_bitrate;
   t *= rate_ctrl->frame_rate_den;
      } else {
         }
   rate_ctrl->peak_bits_picture_integer = rate_ctrl->target_bits_picture;
               picture->quant_i_frames = priv->quant.nQpI;
   picture->quant_p_frames = priv->quant.nQpP;
            picture->frame_num = priv->frame_num;
   picture->num_ref_idx_l0_active_minus1 = 0;
   picture->ref_idx_l0_list[0] = priv->ref_idx_l0;
   picture->num_ref_idx_l1_active_minus1 = 0;
   picture->ref_idx_l1_list[0] = priv->ref_idx_l1;
   picture->enable_vui = (picture->rate_ctrl[0].frame_rate_num != 0);
      }
      static void *create_compute_state(struct pipe_context *pipe,
         {
      struct tgsi_token tokens[1024];
            if (!tgsi_text_translate(source, tokens, ARRAY_SIZE(tokens))) {
         assert(false);
            state.ir_type = PIPE_SHADER_IR_TGSI;
               }
      void enc_InitCompute_common(vid_enc_PrivateType *priv)
   {
      struct pipe_context *pipe = priv->s_pipe;
            /* We need the partial last block support. */
   if (!screen->get_param(screen, PIPE_CAP_COMPUTE_GRID_INFO_LAST_BLOCK))
            static const char *copy_y =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 64\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 1\n"
   "PROPERTY CS_FIXED_BLOCK_DEPTH 1\n"
   "DCL SV[0], THREAD_ID\n"
   "DCL SV[1], BLOCK_ID\n"
   "DCL IMAGE[0], 2D, PIPE_FORMAT_R8_UINT\n"
   "DCL IMAGE[1], 2D, PIPE_FORMAT_R8_UINT, WR\n"
                  "UMAD TEMP[0].x, SV[1], IMM[0], SV[0]\n"
   "MOV TEMP[0].y, SV[1]\n"
   "LOAD TEMP[1].x, IMAGE[0], TEMP[0], 2D, PIPE_FORMAT_R8_UINT\n"
            static const char *copy_uv =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 64\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 1\n"
   "PROPERTY CS_FIXED_BLOCK_DEPTH 1\n"
   "DCL SV[0], THREAD_ID\n"
   "DCL SV[1], BLOCK_ID\n"
   "DCL IMAGE[0], 2D, PIPE_FORMAT_R8_UINT\n"
   "DCL IMAGE[2], 2D, PIPE_FORMAT_R8G8_UINT, WR\n"
   "DCL CONST[0][0]\n" /* .x = offset of the UV portion in the y direction */
   "DCL TEMP[0..4]\n"
   "IMM[0] UINT32 {64, 0, 2, 1}\n"
   /* Destination R8G8 coordinates */
   "UMAD TEMP[0].x, SV[1], IMM[0], SV[0]\n"
   "MOV TEMP[0].y, SV[1]\n"
   /* Source R8 coordinates of U */
   "UMUL TEMP[1].x, TEMP[0], IMM[0].zzzz\n"
   "UADD TEMP[1].y, TEMP[0], CONST[0].xxxx\n"
   /* Source R8 coordinates of V */
                  "LOAD TEMP[3].x, IMAGE[0], TEMP[1], 2D, PIPE_FORMAT_R8_UINT\n"
   "LOAD TEMP[4].x, IMAGE[0], TEMP[2], 2D, PIPE_FORMAT_R8_UINT\n"
   "MOV TEMP[3].y, TEMP[4].xxxx\n"
            priv->copy_y_shader = create_compute_state(pipe, copy_y);
      }
      void enc_ReleaseCompute_common(vid_enc_PrivateType *priv)
   {
               if (priv->copy_y_shader)
         if (priv->copy_uv_shader)
      }
      OMX_ERRORTYPE enc_LoadImage_common(vid_enc_PrivateType * priv, OMX_VIDEO_PORTDEFINITIONTYPE *def,
               {
      struct pipe_context *pipe = priv->s_pipe;
   struct pipe_box box = {};
            if (!inp->resource) {
      struct pipe_sampler_view **views;
            views = vbuf->get_sampler_view_planes(vbuf);
   if (!views)
            ptr = buf->pBuffer;
   box.width = def->nFrameWidth;
   box.height = def->nFrameHeight;
   box.depth = 1;
   pipe->texture_subdata(pipe, views[0]->texture, 0,
               ptr = ((uint8_t*)buf->pBuffer) + (def->nStride * box.height);
   box.width = def->nFrameWidth / 2;
   box.height = def->nFrameHeight / 2;
   box.depth = 1;
   pipe->texture_subdata(pipe, views[1]->texture, 0,
            } else {
                        /* inp->resource uses PIPE_FORMAT_I8 and the layout looks like this:
   *
   * def->nFrameWidth = 4, def->nFrameHeight = 4:
   * |----|
   * |YYYY|
   * |YYYY|
   * |YYYY|
   * |YYYY|
   * |UVUV|
   * |UVUV|
   * |----|
   *
   * The copy has 2 steps:
   * - Copy Y to dst_buf->resources[0] as R8.
   * - Copy UV to dst_buf->resources[1] as R8G8.
   */
   if (priv->copy_y_shader && priv->copy_uv_shader) {
      /* Compute path */
   /* Set shader images for both copies. */
   struct pipe_image_view image[3] = {0};
   image[0].resource = inp->resource;
                  image[1].resource = dst_buf->resources[0];
                  image[2].resource = dst_buf->resources[1];
                           /* Set the constant buffer. */
                  cb.buffer_size = sizeof(constants);
                  /* Use the optimal block size for the linear image layout. */
   struct pipe_grid_info info = {};
   info.block[0] = 64;
   info.block[1] = 1;
                                 info.grid[0] = DIV_ROUND_UP(def->nFrameWidth, 64);
   info.grid[1] = def->nFrameHeight;
                                 info.grid[0] = DIV_ROUND_UP(def->nFrameWidth / 2, 64);
   info.grid[1] = def->nFrameHeight / 2;
                                 /* Unbind. */
   pipe->set_shader_images(pipe, PIPE_SHADER_COMPUTE, 0, 0, 3, NULL);
   pipe->set_constant_buffer(pipe, PIPE_SHADER_COMPUTE, 0, false, NULL);
      } else {
                     box.width = def->nFrameWidth;
                  /* Copy Y */
   pipe->resource_copy_region(pipe,
                  /* Copy U */
   memset(&blit, 0, sizeof(blit));
                  blit.src.box.x = -1;
   blit.src.box.y = def->nFrameHeight;
   blit.src.box.width = def->nFrameWidth;
                                 blit.dst.box.width = def->nFrameWidth / 2;
   blit.dst.box.height = def->nFrameHeight / 2;
                                 /* Copy V */
   blit.src.box.x = 0;
   blit.mask = PIPE_MASK_G;
                        box.width = inp->resource->width0;
   box.height = inp->resource->height0;
   box.depth = inp->resource->depth0;
   buf->pBuffer = pipe->texture_map(pipe, inp->resource, 0,
                        }
