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
      #include "pipe/p_screen.h"
   #include "pipe/p_video_codec.h"
   #include "util/u_memory.h"
   #include "util/u_surface.h"
   #include "vl/vl_video_buffer.h"
   #include "util/vl_vlc.h"
      #include "vl/vl_codec.h"
      #include "entrypoint.h"
   #include "vid_dec.h"
   #include "vid_omx_common.h"
   #include "vid_dec_h264_common.h"
      static OMX_ERRORTYPE vid_dec_Constructor(OMX_COMPONENTTYPE *comp, OMX_STRING name);
   static OMX_ERRORTYPE vid_dec_Destructor(OMX_COMPONENTTYPE *comp);
   static OMX_ERRORTYPE vid_dec_SetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param);
   static OMX_ERRORTYPE vid_dec_GetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param);
   static OMX_ERRORTYPE vid_dec_MessageHandler(OMX_COMPONENTTYPE *comp, internalRequestMessageType *msg);
   static OMX_ERRORTYPE vid_dec_DecodeBuffer(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf);
   static OMX_ERRORTYPE vid_dec_FreeDecBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf);
   static void vid_dec_FrameDecoded(OMX_COMPONENTTYPE *comp, OMX_BUFFERHEADERTYPE* input, OMX_BUFFERHEADERTYPE* output);
      OMX_ERRORTYPE vid_dec_LoaderComponent(stLoaderComponentType *comp)
   {
      comp->componentVersion.s.nVersionMajor = 0;
   comp->componentVersion.s.nVersionMinor = 0;
   comp->componentVersion.s.nRevision = 0;
   comp->componentVersion.s.nStep = 1;
            comp->name = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name == NULL)
            comp->name_specific = CALLOC(comp->name_specific_length, sizeof(char *));
   if (comp->name_specific == NULL)
            comp->role_specific = CALLOC(comp->name_specific_length, sizeof(char *));
   if (comp->role_specific == NULL)
            comp->name_specific[0] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name_specific[0] == NULL)
            comp->name_specific[1] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name_specific[1] == NULL)
            comp->name_specific[2] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name_specific[2] == NULL)
            comp->name_specific[3] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name_specific[3] == NULL)
            comp->role_specific[0] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->role_specific[0] == NULL)
            comp->role_specific[1] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->role_specific[1] == NULL)
            comp->role_specific[2] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->role_specific[2] == NULL)
            comp->role_specific[3] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->role_specific[3] == NULL)
            strcpy(comp->name, OMX_VID_DEC_BASE_NAME);
   strcpy(comp->name_specific[0], OMX_VID_DEC_MPEG2_NAME);
   strcpy(comp->name_specific[1], OMX_VID_DEC_AVC_NAME);
   strcpy(comp->name_specific[2], OMX_VID_DEC_HEVC_NAME);
            strcpy(comp->role_specific[0], OMX_VID_DEC_MPEG2_ROLE);
   strcpy(comp->role_specific[1], OMX_VID_DEC_AVC_ROLE);
   strcpy(comp->role_specific[2], OMX_VID_DEC_HEVC_ROLE);
                           error_specific:
      FREE(comp->role_specific[3]);
   FREE(comp->role_specific[2]);
   FREE(comp->role_specific[1]);
   FREE(comp->role_specific[0]);
   FREE(comp->name_specific[3]);
   FREE(comp->name_specific[2]);
   FREE(comp->name_specific[1]);
         error:
      FREE(comp->role_specific);
                        }
      static OMX_ERRORTYPE vid_dec_Constructor(OMX_COMPONENTTYPE *comp, OMX_STRING name)
   {
      vid_dec_PrivateType *priv;
   omx_base_video_PortType *port;
   struct pipe_screen *screen;
   OMX_ERRORTYPE r;
                     priv = comp->pComponentPrivate = CALLOC(1, sizeof(vid_dec_PrivateType));
   if (!priv)
            r = omx_base_filter_Constructor(comp, name);
   if (r)
                     if (!strcmp(name, OMX_VID_DEC_MPEG2_NAME))
            if (!strcmp(name, OMX_VID_DEC_AVC_NAME))
            if (!strcmp(name, OMX_VID_DEC_HEVC_NAME))
            if (!strcmp(name, OMX_VID_DEC_AV1_NAME))
            if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN)
         else
         priv->messageHandler = vid_dec_MessageHandler;
            comp->SetParameter = vid_dec_SetParameter;
            priv->screen = omx_get_screen();
   if (!priv->screen)
                     if (!vl_codec_supported(screen, priv->profile, false))
            priv->pipe = pipe_create_multimedia_context(screen);
   if (!priv->pipe)
            if (!vl_compositor_init(&priv->compositor, priv->pipe)) {
      priv->pipe->destroy(priv->pipe);
   priv->pipe = NULL;
               if (!vl_compositor_init_state(&priv->cstate, priv->pipe)) {
      vl_compositor_cleanup(&priv->compositor);
   priv->pipe->destroy(priv->pipe);
   priv->pipe = NULL;
               priv->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
   priv->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;
   priv->ports = CALLOC(2, sizeof(omx_base_PortType *));
   if (!priv->ports)
            for (i = 0; i < 2; ++i) {
      priv->ports[i] = CALLOC(1, sizeof(omx_base_video_PortType));
   if (!priv->ports[i])
                        port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   strcpy(port->sPortParam.format.video.cMIMEType,"video/MPEG2");
   port->sPortParam.nBufferCountMin = 8;
   port->sPortParam.nBufferCountActual = 8;
   port->sPortParam.nBufferSize = DEFAULT_OUT_BUFFER_SIZE;
   port->sPortParam.format.video.nFrameWidth = 176;
   port->sPortParam.format.video.nFrameHeight = 144;
   port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
   port->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
   port->Port_SendBufferFunction = vid_dec_DecodeBuffer;
   if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN) {
      port->Port_AllocateBuffer = vid_dec_av1_AllocateInBuffer;
               port->Port_FreeBuffer = vid_dec_FreeDecBuffer;
   port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
   port->sPortParam.nBufferCountActual = 8;
   port->sPortParam.nBufferCountMin = 4;
   port->sPortParam.format.video.nFrameWidth = 176;
   port->sPortParam.format.video.nFrameHeight = 144;
   port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
               }
      static OMX_ERRORTYPE vid_dec_Destructor(OMX_COMPONENTTYPE *comp)
   {
      vid_dec_PrivateType* priv = comp->pComponentPrivate;
            if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN)
            if (priv->ports) {
      for (i = 0; i < priv->sPortTypesParam[OMX_PortDomainVideo].nPorts; ++i) {
      if(priv->ports[i])
      }
   FREE(priv->ports);
               if (priv->pipe) {
      vl_compositor_cleanup_state(&priv->cstate);
   vl_compositor_cleanup(&priv->compositor);
               if (priv->screen)
               }
      static OMX_ERRORTYPE vid_dec_SetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param)
   {
      OMX_COMPONENTTYPE *comp = handle;
   vid_dec_PrivateType *priv = comp->pComponentPrivate;
            if (!param)
            switch(idx) {
   case OMX_IndexParamPortDefinition: {
               r = omx_base_component_SetParameter(handle, idx, param);
   if (r)
            if (def->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
                                    port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
   port->sPortParam.format.video.nFrameWidth = def->format.video.nFrameWidth;
   port->sPortParam.format.video.nFrameHeight = def->format.video.nFrameHeight;
   port->sPortParam.format.video.nStride = def->format.video.nFrameWidth;
                  priv->callbacks->EventHandler(comp, priv->callbackData, OMX_EventPortSettingsChanged,
      }
      }
   case OMX_IndexParamStandardComponentRole: {
               r = checkHeader(param, sizeof(OMX_PARAM_COMPONENTROLETYPE));
   if (r)
            if (!strcmp((char *)role->cRole, OMX_VID_DEC_MPEG2_ROLE)) {
         } else if (!strcmp((char *)role->cRole, OMX_VID_DEC_AVC_ROLE)) {
         } else if (!strcmp((char *)role->cRole, OMX_VID_DEC_HEVC_ROLE)) {
         } else if (!strcmp((char *)role->cRole, OMX_VID_DEC_AV1_ROLE)) {
         } else {
                     }
   case OMX_IndexParamVideoPortFormat: {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *format = param;
            r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   if (r)
            if (format->nPortIndex > 1)
            port = (omx_base_video_PortType *)priv->ports[format->nPortIndex];
   memcpy(&port->sVideoParam, format, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      }
   default:
         }
      }
      static OMX_ERRORTYPE vid_dec_GetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param)
   {
      OMX_COMPONENTTYPE *comp = handle;
   vid_dec_PrivateType *priv = comp->pComponentPrivate;
            if (!param)
            switch(idx) {
   case OMX_IndexParamStandardComponentRole: {
               r = checkHeader(param, sizeof(OMX_PARAM_COMPONENTROLETYPE));
   if (r)
            if (priv->profile == PIPE_VIDEO_PROFILE_MPEG2_MAIN)
         else if (priv->profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH)
         else if (priv->profile == PIPE_VIDEO_PROFILE_HEVC_MAIN)
         else if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN)
                        case OMX_IndexParamVideoInit:
      r = checkHeader(param, sizeof(OMX_PORT_PARAM_TYPE));
   if (r)
            memcpy(param, &priv->sPortTypesParam[OMX_PortDomainVideo], sizeof(OMX_PORT_PARAM_TYPE));
         case OMX_IndexParamVideoPortFormat: {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *format = param;
            r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   if (r)
            if (format->nPortIndex > 1)
            port = (omx_base_video_PortType *)priv->ports[format->nPortIndex];
   memcpy(format, &port->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
               default:
            }
      }
      static OMX_ERRORTYPE vid_dec_MessageHandler(OMX_COMPONENTTYPE* comp, internalRequestMessageType *msg)
   {
               if (msg->messageType == OMX_CommandStateSet) {
      if ((msg->messageParam == OMX_StateIdle ) && (priv->state == OMX_StateLoaded)) {
      if (priv->profile == PIPE_VIDEO_PROFILE_MPEG2_MAIN)
         else if (priv->profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH)
         else if (priv->profile == PIPE_VIDEO_PROFILE_HEVC_MAIN)
         else if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN) {
               } else if ((msg->messageParam == OMX_StateLoaded) && (priv->state == OMX_StateIdle)) {
      if (priv->shadow) {
      priv->shadow->destroy(priv->shadow);
      }
   if (priv->codec) {
      priv->codec->destroy(priv->codec);
                        }
      static OMX_ERRORTYPE vid_dec_DecodeBuffer(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_dec_PrivateType *priv = comp->pComponentPrivate;
   unsigned i = priv->num_in_buffers++;
            priv->in_buffers[i] = buf;
   priv->sizes[i] = buf->nFilledLen;
   priv->inputs[i] = buf->pBuffer;
            while (priv->num_in_buffers > (!!(buf->nFlags & OMX_BUFFERFLAG_EOS) ? 0 : 1)) {
      bool eos = !!(priv->in_buffers[0]->nFlags & OMX_BUFFERFLAG_EOS);
   unsigned min_bits_left = eos ? 32 : MAX2(buf->nFilledLen * 8, 32);
                     if (priv->slice)
            while (vl_vlc_bits_left(&vlc) > min_bits_left) {
      priv->Decode(priv, &vlc, min_bits_left);
               if (priv->slice) {
                              if (priv->num_in_buffers)
         else
               if (eos && priv->frame_started)
            if (priv->frame_finished) {
      priv->frame_finished = false;
   priv->in_buffers[0]->nFilledLen = priv->in_buffers[0]->nAllocLen;
      } else if (eos) {
      if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN)
         else
         priv->in_buffers[0]->nFilledLen = priv->in_buffers[0]->nAllocLen;
      } else {
      priv->in_buffers[0]->nFilledLen = 0;
               if (--priv->num_in_buffers) {
               priv->in_buffers[0] = priv->in_buffers[1];
   priv->sizes[0] = priv->sizes[1] - delta;
   priv->inputs[0] = priv->inputs[1] + delta;
               if (r)
                  }
      static OMX_ERRORTYPE vid_dec_FreeDecBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf)
   {
      OMX_COMPONENTTYPE* comp = port->standCompContainer;
            if (priv->profile == PIPE_VIDEO_PROFILE_AV1_MAIN)
         else
               }
      static void vid_dec_FrameDecoded(OMX_COMPONENTTYPE *comp, OMX_BUFFERHEADERTYPE* input,
         {
                  }
