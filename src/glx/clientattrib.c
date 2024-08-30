   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      #include <assert.h>
   #include "glxclient.h"
   #include "indirect.h"
   #include "indirect_vertex_array.h"
      /*****************************************************************************/
      #ifndef GLX_USE_APPLEGL
   static void
   do_enable_disable(GLenum array, GLboolean val)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
            if (array == GL_TEXTURE_COORD_ARRAY) {
                  if (!__glXSetArrayEnable(state, array, index, val)) {
            }
      void
   __indirect_glEnableClientState(GLenum array)
   {
         }
      void
   __indirect_glDisableClientState(GLenum array)
   {
         }
      /************************************************************************/
      void
   __indirect_glPushClientAttrib(GLuint mask)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
            if (spp < &gc->attributes.stack[__GL_CLIENT_ATTRIB_STACK_DEPTH]) {
      if (!(sp = *spp)) {
      sp = malloc(sizeof(__GLXattribute));
   if (sp == NULL) {
      __glXSetError(gc, GL_OUT_OF_MEMORY);
      }
      }
   sp->mask = mask;
   gc->attributes.stackPointer = spp + 1;
   if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      sp->storePack = state->storePack;
      }
   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
            }
   else {
      __glXSetError(gc, GL_STACK_OVERFLOW);
         }
      void
   __indirect_glPopClientAttrib(void)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
   __GLXattribute **spp = gc->attributes.stackPointer, *sp;
            if (spp > &gc->attributes.stack[0]) {
      --spp;
   sp = *spp;
   assert(sp != 0);
   mask = sp->mask;
            if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      state->storePack = sp->storePack;
      }
   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
                     }
   else {
      __glXSetError(gc, GL_STACK_UNDERFLOW);
         }
   #endif
