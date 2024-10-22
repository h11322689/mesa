   /*
   * Copyright 2013 VMware, Inc.  All rights reserved.
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
   */
         /**
   * VGPU10 sampler and sampler view functions.
   */
         #include "pipe/p_defines.h"
   #include "util/u_bitmask.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_cmd.h"
   #include "svga_context.h"
   #include "svga_format.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_texture.h"
   #include "svga_sampler_view.h"
   #include "svga_shader.h"
   #include "svga_state.h"
   #include "svga_surface.h"
   #include "svga3d_surfacedefs.h"
      /** Get resource handle for a texture or buffer */
   static inline struct svga_winsys_surface *
   svga_resource_handle(struct pipe_resource *res)
   {
      if (res->target == PIPE_BUFFER) {
         }
   else {
            }
         /**
   * This helper function returns TRUE if the specified resource collides with
   * any of the resources bound to any of the currently bound sampler views.
   */
   bool
   svga_check_sampler_view_resource_collision(const struct svga_context *svga,
               {
      struct pipe_screen *screen = svga->pipe.screen;
            if (svga_screen(screen)->debug.no_surface_view) {
                  if (!svga_curr_shader_use_samplers(svga, shader))
            for (i = 0; i < svga->curr.num_sampler_views[shader]; i++) {
      struct svga_pipe_sampler_view *sv =
            if (sv && res == svga_resource_handle(sv->base.texture)) {
                        }
         /**
   * Check if there are any resources that are both bound to a render target
   * and bound as a shader resource for the given type of shader.
   */
   bool
   svga_check_sampler_framebuffer_resource_collision(struct svga_context *svga,
         {
      struct svga_surface *surf;
            for (i = 0; i < svga->curr.framebuffer.nr_cbufs; i++) {
      surf = svga_surface(svga->curr.framebuffer.cbufs[i]);
   if (surf &&
      svga_check_sampler_view_resource_collision(svga, surf->handle,
                        surf = svga_surface(svga->curr.framebuffer.zsbuf);
   if (surf &&
      svga_check_sampler_view_resource_collision(svga, surf->handle, shader)) {
                  }
         /**
   * Create a DX ShaderResourceSamplerView for the given pipe_sampler_view,
   * if needed.
   */
   enum pipe_error
   svga_validate_pipe_sampler_view(struct svga_context *svga,
         {
               if (sv->id == SVGA3D_INVALID_ID) {
      struct svga_screen *ss = svga_screen(svga->pipe.screen);
   struct pipe_resource *texture = sv->base.texture;
   struct svga_winsys_surface *surface;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDim;
   SVGA3dShaderResourceViewDesc viewDesc;
   enum pipe_format viewFormat = sv->base.format;
            /* vgpu10 cannot create a BGRX view for a BGRA resource, so force it to
   * create a BGRA view (and vice versa).
   */
   if (viewFormat == PIPE_FORMAT_B8G8R8X8_UNORM &&
      svga_texture_device_format_has_alpha(texture)) {
      }
   else if (viewFormat == PIPE_FORMAT_B8G8R8A8_UNORM &&
                        if (target == PIPE_BUFFER) {
      unsigned pf_flags;
   assert(texture->target == PIPE_BUFFER);
   svga_translate_texture_buffer_view_format(viewFormat,
                  }
   else {
                                                         if (target == PIPE_BUFFER) {
               viewDesc.buffer.firstElement = sv->base.u.buf.offset / elem_size;
      }
   else {
      viewDesc.tex.mostDetailedMip = sv->base.u.tex.first_level;
   viewDesc.tex.firstArraySlice = sv->base.u.tex.first_layer;
   viewDesc.tex.mipLevels = (sv->base.u.tex.last_level -
               /* arraySize in viewDesc specifies the number of array slices in a
   * texture array. For 3D texture, last_layer in
   * pipe_sampler_view specifies the last slice of the texture
   * which is different from the last slice in a texture array,
   * hence we need to set arraySize to 1 explicitly.
   */
   viewDesc.tex.arraySize =
                  switch (target) {
   case PIPE_BUFFER:
      resourceDim = SVGA3D_RESOURCE_BUFFER;
      case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      resourceDim = SVGA3D_RESOURCE_TEXTURE1D;
      case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
      resourceDim = SVGA3D_RESOURCE_TEXTURE2D;
      case PIPE_TEXTURE_3D:
      resourceDim = SVGA3D_RESOURCE_TEXTURE3D;
      case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
                  default:
      assert(!"Unexpected texture type");
                        ret = SVGA3D_vgpu10_DefineShaderResourceView(svga->swc,
                                 if (ret != PIPE_OK) {
      util_bitmask_clear(svga->sampler_view_id_bm, sv->id);
                     }
         static enum pipe_error
   update_sampler_resources(struct svga_context *svga, uint64_t dirty)
   {
      enum pipe_error ret = PIPE_OK;
                     for (shader = PIPE_SHADER_VERTEX; shader < PIPE_SHADER_COMPUTE; shader++) {
      SVGA3dShaderResourceViewId ids[PIPE_MAX_SAMPLERS];
   struct svga_winsys_surface *surfaces[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
   unsigned count;
   unsigned nviews;
            count = svga->curr.num_sampler_views[shader];
   for (i = 0; i < count; i++) {
                                       ret = svga_validate_pipe_sampler_view(svga, sv);
                  assert(sv->id != SVGA3D_INVALID_ID);
   ids[i] = sv->id;
      }
   else {
      surfaces[i] = NULL;
   ids[i] = SVGA3D_INVALID_ID;
                  for (; i < svga->state.hw_draw.num_sampler_views[shader]; i++) {
      ids[i] = SVGA3D_INVALID_ID;
   surfaces[i] = NULL;
               /* Number of ShaderResources that need to be modified. This includes
   * the one that need to be unbound.
   */
   nviews = MAX2(svga->state.hw_draw.num_sampler_views[shader], count);
   if (nviews > 0) {
      if (count != svga->state.hw_draw.num_sampler_views[shader] ||
      memcmp(sampler_views, svga->state.hw_draw.sampler_views[shader],
         SVGA3dShaderResourceViewId *pIds = ids;
                  /* Loop through the sampler view list to only emit
   * the sampler views that are not already in the
   * corresponding entries in the device's
   * shader resource list.
   */
                                    if (!emit && i == nviews-1) {
      /* Include the last sampler view in the next emit
   * if it is different.
   */
   emit = true;
                     if (emit) {
      /* numSR can only be 0 if the first entry of the list
   * is the same as the one in the device list.
   * In this case, * there is nothing to send yet.
   */
   if (numSR) {
      ret = SVGA3D_vgpu10_SetShaderResources(
            svga->swc,
                           if (ret != PIPE_OK)
      }
   pIds += (numSR + 1);
   pSurf += (numSR + 1);
      }
                     /* Save referenced sampler views in the hw draw state.  */
   svga->state.hw_draw.num_sampler_views[shader] = count;
   for (i = 0; i < nviews; i++) {
      pipe_sampler_view_reference(
      &svga->state.hw_draw.sampler_views[shader][i],
                     /* Handle polygon stipple sampler view */
   if (svga->curr.rast->templ.poly_stipple_enable) {
      const unsigned unit =
         struct svga_pipe_sampler_view *sv = svga->polygon_stipple.sampler_view;
            assert(sv);
   if (!sv) {
                  ret = svga_validate_pipe_sampler_view(svga, sv);
   if (ret != PIPE_OK)
            surface = svga_resource_handle(sv->base.texture);
   ret = SVGA3D_vgpu10_SetShaderResources(
            svga->swc,
   svga_shader_type(PIPE_SHADER_FRAGMENT),
   unit, /* startView */
   1,
   }
      }
         struct svga_tracked_state svga_hw_sampler_bindings = {
      "shader resources emit",
   SVGA_NEW_STIPPLE |
   SVGA_NEW_TEXTURE_BINDING,
      };
            static enum pipe_error
   update_samplers(struct svga_context *svga, uint64_t dirty )
   {
      enum pipe_error ret = PIPE_OK;
                     for (shader = PIPE_SHADER_VERTEX; shader < PIPE_SHADER_COMPUTE; shader++) {
      const unsigned count = svga->curr.num_samplers[shader];
   SVGA3dSamplerId ids[PIPE_MAX_SAMPLERS*2];
   unsigned i;
   unsigned nsamplers = 0;
   bool sampler_state_mapping =
            for (i = 0; i < count; i++) {
                     /* _NEW_FS */
   if (shader == PIPE_SHADER_FRAGMENT) {
                                 /* Use the alternate sampler state with the compare
   * bit disabled when comparison is done in the shader and
   * sampler state mapping is not enabled.
   */
                  if (!sampler_state_mapping) {
      if (sampler) {
      SVGA3dSamplerId id = sampler->id[fs_shadow];
   assert(id != SVGA3D_INVALID_ID);
      }
   else {
         }
      }
   else {
      if (sampler) {
                     /* Check if the sampler id is already on the ids list */
   unsigned k;
   for (k = 0; k < nsamplers; k++) {
                                                if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
      /*
   * add the alternate sampler state as well as the shader
   * might use this alternate sampler state which has comparison
   * disabled when the comparison is done in the shader.
   */
                           for (i = nsamplers; i < svga->state.hw_draw.num_samplers[shader]; i++) {
                  unsigned nsamplerIds =
                        if (nsamplers > SVGA3D_DX_MAX_SAMPLERS) {
      debug_warn_once("Too many sampler states");
               if (nsamplers != svga->state.hw_draw.num_samplers[shader] ||
                     /* HW state is really changing */
   ret = SVGA3D_vgpu10_SetSamplers(svga->swc,
                           if (ret != PIPE_OK)
         memcpy(svga->state.hw_draw.samplers[shader], ids,
                           /* Handle polygon stipple sampler texture */
   if (svga->curr.rast->templ.poly_stipple_enable) {
      const unsigned unit =
                  assert(sampler);
   if (!sampler) {
                  if (svga->state.hw_draw.samplers[PIPE_SHADER_FRAGMENT][unit]
      != sampler->id[0]) {
   ret = SVGA3D_vgpu10_SetSamplers(svga->swc,
                                          /* save the polygon stipple sampler in the hw draw state */
   svga->state.hw_draw.samplers[PIPE_SHADER_FRAGMENT][unit] =
      }
                  }
         struct svga_tracked_state svga_hw_sampler = {
      "texture sampler emit",
   (SVGA_NEW_FS |
   SVGA_NEW_SAMPLER |
   SVGA_NEW_STIPPLE),
      };
         static enum pipe_error
   update_cs_sampler_resources(struct svga_context *svga, uint64_t dirty)
   {
      enum pipe_error ret = PIPE_OK;
                     SVGA3dShaderResourceViewId ids[PIPE_MAX_SAMPLERS];
   struct svga_winsys_surface *surfaces[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
   unsigned count;
   unsigned nviews;
   unsigned i;
            count = svga->curr.num_sampler_views[shader];
   if (!cs || !cs->base.info.uses_samplers)
            for (i = 0; i < count; i++) {
      struct svga_pipe_sampler_view *sv =
            if (sv) {
               ret = svga_validate_pipe_sampler_view(svga, sv);
                  assert(sv->id != SVGA3D_INVALID_ID);
   ids[i] = sv->id;
      }
   else {
      surfaces[i] = NULL;
   ids[i] = SVGA3D_INVALID_ID;
                  for (; i < svga->state.hw_draw.num_sampler_views[shader]; i++) {
      ids[i] = SVGA3D_INVALID_ID;
   surfaces[i] = NULL;
               /* Number of ShaderResources that need to be modified. This includes
   * the one that need to be unbound.
   */
   nviews = MAX2(svga->state.hw_draw.num_sampler_views[shader], count);
   if (nviews > 0) {
      if (count != svga->state.hw_draw.num_sampler_views[shader] ||
      memcmp(sampler_views, svga->state.hw_draw.sampler_views[shader],
         SVGA3dShaderResourceViewId *pIds = ids;
                  /* Loop through the sampler view list to only emit the sampler views
   * that are not already in the corresponding entries in the device's
   * shader resource list.
   */
                                    if (!emit && i == nviews - 1) {
      /* Include the last sampler view in the next emit
   * if it is different.
   */
   emit = true;
                     if (emit) {
      /* numSR can only be 0 if the first entry of the list
   * is the same as the one in the device list.
   * In this case, * there is nothing to send yet.
   */
   if (numSR) {
      ret = SVGA3D_vgpu10_SetShaderResources(svga->swc,
                                    if (ret != PIPE_OK)
      }
   pIds += (numSR + 1);
   pSurf += (numSR + 1);
      }
   else
               /* Save referenced sampler views in the hw draw state.  */
   svga->state.hw_draw.num_sampler_views[shader] = count;
   for (i = 0; i < nviews; i++) {
      pipe_sampler_view_reference(
      &svga->state.hw_draw.sampler_views[shader][i],
         }
      }
         struct svga_tracked_state svga_hw_cs_sampler_bindings = {
      "cs shader resources emit",
   SVGA_NEW_TEXTURE_BINDING,
      };
      static enum pipe_error
   update_cs_samplers(struct svga_context *svga, uint64_t dirty )
   {
      enum pipe_error ret = PIPE_OK;
                     const unsigned count = svga->curr.num_samplers[shader];
   SVGA3dSamplerId ids[PIPE_MAX_SAMPLERS];
   unsigned i;
            for (i = 0; i < count; i++) {
      if (svga->curr.sampler[shader][i]) {
      ids[i] = svga->curr.sampler[shader][i]->id[0];
      }
   else {
                     for (; i < svga->state.hw_draw.num_samplers[shader]; i++) {
                  nsamplers = MAX2(svga->state.hw_draw.num_samplers[shader], count);
   if (nsamplers > 0) {
      if (count != svga->state.hw_draw.num_samplers[shader] ||
      memcmp(ids, svga->state.hw_draw.samplers[shader],
         /* HW state is really changing */
   ret = SVGA3D_vgpu10_SetSamplers(svga->swc,
                                          memcpy(svga->state.hw_draw.samplers[shader], ids,
                           }
         struct svga_tracked_state svga_hw_cs_sampler = {
      "texture cs sampler emit",
   (SVGA_NEW_CS |
   SVGA_NEW_SAMPLER),
      };
