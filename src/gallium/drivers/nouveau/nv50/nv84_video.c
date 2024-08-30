   /*
   * Copyright 2013 Ilia Mirkin
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
   #include <sys/types.h>
   #include <fcntl.h>
      #include "util/format/u_format.h"
   #include "util/u_sampler.h"
   #include "vl/vl_zscan.h"
      #include "nv50/nv84_video.h"
      static int
   nv84_copy_firmware(const char *path, void *dest, ssize_t len)
   {
      int fd = open(path, O_RDONLY | O_CLOEXEC);
   ssize_t r;
   if (fd < 0) {
      fprintf(stderr, "opening firmware file %s failed: %m\n", path);
      }
   r = read(fd, dest, len);
            if (r != len) {
      fprintf(stderr, "reading firmware file %s failed: %m\n", path);
                  }
      static int
   filesize(const char *path)
   {
      int ret;
            ret = stat(path, &statbuf);
   if (ret)
            }
      static struct nouveau_bo *
   nv84_load_firmwares(struct nouveau_device *dev, struct nv84_decoder *dec,
         {
      int ret, size1, size2 = 0;
   struct nouveau_bo *fw;
            size1 = filesize(fw1);
   if (fw2)
         if (size1 < 0 || size2 < 0)
                     ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, dec->vp_fw2_offset + size2, NULL, &fw);
   if (ret)
         ret = BO_MAP(screen, fw, NOUVEAU_BO_WR, dec->client);
   if (ret)
            ret = nv84_copy_firmware(fw1, fw->map, size1);
   if (fw2 && !ret)
         munmap(fw->map, fw->size);
   fw->map = NULL;
   if (!ret)
      error:
      nouveau_bo_ref(NULL, &fw);
      }
      static struct nouveau_bo *
   nv84_load_bsp_firmware(struct nouveau_device *dev, struct nv84_decoder *dec)
   {
      return nv84_load_firmwares(
      }
      static struct nouveau_bo *
   nv84_load_vp_firmware(struct nouveau_device *dev, struct nv84_decoder *dec)
   {
      return nv84_load_firmwares(
         dev, dec,
      }
      static struct nouveau_bo *
   nv84_load_vp_firmware_mpeg(struct nouveau_device *dev, struct nv84_decoder *dec)
   {
      return nv84_load_firmwares(
      }
      static void
   nv84_decoder_decode_bitstream_h264(struct pipe_video_codec *decoder,
                                 {
      struct nv84_decoder *dec = (struct nv84_decoder *)decoder;
                              nv84_decoder_bsp(dec, desc, num_buffers, data, num_bytes, target);
      }
      static void
   nv84_decoder_flush(struct pipe_video_codec *decoder)
   {
   }
      static void
   nv84_decoder_begin_frame_h264(struct pipe_video_codec *decoder,
               {
   }
      static void
   nv84_decoder_end_frame_h264(struct pipe_video_codec *decoder,
               {
   }
      static void
   nv84_decoder_decode_bitstream_mpeg12(struct pipe_video_codec *decoder,
                                 {
                        vl_mpg12_bs_decode(dec->mpeg12_bs,
                     video_target,
      }
      static void
   nv84_decoder_begin_frame_mpeg12(struct pipe_video_codec *decoder,
               {
      struct nouveau_screen *screen = nouveau_screen(decoder->context->screen);
   struct nv84_decoder *dec = (struct nv84_decoder *)decoder;
   struct pipe_mpeg12_picture_desc *desc = (struct pipe_mpeg12_picture_desc *)picture;
            BO_WAIT(screen, dec->mpeg12_bo, NOUVEAU_BO_RDWR, dec->client);
   dec->mpeg12_mb_info = dec->mpeg12_bo->map + 0x100;
   dec->mpeg12_data = dec->mpeg12_bo->map + 0x100 +
         if (desc->intra_matrix) {
      dec->zscan = desc->alternate_scan ? vl_zscan_alternate : vl_zscan_normal;
   for (i = 0; i < 64; i++) {
      dec->mpeg12_intra_matrix[i] = desc->intra_matrix[dec->zscan[i]];
      }
         }
      static void
   nv84_decoder_end_frame_mpeg12(struct pipe_video_codec *decoder,
               {
      nv84_decoder_vp_mpeg12(
         (struct nv84_decoder *)decoder,
      }
      static void
   nv84_decoder_decode_macroblock(struct pipe_video_codec *decoder,
                           {
      const struct pipe_mpeg12_macroblock *mb = (const struct pipe_mpeg12_macroblock *)macroblocks;
   for (int i = 0; i < num_macroblocks; i++, mb++) {
      nv84_decoder_vp_mpeg12_mb(
         (struct nv84_decoder *)decoder,
         }
      static void
   nv84_decoder_destroy(struct pipe_video_codec *decoder)
   {
               nouveau_bo_ref(NULL, &dec->bsp_fw);
   nouveau_bo_ref(NULL, &dec->bsp_data);
   nouveau_bo_ref(NULL, &dec->vp_fw);
   nouveau_bo_ref(NULL, &dec->vp_data);
   nouveau_bo_ref(NULL, &dec->mbring);
   nouveau_bo_ref(NULL, &dec->vpring);
   nouveau_bo_ref(NULL, &dec->bitstream);
   nouveau_bo_ref(NULL, &dec->vp_params);
            nouveau_object_del(&dec->bsp);
            nouveau_bufctx_del(&dec->bsp_bufctx);
   nouveau_pushbuf_destroy(&dec->bsp_pushbuf);
            nouveau_bufctx_del(&dec->vp_bufctx);
   nouveau_pushbuf_destroy(&dec->vp_pushbuf);
                     FREE(dec->mpeg12_bs);
      }
      struct pipe_video_codec *
   nv84_create_decoder(struct pipe_context *context,
         {
      struct nv50_context *nv50 = (struct nv50_context *)context;
   struct nouveau_screen *screen = &nv50->screen->base;
   struct nv84_decoder *dec;
   struct nouveau_pushbuf *bsp_push, *vp_push;
   struct nv50_surface surf;
   struct nv50_miptree mip;
   union pipe_color_union color;
   struct nv04_fifo nv04_data = { .vram = 0xbeef0201, .gart = 0xbeef0202 };
   int ret, i;
   int is_h264 = u_reduce_video_profile(templ->profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC;
            if ((is_h264 && templ->entrypoint != PIPE_VIDEO_ENTRYPOINT_BITSTREAM) ||
      (is_mpeg12 && templ->entrypoint > PIPE_VIDEO_ENTRYPOINT_IDCT)) {
   debug_printf("%x\n", templ->entrypoint);
               if (!is_h264 && !is_mpeg12) {
      debug_printf("invalid profile: %x\n", templ->profile);
               dec = CALLOC_STRUCT(nv84_decoder);
   if (!dec)
            dec->base = *templ;
   dec->base.context = context;
   dec->base.destroy = nv84_decoder_destroy;
   dec->base.flush = nv84_decoder_flush;
   if (is_h264) {
      dec->base.decode_bitstream = nv84_decoder_decode_bitstream_h264;
   dec->base.begin_frame = nv84_decoder_begin_frame_h264;
            dec->frame_mbs = mb(dec->base.width) * mb_half(dec->base.height) * 2;
   dec->frame_size = dec->frame_mbs << 8;
   dec->vpring_deblock = align(0x30 * dec->frame_mbs, 0x100);
   dec->vpring_residual = 0x2000 + MAX2(0x32000, 0x600 * dec->frame_mbs);
      } else if (is_mpeg12) {
      dec->base.decode_macroblock = nv84_decoder_decode_macroblock;
   dec->base.begin_frame = nv84_decoder_begin_frame_mpeg12;
            if (templ->entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      dec->mpeg12_bs = CALLOC_STRUCT(vl_mpg12_bs);
   if (!dec->mpeg12_bs)
         vl_mpg12_bs_init(dec->mpeg12_bs, &dec->base);
         } else {
                  ret = nouveau_client_new(screen->device, &dec->client);
   if (ret)
            if (is_h264) {
      ret = nouveau_object_new(&screen->device->object, 0,
               if (ret)
            ret = nouveau_pushbuf_create(screen, &nv50->base, dec->client, dec->bsp_channel,
         if (ret)
            ret = nouveau_bufctx_new(dec->client, 1, &dec->bsp_bufctx);
   if (ret)
               ret = nouveau_object_new(&screen->device->object, 0,
               if (ret)
         ret = nouveau_pushbuf_create(screen, &nv50->base, dec->client, dec->vp_channel,
         if (ret)
            ret = nouveau_bufctx_new(dec->client, 1, &dec->vp_bufctx);
   if (ret)
            bsp_push = dec->bsp_pushbuf;
            if (is_h264) {
      dec->bsp_fw = nv84_load_bsp_firmware(screen->device, dec);
   dec->vp_fw = nv84_load_vp_firmware(screen->device, dec);
   if (!dec->bsp_fw || !dec->vp_fw)
      }
   if (is_mpeg12) {
      dec->vp_fw = nv84_load_vp_firmware_mpeg(screen->device, dec);
   if (!dec->vp_fw)
               if (is_h264) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP,
         if (ret)
      }
   ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP,
         if (ret)
         if (is_h264) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP,
                        0,
   2 * (dec->vpring_deblock +
      if (ret)
         ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP,
                           if (ret)
         ret = nouveau_bo_new(screen->device, NOUVEAU_BO_GART,
               if (ret)
         ret = BO_MAP(screen, dec->bitstream, NOUVEAU_BO_WR, dec->client);
   if (ret)
         ret = nouveau_bo_new(screen->device, NOUVEAU_BO_GART,
         if (ret)
         ret = BO_MAP(screen, dec->vp_params, NOUVEAU_BO_WR, dec->client);
   if (ret)
      }
   if (is_mpeg12) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_GART,
                           if (ret)
         ret = BO_MAP(screen, dec->mpeg12_bo, NOUVEAU_BO_WR, dec->client);
   if (ret)
               ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
         if (ret)
         ret = BO_MAP(screen, dec->fence, NOUVEAU_BO_WR, dec->client);
   if (ret)
                  if (is_h264) {
      nouveau_pushbuf_bufctx(bsp_push, dec->bsp_bufctx);
   nouveau_bufctx_refn(dec->bsp_bufctx, 0,
         nouveau_bufctx_refn(dec->bsp_bufctx, 0,
               nouveau_pushbuf_bufctx(vp_push, dec->vp_bufctx);
   nouveau_bufctx_refn(dec->vp_bufctx, 0, dec->vp_fw,
         nouveau_bufctx_refn(dec->vp_bufctx, 0, dec->vp_data,
            if (is_h264 && !ret)
      ret = nouveau_object_new(dec->bsp_channel, 0xbeef74b0, 0x74b0,
         if (!ret)
      ret = nouveau_object_new(dec->vp_channel, 0xbeef7476, 0x7476,
         if (ret)
               if (is_h264) {
      /* Zero out some parts of mbring/vpring. there's gotta be some cleaner way
   * of doing this... perhaps makes sense to just copy the relevant logic
   * here. */
   color.f[0] = color.f[1] = color.f[2] = color.f[3] = 0;
   surf.offset = dec->frame_size;
   surf.width = 64;
   surf.height = (templ->max_references + 1) * dec->frame_mbs / 4;
   surf.depth = 1;
   surf.base.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   surf.base.u.tex.level = 0;
   surf.base.texture = &mip.base.base;
   mip.level[0].tile_mode = 0;
   mip.level[0].pitch = surf.width * 4;
   mip.base.domain = NOUVEAU_BO_VRAM;
   mip.base.bo = dec->mbring;
   mip.base.address = dec->mbring->offset;
   context->clear_render_target(context, &surf.base, &color, 0, 0, 64, 4760, false);
   surf.offset = dec->vpring->size / 2 - 0x1000;
   surf.width = 1024;
   surf.height = 1;
   mip.level[0].pitch = surf.width * 4;
   mip.base.bo = dec->vpring;
   mip.base.address = dec->vpring->offset;
   context->clear_render_target(context, &surf.base, &color, 0, 0, 1024, 1, false);
   surf.offset = dec->vpring->size - 0x1000;
            PUSH_SPACE(nv50->base.pushbuf, 5);
   PUSH_REF1(nv50->base.pushbuf, dec->fence, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   /* The clear_render_target is done via 3D engine, so use it to write to a
   * sempahore to indicate that it's done.
   */
   BEGIN_NV04(nv50->base.pushbuf, NV50_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(nv50->base.pushbuf, dec->fence->offset);
   PUSH_DATA (nv50->base.pushbuf, dec->fence->offset);
   PUSH_DATA (nv50->base.pushbuf, 1);
   PUSH_DATA (nv50->base.pushbuf, 0xf010);
                     BEGIN_NV04(bsp_push, SUBC_BSP(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NV04(bsp_push, SUBC_BSP(0x180), 11);
   for (i = 0; i < 11; i++)
         BEGIN_NV04(bsp_push, SUBC_BSP(0x1b8), 1);
            BEGIN_NV04(bsp_push, SUBC_BSP(0x600), 3);
   PUSH_DATAh(bsp_push, dec->bsp_fw->offset);
   PUSH_DATA (bsp_push, dec->bsp_fw->offset);
            BEGIN_NV04(bsp_push, SUBC_BSP(0x628), 2);
   PUSH_DATA (bsp_push, dec->bsp_data->offset >> 8);
   PUSH_DATA (bsp_push, dec->bsp_data->size);
                        BEGIN_NV04(vp_push, SUBC_VP(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NV04(vp_push, SUBC_VP(0x180), 11);
   for (i = 0; i < 11; i++)
            BEGIN_NV04(vp_push, SUBC_VP(0x1b8), 1);
            BEGIN_NV04(vp_push, SUBC_VP(0x600), 3);
   PUSH_DATAh(vp_push, dec->vp_fw->offset);
   PUSH_DATA (vp_push, dec->vp_fw->offset);
            BEGIN_NV04(vp_push, SUBC_VP(0x628), 2);
   PUSH_DATA (vp_push, dec->vp_data->offset >> 8);
   PUSH_DATA (vp_push, dec->vp_data->size);
               fail:
      nv84_decoder_destroy(&dec->base);
      }
      static void
   nv84_video_buffer_resources(struct pipe_video_buffer *buffer,
         {
      struct nv84_video_buffer *buf = (struct nv84_video_buffer *)buffer;
   unsigned num_planes = util_format_get_num_planes(buffer->buffer_format);
                     for (i = 0; i < num_planes; ++i) {
            }
      static struct pipe_sampler_view **
   nv84_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
   {
      struct nv84_video_buffer *buf = (struct nv84_video_buffer *)buffer;
      }
      static struct pipe_sampler_view **
   nv84_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
   {
      struct nv84_video_buffer *buf = (struct nv84_video_buffer *)buffer;
      }
      static struct pipe_surface **
   nv84_video_buffer_surfaces(struct pipe_video_buffer *buffer)
   {
      struct nv84_video_buffer *buf = (struct nv84_video_buffer *)buffer;
      }
      static void
   nv84_video_buffer_destroy(struct pipe_video_buffer *buffer)
   {
      struct nv84_video_buffer *buf = (struct nv84_video_buffer *)buffer;
                     for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      pipe_resource_reference(&buf->resources[i], NULL);
   pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);
   pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);
   pipe_surface_reference(&buf->surfaces[i * 2], NULL);
               nouveau_bo_ref(NULL, &buf->interlaced);
               }
      struct pipe_video_buffer *
   nv84_video_buffer_create(struct pipe_context *pipe,
         {
      struct nv84_video_buffer *buffer;
   struct pipe_resource templ;
   unsigned i, j, component;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface surf_templ;
   struct nv50_miptree *mt0, *mt1;
   struct nouveau_screen *screen = &((struct nv50_context *)pipe)->screen->base;
   union nouveau_bo_config cfg;
            if (template->buffer_format != PIPE_FORMAT_NV12)
            if (!template->interlaced) {
      debug_printf("Require interlaced video buffers\n");
      }
   if (pipe_format_to_chroma_format(template->buffer_format) != PIPE_VIDEO_CHROMA_FORMAT_420) {
      debug_printf("Must use 4:2:0 format\n");
               /*
   * Note that there are always going to be exactly two planes, one for Y,
   * and one for UV. These are also the resources. VP expects these to be
   * adjacent, so they need to belong to the same BO.
            buffer = CALLOC_STRUCT(nv84_video_buffer);
                     buffer->base.buffer_format = template->buffer_format;
   buffer->base.context = pipe;
   buffer->base.destroy = nv84_video_buffer_destroy;
   buffer->base.width = template->width;
   buffer->base.height = template->height;
   buffer->base.get_resources = nv84_video_buffer_resources;
   buffer->base.get_sampler_view_planes = nv84_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = nv84_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = nv84_video_buffer_surfaces;
            memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D_ARRAY;
   templ.depth0 = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = align(template->width, 2);
   templ.height0 = align(template->height, 4) / 2;
   templ.flags = NV50_RESOURCE_FLAG_VIDEO | NV50_RESOURCE_FLAG_NOALLOC;
            cfg.nv50.tile_mode = 0x20;
            buffer->resources[0] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[0])
            templ.format = PIPE_FORMAT_R8G8_UNORM;
   templ.width0 /= 2;
   templ.height0 /= 2;
   buffer->resources[1] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[1])
            mt0 = nv50_miptree(buffer->resources[0]);
            bo_size = mt0->total_size + mt1->total_size;
   if (nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP, 0,
               /* XXX Change reference frame management so that this is only allocated in
   * the decoder when necessary. */
   if (nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP, 0,
                  nouveau_bo_ref(buffer->interlaced, &mt0->base.bo);
   mt0->base.domain = NOUVEAU_BO_VRAM;
            nouveau_bo_ref(buffer->interlaced, &mt1->base.bo);
   mt1->base.domain = NOUVEAU_BO_VRAM;
   mt1->base.offset = mt0->total_size;
            memset(&sv_templ, 0, sizeof(sv_templ));
   for (component = 0, i = 0; i < 2; ++i ) {
      struct pipe_resource *res = buffer->resources[i];
            u_sampler_view_default_template(&sv_templ, res, res->format);
   buffer->sampler_view_planes[i] =
         if (!buffer->sampler_view_planes[i])
            for (j = 0; j < nr_components; ++j, ++component) {
      sv_templ.swizzle_r = sv_templ.swizzle_g = sv_templ.swizzle_b =
                  buffer->sampler_view_components[component] =
         if (!buffer->sampler_view_components[component])
                  memset(&surf_templ, 0, sizeof(surf_templ));
   for (j = 0; j < 2; ++j) {
      surf_templ.format = buffer->resources[j]->format;
   surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = 0;
   buffer->surfaces[j * 2] =
         if (!buffer->surfaces[j * 2])
            surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = 1;
   buffer->surfaces[j * 2 + 1] =
         if (!buffer->surfaces[j * 2 + 1])
                     error:
      nv84_video_buffer_destroy(&buffer->base);
      }
      #define FIRMWARE_BSP_KERN  0x01
   #define FIRMWARE_VP_KERN   0x02
   #define FIRMWARE_BSP_H264  0x04
   #define FIRMWARE_VP_MPEG2  0x08
   #define FIRMWARE_VP_H264_1 0x10
   #define FIRMWARE_VP_H264_2 0x20
   #define FIRMWARE_PRESENT(val, fw) (val & FIRMWARE_ ## fw)
      static int
   firmware_present(struct pipe_screen *pscreen, enum pipe_video_format codec)
   {
      struct nouveau_screen *screen = nouveau_screen(pscreen);
   struct nouveau_object *obj = NULL;
   struct stat s;
   int checked = screen->firmware_info.profiles_checked;
            if (!FIRMWARE_PRESENT(checked, VP_KERN)) {
      ret = nouveau_object_new(screen->channel, 0, 0x7476, NULL, 0, &obj);
   if (!ret)
         nouveau_object_del(&obj);
               if (codec == PIPE_VIDEO_FORMAT_MPEG4_AVC) {
      if (!FIRMWARE_PRESENT(checked, BSP_KERN)) {
      ret = nouveau_object_new(screen->channel, 0, 0x74b0, NULL, 0, &obj);
   if (!ret)
         nouveau_object_del(&obj);
               if (!FIRMWARE_PRESENT(checked, VP_H264_1)) {
      ret = stat("/lib/firmware/nouveau/nv84_vp-h264-1", &s);
   if (!ret && s.st_size > 1000)
                              present = screen->firmware_info.profiles_present;
   return FIRMWARE_PRESENT(present, VP_KERN) &&
      FIRMWARE_PRESENT(present, BSP_KERN) &&
   } else {
      if (!FIRMWARE_PRESENT(checked, VP_MPEG2)) {
      ret = stat("/lib/firmware/nouveau/nv84_vp-mpeg12", &s);
   if (!ret && s.st_size > 1000)
            }
   present = screen->firmware_info.profiles_present;
   return FIRMWARE_PRESENT(present, VP_KERN) &&
         }
      int
   nv84_screen_get_video_param(struct pipe_screen *pscreen,
                     {
               switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      codec = u_reduce_video_profile(profile);
   return (codec == PIPE_VIDEO_FORMAT_MPEG4_AVC ||
            case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
         case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
   case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_MAX_LEVEL:
      switch (profile) {
   case PIPE_VIDEO_PROFILE_MPEG1:
         case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
   case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
         default:
      debug_printf("unknown video profile: %d\n", profile);
         case PIPE_VIDEO_CAP_MAX_MACROBLOCKS:
         default:
      debug_printf("unknown video param: %d\n", param);
         }
      bool
   nv84_screen_video_supported(struct pipe_screen *screen,
                     {
      if (profile != PIPE_VIDEO_PROFILE_UNKNOWN)
               }
