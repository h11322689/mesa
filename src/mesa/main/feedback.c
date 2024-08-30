   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file feedback.c
   * Selection and feedback modes functions.
   */
         #include "util/glheader.h"
   #include "c99_alloca.h"
   #include "context.h"
   #include "enums.h"
   #include "feedback.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "api_exec_decl.h"
   #include "bufferobj.h"
      #include "state_tracker/st_cb_feedback.h"
      #define FB_3D		0x01
   #define FB_4D		0x02
   #define FB_COLOR	0x04
   #define FB_TEXTURE	0X08
            void GLAPIENTRY
   _mesa_FeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
   {
               if (ctx->RenderMode==GL_FEEDBACK) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glFeedbackBuffer" );
      }
   if (size<0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(size<0)" );
      }
   if (!buffer && size > 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(buffer==NULL)" );
   ctx->Feedback.BufferSize = 0;
               switch (type) {
      ctx->Feedback._Mask = 0;
   break;
         ctx->Feedback._Mask = FB_3D;
   break;
         ctx->Feedback._Mask = (FB_3D | FB_COLOR);
   break;
         ctx->Feedback._Mask = (FB_3D | FB_COLOR | FB_TEXTURE);
   break;
         ctx->Feedback._Mask = (FB_3D | FB_4D | FB_COLOR | FB_TEXTURE);
   break;
         default:
   return;
               FLUSH_VERTICES(ctx, _NEW_RENDERMODE, 0); /* Always flush */
   ctx->Feedback.Type = type;
   ctx->Feedback.BufferSize = size;
   ctx->Feedback.Buffer = buffer;
      }
         void GLAPIENTRY
   _mesa_PassThrough( GLfloat token )
   {
               if (ctx->RenderMode==GL_FEEDBACK) {
      FLUSH_VERTICES(ctx, 0, 0);
   _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_PASS_THROUGH_TOKEN );
         }
         /**
   * Put a vertex into the feedback buffer.
   */
   void
   _mesa_feedback_vertex(struct gl_context *ctx,
                     {
      _mesa_feedback_token( ctx, win[0] );
   _mesa_feedback_token( ctx, win[1] );
   if (ctx->Feedback._Mask & FB_3D) {
         }
   if (ctx->Feedback._Mask & FB_4D) {
         }
   if (ctx->Feedback._Mask & FB_COLOR) {
      _mesa_feedback_token( ctx, color[0] );
   _mesa_feedback_token( ctx, color[1] );
   _mesa_feedback_token( ctx, color[2] );
      }
   if (ctx->Feedback._Mask & FB_TEXTURE) {
      _mesa_feedback_token( ctx, texcoord[0] );
   _mesa_feedback_token( ctx, texcoord[1] );
   _mesa_feedback_token( ctx, texcoord[2] );
         }
         /**********************************************************************/
   /** \name Selection */
   /*@{*/
      /**
   * Establish a buffer for selection mode values.
   * 
   * \param size buffer size.
   * \param buffer buffer.
   *
   * \sa glSelectBuffer().
   * 
   * \note this function can't be put in a display list.
   * 
   * Verifies we're not in selection mode, flushes the vertices and initialize
   * the fields in __struct gl_contextRec::Select with the given buffer.
   */
   void GLAPIENTRY
   _mesa_SelectBuffer( GLsizei size, GLuint *buffer )
   {
               if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glSelectBuffer(size)");
               if (ctx->RenderMode==GL_SELECT) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glSelectBuffer" );
               FLUSH_VERTICES(ctx, _NEW_RENDERMODE, 0);
   ctx->Select.Buffer = buffer;
   ctx->Select.BufferSize = size;
   ctx->Select.BufferCount = 0;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
      }
         /**
   * Write a value of a record into the selection buffer.
   * 
   * \param ctx GL context.
   * \param value value.
   *
   * Verifies there is free space in the buffer to write the value and
   * increments the pointer.
   */
   static inline void
   write_record(struct gl_context *ctx, GLuint value)
   {
      if (ctx->Select.BufferCount < ctx->Select.BufferSize) {
         }
      }
         /**
   * Update the hit flag and the maximum and minimum depth values.
   *
   * \param ctx GL context.
   * \param z depth.
   *
   * Sets gl_selection::HitFlag and updates gl_selection::HitMinZ and
   * gl_selection::HitMaxZ.
   */
   void
   _mesa_update_hitflag(struct gl_context *ctx, GLfloat z)
   {
      ctx->Select.HitFlag = GL_TRUE;
   if (z < ctx->Select.HitMinZ) {
         }
   if (z > ctx->Select.HitMaxZ) {
            }
      static void
   alloc_select_resource(struct gl_context *ctx)
   {
               if (!ctx->Const.HardwareAcceleratedSelect)
            if (!ctx->Dispatch.HWSelectModeBeginEnd) {
      ctx->Dispatch.HWSelectModeBeginEnd = _mesa_alloc_dispatch_table(false);
   if (!ctx->Dispatch.HWSelectModeBeginEnd) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Cannot allocate HWSelectModeBeginEnd");
      }
               if (!s->SaveBuffer) {
      s->SaveBuffer = malloc(NAME_STACK_BUFFER_SIZE);
   if (!s->SaveBuffer) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Cannot allocate name stack save buffer");
                  if (!s->Result) {
      s->Result = _mesa_bufferobj_alloc(ctx, -1);
   if (!s->Result) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Cannot allocate select result buffer");
               GLuint init_result[MAX_NAME_STACK_RESULT_NUM * 3];
   for (int i = 0; i < MAX_NAME_STACK_RESULT_NUM; i++) {
      init_result[i * 3] = 0;              /* hit */
   init_result[i * 3 + 1] = 0xffffffff; /* minz */
               bool success = _mesa_bufferobj_data(ctx,
                                 if (!success) {
      _mesa_reference_buffer_object(ctx, &s->Result, NULL);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "Cannot init result buffer");
            }
      static bool
   save_used_name_stack(struct gl_context *ctx)
   {
               if (!ctx->Const.HardwareAcceleratedSelect)
            /* We have two kinds of name stack user:
   *   1. glRasterPos (CPU based) will set HitFlag
   *   2. draw call for GPU will set ResultUsed
   */
   if (!s->ResultUsed && !s->HitFlag)
                     /* save meta data */
   uint8_t *metadata = save;
   metadata[0] = s->HitFlag;
   metadata[1] = s->ResultUsed;
   metadata[2] = s->NameStackDepth;
            /* save hit data */
   int index = 1;
   if (s->HitFlag) {
      float *hit = save;
   hit[index++] = s->HitMinZ;
               /* save name stack */
   memcpy((uint32_t *)save + index, s->NameStack, s->NameStackDepth * sizeof(GLuint));
            s->SaveBufferTail += index * sizeof(GLuint);
            /* if current slot has been used, store result to next slot in result buffer */
   if (s->ResultUsed)
            /* reset fields */
   s->HitFlag = GL_FALSE;
   s->HitMinZ = 1.0;
                     /* return true if we have no enough space for the next name stack data */
   return s->ResultOffset >= MAX_NAME_STACK_RESULT_NUM * 3 * sizeof(GLuint) ||
      }
      static void
   update_hit_record(struct gl_context *ctx)
   {
               if (ctx->Const.HardwareAcceleratedSelect) {
      if (!s->SavedStackNum)
            unsigned size = s->ResultOffset;
   GLuint *result = size ? alloca(size) : NULL;
            unsigned index = 0;
   unsigned *save = s->SaveBuffer;
   for (int i = 0; i < s->SavedStackNum; i++) {
               unsigned zmin, zmax;
   bool cpu_hit = !!metadata[0];
   if (cpu_hit) {
      /* map [0, 1] to [0, UINT_MAX]*/
   zmin = (unsigned) ((float)(~0u) * *(float *)(save++));
      } else {
      zmin = ~0u;
               bool gpu_hit = false;
                     if (gpu_hit) {
                     /* reset data */
   result[index]     = 0;          /* hit */
   result[index + 1] = 0xffffffff; /* minz */
      }
               int depth = metadata[2];
   if (cpu_hit || gpu_hit) {
      /* hit */
   write_record(ctx, depth);
                  for (int j = 0; j < depth; j++)
            }
               /* reset result buffer */
            s->SaveBufferTail = 0;
   s->SavedStackNum = 0;
      } else {
      if (!s->HitFlag)
            /* HitMinZ and HitMaxZ are in [0,1].  Multiply these values by */
   /* 2^32-1 and round to nearest unsigned integer. */
   GLuint zscale = (~0u);
   GLuint zmin = (GLuint) ((GLfloat) zscale * s->HitMinZ);
            write_record(ctx, s->NameStackDepth);
   write_record(ctx, zmin);
   write_record(ctx, zmax);
   for (int i = 0; i < s->NameStackDepth; i++)
            s->HitFlag = GL_FALSE;
   s->HitMinZ = 1.0;
                  }
      static void
   reset_name_stack_to_empty(struct gl_context *ctx)
   {
               s->NameStackDepth = 0;
   s->HitFlag = GL_FALSE;
   s->HitMinZ = 1.0;
            if (ctx->Const.HardwareAcceleratedSelect) {
      s->SaveBufferTail = 0;
   s->SavedStackNum = 0;
   s->ResultUsed = GL_FALSE;
         }
      /**
   * Initialize the name stack.
   */
   void GLAPIENTRY
   _mesa_InitNames( void )
   {
               /* Calls to glInitNames while the render mode is not GL_SELECT are ignored. */
   if (ctx->RenderMode != GL_SELECT)
                     save_used_name_stack(ctx);
                        }
         /**
   * Load the top-most name of the name stack.
   *
   * \param name name.
   */
   void GLAPIENTRY
   _mesa_LoadName( GLuint name )
   {
               if (ctx->RenderMode != GL_SELECT) {
         }
   if (ctx->Select.NameStackDepth == 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glLoadName" );
               if (!ctx->Const.HardwareAcceleratedSelect || save_used_name_stack(ctx)) {
      FLUSH_VERTICES(ctx, 0, 0);
               ctx->Select.NameStack[ctx->Select.NameStackDepth-1] = name;
      }
         /**
   * Push a name into the name stack.
   *
   * \param name name.
   */
   void GLAPIENTRY
   _mesa_PushName( GLuint name )
   {
               if (ctx->RenderMode != GL_SELECT) {
                  if (ctx->Select.NameStackDepth >= MAX_NAME_STACK_DEPTH) {
      _mesa_error( ctx, GL_STACK_OVERFLOW, "glPushName" );
               if (!ctx->Const.HardwareAcceleratedSelect || save_used_name_stack(ctx)) {
      FLUSH_VERTICES(ctx, 0, 0);
               ctx->Select.NameStack[ctx->Select.NameStackDepth++] = name;
      }
         /**
   * Pop a name into the name stack.
   */
   void GLAPIENTRY
   _mesa_PopName( void )
   {
               if (ctx->RenderMode != GL_SELECT) {
                  if (ctx->Select.NameStackDepth == 0) {
      _mesa_error( ctx, GL_STACK_UNDERFLOW, "glPopName" );
               if (!ctx->Const.HardwareAcceleratedSelect || save_used_name_stack(ctx)) {
      FLUSH_VERTICES(ctx, 0, 0);
               ctx->Select.NameStackDepth--;
      }
      /*@}*/
         /**********************************************************************/
   /** \name Render Mode */
   /*@{*/
      /**
   * Set rasterization mode.
   *
   * \param mode rasterization mode.
   *
   * \note this function can't be put in a display list.
   *
   * \sa glRenderMode().
   * 
   * Flushes the vertices and do the necessary cleanup according to the previous
   * rasterization mode, such as writing the hit record or resent the select
   * buffer index when exiting the select mode. Updates
   * __struct gl_contextRec::RenderMode and notifies the driver via the
   * dd_function_table::RenderMode callback.
   */
   GLint GLAPIENTRY
   _mesa_RenderMode( GLenum mode )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLint result;
            if (MESA_VERBOSE & VERBOSE_API)
            FLUSH_VERTICES(ctx, _NEW_RENDERMODE | _NEW_FF_VERT_PROGRAM |
            switch (ctx->RenderMode) {
      result = 0;
   break;
         save_used_name_stack(ctx);
   update_hit_record(ctx);
      if (ctx->Select.BufferCount > ctx->Select.BufferSize) {
         #ifndef NDEBUG
         #endif
         }
   else {
         }
   ctx->Select.BufferCount = 0;
   ctx->Select.Hits = 0;
   /* name stack should be in empty state after exiting GL_SELECT mode */
   reset_name_stack_to_empty(ctx);
   break;
         if (ctx->Feedback.Count > ctx->Feedback.BufferSize) {
      /* overflow */
      }
   else {
         }
   ctx->Feedback.Count = 0;
   break;
         _mesa_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
   return 0;
               switch (mode) {
      case GL_RENDER:
         if (ctx->Select.BufferSize==0) {
      /* haven't called glSelectBuffer yet */
      }
   alloc_select_resource(ctx);
   break;
         if (ctx->Feedback.BufferSize==0) {
      /* haven't called glFeedbackBuffer yet */
      }
   break;
         _mesa_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
   return 0;
                        /* finally update render mode to new one */
               }
      /*@}*/
         /**********************************************************************/
   /** \name Initialization */
   /*@{*/
      /**
   * Initialize context feedback data.
   */
   void _mesa_init_feedback( struct gl_context * ctx )
   {
      /* Feedback */
   ctx->Feedback.Type = GL_2D;   /* TODO: verify */
   ctx->Feedback.Buffer = NULL;
   ctx->Feedback.BufferSize = 0;
            /* Selection/picking */
   ctx->Select.Buffer = NULL;
   ctx->Select.BufferSize = 0;
   ctx->Select.BufferCount = 0;
   ctx->Select.Hits = 0;
            /* Miscellaneous */
      }
      void _mesa_free_feedback(struct gl_context * ctx)
   {
               free(s->SaveBuffer);
      }
      /*@}*/
