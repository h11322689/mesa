      #include "pipe/p_context.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
      #include "nouveau_screen.h"
      #include "nv50/nv50_resource.h"
      static struct pipe_resource *
   nv50_resource_create(struct pipe_screen *screen,
         {
      switch (templ->target) {
   case PIPE_BUFFER:
         default:
            }
      static void
   nv50_resource_destroy(struct pipe_screen *pscreen, struct pipe_resource *res)
   {
      if (res->target == PIPE_BUFFER)
         else
      }
      static struct pipe_resource *
   nv50_resource_from_handle(struct pipe_screen * screen,
                     {
      if (templ->target == PIPE_BUFFER)
         else
      }
      struct pipe_surface *
   nv50_surface_from_buffer(struct pipe_context *pipe,
               {
      struct nv50_surface *sf = CALLOC_STRUCT(nv50_surface);
   if (!sf)
            pipe_reference_init(&sf->base.reference, 1);
            sf->base.format = templ->format;
   sf->base.writable = templ->writable;
   sf->base.u.buf.first_element = templ->u.buf.first_element;
            sf->offset =
                     sf->width = templ->u.buf.last_element - templ->u.buf.first_element + 1;
   sf->height = 1;
            sf->base.width = sf->width;
            sf->base.context = pipe;
      }
      static struct pipe_surface *
   nv50_surface_create(struct pipe_context *pipe,
               {
      if (unlikely(pres->target == PIPE_BUFFER))
            }
      void
   nv50_surface_destroy(struct pipe_context *pipe, struct pipe_surface *ps)
   {
                           }
      void
   nv50_invalidate_resource(struct pipe_context *pipe, struct pipe_resource *res)
   {
      if (res->target == PIPE_BUFFER)
      }
      struct pipe_memory_object *
   nv50_memobj_create_from_handle(struct pipe_screen *screen,
               {
               memobj->bo = nouveau_screen_bo_from_handle(screen, handle, &memobj->stride);
   if (memobj->bo == NULL) {
      FREE(memobj);
      }
   memobj->handle = handle;
               }
      void
   nv50_memobj_destroy(struct pipe_screen *screen,
         {
               free(memobj->handle);
   free(memobj->bo);
      }
      struct pipe_resource *
   nv50_resource_from_memobj(struct pipe_screen *screen,
                     {
      struct nv50_miptree *mt;
            /* only supports 2D, non-mipmapped textures for the moment */
   if ((templ->target != PIPE_TEXTURE_2D &&
      templ->target != PIPE_TEXTURE_RECT) ||
   templ->last_level != 0 ||
   templ->depth0 != 1 ||
   templ->array_size > 1)
         mt = CALLOC_STRUCT(nv50_miptree);
   if (!mt)
                     mt->base.domain = mt->base.bo->flags & NOUVEAU_BO_APER;
            mt->base.base = *templ;
   pipe_reference_init(&mt->base.base.reference, 1);
   mt->base.base.screen = screen;
   mt->level[0].offset = 0;
                     /* no need to adjust bo reference count */
      }
      void
   nv50_init_resource_functions(struct pipe_context *pcontext)
   {
      pcontext->buffer_map = nouveau_buffer_transfer_map;
   pcontext->texture_map = nv50_miptree_transfer_map;
   pcontext->transfer_flush_region = nouveau_buffer_transfer_flush_region;
   pcontext->buffer_unmap = nouveau_buffer_transfer_unmap;
   pcontext->texture_unmap = nv50_miptree_transfer_unmap;
   pcontext->buffer_subdata = u_default_buffer_subdata;
   pcontext->texture_subdata = u_default_texture_subdata;
   pcontext->create_surface = nv50_surface_create;
   pcontext->surface_destroy = nv50_surface_destroy;
      }
      void
   nv50_screen_init_resource_functions(struct pipe_screen *pscreen)
   {
      pscreen->resource_create = nv50_resource_create;
   pscreen->resource_from_handle = nv50_resource_from_handle;
   pscreen->resource_get_handle = nv50_miptree_get_handle;
            pscreen->memobj_create_from_handle = nv50_memobj_create_from_handle;
   pscreen->resource_from_memobj = nv50_resource_from_memobj;
      }
