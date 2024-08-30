   /*
   * Copyright 2011-2013 Maarten Lankhorst
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include <sys/mman.h>
   #include <sys/stat.h>
   #include <stdio.h>
   #include <fcntl.h>
      #include <nvif/class.h>
      #include "nouveau_winsys.h"
   #include "nouveau_screen.h"
   #include "nouveau_context.h"
   #include "nouveau_vp3_video.h"
      #include "util/u_video.h"
   #include "util/format/u_format.h"
   #include "util/u_sampler.h"
      static void
   nouveau_vp3_video_buffer_resources(struct pipe_video_buffer *buffer,
         {
      struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
                     for (i = 0; i < buf->num_planes; ++i) {
            }
      static struct pipe_sampler_view **
   nouveau_vp3_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
   {
      struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
      }
      static struct pipe_sampler_view **
   nouveau_vp3_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
   {
      struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
      }
      static struct pipe_surface **
   nouveau_vp3_video_buffer_surfaces(struct pipe_video_buffer *buffer)
   {
      struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
      }
      static void
   nouveau_vp3_video_buffer_destroy(struct pipe_video_buffer *buffer)
   {
      struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
                     for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      pipe_resource_reference(&buf->resources[i], NULL);
   pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);
   pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);
   pipe_surface_reference(&buf->surfaces[i * 2], NULL);
      }
      }
      struct pipe_video_buffer *
   nouveau_vp3_video_buffer_create(struct pipe_context *pipe,
               {
      struct nouveau_vp3_video_buffer *buffer;
   struct pipe_resource templ;
   unsigned i, j, component;
   struct pipe_sampler_view sv_templ;
            if (templat->buffer_format != PIPE_FORMAT_NV12)
            assert(templat->interlaced);
            buffer = CALLOC_STRUCT(nouveau_vp3_video_buffer);
   if (!buffer)
            buffer->base.buffer_format = templat->buffer_format;
   buffer->base.context = pipe;
   buffer->base.destroy = nouveau_vp3_video_buffer_destroy;
   buffer->base.width = templat->width;
   buffer->base.height = templat->height;
   buffer->base.get_resources = nouveau_vp3_video_buffer_resources;
   buffer->base.get_sampler_view_planes = nouveau_vp3_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = nouveau_vp3_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = nouveau_vp3_video_buffer_surfaces;
            memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D_ARRAY;
   templ.depth0 = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = buffer->base.width;
   templ.height0 = (buffer->base.height + 1)/2;
   templ.flags = flags;
            buffer->resources[0] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[0])
            templ.format = PIPE_FORMAT_R8G8_UNORM;
   buffer->num_planes = 2;
   templ.width0 = (templ.width0 + 1) / 2;
   templ.height0 = (templ.height0 + 1) / 2;
   for (i = 1; i < buffer->num_planes; ++i) {
      buffer->resources[i] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[i])
               memset(&sv_templ, 0, sizeof(sv_templ));
   for (component = 0, i = 0; i < buffer->num_planes; ++i ) {
      struct pipe_resource *res = buffer->resources[i];
            u_sampler_view_default_template(&sv_templ, res, res->format);
   buffer->sampler_view_planes[i] = pipe->create_sampler_view(pipe, res, &sv_templ);
   if (!buffer->sampler_view_planes[i])
            for (j = 0; j < nr_components; ++j, ++component) {
                     buffer->sampler_view_components[component] = pipe->create_sampler_view(pipe, res, &sv_templ);
   if (!buffer->sampler_view_components[component])
      }
         memset(&surf_templ, 0, sizeof(surf_templ));
   for (j = 0; j < buffer->num_planes; ++j) {
      surf_templ.format = buffer->resources[j]->format;
   surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = 0;
   buffer->surfaces[j * 2] = pipe->create_surface(pipe, buffer->resources[j], &surf_templ);
   if (!buffer->surfaces[j * 2])
            surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = 1;
   buffer->surfaces[j * 2 + 1] = pipe->create_surface(pipe, buffer->resources[j], &surf_templ);
   if (!buffer->surfaces[j * 2 + 1])
                     error:
      nouveau_vp3_video_buffer_destroy(&buffer->base);
      }
      static void
   nouveau_vp3_decoder_flush(struct pipe_video_codec *decoder)
   {
   }
      static void
   nouveau_vp3_decoder_begin_frame(struct pipe_video_codec *decoder,
               {
   }
      static void
   nouveau_vp3_decoder_end_frame(struct pipe_video_codec *decoder,
               {
   }
      static void
   nouveau_vp3_decoder_destroy(struct pipe_video_codec *decoder)
   {
      struct nouveau_vp3_decoder *dec = (struct nouveau_vp3_decoder *)decoder;
            nouveau_bo_ref(NULL, &dec->ref_bo);
   nouveau_bo_ref(NULL, &dec->bitplane_bo);
   nouveau_bo_ref(NULL, &dec->inter_bo[0]);
      #if NOUVEAU_VP3_DEBUG_FENCE
         #endif
               for (i = 0; i < NOUVEAU_VP3_VIDEO_QDEPTH; ++i)
            nouveau_object_del(&dec->bsp);
   nouveau_object_del(&dec->vp);
            if (dec->channel[0] != dec->channel[1]) {
      for (i = 0; i < 3; ++i) {
      nouveau_pushbuf_destroy(&dec->pushbuf[i]);
         } else {
      nouveau_pushbuf_destroy(dec->pushbuf);
                  }
      void
   nouveau_vp3_decoder_init_common(struct pipe_video_codec *dec)
   {
      dec->destroy = nouveau_vp3_decoder_destroy;
   dec->flush = nouveau_vp3_decoder_flush;
   dec->begin_frame = nouveau_vp3_decoder_begin_frame;
      }
      static void vp3_getpath(enum pipe_video_profile profile, char *path)
   {
      switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_FORMAT_MPEG12: {
      sprintf(path, "/lib/firmware/nouveau/vuc-vp3-mpeg12-0");
      }
   case PIPE_VIDEO_FORMAT_VC1: {
      sprintf(path, "/lib/firmware/nouveau/vuc-vp3-vc1-0");
      }
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      sprintf(path, "/lib/firmware/nouveau/vuc-vp3-h264-0");
      }
         }
      static void vp4_getpath(enum pipe_video_profile profile, char *path)
   {
      switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_FORMAT_MPEG12: {
      sprintf(path, "/lib/firmware/nouveau/vuc-mpeg12-0");
      }
   case PIPE_VIDEO_FORMAT_MPEG4: {
      sprintf(path, "/lib/firmware/nouveau/vuc-mpeg4-0");
      }
   case PIPE_VIDEO_FORMAT_VC1: {
      sprintf(path, "/lib/firmware/nouveau/vuc-vc1-0");
      }
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      sprintf(path, "/lib/firmware/nouveau/vuc-h264-0");
      }
         }
      int
   nouveau_vp3_load_firmware(struct nouveau_vp3_decoder *dec,
               {
      int fd;
   char path[PATH_MAX];
   ssize_t r;
   uint32_t *end, endval;
            if (chipset >= 0xa3 && chipset != 0xaa && chipset != 0xac)
         else
            if (BO_MAP(screen, dec->fw_bo, NOUVEAU_BO_WR, dec->client))
            fd = open(path, O_RDONLY | O_CLOEXEC);
   if (fd < 0) {
      fprintf(stderr, "opening firmware file %s failed: %m\n", path);
      }
   r = read(fd, dec->fw_bo->map, 0x4000);
            if (r < 0) {
      fprintf(stderr, "reading firmware file %s failed: %m\n", path);
               if (r == 0x4000) {
      fprintf(stderr, "firmware file %s too large!\n", path);
               if (r & 0xff) {
      fprintf(stderr, "firmware file %s wrong size!\n", path);
               end = dec->fw_bo->map + r - 4;
   endval = *end;
   while (endval == *end)
                     switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_FORMAT_MPEG12: {
      assert((r & 0xff) == 0xe0);
   dec->fw_sizes = (0x2e0<<16) | (r - 0x2e0);
      }
   case PIPE_VIDEO_FORMAT_MPEG4: {
      assert((r & 0xff) == 0xe0);
   dec->fw_sizes = (0x2e0<<16) | (r - 0x2e0);
      }
   case PIPE_VIDEO_FORMAT_VC1: {
      assert((r & 0xff) == 0xac);
   dec->fw_sizes = (0x3ac<<16) | (r - 0x3ac);
      }
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      assert((r & 0xff) == 0x70);
   dec->fw_sizes = (0x370<<16) | (r - 0x370);
      }
   default:
      }
   munmap(dec->fw_bo->map, dec->fw_bo->size);
   dec->fw_bo->map = NULL;
      }
      static const struct nouveau_mclass
   nouveau_decoder_msvld[] = {
      { G98_MSVLD, -1 },
   { IGT21A_MSVLD, -1 },
   { GT212_MSVLD, -1 },
   { GF100_MSVLD, -1 },
   { GK104_MSVLD, -1 },
      };
      static int
   firmware_present(struct pipe_screen *pscreen, enum pipe_video_profile profile)
   {
      struct nouveau_screen *screen = nouveau_screen(pscreen);
   int chipset = screen->device->chipset;
   int vp3 = chipset < 0xa3 || chipset == 0xaa || chipset == 0xac;
   int vp5 = chipset >= 0xd0;
            /* For all chipsets, try to create a BSP objects. Assume that if firmware
   * is present for it, firmware is also present for VP/PPP */
   if (!(screen->firmware_info.profiles_checked & 1)) {
      struct nouveau_object *channel = NULL, *bsp = NULL;
   struct nv04_fifo nv04_data = {.vram = 0xbeef0201, .gart = 0xbeef0202};
   struct nvc0_fifo nvc0_args = {};
   struct nve0_fifo nve0_args = {.engine = NVE0_FIFO_ENGINE_BSP};
   void *data = NULL;
            if (chipset < 0xc0) {
      data = &nv04_data;
      } else if (chipset < 0xe0) {
      data = &nvc0_args;
      } else {
      data = &nve0_args;
               /* kepler must have its own channel, so just do this for everyone */
   nouveau_object_new(&screen->device->object, 0,
                  if (channel) {
      ret = nouveau_object_mclass(channel, nouveau_decoder_msvld);
   if (ret >= 0)
      nouveau_object_new(channel, 0, nouveau_decoder_msvld[ret].oclass,
      if (bsp)
         nouveau_object_del(&bsp);
      }
               if (!(screen->firmware_info.profiles_present & 1))
            /* For vp3/vp4 chipsets, make sure that the relevant firmware is present */
   if (!vp5 && !(screen->firmware_info.profiles_checked & (1 << profile))) {
      char path[PATH_MAX];
   struct stat s;
   if (vp3)
         else
         ret = stat(path, &s);
   if (!ret && s.st_size > 1000)
                        }
      int
   nouveau_vp3_screen_get_video_param(struct pipe_screen *pscreen,
                     {
      const int chipset = nouveau_screen(pscreen)->device->chipset;
   /* Feature Set B = vp3, C = vp4, D = vp5 */
   const bool vp3 = chipset < 0xa3 || chipset == 0xaa || chipset == 0xac;
   const bool vp5 = chipset >= 0xd0;
   enum pipe_video_format codec = u_reduce_video_profile(profile);
   switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      /* VP3 does not support MPEG4, VP4+ do. */
   return entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM &&
      profile >= PIPE_VIDEO_PROFILE_MPEG1 &&
   profile < PIPE_VIDEO_PROFILE_HEVC_MAIN &&
   (!vp3 || codec != PIPE_VIDEO_FORMAT_MPEG4) &&
   case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
      switch (codec) {
   case PIPE_VIDEO_FORMAT_MPEG12:
         case PIPE_VIDEO_FORMAT_MPEG4:
         case PIPE_VIDEO_FORMAT_VC1:
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      if (vp3)
         if (vp5)
            case PIPE_VIDEO_FORMAT_UNKNOWN:
         default:
      debug_printf("unknown video codec: %d\n", codec);
         case PIPE_VIDEO_CAP_MAX_HEIGHT:
      switch (codec) {
   case PIPE_VIDEO_FORMAT_MPEG12:
         case PIPE_VIDEO_FORMAT_MPEG4:
         case PIPE_VIDEO_FORMAT_VC1:
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      if (vp3)
         if (vp5)
            case PIPE_VIDEO_FORMAT_UNKNOWN:
         default:
      debug_printf("unknown video codec: %d\n", codec);
         case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
   case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_MAX_LEVEL:
      switch (profile) {
   case PIPE_VIDEO_PROFILE_MPEG1:
         case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
   case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
         case PIPE_VIDEO_PROFILE_MPEG4_SIMPLE:
         case PIPE_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE:
         case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
         case PIPE_VIDEO_PROFILE_VC1_MAIN:
         case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
         default:
      debug_printf("unknown video profile: %d\n", profile);
         case PIPE_VIDEO_CAP_MAX_MACROBLOCKS:
      switch (codec) {
   case PIPE_VIDEO_FORMAT_MPEG12:
         case PIPE_VIDEO_FORMAT_MPEG4:
         case PIPE_VIDEO_FORMAT_VC1:
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      if (vp3)
         if (vp5)
            default:
      debug_printf("unknown video codec: %d\n", codec);
         default:
      debug_printf("unknown video param: %d\n", param);
         }
      bool
   nouveau_vp3_screen_video_supported(struct pipe_screen *screen,
                     {
      if (profile != PIPE_VIDEO_PROFILE_UNKNOWN)
               }
