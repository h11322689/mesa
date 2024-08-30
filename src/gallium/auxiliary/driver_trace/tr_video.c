   #include "tr_public.h"
   #include "tr_dump.h"
   #include "tr_dump_state.h"
   #include "tr_texture.h"
   #include "u_inlines.h"
   #include "u_video.h"
   #include "tr_video.h"
   #include "pipe/p_video_codec.h"
   #include "pipe/p_video_enums.h"
   #include "util/macros.h"
   #include "util/u_memory.h"
   #include "vl/vl_defines.h"
      static void
   trace_video_codec_destroy(struct pipe_video_codec *_codec)
   {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
            trace_dump_call_begin("pipe_video_codec", "destroy");
   trace_dump_arg(ptr, video_codec);
                        }
      static void
   unwrap_refrence_frames_in_place(struct pipe_video_buffer **refrence_frames, unsigned max_num_refrence_frame)
   {
      for (unsigned i=0; i < max_num_refrence_frame; i++) {
      if (refrence_frames[i]) {
         struct trace_video_buffer *tr_buffer = trace_video_buffer(refrence_frames[i]);
         }
      static bool
   unwrap_refrence_frames(struct pipe_picture_desc **picture)
   {
      // only decode pictures use video buffers for refrences
   if ((*picture)->entry_point != PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
         switch (u_reduce_video_profile((*picture)->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12: {
      struct pipe_mpeg12_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_mpeg12_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_MPEG4: {
      struct pipe_mpeg4_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_mpeg4_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_VC1:{
      struct pipe_vc1_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_vc1_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      struct pipe_h264_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_h264_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_HEVC:{
      struct pipe_h265_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_h265_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_JPEG:
         case PIPE_VIDEO_FORMAT_VP9:{
      struct pipe_vp9_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_vp9_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_AV1:{
      struct pipe_av1_picture_desc *copied = mem_dup(*picture, sizeof(struct pipe_av1_picture_desc));
   assert(copied);
   unwrap_refrence_frames_in_place(copied->ref, ARRAY_SIZE(copied->ref));
   unwrap_refrence_frames_in_place(&copied->film_grain_target, 1);
   *picture = (struct pipe_picture_desc*)copied;
      }
   case PIPE_VIDEO_FORMAT_UNKNOWN:
   default:
            }
      static void
   trace_video_codec_begin_frame(struct pipe_video_codec *_codec,
               {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_target = trace_video_buffer(_target);
            trace_dump_call_begin("pipe_video_codec", "begin_frame");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, target);
   trace_dump_arg(pipe_picture_desc, picture);
            bool copied = unwrap_refrence_frames(&picture);
   codec->begin_frame(codec, target, picture);
   if (copied)
      }
      static void
   trace_video_codec_decode_macroblock(struct pipe_video_codec *_codec,
                           {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_target = trace_video_buffer(_target);
            trace_dump_call_begin("pipe_video_codec", "decode_macroblock");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, target);
   trace_dump_arg(pipe_picture_desc, picture);
   // TODO: how to dump pipe_macroblocks? It's only a single pointer,
   //  but each struct has codec dependent size, so can't use generic trace_dump_arg_array
   trace_dump_arg(ptr, macroblocks);
   trace_dump_arg(uint, num_macroblocks);
            bool copied = unwrap_refrence_frames(&picture);
   codec->decode_macroblock(codec, target, picture, macroblocks, num_macroblocks);
   if (copied)
      }
      static void 
   trace_video_codec_decode_bitstream(struct pipe_video_codec *_codec,
                           struct pipe_video_buffer *_target,
   {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_target = trace_video_buffer(_target);
            trace_dump_call_begin("pipe_video_codec", "decode_bitstream");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, target);
            trace_dump_arg(uint, num_buffers);
   trace_dump_arg_array(ptr, buffers, num_buffers);
   trace_dump_arg_array(uint, sizes, num_buffers);
            bool copied = unwrap_refrence_frames(&picture);
   codec->decode_bitstream(codec, target, picture, num_buffers, buffers, sizes);
   if (copied)
      }
      static void
   trace_video_codec_encode_bitstream(struct pipe_video_codec *_codec,
                     {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_source = trace_video_buffer(_source);
            trace_dump_call_begin("pipe_video_codec", "encode_bitstream");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, source);
   trace_dump_arg(ptr, destination);
   trace_dump_arg(ptr, feedback);
               }
      static void
   trace_video_codec_process_frame(struct pipe_video_codec *_codec,
               {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_source = trace_video_buffer(_source);
            trace_dump_call_begin("pipe_video_codec", "process_frame");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, source);
   trace_dump_arg(pipe_vpp_desc, process_properties);
               }
      static void
   trace_video_codec_end_frame(struct pipe_video_codec *_codec,
               {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_target = trace_video_buffer(_target);
            trace_dump_call_begin("pipe_video_codec", "end_frame");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, target);
   trace_dump_arg(pipe_picture_desc, picture);
            bool copied = unwrap_refrence_frames(&picture);
   codec->end_frame(codec, target, picture);
   if (copied)
      }
      static void
   trace_video_codec_flush(struct pipe_video_codec *_codec)
   {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
            trace_dump_call_begin("pipe_video_codec", "flush");
   trace_dump_arg(ptr, codec);
               }
      static void
   trace_video_codec_get_feedback(struct pipe_video_codec *_codec, void *feedback, unsigned *size)
   {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
            trace_dump_call_begin("pipe_video_codec", "get_feedback");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, feedback);
   trace_dump_arg(ptr, size);
               }
      static int
   trace_video_codec_get_decoder_fence(struct pipe_video_codec *_codec,
               {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
            trace_dump_call_begin("pipe_video_codec", "get_decoder_fence");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, fence);
                     trace_dump_ret(int, ret);
               }
      static int
   trace_video_codec_get_processor_fence(struct pipe_video_codec *_codec,
               {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
            trace_dump_call_begin("pipe_video_codec", "get_processor_fence");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, fence);
                     trace_dump_ret(int, ret);
               }
      static void
   trace_video_codec_update_decoder_target(struct pipe_video_codec *_codec,
               {
      struct trace_video_codec *tr_vcodec = trace_video_codec(_codec);
   struct pipe_video_codec *codec = tr_vcodec->video_codec;
   struct trace_video_buffer *tr_old = trace_video_buffer(_old);
   struct pipe_video_buffer *old = tr_old->video_buffer;
   struct trace_video_buffer *tr_updated = trace_video_buffer(_updated);
            trace_dump_call_begin("pipe_video_codec", "update_decoder_target");
   trace_dump_arg(ptr, codec);
   trace_dump_arg(ptr, old);
   trace_dump_arg(ptr, updated);
               }
      struct pipe_video_codec *
   trace_video_codec_create(struct trace_context *tr_ctx,
         {
               if (!video_codec)
            if (!trace_enabled())
            tr_vcodec = rzalloc(NULL, struct trace_video_codec);
   if (!tr_vcodec)
            memcpy(&tr_vcodec->base, video_codec, sizeof(struct pipe_video_codec));
         #define TR_VC_INIT(_member) \
               TR_VC_INIT(destroy);
   TR_VC_INIT(begin_frame);
   TR_VC_INIT(decode_macroblock);
   TR_VC_INIT(decode_bitstream);
   TR_VC_INIT(encode_bitstream);
   TR_VC_INIT(process_frame);
   TR_VC_INIT(end_frame);
   TR_VC_INIT(flush);
   TR_VC_INIT(get_feedback);
   TR_VC_INIT(get_decoder_fence);
   TR_VC_INIT(get_processor_fence);
         #undef TR_VC_INIT
                        error1:
         }
         static void
   trace_video_buffer_destroy(struct pipe_video_buffer *_buffer)
   {
      struct trace_video_buffer *tr_vbuffer = trace_video_buffer(_buffer);
            trace_dump_call_begin("pipe_video_buffer", "destroy");
   trace_dump_arg(ptr, video_buffer);
            for (int i=0; i < VL_NUM_COMPONENTS; i++) {
      pipe_sampler_view_reference(&tr_vbuffer->sampler_view_planes[i], NULL);
      }
   for (int i=0; i < VL_MAX_SURFACES; i++) {
         }
               }
      static void
   trace_video_buffer_get_resources(struct pipe_video_buffer *_buffer, struct pipe_resource **resources)
   {
      struct trace_video_buffer *tr_vbuffer = trace_video_buffer(_buffer);
            trace_dump_call_begin("pipe_video_buffer", "get_resources");
                     // TODO: A `trace_dump_ret_arg` style of function would be more appropriate
   trace_dump_arg_array(ptr, resources, VL_NUM_COMPONENTS);
      }
      static struct pipe_sampler_view **
   trace_video_buffer_get_sampler_view_planes(struct pipe_video_buffer *_buffer)
   {
      struct trace_context *tr_ctx = trace_context(_buffer->context);
   struct trace_video_buffer *tr_vbuffer = trace_video_buffer(_buffer);
            trace_dump_call_begin("pipe_video_buffer", "get_sampler_view_planes");
                     trace_dump_ret_array(ptr, view_planes, VL_NUM_COMPONENTS);
            for (int i=0; i < VL_NUM_COMPONENTS; i++) {
      if (!view_planes || !view_planes[i]) {
         } else if (tr_vbuffer->sampler_view_planes[i] == NULL || (trace_sampler_view(tr_vbuffer->sampler_view_planes[i])->sampler_view != view_planes[i])) {
                        }
      static struct pipe_sampler_view **
   trace_video_buffer_get_sampler_view_components(struct pipe_video_buffer *_buffer)
   {
      struct trace_context *tr_ctx = trace_context(_buffer->context);
   struct trace_video_buffer *tr_vbuffer = trace_video_buffer(_buffer);
            trace_dump_call_begin("pipe_video_buffer", "get_sampler_view_components");
                     trace_dump_ret_array(ptr, view_components, VL_NUM_COMPONENTS);
            for (int i=0; i < VL_NUM_COMPONENTS; i++) {
      if (!view_components || !view_components[i]) {
         } else if (tr_vbuffer->sampler_view_components[i] == NULL || (trace_sampler_view(tr_vbuffer->sampler_view_components[i])->sampler_view != view_components[i])) {
                        }
      static struct pipe_surface **
   trace_video_buffer_get_surfaces(struct pipe_video_buffer *_buffer)
   {
      struct trace_context *tr_ctx = trace_context(_buffer->context);
   struct trace_video_buffer *tr_vbuffer = trace_video_buffer(_buffer);
            trace_dump_call_begin("pipe_video_buffer", "get_surfaces");
                     trace_dump_ret_array(ptr, surfaces, VL_MAX_SURFACES);
            for (int i=0; i < VL_MAX_SURFACES; i++) {
      if (!surfaces || !surfaces[i]) {
         } else if (tr_vbuffer->surfaces[i] == NULL || (trace_surface(tr_vbuffer->surfaces[i])->surface != surfaces[i])){
                        }
         struct pipe_video_buffer *
   trace_video_buffer_create(struct trace_context *tr_ctx,
         {
               if (!video_buffer)
            if (!trace_enabled())
            tr_vbuffer = rzalloc(NULL, struct trace_video_buffer);
   if (!tr_vbuffer)
            memcpy(&tr_vbuffer->base, video_buffer, sizeof(struct pipe_video_buffer));
         #define TR_VB_INIT(_member) \
               TR_VB_INIT(destroy);
   TR_VB_INIT(get_resources);
   TR_VB_INIT(get_sampler_view_planes);
   TR_VB_INIT(get_sampler_view_components);
         #undef TR_VB_INIT
                        error1:
         }
