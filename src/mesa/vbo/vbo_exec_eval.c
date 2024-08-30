   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
   *
   * Authors:
   *    Keith Whitwell <keithw@vmware.com>
   */
      #include "util/glheader.h"
   #include "main/context.h"
   #include "main/macros.h"
   #include "math/m_eval.h"
   #include "main/dispatch.h"
   #include "vbo_exec.h"
   #include "vbo_private.h"
         static void clear_active_eval1( struct vbo_exec_context *exec, GLuint attr ) 
   {
      assert(attr < ARRAY_SIZE(exec->eval.map1));
      }
      static void clear_active_eval2( struct vbo_exec_context *exec, GLuint attr ) 
   {
      assert(attr < ARRAY_SIZE(exec->eval.map2));
      }
      static void set_active_eval1( struct vbo_exec_context *exec, GLuint attr, GLuint dim, 
         {
      assert(attr < ARRAY_SIZE(exec->eval.map1));
   if (!exec->eval.map1[attr].map) {
      exec->eval.map1[attr].map = map;
         } 
      static void set_active_eval2( struct vbo_exec_context *exec, GLuint attr, GLuint dim, 
         {
      assert(attr < ARRAY_SIZE(exec->eval.map2));
   if (!exec->eval.map2[attr].map) {
      exec->eval.map2[attr].map = map;
         } 
      void vbo_exec_eval_update( struct vbo_exec_context *exec )
   {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
                     for (attr = 0; attr < VBO_ATTRIB_FIRST_MATERIAL; attr++) {
      clear_active_eval1( exec, attr );
               if (ctx->Eval.Map1Color4) 
      set_active_eval1( exec, VBO_ATTRIB_COLOR0, 4, &ctx->EvalMap.Map1Color4 );
      if (ctx->Eval.Map2Color4) 
            if (ctx->Eval.Map1TextureCoord4) 
         else if (ctx->Eval.Map1TextureCoord3) 
         else if (ctx->Eval.Map1TextureCoord2) 
         else if (ctx->Eval.Map1TextureCoord1) 
            if (ctx->Eval.Map2TextureCoord4) 
         else if (ctx->Eval.Map2TextureCoord3) 
         else if (ctx->Eval.Map2TextureCoord2) 
         else if (ctx->Eval.Map2TextureCoord1) 
            if (ctx->Eval.Map1Normal) 
            if (ctx->Eval.Map2Normal) 
            if (ctx->Eval.Map1Vertex4) 
         else if (ctx->Eval.Map1Vertex3) 
            if (ctx->Eval.Map2Vertex4) 
         else if (ctx->Eval.Map2Vertex3) 
               }
            void vbo_exec_do_EvalCoord1f(struct vbo_exec_context *exec, GLfloat u)
   {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
            for (attr = 1; attr <= VBO_ATTRIB_TEX7; attr++) {
      struct gl_1d_map *map = exec->eval.map1[attr].map;
   GLfloat uu = (u - map->u1) * map->du;
   fi_type data[4];
      ASSIGN_4V(data, FLOAT_AS_UNION(0), FLOAT_AS_UNION(0),
            _math_horner_bezier_curve(map->Points, &data[0].f, uu,
         exec->eval.map1[attr].sz, 
      COPY_SZ_4V( exec->vtx.attrptr[attr],
         exec->vtx.attr[attr].size,
   data );
               /** Vertex -- EvalCoord1f is a noop if this map not enabled:
   **/
   if (exec->eval.map1[0].map) {
      struct gl_1d_map *map = exec->eval.map1[0].map;
   GLfloat uu = (u - map->u1) * map->du;
                        exec->eval.map1[0].sz, 
               CALL_Vertex4fv(ctx->Dispatch.Current, ( vertex ));
         CALL_Vertex3fv(ctx->Dispatch.Current, ( vertex ));
         }
            void vbo_exec_do_EvalCoord2f( struct vbo_exec_context *exec, 
         {   
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
            for (attr = 1; attr <= VBO_ATTRIB_TEX7; attr++) {
      struct gl_2d_map *map = exec->eval.map2[attr].map;
   GLfloat uu = (u - map->u1) * map->du;
   GLfloat vv = (v - map->v1) * map->dv;
   fi_type data[4];
      ASSIGN_4V(data, FLOAT_AS_UNION(0), FLOAT_AS_UNION(0),
            _math_horner_bezier_surf(map->Points, 
         &data[0].f,
   uu, vv, 
   exec->eval.map2[attr].sz, 
      COPY_SZ_4V( exec->vtx.attrptr[attr],
         exec->vtx.attr[attr].size,
   data );
               /** Vertex -- EvalCoord2f is a noop if this map not enabled:
   **/
   if (exec->eval.map2[0].map) {
      struct gl_2d_map *map = exec->eval.map2[0].map;
   GLfloat uu = (u - map->u1) * map->du;
   GLfloat vv = (v - map->v1) * map->dv;
                     fi_type normal[4];
                     exec->eval.map2[0].sz,
         if (exec->eval.map2[0].sz == 4) {
      du[0] = du[0]*vertex[3] - du[3]*vertex[0];
   du[1] = du[1]*vertex[3] - du[3]*vertex[1];
   du[2] = du[2]*vertex[3] - du[3]*vertex[2];
      dv[0] = dv[0]*vertex[3] - dv[3]*vertex[0];
   dv[1] = dv[1]*vertex[3] - dv[3]*vertex[1];
      }
                     normal[3] = FLOAT_AS_UNION(1.0);
         COPY_SZ_4V( exec->vtx.attrptr[VBO_ATTRIB_NORMAL],
      exec->vtx.attr[VBO_ATTRIB_NORMAL].size,
            }
   else {
         exec->eval.map2[0].sz,
   map->Uorder, map->Vorder);
            CALL_Vertex4fv(ctx->Dispatch.Current, ( vertex ));
         CALL_Vertex3fv(ctx->Dispatch.Current, ( vertex ));
         }
      