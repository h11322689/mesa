   /**************************************************************************
   *
   * Copyright 2019 Advanced Micro Devices, Inc.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: James Zhu <james.zhu<@amd.com>
   *
   **************************************************************************/
      #include <assert.h>
      #include "tgsi/tgsi_text.h"
   #include "vl_compositor_cs.h"
      struct cs_viewport {
      float scale_x;
   float scale_y;
   struct u_rect area;
   int crop_x; /* src */
   int crop_y;
   int translate_x; /* dst */
   int translate_y;
   float sampler0_w;
   float sampler0_h;
   float clamp_x;
   float clamp_y;
   float chroma_clamp_x;
   float chroma_clamp_y;
   float chroma_offset_x;
      };
      const char *compute_shader_video_buffer =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..8]\n"
   "DCL SVIEW[0..2], RECT, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1].xxxx\n"
      /* Translate */
                                 /* Chroma offset + subsampling */
                  /* Scale */
                  /* Clamp coords */
                  /* Fetch texels */
   "TEX_LZ TEMP[4].x, TEMP[2].xyyy, SAMP[0], RECT\n"
                           /* Color Space Conversion */
   "DP4 TEMP[7].x, CONST[0], TEMP[4]\n"
                  "MOV TEMP[5].w, TEMP[4].zzzz\n"
                                          const char *compute_shader_weave =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..8]\n"
   "DCL SVIEW[0..2], 2D_ARRAY, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
   "IMM[1] FLT32 { 1.0, 2.0, 0.0, 0.0}\n"
   "IMM[2] UINT32 { 1, 2, 4, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1].xxxx\n"
      "MOV TEMP[2].xy, TEMP[0].xyyy\n"
                  /* Top Y */
   "U2F TEMP[2], TEMP[2]\n"
                  /* Top UV */
   "MOV TEMP[3], TEMP[2]\n"
   /* Chroma offset */
   "ADD TEMP[3].xy, TEMP[3].xyyy, CONST[8].xyxy\n"
   "DIV TEMP[3].xy, TEMP[3].xyyy, IMM[1].yyyy\n"
                  /* Texture offset */
                                 /* Scale */
   "DIV TEMP[2].xy, TEMP[2].xyyy, CONST[3].zwzw\n"
   "DIV TEMP[12].xy, TEMP[12].xyyy, CONST[3].zwzw\n"
                  /* Weave offset */
   "ADD TEMP[2].y, TEMP[2].yyyy, IMM[3].xxxx\n"
   "ADD TEMP[12].y, TEMP[12].yyyy, -IMM[3].xxxx\n"
                  /* Texture layer */
   "MOV TEMP[14].x, TEMP[2].yyyy\n"
   "MOV TEMP[14].yz, TEMP[3].yyyy\n"
   "ROUND TEMP[15].xyz, TEMP[14].xyzz\n"
   "ADD TEMP[14].xyz, TEMP[14].xyzz, -TEMP[15].xyzz\n"
                  /* Clamp coords */
   "MIN TEMP[2].xy, TEMP[2].xyyy, CONST[7].xyxy\n"
   "MIN TEMP[12].xy, TEMP[12].xyyy, CONST[7].xyxy\n"
                  /* Normalize */
   "DIV TEMP[2].xy, TEMP[2].xyyy, CONST[5].zwzw\n"
   "DIV TEMP[12].xy, TEMP[12].xyyy, CONST[5].zwzw\n"
   "DIV TEMP[15].xy, CONST[5].zwzw, IMM[1].yyyy\n"
                  /* Fetch texels */
   "MOV TEMP[2].z, IMM[1].wwww\n"
   "MOV TEMP[3].z, IMM[1].wwww\n"
   "TEX_LZ TEMP[10].x, TEMP[2].xyzz, SAMP[0], 2D_ARRAY\n"
                  "MOV TEMP[12].z, IMM[1].xxxx\n"
   "MOV TEMP[13].z, IMM[1].xxxx\n"
   "TEX_LZ TEMP[11].x, TEMP[12].xyzz, SAMP[0], 2D_ARRAY\n"
                                 /* Color Space Conversion */
   "DP4 TEMP[9].x, CONST[0], TEMP[6]\n"
                  "MOV TEMP[7].w, TEMP[6].zzzz\n"
                                          const char *compute_shader_rgba =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..5]\n"
   "DCL SVIEW[0], RECT, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1].xxxx\n"
      /* Translate */
                                                               static const char *compute_shader_yuv_weave_y =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..7]\n"
   "DCL SVIEW[0..2], 2D_ARRAY, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
   "IMM[1] FLT32 { 1.0, 2.0, 0.0, 0.0}\n"
   "IMM[2] UINT32 { 1, 2, 4, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1]\n"
      "MOV TEMP[2], TEMP[0]\n"
                  /* Top Y */
   "U2F TEMP[2], TEMP[2]\n"
                  /* Top UV */
   "MOV TEMP[3], TEMP[2]\n"
   "DIV TEMP[3].xy, TEMP[3], IMM[1].yyyy\n"
                  /* Texture offset */
                                 /* Scale */
   "DIV TEMP[2].xy, TEMP[2], CONST[3].zwzw\n"
   "DIV TEMP[12].xy, TEMP[12], CONST[3].zwzw\n"
                  /* Weave offset */
   "ADD TEMP[2].y, TEMP[2].yyyy, IMM[3].xxxx\n"
   "ADD TEMP[12].y, TEMP[12].yyyy, -IMM[3].xxxx\n"
                  /* Texture layer */
   "MOV TEMP[14].x, TEMP[2].yyyy\n"
   "MOV TEMP[14].yz, TEMP[3].yyyy\n"
   "ROUND TEMP[15], TEMP[14]\n"
   "ADD TEMP[14], TEMP[14], -TEMP[15]\n"
                  /* Clamp coords */
   "MIN TEMP[2].xy, TEMP[2].xyyy, CONST[7].xyxy\n"
   "MIN TEMP[12].xy, TEMP[12].xyyy, CONST[7].xyxy\n"
                  /* Normalize */
   "DIV TEMP[2].xy, TEMP[2], CONST[5].zwzw\n"
   "DIV TEMP[12].xy, TEMP[12], CONST[5].zwzw\n"
   "DIV TEMP[15].xy, CONST[5].zwzw, IMM[1].yyyy\n"
                  /* Fetch texels */
   "MOV TEMP[2].z, IMM[1].wwww\n"
   "MOV TEMP[3].z, IMM[1].wwww\n"
   "TEX_LZ TEMP[10].x, TEMP[2], SAMP[0], 2D_ARRAY\n"
                  "MOV TEMP[12].z, IMM[1].xxxx\n"
   "MOV TEMP[13].z, IMM[1].xxxx\n"
   "TEX_LZ TEMP[11].x, TEMP[12], SAMP[0], 2D_ARRAY\n"
                                                static const char *compute_shader_yuv_weave_uv =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..7]\n"
   "DCL SVIEW[0..2], 2D_ARRAY, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
   "IMM[1] FLT32 { 1.0, 2.0, 0.0, 0.0}\n"
   "IMM[2] UINT32 { 1, 2, 4, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1]\n"
      "MOV TEMP[2], TEMP[0]\n"
                  /* Top Y */
   "U2F TEMP[2], TEMP[2]\n"
                  /* Top UV */
   "MOV TEMP[3], TEMP[2]\n"
   "DIV TEMP[3].xy, TEMP[3], IMM[1].yyyy\n"
                  /* Texture offset */
                                 /* Scale */
   "DIV TEMP[2].xy, TEMP[2], CONST[3].zwzw\n"
   "DIV TEMP[12].xy, TEMP[12], CONST[3].zwzw\n"
                  /* Weave offset */
   "ADD TEMP[2].y, TEMP[2].yyyy, IMM[3].xxxx\n"
   "ADD TEMP[12].y, TEMP[12].yyyy, -IMM[3].xxxx\n"
                  /* Texture layer */
   "MOV TEMP[14].x, TEMP[2].yyyy\n"
   "MOV TEMP[14].yz, TEMP[3].yyyy\n"
   "ROUND TEMP[15], TEMP[14]\n"
   "ADD TEMP[14], TEMP[14], -TEMP[15]\n"
                  /* Clamp coords */
   "MIN TEMP[2].xy, TEMP[2].xyyy, CONST[7].xyxy\n"
   "MIN TEMP[12].xy, TEMP[12].xyyy, CONST[7].xyxy\n"
                  /* Normalize */
   "DIV TEMP[2].xy, TEMP[2], CONST[5].zwzw\n"
   "DIV TEMP[12].xy, TEMP[12], CONST[5].zwzw\n"
   "DIV TEMP[15].xy, CONST[5].zwzw, IMM[1].yyyy\n"
                  /* Fetch texels */
   "MOV TEMP[2].z, IMM[1].wwww\n"
   "MOV TEMP[3].z, IMM[1].wwww\n"
   "TEX_LZ TEMP[10].x, TEMP[2], SAMP[0], 2D_ARRAY\n"
                  "MOV TEMP[12].z, IMM[1].xxxx\n"
   "MOV TEMP[13].z, IMM[1].xxxx\n"
   "TEX_LZ TEMP[11].x, TEMP[12], SAMP[0], 2D_ARRAY\n"
                                                         static const char *compute_shader_yuv_y =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..7]\n"
   "DCL SVIEW[0..2], RECT, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1]\n"
               /* Translate */
                  /* Texture offset */
                                 /* Crop */
   "MOV TEMP[4].xy, CONST[6].zwww\n"
                                                                        static const char *compute_shader_yuv_uv =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..7]\n"
   "DCL SVIEW[0..2], RECT, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1]\n"
               /* Translate */
                  /* Texture offset */
                                          /* Crop */
   "MOV TEMP[4].xy, CONST[6].zwww\n"
                                 /* Fetch texels */
                                                   static const char *compute_shader_rgb_yuv_y =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..7]\n"
   "DCL SVIEW[0], RECT, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1]\n"
      /* Translate */
                                                /* Crop */
   "MOV TEMP[4].xy, CONST[6].zwww\n"
                                                                                                static const char *compute_shader_rgb_yuv_uv =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 8\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 8\n"
            "DCL SV[0], THREAD_ID\n"
            "DCL CONST[0..8]\n"
   "DCL SVIEW[0], RECT, FLOAT\n"
            "DCL IMAGE[0], 2D, WR\n"
            "IMM[0] UINT32 { 8, 8, 1, 0}\n"
                     /* Drawn area check */
   "USGE TEMP[1].xy, TEMP[0].xyxy, CONST[4].xyxy\n"
   "USLT TEMP[1].zw, TEMP[0].xyxy, CONST[4].zwzw\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].yyyy\n"
   "AND TEMP[1].x, TEMP[1].xxxx, TEMP[1].zzzz\n"
            "UIF TEMP[1]\n"
      /* Translate */
                                                /* Sample offset */
   "ADD TEMP[3].xy, TEMP[2].xyyy, IMM[1].yyyy\n"
   "ADD TEMP[6].xy, TEMP[2].xyyy, IMM[1].wwww\n"
                  /* Scale */
   "DIV TEMP[2], TEMP[2], CONST[3].zwzw\n"
   "DIV TEMP[3], TEMP[3], CONST[3].zwzw\n"
                  /* Crop */
   "MOV TEMP[4].xy, CONST[6].zwww\n"
   "I2F TEMP[4], TEMP[4]\n"
   "ADD TEMP[2], TEMP[2], TEMP[4]\n"
   "ADD TEMP[3], TEMP[3], TEMP[4]\n"
                  /* Clamp coords */
   "MIN TEMP[2].xy, TEMP[2].xyyy, CONST[7].zwzw\n"
   "MIN TEMP[3].xy, TEMP[3].xyyy, CONST[7].zwzw\n"
                  /* Fetch texels */
   "TEX_LZ TEMP[4].xyz, TEMP[2], SAMP[0], RECT\n"
   "TEX_LZ TEMP[5].xyz, TEMP[3], SAMP[0], RECT\n"
                  "ADD TEMP[4].xyz, TEMP[4].xyzz, TEMP[5].xyzz\n"
   "ADD TEMP[4].xyz, TEMP[4].xyzz, TEMP[8].xyzz\n"
                           /* Color Space Conversion */
                                          static void
   cs_launch(struct vl_compositor *c,
               {
               /* Bind the image */
   struct pipe_image_view image = {0};
   image.resource = c->fb_state.cbufs[0]->texture;
   image.shader_access = image.access = PIPE_IMAGE_ACCESS_READ_WRITE;
                     /* Bind compute shader */
            /* Dispatch compute */
   struct pipe_grid_info info = {0};
   info.block[0] = 8;
   info.block[1] = 8;
   info.block[2] = 1;
   info.grid[0] = DIV_ROUND_UP(draw_area->x1, info.block[0]);
   info.grid[1] = DIV_ROUND_UP(draw_area->y1, info.block[1]);
                     /* Make the result visible to all clients. */
         }
      static inline struct u_rect
   calc_drawn_area(struct vl_compositor_state *s,
         {
      struct vertex2f tl, br;
                     tl = layer->dst.tl;
            /* Scale */
   result.x0 = tl.x * layer->viewport.scale[0] + layer->viewport.translate[0];
   result.y0 = tl.y * layer->viewport.scale[1] + layer->viewport.translate[1];
   result.x1 = br.x * layer->viewport.scale[0] + layer->viewport.translate[0];
            /* Clip */
   result.x0 = MAX2(result.x0, s->scissor.minx);
   result.y0 = MAX2(result.y0, s->scissor.miny);
   result.x1 = MIN2(result.x1, s->scissor.maxx);
   result.y1 = MIN2(result.y1, s->scissor.maxy);
      }
      static inline float
   chroma_offset_x(unsigned location)
   {
      if (location & VL_COMPOSITOR_LOCATION_HORIZONTAL_LEFT)
         else
      }
      static inline float
   chroma_offset_y(unsigned location)
   {
      if (location & VL_COMPOSITOR_LOCATION_VERTICAL_TOP)
         else if (location & VL_COMPOSITOR_LOCATION_VERTICAL_BOTTOM)
         else
      }
      static bool
   set_viewport(struct vl_compositor_state *s,
               {
                        void *ptr = pipe_buffer_map(s->pipe, s->shader_params,
                  if (!ptr)
                     float *ptr_float = (float *)ptr;
   ptr_float += sizeof(vl_csc_matrix) / sizeof(float);
   *ptr_float++ = s->luma_min;
   *ptr_float++ = s->luma_max;
   *ptr_float++ = drawn->scale_x;
            int *ptr_int = (int *)ptr_float;
   *ptr_int++ = drawn->area.x0;
   *ptr_int++ = drawn->area.y0;
   *ptr_int++ = drawn->area.x1;
   *ptr_int++ = drawn->area.y1;
   *ptr_int++ = drawn->translate_x;
            ptr_float = (float *)ptr_int;
   *ptr_float++ = drawn->sampler0_w;
            /* compute_shader_video_buffer uses pixel coordinates based on the
   * Y sampler dimensions. If U/V are using separate planes and are
   * subsampled, we need to scale the coordinates */
   if (samplers[1]) {
      float h_ratio = samplers[1]->texture->width0 /
         *ptr_float++ = h_ratio;
   float v_ratio = samplers[1]->texture->height0 /
            }
   else {
      ptr_float++;
      }
   ptr_int = (int *)ptr_float;
   *ptr_int++ = drawn->crop_x;
            ptr_float = (float *)ptr_int;
   *ptr_float++ = drawn->clamp_x;
   *ptr_float++ = drawn->clamp_y;
   *ptr_float++ = drawn->chroma_clamp_x;
   *ptr_float++ = drawn->chroma_clamp_y;
   *ptr_float++ = drawn->chroma_offset_x;
                        }
      static void
   draw_layers(struct vl_compositor       *c,
               {
                        for (i = 0; i < VL_COMPOSITOR_MAX_LAYERS; ++i) {
      if (s->used_layers & (1 << i)) {
      struct vl_compositor_layer *layer = &s->layers[i];
   struct pipe_sampler_view **samplers = &layer->sampler_views[0];
   unsigned num_sampler_views = !samplers[1] ? 1 : !samplers[2] ? 2 : 3;
                  drawn.area = calc_drawn_area(s, layer);
   drawn.scale_x = layer->viewport.scale[0] /
      ((float)layer->sampler_views[0]->texture->width0 *
      drawn.scale_y  = layer->viewport.scale[1] /
      ((float)layer->sampler_views[0]->texture->height0 *
      drawn.crop_x = (int)(layer->src.tl.x * layer->sampler_views[0]->texture->width0);
   drawn.translate_x = layer->viewport.translate[0];
   drawn.crop_y = (int)(layer->src.tl.y * layer->sampler_views[0]->texture->height0);
   drawn.translate_y = layer->viewport.translate[1];
   drawn.sampler0_w = (float)layer->sampler_views[0]->texture->width0;
   drawn.sampler0_h = (float)layer->sampler_views[0]->texture->height0;
   drawn.clamp_x = (float)samplers[0]->texture->width0 * layer->src.br.x - 0.5;
   drawn.clamp_y = (float)samplers[0]->texture->height0 * layer->src.br.y - 0.5;
   drawn.chroma_clamp_x = (float)sampler1->texture->width0 * layer->src.br.x - 0.5;
   drawn.chroma_clamp_y = (float)sampler1->texture->height0 * layer->src.br.y - 0.5;
   drawn.chroma_offset_x = chroma_offset_x(s->chroma_location);
                  c->pipe->bind_sampler_states(c->pipe, PIPE_SHADER_COMPUTE, 0,
                                 /* Unbind. */
   c->pipe->set_shader_images(c->pipe, PIPE_SHADER_COMPUTE, 0, 0, 1, NULL);
   c->pipe->set_constant_buffer(c->pipe, PIPE_SHADER_COMPUTE, 0, false, NULL);
   c->pipe->set_sampler_views(c->pipe, PIPE_SHADER_FRAGMENT, 0, 0,
         c->pipe->bind_compute_state(c->pipe, NULL);
                  if (dirty) {
      struct u_rect drawn = calc_drawn_area(s, layer);
   dirty->x0 = MIN2(drawn.x0, dirty->x0);
   dirty->y0 = MIN2(drawn.y0, dirty->y0);
   dirty->x1 = MAX2(drawn.x1, dirty->x1);
               }
      void *
   vl_compositor_cs_create_shader(struct vl_compositor *c,
         {
               struct tgsi_token tokens[1024];
   if (!tgsi_text_translate(compute_shader_text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
               struct pipe_compute_state state = {0};
   state.ir_type = PIPE_SHADER_IR_TGSI;
            /* create compute shader */
      }
      void
   vl_compositor_cs_render(struct vl_compositor_state *s,
                           {
      assert(c && s);
            c->fb_state.width = dst_surface->width;
   c->fb_state.height = dst_surface->height;
            if (!s->scissor_valid) {
      s->scissor.minx = 0;
   s->scissor.miny = 0;
   s->scissor.maxx = dst_surface->width;
               if (clear_dirty && dirty_area &&
               c->pipe->clear_render_target(c->pipe, dst_surface, &s->clear_color,
         dirty_area->x0 = dirty_area->y0 = VL_COMPOSITOR_MAX_DIRTY;
                           }
      bool vl_compositor_cs_init_shaders(struct vl_compositor *c)
   {
                  c->cs_video_buffer = vl_compositor_cs_create_shader(c, compute_shader_video_buffer);
   if (!c->cs_video_buffer) {
                        c->cs_weave_rgb = vl_compositor_cs_create_shader(c, compute_shader_weave);
   if (!c->cs_weave_rgb) {
                        c->cs_yuv.weave.y = vl_compositor_cs_create_shader(c, compute_shader_yuv_weave_y);
   c->cs_yuv.weave.uv = vl_compositor_cs_create_shader(c, compute_shader_yuv_weave_uv);
   c->cs_yuv.progressive.y = vl_compositor_cs_create_shader(c, compute_shader_yuv_y);
   c->cs_yuv.progressive.uv = vl_compositor_cs_create_shader(c, compute_shader_yuv_uv);
   if (!c->cs_yuv.weave.y || !c->cs_yuv.weave.uv) {
               }
   if (!c->cs_yuv.progressive.y || !c->cs_yuv.progressive.uv) {
                        c->cs_rgb_yuv.y = vl_compositor_cs_create_shader(c, compute_shader_rgb_yuv_y);
   c->cs_rgb_yuv.uv = vl_compositor_cs_create_shader(c, compute_shader_rgb_yuv_uv);
   if (!c->cs_rgb_yuv.y || !c->cs_rgb_yuv.uv) {
                        }
      void vl_compositor_cs_cleanup_shaders(struct vl_compositor *c)
   {
                  if (c->cs_video_buffer)
         if (c->cs_weave_rgb)
         if (c->cs_yuv.weave.y)
         if (c->cs_yuv.weave.uv)
         if (c->cs_yuv.progressive.y)
         if (c->cs_yuv.progressive.uv)
         if (c->cs_rgb_yuv.y)
         if (c->cs_rgb_yuv.uv)
   }
