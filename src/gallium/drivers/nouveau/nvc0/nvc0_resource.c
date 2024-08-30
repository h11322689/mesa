   #include "drm-uapi/drm_fourcc.h"
      #include "pipe/p_context.h"
   #include "nvc0/nvc0_resource.h"
   #include "nouveau_screen.h"
         static struct pipe_resource *
   nvc0_resource_create(struct pipe_screen *screen,
         {
      switch (templ->target) {
   case PIPE_BUFFER:
         default:
            }
      static struct pipe_resource *
   nvc0_resource_create_with_modifiers(struct pipe_screen *screen,
               {
      switch (templ->target) {
   case PIPE_BUFFER:
         default:
            }
      static void
   nvc0_resource_destroy(struct pipe_screen *pscreen, struct pipe_resource *res)
   {
      if (res->target == PIPE_BUFFER)
         else
      }
      static void
   nvc0_query_dmabuf_modifiers(struct pipe_screen *screen,
                     {
      const int s = nouveau_screen(screen)->tegra_sector_layout ? 0 : 1;
   const uint32_t uc_kind =
         const uint32_t num_uc = uc_kind ? 6 : 0; /* max block height = 32 GOBs */
   const int num_supported = num_uc + 1; /* LINEAR is always supported */
   const uint32_t kind_gen = nvc0_get_kind_generation(screen);
            if (max > num_supported)
            if (!max) {
      max = num_supported;
   external_only = NULL;
            #define NVC0_ADD_MOD(m) do { \
      if (modifiers) modifiers[num] = m; \
   if (external_only) external_only[num] = 0; \
      } while (0)
         for (i = 0; i < max && i < num_uc; i++)
      NVC0_ADD_MOD(DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, s, kind_gen,
         if (i < max)
         #undef NVC0_ADD_MOD
            }
      static bool
   nvc0_is_dmabuf_modifier_supported(struct pipe_screen *screen,
               {
      const int s = nouveau_screen(screen)->tegra_sector_layout ? 0 : 1;
   const uint32_t uc_kind =
         const uint32_t num_uc = uc_kind ? 6 : 0; /* max block height = 32 GOBs */
   const uint32_t kind_gen = nvc0_get_kind_generation(screen);
            if (modifier == DRM_FORMAT_MOD_LINEAR) {
      if (external_only)
                        for (i = 0; i < num_uc; i++) {
      if (DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, s, kind_gen, uc_kind, i) == modifier) {
                                       }
      static struct pipe_resource *
   nvc0_resource_from_handle(struct pipe_screen * screen,
                     {
      if (templ->target == PIPE_BUFFER) {
         } else {
      struct pipe_resource *res = nv50_miptree_from_handle(screen,
               }
      static struct pipe_surface *
   nvc0_surface_create(struct pipe_context *pipe,
               {
      if (unlikely(pres->target == PIPE_BUFFER))
            }
      static struct pipe_resource *
   nvc0_resource_from_user_memory(struct pipe_screen *pipe,
               {
               assert(screen->has_svm);
               }
      void
   nvc0_init_resource_functions(struct pipe_context *pcontext)
   {
      pcontext->buffer_map = nouveau_buffer_transfer_map;
   pcontext->texture_map = nvc0_miptree_transfer_map;
   pcontext->transfer_flush_region = nouveau_buffer_transfer_flush_region;
   pcontext->buffer_unmap = nouveau_buffer_transfer_unmap;
   pcontext->texture_unmap = nvc0_miptree_transfer_unmap;
   pcontext->buffer_subdata = u_default_buffer_subdata;
   pcontext->texture_subdata = u_default_texture_subdata;
   pcontext->create_surface = nvc0_surface_create;
   pcontext->surface_destroy = nv50_surface_destroy;
      }
      void
   nvc0_screen_init_resource_functions(struct pipe_screen *pscreen)
   {
      pscreen->resource_create = nvc0_resource_create;
   pscreen->resource_create_with_modifiers = nvc0_resource_create_with_modifiers;
   pscreen->query_dmabuf_modifiers = nvc0_query_dmabuf_modifiers;
   pscreen->is_dmabuf_modifier_supported = nvc0_is_dmabuf_modifier_supported;
   pscreen->resource_from_handle = nvc0_resource_from_handle;
   pscreen->resource_get_handle = nvc0_miptree_get_handle;
   pscreen->resource_destroy = nvc0_resource_destroy;
            pscreen->memobj_create_from_handle = nv50_memobj_create_from_handle;
   pscreen->resource_from_memobj = nv50_resource_from_memobj;
      }
