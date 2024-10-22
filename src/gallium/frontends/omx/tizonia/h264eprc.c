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
      #include <tizplatform.h>
   #include <tizkernel.h>
   #include <tizutils.h>
      #include "pipe/p_screen.h"
   #include "pipe/p_video_codec.h"
   #include "util/u_memory.h"
      #include "vl/vl_codec.h"
      #include "entrypoint.h"
   #include "h264e.h"
   #include "h264eprc.h"
   #include "vid_omx_common.h"
   #include "vid_enc_common.h"
      static OMX_ERRORTYPE init_port_structs(vid_enc_PrivateType * priv) {
                        /* Initialisation */
   TIZ_INIT_OMX_PORT_STRUCT(priv->in_port_def_,
         TIZ_INIT_OMX_PORT_STRUCT(priv->out_port_def_,
         TIZ_INIT_OMX_PORT_STRUCT(priv->bitrate,
         TIZ_INIT_OMX_PORT_STRUCT(priv->quant,
         TIZ_INIT_OMX_PORT_STRUCT(priv->profile_level,
            /* Get values */
            tiz_check_omx(
      tiz_api_GetParameter(p_krn, handleOf(priv),
      tiz_check_omx(
      tiz_api_GetParameter(p_krn, handleOf(priv),
      tiz_check_omx(
      tiz_api_GetParameter(p_krn, handleOf(priv),
      tiz_check_omx(
      tiz_api_GetParameter(p_krn, handleOf(priv),
      tiz_check_omx(
      tiz_api_GetParameter(p_krn, handleOf(priv),
            }
      static OMX_BUFFERHEADERTYPE * get_input_buffer(vid_enc_PrivateType * priv) {
               if (priv->in_port_disabled_) {
                           tiz_krn_claim_buffer(tiz_get_krn(handleOf(priv)),
                  }
      static OMX_BUFFERHEADERTYPE * get_output_buffer(vid_enc_PrivateType * priv) {
               if (priv->out_port_disabled_) {
                  if (!priv->p_outhdr_) {
      tiz_krn_claim_buffer(tiz_get_krn(handleOf(priv)),
            }
      }
      static OMX_ERRORTYPE h264e_buffer_emptied(vid_enc_PrivateType * priv, OMX_BUFFERHEADERTYPE * p_hdr)
   {
               assert(priv);
            if (!priv->out_port_disabled_) {
               if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0) {
                  r = tiz_krn_release_buffer(tiz_get_krn(handleOf(priv)), 0, p_hdr);
                  }
      static OMX_ERRORTYPE h264e_buffer_filled(vid_enc_PrivateType * priv, OMX_BUFFERHEADERTYPE * p_hdr)
   {
               assert(priv);
   assert(priv->p_outhdr_ == p_hdr);
            if (!priv->in_port_disabled_) {
               if (priv->eos_) {
      /* EOS has been received and all the input data has been consumed
   * already, so its time to propagate the EOS flag */
               r = tiz_krn_release_buffer(tiz_get_krn(handleOf(priv)),
                              }
         static void release_input_header(vid_enc_PrivateType * priv) {
      assert(!priv->in_port_disabled_);
   if (priv->p_inhdr_) {
      (void) tiz_krn_release_buffer(tiz_get_krn(handleOf(priv)),
            }
      }
      static void release_output_header(vid_enc_PrivateType * priv) {
      if (priv->p_outhdr_) {
      assert(!priv->out_port_disabled_);
   (void) tiz_krn_release_buffer(tiz_get_krn(handleOf(priv)),
                     }
      static OMX_ERRORTYPE h264e_release_all_headers(vid_enc_PrivateType * priv)
   {
               release_input_header(priv);
               }
      static void reset_stream_parameters(vid_enc_PrivateType * priv)
   {
      assert(priv);
   init_port_structs(priv);
   priv->p_inhdr_ = 0;
   priv->p_outhdr_ = 0;
      }
      /* Replacement for bellagio's omx_base_filter_BufferMgmtFunction */
   static OMX_ERRORTYPE h264e_manage_buffers(vid_enc_PrivateType* priv) {
      OMX_BUFFERHEADERTYPE * in_buf = priv->p_inhdr_;
   OMX_BUFFERHEADERTYPE * out_buf = priv->p_outhdr_;
            if (in_buf->nFilledLen > 0) {
         } else {
                           /* Release input buffer if possible */
   if (in_buf->nFilledLen == 0) {
                  /* Realase output buffer if filled or eos */
   if ((out_buf->nFilledLen != 0) || priv->eos_) {
                     }
      static struct encode_task *enc_NeedTask(vid_enc_PrivateType * priv)
   {
                  }
      static void enc_ScaleInput(vid_enc_PrivateType * priv, struct pipe_video_buffer **vbuf, unsigned *size)
   {
      OMX_VIDEO_PORTDEFINITIONTYPE *def = &priv->in_port_def_.format.video;
      }
      static void enc_HandleTask(vid_enc_PrivateType * priv, struct encode_task *task,
         {
      unsigned size = priv->out_port_def_.nBufferSize;
   struct pipe_video_buffer *vbuf = task->buf;
            /* -------------- scale input image --------- */
   enc_ScaleInput(priv, &vbuf, &size);
            /* -------------- allocate output buffer --------- */
   task->bitstream = pipe_buffer_create(priv->s_pipe->screen,
                        picture.picture_type = picture_type;
   picture.pic_order_cnt = task->pic_order_cnt;
   if (priv->restricted_b_frames && picture_type == PIPE_H2645_ENC_PICTURE_TYPE_B)
                  /* -------------- encode frame --------- */
   priv->codec->begin_frame(priv->codec, vbuf, &picture.base);
   priv->codec->encode_bitstream(priv->codec, vbuf, task->bitstream, &task->feedback);
      }
      static void enc_ClearBframes(vid_enc_PrivateType * priv, struct input_buf_private *inp)
   {
               if (list_is_empty(&priv->b_frames))
            task = list_entry(priv->b_frames.prev, struct encode_task, list);
            /* promote last from to P frame */
   priv->ref_idx_l0 = priv->ref_idx_l1;
   enc_HandleTask(priv, task, PIPE_H2645_ENC_PICTURE_TYPE_P);
   list_addtail(&task->list, &inp->tasks);
            /* handle B frames */
   LIST_FOR_EACH_ENTRY(task, &priv->b_frames, list) {
      enc_HandleTask(priv, task, PIPE_H2645_ENC_PICTURE_TYPE_B);
   if (!priv->restricted_b_frames)
                        }
      static OMX_ERRORTYPE enc_LoadImage(vid_enc_PrivateType * priv, OMX_BUFFERHEADERTYPE *buf,
         {
      OMX_VIDEO_PORTDEFINITIONTYPE *def = &priv->in_port_def_.format.video;
      }
      static OMX_ERRORTYPE encode_frame(vid_enc_PrivateType * priv, OMX_BUFFERHEADERTYPE * in_buf)
   {
      struct input_buf_private *inp = in_buf->pInputPortPrivate;
   enum pipe_h2645_enc_picture_type picture_type;
   struct encode_task *task;
   unsigned stacked_num = 0;
            enc_MoveTasks(&inp->tasks, &priv->free_tasks);
   task = enc_NeedTask(priv);
   if (!task)
            /* EOS */
   if (in_buf->nFilledLen == 0) {
      if (in_buf->nFlags & OMX_BUFFERFLAG_EOS) {
      in_buf->nFilledLen = in_buf->nAllocLen;
   enc_ClearBframes(priv, inp);
   enc_MoveTasks(&priv->stacked_tasks, &inp->tasks);
      }
               if (in_buf->pOutputPortPrivate) {
      struct pipe_video_buffer *vbuf = in_buf->pOutputPortPrivate;
   in_buf->pOutputPortPrivate = task->buf;
      } else {
      /* ------- load input image into video buffer ---- */
   err = enc_LoadImage(priv, in_buf, task->buf);
   if (err != OMX_ErrorNone) {
      FREE(task);
                  /* -------------- determine picture type --------- */
   if (!(priv->pic_order_cnt % OMX_VID_ENC_IDR_PERIOD_DEFAULT) ||
      priv->force_pic_type.IntraRefreshVOP) {
   enc_ClearBframes(priv, inp);
   picture_type = PIPE_H2645_ENC_PICTURE_TYPE_IDR;
   priv->force_pic_type.IntraRefreshVOP = OMX_FALSE;
      } else if (priv->codec->profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE ||
            !(priv->pic_order_cnt % OMX_VID_ENC_P_PERIOD_DEFAULT) ||
      } else {
                           if (picture_type == PIPE_H2645_ENC_PICTURE_TYPE_B) {
      /* put frame at the tail of the queue */
      } else {
      /* handle I or P frame */
   priv->ref_idx_l0 = priv->ref_idx_l1;
   enc_HandleTask(priv, task, picture_type);
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
      enc_HandleTask(priv, task, PIPE_H2645_ENC_PICTURE_TYPE_B);
   if (!priv->restricted_b_frames)
                                 if (list_is_empty(&inp->tasks)) {
         } else {
            }
      static OMX_ERRORTYPE h264e_prc_create_encoder(void *ap_obj)
   {
      vid_enc_PrivateType *priv = ap_obj;
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
            list_inithead(&priv->free_tasks);
   list_inithead(&priv->used_tasks);
   list_inithead(&priv->b_frames);
               }
      static void h264e_prc_destroy_encoder(void *ap_obj)
   {
      vid_enc_PrivateType *priv = ap_obj;
                     enc_ReleaseTasks(&priv->free_tasks);
   enc_ReleaseTasks(&priv->used_tasks);
   enc_ReleaseTasks(&priv->b_frames);
            for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i)
      if (priv->scale_buffer[i])
         if (priv->s_pipe) {
      vl_compositor_cleanup_state(&priv->cstate);
   vl_compositor_cleanup(&priv->compositor);
   enc_ReleaseCompute_common(priv);
               if (priv->t_pipe)
            if (priv->screen)
      }
      /*
   * h264eprc
   */
      static void * h264e_prc_ctor(void *ap_obj, va_list * app)
   {
      vid_enc_PrivateType *priv = super_ctor(typeOf(ap_obj, "h264eprc"), ap_obj, app);
            if (h264e_prc_create_encoder(ap_obj) != OMX_ErrorNone)
            priv->p_inhdr_ = 0;
   priv->p_outhdr_ = 0;
   priv->profile_level.eProfile = OMX_VIDEO_AVCProfileBaseline;
   priv->profile_level.eLevel = OMX_VIDEO_AVCLevel51;
   priv->force_pic_type.IntraRefreshVOP = OMX_FALSE;
   priv->frame_num = 0;
   priv->pic_order_cnt = 0;
   priv->restricted_b_frames = debug_get_bool_option("OMX_USE_RESTRICTED_B_FRAMES", false);
   priv->scale.xWidth = OMX_VID_ENC_SCALING_WIDTH_DEFAULT;
   priv->scale.xHeight = OMX_VID_ENC_SCALING_WIDTH_DEFAULT;
   priv->in_port_disabled_   = false;
   priv->out_port_disabled_   = false;
               }
      static void * h264e_prc_dtor(void *ap_obj)
   {
      h264e_prc_destroy_encoder(ap_obj);
      }
      static OMX_ERRORTYPE h264e_prc_allocate_resources(void *ap_obj, OMX_U32 a_pid)
   {
               assert(priv);
   if (!priv->screen)
               }
      static OMX_ERRORTYPE h264e_prc_deallocate_resources(void *ap_obj)
   {
         }
      static OMX_ERRORTYPE h264e_prc_prepare_to_transfer(void *ap_obj, OMX_U32 a_pid)
   {
                                                   templat.profile = enc_TranslateOMXProfileToPipe(priv->profile_level.eProfile);
   templat.level = enc_TranslateOMXLevelToPipe(priv->profile_level.eLevel);
   templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_ENCODE;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.width = priv->scale_buffer[priv->current_scale_buffer] ?
         templat.height = priv->scale_buffer[priv->current_scale_buffer] ?
            if (templat.profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE) {
      struct pipe_screen *screen = priv->screen->pscreen;
   templat.max_references = 1;
   priv->stacked_frames_num =
      screen->get_video_param(screen,
               } else {
      templat.max_references = OMX_VID_ENC_P_PERIOD_DEFAULT;
      }
               }
      static OMX_ERRORTYPE h264e_prc_transfer_and_process(void *ap_obj, OMX_U32 a_pid)
   {
         }
      static OMX_ERRORTYPE h264e_prc_stop_and_return(void *ap_obj)
   {
      vid_enc_PrivateType *priv = (vid_enc_PrivateType *) ap_obj;
      }
      static OMX_ERRORTYPE h264e_prc_buffers_ready(const void *ap_obj)
   {
      vid_enc_PrivateType *priv = (vid_enc_PrivateType *) ap_obj;
   OMX_BUFFERHEADERTYPE *in_buf = NULL;
   OMX_BUFFERHEADERTYPE *out_buf = NULL;
                     /* Don't get input buffer if output buffer not found */
   while (!priv->eos_ && (out_buf = get_output_buffer(priv)) && (in_buf = get_input_buffer(priv))) {
      if (!priv->out_port_disabled_) {
                        }
      static OMX_ERRORTYPE h264e_prc_port_flush(const void *ap_obj, OMX_U32 a_pid)
   {
      vid_enc_PrivateType *priv = (vid_enc_PrivateType *) ap_obj;
   if (OMX_ALL == a_pid || OMX_VID_ENC_AVC_INPUT_PORT_INDEX == a_pid) {
      release_input_header(priv);
      }
   if (OMX_ALL == a_pid || OMX_VID_ENC_AVC_OUTPUT_PORT_INDEX == a_pid) {
         }
      }
      static OMX_ERRORTYPE h264e_prc_port_disable(const void *ap_obj, OMX_U32 a_pid)
   {
      vid_enc_PrivateType *priv = (vid_enc_PrivateType *) ap_obj;
   assert(priv);
   if (OMX_ALL == a_pid || OMX_VID_ENC_AVC_INPUT_PORT_INDEX == a_pid) {
      /* Release all buffers */
   h264e_release_all_headers(priv);
   reset_stream_parameters(priv);
      }
   if (OMX_ALL == a_pid || OMX_VID_ENC_AVC_OUTPUT_PORT_INDEX == a_pid) {
      release_output_header(priv);
      }
      }
      static OMX_ERRORTYPE h264e_prc_port_enable(const void *ap_obj, OMX_U32 a_pid)
   {
      vid_enc_PrivateType * priv = (vid_enc_PrivateType *) ap_obj;
   assert(priv);
   if (OMX_ALL == a_pid || OMX_VID_ENC_AVC_INPUT_PORT_INDEX == a_pid) {
      if (priv->in_port_disabled_) {
      reset_stream_parameters(priv);
         }
   if (OMX_ALL == a_pid || OMX_VID_ENC_AVC_OUTPUT_PORT_INDEX == a_pid) {
         }
      }
      /*
   * h264e_prc_class
   */
      static void * h264e_prc_class_ctor(void *ap_obj, va_list * app)
   {
      /* NOTE: Class methods might be added in the future. None for now. */
      }
      /*
   * initialization
   */
      void * h264e_prc_class_init(void * ap_tos, void * ap_hdl)
   {
      void * tizprc = tiz_get_type(ap_hdl, "tizprc");
   void * h264eprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
   (classOf(tizprc), "h264eprc_class", classOf(tizprc),
   sizeof(h264e_prc_class_t),
   /* TIZ_CLASS_COMMENT: */
   ap_tos, ap_hdl,
   /* TIZ_CLASS_COMMENT: class constructor */
   ctor, h264e_prc_class_ctor,
   /* TIZ_CLASS_COMMENT: stop value*/
         }
      void * h264e_prc_init(void * ap_tos, void * ap_hdl)
   {
      void * tizprc = tiz_get_type(ap_hdl, "tizprc");
   void * h264eprc_class = tiz_get_type(ap_hdl, "h264eprc_class");
   TIZ_LOG_CLASS (h264eprc_class);
   void * h264eprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
   (h264eprc_class, "h264eprc", tizprc, sizeof(vid_enc_PrivateType),
   /* TIZ_CLASS_COMMENT: */
   ap_tos, ap_hdl,
   /* TIZ_CLASS_COMMENT: class constructor */
   ctor, h264e_prc_ctor,
   /* TIZ_CLASS_COMMENT: class destructor */
   dtor, h264e_prc_dtor,
   /* TIZ_CLASS_COMMENT: */
   tiz_srv_allocate_resources, h264e_prc_allocate_resources,
   /* TIZ_CLASS_COMMENT: */
   tiz_srv_deallocate_resources, h264e_prc_deallocate_resources,
   /* TIZ_CLASS_COMMENT: */
   tiz_srv_prepare_to_transfer, h264e_prc_prepare_to_transfer,
   /* TIZ_CLASS_COMMENT: */
   tiz_srv_transfer_and_process, h264e_prc_transfer_and_process,
   /* TIZ_CLASS_COMMENT: */
   tiz_srv_stop_and_return, h264e_prc_stop_and_return,
   /* TIZ_CLASS_COMMENT: */
   tiz_prc_buffers_ready, h264e_prc_buffers_ready,
   /* TIZ_CLASS_COMMENT: */
   tiz_prc_port_flush, h264e_prc_port_flush,
   /* TIZ_CLASS_COMMENT: */
   tiz_prc_port_disable, h264e_prc_port_disable,
   /* TIZ_CLASS_COMMENT: */
   tiz_prc_port_enable, h264e_prc_port_enable,
   /* TIZ_CLASS_COMMENT: stop value*/
            }
