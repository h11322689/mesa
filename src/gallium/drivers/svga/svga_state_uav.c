   /**********************************************************
   * Copyright 2022 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "pipe/p_defines.h"
   #include "util/u_bitmask.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_context.h"
   #include "svga_cmd.h"
   #include "svga_debug.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_texture.h"
   #include "svga_surface.h"
   #include "svga_sampler_view.h"
   #include "svga_format.h"
         /**
   * Initialize uav cache.
   */
   void
   svga_uav_cache_init(struct svga_context *svga)
   {
               for (unsigned i = 0; i < ARRAY_SIZE(cache->uaViews); i++) {
      cache->uaViews[i].uaViewId = SVGA3D_INVALID_ID;
      }
   cache->num_uaViews = 0;
      }
         /**
   * Helper function to compare two image view descriptions.
   * Return TRUE if they are identical.
   */
   static bool
   image_view_desc_identical(struct pipe_image_view *img1,
         {
      if ((img1->resource != img2->resource) ||
      (img1->format != img2->format) ||
   (img1->access != img2->access) ||
   (img1->shader_access != img2->shader_access))
         if (img1->resource->target == PIPE_BUFFER) {
      if ((img1->u.buf.offset != img2->u.buf.offset) ||
      (img1->u.buf.size != img2->u.buf.size))
               }
         /**
   * Helper function to compare two shader buffer descriptions.
   * Return TRUE if they are identical.
   */
   static bool
   shader_buffer_desc_identical(struct pipe_shader_buffer *buf1,
         {
         }
         /**
   * Helper function to compare two uav cache entry descriptions.
   * Return TRUE if they are identical.
   */
   static bool
   uav_desc_identical(enum svga_uav_type uav_type,
         {
      if (uav_type == SVGA_IMAGE_VIEW) {
      struct svga_image_view *img = (struct svga_image_view *)desc;
   struct svga_image_view *uav_img = (struct svga_image_view *)uav_desc;
   if (img->resource != uav_img->resource)
               }
   else {
      struct svga_shader_buffer *buf = (struct svga_shader_buffer *)desc;
   struct svga_shader_buffer *uav_buf =
            if (buf->resource != uav_buf->resource)
            if (buf->handle != uav_buf->handle)
                  }
         /**
   * Find a uav object for the specified image view or shader buffer.
   * Returns uav entry if there is a match; otherwise returns NULL.
   */
   static struct svga_uav *
   svga_uav_cache_find_uav(struct svga_context *svga,
                     {
               for (unsigned i = 0; i < cache->num_uaViews; i++) {
      if ((cache->uaViews[i].type == uav_type) &&
      (cache->uaViews[i].uaViewId != SVGA3D_INVALID_ID) &&
   uav_desc_identical(uav_type, desc, &cache->uaViews[i].desc)) {
         }
      }
         /**
   * Add a uav entry to the cache for the specified image view or
   * shaderr bufferr.
   */
   static struct svga_uav *
   svga_uav_cache_add_uav(struct svga_context *svga,
                        enum svga_uav_type uav_type,
      {
      struct svga_cache_uav *cache = &svga->cache_uav;
   unsigned i = cache->next_uaView;
            if (i > ARRAY_SIZE(cache->uaViews)) {
      debug_printf("No room to add uav to the cache.\n");
                        /* update the next available uav slot index */
            uav->type = uav_type;
   memcpy(&uav->desc, desc, desc_len);
   pipe_resource_reference(&uav->resource, res);
                        }
         /**
   * Bump the timestamp of the specified uav for the specified pipeline,
   * so the uav will not be prematurely purged.
   */
   static void
   svga_uav_cache_use_uav(struct svga_context *svga,
               {
      assert(uav != NULL);
               }
         /**
   * Purge any unused uav from the cache.
   */
   static void
   svga_uav_cache_purge(struct svga_context *svga, enum svga_pipe_type pipe_type)
   {
      struct svga_cache_uav *cache = &svga->cache_uav;
   unsigned timestamp = svga->state.uav_timestamp[pipe_type];
   unsigned other_pipe_type = !pipe_type;
            unsigned last_uav = -1;
   for (unsigned i = 0; i < cache->num_uaViews; i++, uav++) {
      if (uav->uaViewId != SVGA3D_INVALID_ID) {
                           /* Reset the timestamp for this uav in the specified
   * pipeline first.
                  /* Then check if the uav is currently in use in other pipeline.
   * If yes, then don't delete the uav yet.
   * If no, then we can mark the uav as to be destroyed.
                     /* The unused uav can be destroyed, but will be destroyed
   * in the next set_image_views or set_shader_buffers,
   * or at context destroy time, because we do not want to
   * restart the state update if the Destroy command cannot be
                        /* Mark this entry as available */
   uav->next_uaView = cache->next_uaView;
   uav->uaViewId = SVGA3D_INVALID_ID;
               }
      }
         /**
   * A helper function to create an uav.
   */
   SVGA3dUAViewId
   svga_create_uav(struct svga_context *svga,
                  SVGA3dUAViewDesc *desc,
      {
      SVGA3dUAViewId uaViewId;
            /* allocate a uav id */
                     ret = SVGA3D_sm5_DefineUAView(svga->swc, uaViewId, surf,
            if (ret != PIPE_OK) {
      util_bitmask_clear(svga->uav_id_bm, uaViewId);
                  }
         /**
   * Destroy any pending unused uav
   */
   void
   svga_destroy_uav(struct svga_context *svga)
   {
                        while ((index = util_bitmask_get_next_index(svga->uav_to_free_id_bm, index))
                     SVGA_RETRY(svga, SVGA3D_sm5_DestroyUAView(svga->swc, index));
   util_bitmask_clear(svga->uav_id_bm, index);
                  }
         /**
   * Rebind ua views.
   * This function is called at the beginning of each new command buffer to make sure
   * the resources associated with the ua views are properly paged-in.
   */
   enum pipe_error
   svga_rebind_uav(struct svga_context *svga)
   {
      struct svga_winsys_context *swc = svga->swc;
   struct svga_hw_draw_state *hw = &svga->state.hw_draw;
                     for (unsigned i = 0; i < hw->num_uavs; i++) {
      if (hw->uaViews[i]) {
      ret = swc->resource_rebind(swc, hw->uaViews[i], NULL,
         if (ret != PIPE_OK)
         }
               }
      static int
   svga_find_uav_from_list(struct svga_context *svga, SVGA3dUAViewId uaViewId,
         {
      for (unsigned i = 0; i < num_uavs; i++) {
      if (uaViewsId[i] == uaViewId)
      }
      }
      /**
   * A helper function to create the uaView lists from the
   * bound shader images and shader buffers.
   */
   static enum pipe_error
   svga_create_uav_list(struct svga_context *svga,
                        enum svga_pipe_type pipe_type,
      {
      enum pipe_shader_type first_shader, last_shader;
   struct svga_uav *uav;
            /* Increase uav timestamp */
            if (pipe_type == SVGA_PIPE_GRAPHICS) {
      first_shader = PIPE_SHADER_VERTEX;
      } else {
      first_shader = PIPE_SHADER_COMPUTE;
               for (enum pipe_shader_type shader = first_shader;
               unsigned num_image_views = svga->curr.num_image_views[shader];
            SVGA_DBG(DEBUG_UAV,
                  /* add enabled shader images to the uav list */
   if (num_image_views) {
      num_image_views = MIN2(num_image_views, num_free_uavs-*num_uavs);
   for (unsigned i = 0; i < num_image_views; i++) {
      struct svga_image_view *cur_image_view =
                                    /* First check if there is already a uav defined for this
   * image view.
   */
                        /* If there isn't one, create a uav for this image view. */
   if (uav == NULL) {
                           /* Add the uav to the cache */
   uav = svga_uav_cache_add_uav(svga, SVGA_IMAGE_VIEW,
                                                                                 /* The uav is not already on the uaView list, add it */
   if (uav_index == -1) {
      uav_index = *num_uavs;
   (*num_uavs)++;
   if (res->target == PIPE_BUFFER)
                                    /* Save the uav slot index for the image view for later reference
   * to create the uav mapping in the shader key.
   */
                     /* add enabled shader buffers to the uav list */
   if (num_shader_buffers) {
      num_shader_buffers = MIN2(num_shader_buffers, num_free_uavs-*num_uavs);
   for (unsigned i = 0; i < num_shader_buffers; i++) {
      struct svga_shader_buffer *cur_sbuf =
                           if (svga_shader_buffer_can_use_srv(svga, shader, i, cur_sbuf)) {
               ret = svga_shader_buffer_bind_srv(svga, shader, i, cur_sbuf);
   if (ret != PIPE_OK)
   } else {
               ret = svga_shader_buffer_unbind_srv(svga, shader, i, cur_sbuf);
                     if (res) {
                           /* First check if there is already a uav defined for this
   * shader buffer.
   */
                        /* If there isn't one, create a uav for this shader buffer. */
   if (uav == NULL) {
                                          /* Add the uav to the cache */
   uav = svga_uav_cache_add_uav(svga, SVGA_SHADER_BUFFER,
                                                                           /* The uav is not already on the uaView list, add it */
   if (uav_index == -1) {
      uav_index = *num_uavs;
   (*num_uavs)++;
                     /* Save the uav slot index for later reference
   * to create the uav mapping in the shader key.
   */
                        /* Since atomic buffers are not specific to a particular shader type,
   * add any enabled atomic buffers to the uav list when we are done adding
   * shader specific uavs.
                     SVGA_DBG(DEBUG_UAV,
            if (num_atomic_buffers) {
               for (unsigned i = 0; i < num_atomic_buffers; i++) {
      struct svga_shader_buffer *cur_sbuf = &svga->curr.atomic_buffers[i];
                  if (res) {
      /* Get the buffer handle that can be bound as uav. */
                  /* First check if there is already a uav defined for this
   * shader buffer.
   */
   uav = svga_uav_cache_find_uav(svga, SVGA_SHADER_BUFFER,
                  /* If there isn't one, create a uav for this shader buffer. */
   if (uav == NULL) {
                                          /* Add the uav to the cache */
   uav = svga_uav_cache_add_uav(svga, SVGA_SHADER_BUFFER,
                                                                           /* The uav is not already on the uaView list, add it */
   if (uav_index == -1) {
      uav_index = *num_uavs;
   (*num_uavs)++;
   uaViews[uav_index] = svga_buffer(res)->handle;
                  /* Save the uav slot index for the atomic buffer for later reference
   * to create the uav mapping in the shader key.
   */
                  /* Reset the rest of the ua views list */
   for (unsigned u = *num_uavs;
      u < ARRAY_SIZE(svga->state.hw_draw.uaViewIds); u++) {
   uaViewIds[u] = SVGA3D_INVALID_ID;
                  }
         /**
   * A helper function to save the current hw uav state.
   */
   static void
   svga_save_uav_state(struct svga_context *svga,
                     enum svga_pipe_type pipe_type,
   {
      enum pipe_shader_type first_shader, last_shader;
            if (pipe_type == SVGA_PIPE_GRAPHICS) {
      first_shader = PIPE_SHADER_VERTEX;
      } else {
      first_shader = PIPE_SHADER_COMPUTE;
               for (enum pipe_shader_type shader = first_shader;
               /**
   * Save the current shader images
   */
   for (i = 0; i < ARRAY_SIZE(svga->curr.image_views[0]); i++) {
      struct svga_image_view *cur_image_view =
                        /* Save the hw state for image view */
               /**
   * Save the current shader buffers
   */
   for (i = 0; i < ARRAY_SIZE(svga->curr.shader_buffers[0]); i++) {
      struct svga_shader_buffer *cur_shader_buffer =
                        /* Save the hw state for image view */
               svga->state.hw_draw.num_image_views[shader] =
         svga->state.hw_draw.num_shader_buffers[shader] =
               /**
   * Save the current atomic buffers
   */
   for (i = 0; i < ARRAY_SIZE(svga->curr.atomic_buffers); i++) {
      struct svga_shader_buffer *cur_buf = &svga->curr.atomic_buffers[i];
            /* Save the hw state for atomic buffers */
                        /**
   * Save the hw state for uaviews
   */
   if (pipe_type == SVGA_PIPE_COMPUTE) {
      svga->state.hw_draw.num_cs_uavs = num_uavs;
   memcpy(svga->state.hw_draw.csUAViewIds, uaViewIds,
         memcpy(svga->state.hw_draw.csUAViews, uaViews,
      }
   else {
      svga->state.hw_draw.num_uavs = num_uavs;
   memcpy(svga->state.hw_draw.uaViewIds, uaViewIds,
         memcpy(svga->state.hw_draw.uaViews, uaViews,
               /* purge the uav cache */
      }
         /**
   * A helper function to determine if we need to resend the SetUAViews command.
   * We need to resend the SetUAViews command when uavSpliceIndex is to
   * be changed because the existing index overlaps with render target views, or
   * the image views/shader buffers are changed.
   */
   static bool
   need_to_set_uav(struct svga_context *svga,
                  int uavSpliceIndex,
      {
      /* If number of render target views changed */
   if (uavSpliceIndex != svga->state.hw_draw.uavSpliceIndex)
            /* If number of render target views + number of ua views exceeds
   * the max uav count, we will need to trim the ua views.
   */
   if ((uavSpliceIndex + num_uavs) > SVGA_MAX_UAVIEWS)
            /* If uavs are different */
   if (memcmp(svga->state.hw_draw.uaViewIds, uaViewIds,
            memcmp(svga->state.hw_draw.uaViews, uaViews,
               /* If image views are different */
   for (enum pipe_shader_type shader = PIPE_SHADER_VERTEX;
      shader < PIPE_SHADER_COMPUTE; shader++) {
   unsigned num_image_views = svga->curr.num_image_views[shader];
   if ((num_image_views != svga->state.hw_draw.num_image_views[shader]) ||
      memcmp(svga->state.hw_draw.image_views[shader],
                     /* If shader buffers are different */
   unsigned num_shader_buffers = svga->curr.num_shader_buffers[shader];
   if ((num_shader_buffers != svga->state.hw_draw.num_shader_buffers[shader]) ||
      memcmp(svga->state.hw_draw.shader_buffers[shader],
         svga->curr.shader_buffers[shader],
            /* If atomic buffers are different */
   unsigned num_atomic_buffers = svga->curr.num_atomic_buffers;
   if ((num_atomic_buffers != svga->state.hw_draw.num_atomic_buffers) ||
      memcmp(svga->state.hw_draw.atomic_buffers, svga->curr.atomic_buffers,
                  }
         /**
   * Update ua views in the HW for the draw pipeline by sending the
   * SetUAViews command.
   */
   static enum pipe_error
   update_uav(struct svga_context *svga, uint64_t dirty)
   {
      enum pipe_error ret = PIPE_OK;
   unsigned num_uavs = 0;
   SVGA3dUAViewId uaViewIds[SVGA_MAX_UAVIEWS];
            /* Determine the uavSpliceIndex since uav and render targets view share the
   * same bind points.
   */
            /* Number of free uav entries available for shader images and buffers */
                     /* Create the uav list for graphics pipeline */
   ret = svga_create_uav_list(svga, SVGA_PIPE_GRAPHICS, num_free_uavs,
         if (ret != PIPE_OK)
            /* check to see if we need to resend the SetUAViews command */
   if (!need_to_set_uav(svga, uavSpliceIndex, num_uavs, uaViewIds, uaViews))
            /* Send the SetUAViews command */
   SVGA_DBG(DEBUG_UAV, "%s: SetUAViews uavSpliceIndex=%d", __func__,
         #ifdef DEBUG
      for (unsigned i = 0; i < ARRAY_SIZE(uaViewIds); i++) {
         }
      #endif
         ret = SVGA3D_sm5_SetUAViews(svga->swc, uavSpliceIndex, SVGA_MAX_UAVIEWS,
         if (ret != PIPE_OK)
            /* Save the uav hw state */
            /* Save the uavSpliceIndex as this determines the starting register index
   * for the first uav used in the shader
   */
         done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         struct svga_tracked_state svga_hw_uav = {
      "shader image view",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_SHADER_BUFFER |
   SVGA_NEW_FRAME_BUFFER),
      };
         /**
   * A helper function to determine if we need to resend the SetCSUAViews command.
   */
   static bool
   need_to_set_cs_uav(struct svga_context *svga,
                     {
               if (svga->state.hw_draw.num_cs_uavs != num_uavs)
            /* If uavs are different */
   if (memcmp(svga->state.hw_draw.csUAViewIds, uaViewIds,
            memcmp(svga->state.hw_draw.csUAViews, uaViews,
               /* If image views are different */
   unsigned num_image_views = svga->curr.num_image_views[shader];
   if ((num_image_views != svga->state.hw_draw.num_image_views[shader]) ||
      memcmp(svga->state.hw_draw.image_views[shader],
         svga->curr.image_views[shader],
         /* If atomic buffers are different */
   unsigned num_atomic_buffers = svga->curr.num_atomic_buffers;
   if ((num_atomic_buffers != svga->state.hw_draw.num_atomic_buffers) ||
      memcmp(svga->state.hw_draw.atomic_buffers, svga->curr.atomic_buffers,
                  }
         /**
   * Update ua views in the HW for the compute pipeline by sending the
   * SetCSUAViews command.
   */
   static enum pipe_error
   update_cs_uav(struct svga_context *svga, uint64_t dirty)
   {
      enum pipe_error ret = PIPE_OK;
   unsigned num_uavs = 0;
   SVGA3dUAViewId uaViewIds[SVGA_MAX_UAVIEWS];
            /* Number of free uav entries available for shader images and buffers */
                     /* Create the uav list */
   ret = svga_create_uav_list(svga, SVGA_PIPE_COMPUTE, num_free_uavs,
         if (ret != PIPE_OK)
            /* Check to see if we need to resend the CSSetUAViews command */
   if (!need_to_set_cs_uav(svga, num_uavs, uaViewIds, uaViews))
                           #ifdef DEBUG
      for (unsigned i = 0; i < ARRAY_SIZE(uaViewIds); i++) {
         }
      #endif
         ret = SVGA3D_sm5_SetCSUAViews(svga->swc, SVGA_MAX_UAVIEWS,
         if (ret != PIPE_OK)
            /* Save the uav hw state */
         done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         struct svga_tracked_state svga_hw_cs_uav = {
      "shader image view",
   (SVGA_NEW_IMAGE_VIEW |
   SVGA_NEW_SHADER_BUFFER |
   SVGA_NEW_FRAME_BUFFER),
      };
