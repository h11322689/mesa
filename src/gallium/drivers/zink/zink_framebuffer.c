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
      #include "zink_context.h"
   #include "zink_framebuffer.h"
      #include "zink_render_pass.h"
   #include "zink_screen.h"
   #include "zink_surface.h"
      #include "util/u_framebuffer.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      void
   zink_destroy_framebuffer(struct zink_screen *screen,
         {
         #if VK_USE_64_BIT_PTR_DEFINES
         #else
         VkFramebuffer *ptr = he->data;
   #endif
                  }
      void
   zink_init_framebuffer(struct zink_screen *screen, struct zink_framebuffer *fb, struct zink_render_pass *rp)
   {
               if (fb->rp == rp)
                     struct hash_entry *he = _mesa_hash_table_search_pre_hashed(&fb->objects, hash, rp);
      #if VK_USE_64_BIT_PTR_DEFINES
         #else
         VkFramebuffer *ptr = he->data;
   #endif
                              VkFramebufferCreateInfo fci;
   fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   fci.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
   fci.renderPass = rp->render_pass;
   fci.attachmentCount = fb->state.num_attachments;
   fci.pAttachments = NULL;
   fci.width = fb->state.width;
   fci.height = fb->state.height;
            VkFramebufferAttachmentsCreateInfo attachments;
   attachments.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
   attachments.pNext = NULL;
   attachments.attachmentImageInfoCount = fb->state.num_attachments;
   attachments.pAttachmentImageInfos = fb->infos;
            if (VKSCR(CreateFramebuffer)(screen->dev, &fci, NULL, &ret) != VK_SUCCESS)
      #if VK_USE_64_BIT_PTR_DEFINES
         #else
      VkFramebuffer *ptr = ralloc(fb, VkFramebuffer);
   if (!ptr) {
      VKSCR(DestroyFramebuffer)(screen->dev, ret, NULL);
      }
   *ptr = ret;
      #endif
   out:
      fb->rp = rp;
      }
      static void
   populate_attachment_info(VkFramebufferAttachmentImageInfo *att, struct zink_surface_info *info)
   {
      att->sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
   att->pNext = NULL;
   memcpy(&att->flags, &info->flags, offsetof(struct zink_surface_info, format));
   att->viewFormatCount = 1 + !!info->format[1];
      }
      static struct zink_framebuffer *
   create_framebuffer_imageless(struct zink_context *ctx, struct zink_framebuffer_state *state)
   {
      struct zink_screen *screen = zink_screen(ctx->base.screen);
   struct zink_framebuffer *fb = rzalloc(ctx, struct zink_framebuffer);
   if (!fb)
                  if (!_mesa_hash_table_init(&fb->objects, fb, _mesa_hash_pointer, _mesa_key_pointer_equal))
         memcpy(&fb->state, state, sizeof(struct zink_framebuffer_state));
   for (int i = 0; i < state->num_attachments; i++)
               fail:
      zink_destroy_framebuffer(screen, fb);
      }
      struct zink_framebuffer *
   zink_get_framebuffer(struct zink_context *ctx)
   {
      assert(zink_screen(ctx->base.screen)->info.have_KHR_imageless_framebuffer);
            struct zink_framebuffer_state state;
            const unsigned cresolve_offset = ctx->fb_state.nr_cbufs + !!have_zsbuf;
   unsigned num_resolves = 0;
   for (int i = 0; i < ctx->fb_state.nr_cbufs; i++) {
      struct pipe_surface *psurf = ctx->fb_state.cbufs[i];
   if (!psurf) {
         }
   struct zink_surface *surface = zink_csurface(psurf);
   struct zink_surface *transient = zink_transient_surface(psurf);
   if (transient) {
      memcpy(&state.infos[i], &transient->info, sizeof(transient->info));
   memcpy(&state.infos[cresolve_offset + i], &surface->info, sizeof(surface->info));
      } else {
                     const unsigned zsresolve_offset = cresolve_offset + num_resolves;
   if (have_zsbuf) {
      struct pipe_surface *psurf = ctx->fb_state.zsbuf;
   struct zink_surface *surface = zink_csurface(psurf);
   struct zink_surface *transient = zink_transient_surface(psurf);
   if (transient) {
      memcpy(&state.infos[state.num_attachments], &transient->info, sizeof(transient->info));
   memcpy(&state.infos[zsresolve_offset], &surface->info, sizeof(surface->info));
      } else {
         }
               /* avoid bitfield explosion */
   assert(state.num_attachments + num_resolves < 16);
   state.num_attachments += num_resolves;
   state.width = MAX2(ctx->fb_state.width, 1);
   state.height = MAX2(ctx->fb_state.height, 1);
   state.layers = MAX2(zink_framebuffer_get_num_layers(&ctx->fb_state), 1) - 1;
            struct zink_framebuffer *fb;
   struct hash_entry *entry = _mesa_hash_table_search(&ctx->framebuffer_cache, &state);
   if (entry)
            fb = create_framebuffer_imageless(ctx, &state);
               }
      void
   debug_describe_zink_framebuffer(char* buf, const struct zink_framebuffer *ptr)
   {
         }
      void
   zink_update_framebuffer_state(struct zink_context *ctx)
   {
      /* get_framebuffer adds a ref if the fb is reused or created;
   * always do get_framebuffer first to avoid deleting the same fb
   * we're about to use
   */
   struct zink_framebuffer *fb = zink_get_framebuffer(ctx);
   ctx->fb_changed |= ctx->framebuffer != fb;
      }
      /* same as u_framebuffer_get_num_layers, but clamp to lowest layer count */
   unsigned
   zink_framebuffer_get_num_layers(const struct pipe_framebuffer_state *fb)
   {
      unsigned i, num_layers = UINT32_MAX;
   if (!(fb->nr_cbufs || fb->zsbuf))
            for (i = 0; i < fb->nr_cbufs; i++) {
      if (fb->cbufs[i]) {
      unsigned num = fb->cbufs[i]->u.tex.last_layer -
   fb->cbufs[i]->u.tex.first_layer + 1;
         }
   if (fb->zsbuf) {
      unsigned num = fb->zsbuf->u.tex.last_layer -
   fb->zsbuf->u.tex.first_layer + 1;
      }
      }
