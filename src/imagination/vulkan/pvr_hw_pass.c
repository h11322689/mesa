   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <limits.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_defs.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_hw_pass.h"
   #include "pvr_private.h"
   #include "util/bitset.h"
   #include "util/list.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_alloc.h"
   #include "vk_format.h"
   #include "vk_log.h"
      struct pvr_render_int_subpass {
      /* Points to the input subpass. This is set to NULL when the subpass is
   * unscheduled.
   */
            /* Count of other subpasses which have this subpass as a dependency. */
            /* Pointers to the other subpasses which have this subpass as a dependency.
   */
            /* Count of subpasses on which this subpass is dependent and which haven't
   * been scheduled yet.
   */
      };
      struct pvr_renderpass_resource {
      /* Resource type allocated for render target. */
            union {
      /* If type == USC_MRT_RESOURCE_TYPE_OUTPUT_REG. */
   struct {
                     /* The offset in bytes within the output register. */
               /* If type == USC_MRT_RESOURCE_TYPE_MEMORY.  */
   struct {
                     /* The offset (in dwords) within the tile buffer. */
            };
      struct pvr_render_int_attachment {
      /* Points to the corresponding input attachment. */
            /* True if this attachment is referenced in the currently open render. */
            /* Operation to use when this attachment is non-resident and referenced as a
   * color or depth attachment.
   */
            /* Operation to use for the stencil component when this attachment is
   * non-resident and referenced as a color or depth attachment.
   */
            /* Count of uses of this attachment in unscheduled subpasses. */
            /* Count of uses of the stencil component of this attachment in unscheduled
   * subpasses.
   */
            /* If this attachment has currently allocated on-chip storage then details of
   * the allocated location.
   */
            /* Index of the subpass in the current render where the attachment is first
   * used. VK_ATTACHMENT_UNUSED if the attachment isn't used in the current
   * render.
   */
            /* Index of the subpass in the current render where the attachment is last
   * used.
   */
            /* Index of the subpass (global) where the attachment is last read. */
            /* If this attachment has currently allocated on-chip storage then the entry
   * in context.active_surf_list.
   */
            /* During pvr_close_render: if this attachment has allocated on-chip storage
   * then the index in pvr_renderpass_hwsetup_render.eot_setup.mrt_resources
   * with details of the storage location. Otherwise -1.
   */
            /* Index of the last render where the attachment was the source of an MSAA
   * resolve.
   */
            /* Index of the last render where the attachment was the destination of an
   * MSAA resolve.
   */
            /* true if the attachment is used with a z replicate in the current render.
   */
            /* true if this attachment can be resolved by the PBE. */
            /* true if this attachment requires an EOT attachment. */
      };
      /* Which parts of the output registers/a tile buffer are currently allocated. */
   struct pvr_renderpass_alloc_buffer {
      /* Bit array. A bit is set if the corresponding dword is allocated. */
      };
      struct pvr_renderpass_alloc {
      /* Which pixel output registers are allocated. */
            /* Range of allocated output registers. */
            /* Number of tile buffers allocated. */
            /* Which parts of each tile buffer are allocated. Length is
   * tile_buffers_count.
   */
      };
      struct pvr_renderpass_subpass {
      /* A pointer to the input subpass description. */
            /* true if the depth attachment for this subpass has z replication enabled.
   */
            /* Which pixel output registers/tile buffer locations are allocated during
   * this subpass.
   */
      };
      struct pvr_renderpass_context {
      /* Internal information about each input attachment. */
            /* Internal information about each input subpass. */
            /* Input structure. */
            /* Output structure. */
            /* In-progress render. */
            /* Information about each subpass in the current render. */
            /* Which parts of color storage are currently allocated. */
            /* Attachment which is currently allocated the on-chip depth/stencil. */
            /* Attachment which is loaded into the on-chip depth/stencil at the start of
   * the render.
   */
            /* Attachment which the depth/stencil attachment should be resolved to at the
   * end of the render.
   */
            /* Count of surfaces which are allocated on-chip color storage. */
            /* List of attachment/ranges which are allocated on-chip color storage. */
               };
      struct pvr_render_int_subpass_dsts {
      struct pvr_renderpass_resource *color;
   struct pvr_renderpass_resource incoming_zrep;
      };
      struct pvr_render_subpass_depth_params {
      bool existing_ds_is_input;
   bool incoming_ds_is_input;
      };
      struct pvr_renderpass_storage_firstuse_buffer {
      /* For each pixel output register/tile buffer location: true if the output
   * register has been allocated in the current render.
   */
      };
      struct pvr_renderpass_storage_firstuse {
      /* First use information for pixel output registers. */
            /* First use information for tile buffers. */
      };
      /** Copy information about allocated color storage. */
   static VkResult pvr_copy_alloc(struct pvr_renderpass_context *ctx,
               {
      dst->output_reg = src->output_reg;
            dst->tile_buffers_count = src->tile_buffers_count;
   if (dst->tile_buffers_count > 0U) {
      dst->tile_buffers =
      vk_alloc(ctx->allocator,
            sizeof(dst->tile_buffers[0U]) * dst->tile_buffers_count,
   if (!dst->tile_buffers)
            memcpy(dst->tile_buffers,
            } else {
                     }
      /** Free information about allocated color storage. */
   static void pvr_free_alloc(struct pvr_renderpass_context *ctx,
         {
      if (alloc->tile_buffers)
               }
      static void pvr_reset_render(struct pvr_renderpass_context *ctx)
   {
      ctx->int_ds_attach = NULL;
   ctx->active_surfaces = 0U;
            memset(&ctx->alloc.output_reg, 0U, sizeof(ctx->alloc.output_reg));
   ctx->alloc.output_regs_count = 0U;
   ctx->alloc.tile_buffers_count = 0U;
            ctx->hw_render = NULL;
   ctx->subpasses = NULL;
      }
      /** Gets the amount of memory to allocate per-core for a tile buffer. */
   static uint32_t
   pvr_get_tile_buffer_size_per_core(const struct pvr_device *device)
   {
      uint32_t clusters =
            /* Round the number of clusters up to the next power of two. */
   if (!PVR_HAS_FEATURE(&device->pdevice->dev_info, tile_per_usc))
            /* Tile buffer is (total number of partitions across all clusters) * 16 * 16
   * (quadrant size in pixels).
   */
   return device->pdevice->dev_runtime_info.total_reserved_partition_size *
      }
      /**
   * Gets the amount of memory to allocate for a tile buffer on the current BVNC.
   */
   uint32_t pvr_get_tile_buffer_size(const struct pvr_device *device)
   {
      /* On a multicore system duplicate the buffer for each core. */
   return pvr_get_tile_buffer_size_per_core(device) *
      }
      static void
   pvr_finalise_mrt_setup(const struct pvr_device *device,
               {
      mrt->num_output_regs = hw_render->output_regs_count;
   mrt->num_tile_buffers = hw_render->tile_buffers_count;
      }
      /**
   * Copy information about the number of pixel output registers and tile buffers
   * required for the current render to the output structure.
   */
   static void pvr_finalise_po_alloc(const struct pvr_device *device,
         {
               /* The number of output registers must be a power of two. */
   hw_render->output_regs_count =
            assert(ctx->alloc.tile_buffers_count <= ctx->pass->max_tilebuffer_count);
            /* Copy the number of output registers and tile buffers to each subpass. */
   for (uint32_t i = 0U; i < hw_render->subpass_count; i++) {
      struct pvr_renderpass_hwsetup_subpass *hw_subpass =
                        pvr_finalise_mrt_setup(device, hw_render, &hw_render->init_setup);
      }
      /** Mark that device memory must be allocated for an attachment. */
   static void pvr_mark_surface_alloc(struct pvr_renderpass_context *ctx,
         {
               assert(attach_idx < ctx->pass->attachment_count);
      }
      /**
   * Check if there is space in a buffer for storing a render target of a
   * specified size.
   */
   static int32_t
   pvr_is_space_in_buffer(const struct pvr_device_info *dev_info,
               {
      const uint32_t max_out_regs = rogue_get_max_output_regs_per_pixel(dev_info);
            if (PVR_HAS_FEATURE(dev_info, pbe2_in_xe)) {
      /* For a 64-bit/128-bit source format: the start offset must be even. */
   if (pixel_size == 2U || pixel_size == 4U)
                        for (uint32_t i = 0U; i <= (max_out_regs - pixel_size); i += alignment) {
      if (!BITSET_TEST_RANGE(buffer->allocs, i, i + pixel_size - 1U))
                  }
      static VkResult
   pvr_surface_setup_render_init(struct pvr_renderpass_context *ctx,
                                 {
      const uint32_t pixel_size =
         struct pvr_renderpass_hwsetup_render *hw_render = ctx->hw_render;
   struct pvr_renderpass_storage_firstuse_buffer *buffer;
            /* Check if this is the first use of all the allocated registers. */
   if (resource->type == USC_MRT_RESOURCE_TYPE_OUTPUT_REG) {
      buffer = &first_use->output_reg;
      } else {
      assert(resource->mem.tile_buffer < ctx->alloc.tile_buffers_count);
   buffer = &first_use->tile_buffers[resource->mem.tile_buffer];
               *use_render_init = true;
   for (uint32_t i = 0U; i < pixel_size; i++) {
      /* Don't initialize at the render level if the output registers were
   * previously allocated a different attachment.
   */
   if (buffer->used[start + i])
            /* Don't use render init for future attachments allocated to the same
   * registers.
   */
               if (load_op == VK_ATTACHMENT_LOAD_OP_DONT_CARE)
            if (*use_render_init) {
      struct pvr_renderpass_colorinit *new_color_init;
            /* Initialize the storage at the start of the render. */
   new_color_init = vk_realloc(ctx->allocator,
                                 if (!new_color_init)
            hw_render->color_init = new_color_init;
   hw_render->color_init[hw_render->color_init_count].index =
                  /* Set the destination for the attachment load/clear. */
   assert(hw_render->init_setup.num_render_targets ==
            new_mrt = vk_realloc(ctx->allocator,
                        hw_render->init_setup.mrt_resources,
      if (!new_mrt)
            hw_render->init_setup.mrt_resources = new_mrt;
   hw_render->init_setup
                                 }
      static VkResult
   pvr_subpass_setup_render_init(struct pvr_renderpass_context *ctx)
   {
      struct pvr_renderpass_hwsetup_render *hw_render = ctx->hw_render;
   struct pvr_renderpass_storage_firstuse first_use = { 0 };
   bool first_ds = true;
            if (ctx->alloc.tile_buffers_count > 0U) {
      first_use.tile_buffers = vk_zalloc(ctx->allocator,
                           if (!first_use.tile_buffers)
               for (uint32_t i = 0U; i < hw_render->subpass_count; i++) {
      struct pvr_renderpass_hwsetup_subpass *hw_subpass =
         struct pvr_renderpass_subpass *subpass = &ctx->subpasses[i];
            /* If this is the first depth attachment in the render then clear at the
   * render level, not the subpass level.
   */
   if (first_ds &&
      (hw_subpass->depth_initop == VK_ATTACHMENT_LOAD_OP_CLEAR ||
                  assert(input_subpass->depth_stencil_attachment !=
         assert(input_subpass->depth_stencil_attachment <
                        assert(hw_render->ds_attach_idx == VK_ATTACHMENT_UNUSED ||
                                 if (hw_subpass->stencil_clear) {
      hw_render->stencil_init = VK_ATTACHMENT_LOAD_OP_CLEAR;
                  if (input_subpass->depth_stencil_attachment != VK_ATTACHMENT_UNUSED)
            for (uint32_t j = 0U; j < input_subpass->color_count; j++) {
      struct usc_mrt_resource *mrt = &hw_subpass->setup.mrt_resources[j];
                                                         /* Is this the first use of the attachment? */
   if (int_attach->first_use == (int32_t)i) {
      /* Set if we should initialize the attachment storage at the
   * render level.
   */
   bool use_render_init;
   result = pvr_surface_setup_render_init(ctx,
                                 if (result != VK_SUCCESS) {
                                 /* On success don't initialize the attachment at the subpass level.
   */
   if (use_render_init)
      } else {
      /* This attachment is already present in on-chip storage so don't
   * do anything.
   */
   assert(hw_subpass->color_initops[j] ==
                     if (!first_use.tile_buffers)
               }
      static void
   pvr_mark_storage_allocated_in_buffer(struct pvr_renderpass_alloc_buffer *buffer,
               {
      assert(!BITSET_TEST_RANGE(buffer->allocs, start, start + pixel_size - 1U));
      }
      static VkResult
   pvr_mark_storage_allocated(struct pvr_renderpass_context *ctx,
                     {
      /* Number of dwords to allocate for the attachment. */
   const uint32_t pixel_size =
            if (resource->type == USC_MRT_RESOURCE_TYPE_OUTPUT_REG) {
      /* Update the locations used in the pixel output registers. */
   pvr_mark_storage_allocated_in_buffer(&alloc->output_reg,
                  /* Update the range of pixel output registers used. */
   alloc->output_regs_count =
      } else {
               if (resource->mem.tile_buffer >= alloc->tile_buffers_count) {
      /* Grow the number of tile buffers. */
   struct pvr_renderpass_alloc_buffer *new_tile_buffers = vk_realloc(
      ctx->allocator,
   alloc->tile_buffers,
   sizeof(alloc->tile_buffers[0U]) * (resource->mem.tile_buffer + 1U),
   8U,
                     alloc->tile_buffers = new_tile_buffers;
   memset(
      &alloc->tile_buffers[alloc->tile_buffers_count],
   0U,
   sizeof(alloc->tile_buffers[0U]) *
      alloc->tile_buffers_count = resource->mem.tile_buffer + 1U;
               /* Update the locations used in the tile buffer. */
   pvr_mark_storage_allocated_in_buffer(
      &alloc->tile_buffers[resource->mem.tile_buffer],
               /* The hardware makes the bit depth of the on-chip storage and memory
   * storage the same so make sure the memory storage is large enough to
   * accommodate the largest render target.
   */
   alloc->output_regs_count =
                  }
      static VkResult
   pvr_surface_alloc_color_storage(const struct pvr_device_info *dev_info,
                           {
      /* Number of dwords to allocate for the attachment. */
   const uint32_t pixel_size =
            /* Try allocating pixel output registers. */
   const int32_t output_reg =
         if (output_reg != -1) {
      resource->type = USC_MRT_RESOURCE_TYPE_OUTPUT_REG;
   resource->reg.output_reg = (uint32_t)output_reg;
      } else {
               /* Mark the attachment as using a tile buffer. */
            /* Try allocating from an existing tile buffer. */
   for (i = 0U; i < alloc->tile_buffers_count; i++) {
      const int32_t tile_buffer_offset =
      pvr_is_space_in_buffer(dev_info,
               if (tile_buffer_offset != -1) {
      resource->mem.tile_buffer = i;
   resource->mem.offset_dw = (uint32_t)tile_buffer_offset;
                  if (i == alloc->tile_buffers_count) {
      /* Check for reaching the maximum number of tile buffers. */
                  /* Use a newly allocated tile buffer. */
   resource->mem.tile_buffer = i;
                  /* Update which parts of the pixel outputs/tile buffers are used. */
      }
      /** Free the storage allocated to an attachment. */
   static void
   pvr_free_buffer_storage(struct pvr_renderpass_alloc_buffer *buffer,
               {
      const uint32_t pixel_size = DIV_ROUND_UP(
      vk_format_get_blocksizebits(int_attach->attachment->vk_format),
            }
      /** Free the storage allocated to an attachment. */
   static void
   pvr_free_surface_storage(struct pvr_renderpass_context *ctx,
         {
      struct usc_mrt_resource *resource = &int_attach->resource;
                     /* Mark the storage as free. */
   if (resource->type == USC_MRT_RESOURCE_TYPE_OUTPUT_REG) {
      pvr_free_buffer_storage(&alloc->output_reg,
            } else {
                        assert(resource->mem.tile_buffer < alloc->tile_buffers_count);
   tile_buffer = &alloc->tile_buffers[resource->mem.tile_buffer];
               /* Mark that the attachment doesn't have allocated storage. */
            /* Remove from the list of surfaces with allocated on-chip storage. */
   assert(ctx->active_surfaces > 0U);
   ctx->active_surfaces--;
      }
      static void pvr_reset_surface(struct pvr_renderpass_context *ctx,
         {
      /* Reset information about the range of uses. */
   int_attach->first_use = int_attach->last_use = -1;
               }
      static void
   pvr_make_surface_active(struct pvr_renderpass_context *ctx,
               {
      /* Add to the list of surfaces with on-chip storage. */
   assert(int_attach->first_use == -1);
   int_attach->first_use = subpass_num;
   ctx->active_surfaces++;
      }
      /**
   * For a subpass copy details of storage locations for the input/color to the
   * output structure.
   */
   static VkResult
   pvr_copy_storage_details(struct pvr_renderpass_context *ctx,
               {
      struct pvr_render_subpass *input_subpass = subpass->input_subpass;
   const uint32_t max_rts =
                  if (max_rts == 0)
            hw_subpass->setup.mrt_resources =
      vk_zalloc(ctx->allocator,
            sizeof(hw_subpass->setup.mrt_resources[0U]) * max_rts,
   if (!hw_subpass->setup.mrt_resources) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               for (uint32_t i = 0U; i < input_subpass->color_count; i++) {
      const uint32_t attach_idx = input_subpass->color_attachments[i];
            if (attach_idx == VK_ATTACHMENT_UNUSED)
                     /* Record for the subpass where the color attachment is stored. */
   assert(int_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID);
                        if (input_subpass->input_count == 0)
            /* For this subpass's input attachments. */
   hw_subpass->input_access = vk_alloc(ctx->allocator,
                           if (!hw_subpass->input_access) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               for (uint32_t i = 0U; i < input_subpass->input_count; i++) {
      const uint32_t attach_idx = input_subpass->input_attachments[i];
            if (attach_idx == VK_ATTACHMENT_UNUSED)
                     if (int_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID) {
               /* Access the input attachment from on-chip storage. */
   if (int_attach->z_replicate) {
      hw_subpass->input_access[i].type =
      } else {
      hw_subpass->input_access[i].type =
               /* If this attachment is also a color attachment then point to the
   * color attachment's resource.
   */
   for (uint32_t j = 0U; j < input_subpass->color_count; j++) {
      if (input_subpass->color_attachments[j] == (int32_t)attach_idx) {
      hw_subpass->input_access[i].on_chip_rt = j;
   is_color = true;
                                                   /* Record the location of the storage for the attachment. */
         } else {
      /* Access the input attachment from memory. */
   hw_subpass->input_access[i].type =
                              end_copy_storage_details:
      if (hw_subpass->input_access) {
      vk_free(ctx->allocator, hw_subpass->input_access);
               if (hw_subpass->setup.mrt_resources) {
      vk_free(ctx->allocator, hw_subpass->setup.mrt_resources);
                  }
      /**
   * For a subpass copy details of any storage location for a replicated version
   * of the depth attachment to the output structure.
   */
   static VkResult
   pvr_copy_z_replicate_details(struct pvr_renderpass_context *ctx,
               {
      struct pvr_render_subpass *input_subpass = subpass->input_subpass;
   struct pvr_render_int_attachment *int_ds_attach;
   uint32_t z_replicate;
            assert(input_subpass->depth_stencil_attachment >= 0U &&
                                    /* Is the replicated depth also an input attachment? */
   for (uint32_t i = 0U; i < input_subpass->input_count; i++) {
      const uint32_t attach_idx = input_subpass->input_attachments[i];
            if (attach_idx == VK_ATTACHMENT_UNUSED)
                     if (int_attach == int_ds_attach) {
      z_replicate = hw_subpass->input_access[i].on_chip_rt;
   found = true;
                  if (!found)
            /* If the Z replicate attachment isn't also an input attachment then grow the
   * array of locations.
   */
   assert(z_replicate <= hw_subpass->setup.num_render_targets);
   if (z_replicate == hw_subpass->setup.num_render_targets) {
      struct usc_mrt_resource *mrt =
      vk_realloc(ctx->allocator,
            hw_subpass->setup.mrt_resources,
   sizeof(hw_subpass->setup.mrt_resources[0U]) *
         if (!mrt)
            hw_subpass->setup.mrt_resources = mrt;
               /* Copy the location of the Z replicate. */
   assert(int_ds_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID);
   hw_subpass->setup.mrt_resources[z_replicate] = int_ds_attach->resource;
               }
      static void pvr_dereference_surface(struct pvr_renderpass_context *ctx,
               {
               assert(int_attach->remaining_count > 0U);
            if (int_attach->remaining_count == 0U) {
      if (int_attach->first_use != -1)
            if (int_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID)
               if (int_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      assert(int_attach->stencil_remaining_count > 0U);
         }
      static void pvr_free_render(struct pvr_renderpass_context *ctx)
   {
               if (ctx->subpasses) {
      for (uint32_t i = 0U; i < ctx->hw_render->subpass_count; i++)
            vk_free(ctx->allocator, ctx->subpasses);
         }
      static bool pvr_render_has_side_effects(struct pvr_renderpass_context *ctx)
   {
      struct pvr_renderpass_hwsetup_render *hw_render = ctx->hw_render;
            if ((hw_render->depth_init == VK_ATTACHMENT_LOAD_OP_CLEAR &&
      hw_render->depth_store) ||
   (hw_render->stencil_init == VK_ATTACHMENT_LOAD_OP_CLEAR &&
   hw_render->stencil_store)) {
               for (uint32_t i = 0U; i < hw_render->eot_surface_count; i++) {
      const struct pvr_renderpass_hwsetup_eot_surface *eot_attach =
         const struct pvr_render_pass_attachment *attachment =
            if (attachment->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR &&
      attachment->store_op == VK_ATTACHMENT_STORE_OP_STORE) {
               if (eot_attach->need_resolve)
                  }
      static VkResult pvr_close_render(const struct pvr_device *device,
         {
      struct pvr_renderpass_hwsetup_render *hw_render = ctx->hw_render;
   struct pvr_renderpass_hwsetup_eot_surface *eot_attach;
   struct usc_mrt_setup *eot_setup;
   int32_t mrt_idx;
            /* Render already closed. */
   if (!hw_render)
            /* Setup render and allocate resources for color/depth loads and clears. */
   result = pvr_subpass_setup_render_init(ctx);
   if (result != VK_SUCCESS)
            /* Reset surfaces whose last use was in the current render. */
   list_for_each_entry_safe (struct pvr_render_int_attachment,
                        if (int_attach->last_use != -1) {
      assert(int_attach->resource.type == USC_MRT_RESOURCE_TYPE_INVALID);
                  /* Check if the depth attachment has uses in future subpasses. */
   if (ctx->int_ds_attach) {
      /* Store the depth to the attachment at the end of the render. */
   if (ctx->int_ds_attach->remaining_count > 0U)
            /* Store the stencil to the attachment at the end of the render. */
   if (ctx->int_ds_attach->stencil_remaining_count > 0U)
            if (hw_render->depth_store || hw_render->stencil_store) {
      assert(hw_render->ds_attach_idx == VK_ATTACHMENT_UNUSED ||
                        /* Allocate memory for the attachment. */
               /* Load the depth and stencil before the next use. */
   ctx->int_ds_attach->load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
               eot_setup = &hw_render->eot_setup;
            /* Set the number of pixel output registers/tile buffers allocated for the
   * render and copy the information to all subpasses and the EOT program.
   */
            /* If any attachment are used with z replicate then they will be stored to by
   * the ISP. So remove them from the list to store to using the PBE.
   */
   list_for_each_entry_safe (struct pvr_render_int_attachment,
                        if (int_attach->z_replicate)
               /* Number of surfaces with allocated on-chip storage. */
   eot_setup->num_render_targets = ctx->active_surfaces;
   eot_setup->mrt_resources = vk_alloc(ctx->allocator,
                           if (!eot_setup->mrt_resources)
            /* Record the location of the on-chip storage. */
   mrt_idx = 0U;
   list_for_each_entry_safe (struct pvr_render_int_attachment,
                        assert(int_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID);
   assert(int_attach->remaining_count > 0U);
   if (int_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
            /* Copy the location of the source data for this attachment. */
            assert(int_attach->mrt_idx == -1);
               }
            hw_render->eot_surface_count = 0U;
            /* Count the number of surfaces to store to at the end of the subpass. */
   for (uint32_t i = 0U; i < hw_render->subpass_count; i++) {
      struct pvr_renderpass_subpass *subpass = &ctx->subpasses[i];
            for (uint32_t j = 0U; j < input_subpass->color_count; j++) {
      const uint32_t resolve_output =
      input_subpass->resolve_attachments
                                             if (list_is_linked(&color_attach->link)) {
                     /* If a color attachment is resolved it will have an extra
   * remaining usage.
   */
   if (color_attach->remaining_count > rem_count &&
      !color_attach->eot_surf_required) {
   color_attach->eot_surf_required = true;
                  if (resolve_output != VK_ATTACHMENT_UNUSED) {
                     if (!int_resolve_attach->eot_surf_required) {
      int_resolve_attach->eot_surf_required = true;
                                 hw_render->eot_surfaces = vk_alloc(ctx->allocator,
                           if (!hw_render->eot_surfaces)
                     for (uint32_t i = 0U; i < hw_render->subpass_count; i++) {
      struct pvr_renderpass_subpass *subpass = &ctx->subpasses[i];
            for (uint32_t j = 0U; j < input_subpass->color_count; j++) {
      const uint32_t resolve_output =
      input_subpass->resolve_attachments
                                             if (resolve_output != VK_ATTACHMENT_UNUSED) {
      struct pvr_render_int_attachment *resolve_src =
                                       /* Dereference the source to the resolve. */
                                 /* The attachment has been written so load the attachment the
   * next time it is referenced.
                  eot_attach->mrt_idx = resolve_src->mrt_idx;
                           if (!resolve_src->is_pbe_downscalable) {
                        } else if (resolve_src->remaining_count == 0U) {
      eot_attach->resolve_type = PVR_RESOLVE_TYPE_PBE;
      } else {
                                                         assert(color_attach->mrt_idx >= 0);
                  eot_attach->mrt_idx = color_attach->mrt_idx;
   eot_attach->attachment_idx = color_attach->attachment->index;
                                                      /* Count the number of extra resolves we can do through the PBE. */
   for (uint32_t i = 0U; i < hw_render->eot_surface_count; i++) {
               if (eot_attach->need_resolve &&
      eot_attach->resolve_type == PVR_RESOLVE_TYPE_INVALID) {
   if (hw_render->pbe_emits == PVR_NUM_PBE_EMIT_REGS) {
         } else {
      eot_attach->resolve_type = PVR_RESOLVE_TYPE_PBE;
                              /* Check for side effects in the final render. */
            /* Reset active surfaces. */
   list_for_each_entry_safe (struct pvr_render_int_attachment,
                        int_attach->mrt_idx = -1;
               assert(ctx->active_surfaces == 0U);
            pvr_free_render(ctx);
               }
      static bool pvr_is_input(struct pvr_render_subpass *subpass,
         {
      if (attach_idx == VK_ATTACHMENT_UNUSED)
            for (uint32_t i = 0U; i < subpass->input_count; i++) {
      if (subpass->input_attachments[i] == attach_idx)
                  }
      static bool
   pvr_depth_zls_conflict(struct pvr_renderpass_context *ctx,
               {
      if (!ctx->int_ds_attach)
            /* No conflict if the incoming subpass doesn't have a depth/stencil
   * attachment.
   */
   if (!int_ds_attach)
            /* No conflict if the incoming depth/stencil attachment is the same as the
   * existing one.
   */
   if (ctx->int_ds_attach == int_ds_attach)
            /* If the existing depth/stencil attachment is used later, then we can't
   * overwrite it.
   *
   * The exception is if the only use is as an input attachment in the incoming
   * subpass in which case we can use the Z replicate feature to save the
   * value.
   */
   if (ctx->int_ds_attach->remaining_count > 0U &&
      !(existing_ds_is_input && ctx->int_ds_attach->remaining_count == 1U)) {
               if (ctx->int_ds_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT &&
      ctx->int_ds_attach->stencil_remaining_count > 0U) {
               /* We can't load midrender so fail if the new depth/stencil attachment is
   * already initialized.
   */
   if (int_ds_attach->load_op == VK_ATTACHMENT_LOAD_OP_LOAD)
            if (int_ds_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT &&
      int_ds_attach->stencil_load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
                  }
      static void
   pvr_set_surface_resource(struct pvr_render_int_attachment *int_attach,
         {
               switch (resource->type) {
   case USC_MRT_RESOURCE_TYPE_OUTPUT_REG:
      int_attach->resource.reg.output_reg = resource->reg.output_reg;
   int_attach->resource.reg.offset = resource->reg.offset;
         case USC_MRT_RESOURCE_TYPE_MEMORY:
      int_attach->resource.mem.tile_buffer = resource->mem.tile_buffer;
   int_attach->resource.mem.offset_dw = resource->mem.offset_dw;
         default:
            }
      static bool pvr_equal_resources(struct pvr_renderpass_resource *resource1,
         {
      if (resource1->type != resource2->type)
            switch (resource1->type) {
   case USC_MRT_RESOURCE_TYPE_OUTPUT_REG:
      return resource1->reg.output_reg == resource2->reg.output_reg &&
         case USC_MRT_RESOURCE_TYPE_MEMORY:
      return resource1->mem.tile_buffer == resource2->mem.tile_buffer &&
         default:
            }
      static VkResult
   pvr_enable_z_replicate(struct pvr_renderpass_context *ctx,
                     {
      struct pvr_render_int_attachment *int_attach =
                  /* If Z replication was already enabled for the attachment then nothing more
   * to do.
   */
   if (!int_attach->z_replicate) {
      /* Copy details of the storage for the replicated value to the attachment.
   */
   assert(int_attach->resource.type == USC_MRT_RESOURCE_TYPE_INVALID);
   assert(replicate_dst->type != USC_MRT_RESOURCE_TYPE_INVALID);
      } else {
      assert(int_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID);
               /* Find the first subpass where the attachment is written. */
   for (uint32_t i = 0U; i < hw_render->subpass_count; i++) {
      struct pvr_renderpass_subpass *subpass = &ctx->subpasses[i];
            if (input_subpass->depth_stencil_attachment == replicate_attach_idx) {
      first_use = i;
         }
            /* For all subpasses from the first write. */
   for (uint32_t i = first_use; i < hw_render->subpass_count; i++) {
      struct pvr_renderpass_subpass *subpass = &ctx->subpasses[i];
            /* If the subpass writes to the attachment then enable z replication. */
   if (input_subpass->depth_stencil_attachment == replicate_attach_idx &&
                     if (i != (hw_render->subpass_count - 1U)) {
      /* Copy the details of the storage for replicated value. */
   const VkResult result =
      pvr_copy_z_replicate_details(ctx,
            if (result != VK_SUCCESS)
                     if (!int_attach->z_replicate) {
      /* Add the storage for the replicated value to locations in use at each
   * subpass.
   */
   for (uint32_t i = first_use; i < (hw_render->subpass_count - 1U); i++) {
               pvr_mark_storage_allocated(ctx,
                           /* Add the depth attachment to the list of surfaces with allocated
   * storage.
   */
                           }
      static bool pvr_is_pending_resolve_dest(struct pvr_renderpass_context *ctx,
         {
               return int_attach->last_resolve_dst_render != -1 &&
            }
      static bool pvr_is_pending_resolve_src(struct pvr_renderpass_context *ctx,
         {
               return int_attach->last_resolve_src_render != -1 &&
            }
      static bool pvr_exceeds_pbe_registers(struct pvr_renderpass_context *ctx,
         {
      int32_t live_outputs[PVR_NUM_PBE_EMIT_REGS];
            /* Count all color outputs so far. */
   for (uint32_t i = 0U; i < ctx->hw_render->subpass_count; i++) {
      struct pvr_render_subpass *input_subpass =
            for (uint32_t j = 0U; j < input_subpass->color_count; j++) {
      const uint32_t global_color_attach =
                                                               for (uint32_t k = 0U; k < num_live_outputs; k++) {
      if (live_outputs[k] == global_color_attach) {
      found = true;
                  if (!found)
                           /* Check if adding all the color outputs of the new subpass to the render
   * would exceed the limit.
   */
   for (uint32_t i = 0U; i < subpass->color_count; i++) {
      const uint32_t global_color_attach = subpass->color_attachments[i];
   struct pvr_render_int_attachment *int_attach;
            if (global_color_attach == VK_ATTACHMENT_UNUSED)
                     if (int_attach->last_read <= (int32_t)subpass->index)
            for (uint32_t j = 0U; j < num_live_outputs; j++) {
      if (live_outputs[j] == global_color_attach) {
      found = true;
                  if (!found) {
                                       }
      static void pvr_merge_alloc_buffer(struct pvr_renderpass_alloc_buffer *dst,
         {
      for (uint32_t i = 0U; i < ARRAY_SIZE(dst->allocs); i++)
      }
      static VkResult pvr_merge_alloc(struct pvr_renderpass_context *ctx,
               {
               dst->output_regs_count =
            if (dst->tile_buffers_count < src->tile_buffers_count) {
      struct pvr_renderpass_alloc_buffer *new_tile_buffers =
      vk_realloc(ctx->allocator,
            dst->tile_buffers,
   sizeof(dst->tile_buffers[0U]) * src->tile_buffers_count,
   if (!new_tile_buffers)
            dst->tile_buffers = new_tile_buffers;
   memset(dst->tile_buffers + dst->tile_buffers_count,
         0U,
   sizeof(dst->tile_buffers[0U]) *
               for (uint32_t i = 0U; i < src->tile_buffers_count; i++)
               }
      static VkResult
   pvr_is_z_replicate_space_available(const struct pvr_device_info *dev_info,
                           {
      struct pvr_renderpass_hwsetup_render *hw_render = ctx->hw_render;
   struct pvr_render_int_attachment *int_attach;
   struct pvr_renderpass_alloc combined_alloc;
   uint32_t first_use;
            /* If z replication was already enabled by a previous subpass then storage
   * will already be allocated.
   */
            int_attach = &ctx->int_attach[attach_idx];
   if (int_attach->z_replicate) {
      assert(int_attach->resource.type != USC_MRT_RESOURCE_TYPE_INVALID);
               /* Find the subpass where the depth is first written. */
   if (hw_render) {
      first_use = hw_render->subpass_count;
   for (uint32_t i = 0U; i < hw_render->subpass_count; i++) {
                     if (input_subpass->depth_stencil_attachment == (int32_t)attach_idx) {
      first_use = i;
                     /* Get the registers used in any subpass after the depth is first written.
   * Start with registers used in the incoming subpass.
   */
   result = pvr_copy_alloc(ctx, &combined_alloc, alloc);
   if (result != VK_SUCCESS)
            if (hw_render) {
      /* Merge in registers used in previous subpasses. */
   for (uint32_t i = first_use; i < hw_render->subpass_count; i++) {
               result = pvr_merge_alloc(ctx, &combined_alloc, &subpass->alloc);
   if (result != VK_SUCCESS) {
      pvr_free_alloc(ctx, &combined_alloc);
                     result = pvr_surface_alloc_color_storage(dev_info,
                              pvr_free_alloc(ctx, &combined_alloc);
   if (result != VK_SUCCESS)
            return pvr_mark_storage_allocated(ctx,
                  }
      static VkResult
   pvr_is_subpass_space_available(const struct pvr_device_info *dev_info,
                                 {
               /* Mark pointers in return structures as not allocated. */
   sp_dsts->color = NULL;
            /* Allocate space for which locations are in use after this subpass. */
   result = pvr_copy_alloc(ctx, alloc, &ctx->alloc);
   if (result != VK_SUCCESS)
            /* Allocate space to store our results. */
   if (subpass->color_count > 0U) {
      sp_dsts->color =
      vk_alloc(ctx->allocator,
            sizeof(sp_dsts->color[0U]) * subpass->color_count,
   if (!sp_dsts->color) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
         } else {
                  sp_dsts->existing_zrep.type = USC_MRT_RESOURCE_TYPE_INVALID;
            for (uint32_t i = 0U; i < subpass->color_count; i++) {
      const uint32_t attach_idx = subpass->color_attachments[i];
            if (attach_idx == VK_ATTACHMENT_UNUSED)
                     assert(vk_format_get_blocksizebits(int_attach->attachment->vk_format) >
            /* Is the attachment not allocated on-chip storage? */
   if (int_attach->resource.type == USC_MRT_RESOURCE_TYPE_INVALID) {
      result = pvr_surface_alloc_color_storage(dev_info,
                                          /* Avoid merging subpasses which result in tile buffers having to be
   * used. The benefit of merging must be weighed against the cost of
   * writing/reading to tile buffers.
   */
   if (ctx->hw_render &&
      sp_dsts->color[i].type != USC_MRT_RESOURCE_TYPE_OUTPUT_REG) {
   result = vk_error(NULL, VK_ERROR_TOO_MANY_OBJECTS);
         } else {
                     if (sp_depth->existing_ds_is_input) {
      result = pvr_is_z_replicate_space_available(dev_info,
                           if (result != VK_SUCCESS)
               if (sp_depth->incoming_ds_is_input) {
      if (sp_depth->existing_ds_attach != subpass->depth_stencil_attachment) {
      result = pvr_is_z_replicate_space_available(
      dev_info,
   ctx,
   alloc,
   subpass->depth_stencil_attachment,
      if (result != VK_SUCCESS)
      } else {
                           err_free_alloc:
      pvr_free_alloc(ctx, alloc);
   if (sp_dsts->color)
                        }
      static bool
   pvr_can_combine_with_render(const struct pvr_device_info *dev_info,
                              struct pvr_renderpass_context *ctx,
      {
      VkResult result;
            /* Mark pointers in return structures as not allocated. */
   sp_dsts->color = NULL;
            /* The hardware doesn't support replicating the stencil, so we need to store
   * the depth to memory if a stencil attachment is used as an input
   * attachment.
   */
   if (sp_depth->existing_ds_is_input &&
      ctx->int_ds_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
               if (sp_depth->incoming_ds_is_input && int_ds_attach &&
      int_ds_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT &&
   ctx->hw_render) {
               /* Can't mix multiple sample counts into same render. */
   if (ctx->hw_render &&
      ctx->hw_render->sample_count != subpass->sample_count) {
               /* If the depth is used by both the render and the incoming subpass and
   * either the existing depth must be saved or the new depth must be loaded
   * then we can't merge.
   */
   ret = pvr_depth_zls_conflict(ctx,
               if (ret)
            /* Check if any of the subpass's dependencies are marked that the two
   * subpasses can't be in the same render.
   */
   for (uint32_t i = 0U; i < subpass->dep_count; i++) {
      const uint32_t dep = subpass->dep_list[i];
   if (subpass->flush_on_dep[i] && ctx->hw_setup->subpass_map[dep].render ==
                           /* Check if one of the input/color attachments is written by an MSAA resolve
   * in an existing subpass in the current render.
   */
   for (uint32_t i = 0U; i < subpass->input_count; i++) {
      const uint32_t attach_idx = subpass->input_attachments[i];
   if (attach_idx != VK_ATTACHMENT_UNUSED &&
      pvr_is_pending_resolve_dest(ctx, attach_idx)) {
                  for (uint32_t i = 0U; i < subpass->color_count; i++) {
      if (subpass->color_attachments[i] != VK_ATTACHMENT_UNUSED &&
      (pvr_is_pending_resolve_dest(ctx, subpass->color_attachments[i]) ||
   pvr_is_pending_resolve_src(ctx, subpass->color_attachments[i]))) {
               if (subpass->resolve_attachments &&
      subpass->resolve_attachments[i] != VK_ATTACHMENT_UNUSED &&
   pvr_is_pending_resolve_dest(ctx, subpass->resolve_attachments[i])) {
                  /* No chance of exceeding PBE registers in a single subpass. */
   if (ctx->hw_render) {
      ret = pvr_exceeds_pbe_registers(ctx, subpass);
   if (ret)
               /* Check we can allocate storage for the new subpass's color attachments and
   * any z replications.
   */
   result = pvr_is_subpass_space_available(dev_info,
                                 if (result != VK_SUCCESS)
               }
      static VkResult
   pvr_merge_subpass(const struct pvr_device *device,
                     {
      struct pvr_renderpass_hwsetup_subpass *new_hw_subpasses;
   struct pvr_renderpass_hwsetup_subpass *hw_subpass;
   struct pvr_render_int_attachment *int_ds_attach;
   struct pvr_renderpass_hwsetup_render *hw_render;
   struct pvr_render_subpass_depth_params sp_depth;
   struct pvr_renderpass_subpass *new_subpasses;
   struct pvr_render_int_subpass_dsts sp_dsts;
   struct pvr_renderpass_subpass *subpass;
   struct pvr_renderpass_alloc alloc;
   VkResult result;
            /* Depth attachment for the incoming subpass. */
   if (input_subpass->depth_stencil_attachment != VK_ATTACHMENT_UNUSED)
         else
            /* Attachment ID for the existing depth attachment. */
   if (ctx->int_ds_attach)
         else
            /* Is the incoming depth attachment used as an input to the incoming subpass?
   */
   sp_depth.incoming_ds_is_input =
            /* Is the current depth attachment used as an input to the incoming subpass?
   */
   sp_depth.existing_ds_is_input =
            /* Can the incoming subpass be combined with the existing render? Also checks
   * if space is available for the subpass results and return the allocated
   * locations.
   */
   ret = pvr_can_combine_with_render(&device->pdevice->dev_info,
                                       if (!ret) {
      result = pvr_close_render(device, ctx);
   if (result != VK_SUCCESS)
            sp_depth.existing_ds_is_input = false;
            /* Allocate again in a new render. */
   result = pvr_is_subpass_space_available(&device->pdevice->dev_info,
                                 assert(result != VK_ERROR_TOO_MANY_OBJECTS);
   if (result != VK_SUCCESS)
               /* If there isn't an in-progress render then allocate one. */
   if (!ctx->hw_render) {
      struct pvr_renderpass_hwsetup *hw_setup = ctx->hw_setup;
   struct pvr_renderpass_hwsetup_render *new_hw_render = vk_realloc(
      ctx->allocator,
   hw_setup->renders,
   sizeof(hw_setup->renders[0U]) * (hw_setup->render_count + 1U),
   8U,
      if (!new_hw_render) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
                        ctx->hw_render = &hw_setup->renders[hw_setup->render_count];
   memset(ctx->hw_render, 0U, sizeof(*hw_render));
   ctx->hw_render->ds_attach_idx = VK_ATTACHMENT_UNUSED;
   hw_setup->render_count++;
   ctx->hw_render->depth_init = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   ctx->hw_render->stencil_init = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
               /* Allocate a new subpass in the in-progress render. */
            new_hw_subpasses = vk_realloc(ctx->allocator,
                                 if (!new_hw_subpasses) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               hw_render->subpasses = new_hw_subpasses;
            new_subpasses =
      vk_realloc(ctx->allocator,
            ctx->subpasses,
   sizeof(ctx->subpasses[0U]) * (hw_render->subpass_count + 1U),
   if (!new_subpasses) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
                        subpass = &ctx->subpasses[hw_render->subpass_count];
   subpass->input_subpass = input_subpass;
            /* Save the allocation state at the subpass. */
   result = pvr_copy_alloc(ctx, &subpass->alloc, &alloc);
   if (result != VK_SUCCESS)
                     memset(hw_subpass, 0U, sizeof(*hw_subpass));
   hw_subpass->index = input_subpass->index;
   hw_subpass->z_replicate = -1;
            if (int_ds_attach && ctx->int_ds_attach != int_ds_attach) {
      bool setup_render_ds = false;
   bool stencil_load = false;
            if (int_ds_attach->load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
      depth_load = true;
   setup_render_ds = true;
                  assert(!ctx->ds_load_surface);
      } else if (int_ds_attach->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                  if (int_ds_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      if (int_ds_attach->stencil_load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
      stencil_load = true;
   setup_render_ds = true;
      } else if (int_ds_attach->stencil_load_op ==
                           /* If the depth is loaded then allocate external memory for the depth
   * attachment.
   */
   if (depth_load || stencil_load)
            if (setup_render_ds) {
      assert(hw_render->ds_attach_idx == VK_ATTACHMENT_UNUSED);
                           /* Set up the initialization operations for subpasses. */
   hw_subpass->color_initops = vk_alloc(ctx->allocator,
                           if (!hw_subpass->color_initops) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               for (uint32_t i = 0U; i < input_subpass->color_count; i++) {
      const uint32_t attach_idx = input_subpass->color_attachments[i];
            if (attach_idx == VK_ATTACHMENT_UNUSED)
                     if (int_attach->first_use == -1) {
               /* If the attachment is loaded then off-chip memory must be
   * allocated for it.
   */
                  /* The attachment has been written so load the attachment the next
   * time it is referenced.
   */
      } else {
                     /* Copy the destinations allocated for the color attachments. */
   for (uint32_t i = 0U; i < input_subpass->color_count; i++) {
      const uint32_t attach_idx = input_subpass->color_attachments[i];
   struct pvr_render_int_attachment *int_attach;
            if (attach_idx == VK_ATTACHMENT_UNUSED)
            int_attach = &ctx->int_attach[attach_idx];
            if (int_attach->first_use == -1) {
      assert(int_attach->resource.type == USC_MRT_RESOURCE_TYPE_INVALID);
                  /* If this attachment is being used for the first time then add it
   * to the active list.
   */
   pvr_make_surface_active(ctx,
            } else {
                     /* We can't directly read the on-chip depth so mark subpasses where the depth
   * is written to replicate the value into part of the color storage.
   */
   if (sp_depth.existing_ds_is_input) {
      result = pvr_enable_z_replicate(ctx,
                     if (result != VK_SUCCESS)
               if (sp_depth.incoming_ds_is_input) {
      if (input_subpass->depth_stencil_attachment !=
      sp_depth.existing_ds_attach) {
   result =
      pvr_enable_z_replicate(ctx,
                  if (result != VK_SUCCESS)
      } else {
      assert(pvr_equal_resources(&sp_dsts.existing_zrep,
                  /* Copy the locations of color/input attachments to the output structure.
   * N.B. Need to do this after Z replication in case the replicated depth is
   * an input attachment for the incoming subpass.
   */
   result = pvr_copy_storage_details(ctx, hw_subpass, subpass);
   if (result != VK_SUCCESS)
            if (subpass->z_replicate) {
      result = pvr_copy_z_replicate_details(ctx, hw_subpass, subpass);
   if (result != VK_SUCCESS)
               /* Copy the allocation at the subpass. This will then be updated if this was
   * last use of any attachment.
   */
   pvr_free_alloc(ctx, &ctx->alloc);
            /* Free information about subpass destinations. */
   if (sp_dsts.color)
                           end_merge_subpass:
      if (sp_dsts.color)
                        }
      static void
   pvr_dereference_color_output_list(struct pvr_renderpass_context *ctx,
               {
      for (uint32_t i = 0U; i < subpass->color_count; i++) {
               if (attach_idx != VK_ATTACHMENT_UNUSED)
         }
      static void pvr_dereference_surface_list(struct pvr_renderpass_context *ctx,
                     {
      for (uint32_t i = 0U; i < count; i++) {
      if (attachments[i] != VK_ATTACHMENT_UNUSED)
         }
      static VkResult pvr_schedule_subpass(const struct pvr_device *device,
               {
      struct pvr_renderpass_hwsetup_subpass *hw_subpass;
   struct pvr_renderpass_hwsetup_render *hw_render;
   struct pvr_render_int_subpass *int_subpass;
   struct pvr_render_subpass *subpass;
   uint32_t subpass_num;
            int_subpass = &ctx->int_subpasses[subpass_idx];
            result = pvr_merge_subpass(device, ctx, subpass, &hw_subpass);
   if (result != VK_SUCCESS)
            hw_render = ctx->hw_render;
            /* Record where the subpass was scheduled. */
   ctx->hw_setup->subpass_map[subpass_idx].render =
                  /* Check this subpass was the last use of any attachments. */
   pvr_dereference_color_output_list(ctx, subpass_num, subpass);
   pvr_dereference_surface_list(ctx,
                     if (subpass->depth_stencil_attachment != VK_ATTACHMENT_UNUSED) {
      struct pvr_render_int_attachment *int_depth_attach =
            assert(int_depth_attach->remaining_count > 0U);
            if (int_depth_attach->remaining_count == 0U) {
                     if (int_depth_attach->z_replicate)
               if (int_depth_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      assert(int_depth_attach->stencil_remaining_count > 0U);
               /* The depth attachment has initialized data so load it from memory if it
   * is referenced again.
   */
   int_depth_attach->load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
               /* Mark surfaces which have been the source or destination of an MSAA resolve
   * in the current render.
   */
   for (uint32_t i = 0U; i < subpass->color_count; i++) {
      struct pvr_render_int_attachment *resolve_src;
            if (!subpass->resolve_attachments)
            if (subpass->resolve_attachments[i] == VK_ATTACHMENT_UNUSED)
            assert(subpass->color_attachments[i] <
                  assert(subpass->resolve_attachments[i] <
                  /* Mark the resolve source. */
   assert(resolve_src->last_resolve_src_render <
                  /* Mark the resolve destination. */
   assert(resolve_dst->last_resolve_dst_render <
                  /* If we can't down scale through the PBE then the src must be stored
   * for transfer down scale.
   */
   if (!resolve_src->is_pbe_downscalable &&
      resolve_src->last_read < (int32_t)ctx->pass->subpass_count) {
   resolve_src->last_read = (int32_t)ctx->pass->subpass_count;
                  /* For subpasses dependent on this subpass decrement the unscheduled
   * dependency count.
   */
   for (uint32_t i = 0U; i < int_subpass->out_subpass_count; i++) {
      struct pvr_render_int_subpass *int_dst_subpass =
            assert(int_dst_subpass->in_subpass_count > 0U);
                  }
      static uint32_t pvr_count_uses_in_list(uint32_t *attachments,
               {
               for (uint32_t i = 0U; i < size; i++) {
      if (attachments[i] == attach_idx)
                  }
      static uint32_t
   pvr_count_uses_in_color_output_list(struct pvr_render_subpass *subpass,
         {
               for (uint32_t i = 0U; i < subpass->color_count; i++) {
      if (subpass->color_attachments[i] == attach_idx) {
               if (subpass->resolve_attachments &&
      subpass->resolve_attachments[i] != VK_ATTACHMENT_UNUSED)
                  }
      void pvr_destroy_renderpass_hwsetup(const VkAllocationCallbacks *alloc,
         {
      for (uint32_t i = 0U; i < hw_setup->render_count; i++) {
               vk_free(alloc, hw_render->eot_surfaces);
   vk_free(alloc, hw_render->eot_setup.mrt_resources);
   vk_free(alloc, hw_render->init_setup.mrt_resources);
            for (uint32_t j = 0U; j < hw_render->subpass_count; j++) {
                     vk_free(alloc, subpass->color_initops);
   vk_free(alloc, subpass->input_access);
                           vk_free(alloc, hw_setup->renders);
      }
      VkResult pvr_create_renderpass_hwsetup(
      struct pvr_device *device,
   const VkAllocationCallbacks *alloc,
   struct pvr_render_pass *pass,
   bool disable_merge,
      {
      struct pvr_render_int_attachment *int_attachments;
   struct pvr_render_int_subpass *int_subpasses;
   struct pvr_renderpass_hw_map *subpass_map;
   struct pvr_renderpass_hwsetup *hw_setup;
   struct pvr_renderpass_context *ctx;
   bool *surface_allocate;
            VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &hw_setup, __typeof__(*hw_setup), 1);
   vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                        if (!vk_multialloc_zalloc(&ma, alloc, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT))
            hw_setup->surface_allocate = surface_allocate;
            VK_MULTIALLOC(ma_ctx);
   vk_multialloc_add(&ma_ctx, &ctx, __typeof__(*ctx), 1);
   vk_multialloc_add(&ma_ctx,
                     vk_multialloc_add(&ma_ctx,
                        if (!vk_multialloc_zalloc(&ma_ctx,
                  vk_free(alloc, hw_setup);
               ctx->pass = pass;
   ctx->hw_setup = hw_setup;
   ctx->int_attach = int_attachments;
   ctx->int_subpasses = int_subpasses;
            for (uint32_t i = 0U; i < pass->attachment_count; i++) {
      struct pvr_render_pass_attachment *attachment = &pass->attachments[i];
   struct pvr_render_int_attachment *int_attach = &ctx->int_attach[i];
   const uint32_t pixel_size =
         const uint32_t part_bits =
            int_attach->resource.type = USC_MRT_RESOURCE_TYPE_INVALID;
   int_attach->resource.intermediate_size =
      DIV_ROUND_UP(vk_format_get_blocksizebits(attachment->vk_format),
      int_attach->resource.mrt_desc.intermediate_size =
            for (uint32_t j = 0U; j < pixel_size; j++)
            if (part_bits > 0U) {
      int_attach->resource.mrt_desc.valid_mask[pixel_size] =
               int_attach->load_op = pass->attachments[i].load_op;
   int_attach->stencil_load_op = pass->attachments[i].stencil_load_op;
   int_attach->attachment = attachment;
   int_attach->first_use = -1;
   int_attach->last_use = -1;
   int_attach->last_read = -1;
   int_attach->mrt_idx = -1;
   int_attach->last_resolve_dst_render = -1;
   int_attach->last_resolve_src_render = -1;
   int_attach->z_replicate = false;
            /* Count the number of references to this attachment in subpasses. */
   for (uint32_t j = 0U; j < pass->subpass_count; j++) {
      struct pvr_render_subpass *subpass = &pass->subpasses[j];
   const uint32_t color_output_uses =
         const uint32_t input_attachment_uses =
      pvr_count_uses_in_list(subpass->input_attachments,
                                             if ((uint32_t)subpass->depth_stencil_attachment == i)
               if (int_attach->attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      int_attach->stencil_remaining_count = int_attach->remaining_count;
   if (pass->attachments[i].stencil_store_op ==
      VK_ATTACHMENT_STORE_OP_STORE) {
                  if (pass->attachments[i].store_op == VK_ATTACHMENT_STORE_OP_STORE) {
      int_attach->remaining_count++;
                  for (uint32_t i = 0U; i < pass->subpass_count; i++) {
               int_subpass->subpass = &pass->subpasses[i];
   int_subpass->out_subpass_count = 0U;
   int_subpass->out_subpasses = NULL;
               /* For each dependency of a subpass create an edge in the opposite
   * direction.
   */
   for (uint32_t i = 0U; i < pass->subpass_count; i++) {
               for (uint32_t j = 0U; j < int_subpass->in_subpass_count; j++) {
      uint32_t src_idx = int_subpass->subpass->dep_list[j];
                                    out_subpasses =
      vk_realloc(ctx->allocator,
            int_src_subpass->out_subpasses,
   sizeof(int_src_subpass->out_subpasses[0U]) *
         if (!out_subpasses) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               int_src_subpass->out_subpasses = out_subpasses;
   int_src_subpass->out_subpasses[int_src_subpass->out_subpass_count] =
                                 for (uint32_t i = 0U; i < pass->subpass_count; i++) {
               /* Find a subpass with no unscheduled dependencies. */
   for (j = 0U; j < pass->subpass_count; j++) {
               if (int_subpass->subpass && int_subpass->in_subpass_count == 0U)
      }
            result = pvr_schedule_subpass(device, ctx, j);
   if (result != VK_SUCCESS)
            if (disable_merge) {
      result = pvr_close_render(device, ctx);
   if (result != VK_SUCCESS)
                           /* Finalise the last in-progress render. */
         end_create_renderpass_hwsetup:
      if (result != VK_SUCCESS) {
               if (hw_setup) {
      pvr_destroy_renderpass_hwsetup(alloc, hw_setup);
                  for (uint32_t i = 0U; i < pass->subpass_count; i++) {
               if (int_subpass->out_subpass_count > 0U)
                                    }
