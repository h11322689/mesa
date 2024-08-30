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
      /*
   * Authors:
   *      Christian König <christian.koenig@amd.com>
   *
   */
         #include <assert.h>
      #include <OMX_Video.h>
      /* bellagio defines a DEBUG macro that we don't want */
   #ifndef DEBUG
   #include <bellagio/omxcore.h>
   #undef DEBUG
   #else
   #include <bellagio/omxcore.h>
   #endif
      #include <bellagio/omx_base_video_port.h>
      #include "pipe/p_screen.h"
   #include "pipe/p_video_codec.h"
   #include "util/u_memory.h"
      #include "vl/vl_codec.h"
      #include "entrypoint.h"
   #include "vid_enc.h"
   #include "vid_omx_common.h"
   #include "vid_enc_common.h"
      static OMX_ERRORTYPE vid_enc_Constructor(OMX_COMPONENTTYPE *comp, OMX_STRING name);
   static OMX_ERRORTYPE vid_enc_Destructor(OMX_COMPONENTTYPE *comp);
   static OMX_ERRORTYPE vid_enc_SetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param);
   static OMX_ERRORTYPE vid_enc_GetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param);
   static OMX_ERRORTYPE vid_enc_SetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config);
   static OMX_ERRORTYPE vid_enc_GetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config);
   static OMX_ERRORTYPE vid_enc_MessageHandler(OMX_COMPONENTTYPE *comp, internalRequestMessageType *msg);
   static OMX_ERRORTYPE vid_enc_AllocateInBuffer(omx_base_PortType *port, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
         static OMX_ERRORTYPE vid_enc_UseInBuffer(omx_base_PortType *port, OMX_BUFFERHEADERTYPE **buf, OMX_U32 idx,
         static OMX_ERRORTYPE vid_enc_FreeInBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf);
   static OMX_ERRORTYPE vid_enc_EncodeFrame(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf);
   static OMX_ERRORTYPE vid_enc_AllocateOutBuffer(omx_base_PortType *comp, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
         static OMX_ERRORTYPE vid_enc_FreeOutBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf);
   static void vid_enc_BufferEncoded(OMX_COMPONENTTYPE *comp, OMX_BUFFERHEADERTYPE* input, OMX_BUFFERHEADERTYPE* output);
      OMX_ERRORTYPE vid_enc_LoaderComponent(stLoaderComponentType *comp)
   {
      comp->componentVersion.s.nVersionMajor = 0;
   comp->componentVersion.s.nVersionMinor = 0;
   comp->componentVersion.s.nRevision = 0;
   comp->componentVersion.s.nStep = 1;
   comp->name_specific_length = 1;
            comp->name = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (!comp->name)
            comp->name_specific = CALLOC(1, sizeof(char *));
   if (!comp->name_specific)
            comp->role_specific = CALLOC(1, sizeof(char *));
   if (!comp->role_specific)
            comp->name_specific[0] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name_specific[0] == NULL)
            comp->role_specific[0] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->role_specific[0] == NULL)
            strcpy(comp->name, OMX_VID_ENC_BASE_NAME);
   strcpy(comp->name_specific[0], OMX_VID_ENC_AVC_NAME);
                  error_specific:
      FREE(comp->role_specific[0]);
         error_arrays:
      FREE(comp->role_specific);
                        }
      static OMX_ERRORTYPE vid_enc_Constructor(OMX_COMPONENTTYPE *comp, OMX_STRING name)
   {
      vid_enc_PrivateType *priv;
   omx_base_video_PortType *port;
   struct pipe_screen *screen;
   OMX_ERRORTYPE r;
                     priv = comp->pComponentPrivate = CALLOC(1, sizeof(vid_enc_PrivateType));
   if (!priv)
            r = omx_base_filter_Constructor(comp, name);
   if (r)
            priv->BufferMgmtCallback = vid_enc_BufferEncoded;
   priv->messageHandler = vid_enc_MessageHandler;
            comp->SetParameter = vid_enc_SetParameter;
   comp->GetParameter = vid_enc_GetParameter;
   comp->GetConfig = vid_enc_GetConfig;
            priv->screen = omx_get_screen();
   if (!priv->screen)
            screen = priv->screen->pscreen;
   if (!vl_codec_supported(screen, PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH, true))
            priv->s_pipe = pipe_create_multimedia_context(screen);
   if (!priv->s_pipe)
                     if (!vl_compositor_init(&priv->compositor, priv->s_pipe)) {
      priv->s_pipe->destroy(priv->s_pipe);
   priv->s_pipe = NULL;
               if (!vl_compositor_init_state(&priv->cstate, priv->s_pipe)) {
      vl_compositor_cleanup(&priv->compositor);
   priv->s_pipe->destroy(priv->s_pipe);
   priv->s_pipe = NULL;
               priv->t_pipe = pipe_create_multimedia_context(screen);
   if (!priv->t_pipe)
            priv->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
   priv->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;
   priv->ports = CALLOC(2, sizeof(omx_base_PortType *));
   if (!priv->ports)
            for (i = 0; i < 2; ++i) {
      priv->ports[i] = CALLOC(1, sizeof(omx_base_video_PortType));
   if (!priv->ports[i])
                        port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   port->sPortParam.format.video.nFrameWidth = 176;
   port->sPortParam.format.video.nFrameHeight = 144;
   port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
   port->sVideoParam.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
   port->sPortParam.nBufferCountActual = 8;
            port->Port_SendBufferFunction = vid_enc_EncodeFrame;
   port->Port_AllocateBuffer = vid_enc_AllocateInBuffer;
   port->Port_UseBuffer = vid_enc_UseInBuffer;
            port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
   strcpy(port->sPortParam.format.video.cMIMEType,"video/H264");
   port->sPortParam.format.video.nFrameWidth = 176;
   port->sPortParam.format.video.nFrameHeight = 144;
   port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
            port->Port_AllocateBuffer = vid_enc_AllocateOutBuffer;
            priv->bitrate.eControlRate = OMX_Video_ControlRateDisable;
            priv->quant.nQpI = OMX_VID_ENC_QUANT_I_FRAMES_DEFAULT;
   priv->quant.nQpP = OMX_VID_ENC_QUANT_P_FRAMES_DEFAULT;
            priv->profile_level.eProfile = OMX_VIDEO_AVCProfileBaseline;
            priv->force_pic_type.IntraRefreshVOP = OMX_FALSE;
   priv->frame_num = 0;
   priv->pic_order_cnt = 0;
            priv->scale.xWidth = OMX_VID_ENC_SCALING_WIDTH_DEFAULT;
            list_inithead(&priv->free_tasks);
   list_inithead(&priv->used_tasks);
   list_inithead(&priv->b_frames);
               }
      static OMX_ERRORTYPE vid_enc_Destructor(OMX_COMPONENTTYPE *comp)
   {
      vid_enc_PrivateType* priv = comp->pComponentPrivate;
            enc_ReleaseTasks(&priv->free_tasks);
   enc_ReleaseTasks(&priv->used_tasks);
   enc_ReleaseTasks(&priv->b_frames);
            if (priv->ports) {
      for (i = 0; i < priv->sPortTypesParam[OMX_PortDomainVideo].nPorts; ++i) {
      if(priv->ports[i])
      }
   FREE(priv->ports);
               for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i)
      if (priv->scale_buffer[i])
         if (priv->s_pipe) {
      vl_compositor_cleanup_state(&priv->cstate);
   vl_compositor_cleanup(&priv->compositor);
   enc_ReleaseCompute_common(priv);
               if (priv->t_pipe)
            if (priv->screen)
               }
      static OMX_ERRORTYPE enc_AllocateBackTexture(omx_base_PortType *port,
                     {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct pipe_resource buf_templ;
   struct pipe_box box = {};
            memset(&buf_templ, 0, sizeof buf_templ);
   buf_templ.target = PIPE_TEXTURE_2D;
   buf_templ.format = PIPE_FORMAT_I8_UNORM;
   buf_templ.bind = PIPE_BIND_LINEAR;
   buf_templ.usage = PIPE_USAGE_STAGING;
   buf_templ.flags = 0;
   buf_templ.width0 = port->sPortParam.format.video.nFrameWidth;
   buf_templ.height0 = port->sPortParam.format.video.nFrameHeight * 3 / 2;
   buf_templ.depth0 = 1;
            *resource = priv->s_pipe->screen->resource_create(priv->s_pipe->screen, &buf_templ);
   if (!*resource)
            box.width = (*resource)->width0;
   box.height = (*resource)->height0;
   box.depth = (*resource)->depth0;
   ptr = priv->s_pipe->texture_map(priv->s_pipe, *resource, 0, PIPE_MAP_WRITE, &box, transfer);
   if (map)
               }
      static OMX_ERRORTYPE vid_enc_SetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param)
   {
      OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
            if (!param)
            switch(idx) {
   case OMX_IndexParamPortDefinition: {
               r = omx_base_component_SetParameter(handle, idx, param);
   if (r)
            if (def->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
      omx_base_video_PortType *port;
   unsigned framesize;
                  port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   enc_AllocateBackTexture(priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX],
         port->sPortParam.format.video.nStride = transfer->stride;
                  framesize = port->sPortParam.format.video.nStride *
                                                priv->callbacks->EventHandler(comp, priv->callbackData, OMX_EventPortSettingsChanged,
      }
      }
   case OMX_IndexParamStandardComponentRole: {
               r = checkHeader(param, sizeof(OMX_PARAM_COMPONENTROLETYPE));
   if (r)
            if (strcmp((char *)role->cRole, OMX_VID_ENC_AVC_ROLE)) {
                     }
   case OMX_IndexParamVideoBitrate: {
               r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
   if (r)
                        }
   case OMX_IndexParamVideoQuantization: {
               r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
   if (r)
                        }
   case OMX_IndexParamVideoProfileLevelCurrent: {
               r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
   if (r)
                        }
   default:
         }
      }
      static OMX_ERRORTYPE vid_enc_GetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param)
   {
      OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
            if (!param)
            switch(idx) {
   case OMX_IndexParamStandardComponentRole: {
               r = checkHeader(param, sizeof(OMX_PARAM_COMPONENTROLETYPE));
   if (r)
            strcpy((char *)role->cRole, OMX_VID_ENC_AVC_ROLE);
      }
   case OMX_IndexParamVideoInit:
      r = checkHeader(param, sizeof(OMX_PORT_PARAM_TYPE));
   if (r)
            memcpy(param, &priv->sPortTypesParam[OMX_PortDomainVideo], sizeof(OMX_PORT_PARAM_TYPE));
         case OMX_IndexParamVideoPortFormat: {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *format = param;
            r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   if (r)
            if (format->nPortIndex > 1)
         if (format->nIndex >= 1)
            port = (omx_base_video_PortType *)priv->ports[format->nPortIndex];
   memcpy(format, &port->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      }
   case OMX_IndexParamVideoBitrate: {
               r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
   if (r)
            bitrate->eControlRate = priv->bitrate.eControlRate;
               }
   case OMX_IndexParamVideoQuantization: {
               r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
   if (r)
            quant->nQpI = priv->quant.nQpI;
   quant->nQpP = priv->quant.nQpP;
               }
   case OMX_IndexParamVideoProfileLevelCurrent: {
               r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
   if (r)
            profile_level->eProfile = priv->profile_level.eProfile;
               }
   default:
         }
      }
      static OMX_ERRORTYPE vid_enc_SetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config)
   {
      OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_ERRORTYPE r;
            if (!config)
            switch(idx) {
   case OMX_IndexConfigVideoIntraVOPRefresh: {
               r = checkHeader(config, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE));
   if (r)
                        }
   case OMX_IndexConfigCommonScale: {
               r = checkHeader(config, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
   if (r)
            if (scale->xWidth < 176 || scale->xHeight < 144)
            for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i) {
      if (priv->scale_buffer[i]) {
      priv->scale_buffer[i]->destroy(priv->scale_buffer[i]);
                  priv->scale = *scale;
   if (priv->scale.xWidth != 0xffffffff && priv->scale.xHeight != 0xffffffff) {
               templat.buffer_format = PIPE_FORMAT_NV12;
   templat.width = priv->scale.xWidth;
   templat.height = priv->scale.xHeight;
   templat.interlaced = false;
   for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i) {
      priv->scale_buffer[i] = priv->s_pipe->create_video_buffer(priv->s_pipe, &templat);
   if (!priv->scale_buffer[i])
                     }
   default:
                     }
      static OMX_ERRORTYPE vid_enc_GetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config)
   {
      OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
            if (!config)
            switch(idx) {
   case OMX_IndexConfigCommonScale: {
               r = checkHeader(config, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
   if (r)
            scale->xWidth = priv->scale.xWidth;
               }
   default:
                     }
      static OMX_ERRORTYPE vid_enc_MessageHandler(OMX_COMPONENTTYPE* comp, internalRequestMessageType *msg)
   {
               if (msg->messageType == OMX_CommandStateSet) {
                                          templat.profile = enc_TranslateOMXProfileToPipe(priv->profile_level.eProfile);
   templat.level = enc_TranslateOMXLevelToPipe(priv->profile_level.eLevel);
   templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_ENCODE;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.width = priv->scale_buffer[priv->current_scale_buffer] ?
                        if (templat.profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE) {
      struct pipe_screen *screen = priv->screen->pscreen;
   templat.max_references = 1;
   priv->stacked_frames_num =
      screen->get_video_param(screen,
               } else {
      templat.max_references = OMX_VID_ENC_P_PERIOD_DEFAULT;
                  } else if ((msg->messageParam == OMX_StateLoaded) && (priv->state == OMX_StateIdle)) {
      if (priv->codec) {
      priv->codec->destroy(priv->codec);
                        }
      static OMX_ERRORTYPE vid_enc_AllocateInBuffer(omx_base_PortType *port, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
         {
      struct input_buf_private *inp;
            r = base_port_AllocateBuffer(port, buf, idx, private, size);
   if (r)
            inp = (*buf)->pInputPortPrivate = CALLOC_STRUCT(input_buf_private);
   if (!inp) {
      base_port_FreeBuffer(port, idx, *buf);
                        FREE((*buf)->pBuffer);
   r = enc_AllocateBackTexture(port, &inp->resource, &inp->transfer, &(*buf)->pBuffer);
   if (r) {
      FREE(inp);
   base_port_FreeBuffer(port, idx, *buf);
                  }
      static OMX_ERRORTYPE vid_enc_UseInBuffer(omx_base_PortType *port, OMX_BUFFERHEADERTYPE **buf, OMX_U32 idx,
         {
      struct input_buf_private *inp;
            r = base_port_UseBuffer(port, buf, idx, private, size, mem);
   if (r)
            inp = (*buf)->pInputPortPrivate = CALLOC_STRUCT(input_buf_private);
   if (!inp) {
      base_port_FreeBuffer(port, idx, *buf);
                           }
      static OMX_ERRORTYPE vid_enc_FreeInBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
            if (inp) {
      enc_ReleaseTasks(&inp->tasks);
   if (inp->transfer)
         pipe_resource_reference(&inp->resource, NULL);
      }
               }
      static OMX_ERRORTYPE vid_enc_AllocateOutBuffer(omx_base_PortType *port, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
         {
               r = base_port_AllocateBuffer(port, buf, idx, private, size);
   if (r)
            FREE((*buf)->pBuffer);
   (*buf)->pBuffer = NULL;
   (*buf)->pOutputPortPrivate = CALLOC(1, sizeof(struct output_buf_private));
   if (!(*buf)->pOutputPortPrivate) {
      base_port_FreeBuffer(port, idx, *buf);
                  }
      static OMX_ERRORTYPE vid_enc_FreeOutBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
            if (buf->pOutputPortPrivate) {
      struct output_buf_private *outp = buf->pOutputPortPrivate;
   if (outp->transfer)
         pipe_resource_reference(&outp->bitstream, NULL);
   FREE(outp);
      }
               }
      static struct encode_task *enc_NeedTask(omx_base_PortType *port)
   {
      OMX_VIDEO_PORTDEFINITIONTYPE *def = &port->sPortParam.format.video;
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
               }
      static OMX_ERRORTYPE enc_LoadImage(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf,
         {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_VIDEO_PORTDEFINITIONTYPE *def = &port->sPortParam.format.video;
      }
      static void enc_ScaleInput(omx_base_PortType *port, struct pipe_video_buffer **vbuf, unsigned *size)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_VIDEO_PORTDEFINITIONTYPE *def = &port->sPortParam.format.video;
      }
      static void enc_ControlPicture(omx_base_PortType *port, struct pipe_h264_enc_picture_desc *picture)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
      }
      static void enc_HandleTask(omx_base_PortType *port, struct encode_task *task,
         {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   unsigned size = priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]->sPortParam.nBufferSize;
   struct pipe_video_buffer *vbuf = task->buf;
            /* -------------- scale input image --------- */
   enc_ScaleInput(port, &vbuf, &size);
            /* -------------- allocate output buffer --------- */
   task->bitstream = pipe_buffer_create(priv->s_pipe->screen,
                        picture.picture_type = picture_type;
   picture.pic_order_cnt = task->pic_order_cnt;
   picture.base.profile = enc_TranslateOMXProfileToPipe(priv->profile_level.eProfile);
   picture.base.entry_point = PIPE_VIDEO_ENTRYPOINT_ENCODE;
   if (priv->restricted_b_frames && picture_type == PIPE_H2645_ENC_PICTURE_TYPE_B)
                  /* -------------- encode frame --------- */
   priv->codec->begin_frame(priv->codec, vbuf, &picture.base);
   priv->codec->encode_bitstream(priv->codec, vbuf, task->bitstream, &task->feedback);
      }
      static void enc_ClearBframes(omx_base_PortType *port, struct input_buf_private *inp)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
            if (list_is_empty(&priv->b_frames))
            task = list_entry(priv->b_frames.prev, struct encode_task, list);
            /* promote last from to P frame */
   priv->ref_idx_l0 = priv->ref_idx_l1;
   enc_HandleTask(port, task, PIPE_H2645_ENC_PICTURE_TYPE_P);
   list_addtail(&task->list, &inp->tasks);
            /* handle B frames */
   LIST_FOR_EACH_ENTRY(task, &priv->b_frames, list) {
      enc_HandleTask(port, task, PIPE_H2645_ENC_PICTURE_TYPE_B);
   if (!priv->restricted_b_frames)
                        }
      static OMX_ERRORTYPE vid_enc_EncodeFrame(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct input_buf_private *inp = buf->pInputPortPrivate;
   enum pipe_h2645_enc_picture_type picture_type;
   struct encode_task *task;
   unsigned stacked_num = 0;
            enc_MoveTasks(&inp->tasks, &priv->free_tasks);
   task = enc_NeedTask(port);
   if (!task)
            if (buf->nFilledLen == 0) {
      if (buf->nFlags & OMX_BUFFERFLAG_EOS) {
      buf->nFilledLen = buf->nAllocLen;
   enc_ClearBframes(port, inp);
   enc_MoveTasks(&priv->stacked_tasks, &inp->tasks);
      }
               if (buf->pOutputPortPrivate) {
      struct pipe_video_buffer *vbuf = buf->pOutputPortPrivate;
   buf->pOutputPortPrivate = task->buf;
      } else {
      /* ------- load input image into video buffer ---- */
   err = enc_LoadImage(port, buf, task->buf);
   if (err != OMX_ErrorNone) {
      FREE(task);
                  /* -------------- determine picture type --------- */
   if (!(priv->pic_order_cnt % OMX_VID_ENC_IDR_PERIOD_DEFAULT) ||
      priv->force_pic_type.IntraRefreshVOP) {
   enc_ClearBframes(port, inp);
   picture_type = PIPE_H2645_ENC_PICTURE_TYPE_IDR;
   priv->force_pic_type.IntraRefreshVOP = OMX_FALSE;
   priv->frame_num = 0;
      } else if (priv->codec->profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE ||
            !(priv->pic_order_cnt % OMX_VID_ENC_P_PERIOD_DEFAULT) ||
      } else {
                           if (picture_type == PIPE_H2645_ENC_PICTURE_TYPE_B) {
      /* put frame at the tail of the queue */
      } else {
      /* handle I or P frame */
   priv->ref_idx_l0 = priv->ref_idx_l1;
   enc_HandleTask(port, task, picture_type);
   list_addtail(&task->list, &priv->stacked_tasks);
   LIST_FOR_EACH_ENTRY(task, &priv->stacked_tasks, list) {
         }
   if (stacked_num == priv->stacked_frames_num) {
      struct encode_task *t;
   t = list_entry(priv->stacked_tasks.next, struct encode_task, list);
   list_del(&t->list);
      }
            /* handle B frames */
   LIST_FOR_EACH_ENTRY(task, &priv->b_frames, list) {
      enc_HandleTask(port, task, PIPE_H2645_ENC_PICTURE_TYPE_B);
   if (!priv->restricted_b_frames)
                                 if (list_is_empty(&inp->tasks))
         else
      }
      static void vid_enc_BufferEncoded(OMX_COMPONENTTYPE *comp, OMX_BUFFERHEADERTYPE* input, OMX_BUFFERHEADERTYPE* output)
   {
      vid_enc_PrivateType *priv = comp->pComponentPrivate;
      }
