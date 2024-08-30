   /*
   * Copyright 2018 Collabora Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "zink_resource.h"
      #include "zink_batch.h"
   #include "zink_clear.h"
   #include "zink_context.h"
   #include "zink_fence.h"
   #include "zink_format.h"
   #include "zink_program.h"
   #include "zink_screen.h"
   #include "zink_kopper.h"
      #ifdef VK_USE_PLATFORM_METAL_EXT
   #include "QuartzCore/CAMetalLayer.h"
   #endif
      #include "vk_format.h"
   #include "util/u_blitter.h"
   #include "util/u_debug.h"
   #include "util/format/u_format.h"
   #include "util/u_transfer_helper.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "util/os_file.h"
   #include "frontend/winsys_handle.h"
      #if !defined(__APPLE__)
   #define ZINK_USE_DMABUF
   #endif
      #if defined(ZINK_USE_DMABUF) && !defined(_WIN32)
   #include "drm-uapi/drm_fourcc.h"
   #else
   /* these won't actually be used */
   #define DRM_FORMAT_MOD_INVALID 0
   #define DRM_FORMAT_MOD_LINEAR 0
   #endif
      #if defined(__APPLE__)
   // Source of MVK_VERSION
   #include "MoltenVK/vk_mvk_moltenvk.h"
   #endif
      #define ZINK_EXTERNAL_MEMORY_HANDLE 999
            struct zink_debug_mem_entry {
      uint32_t count;
   uint64_t size;
      };
      static const char *
   zink_debug_mem_add(struct zink_screen *screen, uint64_t size, const char *name)
   {
               simple_mtx_lock(&screen->debug_mem_lock);
   struct hash_entry *entry = _mesa_hash_table_search(screen->debug_mem_sizes, name);
            if (!entry) {
      debug_bos = calloc(1, sizeof(struct zink_debug_mem_entry));
   debug_bos->name = strdup(name);
      } else {
                  debug_bos->count++;
   debug_bos->size += align(size, 4096);
               }
      static void
   zink_debug_mem_del(struct zink_screen *screen, struct zink_bo *bo)
   {
      simple_mtx_lock(&screen->debug_mem_lock);
   struct hash_entry *entry = _mesa_hash_table_search(screen->debug_mem_sizes, bo->name);
   /* If we're finishing the BO, it should have been added already */
            struct zink_debug_mem_entry *debug_bos = entry->data;
   debug_bos->count--;
   debug_bos->size -= align(zink_bo_get_size(bo), 4096);
   if (!debug_bos->count) {
      _mesa_hash_table_remove(screen->debug_mem_sizes, entry);
   free((void*)debug_bos->name);
      }
      }
      static int
   debug_bos_count_compare(const void *in_a, const void *in_b)
   {
      struct zink_debug_mem_entry *a = *(struct zink_debug_mem_entry **)in_a;
   struct zink_debug_mem_entry *b = *(struct zink_debug_mem_entry **)in_b;
      }
      void
   zink_debug_mem_print_stats(struct zink_screen *screen)
   {
               /* Put the HT's sizes data in an array so we can sort by number of allocations. */
   struct util_dynarray dyn;
            uint32_t size = 0;
   uint32_t count = 0;
   hash_table_foreach(screen->debug_mem_sizes, entry)
   {
      struct zink_debug_mem_entry *debug_bos = entry->data;
   util_dynarray_append(&dyn, struct zink_debug_mem_entry *, debug_bos);
   size += debug_bos->size / 1024;
               qsort(dyn.data,
                  util_dynarray_foreach(&dyn, struct zink_debug_mem_entry *, entryp)
   {
      struct zink_debug_mem_entry *debug_bos = *entryp;
   mesa_logi("%30s: %4d bos, %lld kb\n", debug_bos->name, debug_bos->count,
                                    }
      static bool
   equals_ivci(const void *a, const void *b)
   {
      const uint8_t *pa = a;
   const uint8_t *pb = b;
   size_t offset = offsetof(VkImageViewCreateInfo, flags);
      }
      static bool
   equals_bvci(const void *a, const void *b)
   {
      const uint8_t *pa = a;
   const uint8_t *pb = b;
   size_t offset = offsetof(VkBufferViewCreateInfo, flags);
      }
      static void
   zink_transfer_flush_region(struct pipe_context *pctx,
                  void
   debug_describe_zink_resource_object(char *buf, const struct zink_resource_object *ptr)
   {
         }
      void
   zink_destroy_resource_object(struct zink_screen *screen, struct zink_resource_object *obj)
   {
      if (obj->is_buffer) {
      while (util_dynarray_contains(&obj->views, VkBufferView))
      } else {
      while (util_dynarray_contains(&obj->views, VkImageView))
      }
   if (!obj->dt && zink_debug & ZINK_DEBUG_MEM)
         util_dynarray_fini(&obj->views);
   for (unsigned i = 0; i < ARRAY_SIZE(obj->copies); i++)
         if (obj->is_buffer) {
      VKSCR(DestroyBuffer)(screen->dev, obj->buffer, NULL);
      } else if (obj->dt) {
         } else if (!obj->is_aux) {
            #if defined(ZINK_USE_DMABUF) && !defined(_WIN32)
         #endif
               simple_mtx_destroy(&obj->view_lock);
   if (obj->dt) {
         } else
            }
      static void
   zink_resource_destroy(struct pipe_screen *pscreen,
         {
      struct zink_screen *screen = zink_screen(pscreen);
   struct zink_resource *res = zink_resource(pres);
   if (pres->target == PIPE_BUFFER) {
      util_range_destroy(&res->valid_buffer_range);
   util_idalloc_mt_free(&screen->buffer_ids, res->base.buffer_id_unique);
   assert(!_mesa_hash_table_num_entries(&res->bufferview_cache));
   simple_mtx_destroy(&res->bufferview_mtx);
      } else {
      assert(!_mesa_hash_table_num_entries(&res->surface_cache));
   simple_mtx_destroy(&res->surface_mtx);
      }
            zink_resource_object_reference(screen, &res->obj, NULL);
   threaded_resource_deinit(pres);
      }
      static VkImageAspectFlags
   aspect_from_format(enum pipe_format fmt)
   {
      if (util_format_is_depth_or_stencil(fmt)) {
      VkImageAspectFlags aspect = 0;
   const struct util_format_description *desc = util_format_description(fmt);
   if (util_format_has_depth(desc))
         if (util_format_has_stencil(desc))
            } else
      }
      static VkBufferCreateInfo
   create_bci(struct zink_screen *screen, const struct pipe_resource *templ, unsigned bind)
   {
      VkBufferCreateInfo bci;
   bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bci.pNext = NULL;
   bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   bci.queueFamilyIndexCount = 0;
   bci.pQueueFamilyIndices = NULL;
   bci.size = templ->width0;
   bci.flags = 0;
            if (bind & ZINK_BIND_DESCRIPTOR) {
      /* gallium sizes are all uint32_t, while the total size of this buffer may exceed that limit */
   bci.usage = 0;
   if (bind & ZINK_BIND_SAMPLER_DESCRIPTOR)
         if (bind & ZINK_BIND_RESOURCE_DESCRIPTOR)
      } else {
      bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                  bci.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
               VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
   VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
      }
   if (screen->info.have_KHR_buffer_device_address)
            if (bind & PIPE_BIND_SHADER_IMAGE)
            if (bind & PIPE_BIND_QUERY_BUFFER)
            if (templ->flags & PIPE_RESOURCE_FLAG_SPARSE)
            }
      typedef enum {
      USAGE_FAIL_NONE,
   USAGE_FAIL_ERROR,
      } usage_fail;
      static usage_fail
   check_ici(struct zink_screen *screen, VkImageCreateInfo *ici, uint64_t modifier)
   {
      VkImageFormatProperties image_props;
   VkResult ret;
   bool optimalDeviceAccess = true;
   assert(modifier == DRM_FORMAT_MOD_INVALID ||
         if (VKSCR(GetPhysicalDeviceImageFormatProperties2)) {
      VkImageFormatProperties2 props2;
   props2.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
   props2.pNext = NULL;
   VkSamplerYcbcrConversionImageFormatProperties ycbcr_props;
   ycbcr_props.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES;
   ycbcr_props.pNext = NULL;
   if (screen->info.have_KHR_sampler_ycbcr_conversion)
         VkHostImageCopyDevicePerformanceQueryEXT hic = {
      VK_STRUCTURE_TYPE_HOST_IMAGE_COPY_DEVICE_PERFORMANCE_QUERY_EXT,
      };
   if (screen->info.have_EXT_host_image_copy && ici->usage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT)
         VkPhysicalDeviceImageFormatInfo2 info;
   info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
   /* possibly VkImageFormatListCreateInfo */
   info.pNext = ici->pNext;
   info.format = ici->format;
   info.type = ici->imageType;
   info.tiling = ici->tiling;
   info.usage = ici->usage;
            VkPhysicalDeviceImageDrmFormatModifierInfoEXT mod_info;
   if (modifier != DRM_FORMAT_MOD_INVALID) {
      mod_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
   mod_info.pNext = info.pNext;
   mod_info.drmFormatModifier = modifier;
   mod_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   mod_info.queueFamilyIndexCount = 0;
   mod_info.pQueueFamilyIndices = NULL;
               ret = VKSCR(GetPhysicalDeviceImageFormatProperties2)(screen->pdev, &info, &props2);
   /* this is using VK_IMAGE_CREATE_EXTENDED_USAGE_BIT and can't be validated */
   if (vk_format_aspects(ici->format) & VK_IMAGE_ASPECT_PLANE_1_BIT)
         image_props = props2.imageFormatProperties;
   if (screen->info.have_EXT_host_image_copy && ici->usage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT)
      } else
      ret = VKSCR(GetPhysicalDeviceImageFormatProperties)(screen->pdev, ici->format, ici->imageType,
      if (ret != VK_SUCCESS)
         if (ici->extent.depth > image_props.maxExtent.depth ||
      ici->extent.height > image_props.maxExtent.height ||
   ici->extent.width > image_props.maxExtent.width)
      if (ici->mipLevels > image_props.maxMipLevels)
         if (ici->arrayLayers > image_props.maxArrayLayers)
         if (!optimalDeviceAccess)
            }
      static VkImageUsageFlags
   get_image_usage_for_feats(struct zink_screen *screen, VkFormatFeatureFlags2 feats, const struct pipe_resource *templ, unsigned bind, bool *need_extended)
   {
      VkImageUsageFlags usage = 0;
   bool is_planar = util_format_get_num_planes(templ->format) > 1;
            if (bind & ZINK_BIND_TRANSIENT)
         else {
      /* sadly, gallium doesn't let us know if it'll ever need this, so we have to assume */
   if (is_planar || (feats & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT))
         if (is_planar || (feats & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
         if (feats & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            if ((is_planar || (feats & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) && (bind & PIPE_BIND_SHADER_IMAGE)) {
      assert(templ->nr_samples <= 1 || screen->info.feats.features.shaderStorageImageMultisample);
                  if (bind & PIPE_BIND_RENDER_TARGET) {
      if (feats & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
      usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   if (!(bind & ZINK_BIND_TRANSIENT) && (bind & (PIPE_BIND_LINEAR | PIPE_BIND_SHARED)) != (PIPE_BIND_LINEAR | PIPE_BIND_SHARED))
         if (!(bind & ZINK_BIND_TRANSIENT) && screen->info.have_EXT_attachment_feedback_loop_layout)
      } else {
      /* trust that gallium isn't going to give us anything wild */
   *need_extended = true;
         } else if ((bind & PIPE_BIND_SAMPLER_VIEW) && !util_format_is_depth_or_stencil(templ->format)) {
      if (!(feats & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
      /* ensure we can u_blitter this later */
   *need_extended = true;
      }
               if (bind & PIPE_BIND_DEPTH_STENCIL) {
      if (feats & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
         else
         if (screen->info.have_EXT_attachment_feedback_loop_layout && !(bind & ZINK_BIND_TRANSIENT))
      /* this is unlikely to occur and has been included for completeness */
   } else if (bind & PIPE_BIND_SAMPLER_VIEW && !(usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
      if (feats & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
         else
               if (bind & PIPE_BIND_STREAM_OUTPUT)
            if (screen->info.have_EXT_host_image_copy && feats & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT)
               }
      static VkFormatFeatureFlags
   find_modifier_feats(const struct zink_modifier_prop *prop, uint64_t modifier, uint64_t *mod)
   {
      for (unsigned j = 0; j < prop->drmFormatModifierCount; j++) {
      if (prop->pDrmFormatModifierProperties[j].drmFormatModifier == modifier) {
      *mod = modifier;
         }
      }
      /* check HIC optimalness */
   static bool
   suboptimal_check_ici(struct zink_screen *screen, VkImageCreateInfo *ici, uint64_t *mod)
   {
      usage_fail fail = check_ici(screen, ici, *mod);
   if (!fail)
         if (fail == USAGE_FAIL_SUBOPTIMAL) {
      ici->usage &= ~VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
   fail = check_ici(screen, ici, *mod);
   if (!fail)
      }
      }
      /* If the driver can't do mutable with this ICI, then try again after removing mutable (and
   * thus also the list of formats we might might mutate to)
   */
   static bool
   double_check_ici(struct zink_screen *screen, VkImageCreateInfo *ici, VkImageUsageFlags usage, uint64_t *mod)
   {
      if (!usage)
                     if (suboptimal_check_ici(screen, ici, mod))
         usage_fail fail = check_ici(screen, ici, *mod);
   if (!fail)
         if (fail == USAGE_FAIL_SUBOPTIMAL) {
      ici->usage &= ~VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
   fail = check_ici(screen, ici, *mod);
   if (!fail)
      }
   const void *pNext = ici->pNext;
   if (pNext) {
      VkBaseOutStructure *prev = NULL;
   VkBaseOutStructure *fmt_list = NULL;
   vk_foreach_struct(strct, (void*)ici->pNext) {
      if (strct->sType == VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO) {
      fmt_list = strct;
   if (prev) {
         } else {
         }
   fmt_list->pNext = NULL;
      }
      }
   ici->flags &= ~VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
   if (suboptimal_check_ici(screen, ici, mod))
         fmt_list->pNext = (void*)ici->pNext;
   ici->pNext = fmt_list;
      }
      }
      static VkImageUsageFlags
   get_image_usage(struct zink_screen *screen, VkImageCreateInfo *ici, const struct pipe_resource *templ, unsigned bind, unsigned modifiers_count, uint64_t *modifiers, uint64_t *mod)
   {
      VkImageTiling tiling = ici->tiling;
   bool need_extended = false;
   *mod = DRM_FORMAT_MOD_INVALID;
   if (modifiers_count) {
      bool have_linear = false;
   const struct zink_modifier_prop *prop = &screen->modifier_props[templ->format];
   assert(tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);
   bool found = false;
   uint64_t good_mod = 0;
   VkImageUsageFlags good_usage = 0;
   for (unsigned i = 0; i < modifiers_count; i++) {
      if (modifiers[i] == DRM_FORMAT_MOD_LINEAR) {
      have_linear = true;
   if (!screen->info.have_EXT_image_drm_format_modifier)
            }
   VkFormatFeatureFlags feats = find_modifier_feats(prop, modifiers[i], mod);
   if (feats) {
      VkImageUsageFlags usage = get_image_usage_for_feats(screen, feats, templ, bind, &need_extended);
   assert(!need_extended);
   if (double_check_ici(screen, ici, usage, mod)) {
      if (!found) {
      found = true;
   good_mod = modifiers[i];
         } else {
               }
   if (found) {
      *mod = good_mod;
      }
   /* only try linear if no other options available */
   if (have_linear) {
      VkFormatFeatureFlags feats = find_modifier_feats(prop, DRM_FORMAT_MOD_LINEAR, mod);
   if (feats) {
      VkImageUsageFlags usage = get_image_usage_for_feats(screen, feats, templ, bind, &need_extended);
   assert(!need_extended);
   if (double_check_ici(screen, ici, usage, mod))
            } else {
      struct zink_format_props props = screen->format_props[templ->format];
   VkFormatFeatureFlags2 feats = tiling == VK_IMAGE_TILING_LINEAR ? props.linearTilingFeatures : props.optimalTilingFeatures;
   if (ici->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)
         VkImageUsageFlags usage = get_image_usage_for_feats(screen, feats, templ, bind, &need_extended);
   if (need_extended) {
      ici->flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
   feats = UINT32_MAX;
      }
   if (double_check_ici(screen, ici, usage, mod))
         if (util_format_is_depth_or_stencil(templ->format)) {
      if (!(templ->bind & PIPE_BIND_DEPTH_STENCIL)) {
      usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   if (double_check_ici(screen, ici, usage, mod))
         } else if (!(templ->bind & PIPE_BIND_RENDER_TARGET)) {
      usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   if (double_check_ici(screen, ici, usage, mod))
         }
   *mod = DRM_FORMAT_MOD_INVALID;
      }
      static uint64_t
   eval_ici(struct zink_screen *screen, VkImageCreateInfo *ici, const struct pipe_resource *templ, unsigned bind, unsigned modifiers_count, uint64_t *modifiers, bool *success)
   {
      /* sampleCounts will be set to VK_SAMPLE_COUNT_1_BIT if at least one of the following conditions is true:
   * - flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
   *
   * 44.1.1. Supported Sample Counts
   */
   bool want_cube = ici->samples == 1 &&
                        if (ici->tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
            bool first = true;
   bool tried[2] = {0};
      retry:
      while (!ici->usage) {
      if (!first) {
      switch (ici->tiling) {
   case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
      ici->tiling = VK_IMAGE_TILING_OPTIMAL;
   modifiers_count = 0;
      case VK_IMAGE_TILING_OPTIMAL:
      ici->tiling = VK_IMAGE_TILING_LINEAR;
      case VK_IMAGE_TILING_LINEAR:
      if (bind & PIPE_BIND_LINEAR) {
      *success = false;
      }
   ici->tiling = VK_IMAGE_TILING_OPTIMAL;
      default:
         }
   if (tried[ici->tiling]) {
      if (ici->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT) {
      *success = false;
      }
   ici->flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
   tried[0] = false;
   tried[1] = false;
   first = true;
         }
   ici->usage = get_image_usage(screen, ici, templ, bind, modifiers_count, modifiers, &mod);
   first = false;
   if (ici->tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
      }
   if (want_cube) {
      ici->flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
   if (get_image_usage(screen, ici, templ, bind, modifiers_count, modifiers, &mod) != ici->usage)
               *success = true;
      }
      static void
   init_ici(struct zink_screen *screen, VkImageCreateInfo *ici, const struct pipe_resource *templ, unsigned bind, unsigned modifiers_count)
   {
      ici->sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   /* pNext may already be set */
   if (util_format_get_num_planes(templ->format) > 1)
         else if (bind & ZINK_BIND_MUTABLE)
         else
         if (ici->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
      /* unset VkImageFormatListCreateInfo if mutable */
      else if (ici->pNext)
      /* add mutable if VkImageFormatListCreateInfo */
      ici->usage = 0;
   ici->queueFamilyIndexCount = 0;
            /* assume we're going to be doing some CompressedTexSubImage */
   if (util_format_is_compressed(templ->format) && (ici->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) &&
      !vk_find_struct_const(ici->pNext, IMAGE_FORMAT_LIST_CREATE_INFO))
         if (templ->flags & PIPE_RESOURCE_FLAG_SPARSE)
            bool need_2D = false;
   switch (templ->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      if (templ->flags & PIPE_RESOURCE_FLAG_SPARSE)
         if (util_format_is_depth_or_stencil(templ->format))
         ici->imageType = need_2D ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
         case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
      ici->imageType = VK_IMAGE_TYPE_2D;
         case PIPE_TEXTURE_3D:
      ici->imageType = VK_IMAGE_TYPE_3D;
   ici->flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
   if (screen->info.have_EXT_image_2d_view_of_3d)
               case PIPE_BUFFER:
            default:
                  if (screen->info.have_EXT_sample_locations &&
      bind & PIPE_BIND_DEPTH_STENCIL &&
   util_format_has_depth(util_format_description(templ->format)))
         ici->format = zink_get_format(screen, templ->format);
   ici->extent.width = templ->width0;
   ici->extent.height = templ->height0;
   ici->extent.depth = templ->depth0;
   ici->mipLevels = templ->last_level + 1;
   ici->arrayLayers = MAX2(templ->array_size, 1);
   ici->samples = templ->nr_samples ? templ->nr_samples : VK_SAMPLE_COUNT_1_BIT;
   ici->tiling = screen->info.have_EXT_image_drm_format_modifier && modifiers_count ?
               /* XXX: does this have perf implications anywhere? hopefully not */
   if (ici->samples == VK_SAMPLE_COUNT_1_BIT &&
      screen->info.have_EXT_multisampled_render_to_single_sampled &&
   ici->tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
      ici->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (templ->target == PIPE_TEXTURE_CUBE)
      }
      static struct zink_resource_object *
   resource_object_create(struct zink_screen *screen, const struct pipe_resource *templ, struct winsys_handle *whandle, bool *linear,
         {
      struct zink_resource_object *obj = CALLOC_STRUCT(zink_resource_object);
   unsigned max_level = 0;
   if (!obj)
         simple_mtx_init(&obj->view_lock, mtx_plain);
   util_dynarray_init(&obj->views, NULL);
   u_rwlock_init(&obj->copy_lock);
   obj->unordered_read = true;
   obj->unordered_write = true;
            VkMemoryRequirements reqs = {0};
            /* figure out aux plane count */
   if (whandle && whandle->plane >= util_format_get_num_planes(whandle->format))
         struct pipe_resource *pnext = templ->next;
   for (obj->plane_count = 1; pnext; obj->plane_count++, pnext = pnext->next) {
      struct zink_resource *next = zink_resource(pnext);
   if (!next->obj->is_aux)
               bool need_dedicated = false;
      #if !defined(_WIN32)
         #else
         #endif
      unsigned num_planes = util_format_get_num_planes(templ->format);
   VkImageAspectFlags plane_aspects[] = {
      VK_IMAGE_ASPECT_PLANE_0_BIT,
   VK_IMAGE_ASPECT_PLANE_1_BIT,
      };
   VkExternalMemoryHandleTypeFlags external = 0;
   bool needs_export = (templ->bind & (ZINK_BIND_VIDEO | ZINK_BIND_DMABUF)) != 0;
   if (whandle) {
      if (whandle->type == WINSYS_HANDLE_TYPE_FD || whandle->type == ZINK_EXTERNAL_MEMORY_HANDLE)
         else
      }
   if (needs_export) {
      #if !defined(_WIN32)
         #else
         #endif
         } else if (screen->info.have_EXT_external_memory_dma_buf) {
      external = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
      } else {
      /* can't export anything, fail early */
                  /* we may export WINSYS_HANDLE_TYPE_FD handle which is dma-buf */
   if (shared && screen->info.have_EXT_external_memory_dma_buf)
            pipe_reference_init(&obj->reference, 1);
   if (loader_private) {
      obj->bo = CALLOC_STRUCT(zink_bo);
   if (!obj->bo) {
      mesa_loge("ZINK: failed to allocate obj->bo!");
      }
         obj->transfer_dst = true;
      } else if (templ->target == PIPE_BUFFER) {
      VkBufferCreateInfo bci = create_bci(screen, templ, templ->bind);
            if (user_mem) {
      embci.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
   embci.pNext = bci.pNext;
   embci.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
               if (VKSCR(CreateBuffer)(screen->dev, &bci, NULL, &obj->buffer) != VK_SUCCESS) {
      mesa_loge("ZINK: vkCreateBuffer failed");
               if (!(templ->bind & (PIPE_BIND_SHADER_IMAGE | ZINK_BIND_DESCRIPTOR))) {
      bci.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
   if (VKSCR(CreateBuffer)(screen->dev, &bci, NULL, &obj->storage_buffer) != VK_SUCCESS) {
      mesa_loge("ZINK: vkCreateBuffer failed");
                  if (modifiers_count) {
      assert(modifiers_count == 3);
   /* this is the DGC path because there's no other way to pass mem bits and I don't wanna copy/paste everything around */
   reqs.size = modifiers[0];
   reqs.alignment = modifiers[1];
      } else {
         }
   if (templ->usage == PIPE_USAGE_STAGING)
         else if (templ->usage == PIPE_USAGE_STREAM)
         else if (templ->usage == PIPE_USAGE_IMMUTABLE)
         else
         obj->is_buffer = true;
   obj->transfer_dst = true;
   obj->vkflags = bci.flags;
   obj->vkusage = bci.usage;
      } else {
      max_level = templ->last_level + 1;
   bool winsys_modifier = (export_types & VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT) && whandle && whandle->modifier != DRM_FORMAT_MOD_INVALID;
   uint64_t *ici_modifiers = winsys_modifier ? &whandle->modifier : modifiers;
   unsigned ici_modifier_count = winsys_modifier ? 1 : modifiers_count;
   bool success = false;
   VkImageCreateInfo ici;
   enum pipe_format srgb = PIPE_FORMAT_NONE;
   /* we often need to be able to mutate between srgb and linear, but we don't need general
   * image view/shader image format compatibility (that path means losing fast clears or compression on some hardware).
   */
   if (!(templ->bind & ZINK_BIND_MUTABLE)) {
      srgb = util_format_is_srgb(templ->format) ? util_format_linear(templ->format) : util_format_srgb(templ->format);
   /* why do these helpers have different default return values? */
   if (srgb == templ->format)
      }
   VkFormat formats[2];
   VkImageFormatListCreateInfo format_list;
   if (srgb) {
      formats[0] = zink_get_format(screen, templ->format);
   formats[1] = zink_get_format(screen, srgb);
   /* only use format list if both formats have supported vk equivalents */
   if (formats[0] && formats[1]) {
      format_list.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
   format_list.pNext = NULL;
                     } else {
            } else {
         }
   init_ici(screen, &ici, templ, templ->bind, ici_modifier_count);
   uint64_t mod = eval_ici(screen, &ici, templ, templ->bind, ici_modifier_count, ici_modifiers, &success);
   if (ici.format == VK_FORMAT_A8_UNORM_KHR && !success) {
      ici.format = zink_get_format(screen, zink_format_get_emulated_alpha(templ->format));
      }
   if (ici.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT && srgb &&
      util_format_get_nr_components(srgb) == 4 &&
   !(ici.flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)) {
   mesa_loge("zink: refusing to create possibly-srgb dmabuf due to missing driver support: %s not supported!", util_format_name(srgb));
      }
   VkExternalMemoryImageCreateInfo emici;
   VkImageDrmFormatModifierExplicitCreateInfoEXT idfmeci;
   VkImageDrmFormatModifierListCreateInfoEXT idfmlci;
   VkSubresourceLayout plane_layouts[4];
   VkSubresourceLayout plane_layout = {
      .offset = whandle ? whandle->offset : 0,
   .size = 0,
   .rowPitch = whandle ? whandle->stride : 0,
   .arrayPitch = 0,
      };
   if (!success)
                     if (shared || external) {
      emici.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
   emici.pNext = ici.pNext;
                  assert(ici.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT || mod != DRM_FORMAT_MOD_INVALID);
   if (whandle && ici.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      assert(mod == whandle->modifier || !winsys_modifier);
   idfmeci.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
                  idfmeci.drmFormatModifierPlaneCount = obj->plane_count;
   plane_layouts[0] = plane_layout;
   pnext = templ->next;
   for (unsigned i = 1; i < obj->plane_count; i++, pnext = pnext->next) {
      struct zink_resource *next = zink_resource(pnext);
   obj->plane_offsets[i] = plane_layouts[i].offset = next->obj->plane_offsets[i];
   obj->plane_strides[i] = plane_layouts[i].rowPitch = next->obj->plane_strides[i];
   plane_layouts[i].size = 0;
   plane_layouts[i].arrayPitch = 0;
                        } else if (ici.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      idfmlci.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT;
   idfmlci.pNext = ici.pNext;
   idfmlci.drmFormatModifierCount = modifiers_count;
   idfmlci.pDrmFormatModifiers = modifiers;
      } else if (ici.tiling == VK_IMAGE_TILING_OPTIMAL) {
            } else if (user_mem) {
      emici.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
   emici.pNext = ici.pNext;
   emici.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
               if (linear)
            if (ici.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
      #if defined(ZINK_USE_DMABUF) && !defined(_WIN32)
         if (obj->is_aux) {
      obj->modifier = mod;
   obj->modifier_aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT << whandle->plane;
   obj->plane_offsets[whandle->plane] = whandle->offset;
   obj->plane_strides[whandle->plane] = whandle->stride;
   obj->handle = os_dupfd_cloexec(whandle->handle);
   if (obj->handle < 0) {
      mesa_loge("ZINK: failed to dup dmabuf fd: %s\n", strerror(errno));
      }
      #endif
            VkFormatFeatureFlags feats = 0;
   switch (ici.tiling) {
   case VK_IMAGE_TILING_LINEAR:
      feats = screen->format_props[templ->format].linearTilingFeatures;
      case VK_IMAGE_TILING_OPTIMAL:
      feats = screen->format_props[templ->format].optimalTilingFeatures;
      case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
      feats = VK_FORMAT_FEATURE_FLAG_BITS_MAX_ENUM;
   /*
      If is tiling then VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, the value of
   imageCreateFormatFeatures is found by calling vkGetPhysicalDeviceFormatProperties2
   with VkImageFormatProperties::format equal to VkImageCreateInfo::format and with
   VkDrmFormatModifierPropertiesListEXT chained into VkImageFormatProperties2; by
   collecting all members of the returned array
   VkDrmFormatModifierPropertiesListEXT::pDrmFormatModifierProperties
   whose drmFormatModifier belongs to imageCreateDrmFormatModifiers; and by taking the bitwise
   intersection, over the collected array members, of drmFormatModifierTilingFeatures.
   (The resultant imageCreateFormatFeatures may be empty).
      */
   for (unsigned i = 0; i < screen->modifier_props[templ->format].drmFormatModifierCount; i++)
            default:
         }
   obj->vkfeats = feats;
   if (util_format_is_yuv(templ->format)) {
      if (feats & VK_FORMAT_FEATURE_DISJOINT_BIT)
         VkSamplerYcbcrConversionCreateInfo sycci = {0};
   sycci.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
   sycci.pNext = NULL;
   sycci.format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
   sycci.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
   sycci.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
   sycci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   sycci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   sycci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   sycci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   if (!feats || (feats & VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT)) {
      sycci.xChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN;
      } else {
      assert(feats & VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT);
   sycci.xChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
      }
   sycci.chromaFilter = VK_FILTER_LINEAR;
   sycci.forceExplicitReconstruction = VK_FALSE;
   VkResult res = VKSCR(CreateSamplerYcbcrConversion)(screen->dev, &sycci, NULL, &obj->sampler_conversion);
   if (res != VK_SUCCESS) {
      mesa_loge("ZINK: vkCreateSamplerYcbcrConversion failed");
         } else if (whandle) {
                  VkResult result = VKSCR(CreateImage)(screen->dev, &ici, NULL, &obj->image);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkCreateImage failed (%s)", vk_Result_to_str(result));
               if (ici.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      VkImageDrmFormatModifierPropertiesEXT modprops = {0};
   modprops.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT;
   result = VKSCR(GetImageDrmFormatModifierPropertiesEXT)(screen->dev, obj->image, &modprops);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkGetImageDrmFormatModifierPropertiesEXT failed");
      }
   obj->modifier = modprops.drmFormatModifier;
   unsigned num_dmabuf_planes = screen->base.get_dmabuf_modifier_planes(&screen->base, obj->modifier, templ->format);
   obj->modifier_aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
   if (num_dmabuf_planes > 1)
         if (num_dmabuf_planes > 2)
         if (num_dmabuf_planes > 3)
                     if (VKSCR(GetImageMemoryRequirements2)) {
      VkMemoryRequirements2 req2;
   req2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
   VkImageMemoryRequirementsInfo2 info2;
   info2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
   info2.pNext = NULL;
   info2.image = obj->image;
   VkMemoryDedicatedRequirements ded;
   ded.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
   ded.pNext = NULL;
   req2.pNext = &ded;
   VkImagePlaneMemoryRequirementsInfo plane;
   plane.sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO;
   plane.pNext = NULL;
   if (num_planes > 1)
         unsigned offset = 0;
   for (unsigned i = 0; i < num_planes; i++) {
      assert(i < ARRAY_SIZE(plane_aspects));
   plane.planeAspect = plane_aspects[i];
   VKSCR(GetImageMemoryRequirements2)(screen->dev, &info2, &req2);
   if (!i)
         obj->plane_offsets[i] = offset;
   offset += req2.memoryRequirements.size;
   reqs.size += req2.memoryRequirements.size;
   reqs.memoryTypeBits |= req2.memoryRequirements.memoryTypeBits;
         } else {
         }
   if (templ->usage == PIPE_USAGE_STAGING && ici.tiling == VK_IMAGE_TILING_LINEAR)
   flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
   else
            obj->vkflags = ici.flags;
      }
            if (templ->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT || templ->usage == PIPE_USAGE_DYNAMIC)
         else if (!(flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
                  if (templ->bind & ZINK_BIND_TRANSIENT)
            if (user_mem) {
      VkExternalMemoryHandleTypeFlagBits handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
   VkMemoryHostPointerPropertiesEXT memory_host_pointer_properties = {0};
   memory_host_pointer_properties.sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
   memory_host_pointer_properties.pNext = NULL;
   VkResult res = VKSCR(GetMemoryHostPointerPropertiesEXT)(screen->dev, handle_type, user_mem, &memory_host_pointer_properties);
   if (res != VK_SUCCESS) {
      mesa_loge("ZINK: vkGetMemoryHostPointerPropertiesEXT failed");
      }
   reqs.memoryTypeBits &= memory_host_pointer_properties.memoryTypeBits;
               VkMemoryAllocateInfo mai;
   enum zink_alloc_flag aflags = templ->flags & PIPE_RESOURCE_FLAG_SPARSE ? ZINK_ALLOC_SPARSE : 0;
   mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   mai.pNext = NULL;
   mai.allocationSize = reqs.size;
            VkMemoryDedicatedAllocateInfo ded_alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
   .pNext = mai.pNext,
   .image = obj->image,
               if (screen->info.have_KHR_dedicated_allocation && need_dedicated) {
      ded_alloc_info.pNext = mai.pNext;
               VkExportMemoryAllocateInfo emai;
   if ((templ->bind & ZINK_BIND_VIDEO) || ((templ->bind & PIPE_BIND_SHARED) && shared) || (templ->bind & ZINK_BIND_DMABUF)) {
      emai.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            emai.pNext = mai.pNext;
   mai.pNext = &emai;
            #ifdef ZINK_USE_DMABUF
      #if !defined(_WIN32)
      VkImportMemoryFdInfoKHR imfi = {
      VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
               if (whandle) {
      imfi.pNext = NULL;
   imfi.handleType = external;
   imfi.fd = os_dupfd_cloexec(whandle->handle);
   if (imfi.fd < 0) {
      mesa_loge("ZINK: failed to dup dmabuf fd: %s\n", strerror(errno));
               imfi.pNext = mai.pNext;
         #else
      VkImportMemoryWin32HandleInfoKHR imfi = {
      VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
               if (whandle) {
      HANDLE source_target = GetCurrentProcess();
                     if (!result || !out_handle) {
      mesa_loge("ZINK: failed to DuplicateHandle with winerr: %08x\n", (int)GetLastError());
               imfi.pNext = NULL;
   imfi.handleType = external;
            imfi.pNext = mai.pNext;
         #endif
      #endif
         VkImportMemoryHostPointerInfoEXT imhpi = {
      VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
      };
   if (user_mem) {
      imhpi.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
   imhpi.pHostPointer = (void*)user_mem;
   imhpi.pNext = mai.pNext;
               unsigned alignment = MAX2(reqs.alignment, 256);
   if (templ->usage == PIPE_USAGE_STAGING && obj->is_buffer)
                  if (zink_mem_type_idx_from_types(screen, heap, reqs.memoryTypeBits) == UINT32_MAX) {
      /* not valid based on reqs; demote to more compatible type */
   switch (heap) {
   case ZINK_HEAP_DEVICE_LOCAL_VISIBLE:
      heap = ZINK_HEAP_DEVICE_LOCAL;
      case ZINK_HEAP_HOST_VISIBLE_COHERENT_CACHED:
      heap = ZINK_HEAP_HOST_VISIBLE_COHERENT;
      default:
         }
            retry:
      /* iterate over all available memory types to reduce chance of oom */
   for (unsigned i = 0; !obj->bo && i < screen->heap_count[heap]; i++) {
      if (!(reqs.memoryTypeBits & BITFIELD_BIT(screen->heap_map[heap][i])))
            mai.memoryTypeIndex = screen->heap_map[heap][i];
   obj->bo = zink_bo(zink_bo_create(screen, reqs.size, alignment, heap, mai.pNext ? ZINK_ALLOC_NO_SUBALLOC : 0, mai.memoryTypeIndex, mai.pNext));
   if (!obj->bo) {
      if (heap == ZINK_HEAP_DEVICE_LOCAL_VISIBLE) {
      /* demote BAR allocations to a different heap on failure to avoid oom */
   if (templ->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT || templ->usage == PIPE_USAGE_DYNAMIC)
         else
                  }
   if (!obj->bo)
         if (aflags == ZINK_ALLOC_SPARSE) {
         } else {
      obj->offset = zink_bo_get_offset(obj->bo);
      }
   if (zink_debug & ZINK_DEBUG_MEM) {
      char buf[4096];
   unsigned idx = 0;
   if (obj->is_buffer) {
      size_t size = (size_t)DIV_ROUND_UP(obj->size, 1024);
   if (templ->bind == PIPE_BIND_QUERY_BUFFER && templ->usage == PIPE_USAGE_STAGING) //internal qbo
         else
      } else {
         }
   /*
   zink_vkflags_func flag_func = obj->is_buffer ? (zink_vkflags_func)vk_BufferCreateFlagBits_to_str : (zink_vkflags_func)vk_ImageCreateFlagBits_to_str;
   zink_vkflags_func usage_func = obj->is_buffer ? (zink_vkflags_func)vk_BufferUsageFlagBits_to_str : (zink_vkflags_func)vk_ImageUsageFlagBits_to_str;
   if (obj->vkflags) {
      buf[idx++] = '[';
   idx += zink_string_vkflags_unroll(&buf[idx], sizeof(buf) - idx, obj->vkflags, flag_func);
      }
   if (obj->vkusage) {
      buf[idx++] = '[';
   idx += zink_string_vkflags_unroll(&buf[idx], sizeof(buf) - idx, obj->vkusage, usage_func);
      }
   */
   buf[idx] = 0;
               obj->coherent = screen->info.mem_props.memoryTypes[obj->bo->base.placement].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   if (!(templ->flags & PIPE_RESOURCE_FLAG_SPARSE)) {
                  if (templ->target == PIPE_BUFFER) {
      if (!(templ->flags & PIPE_RESOURCE_FLAG_SPARSE)) {
      if (VKSCR(BindBufferMemory)(screen->dev, obj->buffer, zink_bo_get_mem(obj->bo), obj->offset) != VK_SUCCESS) {
      mesa_loge("ZINK: vkBindBufferMemory failed");
      }
   if (obj->storage_buffer && VKSCR(BindBufferMemory)(screen->dev, obj->storage_buffer, zink_bo_get_mem(obj->bo), obj->offset) != VK_SUCCESS) {
      mesa_loge("ZINK: vkBindBufferMemory failed");
            } else {
      if (num_planes > 1) {
      VkBindImageMemoryInfo infos[3];
   VkBindImagePlaneMemoryInfo planes[3];
   for (unsigned i = 0; i < num_planes; i++) {
      infos[i].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
   infos[i].image = obj->image;
   infos[i].memory = zink_bo_get_mem(obj->bo);
   infos[i].memoryOffset = obj->plane_offsets[i];
   if (templ->bind & ZINK_BIND_VIDEO) {
      infos[i].pNext = &planes[i];
   planes[i].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
   planes[i].pNext = NULL;
         }
   if (VKSCR(BindImageMemory2)(screen->dev, num_planes, infos) != VK_SUCCESS) {
      mesa_loge("ZINK: vkBindImageMemory2 failed");
         } else {
      if (!(templ->flags & PIPE_RESOURCE_FLAG_SPARSE))
      if (VKSCR(BindImageMemory)(screen->dev, obj->image, zink_bo_get_mem(obj->bo), obj->offset) != VK_SUCCESS) {
      mesa_loge("ZINK: vkBindImageMemory failed");
                  for (unsigned i = 0; i < max_level; i++)
               fail3:
            fail2:
      if (templ->target == PIPE_BUFFER) {
      VKSCR(DestroyBuffer)(screen->dev, obj->buffer, NULL);
      } else
      fail1:
      FREE(obj);
      }
      static struct pipe_resource *
   resource_create(struct pipe_screen *pscreen,
                  const struct pipe_resource *templ,
   struct winsys_handle *whandle,
      {
      struct zink_screen *screen = zink_screen(pscreen);
            if (!res) {
      mesa_loge("ZINK: failed to allocate res!");
               if (modifiers_count > 0 && screen->info.have_EXT_image_drm_format_modifier) {
      /* for rebinds */
   res->modifiers_count = modifiers_count;
   res->modifiers = mem_dup(modifiers, modifiers_count * sizeof(uint64_t));
   if (!res->modifiers) {
      FREE_CL(res);
                           bool allow_cpu_storage = (templ->target == PIPE_BUFFER) &&
         threaded_resource_init(&res->base.b, allow_cpu_storage);
   pipe_reference_init(&res->base.b.reference, 1);
            bool linear = false;
   struct pipe_resource templ2 = *templ;
   if (templ2.flags & PIPE_RESOURCE_FLAG_SPARSE)
         if (screen->faked_e5sparse && templ->format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
      templ2.flags &= ~PIPE_RESOURCE_FLAG_SPARSE;
      }
   res->obj = resource_object_create(screen, &templ2, whandle, &linear, res->modifiers, res->modifiers_count, loader_private, user_mem);
   if (!res->obj) {
      free(res->modifiers);
   FREE_CL(res);
               res->queue = VK_QUEUE_FAMILY_IGNORED;
   res->internal_format = templ->format;
   if (templ->target == PIPE_BUFFER) {
      util_range_init(&res->valid_buffer_range);
   res->base.b.bind |= PIPE_BIND_SHADER_IMAGE;
   if (!screen->resizable_bar && templ->width0 >= 8196) {
      /* We don't want to evict buffers from VRAM by mapping them for CPU access,
   * because they might never be moved back again. If a buffer is large enough,
   * upload data by copying from a temporary GTT buffer. 8K might not seem much,
   * but there can be 100000 buffers.
   *
   * This tweak improves performance for viewperf.
   */
      }
   if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB || zink_debug & ZINK_DEBUG_DGC)
      } else {
      if (templ->flags & PIPE_RESOURCE_FLAG_SPARSE)
         if (templ->flags & PIPE_RESOURCE_FLAG_SPARSE) {
      uint32_t count = 1;
   VKSCR(GetImageSparseMemoryRequirements)(screen->dev, res->obj->image, &count, &res->sparse);
      }
   res->format = zink_get_format(screen, templ->format);
   if (templ->target == PIPE_TEXTURE_1D || templ->target == PIPE_TEXTURE_1D_ARRAY) {
      res->need_2D = (screen->need_2D_zs && util_format_is_depth_or_stencil(templ->format)) ||
      }
   res->dmabuf = whandle && whandle->type == WINSYS_HANDLE_TYPE_FD;
   if (res->dmabuf)
         res->layout = res->dmabuf ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;
   res->linear = linear;
               if (loader_private) {
      if (templ->bind & PIPE_BIND_DISPLAY_TARGET) {
      /* backbuffer */
   res->obj->dt = zink_kopper_displaytarget_create(screen,
                                       if (!res->obj->dt) {
      mesa_loge("zink: could not create swapchain");
   FREE(res->obj);
   free(res->modifiers);
   FREE_CL(res);
      }
   struct kopper_displaytarget *cdt = res->obj->dt;
   if (cdt->swapchain->num_acquires) {
      /* this should be a reused swapchain after a MakeCurrent dance that deleted the original resource */
   for (unsigned i = 0; i < cdt->swapchain->num_images; i++) {
      if (!cdt->swapchain->images[i].acquired)
         res->obj->dt_idx = i;
   res->obj->image = cdt->swapchain->images[i].image;
            } else {
      /* frontbuffer */
   struct zink_resource *back = (void*)loader_private;
   struct kopper_displaytarget *cdt = back->obj->dt;
   cdt->refcount++;
   assert(back->obj->dt);
      }
   struct kopper_displaytarget *cdt = res->obj->dt;
   if (zink_kopper_has_srgb(cdt))
         if (cdt->swapchain->scci.flags == VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR)
         res->obj->vkusage = cdt->swapchain->scci.imageUsage;
   res->base.b.bind |= PIPE_BIND_DISPLAY_TARGET;
   res->linear = false;
               if (!res->obj->host_visible) {
      res->base.b.flags |= PIPE_RESOURCE_FLAG_DONT_MAP_DIRECTLY;
      }
   if (res->obj->is_buffer) {
      res->base.buffer_id_unique = util_idalloc_mt_alloc(&screen->buffer_ids);
   _mesa_hash_table_init(&res->bufferview_cache, NULL, NULL, equals_bvci);
      } else {
      _mesa_hash_table_init(&res->surface_cache, NULL, NULL, equals_ivci);
      }
   if (res->obj->exportable)
            }
      static struct pipe_resource *
   zink_resource_create(struct pipe_screen *pscreen,
         {
         }
      static struct pipe_resource *
   zink_resource_create_with_modifiers(struct pipe_screen *pscreen, const struct pipe_resource *templ,
         {
         }
      static struct pipe_resource *
   zink_resource_create_drawable(struct pipe_screen *pscreen,
               {
         }
      static bool
   add_resource_bind(struct zink_context *ctx, struct zink_resource *res, unsigned bind)
   {
      struct zink_screen *screen = zink_screen(ctx->base.screen);
   assert((res->base.b.bind & bind) == 0);
   res->base.b.bind |= bind;
   struct zink_resource_object *old_obj = res->obj;
   if (bind & ZINK_BIND_DMABUF && !res->modifiers_count && screen->info.have_EXT_image_drm_format_modifier) {
      res->modifiers_count = 1;
   res->modifiers = malloc(res->modifiers_count * sizeof(uint64_t));
   if (!res->modifiers) {
      mesa_loge("ZINK: failed to allocate res->modifiers!");
                  }
   struct zink_resource_object *new_obj = resource_object_create(screen, &res->base.b, NULL, &res->linear, res->modifiers, res->modifiers_count, NULL, NULL);
   if (!new_obj) {
      debug_printf("new backing resource alloc failed!\n");
   res->base.b.bind &= ~bind;
      }
   struct zink_resource staging = *res;
   staging.obj = old_obj;
   staging.all_binds = 0;
   res->layout = VK_IMAGE_LAYOUT_UNDEFINED;
   res->obj = new_obj;
   res->queue = VK_QUEUE_FAMILY_IGNORED;
   for (unsigned i = 0; i <= res->base.b.last_level; i++) {
      struct pipe_box box = {0, 0, 0,
               box.depth = util_num_layers(&res->base.b, i);
      }
   zink_resource_object_reference(screen, &old_obj, NULL);
      }
      static bool
   zink_resource_get_param(struct pipe_screen *pscreen, struct pipe_context *pctx,
                           struct pipe_resource *pres,
   unsigned plane,
   unsigned layer,
   {
      struct zink_screen *screen = zink_screen(pscreen);
   struct zink_resource *res = zink_resource(pres);
   struct zink_resource_object *obj = res->obj;
   struct winsys_handle whandle;
   VkImageAspectFlags aspect;
   if (obj->modifier_aspect) {
      switch (plane) {
   case 0:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
      case 1:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT;
      case 2:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT;
      case 3:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT;
      default:
            } else if (res->obj->sampler_conversion) {
         } else {
         }
   switch (param) {
   case PIPE_RESOURCE_PARAM_NPLANES:
      if (screen->info.have_EXT_image_drm_format_modifier)
         else
               case PIPE_RESOURCE_PARAM_STRIDE: {
      VkImageSubresource sub_res = {0};
                              *value = sub_res_layout.rowPitch;
               case PIPE_RESOURCE_PARAM_OFFSET: {
         VkImageSubresource isr = {
      aspect,
   level,
      };
   VkSubresourceLayout srl;
   VKSCR(GetImageSubresourceLayout)(screen->dev, obj->image, &isr, &srl);
   *value = srl.offset;
            case PIPE_RESOURCE_PARAM_MODIFIER: {
      *value = obj->modifier;
               case PIPE_RESOURCE_PARAM_LAYER_STRIDE: {
         VkImageSubresource isr = {
      aspect,
   level,
      };
   VkSubresourceLayout srl;
   VKSCR(GetImageSubresourceLayout)(screen->dev, obj->image, &isr, &srl);
   if (res->base.b.target == PIPE_TEXTURE_3D)
         else
                        case PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS:
   case PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED:
      #ifdef ZINK_USE_DMABUF
         memset(&whandle, 0, sizeof(whandle));
   if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED)
         if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS)
         else if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD)
            if (!pscreen->resource_get_handle(pscreen, pctx, pres, &whandle, handle_usage))
      #ifdef _WIN32
         #else
         #endif
         #else
         (void)whandle;
   #endif
      }
   }
      }
      static bool
   zink_resource_get_handle(struct pipe_screen *pscreen,
                           {
      if (tex->target == PIPE_BUFFER)
            #ifdef ZINK_USE_DMABUF
         struct zink_resource *res = zink_resource(tex);
   struct zink_screen *screen = zink_screen(pscreen);
      #if !defined(_WIN32)
         if (whandle->type == WINSYS_HANDLE_TYPE_KMS && screen->drm_fd == -1) {
         } else {
      if (!res->obj->exportable) {
      assert(!zink_resource_usage_is_unflushed(res));
   if (!screen->info.have_EXT_image_drm_format_modifier) {
      static bool warned = false;
   warn_missing_feature(warned, "EXT_image_drm_format_modifier");
      }
   unsigned bind = ZINK_BIND_DMABUF;
   if (!(res->base.b.bind & PIPE_BIND_SHARED))
         zink_screen_lock_context(screen);
   if (!add_resource_bind(screen->copy_context, res, bind)) {
      zink_screen_unlock_context(screen);
      }
   if (res->all_binds)
         screen->copy_context->base.flush(&screen->copy_context->base, NULL, 0);
   zink_screen_unlock_context(screen);
               VkMemoryGetFdInfoKHR fd_info = {0};
   int fd;
   fd_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
   fd_info.memory = zink_bo_get_mem(obj->bo);
   if (whandle->type == WINSYS_HANDLE_TYPE_FD)
         else
         VkResult result = VKSCR(GetMemoryFdKHR)(screen->dev, &fd_info, &fd);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkGetMemoryFdKHR failed");
      }
   if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
      uint32_t h;
   bool ret = zink_bo_get_kms_handle(screen, obj->bo, fd, &h);
   close(fd);
   if (!ret)
                        #else
         VkMemoryGetWin32HandleInfoKHR handle_info = {0};
   HANDLE handle;
   handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
   //TODO: remove for wsi
   handle_info.memory = zink_bo_get_mem(obj->bo);
   handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
   VkResult result = VKSCR(GetMemoryWin32HandleKHR)(screen->dev, &handle_info, &handle);
   if (result != VK_SUCCESS)
         #endif
         uint64_t value;
   zink_resource_get_param(pscreen, context, tex, 0, 0, 0, PIPE_RESOURCE_PARAM_MODIFIER, 0, &value);
   whandle->modifier = value;
   zink_resource_get_param(pscreen, context, tex, 0, 0, 0, PIPE_RESOURCE_PARAM_OFFSET, 0, &value);
   whandle->offset = value;
   zink_resource_get_param(pscreen, context, tex, 0, 0, 0, PIPE_RESOURCE_PARAM_STRIDE, 0, &value);
   #else
         #endif
      }
      }
      static struct pipe_resource *
   zink_resource_from_handle(struct pipe_screen *pscreen,
                     {
   #ifdef ZINK_USE_DMABUF
      if (whandle->modifier != DRM_FORMAT_MOD_INVALID &&
      !zink_screen(pscreen)->info.have_EXT_image_drm_format_modifier)
         struct pipe_resource templ2 = *templ;
   if (templ->format == PIPE_FORMAT_NONE)
            uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
   int modifier_count = 1;
   if (whandle->modifier != DRM_FORMAT_MOD_INVALID)
         else {
      if (!zink_screen(pscreen)->driver_workarounds.can_do_invalid_linear_modifier) {
      mesa_loge("zink: display server doesn't support DRI3 modifiers and driver can't handle INVALID<->LINEAR!");
      }
      }
   templ2.bind |= ZINK_BIND_DMABUF;
   struct pipe_resource *pres = resource_create(pscreen, &templ2, whandle, usage, &modifier, modifier_count, NULL, NULL);
   if (pres) {
      struct zink_resource *res = zink_resource(pres);
   if (pres->target != PIPE_BUFFER)
         else
            }
      #else
         #endif
   }
      static struct pipe_resource *
   zink_resource_from_user_memory(struct pipe_screen *pscreen,
               {
         }
      struct zink_memory_object {
      struct pipe_memory_object b;
      };
      static struct pipe_memory_object *
   zink_memobj_create_from_handle(struct pipe_screen *pscreen, struct winsys_handle *whandle, bool dedicated)
   {
      struct zink_memory_object *memobj = CALLOC_STRUCT(zink_memory_object);
   if (!memobj)
         memcpy(&memobj->whandle, whandle, sizeof(struct winsys_handle));
         #ifdef ZINK_USE_DMABUF
      #if !defined(_WIN32)
         #else
      HANDLE source_target = GetCurrentProcess();
            DuplicateHandle(source_target, whandle->handle, source_target, &out_handle, 0, false, DUPLICATE_SAME_ACCESS);
         #endif /* _WIN32 */
   #endif /* ZINK_USE_DMABUF */
            }
      static void
   zink_memobj_destroy(struct pipe_screen *pscreen, struct pipe_memory_object *pmemobj)
   {
   #ifdef ZINK_USE_DMABUF
            #if !defined(_WIN32)
         #else
         #endif /* _WIN32 */
   #endif /* ZINK_USE_DMABUF */
            }
      static struct pipe_resource *
   zink_resource_from_memobj(struct pipe_screen *pscreen,
                     {
               struct pipe_resource *pres = resource_create(pscreen, templ, &memobj->whandle, 0, NULL, 0, NULL, NULL);
   if (pres) {
      if (pres->target != PIPE_BUFFER)
         else
      }
      }
      static bool
   invalidate_buffer(struct zink_context *ctx, struct zink_resource *res)
   {
                        if (res->base.b.flags & PIPE_RESOURCE_FLAG_SPARSE)
            struct pipe_box box = {0, 0, 0, res->base.b.width0, 0, 0};
   if (res->valid_buffer_range.start > res->valid_buffer_range.end &&
      !zink_resource_copy_box_intersects(res, 0, &box))
         if (res->so_valid)
         /* force counter buffer reset */
            util_range_set_empty(&res->valid_buffer_range);
   if (!zink_resource_has_usage(res))
            struct zink_resource_object *new_obj = resource_object_create(screen, &res->base.b, NULL, NULL, NULL, 0, NULL, 0);
   if (!new_obj) {
      debug_printf("new backing resource alloc failed!\n");
      }
   bool needs_bda = !!res->obj->bda;
   /* this ref must be transferred before rebind or else BOOM */
   zink_batch_reference_resource_move(&ctx->batch, res);
   res->obj = new_obj;
   res->queue = VK_QUEUE_FAMILY_IGNORED;
   if (needs_bda)
         zink_resource_rebind(ctx, res);
      }
         static void
   zink_resource_invalidate(struct pipe_context *pctx, struct pipe_resource *pres)
   {
      if (pres->target == PIPE_BUFFER)
         else {
      struct zink_resource *res = zink_resource(pres);
   if (res->valid && res->fb_bind_count)
               }
      static void
   zink_transfer_copy_bufimage(struct zink_context *ctx,
                     {
      assert((trans->base.b.usage & (PIPE_MAP_DEPTH_ONLY | PIPE_MAP_STENCIL_ONLY)) !=
                     struct pipe_box box = trans->base.b.box;
   int x = box.x;
   if (buf2img)
            if (dst->obj->transfer_dst)
      zink_copy_image_buffer(ctx, dst, src, trans->base.b.level, buf2img ? x : 0,
      else
      util_blitter_copy_texture(ctx->blitter, &dst->base.b, trans->base.b.level,
         }
      ALWAYS_INLINE static void
   align_offset_size(const VkDeviceSize alignment, VkDeviceSize *offset, VkDeviceSize *size, VkDeviceSize obj_size)
   {
      VkDeviceSize align = *offset % alignment;
   if (alignment - 1 > *offset)
         else
         align = alignment - (*size % alignment);
   if (*offset + *size + align > obj_size)
         else
      }
      VkMappedMemoryRange
   zink_resource_init_mem_range(struct zink_screen *screen, struct zink_resource_object *obj, VkDeviceSize offset, VkDeviceSize size)
   {
      assert(obj->size);
   align_offset_size(screen->info.props.limits.nonCoherentAtomSize, &offset, &size, obj->size);
   VkMappedMemoryRange range = {
      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
   NULL,
   zink_bo_get_mem(obj->bo),
   offset,
      };
   assert(range.size);
      }
      static void *
   map_resource(struct zink_screen *screen, struct zink_resource *res)
   {
      assert(res->obj->host_visible);
      }
      static void
   unmap_resource(struct zink_screen *screen, struct zink_resource *res)
   {
         }
      static struct zink_transfer *
   create_transfer(struct zink_context *ctx, struct pipe_resource *pres, unsigned usage, const struct pipe_box *box)
   {
               if (usage & PIPE_MAP_THREAD_SAFE)
         else if (usage & TC_TRANSFER_MAP_THREADED_UNSYNC)
         else
         if (!trans)
                     trans->base.b.usage = usage;
   trans->base.b.box = *box;
      }
      static void
   destroy_transfer(struct zink_context *ctx, struct zink_transfer *trans)
   {
      if (trans->base.b.usage & PIPE_MAP_THREAD_SAFE) {
         } else {
      /* Don't use pool_transfers_unsync. We are always in the driver
   * thread. Freeing an object into a different pool is allowed.
   */
         }
      static void *
   zink_buffer_map(struct pipe_context *pctx,
                     struct pipe_resource *pres,
   unsigned level,
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_resource *res = zink_resource(pres);
   struct zink_transfer *trans = create_transfer(ctx, pres, usage, box);
   if (!trans)
                     if (res->base.is_user_ptr)
            /* See if the buffer range being mapped has never been initialized,
   * in which case it can be mapped unsynchronized. */
   if (!(usage & (PIPE_MAP_UNSYNCHRONIZED | TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED)) &&
      usage & PIPE_MAP_WRITE && !res->base.is_shared &&
   !util_ranges_intersect(&res->valid_buffer_range, box->x, box->x + box->width) &&
   !zink_resource_copy_box_intersects(res, 0, box)) {
               /* If discarding the entire range, discard the whole resource instead. */
   if (usage & PIPE_MAP_DISCARD_RANGE && box->x == 0 && box->width == res->base.b.width0) {
                  /* If a buffer in VRAM is too large and the range is discarded, don't
   * map it directly. This makes sure that the buffer stays in VRAM.
   */
   bool force_discard_range = false;
   if (usage & (PIPE_MAP_DISCARD_WHOLE_RESOURCE | PIPE_MAP_DISCARD_RANGE) &&
      !(usage & PIPE_MAP_PERSISTENT) &&
   res->base.b.flags & PIPE_RESOURCE_FLAG_DONT_MAP_DIRECTLY) {
   usage &= ~(PIPE_MAP_DISCARD_WHOLE_RESOURCE | PIPE_MAP_UNSYNCHRONIZED);
   usage |= PIPE_MAP_DISCARD_RANGE;
               if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE &&
      !(usage & (PIPE_MAP_UNSYNCHRONIZED | TC_TRANSFER_MAP_NO_INVALIDATE))) {
            if (invalidate_buffer(ctx, res)) {
      /* At this point, the buffer is always idle. */
      } else {
      /* Fall back to a temporary buffer. */
                  unsigned map_offset = box->x;
   if (usage & PIPE_MAP_DISCARD_RANGE &&
      (!res->obj->host_visible ||
            /* Check if mapping this buffer would cause waiting for the GPU.
            if (!res->obj->host_visible || force_discard_range ||
      !zink_resource_usage_check_completion(screen, res, ZINK_RESOURCE_ACCESS_RW)) {
                  /* If we are not called from the driver thread, we have
   * to use the uploader from u_threaded_context, which is
   * local to the calling thread.
   */
   struct u_upload_mgr *mgr;
   if (usage & TC_TRANSFER_MAP_THREADED_UNSYNC)
         else
         u_upload_alloc(mgr, 0, box->width,
               res = zink_resource(trans->staging_res);
   trans->offset = offset;
   usage |= PIPE_MAP_UNSYNCHRONIZED;
      } else {
      /* At this point, the buffer is always idle (we checked it above). */
         } else if (usage & PIPE_MAP_DONTBLOCK) {
      /* sparse/device-local will always need to wait since it has to copy */
   if (!res->obj->host_visible)
         if (!zink_resource_usage_check_completion(screen, res, ZINK_RESOURCE_ACCESS_WRITE))
            } else if (!(usage & PIPE_MAP_UNSYNCHRONIZED) &&
            (((usage & PIPE_MAP_READ) && !(usage & PIPE_MAP_PERSISTENT) &&
         /* the above conditional catches uncached reads and non-HV writes */
   assert(!(usage & (TC_TRANSFER_MAP_THREADED_UNSYNC)));
   /* any read, non-HV write, or unmappable that reaches this point needs staging */
   overwrite:
            trans->offset = box->x % screen->info.props.limits.minMemoryMapAlignment;
   trans->staging_res = pipe_buffer_create(&screen->base, PIPE_BIND_LINEAR, PIPE_USAGE_STAGING, box->width + trans->offset);
   if (!trans->staging_res)
         struct zink_resource *staging_res = zink_resource(trans->staging_res);
   if (usage & PIPE_MAP_THREAD_SAFE) {
      /* this map can't access the passed context: use the copy context */
   zink_screen_lock_context(screen);
      }
   zink_copy_buffer(ctx, staging_res, res, trans->offset, box->x, box->width);
   res = staging_res;
   usage &= ~PIPE_MAP_UNSYNCHRONIZED;
         } else if ((usage & PIPE_MAP_UNSYNCHRONIZED) && !res->obj->host_visible) {
      trans->offset = box->x % screen->info.props.limits.minMemoryMapAlignment;
   trans->staging_res = pipe_buffer_create(&screen->base, PIPE_BIND_LINEAR, PIPE_USAGE_STAGING, box->width + trans->offset);
   if (!trans->staging_res)
         struct zink_resource *staging_res = zink_resource(trans->staging_res);
   res = staging_res;
               if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      if (usage & PIPE_MAP_WRITE) {
      if (!(usage & PIPE_MAP_READ)) {
      zink_resource_usage_try_wait(ctx, res, ZINK_RESOURCE_ACCESS_RW);
   if (zink_resource_has_unflushed_usage(res))
      }
      } else
         res->obj->access = 0;
   res->obj->access_stage = 0;
   res->obj->last_write = 0;
               if (!ptr) {
      /* if writing to a streamout buffer, ensure synchronization next time it's used */
   if (usage & PIPE_MAP_WRITE && res->so_valid) {
      ctx->dirty_so_targets = true;
   /* force counter buffer reset */
      }
   ptr = map_resource(screen, res);
   if (!ptr)
                        #if defined(MVK_VERSION)
         // Work around for MoltenVk limitation specifically on coherent memory
   // MoltenVk returns blank memory ranges when there should be data present
   // This is a known limitation of MoltenVK.
            #endif
         ) {
   VkDeviceSize size = box->width;
   VkDeviceSize offset = res->obj->offset + trans->offset;
   VkMappedMemoryRange range = zink_resource_init_mem_range(screen, res->obj, offset, size);
   if (VKSCR(InvalidateMappedMemoryRanges)(screen->dev, 1, &range) != VK_SUCCESS) {
      mesa_loge("ZINK: vkInvalidateMappedMemoryRanges failed");
   zink_bo_unmap(screen, res->obj->bo);
         }
   trans->base.b.usage = usage;
   if (usage & PIPE_MAP_WRITE)
         success:
      /* ensure the copy context gets unlocked */
   if (ctx == screen->copy_context)
         *transfer = &trans->base.b;
         fail:
      if (ctx == screen->copy_context)
         destroy_transfer(ctx, trans);
      }
      static void *
   zink_image_map(struct pipe_context *pctx,
                     struct pipe_resource *pres,
   unsigned level,
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_resource *res = zink_resource(pres);
   struct zink_transfer *trans = create_transfer(ctx, pres, usage, box);
   if (!trans)
            trans->base.b.level = level;
   if (zink_is_swapchain(res))
      /* this is probably a multi-chain which has already been acquired */
         void *ptr;
   if (usage & PIPE_MAP_WRITE && !(usage & PIPE_MAP_READ))
      /* this is like a blit, so we can potentially dump some clears or maybe we have to  */
      else if (usage & PIPE_MAP_READ)
      /* if the map region intersects with any clears then we have to apply them */
      if (!res->linear || !res->obj->host_visible) {
      enum pipe_format format = pres->format;
   if (usage & PIPE_MAP_DEPTH_ONLY)
         else if (usage & PIPE_MAP_STENCIL_ONLY)
         trans->base.b.stride = util_format_get_stride(format, box->width);
   trans->base.b.layer_stride = util_format_get_2d_size(format,
                  struct pipe_resource templ = *pres;
   templ.next = NULL;
   templ.format = format;
   templ.usage = usage & PIPE_MAP_READ ? PIPE_USAGE_STAGING : PIPE_USAGE_STREAM;
   templ.target = PIPE_BUFFER;
   templ.bind = PIPE_BIND_LINEAR;
   templ.width0 = trans->base.b.layer_stride * box->depth;
   templ.height0 = templ.depth0 = 0;
   templ.last_level = 0;
   templ.array_size = 1;
            trans->staging_res = zink_resource_create(pctx->screen, &templ);
   if (!trans->staging_res)
                     if (usage & PIPE_MAP_READ) {
      /* force multi-context sync */
   if (zink_resource_usage_is_unflushed_write(res))
         zink_transfer_copy_bufimage(ctx, staging_res, res, trans);
   /* need to wait for rendering to finish */
                  } else {
      assert(res->linear);
   ptr = map_resource(screen, res);
   if (!ptr)
         if (zink_resource_has_usage(res)) {
      if (usage & PIPE_MAP_WRITE)
         else
      }
   VkImageSubresource isr = {
      res->modifiers ? res->obj->modifier_aspect : res->aspect,
   level,
      };
   VkSubresourceLayout srl;
   VKSCR(GetImageSubresourceLayout)(screen->dev, res->obj->image, &isr, &srl);
   trans->base.b.stride = srl.rowPitch;
   if (res->base.b.target == PIPE_TEXTURE_3D)
         else
         trans->offset = srl.offset;
   trans->depthPitch = srl.depthPitch;
   const struct util_format_description *desc = util_format_description(res->base.b.format);
   unsigned offset = srl.offset +
                     if (!res->obj->coherent) {
      VkDeviceSize size = (VkDeviceSize)box->width * box->height * desc->block.bits / 8;
   VkMappedMemoryRange range = zink_resource_init_mem_range(screen, res->obj, res->obj->offset + offset, size);
   if (VKSCR(FlushMappedMemoryRanges)(screen->dev, 1, &range) != VK_SUCCESS) {
            }
      }
   if (!ptr)
         if (usage & PIPE_MAP_WRITE) {
      if (!res->valid && res->fb_bind_count)
                     if (sizeof(void*) == 4)
            *transfer = &trans->base.b;
         fail:
      destroy_transfer(ctx, trans);
      }
      static void
   zink_image_subdata(struct pipe_context *pctx,
                     struct pipe_resource *pres,
   unsigned level,
   unsigned usage,
   const struct pipe_box *box,
   {
      struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_context *ctx = zink_context(pctx);
            /* flush clears to avoid subdata conflict */
   if (res->obj->vkusage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT)
         /* only use HIC if supported on image and no pending usage */
   while (res->obj->vkusage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT &&
            /* uninit images are always supported */
   bool change_layout = res->layout == VK_IMAGE_LAYOUT_UNDEFINED || res->layout == VK_IMAGE_LAYOUT_PREINITIALIZED;
   if (!change_layout) {
      /* image in some other layout: test for support */
   bool can_copy_layout = false;
   for (unsigned i = 0; i < screen->info.hic_props.copyDstLayoutCount; i++) {
      if (screen->info.hic_props.pCopyDstLayouts[i] == res->layout) {
      can_copy_layout = true;
         }
   /* some layouts don't permit HIC copies */
   if (!can_copy_layout)
      }
   bool is_arrayed = false;
   switch (pres->target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      is_arrayed = true;
      default: break;
   }
   /* recalc strides into texel strides because HIC spec is insane */
   unsigned vk_stride = util_format_get_stride(pres->format, 1);
   stride /= vk_stride;
   unsigned vk_layer_stride = util_format_get_2d_size(pres->format, stride, 1) * vk_stride;
            VkHostImageLayoutTransitionInfoEXT t = {
      VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
   NULL,
   res->obj->image,
   res->layout,
   /* GENERAL support is guaranteed */
   VK_IMAGE_LAYOUT_GENERAL,
      };
   /* only pre-transition uninit images to avoid thrashing */
   if (change_layout) {
      VKSCR(TransitionImageLayoutEXT)(screen->dev, 1, &t);
      }
   VkMemoryToImageCopyEXT region = {
      VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT,
   NULL,
   data,
   stride,
   layer_stride,
   {res->aspect, level, is_arrayed ? box->z : 0, is_arrayed ? box->depth : 1},
   {box->x, box->y, is_arrayed ? 0 : box->z},
      };
   VkCopyMemoryToImageInfoEXT copy = {
      VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT,
   NULL,
   0,
   res->obj->image,
   res->layout,
   1,
      };
   VKSCR(CopyMemoryToImageEXT)(screen->dev, &copy);
   if (change_layout && screen->can_hic_shader_read && !pres->last_level && !box->x && !box->y && !box->z &&
      box->width == pres->width0 && box->height == pres->height0 &&
   ((is_arrayed && box->depth == pres->array_size) || (!is_arrayed && box->depth == pres->depth0))) {
   /* assume full copy single-mip images use shader read access */
   t.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
   t.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   VKSCR(TransitionImageLayoutEXT)(screen->dev, 1, &t);
   res->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
   /* make sure image is marked as having data */
   res->valid = true;
      }
   /* fallback case for per-resource unsupported or device-level unsupported */
      }
      static void
   zink_transfer_flush_region(struct pipe_context *pctx,
               {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_resource *res = zink_resource(ptrans->resource);
            if (trans->base.b.usage & PIPE_MAP_WRITE) {
      struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_resource *m = trans->staging_res ? zink_resource(trans->staging_res) :
         ASSERTED VkDeviceSize size, src_offset, dst_offset = 0;
   if (m->obj->is_buffer) {
      size = box->width;
   src_offset = box->x + (trans->staging_res ? trans->offset : ptrans->box.x);
      } else {
      size = (VkDeviceSize)box->width * box->height * util_format_get_blocksize(m->base.b.format);
   src_offset = trans->offset +
            box->z * trans->depthPitch +
         }
   if (!m->obj->coherent) {
      VkMappedMemoryRange range = zink_resource_init_mem_range(screen, m->obj, m->obj->offset, m->obj->size);
   if (VKSCR(FlushMappedMemoryRanges)(screen->dev, 1, &range) != VK_SUCCESS) {
            }
   if (trans->staging_res) {
               if (ptrans->resource->target == PIPE_BUFFER)
         else
            }
      /* used to determine whether to emit a TRANSFER_DST barrier on copies */
   bool
   zink_resource_copy_box_intersects(struct zink_resource *res, unsigned level, const struct pipe_box *box)
   {
      /* if there are no valid copy rects tracked, this needs a barrier */
   if (!res->obj->copies_valid)
         /* untracked huge miplevel */
   if (level >= ARRAY_SIZE(res->obj->copies))
         u_rwlock_rdlock(&res->obj->copy_lock);
   struct pipe_box *b = res->obj->copies[level].data;
   unsigned num_boxes = util_dynarray_num_elements(&res->obj->copies[level], struct pipe_box);
   bool (*intersect)(const struct pipe_box *, const struct pipe_box *);
   /* determine intersection function based on dimensionality */
   switch (res->base.b.target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      intersect = u_box_test_intersection_1d;
         case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D:
      intersect = u_box_test_intersection_2d;
         default:
      intersect = u_box_test_intersection_3d;
      }
   /* if any of the tracked boxes intersect with this one, a barrier is needed */
   bool ret = false;
   for (unsigned i = 0; i < num_boxes; i++) {
      if (intersect(box, b + i)) {
      ret = true;
         }
   u_rwlock_rdunlock(&res->obj->copy_lock);
   /* no intersection = no barrier */
      }
      /* track a new region for TRANSFER_DST barrier emission */
   void
   zink_resource_copy_box_add(struct zink_context *ctx, struct zink_resource *res, unsigned level, const struct pipe_box *box)
   {
      u_rwlock_wrlock(&res->obj->copy_lock);
   if (res->obj->copies_valid) {
      struct pipe_box *b = res->obj->copies[level].data;
   unsigned num_boxes = util_dynarray_num_elements(&res->obj->copies[level], struct pipe_box);
   for (unsigned i = 0; i < num_boxes; i++) {
      switch (res->base.b.target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      /* no-op included region */
                  /* try to merge adjacent regions */
   if (b[i].x == box->x + box->width) {
      b[i].x -= box->width;
   b[i].width += box->width;
      }
   if (b[i].x + b[i].width == box->x) {
                        /* try to merge into region */
   if (box->x <= b[i].x && box->x + box->width >= b[i].x + b[i].width) {
      *b = *box;
                  case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D:
      /* no-op included region */
   if (b[i].x <= box->x && b[i].x + b[i].width >= box->x + box->width &&
                  /* try to merge adjacent regions */
   if (b[i].y == box->y && b[i].height == box->height) {
      if (b[i].x == box->x + box->width) {
      b[i].x -= box->width;
   b[i].width += box->width;
      }
   if (b[i].x + b[i].width == box->x) {
      b[i].width += box->width;
         } else if (b[i].x == box->x && b[i].width == box->width) {
      if (b[i].y == box->y + box->height) {
      b[i].y -= box->height;
   b[i].height += box->height;
      }
   if (b[i].y + b[i].height == box->y) {
      b[i].height += box->height;
                  /* try to merge into region */
   if (box->x <= b[i].x && box->x + box->width >= b[i].x + b[i].width &&
      box->y <= b[i].y && box->y + box->height >= b[i].y + b[i].height) {
   *b = *box;
                  default:
      /* no-op included region */
   if (b[i].x <= box->x && b[i].x + b[i].width >= box->x + box->width &&
                              if (b[i].z == box->z && b[i].depth == box->depth) {
      if (b[i].y == box->y && b[i].height == box->height) {
      if (b[i].x == box->x + box->width) {
      b[i].x -= box->width;
   b[i].width += box->width;
      }
   if (b[i].x + b[i].width == box->x) {
      b[i].width += box->width;
         } else if (b[i].x == box->x && b[i].width == box->width) {
      if (b[i].y == box->y + box->height) {
      b[i].y -= box->height;
   b[i].height += box->height;
      }
   if (b[i].y + b[i].height == box->y) {
      b[i].height += box->height;
            } else if (b[i].x == box->x && b[i].width == box->width) {
      if (b[i].y == box->y && b[i].height == box->height) {
      if (b[i].z == box->z + box->depth) {
      b[i].z -= box->depth;
   b[i].depth += box->depth;
      }
   if (b[i].z + b[i].depth == box->z) {
      b[i].depth += box->depth;
         } else if (b[i].z == box->z && b[i].depth == box->depth) {
      if (b[i].y == box->y + box->height) {
      b[i].y -= box->height;
   b[i].height += box->height;
      }
   if (b[i].y + b[i].height == box->y) {
      b[i].height += box->height;
            } else if (b[i].y == box->y && b[i].height == box->height) {
      if (b[i].z == box->z && b[i].depth == box->depth) {
      if (b[i].x == box->x + box->width) {
      b[i].x -= box->width;
   b[i].width += box->width;
      }
   if (b[i].x + b[i].width == box->x) {
      b[i].width += box->width;
         } else if (b[i].x == box->x && b[i].width == box->width) {
      if (b[i].z == box->z + box->depth) {
      b[i].z -= box->depth;
   b[i].depth += box->depth;
      }
   if (b[i].z + b[i].depth == box->z) {
      b[i].depth += box->depth;
                     /* try to merge into region */
   if (box->x <= b[i].x && box->x + box->width >= b[i].x + b[i].width &&
                                 }
   util_dynarray_append(&res->obj->copies[level], struct pipe_box, *box);
   if (!res->copies_warned && util_dynarray_num_elements(&res->obj->copies[level], struct pipe_box) > 100) {
      perf_debug(ctx, "zink: PERF WARNING! > 100 copy boxes detected for %p\n", res);
   mesa_logw("zink: PERF WARNING! > 100 copy boxes detected for %p\n", res);
      }
      out:
         }
      void
   zink_resource_copies_reset(struct zink_resource *res)
   {
      if (!res->obj->copies_valid)
         u_rwlock_wrlock(&res->obj->copy_lock);
   unsigned max_level = res->base.b.target == PIPE_BUFFER ? 1 : (res->base.b.last_level + 1);
   if (res->base.b.target == PIPE_BUFFER) {
      /* flush transfer regions back to valid range on reset */
   struct pipe_box *b = res->obj->copies[0].data;
   unsigned num_boxes = util_dynarray_num_elements(&res->obj->copies[0], struct pipe_box);
   for (unsigned i = 0; i < num_boxes; i++)
      }
   for (unsigned i = 0; i < max_level; i++)
         res->obj->copies_valid = false;
   res->obj->copies_need_reset = false;
      }
      static void
   transfer_unmap(struct pipe_context *pctx, struct pipe_transfer *ptrans)
   {
      struct zink_context *ctx = zink_context(pctx);
            if (!(trans->base.b.usage & (PIPE_MAP_FLUSH_EXPLICIT | PIPE_MAP_COHERENT))) {
      /* flush_region is relative to the mapped region: use only the extents */
   struct pipe_box box = ptrans->box;
   box.x = box.y = box.z = 0;
               if (trans->staging_res)
                     }
      static void
   do_transfer_unmap(struct zink_screen *screen, struct zink_transfer *trans)
   {
      struct zink_resource *res = zink_resource(trans->staging_res);
   if (!res)
            }
      void
   zink_screen_buffer_unmap(struct pipe_screen *pscreen, struct pipe_transfer *ptrans)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   struct zink_transfer *trans = (struct zink_transfer *)ptrans;
   if (trans->base.b.usage & PIPE_MAP_ONCE && !trans->staging_res)
            }
      static void
   zink_buffer_unmap(struct pipe_context *pctx, struct pipe_transfer *ptrans)
   {
      struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_transfer *trans = (struct zink_transfer *)ptrans;
   if (trans->base.b.usage & PIPE_MAP_ONCE && !trans->staging_res)
            }
      static void
   zink_image_unmap(struct pipe_context *pctx, struct pipe_transfer *ptrans)
   {
      struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_transfer *trans = (struct zink_transfer *)ptrans;
   if (sizeof(void*) == 4)
            }
      static void
   zink_buffer_subdata(struct pipe_context *ctx, struct pipe_resource *buffer,
         {
      struct pipe_transfer *transfer = NULL;
   struct pipe_box box;
                     if (!(usage & PIPE_MAP_DIRECTLY))
            u_box_1d(offset, size, &box);
   map = zink_buffer_map(ctx, buffer, 0, usage, &box, &transfer);
   if (!map)
            memcpy(map, data, size);
      }
      static struct pipe_resource *
   zink_resource_get_separate_stencil(struct pipe_resource *pres)
   {
      /* For packed depth-stencil, we treat depth as the primary resource
   * and store S8 as the "second plane" resource.
   */
   if (pres->next && pres->next->format == PIPE_FORMAT_S8_UINT)
                  }
      static bool
   resource_object_add_bind(struct zink_context *ctx, struct zink_resource *res, unsigned bind)
   {
      /* base resource already has the cap */
   if (res->base.b.bind & bind)
         if (res->obj->is_buffer) {
      unreachable("zink: all buffers should have this bit");
      }
   assert(!res->obj->dt);
   zink_fb_clears_apply_region(ctx, &res->base.b, (struct u_rect){0, res->base.b.width0, 0, res->base.b.height0});
   bool ret = add_resource_bind(ctx, res, bind);
   if (ret)
               }
      bool
   zink_resource_object_init_storage(struct zink_context *ctx, struct zink_resource *res)
   {
         }
      bool
   zink_resource_object_init_mutable(struct zink_context *ctx, struct zink_resource *res)
   {
         }
      VkDeviceAddress
   zink_resource_get_address(struct zink_screen *screen, struct zink_resource *res)
   {
      assert(res->obj->is_buffer);
   if (!res->obj->bda) {
      VkBufferDeviceAddressInfo info = {
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
   NULL,
      };
      }
      }
      void
   zink_resource_setup_transfer_layouts(struct zink_context *ctx, struct zink_resource *src, struct zink_resource *dst)
   {
      if (src == dst) {
      /* The Vulkan 1.1 specification says the following about valid usage
   * of vkCmdBlitImage:
   *
   * "srcImageLayout must be VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
   *  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL"
   *
   * and:
   *
   * "dstImageLayout must be VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
   *  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL"
   *
   * Since we cant have the same image in two states at the same time,
   * we're effectively left with VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or
   * VK_IMAGE_LAYOUT_GENERAL. And since this isn't a present-related
   * operation, VK_IMAGE_LAYOUT_GENERAL seems most appropriate.
   */
   zink_screen(ctx->base.screen)->image_barrier(ctx, src,
                  } else {
      zink_screen(ctx->base.screen)->image_barrier(ctx, src,
                        zink_screen(ctx->base.screen)->image_barrier(ctx, dst,
                     }
      void
   zink_get_depth_stencil_resources(struct pipe_resource *res,
               {
      if (!res) {
      if (out_z) *out_z = NULL;
   if (out_s) *out_s = NULL;
               if (res->format != PIPE_FORMAT_S8_UINT) {
      if (out_z) *out_z = zink_resource(res);
      } else {
      if (out_z) *out_z = NULL;
         }
      static void
   zink_resource_set_separate_stencil(struct pipe_resource *pres,
         {
      assert(util_format_has_depth(util_format_description(pres->format)));
      }
      static enum pipe_format
   zink_resource_get_internal_format(struct pipe_resource *pres)
   {
      struct zink_resource *res = zink_resource(pres);
      }
      static const struct u_transfer_vtbl transfer_vtbl = {
      .resource_create       = zink_resource_create,
   .resource_destroy      = zink_resource_destroy,
   .transfer_map          = zink_image_map,
   .transfer_unmap        = zink_image_unmap,
   .transfer_flush_region = zink_transfer_flush_region,
   .get_internal_format   = zink_resource_get_internal_format,
   .set_stencil           = zink_resource_set_separate_stencil,
      };
      bool
   zink_screen_resource_init(struct pipe_screen *pscreen)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   pscreen->resource_create = u_transfer_helper_resource_create;
   pscreen->resource_create_with_modifiers = zink_resource_create_with_modifiers;
   pscreen->resource_create_drawable = zink_resource_create_drawable;
   pscreen->resource_destroy = u_transfer_helper_resource_destroy;
   pscreen->transfer_helper = u_transfer_helper_create(&transfer_vtbl,
      U_TRANSFER_HELPER_SEPARATE_Z32S8 | U_TRANSFER_HELPER_SEPARATE_STENCIL |
   U_TRANSFER_HELPER_INTERLEAVE_IN_PLACE |
   U_TRANSFER_HELPER_MSAA_MAP |
         if (screen->info.have_KHR_external_memory_fd || screen->info.have_KHR_external_memory_win32) {
      pscreen->resource_get_handle = zink_resource_get_handle;
      }
   if (screen->info.have_EXT_external_memory_host) {
         }
   if (screen->instance_info.have_KHR_external_memory_capabilities) {
      pscreen->memobj_create_from_handle = zink_memobj_create_from_handle;
   pscreen->memobj_destroy = zink_memobj_destroy;
      }
   pscreen->resource_get_param = zink_resource_get_param;
      }
      void
   zink_context_resource_init(struct pipe_context *pctx)
   {
      pctx->buffer_map = zink_buffer_map;
   pctx->buffer_unmap = zink_buffer_unmap;
   pctx->texture_map = u_transfer_helper_transfer_map;
            pctx->transfer_flush_region = u_transfer_helper_transfer_flush_region;
   pctx->buffer_subdata = zink_buffer_subdata;
   pctx->texture_subdata = zink_image_subdata;
      }
