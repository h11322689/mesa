   /*
   * Copyright © 2022 Collabora Ltd
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "vk_meta_private.h"
      #include "vk_command_buffer.h"
   #include "vk_command_pool.h"
   #include "vk_device.h"
      #include "nir_builder.h"
      const VkPipelineVertexInputStateCreateInfo vk_meta_draw_rects_vi_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 1,
   .pVertexBindingDescriptions = &(const VkVertexInputBindingDescription) {
      .binding = 0,
   .stride = 4 * sizeof(uint32_t),
      },
   .vertexAttributeDescriptionCount = 1,
   .pVertexAttributeDescriptions = &(const VkVertexInputAttributeDescription) {
      .location = 0,
   .binding = 0,
   .format = VK_FORMAT_R32G32B32A32_UINT,
         };
      const VkPipelineInputAssemblyStateCreateInfo vk_meta_draw_rects_ia_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
   .topology = VK_PRIMITIVE_TOPOLOGY_META_RECT_LIST_MESA,
      };
      const VkPipelineViewportStateCreateInfo vk_meta_draw_rects_vs_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
      };
      nir_shader *
   vk_meta_draw_rects_vs_nir(struct vk_meta_device *device, bool use_gs)
   {
      nir_builder build = nir_builder_init_simple_shader(MESA_SHADER_VERTEX, NULL,
                  nir_variable *in = nir_variable_create(b->shader, nir_var_shader_in,
                  nir_variable *pos =
      nir_variable_create(b->shader, nir_var_shader_out, glsl_vec4_type(),
               nir_variable *layer =
      nir_variable_create(b->shader, nir_var_shader_out, glsl_int_type(),
               nir_def *vtx = nir_load_var(b, in);
   nir_store_var(b, pos, nir_vec4(b, nir_channel(b, vtx, 0),
                              nir_store_var(b, layer, nir_iadd(b, nir_load_instance_id(b),
                     }
      nir_shader *
   vk_meta_draw_rects_gs_nir(struct vk_meta_device *device)
   {
      nir_builder build =
      nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY, NULL,
               nir_variable *pos_in =
      nir_variable_create(b->shader, nir_var_shader_in,
               nir_variable *layer_in =
      nir_variable_create(b->shader, nir_var_shader_in,
               nir_variable *pos_out =
      nir_variable_create(b->shader, nir_var_shader_out,
               nir_variable *layer_out =
      nir_variable_create(b->shader, nir_var_shader_out,
               for (unsigned i = 0; i < 3; i++) {
      nir_deref_instr *pos_in_deref =
         nir_deref_instr *layer_in_deref =
            nir_store_var(b, pos_out, nir_load_deref(b, pos_in_deref), 0xf);
   nir_store_var(b, layer_out, nir_load_deref(b, layer_in_deref), 1);
                        struct shader_info *info = &build.shader->info;
   info->gs.input_primitive = MESA_PRIM_TRIANGLES;
   info->gs.output_primitive = MESA_PRIM_TRIANGLE_STRIP;
   info->gs.vertices_in = 3;
   info->gs.vertices_out = 3;
   info->gs.invocations = 1;
               }
      struct vertex {
      float x, y, z;
      };
      static void
   setup_viewport_scissor(struct vk_command_buffer *cmd,
                     {
      const struct vk_device_dispatch_table *disp =
                  assert(rects[0].x0 < rects[0].x1 && rects[0].y0 < rects[0].y1);
   uint32_t xbits = rects[0].x1 - 1, ybits = rects[0].y1 - 1;
   float zmin = rects[0].z, zmax = rects[0].z;
   for (uint32_t r = 1; r < rect_count; r++) {
      assert(rects[r].x0 < rects[r].x1 && rects[r].y0 < rects[r].y1);
   xbits |= rects[r].x1 - 1;
   ybits |= rects[r].y1 - 1;
   zmin = fminf(zmin, rects[r].z);
               /* Annoyingly, we don't actually know the render area.  We assume that all
   * our rects are inside the render area.  We further assume the maximum
   * image and/or viewport size is a power of two.  This means we can round
   * up to a power of two without going outside any maximums.  Using a power
   * of two will ensure we don't lose precision when scaling coordinates.
   */
   int xmax_log2 = 1 + util_logbase2(xbits);
            assert(xmax_log2 >= 0 && xmax_log2 <= 31);
            /* We don't care about precise bounds on Z, only that it's inside [0, 1] if
   * the implementaiton only supports [0, 1].
   */
   if (zmin >= 0.0f && zmax <= 1.0f) {
      zmin = 0.0f;
               VkViewport viewport = {
      .x = 0,
   .y = 0,
   .width = ldexpf(1.0, xmax_log2),
   .height = ldexpf(1.0, ymax_log2),
   .minDepth = zmin,
      };
            VkRect2D scissor = {
      .offset = { 0, 0 },
      };
            /* Scaling factors */
   *x_scale = ldexpf(2.0, -xmax_log2);
      }
      static const uint32_t rect_vb_size_B = 6 * 4 * sizeof(float);
      static VkResult
   create_vertex_buffer(struct vk_command_buffer *cmd,
                        struct vk_meta_device *meta,
      {
               const VkBufferCreateInfo vtx_buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .size = rect_count * rect_vb_size_B,
   .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
   .queueFamilyIndexCount = 1,
               result = vk_meta_create_buffer(cmd, meta, &vtx_buffer_info, buffer_out);
   if (unlikely(result != VK_SUCCESS))
            void *map;
   result = meta->cmd_bind_map_buffer(cmd, meta, *buffer_out, &map);
   if (unlikely(result != VK_SUCCESS))
            for (uint32_t r = 0; r < rect_count; r++) {
      float x0 = rects[r].x0 * x_scale - 1.0f;
   float y0 = rects[r].y0 * y_scale - 1.0f;
   float x1 = rects[r].x1 * x_scale - 1.0f;
   float y1 = rects[r].y1 * y_scale - 1.0f;
   float z = rects[r].z;
            struct vertex rect_vb_data[6] = {
      { x0, y1, z, w },
                  { x1, y0, z, w },
   { x1, y1, z, w },
      };
   assert(sizeof(rect_vb_data) == rect_vb_size_B);
                  }
      void
   vk_meta_draw_volume(struct vk_command_buffer *cmd,
                     {
      const struct vk_device_dispatch_table *disp =
                  float x_scale, y_scale;
            VkBuffer vtx_buffer;
   VkResult result = create_vertex_buffer(cmd, meta, x_scale, y_scale,
         if (unlikely(result != VK_SUCCESS)) {
      /* TODO: Report error */
               const VkDeviceSize zero = 0;
               }
      void
   vk_meta_draw_rects(struct vk_command_buffer *cmd,
                     {
      const struct vk_device_dispatch_table *disp =
                  /* Two triangles with VK_FORMAT_R16G16_UINT */
   const uint32_t rect_vb_size_B = 6 * 3 * sizeof(float);
   const uint32_t rects_per_draw =
            if (rect_count == 0)
            float x_scale, y_scale;
            uint32_t next_rect = 0;
   while (next_rect < rect_count) {
               VkBuffer vtx_buffer;
   VkResult result = create_vertex_buffer(cmd, meta, x_scale, y_scale,
               if (unlikely(result != VK_SUCCESS)) {
      /* TODO: Report error */
               const VkDeviceSize zero = 0;
                        }
      }
