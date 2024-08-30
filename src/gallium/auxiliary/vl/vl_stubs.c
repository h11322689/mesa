   #include <assert.h>
      #include "vl_decoder.h"
   #include "vl_mpeg12_bitstream.h"
   #include "vl_mpeg12_decoder.h"
   #include "vl_video_buffer.h"
   #include "vl_zscan.h"
         /*
   * vl_decoder stubs
   */
   bool
   vl_profile_supported(struct pipe_screen *screen,
               {
      assert(0);
      }
      int
   vl_level_supported(struct pipe_screen *screen,
         {
      assert(0);
      }
      struct pipe_video_codec *
   vl_create_decoder(struct pipe_context *pipe,
         {
      assert(0);
      }
         /*
   * vl_video_buffer stubs
   */
   void
   vl_get_video_buffer_formats(struct pipe_screen *screen, enum pipe_format format,
         {
         }
      bool
   vl_video_buffer_is_format_supported(struct pipe_screen *screen,
                     {
      assert(0);
      }
      unsigned
   vl_video_buffer_max_size(struct pipe_screen *screen)
   {
      assert(0);
      }
      void
   vl_video_buffer_set_associated_data(struct pipe_video_buffer *vbuf,
                     {
         }
      void *
   vl_video_buffer_get_associated_data(struct pipe_video_buffer *vbuf,
         {
      assert(0);
      }
      void
   vl_video_buffer_template(struct pipe_resource *templ,
                           const struct pipe_video_buffer *tmpl,
   {
         }
      struct pipe_video_buffer *
   vl_video_buffer_create(struct pipe_context *pipe,
         {
      assert(0);
      }
      struct pipe_video_buffer *
   vl_video_buffer_create_ex2(struct pipe_context *pipe,
               {
      assert(0);
      }
      void
   vl_video_buffer_destroy(struct pipe_video_buffer *buffer)
   {
         }
      /*
   * vl_mpeg12_bitstream stubs
   */
   void
   vl_mpg12_bs_init(struct vl_mpg12_bs *bs, struct pipe_video_codec *decoder)
   {
         }
      void
   vl_mpg12_bs_decode(struct vl_mpg12_bs *bs,
                     struct pipe_video_buffer *target,
   struct pipe_mpeg12_picture_desc *picture,
   {
         }
         /*
   * vl_mpeg12_decoder stubs
   */
   struct pipe_video_codec *
   vl_create_mpeg12_decoder(struct pipe_context *pipe,
         {
      assert(0);
      }
      /*
   * vl_zscan
   */
   const int vl_zscan_normal[] = {0};
   const int vl_zscan_alternate[] = {0};
