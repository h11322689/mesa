   /*
   * Copyright © 2019 Intel Corporation
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
      #include <string.h>
   #include <stdlib.h>
   #include <assert.h>
      #include <vulkan/vulkan_core.h>
   #include <vulkan/vk_layer.h>
      #include "git_sha1.h"
      #include "imgui.h"
      #include "overlay_params.h"
      #include "util/u_debug.h"
   #include "util/hash_table.h"
   #include "util/list.h"
   #include "util/ralloc.h"
   #include "util/os_time.h"
   #include "util/os_socket.h"
   #include "util/simple_mtx.h"
   #include "util/u_math.h"
      #include "vk_enum_to_str.h"
   #include "vk_dispatch_table.h"
   #include "vk_util.h"
      /* Mapped from VkInstace/VkPhysicalDevice */
   struct instance_data {
      struct vk_instance_dispatch_table vtable;
   struct vk_physical_device_dispatch_table pd_vtable;
            struct overlay_params params;
                              /* Dumping of frame stats to a file has been enabled. */
            /* Dumping of frame stats to a file has been enabled and started. */
      };
      struct frame_stat {
         };
      /* Mapped from VkDevice */
   struct queue_data;
   struct device_data {
                        struct vk_device_dispatch_table vtable;
   VkPhysicalDevice physical_device;
                              struct queue_data **queues;
                     /* For a single frame */
      };
      /* Mapped from VkCommandBuffer */
   struct command_buffer_data {
                        VkCommandBuffer cmd_buffer;
   VkQueryPool pipeline_query_pool;
   VkQueryPool timestamp_query_pool;
                        };
      /* Mapped from VkQueue */
   struct queue_data {
               VkQueue queue;
   VkQueueFlags flags;
   uint32_t family_index;
                        };
      struct overlay_draw {
                                 VkSemaphore semaphore;
            VkBuffer vertex_buffer;
   VkDeviceMemory vertex_buffer_mem;
            VkBuffer index_buffer;
   VkDeviceMemory index_buffer_mem;
      };
      /* Mapped from VkSwapchainKHR */
   struct swapchain_data {
               VkSwapchainKHR swapchain;
   unsigned width, height;
            uint32_t n_images;
   VkImage *images;
   VkImageView *image_views;
                     VkDescriptorPool descriptor_pool;
   VkDescriptorSetLayout descriptor_layout;
                     VkPipelineLayout pipeline_layout;
                              bool font_uploaded;
   VkImage font_image;
   VkImageView font_image_view;
   VkDeviceMemory font_mem;
   VkBuffer upload_font_buffer;
            /**/
   ImGuiContext* imgui_context;
            /**/
   uint64_t n_frames;
            unsigned n_frames_since_update;
   uint64_t last_fps_update;
            enum overlay_param_enabled stat_selector;
   double time_dividor;
   struct frame_stat stats_min, stats_max;
            /* Over a single frame */
            /* Over fps_sampling_period */
      };
      static const VkQueryPipelineStatisticFlags overlay_query_flags =
      VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
   VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
   VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
   VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
   VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
   VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
   VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
   VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
   VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
   VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT |
      #define OVERLAY_QUERY_COUNT (11)
      static struct hash_table_u64 *vk_object_to_data = NULL;
   static simple_mtx_t vk_object_to_data_mutex = SIMPLE_MTX_INITIALIZER;
      thread_local ImGuiContext* __MesaImGui;
      static inline void ensure_vk_object_map(void)
   {
      if (!vk_object_to_data)
      }
      #define HKEY(obj) ((uint64_t)(obj))
   #define FIND(type, obj) ((type *)find_object_data(HKEY(obj)))
      static void *find_object_data(uint64_t obj)
   {
      simple_mtx_lock(&vk_object_to_data_mutex);
   ensure_vk_object_map();
   void *data = _mesa_hash_table_u64_search(vk_object_to_data, obj);
   simple_mtx_unlock(&vk_object_to_data_mutex);
      }
      static void map_object(uint64_t obj, void *data)
   {
      simple_mtx_lock(&vk_object_to_data_mutex);
   ensure_vk_object_map();
   _mesa_hash_table_u64_insert(vk_object_to_data, obj, data);
      }
      static void unmap_object(uint64_t obj)
   {
      simple_mtx_lock(&vk_object_to_data_mutex);
   _mesa_hash_table_u64_remove(vk_object_to_data, obj);
      }
      /**/
      #define VK_CHECK(expr) \
      do { \
      VkResult __result = (expr); \
   if (__result != VK_SUCCESS) { \
      fprintf(stderr, "'%s' line %i failed with %s\n", \
               /**/
      static VkLayerInstanceCreateInfo *get_instance_chain_info(const VkInstanceCreateInfo *pCreateInfo,
         {
      vk_foreach_struct_const(item, pCreateInfo->pNext) {
      if (item->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
      ((VkLayerInstanceCreateInfo *) item)->function == func)
   }
   unreachable("instance chain info not found");
      }
      static VkLayerDeviceCreateInfo *get_device_chain_info(const VkDeviceCreateInfo *pCreateInfo,
         {
      vk_foreach_struct_const(item, pCreateInfo->pNext) {
      if (item->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
      ((VkLayerDeviceCreateInfo *) item)->function == func)
   }
   unreachable("device chain info not found");
      }
      static void
   free_chain(struct VkBaseOutStructure *chain)
   {
      while (chain) {
      void *node = chain;
   chain = chain->pNext;
         }
      static struct VkBaseOutStructure *
   clone_chain(const struct VkBaseInStructure *chain)
   {
               vk_foreach_struct_const(item, chain) {
      size_t item_size = vk_structure_type_size(item);
   if (item_size == 0) {
      free_chain(head);
               struct VkBaseOutStructure *new_item =
                     if (!head)
         if (tail)
                        }
      /**/
      static struct instance_data *new_instance_data(VkInstance instance)
   {
      struct instance_data *data = rzalloc(NULL, struct instance_data);
   data->instance = instance;
   data->control_client = -1;
   map_object(HKEY(data->instance), data);
      }
      static void destroy_instance_data(struct instance_data *data)
   {
      if (data->params.output_file)
         if (data->params.control >= 0)
         unmap_object(HKEY(data->instance));
      }
      static void instance_data_map_physical_devices(struct instance_data *instance_data,
         {
      uint32_t physicalDeviceCount = 0;
   instance_data->vtable.EnumeratePhysicalDevices(instance_data->instance,
                  VkPhysicalDevice *physicalDevices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
   instance_data->vtable.EnumeratePhysicalDevices(instance_data->instance,
                  for (uint32_t i = 0; i < physicalDeviceCount; i++) {
      if (map)
         else
                  }
      /**/
   static struct device_data *new_device_data(VkDevice device, struct instance_data *instance)
   {
      struct device_data *data = rzalloc(NULL, struct device_data);
   data->instance = instance;
   data->device = device;
   map_object(HKEY(data->device), data);
      }
      static struct queue_data *new_queue_data(VkQueue queue,
                     {
      struct queue_data *data = rzalloc(device_data, struct queue_data);
   data->device = device_data;
   data->queue = queue;
   data->flags = family_props->queueFlags;
   data->timestamp_mask = (1ull << family_props->timestampValidBits) - 1;
   data->family_index = family_index;
   list_inithead(&data->running_command_buffer);
            /* Fence synchronizing access to queries on that queue. */
   VkFenceCreateInfo fence_info = {};
   fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   VK_CHECK(device_data->vtable.CreateFence(device_data->device,
                        if (data->flags & VK_QUEUE_GRAPHICS_BIT)
               }
      static void destroy_queue(struct queue_data *data)
   {
      struct device_data *device_data = data->device;
   device_data->vtable.DestroyFence(device_data->device, data->queries_fence, NULL);
   unmap_object(HKEY(data->queue));
      }
      static void device_map_queues(struct device_data *data,
         {
      for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++)
                  struct instance_data *instance_data = data->instance;
   uint32_t n_family_props;
   instance_data->pd_vtable.GetPhysicalDeviceQueueFamilyProperties(data->physical_device,
               VkQueueFamilyProperties *family_props =
         instance_data->pd_vtable.GetPhysicalDeviceQueueFamilyProperties(data->physical_device,
                  uint32_t queue_index = 0;
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      for (uint32_t j = 0; j < pCreateInfo->pQueueCreateInfos[i].queueCount; j++) {
      VkQueue queue;
   data->vtable.GetDeviceQueue(data->device,
                           data->queues[queue_index++] =
      new_queue_data(queue, &family_props[pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex],
                  }
      static void device_unmap_queues(struct device_data *data)
   {
      for (uint32_t i = 0; i < data->n_queues; i++)
      }
      static void destroy_device_data(struct device_data *data)
   {
      unmap_object(HKEY(data->device));
      }
      /**/
   static struct command_buffer_data *new_command_buffer_data(VkCommandBuffer cmd_buffer,
                                 {
      struct command_buffer_data *data = rzalloc(NULL, struct command_buffer_data);
   data->device = device_data;
   data->cmd_buffer = cmd_buffer;
   data->level = level;
   data->pipeline_query_pool = pipeline_query_pool;
   data->timestamp_query_pool = timestamp_query_pool;
   data->query_index = query_index;
   list_inithead(&data->link);
   map_object(HKEY(data->cmd_buffer), data);
      }
      static void destroy_command_buffer_data(struct command_buffer_data *data)
   {
      unmap_object(HKEY(data->cmd_buffer));
   list_delinit(&data->link);
      }
      /**/
   static struct swapchain_data *new_swapchain_data(VkSwapchainKHR swapchain,
         {
      struct instance_data *instance_data = device_data->instance;
   struct swapchain_data *data = rzalloc(NULL, struct swapchain_data);
   data->device = device_data;
   data->swapchain = swapchain;
   data->window_size = ImVec2(instance_data->params.width, instance_data->params.height);
   list_inithead(&data->draws);
   map_object(HKEY(data->swapchain), data);
      }
      static void destroy_swapchain_data(struct swapchain_data *data)
   {
      unmap_object(HKEY(data->swapchain));
      }
      struct overlay_draw *get_overlay_draw(struct swapchain_data *data)
   {
      struct device_data *device_data = data->device;
   struct overlay_draw *draw = list_is_empty(&data->draws) ?
            VkSemaphoreCreateInfo sem_info = {};
            if (draw && device_data->vtable.GetFenceStatus(device_data->device, draw->fence) == VK_SUCCESS) {
      list_del(&draw->link);
   VK_CHECK(device_data->vtable.ResetFences(device_data->device,
         list_addtail(&draw->link, &data->draws);
                        VkCommandBufferAllocateInfo cmd_buffer_info = {};
   cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   cmd_buffer_info.commandPool = data->command_pool;
   cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   cmd_buffer_info.commandBufferCount = 1;
   VK_CHECK(device_data->vtable.AllocateCommandBuffers(device_data->device,
               VK_CHECK(device_data->set_device_loader_data(device_data->device,
               VkFenceCreateInfo fence_info = {};
   fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   VK_CHECK(device_data->vtable.CreateFence(device_data->device,
                        VK_CHECK(device_data->vtable.CreateSemaphore(device_data->device, &sem_info,
         VK_CHECK(device_data->vtable.CreateSemaphore(device_data->device, &sem_info,
                        }
      static const char *param_unit(enum overlay_param_enabled param)
   {
      switch (param) {
   case OVERLAY_PARAM_ENABLED_frame_timing:
   case OVERLAY_PARAM_ENABLED_acquire_timing:
   case OVERLAY_PARAM_ENABLED_present_timing:
         case OVERLAY_PARAM_ENABLED_gpu_timing:
         default:
            }
      static void parse_command(struct instance_data *instance_data,
               {
      if (!strncmp(cmd, "capture", cmdlen)) {
      int value = atoi(param);
            if (enabled) {
         } else {
      instance_data->capture_enabled = false;
            }
      #define BUFSIZE 4096
      /**
   * This function will process commands through the control file.
   *
   * A command starts with a colon, followed by the command, and followed by an
   * option '=' and a parameter.  It has to end with a semi-colon. A full command
   * + parameter looks like:
   *
   *    :cmd=param;
   */
   static void process_char(struct instance_data *instance_data, char c)
   {
      static char cmd[BUFSIZE];
            static unsigned cmdpos = 0;
   static unsigned parampos = 0;
   static bool reading_cmd = false;
            switch (c) {
   case ':':
      cmdpos = 0;
   parampos = 0;
   reading_cmd = true;
   reading_param = false;
      case ';':
      if (!reading_cmd)
         cmd[cmdpos++] = '\0';
   param[parampos++] = '\0';
   parse_command(instance_data, cmd, cmdpos, param, parampos);
   reading_cmd = false;
   reading_param = false;
      case '=':
      if (!reading_cmd)
         reading_param = true;
      default:
      if (!reading_cmd)
            if (reading_param) {
      /* overflow means an invalid parameter */
   if (parampos >= BUFSIZE - 1) {
      reading_cmd = false;
   reading_param = false;
                  } else {
      /* overflow means an invalid command */
   if (cmdpos >= BUFSIZE - 1) {
      reading_cmd = false;
                        }
      static void control_send(struct instance_data *instance_data,
               {
      unsigned msglen = 0;
                              memcpy(&buffer[msglen], cmd, cmdlen);
            if (paramlen > 0) {
      buffer[msglen++] = '=';
   memcpy(&buffer[msglen], param, paramlen);
   msglen += paramlen;
                  }
      static void control_send_connection_string(struct device_data *device_data)
   {
               const char *controlVersionCmd = "MesaOverlayControlVersion";
            control_send(instance_data, controlVersionCmd, strlen(controlVersionCmd),
            const char *deviceCmd = "DeviceName";
            control_send(instance_data, deviceCmd, strlen(deviceCmd),
            const char *mesaVersionCmd = "MesaVersion";
            control_send(instance_data, mesaVersionCmd, strlen(mesaVersionCmd),
      }
      static void control_client_check(struct device_data *device_data)
   {
               /* Already connected, just return. */
   if (instance_data->control_client >= 0)
            int socket = os_socket_accept(instance_data->params.control);
   if (socket == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK && errno != ECONNABORTED)
                     if (socket >= 0) {
      os_socket_block(socket, false);
   instance_data->control_client = socket;
         }
      static void control_client_disconnected(struct instance_data *instance_data)
   {
      os_socket_close(instance_data->control_client);
      }
      static void process_control_socket(struct instance_data *instance_data)
   {
      const int client = instance_data->control_client;
   if (client >= 0) {
               while (true) {
               if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                          } else if (n == 0) {
      /* recv() returns 0 when the client disconnects */
               for (ssize_t i = 0; i < n; i++) {
                  /* If we try to read BUFSIZE and receive BUFSIZE bytes from the
   * socket, there's a good chance that there's still more data to be
   * read, so we will try again. Otherwise, simply be done for this
   * iteration and try again on the next frame.
   */
   if (n < BUFSIZE)
            }
      static void snapshot_swapchain_frame(struct swapchain_data *data)
   {
      struct device_data *device_data = data->device;
   struct instance_data *instance_data = device_data->instance;
   uint32_t f_idx = data->n_frames % ARRAY_SIZE(data->frames_stats);
            if (instance_data->params.control >= 0) {
      control_client_check(device_data);
               if (data->last_present_time) {
      data->frame_stats.stats[OVERLAY_PARAM_ENABLED_frame_timing] =
               memset(&data->frames_stats[f_idx], 0, sizeof(data->frames_stats[f_idx]));
   for (int s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++) {
      data->frames_stats[f_idx].stats[s] += device_data->frame_stats.stats[s] + data->frame_stats.stats[s];
               /* If capture has been enabled but it hasn't started yet, it means we are on
   * the first snapshot after it has been enabled. At this point we want to
   * use the stats captured so far to update the display, but we don't want
   * this data to cause noise to the stats that we want to capture from now
   * on.
   *
   * capture_begin == true will trigger an update of the fps on display, and a
   * flush of the data, but no stats will be written to the output file. This
   * way, we will have only stats from after the capture has been enabled
   * written to the output_file.
   */
   const bool capture_begin =
            if (data->last_fps_update) {
      double elapsed = (double)(now - data->last_fps_update); /* us */
   if (capture_begin ||
      elapsed >= instance_data->params.fps_sampling_period) {
   data->fps = 1000000.0f * data->n_frames_since_update / elapsed;
   if (instance_data->capture_started) {
                  #define OVERLAY_PARAM_BOOL(name) \
                  if (instance_data->params.enabled[OVERLAY_PARAM_ENABLED_##name]) { \
      fprintf(instance_data->params.output_file, \
         #define OVERLAY_PARAM_CUSTOM(name)
         #undef OVERLAY_PARAM_BOOL
   #undef OVERLAY_PARAM_CUSTOM
                              for (int s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++) {
      if (!instance_data->params.enabled[s])
         if (s == OVERLAY_PARAM_ENABLED_fps) {
      fprintf(instance_data->params.output_file,
      } else {
      fprintf(instance_data->params.output_file,
               }
   fprintf(instance_data->params.output_file, "\n");
               memset(&data->accumulated_stats, 0, sizeof(data->accumulated_stats));
                  if (capture_begin)
         } else {
                  memset(&device_data->frame_stats, 0, sizeof(device_data->frame_stats));
            data->last_present_time = now;
   data->n_frames++;
      }
      static float get_time_stat(void *_data, int _idx)
   {
      struct swapchain_data *data = (struct swapchain_data *) _data;
   if ((ARRAY_SIZE(data->frames_stats) - _idx) > data->n_frames)
         int idx = ARRAY_SIZE(data->frames_stats) +
      data->n_frames < ARRAY_SIZE(data->frames_stats) ?
   _idx - data->n_frames :
      idx %= ARRAY_SIZE(data->frames_stats);
   /* Time stats are in us. */
      }
      static float get_stat(void *_data, int _idx)
   {
      struct swapchain_data *data = (struct swapchain_data *) _data;
   if ((ARRAY_SIZE(data->frames_stats) - _idx) > data->n_frames)
         int idx = ARRAY_SIZE(data->frames_stats) +
      data->n_frames < ARRAY_SIZE(data->frames_stats) ?
   _idx - data->n_frames :
      idx %= ARRAY_SIZE(data->frames_stats);
      }
      static void position_layer(struct swapchain_data *data)
      {
      struct device_data *device_data = data->device;
   struct instance_data *instance_data = device_data->instance;
            ImGui::SetNextWindowBgAlpha(0.5);
   ImGui::SetNextWindowSize(data->window_size, ImGuiCond_Always);
   switch (instance_data->params.position) {
   case LAYER_POSITION_TOP_LEFT:
      ImGui::SetNextWindowPos(ImVec2(margin, margin), ImGuiCond_Always);
      case LAYER_POSITION_TOP_RIGHT:
      ImGui::SetNextWindowPos(ImVec2(data->width - data->window_size.x - margin, margin),
            case LAYER_POSITION_BOTTOM_LEFT:
      ImGui::SetNextWindowPos(ImVec2(margin, data->height - data->window_size.y - margin),
            case LAYER_POSITION_BOTTOM_RIGHT:
      ImGui::SetNextWindowPos(ImVec2(data->width - data->window_size.x - margin,
                     }
      static void compute_swapchain_display(struct swapchain_data *data)
   {
      struct device_data *device_data = data->device;
            ImGui::SetCurrentContext(data->imgui_context);
   ImGui::NewFrame();
   position_layer(data);
   ImGui::Begin("Mesa overlay");
   if (instance_data->params.enabled[OVERLAY_PARAM_ENABLED_device])
            if (instance_data->params.enabled[OVERLAY_PARAM_ENABLED_format]) {
      const char *format_name = vk_Format_to_str(data->format);
   format_name = format_name ? (format_name + strlen("VK_FORMAT_")) : "unknown";
      }
   if (instance_data->params.enabled[OVERLAY_PARAM_ENABLED_frame])
         if (instance_data->params.enabled[OVERLAY_PARAM_ENABLED_fps])
            /* Recompute min/max */
   for (uint32_t s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++) {
      data->stats_min.stats[s] = UINT64_MAX;
      }
   for (uint32_t f = 0; f < MIN2(data->n_frames, ARRAY_SIZE(data->frames_stats)); f++) {
      for (uint32_t s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++) {
      data->stats_min.stats[s] = MIN2(data->frames_stats[f].stats[s],
         data->stats_max.stats[s] = MAX2(data->frames_stats[f].stats[s],
         }
   for (uint32_t s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++) {
                  for (uint32_t s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++) {
      if (!instance_data->params.enabled[s] ||
      s == OVERLAY_PARAM_ENABLED_device ||
   s == OVERLAY_PARAM_ENABLED_format ||
   s == OVERLAY_PARAM_ENABLED_fps ||
               char hash[40];
   snprintf(hash, sizeof(hash), "##%s", overlay_param_names[s]);
   data->stat_selector = (enum overlay_param_enabled) s;
   data->time_dividor = 1000.0f;
   if (s == OVERLAY_PARAM_ENABLED_gpu_timing)
            if (s == OVERLAY_PARAM_ENABLED_frame_timing ||
      s == OVERLAY_PARAM_ENABLED_acquire_timing ||
   s == OVERLAY_PARAM_ENABLED_present_timing ||
   s == OVERLAY_PARAM_ENABLED_gpu_timing) {
   double min_time = data->stats_min.stats[s] / data->time_dividor;
   double max_time = data->stats_max.stats[s] / data->time_dividor;
   ImGui::PlotHistogram(hash, get_time_stat, data,
                     ImGui::Text("%s: %.3fms [%.3f, %.3f]", overlay_param_names[s],
            } else {
      ImGui::PlotHistogram(hash, get_stat, data,
                        ARRAY_SIZE(data->frames_stats), 0,
      ImGui::Text("%s: %.0f [%" PRIu64 ", %" PRIu64 "]", overlay_param_names[s],
               }
   data->window_size = ImVec2(data->window_size.x, ImGui::GetCursorPosY() + 10.0f);
   ImGui::End();
   ImGui::EndFrame();
      }
      static uint32_t vk_memory_type(struct device_data *data,
               {
      VkPhysicalDeviceMemoryProperties prop;
   data->instance->pd_vtable.GetPhysicalDeviceMemoryProperties(data->physical_device, &prop);
   for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
      if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1<<i))
         }
      static void ensure_swapchain_fonts(struct swapchain_data *data,
         {
      if (data->font_uploaded)
                     struct device_data *device_data = data->device;
   ImGuiIO& io = ImGui::GetIO();
   unsigned char* pixels;
   int width, height;
   io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            /* Upload buffer */
   VkBufferCreateInfo buffer_info = {};
   buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   buffer_info.size = upload_size;
   buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
   buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   VK_CHECK(device_data->vtable.CreateBuffer(device_data->device, &buffer_info,
         VkMemoryRequirements upload_buffer_req;
   device_data->vtable.GetBufferMemoryRequirements(device_data->device,
               VkMemoryAllocateInfo upload_alloc_info = {};
   upload_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   upload_alloc_info.allocationSize = upload_buffer_req.size;
   upload_alloc_info.memoryTypeIndex = vk_memory_type(device_data,
               VK_CHECK(device_data->vtable.AllocateMemory(device_data->device,
                     VK_CHECK(device_data->vtable.BindBufferMemory(device_data->device,
                  /* Upload to Buffer */
   char* map = NULL;
   VK_CHECK(device_data->vtable.MapMemory(device_data->device,
               memcpy(map, pixels, upload_size);
   VkMappedMemoryRange range[1] = {};
   range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   range[0].memory = data->upload_font_buffer_mem;
   range[0].size = upload_size;
   VK_CHECK(device_data->vtable.FlushMappedMemoryRanges(device_data->device, 1, range));
   device_data->vtable.UnmapMemory(device_data->device,
            /* Copy buffer to image */
   VkImageMemoryBarrier copy_barrier[1] = {};
   copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   copy_barrier[0].image = data->font_image;
   copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   copy_barrier[0].subresourceRange.levelCount = 1;
   copy_barrier[0].subresourceRange.layerCount = 1;
   device_data->vtable.CmdPipelineBarrier(command_buffer,
                              VkBufferImageCopy region = {};
   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.layerCount = 1;
   region.imageExtent.width = width;
   region.imageExtent.height = height;
   region.imageExtent.depth = 1;
   device_data->vtable.CmdCopyBufferToImage(command_buffer,
                              VkImageMemoryBarrier use_barrier[1] = {};
   use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
   use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   use_barrier[0].image = data->font_image;
   use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   use_barrier[0].subresourceRange.levelCount = 1;
   use_barrier[0].subresourceRange.layerCount = 1;
   device_data->vtable.CmdPipelineBarrier(command_buffer,
                                          /* Store our identifier */
      }
      static void CreateOrResizeBuffer(struct device_data *data,
                           {
      if (*buffer != VK_NULL_HANDLE)
         if (*buffer_memory)
            VkBufferCreateInfo buffer_info = {};
   buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   buffer_info.size = new_size;
   buffer_info.usage = usage;
   buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkMemoryRequirements req;
   data->vtable.GetBufferMemoryRequirements(data->device, *buffer, &req);
   VkMemoryAllocateInfo alloc_info = {};
   alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   alloc_info.allocationSize = req.size;
   alloc_info.memoryTypeIndex =
                  VK_CHECK(data->vtable.BindBufferMemory(data->device, *buffer, *buffer_memory, 0));
      }
      static struct overlay_draw *render_swapchain_display(struct swapchain_data *data,
                           {
      ImDrawData* draw_data = ImGui::GetDrawData();
   if (draw_data->TotalVtxCount == 0)
            struct device_data *device_data = data->device;
                     VkRenderPassBeginInfo render_pass_info = {};
   render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   render_pass_info.renderPass = data->render_pass;
   render_pass_info.framebuffer = data->framebuffers[image_index];
   render_pass_info.renderArea.extent.width = data->width;
            VkCommandBufferBeginInfo buffer_begin_info = {};
                              /* Bounce the image to display back to color attachment layout for
   * rendering on top of it.
   */
   VkImageMemoryBarrier imb;
   imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   imb.pNext = nullptr;
   imb.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   imb.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   imb.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   imb.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   imb.image = data->images[image_index];
   imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   imb.subresourceRange.baseMipLevel = 0;
   imb.subresourceRange.levelCount = 1;
   imb.subresourceRange.baseArrayLayer = 0;
   imb.subresourceRange.layerCount = 1;
   imb.srcQueueFamilyIndex = present_queue->family_index;
   imb.dstQueueFamilyIndex = device_data->graphic_queue->family_index;
   device_data->vtable.CmdPipelineBarrier(draw->command_buffer,
                                          device_data->vtable.CmdBeginRenderPass(draw->command_buffer, &render_pass_info,
            /* Create/Resize vertex & index buffers */
   size_t vertex_size = ALIGN(draw_data->TotalVtxCount * sizeof(ImDrawVert), device_data->properties.limits.nonCoherentAtomSize);
   size_t index_size = ALIGN(draw_data->TotalIdxCount * sizeof(ImDrawIdx), device_data->properties.limits.nonCoherentAtomSize);
   if (draw->vertex_buffer_size < vertex_size) {
      CreateOrResizeBuffer(device_data,
                        }
   if (draw->index_buffer_size < index_size) {
      CreateOrResizeBuffer(device_data,
                                 /* Upload vertex & index data */
   ImDrawVert* vtx_dst = NULL;
   ImDrawIdx* idx_dst = NULL;
   VK_CHECK(device_data->vtable.MapMemory(device_data->device, draw->vertex_buffer_mem,
         VK_CHECK(device_data->vtable.MapMemory(device_data->device, draw->index_buffer_mem,
         for (int n = 0; n < draw_data->CmdListsCount; n++)
      {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];
   memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
   memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
   vtx_dst += cmd_list->VtxBuffer.Size;
         VkMappedMemoryRange range[2] = {};
   range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   range[0].memory = draw->vertex_buffer_mem;
   range[0].size = VK_WHOLE_SIZE;
   range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   range[1].memory = draw->index_buffer_mem;
   range[1].size = VK_WHOLE_SIZE;
   VK_CHECK(device_data->vtable.FlushMappedMemoryRanges(device_data->device, 2, range));
   device_data->vtable.UnmapMemory(device_data->device, draw->vertex_buffer_mem);
            /* Bind pipeline and descriptor sets */
   device_data->vtable.CmdBindPipeline(draw->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data->pipeline);
   VkDescriptorSet desc_set[1] = { data->descriptor_set };
   device_data->vtable.CmdBindDescriptorSets(draw->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            /* Bind vertex & index buffers */
   VkBuffer vertex_buffers[1] = { draw->vertex_buffer };
   VkDeviceSize vertex_offset[1] = { 0 };
   device_data->vtable.CmdBindVertexBuffers(draw->command_buffer, 0, 1, vertex_buffers, vertex_offset);
            /* Setup viewport */
   VkViewport viewport;
   viewport.x = 0;
   viewport.y = 0;
   viewport.width = draw_data->DisplaySize.x;
   viewport.height = draw_data->DisplaySize.y;
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
               /* Setup scale and translation through push constants :
   *
   * Our visible imgui space lies from draw_data->DisplayPos (top left) to
   * draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin
   * is typically (0,0) for single viewport apps.
   */
   float scale[2];
   scale[0] = 2.0f / draw_data->DisplaySize.x;
   scale[1] = 2.0f / draw_data->DisplaySize.y;
   float translate[2];
   translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
   translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
   device_data->vtable.CmdPushConstants(draw->command_buffer, data->pipeline_layout,
               device_data->vtable.CmdPushConstants(draw->command_buffer, data->pipeline_layout,
                  // Render the command lists:
   int vtx_offset = 0;
   int idx_offset = 0;
   ImVec2 display_pos = draw_data->DisplayPos;
   for (int n = 0; n < draw_data->CmdListsCount; n++)
   {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];
   for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
   {
         const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
   // Apply scissor/clipping rectangle
   // FIXME: We could clamp width/height based on clamped min/max values.
   VkRect2D scissor;
   scissor.offset.x = (int32_t)(pcmd->ClipRect.x - display_pos.x) > 0 ? (int32_t)(pcmd->ClipRect.x - display_pos.x) : 0;
   scissor.offset.y = (int32_t)(pcmd->ClipRect.y - display_pos.y) > 0 ? (int32_t)(pcmd->ClipRect.y - display_pos.y) : 0;
   scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                                 }
                        if (device_data->graphic_queue->family_index != present_queue->family_index)
   {
      /* Transfer the image back to the present queue family
   * image layout was already changed to present by the render pass
   */
   imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   imb.pNext = nullptr;
   imb.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   imb.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   imb.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   imb.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   imb.image = data->images[image_index];
   imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   imb.subresourceRange.baseMipLevel = 0;
   imb.subresourceRange.levelCount = 1;
   imb.subresourceRange.baseArrayLayer = 0;
   imb.subresourceRange.layerCount = 1;
   imb.srcQueueFamilyIndex = device_data->graphic_queue->family_index;
   imb.dstQueueFamilyIndex = present_queue->family_index;
   device_data->vtable.CmdPipelineBarrier(draw->command_buffer,
                                                      /* When presenting on a different queue than where we're drawing the
   * overlay *AND* when the application does not provide a semaphore to
   * vkQueuePresent, insert our own cross engine synchronization
   * semaphore.
   */
   if (n_wait_semaphores == 0 && device_data->graphic_queue->queue != present_queue->queue) {
      VkPipelineStageFlags stages_wait = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
   VkSubmitInfo submit_info = {};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submit_info.commandBufferCount = 0;
   submit_info.pWaitDstStageMask = &stages_wait;
   submit_info.waitSemaphoreCount = 0;
   submit_info.signalSemaphoreCount = 1;
                     submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submit_info.commandBufferCount = 1;
   submit_info.pWaitDstStageMask = &stages_wait;
   submit_info.pCommandBuffers = &draw->command_buffer;
   submit_info.waitSemaphoreCount = 1;
   submit_info.pWaitSemaphores = &draw->cross_engine_semaphore;
   submit_info.signalSemaphoreCount = 1;
               } else {
      VkPipelineStageFlags *stages_wait = (VkPipelineStageFlags*) malloc(sizeof(VkPipelineStageFlags) * n_wait_semaphores);
   for (unsigned i = 0; i < n_wait_semaphores; i++)
   {
      // wait in the fragment stage until the swapchain image is ready
               VkSubmitInfo submit_info = {};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &draw->command_buffer;
   submit_info.pWaitDstStageMask = stages_wait;
   submit_info.waitSemaphoreCount = n_wait_semaphores;
   submit_info.pWaitSemaphores = wait_semaphores;
   submit_info.signalSemaphoreCount = 1;
                                    }
      static const uint32_t overlay_vert_spv[] = {
   #include "overlay.vert.spv.h"
   };
   static const uint32_t overlay_frag_spv[] = {
   #include "overlay.frag.spv.h"
   };
      static void setup_swapchain_data_pipeline(struct swapchain_data *data)
   {
      struct device_data *device_data = data->device;
            /* Create shader modules */
   VkShaderModuleCreateInfo vert_info = {};
   vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   vert_info.codeSize = sizeof(overlay_vert_spv);
   vert_info.pCode = overlay_vert_spv;
   VK_CHECK(device_data->vtable.CreateShaderModule(device_data->device,
         VkShaderModuleCreateInfo frag_info = {};
   frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   frag_info.codeSize = sizeof(overlay_frag_spv);
   frag_info.pCode = (uint32_t*)overlay_frag_spv;
   VK_CHECK(device_data->vtable.CreateShaderModule(device_data->device,
            /* Font sampler */
   VkSamplerCreateInfo sampler_info = {};
   sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   sampler_info.magFilter = VK_FILTER_LINEAR;
   sampler_info.minFilter = VK_FILTER_LINEAR;
   sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_info.minLod = -1000;
   sampler_info.maxLod = 1000;
   sampler_info.maxAnisotropy = 1.0f;
   VK_CHECK(device_data->vtable.CreateSampler(device_data->device, &sampler_info,
            /* Descriptor pool */
   VkDescriptorPoolSize sampler_pool_size = {};
   sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   sampler_pool_size.descriptorCount = 1;
   VkDescriptorPoolCreateInfo desc_pool_info = {};
   desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   desc_pool_info.maxSets = 1;
   desc_pool_info.poolSizeCount = 1;
   desc_pool_info.pPoolSizes = &sampler_pool_size;
   VK_CHECK(device_data->vtable.CreateDescriptorPool(device_data->device,
                  /* Descriptor layout */
   VkSampler sampler[1] = { data->font_sampler };
   VkDescriptorSetLayoutBinding binding[1] = {};
   binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   binding[0].descriptorCount = 1;
   binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   binding[0].pImmutableSamplers = sampler;
   VkDescriptorSetLayoutCreateInfo set_layout_info = {};
   set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   set_layout_info.bindingCount = 1;
   set_layout_info.pBindings = binding;
   VK_CHECK(device_data->vtable.CreateDescriptorSetLayout(device_data->device,
                  /* Descriptor set */
   VkDescriptorSetAllocateInfo alloc_info = {};
   alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   alloc_info.descriptorPool = data->descriptor_pool;
   alloc_info.descriptorSetCount = 1;
   alloc_info.pSetLayouts = &data->descriptor_layout;
   VK_CHECK(device_data->vtable.AllocateDescriptorSets(device_data->device,
                  /* Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full
   * 3d projection matrix
   */
   VkPushConstantRange push_constants[1] = {};
   push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   push_constants[0].offset = sizeof(float) * 0;
   push_constants[0].size = sizeof(float) * 4;
   VkPipelineLayoutCreateInfo layout_info = {};
   layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   layout_info.setLayoutCount = 1;
   layout_info.pSetLayouts = &data->descriptor_layout;
   layout_info.pushConstantRangeCount = 1;
   layout_info.pPushConstantRanges = push_constants;
   VK_CHECK(device_data->vtable.CreatePipelineLayout(device_data->device,
                  VkPipelineShaderStageCreateInfo stage[2] = {};
   stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
   stage[0].module = vert_module;
   stage[0].pName = "main";
   stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   stage[1].module = frag_module;
            VkVertexInputBindingDescription binding_desc[1] = {};
   binding_desc[0].stride = sizeof(ImDrawVert);
            VkVertexInputAttributeDescription attribute_desc[3] = {};
   attribute_desc[0].location = 0;
   attribute_desc[0].binding = binding_desc[0].binding;
   attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
   attribute_desc[0].offset = IM_OFFSETOF(ImDrawVert, pos);
   attribute_desc[1].location = 1;
   attribute_desc[1].binding = binding_desc[0].binding;
   attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
   attribute_desc[1].offset = IM_OFFSETOF(ImDrawVert, uv);
   attribute_desc[2].location = 2;
   attribute_desc[2].binding = binding_desc[0].binding;
   attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
            VkPipelineVertexInputStateCreateInfo vertex_info = {};
   vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertex_info.vertexBindingDescriptionCount = 1;
   vertex_info.pVertexBindingDescriptions = binding_desc;
   vertex_info.vertexAttributeDescriptionCount = 3;
            VkPipelineInputAssemblyStateCreateInfo ia_info = {};
   ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            VkPipelineViewportStateCreateInfo viewport_info = {};
   viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewport_info.viewportCount = 1;
            VkPipelineRasterizationStateCreateInfo raster_info = {};
   raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   raster_info.polygonMode = VK_POLYGON_MODE_FILL;
   raster_info.cullMode = VK_CULL_MODE_NONE;
   raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            VkPipelineMultisampleStateCreateInfo ms_info = {};
   ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            VkPipelineColorBlendAttachmentState color_attachment[1] = {};
   color_attachment[0].blendEnable = VK_TRUE;
   color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
   color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
   color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VkPipelineDepthStencilStateCreateInfo depth_info = {};
            VkPipelineColorBlendStateCreateInfo blend_info = {};
   blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   blend_info.attachmentCount = 1;
            VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
   VkPipelineDynamicStateCreateInfo dynamic_state = {};
   dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamic_state.dynamicStateCount = (uint32_t)IM_ARRAYSIZE(dynamic_states);
            VkGraphicsPipelineCreateInfo info = {};
   info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   info.flags = 0;
   info.stageCount = 2;
   info.pStages = stage;
   info.pVertexInputState = &vertex_info;
   info.pInputAssemblyState = &ia_info;
   info.pViewportState = &viewport_info;
   info.pRasterizationState = &raster_info;
   info.pMultisampleState = &ms_info;
   info.pDepthStencilState = &depth_info;
   info.pColorBlendState = &blend_info;
   info.pDynamicState = &dynamic_state;
   info.layout = data->pipeline_layout;
   info.renderPass = data->render_pass;
   VK_CHECK(
      device_data->vtable.CreateGraphicsPipelines(device_data->device, VK_NULL_HANDLE,
               device_data->vtable.DestroyShaderModule(device_data->device, vert_module, NULL);
            ImGuiIO& io = ImGui::GetIO();
   unsigned char* pixels;
   int width, height;
            /* Font image */
   VkImageCreateInfo image_info = {};
   image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   image_info.imageType = VK_IMAGE_TYPE_2D;
   image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
   image_info.extent.width = width;
   image_info.extent.height = height;
   image_info.extent.depth = 1;
   image_info.mipLevels = 1;
   image_info.arrayLayers = 1;
   image_info.samples = VK_SAMPLE_COUNT_1_BIT;
   image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
   image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   VK_CHECK(device_data->vtable.CreateImage(device_data->device, &image_info,
         VkMemoryRequirements font_image_req;
   device_data->vtable.GetImageMemoryRequirements(device_data->device,
         VkMemoryAllocateInfo image_alloc_info = {};
   image_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   image_alloc_info.allocationSize = font_image_req.size;
   image_alloc_info.memoryTypeIndex = vk_memory_type(device_data,
               VK_CHECK(device_data->vtable.AllocateMemory(device_data->device, &image_alloc_info,
         VK_CHECK(device_data->vtable.BindImageMemory(device_data->device,
                  /* Font image view */
   VkImageViewCreateInfo view_info = {};
   view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   view_info.image = data->font_image;
   view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
   view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   view_info.subresourceRange.levelCount = 1;
   view_info.subresourceRange.layerCount = 1;
   VK_CHECK(device_data->vtable.CreateImageView(device_data->device, &view_info,
            /* Descriptor set */
   VkDescriptorImageInfo desc_image[1] = {};
   desc_image[0].sampler = data->font_sampler;
   desc_image[0].imageView = data->font_image_view;
   desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   VkWriteDescriptorSet write_desc[1] = {};
   write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   write_desc[0].dstSet = data->descriptor_set;
   write_desc[0].descriptorCount = 1;
   write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   write_desc[0].pImageInfo = desc_image;
      }
      static void setup_swapchain_data(struct swapchain_data *data,
         {
      data->width = pCreateInfo->imageExtent.width;
   data->height = pCreateInfo->imageExtent.height;
            data->imgui_context = ImGui::CreateContext();
            ImGui::GetIO().IniFilename = NULL;
                     /* Render pass */
   VkAttachmentDescription attachment_desc = {};
   attachment_desc.format = pCreateInfo->imageFormat;
   attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
   attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   VkAttachmentReference color_attachment = {};
   color_attachment.attachment = 0;
   color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &color_attachment;
   VkSubpassDependency dependency = {};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   VkRenderPassCreateInfo render_pass_info = {};
   render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   render_pass_info.attachmentCount = 1;
   render_pass_info.pAttachments = &attachment_desc;
   render_pass_info.subpassCount = 1;
   render_pass_info.pSubpasses = &subpass;
   render_pass_info.dependencyCount = 1;
   render_pass_info.pDependencies = &dependency;
   VK_CHECK(device_data->vtable.CreateRenderPass(device_data->device,
                           VK_CHECK(device_data->vtable.GetSwapchainImagesKHR(device_data->device,
                        data->images = ralloc_array(data, VkImage, data->n_images);
   data->image_views = ralloc_array(data, VkImageView, data->n_images);
            VK_CHECK(device_data->vtable.GetSwapchainImagesKHR(device_data->device,
                        /* Image views */
   VkImageViewCreateInfo view_info = {};
   view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   view_info.format = pCreateInfo->imageFormat;
   view_info.components.r = VK_COMPONENT_SWIZZLE_R;
   view_info.components.g = VK_COMPONENT_SWIZZLE_G;
   view_info.components.b = VK_COMPONENT_SWIZZLE_B;
   view_info.components.a = VK_COMPONENT_SWIZZLE_A;
   view_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
   for (uint32_t i = 0; i < data->n_images; i++) {
      view_info.image = data->images[i];
   VK_CHECK(device_data->vtable.CreateImageView(device_data->device,
                     /* Framebuffers */
   VkImageView attachment[1];
   VkFramebufferCreateInfo fb_info = {};
   fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   fb_info.renderPass = data->render_pass;
   fb_info.attachmentCount = 1;
   fb_info.pAttachments = attachment;
   fb_info.width = data->width;
   fb_info.height = data->height;
   fb_info.layers = 1;
   for (uint32_t i = 0; i < data->n_images; i++) {
      attachment[0] = data->image_views[i];
   VK_CHECK(device_data->vtable.CreateFramebuffer(device_data->device, &fb_info,
               /* Command buffer pool */
   VkCommandPoolCreateInfo cmd_buffer_pool_info = {};
   cmd_buffer_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   cmd_buffer_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   cmd_buffer_pool_info.queueFamilyIndex = device_data->graphic_queue->family_index;
   VK_CHECK(device_data->vtable.CreateCommandPool(device_data->device,
            }
      static void shutdown_swapchain_data(struct swapchain_data *data)
   {
               list_for_each_entry_safe(struct overlay_draw, draw, &data->draws, link) {
      device_data->vtable.DestroySemaphore(device_data->device, draw->cross_engine_semaphore, NULL);
   device_data->vtable.DestroySemaphore(device_data->device, draw->semaphore, NULL);
   device_data->vtable.DestroyFence(device_data->device, draw->fence, NULL);
   device_data->vtable.DestroyBuffer(device_data->device, draw->vertex_buffer, NULL);
   device_data->vtable.DestroyBuffer(device_data->device, draw->index_buffer, NULL);
   device_data->vtable.FreeMemory(device_data->device, draw->vertex_buffer_mem, NULL);
               for (uint32_t i = 0; i < data->n_images; i++) {
      device_data->vtable.DestroyImageView(device_data->device, data->image_views[i], NULL);
                                 device_data->vtable.DestroyPipeline(device_data->device, data->pipeline, NULL);
            device_data->vtable.DestroyDescriptorPool(device_data->device,
         device_data->vtable.DestroyDescriptorSetLayout(device_data->device,
            device_data->vtable.DestroySampler(device_data->device, data->font_sampler, NULL);
   device_data->vtable.DestroyImageView(device_data->device, data->font_image_view, NULL);
   device_data->vtable.DestroyImage(device_data->device, data->font_image, NULL);
            device_data->vtable.DestroyBuffer(device_data->device, data->upload_font_buffer, NULL);
               }
      static struct overlay_draw *before_present(struct swapchain_data *swapchain_data,
                           {
      struct instance_data *instance_data = swapchain_data->device->instance;
                     if (!instance_data->params.no_display && swapchain_data->n_frames > 0) {
      compute_swapchain_display(swapchain_data);
   draw = render_swapchain_display(swapchain_data, present_queue,
                        }
      static VkResult overlay_CreateSwapchainKHR(
      VkDevice                                    device,
   const VkSwapchainCreateInfoKHR*             pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      struct device_data *device_data = FIND(struct device_data, device);
   VkResult result = device_data->vtable.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
            struct swapchain_data *swapchain_data = new_swapchain_data(*pSwapchain, device_data);
   setup_swapchain_data(swapchain_data, pCreateInfo);
      }
      static void overlay_DestroySwapchainKHR(
      VkDevice                                    device,
   VkSwapchainKHR                              swapchain,
      {
      if (swapchain == VK_NULL_HANDLE) {
      struct device_data *device_data = FIND(struct device_data, device);
   device_data->vtable.DestroySwapchainKHR(device, swapchain, pAllocator);
               struct swapchain_data *swapchain_data =
            shutdown_swapchain_data(swapchain_data);
   swapchain_data->device->vtable.DestroySwapchainKHR(device, swapchain, pAllocator);
      }
      static VkResult overlay_QueuePresentKHR(
      VkQueue                                     queue,
      {
      struct queue_data *queue_data = FIND(struct queue_data, queue);
   struct device_data *device_data = queue_data->device;
   struct instance_data *instance_data = device_data->instance;
                     if (list_length(&queue_data->running_command_buffer) > 0) {
      /* Before getting the query results, make sure the operations have
   * completed.
   */
   VK_CHECK(device_data->vtable.ResetFences(device_data->device,
         VK_CHECK(device_data->vtable.QueueSubmit(queue, 0, NULL, queue_data->queries_fence));
   VK_CHECK(device_data->vtable.WaitForFences(device_data->device,
                  /* Now get the results. */
   list_for_each_entry_safe(struct command_buffer_data, cmd_buffer_data,
                     if (cmd_buffer_data->pipeline_query_pool) {
      memset(query_results, 0, sizeof(query_results));
   VK_CHECK(device_data->vtable.GetQueryPoolResults(device_data->device,
                              for (uint32_t i = OVERLAY_PARAM_ENABLED_vertices;
      i <= OVERLAY_PARAM_ENABLED_compute_invocations; i++) {
         }
   if (cmd_buffer_data->timestamp_query_pool) {
      uint64_t gpu_timestamps[2] = { 0 };
   VK_CHECK(device_data->vtable.GetQueryPoolResults(device_data->device,
                              gpu_timestamps[0] &= queue_data->timestamp_mask;
   gpu_timestamps[1] &= queue_data->timestamp_mask;
   device_data->frame_stats.stats[OVERLAY_PARAM_ENABLED_gpu_timing] +=
      (gpu_timestamps[1] - gpu_timestamps[0]) *
                  /* Otherwise we need to add our overlay drawing semaphore to the list of
   * semaphores to wait on. If we don't do that the presented picture might
   * be have incomplete overlay drawings.
   */
   VkResult result = VK_SUCCESS;
   if (instance_data->params.no_display) {
      for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
      VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];
                           before_present(swapchain_data,
                              VkPresentInfoKHR present_info = *pPresentInfo;
   present_info.swapchainCount = 1;
                  uint64_t ts0 = os_time_get();
   result = queue_data->device->vtable.QueuePresentKHR(queue, &present_info);
   uint64_t ts1 = os_time_get();
         } else {
      for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
      VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];
                           VkPresentInfoKHR present_info = *pPresentInfo;
   present_info.swapchainCount = 1;
                  struct overlay_draw *draw = before_present(swapchain_data,
                              /* Because the submission of the overlay draw waits on the semaphores
   * handed for present, we don't need to have this present operation
   * wait on them as well, we can just wait on the overlay submission
   * semaphore.
   */
                  uint64_t ts0 = os_time_get();
   VkResult chain_result = queue_data->device->vtable.QueuePresentKHR(queue, &present_info);
   uint64_t ts1 = os_time_get();
   swapchain_data->frame_stats.stats[OVERLAY_PARAM_ENABLED_present_timing] += ts1 - ts0;
   if (pPresentInfo->pResults)
         if (chain_result != VK_SUCCESS && result == VK_SUCCESS)
         }
      }
      static VkResult overlay_AcquireNextImageKHR(
      VkDevice                                    device,
   VkSwapchainKHR                              swapchain,
   uint64_t                                    timeout,
   VkSemaphore                                 semaphore,
   VkFence                                     fence,
      {
      struct swapchain_data *swapchain_data =
                  uint64_t ts0 = os_time_get();
   VkResult result = device_data->vtable.AcquireNextImageKHR(device, swapchain, timeout,
                  swapchain_data->frame_stats.stats[OVERLAY_PARAM_ENABLED_acquire_timing] += ts1 - ts0;
               }
      static VkResult overlay_AcquireNextImage2KHR(
      VkDevice                                    device,
   const VkAcquireNextImageInfoKHR*            pAcquireInfo,
      {
      struct swapchain_data *swapchain_data =
                  uint64_t ts0 = os_time_get();
   VkResult result = device_data->vtable.AcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
            swapchain_data->frame_stats.stats[OVERLAY_PARAM_ENABLED_acquire_timing] += ts1 - ts0;
               }
      static void overlay_CmdDraw(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    vertexCount,
   uint32_t                                    instanceCount,
   uint32_t                                    firstVertex,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_draw]++;
   struct device_data *device_data = cmd_buffer_data->device;
   device_data->vtable.CmdDraw(commandBuffer, vertexCount, instanceCount,
      }
      static void overlay_CmdDrawIndexed(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    indexCount,
   uint32_t                                    instanceCount,
   uint32_t                                    firstIndex,
   int32_t                                     vertexOffset,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_draw_indexed]++;
   struct device_data *device_data = cmd_buffer_data->device;
   device_data->vtable.CmdDrawIndexed(commandBuffer, indexCount, instanceCount,
      }
      static void overlay_CmdDrawIndirect(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    buffer,
   VkDeviceSize                                offset,
   uint32_t                                    drawCount,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_draw_indirect]++;
   struct device_data *device_data = cmd_buffer_data->device;
      }
      static void overlay_CmdDrawIndexedIndirect(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    buffer,
   VkDeviceSize                                offset,
   uint32_t                                    drawCount,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_draw_indexed_indirect]++;
   struct device_data *device_data = cmd_buffer_data->device;
      }
      static void overlay_CmdDrawIndirectCount(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    buffer,
   VkDeviceSize                                offset,
   VkBuffer                                    countBuffer,
   VkDeviceSize                                countBufferOffset,
   uint32_t                                    maxDrawCount,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_draw_indirect_count]++;
   struct device_data *device_data = cmd_buffer_data->device;
   device_data->vtable.CmdDrawIndirectCount(commandBuffer, buffer, offset,
            }
      static void overlay_CmdDrawIndexedIndirectCount(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    buffer,
   VkDeviceSize                                offset,
   VkBuffer                                    countBuffer,
   VkDeviceSize                                countBufferOffset,
   uint32_t                                    maxDrawCount,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_draw_indexed_indirect_count]++;
   struct device_data *device_data = cmd_buffer_data->device;
   device_data->vtable.CmdDrawIndexedIndirectCount(commandBuffer, buffer, offset,
            }
      static void overlay_CmdDispatch(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    groupCountX,
   uint32_t                                    groupCountY,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_dispatch]++;
   struct device_data *device_data = cmd_buffer_data->device;
      }
      static void overlay_CmdDispatchIndirect(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    buffer,
      {
      struct command_buffer_data *cmd_buffer_data =
         cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_dispatch_indirect]++;
   struct device_data *device_data = cmd_buffer_data->device;
      }
      static void overlay_CmdBindPipeline(
      VkCommandBuffer                             commandBuffer,
   VkPipelineBindPoint                         pipelineBindPoint,
      {
      struct command_buffer_data *cmd_buffer_data =
         switch (pipelineBindPoint) {
   case VK_PIPELINE_BIND_POINT_GRAPHICS: cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_pipeline_graphics]++; break;
   case VK_PIPELINE_BIND_POINT_COMPUTE: cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_pipeline_compute]++; break;
   case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR: cmd_buffer_data->stats.stats[OVERLAY_PARAM_ENABLED_pipeline_raytracing]++; break;
   default: break;
   }
   struct device_data *device_data = cmd_buffer_data->device;
      }
      static VkResult overlay_BeginCommandBuffer(
      VkCommandBuffer                             commandBuffer,
      {
      struct command_buffer_data *cmd_buffer_data =
                           /* We don't record any query in secondary command buffers, just make sure
   * we have the right inheritance.
   */
   if (cmd_buffer_data->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
               struct VkBaseOutStructure *new_pnext =
                  /* If there was no pNext chain given or we managed to copy it, we can
   * add our stuff in there.
   *
   * Otherwise, keep the old pointer. We failed to copy the pNext chain,
   * meaning there is an unknown extension somewhere in there.
   */
   if (new_pnext || pBeginInfo->pNext == NULL) {
               VkCommandBufferInheritanceInfo *parent_inhe_info = (VkCommandBufferInheritanceInfo *)
         inhe_info = (VkCommandBufferInheritanceInfo) {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
   NULL,
   VK_NULL_HANDLE,
   0,
   VK_NULL_HANDLE,
   VK_FALSE,
   0,
               if (parent_inhe_info)
         else
               VkResult result = device_data->vtable.BeginCommandBuffer(
                                 /* Otherwise record a begin query as first command. */
            if (result == VK_SUCCESS) {
      if (cmd_buffer_data->pipeline_query_pool) {
      device_data->vtable.CmdResetQueryPool(commandBuffer,
            }
   if (cmd_buffer_data->timestamp_query_pool) {
      device_data->vtable.CmdResetQueryPool(commandBuffer,
            }
   if (cmd_buffer_data->pipeline_query_pool) {
      device_data->vtable.CmdBeginQuery(commandBuffer,
            }
   if (cmd_buffer_data->timestamp_query_pool) {
      device_data->vtable.CmdWriteTimestamp(commandBuffer,
                                 }
      static VkResult overlay_EndCommandBuffer(
         {
      struct command_buffer_data *cmd_buffer_data =
                  if (cmd_buffer_data->timestamp_query_pool) {
      device_data->vtable.CmdWriteTimestamp(commandBuffer,
                  }
   if (cmd_buffer_data->pipeline_query_pool) {
      device_data->vtable.CmdEndQuery(commandBuffer,
                        }
      static VkResult overlay_ResetCommandBuffer(
      VkCommandBuffer                             commandBuffer,
      {
      struct command_buffer_data *cmd_buffer_data =
                              }
      static void overlay_CmdExecuteCommands(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    commandBufferCount,
      {
      struct command_buffer_data *cmd_buffer_data =
                  /* Add the stats of the executed command buffers to the primary one. */
   for (uint32_t c = 0; c < commandBufferCount; c++) {
      struct command_buffer_data *sec_cmd_buffer_data =
            for (uint32_t s = 0; s < OVERLAY_PARAM_ENABLED_MAX; s++)
                  }
      static VkResult overlay_AllocateCommandBuffers(
      VkDevice                           device,
   const VkCommandBufferAllocateInfo* pAllocateInfo,
      {
      struct device_data *device_data = FIND(struct device_data, device);
   VkResult result =
         if (result != VK_SUCCESS)
            VkQueryPool pipeline_query_pool = VK_NULL_HANDLE;
   VkQueryPool timestamp_query_pool = VK_NULL_HANDLE;
   if (device_data->pipeline_statistics_enabled &&
      pAllocateInfo->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
   VkQueryPoolCreateInfo pool_info = {
      VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
   NULL,
   0,
   VK_QUERY_TYPE_PIPELINE_STATISTICS,
   pAllocateInfo->commandBufferCount,
      };
   VK_CHECK(device_data->vtable.CreateQueryPool(device_data->device, &pool_info,
      }
   if (device_data->instance->params.enabled[OVERLAY_PARAM_ENABLED_gpu_timing]) {
      VkQueryPoolCreateInfo pool_info = {
      VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
   NULL,
   0,
   VK_QUERY_TYPE_TIMESTAMP,
   pAllocateInfo->commandBufferCount * 2,
      };
   VK_CHECK(device_data->vtable.CreateQueryPool(device_data->device, &pool_info,
               for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      new_command_buffer_data(pCommandBuffers[i], pAllocateInfo->level,
                     if (pipeline_query_pool)
         if (timestamp_query_pool)
               }
      static void overlay_FreeCommandBuffers(
      VkDevice               device,
   VkCommandPool          commandPool,
   uint32_t               commandBufferCount,
      {
      struct device_data *device_data = FIND(struct device_data, device);
   for (uint32_t i = 0; i < commandBufferCount; i++) {
      struct command_buffer_data *cmd_buffer_data =
            /* It is legal to free a NULL command buffer*/
   if (!cmd_buffer_data)
            uint64_t count = (uintptr_t)find_object_data(HKEY(cmd_buffer_data->pipeline_query_pool));
   if (count == 1) {
      unmap_object(HKEY(cmd_buffer_data->pipeline_query_pool));
   device_data->vtable.DestroyQueryPool(device_data->device,
      } else if (count != 0) {
         }
   count = (uintptr_t)find_object_data(HKEY(cmd_buffer_data->timestamp_query_pool));
   if (count == 1) {
      unmap_object(HKEY(cmd_buffer_data->timestamp_query_pool));
   device_data->vtable.DestroyQueryPool(device_data->device,
      } else if (count != 0) {
         }
               device_data->vtable.FreeCommandBuffers(device, commandPool,
      }
      static VkResult overlay_QueueSubmit(
      VkQueue                                     queue,
   uint32_t                                    submitCount,
   const VkSubmitInfo*                         pSubmits,
      {
      struct queue_data *queue_data = FIND(struct queue_data, queue);
                     for (uint32_t s = 0; s < submitCount; s++) {
      for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; c++) {
                     /* Merge the submitted command buffer stats into the device. */
                  /* Attach the command buffer to the queue so we remember to read its
   * pipeline statistics & timestamps at QueuePresent().
   */
   if (!cmd_buffer_data->pipeline_query_pool &&
                  if (list_is_empty(&cmd_buffer_data->link)) {
      list_addtail(&cmd_buffer_data->link,
      } else {
      fprintf(stderr, "Command buffer submitted multiple times before present.\n"
                        }
      static VkResult overlay_QueueSubmit2KHR(
      VkQueue                                     queue,
   uint32_t                                    submitCount,
   const VkSubmitInfo2*                        pSubmits,
      {
      struct queue_data *queue_data = FIND(struct queue_data, queue);
                     for (uint32_t s = 0; s < submitCount; s++) {
      for (uint32_t c = 0; c < pSubmits[s].commandBufferInfoCount; c++) {
                     /* Merge the submitted command buffer stats into the device. */
                  /* Attach the command buffer to the queue so we remember to read its
   * pipeline statistics & timestamps at QueuePresent().
   */
   if (!cmd_buffer_data->pipeline_query_pool &&
                  if (list_is_empty(&cmd_buffer_data->link)) {
      list_addtail(&cmd_buffer_data->link,
      } else {
      fprintf(stderr, "Command buffer submitted multiple times before present.\n"
                        }
      static VkResult overlay_CreateDevice(
      VkPhysicalDevice                            physicalDevice,
   const VkDeviceCreateInfo*                   pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      struct instance_data *instance_data =
         VkLayerDeviceCreateInfo *chain_info =
            assert(chain_info->u.pLayerInfo);
   PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
   PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
   PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(NULL, "vkCreateDevice");
   if (fpCreateDevice == NULL) {
                  // Advance the link info for the next element on the chain
            VkPhysicalDeviceFeatures device_features = {};
                     struct VkBaseOutStructure *new_pnext =
         if (new_pnext != NULL) {
               VkPhysicalDeviceFeatures2 *device_features2 = (VkPhysicalDeviceFeatures2 *)
         if (device_features2) {
      /* Can't use device_info->pEnabledFeatures when VkPhysicalDeviceFeatures2 is present */
      } else {
      if (create_info.pEnabledFeatures)
         device_features_ptr = &device_features;
               if (instance_data->pipeline_statistics_enabled) {
      device_features_ptr->inheritedQueries = true;
                  VkResult result = fpCreateDevice(physicalDevice, &create_info, pAllocator, pDevice);
   free_chain(new_pnext);
            struct device_data *device_data = new_device_data(*pDevice, instance_data);
   device_data->physical_device = physicalDevice;
   vk_device_dispatch_table_load(&device_data->vtable,
            instance_data->pd_vtable.GetPhysicalDeviceProperties(device_data->physical_device,
            VkLayerDeviceCreateInfo *load_data_info =
                           device_data->pipeline_statistics_enabled =
      new_pnext != NULL &&
            }
      static void overlay_DestroyDevice(
      VkDevice                                    device,
      {
      struct device_data *device_data = FIND(struct device_data, device);
   device_unmap_queues(device_data);
   device_data->vtable.DestroyDevice(device, pAllocator);
      }
      static VkResult overlay_CreateInstance(
      const VkInstanceCreateInfo*                 pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      VkLayerInstanceCreateInfo *chain_info =
            assert(chain_info->u.pLayerInfo);
   PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
         PFN_vkCreateInstance fpCreateInstance =
         if (fpCreateInstance == NULL) {
                  // Advance the link info for the next element on the chain
            VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
            struct instance_data *instance_data = new_instance_data(*pInstance);
   vk_instance_dispatch_table_load(&instance_data->vtable,
               vk_physical_device_dispatch_table_load(&instance_data->pd_vtable,
                                 /* If there's no control file, and an output_file was specified, start
   * capturing fps data right away.
   */
   instance_data->capture_enabled =
                  for (int i = OVERLAY_PARAM_ENABLED_vertices;
      i <= OVERLAY_PARAM_ENABLED_compute_invocations; i++) {
   if (instance_data->params.enabled[i]) {
      instance_data->pipeline_statistics_enabled = true;
                     }
      static void overlay_DestroyInstance(
      VkInstance                                  instance,
      {
      struct instance_data *instance_data = FIND(struct instance_data, instance);
   instance_data_map_physical_devices(instance_data, false);
   instance_data->vtable.DestroyInstance(instance, pAllocator);
      }
      static const struct {
      const char *name;
      } name_to_funcptr_map[] = {
      { "vkGetInstanceProcAddr", (void *) vkGetInstanceProcAddr },
      #define ADD_HOOK(fn) { "vk" # fn, (void *) overlay_ ## fn }
   #define ADD_ALIAS_HOOK(alias, fn) { "vk" # alias, (void *) overlay_ ## fn }
      ADD_HOOK(AllocateCommandBuffers),
   ADD_HOOK(FreeCommandBuffers),
   ADD_HOOK(ResetCommandBuffer),
   ADD_HOOK(BeginCommandBuffer),
   ADD_HOOK(EndCommandBuffer),
            ADD_HOOK(CmdDraw),
   ADD_HOOK(CmdDrawIndexed),
   ADD_HOOK(CmdDrawIndirect),
   ADD_HOOK(CmdDrawIndexedIndirect),
   ADD_HOOK(CmdDispatch),
   ADD_HOOK(CmdDispatchIndirect),
   ADD_HOOK(CmdDrawIndirectCount),
   ADD_ALIAS_HOOK(CmdDrawIndirectCountKHR, CmdDrawIndirectCount),
   ADD_HOOK(CmdDrawIndexedIndirectCount),
                     ADD_HOOK(CreateSwapchainKHR),
   ADD_HOOK(QueuePresentKHR),
   ADD_HOOK(DestroySwapchainKHR),
   ADD_HOOK(AcquireNextImageKHR),
            ADD_HOOK(QueueSubmit),
            ADD_HOOK(CreateDevice),
            ADD_HOOK(CreateInstance),
      #undef ADD_HOOK
   #undef ADD_ALIAS_HOOK
   };
      static void *find_ptr(const char *name)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(name_to_funcptr_map); i++) {
      if (strcmp(name, name_to_funcptr_map[i].name) == 0)
                  }
      PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev,
         {
      void *ptr = find_ptr(funcName);
                     struct device_data *device_data = FIND(struct device_data, dev);
   if (device_data->vtable.GetDeviceProcAddr == NULL) return NULL;
      }
      PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance,
         {
      void *ptr = find_ptr(funcName);
                     struct instance_data *instance_data = FIND(struct instance_data, instance);
   if (instance_data->vtable.GetInstanceProcAddr == NULL) return NULL;
      }
