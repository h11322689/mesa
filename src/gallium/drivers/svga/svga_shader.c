   /**********************************************************
   * Copyright 2008-2023 VMware, Inc.  All rights reserved.
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
      #include "util/u_bitmask.h"
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "svga_context.h"
   #include "svga_cmd.h"
   #include "svga_format.h"
   #include "svga_shader.h"
   #include "svga_tgsi.h"
   #include "svga_resource_texture.h"
   #include "VGPU10ShaderTokens.h"
      #include "compiler/nir/nir.h"
   #include "compiler/glsl/gl_nir.h"
   #include "nir/nir_to_tgsi.h"
         /**
   * This bit isn't really used anywhere.  It only serves to help
   * generate a unique "signature" for the vertex shader output bitmask.
   * Shader input/output signatures are used to resolve shader linking
   * issues.
   */
   #define FOG_GENERIC_BIT (((uint64_t) 1) << 63)
         /**
   * Use the shader info to generate a bitmask indicating which generic
   * inputs are used by the shader.  A set bit indicates that GENERIC[i]
   * is used.
   */
   uint64_t
   svga_get_generic_inputs_mask(const struct tgsi_shader_info *info)
   {
      unsigned i;
            for (i = 0; i < info->num_inputs; i++) {
      if (info->input_semantic_name[i] == TGSI_SEMANTIC_GENERIC) {
      unsigned j = info->input_semantic_index[i];
   assert(j < sizeof(mask) * 8);
                     }
         /**
   * Scan shader info to return a bitmask of written outputs.
   */
   uint64_t
   svga_get_generic_outputs_mask(const struct tgsi_shader_info *info)
   {
      unsigned i;
            for (i = 0; i < info->num_outputs; i++) {
      switch (info->output_semantic_name[i]) {
   case TGSI_SEMANTIC_GENERIC:
      {
      unsigned j = info->output_semantic_index[i];
   assert(j < sizeof(mask) * 8);
      }
      case TGSI_SEMANTIC_FOG:
      mask |= FOG_GENERIC_BIT;
                     }
            /**
   * Given a mask of used generic variables (as returned by the above functions)
   * fill in a table which maps those indexes to small integers.
   * This table is used by the remap_generic_index() function in
   * svga_tgsi_decl_sm30.c
   * Example: if generics_mask = binary(1010) it means that GENERIC[1] and
   * GENERIC[3] are used.  The remap_table will contain:
   *   table[1] = 0;
   *   table[3] = 1;
   * The remaining table entries will be filled in with the next unused
   * generic index (in this example, 2).
   */
   void
   svga_remap_generics(uint64_t generics_mask,
         {
      /* Note texcoord[0] is reserved so start at 1 */
            for (i = 0; i < MAX_GENERIC_VARYING; i++) {
                  /* for each bit set in generic_mask */
   while (generics_mask) {
      unsigned index = ffsll(generics_mask) - 1;
   remap_table[index] = count++;
         }
         /**
   * Use the generic remap table to map a TGSI generic varying variable
   * index to a small integer.  If the remapping table doesn't have a
   * valid value for the given index (the table entry is -1) it means
   * the fragment shader doesn't use that VS output.  Just allocate
   * the next free value in that case.  Alternately, we could cull
   * VS instructions that write to register, or replace the register
   * with a dummy temp register.
   * XXX TODO: we should do one of the later as it would save precious
   * texcoord registers.
   */
   int
   svga_remap_generic_index(int8_t remap_table[MAX_GENERIC_VARYING],
         {
               if (generic_index >= MAX_GENERIC_VARYING) {
      /* just don't return a random/garbage value */
               if (remap_table[generic_index] == -1) {
      /* This is a VS output that has no matching PS input.  Find a
   * free index.
   */
   int i, max = 0;
   for (i = 0; i < MAX_GENERIC_VARYING; i++) {
         }
                  }
      static const enum pipe_swizzle copy_alpha[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Z,
   PIPE_SWIZZLE_W,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
      static const enum pipe_swizzle set_alpha[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Z,
   PIPE_SWIZZLE_1,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
      static const enum pipe_swizzle set_000X[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
      static const enum pipe_swizzle set_XXXX[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
      static const enum pipe_swizzle set_XXX1[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_1,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
      static const enum pipe_swizzle set_XXXY[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
      static const enum pipe_swizzle set_YYYY[PIPE_SWIZZLE_MAX] = {
      PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_0,
   PIPE_SWIZZLE_1,
      };
         static VGPU10_RESOURCE_RETURN_TYPE
   vgpu10_return_type(enum pipe_format format)
   {
      if (util_format_is_unorm(format))
         else if (util_format_is_snorm(format))
         else if (util_format_is_pure_uint(format))
         else if (util_format_is_pure_sint(format))
         else if (util_format_is_float(format))
         else
      }
         /**
   * A helper function to return TRUE if the specified format
   * is a supported format for sample_c instruction.
   */
   static bool
   isValidSampleCFormat(enum pipe_format format)
   {
         }
         /**
   * Initialize the shader-neutral fields of svga_compile_key from context
   * state.  This is basically the texture-related state.
   */
   void
   svga_init_shader_key_common(const struct svga_context *svga,
                     {
      unsigned i, idx = 0;
                     /* In case the number of samplers and sampler_views doesn't match,
   * loop over the upper of the two counts.
   */
   key->num_textures = MAX2(svga->curr.num_sampler_views[shader_type],
            if (!shader->info.uses_samplers)
                     /* Set sampler_state_mapping only if GL43 is supported and
   * the number of samplers exceeds SVGA limit or the sampler state
   * mapping env is set.
   */
   bool sampler_state_mapping =
            key->sampler_state_mapping =
            for (i = 0; i < key->num_textures; i++) {
      struct pipe_sampler_view *view = svga->curr.sampler_views[shader_type][i];
   const struct svga_sampler_state
            if (view) {
                     key->tex[i].target = target;
   key->tex[i].sampler_return_type = vgpu10_return_type(view->format);
   key->tex[i].sampler_view = 1;
               /* 1D/2D array textures with one slice and cube map array textures
   * with one cube are treated as non-arrays by the SVGA3D device.
   * Set the is_array flag only if we know that we have more than 1
   * element.  This will be used to select shader instruction/resource
   * types during shader translation.
   */
   switch (target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      key->tex[i].is_array = view->texture->array_size > 1;
      case PIPE_TEXTURE_CUBE_ARRAY:
      key->tex[i].is_array = view->texture->array_size > 6;
      default:
                                 const enum pipe_swizzle *swizzle_tab;
   if (target == PIPE_BUFFER) {
                                       svga_translate_texture_buffer_view_format(view->format,
         if (tf_flags & TF_000X)
         else if (tf_flags & TF_XXXX)
         else if (tf_flags & TF_XXX1)
         else if (tf_flags & TF_XXXY)
         else
      }
   else {
      /* If we have a non-alpha view into an svga3d surface with an
   * alpha channel, then explicitly set the alpha channel to 1
   * when sampling. Note that we need to check the
   * actual device format to cover also imported surface cases.
   */
   swizzle_tab =
                        if (view->texture->format == PIPE_FORMAT_DXT1_RGB ||
                  if (view->format == PIPE_FORMAT_X24S8_UINT ||
                  /* Save the compare function as we need to handle
   * depth compare in the shader.
   */
                  /* Set the compare_in_shader bit if the view format
   * is not a supported format for shadow compare.
   * In this case, we'll do the comparison in the shader.
   */
   if ((sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) &&
      !isValidSampleCFormat(view->format)) {
                  key->tex[i].swizzle_r = swizzle_tab[view->swizzle_r];
   key->tex[i].swizzle_g = swizzle_tab[view->swizzle_g];
   key->tex[i].swizzle_b = swizzle_tab[view->swizzle_b];
      }
   key->tex[i].sampler_view = 0;
                  if (sampler) {
      if (!sampler->normalized_coords) {
      if (view) {
      }
                           if (sampler->magfilter == SVGA3D_TEX_FILTER_NEAREST ||
      sampler->minfilter == SVGA3D_TEX_FILTER_NEAREST) {
                  if (!sampler_state_mapping) {
      /* Use the same index if sampler state mapping is not supported */
   key->tex[i].sampler_index = i;
                        /* The current samplers list can have redundant entries.
   * In order to allow the number of bound samplers within the
   * max limit supported by SVGA, we'll recreate the list with
                  /* Check to see if this sampler is already on the list.
   * If so, set the sampler index of this sampler to the
   * same sampler index.
   */
                                    /* if this sampler is not added to the new list yet,
                                                                                          }
   else {
         }
                                       /* Save info about which constant buffers are to be viewed
   * as srv raw buffers in the shader key.
   */
   if (shader->info.const_buffers_declared &
      svga->state.raw_constbufs[shader_type]) {
   key->raw_constbufs = svga->state.raw_constbufs[shader_type] &
               /* beginning index for srv for raw constant buffers */
            if (shader->info.uses_images || shader->info.uses_hw_atomic ||
               /* Save the uavSpliceIndex which is the index used for the first uav
   * in the draw pipeline. For compute, uavSpliceIndex is always 0.
   */
                           /* Also get the texture data type to be used in the uav declaration */
                                             if (resource) {
                              /* Save the image resource target in the shader key because
   * for single layer image view, the resource target in the
   * tgsi shader is changed to a different texture target.
   */
   key->images[i].resource_target = resource->target;
   if (resource->target == PIPE_TEXTURE_3D ||
      resource->target == PIPE_TEXTURE_1D_ARRAY ||
   resource->target == PIPE_TEXTURE_2D_ARRAY ||
   resource->target == PIPE_TEXTURE_CUBE ||
   resource->target == PIPE_TEXTURE_CUBE_ARRAY) {
   key->images[i].is_single_layer =
                        }
   else
                              /* Save info about which shader buffers are to be viewed
   * as srv raw buffers in the shader key.
   */
   if (shader->info.shader_buffers_declared &
      svga->state.raw_shaderbufs[shader_type]) {
   key->raw_shaderbufs = svga->state.raw_shaderbufs[shader_type] &
         key->srv_raw_shaderbuf_index = key->srv_raw_constbuf_index +
                                 if (cur_sbuf->resource && (!(key->raw_shaderbufs & (1 << i))))
         else
                                          if (cur_buf->resource)
         else
                                 key->clamp_vertex_color = svga->curr.rast ?
      }
         /** Search for a compiled shader variant with the same compile key */
   struct svga_shader_variant *
   svga_search_shader_key(const struct svga_shader *shader,
         {
                        for ( ; variant; variant = variant->next) {
      if (svga_compile_keys_equal(key, &variant->key))
      }
      }
      /** Search for a shader with the same token key */
   struct svga_shader *
   svga_search_shader_token_key(struct svga_shader *pshader,
         {
                        for ( ; shader; shader = shader->next) {
      if (memcmp(key, &shader->token_key, sizeof(struct svga_token_key)) == 0)
      }
      }
      /**
   * Helper function to define a gb shader for non-vgpu10 device
   */
   static enum pipe_error
   define_gb_shader_vgpu9(struct svga_context *svga,
               {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
            /**
   * Create gb memory for the shader and upload the shader code.
   * Kernel module will allocate an id for the shader and issue
   * the DefineGBShader command.
   */
   variant->gb_shader = sws->shader_create(sws, variant->type,
                     if (!variant->gb_shader)
                        }
      /**
   * Helper function to define a gb shader for vgpu10 device
   */
   static enum pipe_error
   define_gb_shader_vgpu10(struct svga_context *svga,
               {
      struct svga_winsys_context *swc = svga->swc;
   enum pipe_error ret;
            /**
   * Shaders in VGPU10 enabled device reside in the device COTable.
   * SVGA driver will allocate an integer ID for the shader and
   * issue DXDefineShader and DXBindShader commands.
   */
   variant->id = util_bitmask_add(svga->shader_id_bm);
   if (variant->id == UTIL_BITMASK_INVALID_INDEX) {
                  /* Create gb memory for the shader and upload the shader code */
   variant->gb_shader = swc->shader_create(swc,
                                       if (!variant->gb_shader) {
      /* Free the shader ID */
   assert(variant->id != UTIL_BITMASK_INVALID_INDEX);
               /**
   * Since we don't want to do any flush within state emission to avoid
   * partial state in a command buffer, it's important to make sure that
   * there is enough room to send both the DXDefineShader & DXBindShader
   * commands in the same command buffer. So let's send both
   * commands in one command reservation. If it fails, we'll undo
   * the shader creation and return an error.
   */
   ret = SVGA3D_vgpu10_DefineAndBindShader(swc, variant->gb_shader,
                  if (ret != PIPE_OK)
                  fail:
      swc->shader_destroy(swc, variant->gb_shader);
         fail_no_allocation:
      util_bitmask_clear(svga->shader_id_bm, variant->id);
               }
      /**
   * Issue the SVGA3D commands to define a new shader.
   * \param variant  contains the shader tokens, etc.  The result->id field will
   *                 be set here.
   */
   enum pipe_error
   svga_define_shader(struct svga_context *svga,
         {
      unsigned codeLen = variant->nr_tokens * sizeof(variant->tokens[0]);
                              if (svga_have_gb_objects(svga)) {
      if (svga_have_vgpu10(svga))
         else
      }
   else {
      /* Allocate an integer ID for the shader */
   variant->id = util_bitmask_add(svga->shader_id_bm);
   if (variant->id == UTIL_BITMASK_INVALID_INDEX) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
               /* Issue SVGA3D device command to define the shader */
   ret = SVGA3D_DefineShader(svga->swc,
                           if (ret != PIPE_OK) {
      /* free the ID */
   assert(variant->id != UTIL_BITMASK_INVALID_INDEX);
   util_bitmask_clear(svga->shader_id_bm, variant->id);
               done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         /**
   * Issue the SVGA3D commands to set/bind a shader.
   * \param result  the shader to bind.
   */
   enum pipe_error
   svga_set_shader(struct svga_context *svga,
               {
      enum pipe_error ret;
            assert(type == SVGA3D_SHADERTYPE_VS ||
         type == SVGA3D_SHADERTYPE_GS ||
   type == SVGA3D_SHADERTYPE_PS ||
   type == SVGA3D_SHADERTYPE_HS ||
            if (svga_have_gb_objects(svga)) {
      struct svga_winsys_gb_shader *gbshader =
            if (svga_have_vgpu10(svga))
         else
      }
   else {
                     }
         struct svga_shader_variant *
   svga_new_shader_variant(struct svga_context *svga, enum pipe_shader_type type)
   {
               switch (type) {
   case PIPE_SHADER_FRAGMENT:
      variant = CALLOC(1, sizeof(struct svga_fs_variant));
      case PIPE_SHADER_GEOMETRY:
      variant = CALLOC(1, sizeof(struct svga_gs_variant));
      case PIPE_SHADER_VERTEX:
      variant = CALLOC(1, sizeof(struct svga_vs_variant));
      case PIPE_SHADER_TESS_EVAL:
      variant = CALLOC(1, sizeof(struct svga_tes_variant));
      case PIPE_SHADER_TESS_CTRL:
      variant = CALLOC(1, sizeof(struct svga_tcs_variant));
      case PIPE_SHADER_COMPUTE:
      variant = CALLOC(1, sizeof(struct svga_cs_variant));
      default:
                  if (variant) {
      variant->type = svga_shader_type(type);
      }
      }
         void
   svga_destroy_shader_variant(struct svga_context *svga,
         {
      if (svga_have_gb_objects(svga) && variant->gb_shader) {
      if (svga_have_vgpu10(svga)) {
      struct svga_winsys_context *swc = svga->swc;
   swc->shader_destroy(swc, variant->gb_shader);
   SVGA_RETRY(svga, SVGA3D_vgpu10_DestroyShader(svga->swc, variant->id));
      }
   else {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
      }
      }
   else {
      if (variant->id != UTIL_BITMASK_INVALID_INDEX) {
      SVGA_RETRY(svga, SVGA3D_DestroyShader(svga->swc, variant->id,
                        FREE(variant->signature);
   FREE((unsigned *)variant->tokens);
               }
      /*
   * Rebind shaders.
   * Called at the beginning of every new command buffer to ensure that
   * shaders are properly paged-in. Instead of sending the SetShader
   * command, this function sends a private allocation command to
   * page in a shader. This avoids emitting redundant state to the device
   * just to page in a resource.
   */
   enum pipe_error
   svga_rebind_shaders(struct svga_context *svga)
   {
      struct svga_winsys_context *swc = svga->swc;
   struct svga_hw_draw_state *hw = &svga->state.hw_draw;
                     /**
   * If the underlying winsys layer does not need resource rebinding,
   * just clear the rebind flags and return.
   */
   if (swc->resource_rebind == NULL) {
      svga->rebind.flags.vs = 0;
   svga->rebind.flags.gs = 0;
   svga->rebind.flags.fs = 0;
   svga->rebind.flags.tcs = 0;
                        if (svga->rebind.flags.vs && hw->vs && hw->vs->gb_shader) {
      ret = swc->resource_rebind(swc, NULL, hw->vs->gb_shader, SVGA_RELOC_READ);
   if (ret != PIPE_OK)
      }
            if (svga->rebind.flags.gs && hw->gs && hw->gs->gb_shader) {
      ret = swc->resource_rebind(swc, NULL, hw->gs->gb_shader, SVGA_RELOC_READ);
   if (ret != PIPE_OK)
      }
            if (svga->rebind.flags.fs && hw->fs && hw->fs->gb_shader) {
      ret = swc->resource_rebind(swc, NULL, hw->fs->gb_shader, SVGA_RELOC_READ);
   if (ret != PIPE_OK)
      }
            if (svga->rebind.flags.tcs && hw->tcs && hw->tcs->gb_shader) {
      ret = swc->resource_rebind(swc, NULL, hw->tcs->gb_shader, SVGA_RELOC_READ);
   if (ret != PIPE_OK)
      }
            if (svga->rebind.flags.tes && hw->tes && hw->tes->gb_shader) {
      ret = swc->resource_rebind(swc, NULL, hw->tes->gb_shader, SVGA_RELOC_READ);
   if (ret != PIPE_OK)
      }
               }
         /**
   * Helper function to create a shader object.
   */
   struct svga_shader *
   svga_create_shader(struct pipe_context *pipe,
                     {
      struct svga_context *svga = svga_context(pipe);
   struct svga_shader *shader = CALLOC(1, shader_structlen);
            if (shader == NULL)
            shader->id = svga->debug.shader_id++;
            if (templ->type == PIPE_SHADER_IR_NIR) {
      /* nir_to_tgsi requires lowered images */
      }
   shader->tokens = pipe_shader_state_to_tgsi_tokens(pipe->screen, templ);
            /* Collect basic info of the shader */
            /* check for any stream output declarations */
   if (templ->stream_output.num_outputs) {
      shader->stream_output = svga_create_stream_output(svga, shader,
                  }
         /**
   * Helper function to compile a shader.
   * Depending on the shader IR type, it calls the corresponding
   * compile shader function.
   */
   enum pipe_error
   svga_compile_shader(struct svga_context *svga,
                     {
      struct svga_shader_variant *variant = NULL;
            if (shader->type == PIPE_SHADER_IR_TGSI) {
         } else {
      debug_printf("Unexpected nir shader\n");
               if (variant == NULL) {
      if (shader->get_dummy_shader != NULL) {
      debug_printf("Failed to compile shader, using dummy shader.\n");
         }
   else if (svga_shader_too_large(svga, variant)) {
      /* too big, use shader */
   if (shader->get_dummy_shader != NULL) {
      debug_printf("Shader too large (%u bytes), using dummy shader.\n",
                                 /* Use simple pass-through shader instead */
                  if (variant == NULL)
            ret = svga_define_shader(svga, variant);
   if (ret != PIPE_OK) {
      svga_destroy_shader_variant(svga, variant);
                        /* insert variant at head of linked list */
   variant->next = shader->variants;
               }
