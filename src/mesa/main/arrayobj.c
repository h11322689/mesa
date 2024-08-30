   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * (C) Copyright IBM Corporation 2006
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
   * \file arrayobj.c
   *
   * Implementation of Vertex Array Objects (VAOs), from OpenGL 3.1+ /
   * the GL_ARB_vertex_array_object extension.
   *
   * \todo
   * The code in this file borrows a lot from bufferobj.c.  There's a certain
   * amount of cruft left over from that origin that may be unnecessary.
   *
   * \author Ian Romanick <idr@us.ibm.com>
   * \author Brian Paul
   */
         #include "util/glheader.h"
   #include "hash.h"
   #include "image.h"
      #include "context.h"
   #include "bufferobj.h"
   #include "arrayobj.h"
   #include "draw_validate.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "state.h"
   #include "varray.h"
   #include "util/bitscan.h"
   #include "util/u_atomic.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      const GLubyte
   _mesa_vao_attribute_map[ATTRIBUTE_MAP_MODE_MAX][VERT_ATTRIB_MAX] =
   {
      /* ATTRIBUTE_MAP_MODE_IDENTITY
   *
   * Grab vertex processing attribute VERT_ATTRIB_POS from
   * the VAO attribute VERT_ATTRIB_POS, and grab vertex processing
   * attribute VERT_ATTRIB_GENERIC0 from the VAO attribute
   * VERT_ATTRIB_GENERIC0.
   */
   {
      VERT_ATTRIB_POS,                 /* VERT_ATTRIB_POS */
   VERT_ATTRIB_NORMAL,              /* VERT_ATTRIB_NORMAL */
   VERT_ATTRIB_COLOR0,              /* VERT_ATTRIB_COLOR0 */
   VERT_ATTRIB_COLOR1,              /* VERT_ATTRIB_COLOR1 */
   VERT_ATTRIB_FOG,                 /* VERT_ATTRIB_FOG */
   VERT_ATTRIB_COLOR_INDEX,         /* VERT_ATTRIB_COLOR_INDEX */
   VERT_ATTRIB_TEX0,                /* VERT_ATTRIB_TEX0 */
   VERT_ATTRIB_TEX1,                /* VERT_ATTRIB_TEX1 */
   VERT_ATTRIB_TEX2,                /* VERT_ATTRIB_TEX2 */
   VERT_ATTRIB_TEX3,                /* VERT_ATTRIB_TEX3 */
   VERT_ATTRIB_TEX4,                /* VERT_ATTRIB_TEX4 */
   VERT_ATTRIB_TEX5,                /* VERT_ATTRIB_TEX5 */
   VERT_ATTRIB_TEX6,                /* VERT_ATTRIB_TEX6 */
   VERT_ATTRIB_TEX7,                /* VERT_ATTRIB_TEX7 */
   VERT_ATTRIB_POINT_SIZE,          /* VERT_ATTRIB_POINT_SIZE */
   VERT_ATTRIB_GENERIC0,            /* VERT_ATTRIB_GENERIC0 */
   VERT_ATTRIB_GENERIC1,            /* VERT_ATTRIB_GENERIC1 */
   VERT_ATTRIB_GENERIC2,            /* VERT_ATTRIB_GENERIC2 */
   VERT_ATTRIB_GENERIC3,            /* VERT_ATTRIB_GENERIC3 */
   VERT_ATTRIB_GENERIC4,            /* VERT_ATTRIB_GENERIC4 */
   VERT_ATTRIB_GENERIC5,            /* VERT_ATTRIB_GENERIC5 */
   VERT_ATTRIB_GENERIC6,            /* VERT_ATTRIB_GENERIC6 */
   VERT_ATTRIB_GENERIC7,            /* VERT_ATTRIB_GENERIC7 */
   VERT_ATTRIB_GENERIC8,            /* VERT_ATTRIB_GENERIC8 */
   VERT_ATTRIB_GENERIC9,            /* VERT_ATTRIB_GENERIC9 */
   VERT_ATTRIB_GENERIC10,           /* VERT_ATTRIB_GENERIC10 */
   VERT_ATTRIB_GENERIC11,           /* VERT_ATTRIB_GENERIC11 */
   VERT_ATTRIB_GENERIC12,           /* VERT_ATTRIB_GENERIC12 */
   VERT_ATTRIB_GENERIC13,           /* VERT_ATTRIB_GENERIC13 */
   VERT_ATTRIB_GENERIC14,           /* VERT_ATTRIB_GENERIC14 */
   VERT_ATTRIB_GENERIC15,           /* VERT_ATTRIB_GENERIC15 */
               /* ATTRIBUTE_MAP_MODE_POSITION
   *
   * Grab vertex processing attribute VERT_ATTRIB_POS as well as
   * vertex processing attribute VERT_ATTRIB_GENERIC0 from the
   * VAO attribute VERT_ATTRIB_POS.
   */
   {
      VERT_ATTRIB_POS,                 /* VERT_ATTRIB_POS */
   VERT_ATTRIB_NORMAL,              /* VERT_ATTRIB_NORMAL */
   VERT_ATTRIB_COLOR0,              /* VERT_ATTRIB_COLOR0 */
   VERT_ATTRIB_COLOR1,              /* VERT_ATTRIB_COLOR1 */
   VERT_ATTRIB_FOG,                 /* VERT_ATTRIB_FOG */
   VERT_ATTRIB_COLOR_INDEX,         /* VERT_ATTRIB_COLOR_INDEX */
   VERT_ATTRIB_TEX0,                /* VERT_ATTRIB_TEX0 */
   VERT_ATTRIB_TEX1,                /* VERT_ATTRIB_TEX1 */
   VERT_ATTRIB_TEX2,                /* VERT_ATTRIB_TEX2 */
   VERT_ATTRIB_TEX3,                /* VERT_ATTRIB_TEX3 */
   VERT_ATTRIB_TEX4,                /* VERT_ATTRIB_TEX4 */
   VERT_ATTRIB_TEX5,                /* VERT_ATTRIB_TEX5 */
   VERT_ATTRIB_TEX6,                /* VERT_ATTRIB_TEX6 */
   VERT_ATTRIB_TEX7,                /* VERT_ATTRIB_TEX7 */
   VERT_ATTRIB_POINT_SIZE,          /* VERT_ATTRIB_POINT_SIZE */
   VERT_ATTRIB_POS,                 /* VERT_ATTRIB_GENERIC0 */
   VERT_ATTRIB_GENERIC1,            /* VERT_ATTRIB_GENERIC1 */
   VERT_ATTRIB_GENERIC2,            /* VERT_ATTRIB_GENERIC2 */
   VERT_ATTRIB_GENERIC3,            /* VERT_ATTRIB_GENERIC3 */
   VERT_ATTRIB_GENERIC4,            /* VERT_ATTRIB_GENERIC4 */
   VERT_ATTRIB_GENERIC5,            /* VERT_ATTRIB_GENERIC5 */
   VERT_ATTRIB_GENERIC6,            /* VERT_ATTRIB_GENERIC6 */
   VERT_ATTRIB_GENERIC7,            /* VERT_ATTRIB_GENERIC7 */
   VERT_ATTRIB_GENERIC8,            /* VERT_ATTRIB_GENERIC8 */
   VERT_ATTRIB_GENERIC9,            /* VERT_ATTRIB_GENERIC9 */
   VERT_ATTRIB_GENERIC10,           /* VERT_ATTRIB_GENERIC10 */
   VERT_ATTRIB_GENERIC11,           /* VERT_ATTRIB_GENERIC11 */
   VERT_ATTRIB_GENERIC12,           /* VERT_ATTRIB_GENERIC12 */
   VERT_ATTRIB_GENERIC13,           /* VERT_ATTRIB_GENERIC13 */
   VERT_ATTRIB_GENERIC14,           /* VERT_ATTRIB_GENERIC14 */
   VERT_ATTRIB_GENERIC15,           /* VERT_ATTRIB_GENERIC15 */
               /* ATTRIBUTE_MAP_MODE_GENERIC0
   *
   * Grab vertex processing attribute VERT_ATTRIB_POS as well as
   * vertex processing attribute VERT_ATTRIB_GENERIC0 from the
   * VAO attribute VERT_ATTRIB_GENERIC0.
   */
   {
      VERT_ATTRIB_GENERIC0,            /* VERT_ATTRIB_POS */
   VERT_ATTRIB_NORMAL,              /* VERT_ATTRIB_NORMAL */
   VERT_ATTRIB_COLOR0,              /* VERT_ATTRIB_COLOR0 */
   VERT_ATTRIB_COLOR1,              /* VERT_ATTRIB_COLOR1 */
   VERT_ATTRIB_FOG,                 /* VERT_ATTRIB_FOG */
   VERT_ATTRIB_COLOR_INDEX,         /* VERT_ATTRIB_COLOR_INDEX */
   VERT_ATTRIB_TEX0,                /* VERT_ATTRIB_TEX0 */
   VERT_ATTRIB_TEX1,                /* VERT_ATTRIB_TEX1 */
   VERT_ATTRIB_TEX2,                /* VERT_ATTRIB_TEX2 */
   VERT_ATTRIB_TEX3,                /* VERT_ATTRIB_TEX3 */
   VERT_ATTRIB_TEX4,                /* VERT_ATTRIB_TEX4 */
   VERT_ATTRIB_TEX5,                /* VERT_ATTRIB_TEX5 */
   VERT_ATTRIB_TEX6,                /* VERT_ATTRIB_TEX6 */
   VERT_ATTRIB_TEX7,                /* VERT_ATTRIB_TEX7 */
   VERT_ATTRIB_POINT_SIZE,          /* VERT_ATTRIB_POINT_SIZE */
   VERT_ATTRIB_GENERIC0,            /* VERT_ATTRIB_GENERIC0 */
   VERT_ATTRIB_GENERIC1,            /* VERT_ATTRIB_GENERIC1 */
   VERT_ATTRIB_GENERIC2,            /* VERT_ATTRIB_GENERIC2 */
   VERT_ATTRIB_GENERIC3,            /* VERT_ATTRIB_GENERIC3 */
   VERT_ATTRIB_GENERIC4,            /* VERT_ATTRIB_GENERIC4 */
   VERT_ATTRIB_GENERIC5,            /* VERT_ATTRIB_GENERIC5 */
   VERT_ATTRIB_GENERIC6,            /* VERT_ATTRIB_GENERIC6 */
   VERT_ATTRIB_GENERIC7,            /* VERT_ATTRIB_GENERIC7 */
   VERT_ATTRIB_GENERIC8,            /* VERT_ATTRIB_GENERIC8 */
   VERT_ATTRIB_GENERIC9,            /* VERT_ATTRIB_GENERIC9 */
   VERT_ATTRIB_GENERIC10,           /* VERT_ATTRIB_GENERIC10 */
   VERT_ATTRIB_GENERIC11,           /* VERT_ATTRIB_GENERIC11 */
   VERT_ATTRIB_GENERIC12,           /* VERT_ATTRIB_GENERIC12 */
   VERT_ATTRIB_GENERIC13,           /* VERT_ATTRIB_GENERIC13 */
   VERT_ATTRIB_GENERIC14,           /* VERT_ATTRIB_GENERIC14 */
   VERT_ATTRIB_GENERIC15,           /* VERT_ATTRIB_GENERIC15 */
         };
         /**
   * Look up the array object for the given ID.
   *
   * \returns
   * Either a pointer to the array object with the specified ID or \c NULL for
   * a non-existent ID.  The spec defines ID 0 as being technically
   * non-existent.
   */
      struct gl_vertex_array_object *
   _mesa_lookup_vao(struct gl_context *ctx, GLuint id)
   {
      /* The ARB_direct_state_access specification says:
   *
   *    "<vaobj> is [compatibility profile:
   *     zero, indicating the default vertex array object, or]
   *     the name of the vertex array object."
   */
   if (id == 0) {
      if (_mesa_is_desktop_gl_compat(ctx))
               } else {
               if (ctx->Array.LastLookedUpVAO &&
      ctx->Array.LastLookedUpVAO->Name == id) {
      } else {
                                       }
         /**
   * Looks up the array object for the given ID.
   *
   * While _mesa_lookup_vao doesn't generate an error if the object does not
   * exist, this function comes in two variants.
   * If is_ext_dsa is false, this function generates a GL_INVALID_OPERATION
   * error if the array object does not exist. It also returns the default
   * array object when ctx is a compatibility profile context and id is zero.
   * If is_ext_dsa is true, 0 is not a valid name. If the name exists but
   * the object has never been bound, it is initialized.
   */
   struct gl_vertex_array_object *
   _mesa_lookup_vao_err(struct gl_context *ctx, GLuint id,
         {
      /* The ARB_direct_state_access specification says:
   *
   *    "<vaobj> is [compatibility profile:
   *     zero, indicating the default vertex array object, or]
   *     the name of the vertex array object."
   */
   if (id == 0) {
      if (is_ext_dsa || _mesa_is_desktop_gl_core(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "%s(zero is not valid vaobj name%s)",
                  } else {
               if (ctx->Array.LastLookedUpVAO &&
      ctx->Array.LastLookedUpVAO->Name == id) {
      } else {
                     /* The ARB_direct_state_access specification says:
   *
   *    "An INVALID_OPERATION error is generated if <vaobj> is not
   *     [compatibility profile: zero or] the name of an existing
   *     vertex array object."
   */
   if (!vao || (!is_ext_dsa && !vao->EverBound)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The EXT_direct_state_access specification says:
   *
   *    "If the vertex array object named by the vaobj parameter has not
   *     been previously bound but has been generated (without subsequent
   *     deletion) by GenVertexArrays, the GL first creates a new state
   *     vector in the same manner as when BindVertexArray creates a new
   *     vertex array object."
   */
                                    }
         /**
   * For all the vertex binding points in the array object, unbind any pointers
   * to any buffer objects (VBOs).
   * This is done just prior to array object destruction.
   */
   void
   _mesa_unbind_array_object_vbos(struct gl_context *ctx,
         {
               for (i = 0; i < ARRAY_SIZE(obj->BufferBinding); i++)
      }
         /**
   * Allocate and initialize a new vertex array object.
   */
   struct gl_vertex_array_object *
   _mesa_new_vao(struct gl_context *ctx, GLuint name)
   {
      struct gl_vertex_array_object *obj = MALLOC_STRUCT(gl_vertex_array_object);
   if (obj)
            }
         /**
   * Delete an array object.
   */
   void
   _mesa_delete_vao(struct gl_context *ctx, struct gl_vertex_array_object *obj)
   {
      _mesa_unbind_array_object_vbos(ctx, obj);
   _mesa_reference_buffer_object(ctx, &obj->IndexBufferObj, NULL);
   free(obj->Label);
      }
         /**
   * Set ptr to vao w/ reference counting.
   * Note: this should only be called from the _mesa_reference_vao()
   * inline function.
   */
   void
   _mesa_reference_vao_(struct gl_context *ctx,
               {
               if (*ptr) {
      /* Unreference the old array object */
            bool deleteFlag;
   if (oldObj->SharedAndImmutable) {
         } else {
      assert(oldObj->RefCount > 0);
   oldObj->RefCount--;
               if (deleteFlag)
               }
            if (vao) {
      /* reference new array object */
   if (vao->SharedAndImmutable) {
         } else {
      assert(vao->RefCount > 0);
                     }
         /**
   * Initialize a gl_vertex_array_object's arrays.
   */
   void
   _mesa_initialize_vao(struct gl_context *ctx,
               {
      memcpy(vao, &ctx->Array.DefaultVAOState, sizeof(*vao));
      }
         /**
   * Compute the offset range for the provided binding.
   *
   * This is a helper function for the below.
   */
   static void
   compute_vbo_offset_range(const struct gl_vertex_array_object *vao,
               {
      /* The function is meant to work on VBO bindings */
            /* Start with an inverted range of relative offsets. */
   GLuint min_offset = ~(GLuint)0;
            /* We work on the unmapped originaly VAO array entries. */
   GLbitfield mask = vao->Enabled & binding->_BoundArrays;
   /* The binding should be active somehow, not to return inverted ranges */
   assert(mask);
   while (mask) {
      const int i = u_bit_scan(&mask);
   const GLuint off = vao->VertexAttrib[i].RelativeOffset;
   min_offset = MIN2(off, min_offset);
               *min = binding->Offset + (GLsizeiptr)min_offset;
      }
         /**
   * Update the unique binding and pos/generic0 map tracking in the vao.
   *
   * The idea is to build up information in the vao so that a consuming
   * backend can execute the following to set up buffer and vertex element
   * information:
   *
   * const GLbitfield inputs_read = VERT_BIT_ALL; // backend vp inputs
   *
   * // Attribute data is in a VBO.
   * GLbitfield vbomask = inputs_read & _mesa_draw_vbo_array_bits(ctx);
   * while (vbomask) {
   *    // The attribute index to start pulling a binding
   *    const gl_vert_attrib i = ffs(vbomask) - 1;
   *    const struct gl_vertex_buffer_binding *const binding
   *       = _mesa_draw_buffer_binding(vao, i);
   *
   *    <insert code to handle the vertex buffer object at binding>
   *
   *    const GLbitfield boundmask = _mesa_draw_bound_attrib_bits(binding);
   *    GLbitfield attrmask = vbomask & boundmask;
   *    assert(attrmask);
   *    // Walk attributes belonging to the binding
   *    while (attrmask) {
   *       const gl_vert_attrib attr = u_bit_scan(&attrmask);
   *       const struct gl_array_attributes *const attrib
   *          = _mesa_draw_array_attrib(vao, attr);
   *
   *       <insert code to handle the vertex element refering to the binding>
   *    }
   *    vbomask &= ~boundmask;
   * }
   *
   * // Process user space buffers
   * GLbitfield usermask = inputs_read & _mesa_draw_user_array_bits(ctx);
   * while (usermask) {
   *    // The attribute index to start pulling a binding
   *    const gl_vert_attrib i = ffs(usermask) - 1;
   *    const struct gl_vertex_buffer_binding *const binding
   *       = _mesa_draw_buffer_binding(vao, i);
   *
   *    <insert code to handle a set of interleaved user space arrays at binding>
   *
   *    const GLbitfield boundmask = _mesa_draw_bound_attrib_bits(binding);
   *    GLbitfield attrmask = usermask & boundmask;
   *    assert(attrmask);
   *    // Walk interleaved attributes with a common stride and instance divisor
   *    while (attrmask) {
   *       const gl_vert_attrib attr = u_bit_scan(&attrmask);
   *       const struct gl_array_attributes *const attrib
   *          = _mesa_draw_array_attrib(vao, attr);
   *
   *       <insert code to handle non vbo vertex arrays>
   *    }
   *    usermask &= ~boundmask;
   * }
   *
   * // Process values that should have better been uniforms in the application
   * GLbitfield curmask = inputs_read & _mesa_draw_current_bits(ctx);
   * while (curmask) {
   *    const gl_vert_attrib attr = u_bit_scan(&curmask);
   *    const struct gl_array_attributes *const attrib
   *       = _mesa_draw_current_attrib(ctx, attr);
   *
   *    <insert code to handle current values>
   * }
   *
   *
   * Note that the scan below must not incoporate any context state.
   * The rationale is that once a VAO is finalized it should not
   * be touched anymore. That means, do not incorporate the
   * gl_context::Array._DrawVAOEnabledAttribs bitmask into this scan.
   * A backend driver may further reduce the handled vertex processing
   * inputs based on their vertex shader inputs. But scanning for
   * collapsable binding points to reduce relocs is done based on the
   * enabled arrays.
   * Also VAOs may be shared between contexts due to their use in dlists
   * thus no context state should bleed into the VAO.
   */
   void
   _mesa_update_vao_derived_arrays(struct gl_context *ctx,
         {
      assert(!vao->IsDynamic);
   /* Make sure we do not run into problems with shared objects */
            /* Limit used for common binding scanning below. */
   const GLsizeiptr MaxRelativeOffset =
            /* The gl_vertex_array_object::_AttributeMapMode denotes the way
   * VERT_ATTRIB_{POS,GENERIC0} mapping is done.
   *
   * This mapping is used to map between the OpenGL api visible
   * VERT_ATTRIB_* arrays to mesa driver arrayinputs or shader inputs.
   * The mapping only depends on the enabled bits of the
   * VERT_ATTRIB_{POS,GENERIC0} arrays and is tracked in the VAO.
   *
   * This map needs to be applied when finally translating to the bitmasks
   * as consumed by the driver backends. The duplicate scanning is here
   * can as well be done in the OpenGL API numbering without this map.
   */
   const gl_attribute_map_mode mode = vao->_AttributeMapMode;
   /* Enabled array bits. */
   const GLbitfield enabled = vao->Enabled;
   /* VBO array bits. */
            /* More than 4 updates turn the VAO to dynamic. */
   if (ctx->Const.AllowDynamicVAOFastPath && ++vao->NumUpdates > 4) {
      vao->IsDynamic = true;
   /* IsDynamic changes how vertex elements map to vertex buffers. */
   ctx->Array.NewVertexElements = true;
               /* Walk those enabled arrays that have a real vbo attached */
   GLbitfield mask = enabled;
   while (mask) {
      /* Do not use u_bit_scan as we can walk multiple attrib arrays at once */
   const int i = ffs(mask) - 1;
   /* The binding from the first to be processed attribute. */
   const GLuint bindex = vao->VertexAttrib[i].BufferBindingIndex;
            /* The scan goes different for user space arrays than vbos */
   if (binding->BufferObj) {
                                    /*
   * If there is nothing left to scan just update the effective binding
   * information. If the VAO is already only using a single binding point
   * we end up here. So the overhead of this scan for an application
                  GLbitfield scanmask = mask & vbos & ~bound;
   /* Is there something left to scan? */
   if (scanmask == 0) {
      /* Just update the back reference from the attrib to the binding and
   * the effective offset.
   */
   GLbitfield attrmask = eff_bound_arrays;
   while (attrmask) {
                     /* Update the index into the common binding point and offset */
   attrib2->_EffBufferBindingIndex = bindex;
   attrib2->_EffRelativeOffset = attrib2->RelativeOffset;
      }
   /* Finally this is the set of effectively bound arrays with the
   * original binding offset.
   */
   binding->_EffOffset = binding->Offset;
   /* The bound arrays past the VERT_ATTRIB_{POS,GENERIC0} mapping. */
               } else {
      /* In the VBO case, scan for attribute/binding
   * combinations with relative bindings in the range of
   * [0, ctx->Const.MaxVertexAttribRelativeOffset].
   * Note that this does also go beyond just interleaved arrays
   * as long as they use the same VBO, binding parameters and the
                  GLsizeiptr min_offset, max_offset;
                  /* Now scan. */
   while (scanmask) {
      /* Do not use u_bit_scan as we can walk multiple
   * attrib arrays at once
   */
   const int j = ffs(scanmask) - 1;
   const struct gl_array_attributes *attrib2 =
                        /* Remove those attrib bits from the mask that are bound to the
   * same effective binding point.
                        /* Check if we have an identical binding */
   if (binding->Stride != binding2->Stride)
         if (binding->InstanceDivisor != binding2->InstanceDivisor)
         if (binding->BufferObj != binding2->BufferObj)
         /* Check if we can fold both bindings into a common binding */
   GLsizeiptr min_offset2, max_offset2;
   compute_vbo_offset_range(vao, binding2,
         /* If the relative offset is within the limits ... */
   if (min_offset + MaxRelativeOffset < max_offset2)
         if (min_offset2 + MaxRelativeOffset < max_offset)
         /* ... add this array to the effective binding */
   eff_bound_arrays |= bound2;
   min_offset = MIN2(min_offset, min_offset2);
                     /* Update the back reference from the attrib to the binding */
   GLbitfield attrmask = eff_bound_arrays;
   while (attrmask) {
      const int j = u_bit_scan(&attrmask);
                        /* Update the index into the common binding point and offset */
   attrib2->_EffBufferBindingIndex = bindex;
   attrib2->_EffRelativeOffset =
            }
   /* Finally this is the set of effectively bound arrays */
   binding->_EffOffset = min_offset;
   /* The bound arrays past the VERT_ATTRIB_{POS,GENERIC0} mapping. */
   binding->_EffBoundArrays =
                           } else {
                                    /* Note that user space array pointers can only happen using a one
   * to one binding point to array mapping.
   * The OpenGL 4.x/ARB_vertex_attrib_binding api does not support
   * user space arrays collected at multiple binding points.
   * The only provider of user space interleaved arrays with a single
   * binding point is the mesa internal vbo module. But that one
   * provides a perfect interleaved set of arrays.
   *
   * If this would not be true we would potentially get attribute arrays
   * with user space pointers that may not lie within the
   * MaxRelativeOffset range but still attached to a single binding.
   * Then we would need to store the effective attribute and binding
   * grouping information in a seperate array beside
   * gl_array_attributes/gl_vertex_buffer_binding.
   */
                                                /* Walk other user space arrays and see which are interleaved
   * using the same binding parameters.
   */
   GLbitfield scanmask = mask & ~vbos & ~bound;
   while (scanmask) {
      const int j = u_bit_scan(&scanmask);
   const struct gl_array_attributes *attrib2 = &vao->VertexAttrib[j];
                  /* See the comment at the same assert above. */
                  /* Check if we have an identical binding */
   if (binding->Stride != binding2->Stride)
         if (binding->InstanceDivisor != binding2->InstanceDivisor)
         if (ptr <= attrib2->Ptr) {
      if (ptr + binding->Stride < attrib2->Ptr +
      attrib2->Format._ElementSize)
      unsigned end = attrib2->Ptr + attrib2->Format._ElementSize - ptr;
      } else {
      if (attrib2->Ptr + binding->Stride < ptr + vertex_end)
                                                      /* Update the back reference from the attrib to the binding */
   GLbitfield attrmask = eff_bound_arrays;
   while (attrmask) {
                     /* Update the index into the common binding point and the offset */
   attrib2->_EffBufferBindingIndex = bindex;
   attrib2->_EffRelativeOffset = attrib2->Ptr - ptr;
      }
   /* Finally this is the set of effectively bound arrays */
   binding->_EffOffset = (GLintptr)ptr;
   /* The bound arrays past the VERT_ATTRIB_{POS,GENERIC0} mapping. */
                  /* Mark all the effective bound arrays as processed. */
               #ifndef NDEBUG
      /* Make sure the above code works as expected. */
   for (gl_vert_attrib attr = 0; attr < VERT_ATTRIB_MAX; ++attr) {
      /* Query the original api defined attrib/binding information ... */
   const unsigned char *const map =_mesa_vao_attribute_map[mode];
   if (vao->Enabled & VERT_BIT(map[attr])) {
      const struct gl_array_attributes *attrib =
         const struct gl_vertex_buffer_binding *binding =
         /* ... and compare that with the computed attrib/binding */
   const struct gl_vertex_buffer_binding *binding2 =
         assert(binding->Stride == binding2->Stride);
   assert(binding->InstanceDivisor == binding2->InstanceDivisor);
   assert(binding->BufferObj == binding2->BufferObj);
   if (binding->BufferObj) {
      assert(attrib->_EffRelativeOffset <= MaxRelativeOffset);
   assert(binding->Offset + attrib->RelativeOffset ==
      } else {
      assert(attrib->_EffRelativeOffset < binding->Stride);
   assert((GLintptr)attrib->Ptr ==
               #endif
   }
         void
   _mesa_set_vao_immutable(struct gl_context *ctx,
         {
      _mesa_update_vao_derived_arrays(ctx, vao);
      }
         /**
   * Map buffer objects used in attribute arrays.
   */
   void
   _mesa_vao_map_arrays(struct gl_context *ctx, struct gl_vertex_array_object *vao,
         {
      GLbitfield mask = vao->Enabled & vao->VertexAttribBufferMask;
   while (mask) {
      /* Do not use u_bit_scan as we can walk multiple attrib arrays at once */
   const gl_vert_attrib attr = ffs(mask) - 1;
   const GLubyte bindex = vao->VertexAttrib[attr].BufferBindingIndex;
   struct gl_vertex_buffer_binding *binding = &vao->BufferBinding[bindex];
            struct gl_buffer_object *bo = binding->BufferObj;
   assert(bo);
   if (_mesa_bufferobj_mapped(bo, MAP_INTERNAL))
                  }
         /**
   * Map buffer objects used in the vao, attribute arrays and index buffer.
   */
   void
   _mesa_vao_map(struct gl_context *ctx, struct gl_vertex_array_object *vao,
         {
               /* map the index buffer, if there is one, and not already mapped */
   if (bo && !_mesa_bufferobj_mapped(bo, MAP_INTERNAL))
               }
         /**
   * Unmap buffer objects used in attribute arrays.
   */
   void
   _mesa_vao_unmap_arrays(struct gl_context *ctx,
         {
      GLbitfield mask = vao->Enabled & vao->VertexAttribBufferMask;
   while (mask) {
      /* Do not use u_bit_scan as we can walk multiple attrib arrays at once */
   const gl_vert_attrib attr = ffs(mask) - 1;
   const GLubyte bindex = vao->VertexAttrib[attr].BufferBindingIndex;
   struct gl_vertex_buffer_binding *binding = &vao->BufferBinding[bindex];
            struct gl_buffer_object *bo = binding->BufferObj;
   assert(bo);
   if (!_mesa_bufferobj_mapped(bo, MAP_INTERNAL))
                  }
         /**
   * Unmap buffer objects used in the vao, attribute arrays and index buffer.
   */
   void
   _mesa_vao_unmap(struct gl_context *ctx, struct gl_vertex_array_object *vao)
   {
               /* unmap the index buffer, if there is one, and still mapped */
   if (bo && _mesa_bufferobj_mapped(bo, MAP_INTERNAL))
               }
         /**********************************************************************/
   /* API Functions                                                      */
   /**********************************************************************/
         /**
   * ARB version of glBindVertexArray()
   */
   static ALWAYS_INLINE void
   bind_vertex_array(struct gl_context *ctx, GLuint id, bool no_error)
   {
      struct gl_vertex_array_object *const oldObj = ctx->Array.VAO;
                     if (oldObj->Name == id)
            /*
   * Get pointer to new array object (newObj)
   */
   if (id == 0) {
      /* The spec says there is no array object named 0, but we use
   * one internally because it simplifies things.
   */
      }
   else {
      /* non-default array object */
   newObj = _mesa_lookup_vao(ctx, id);
   if (!no_error && !newObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                                 _mesa_reference_vao(ctx, &ctx->Array.VAO, newObj);
            /* Update the valid-to-render state if binding on unbinding default VAO
   * if drawing with the default VAO is invalid.
   */
   if (_mesa_is_desktop_gl_core(ctx) &&
      (oldObj == ctx->Array.DefaultVAO) != (newObj == ctx->Array.DefaultVAO))
   }
         void GLAPIENTRY
   _mesa_BindVertexArray_no_error(GLuint id)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_BindVertexArray(GLuint id)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * Delete a set of array objects.
   *
   * \param n      Number of array objects to delete.
   * \param ids    Array of \c n array object IDs.
   */
   static void
   delete_vertex_arrays(struct gl_context *ctx, GLsizei n, const GLuint *ids)
   {
               for (i = 0; i < n; i++) {
      /* IDs equal to 0 should be silently ignored. */
   if (!ids[i])
                     if (obj) {
               /* If the array object is currently bound, the spec says "the binding
   * for that object reverts to zero and the default vertex array
   * becomes current."
   */
                                                /* Unreference the array object.
   * If refcount hits zero, the object will be deleted.
   */
            }
         void GLAPIENTRY
   _mesa_DeleteVertexArrays_no_error(GLsizei n, const GLuint *ids)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DeleteVertexArrays(GLsizei n, const GLuint *ids)
   {
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteVertexArray(n)");
                  }
         /**
   * Generate a set of unique array object IDs and store them in \c arrays.
   * Helper for _mesa_GenVertexArrays() and _mesa_CreateVertexArrays()
   * below.
   *
   * \param n       Number of IDs to generate.
   * \param arrays  Array of \c n locations to store the IDs.
   * \param create  Indicates that the objects should also be created.
   * \param func    The name of the GL entry point.
   */
   static void
   gen_vertex_arrays(struct gl_context *ctx, GLsizei n, GLuint *arrays,
         {
               if (!arrays)
                     /* For the sake of simplicity we create the array objects in both
   * the Gen* and Create* cases.  The only difference is the value of
   * EverBound, which is set to true in the Create* case.
   */
   for (i = 0; i < n; i++) {
               obj = _mesa_new_vao(ctx, arrays[i]);
   if (!obj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      }
   obj->EverBound = create;
         }
         static void
   gen_vertex_arrays_err(struct gl_context *ctx, GLsizei n, GLuint *arrays,
         {
      if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", func);
                  }
         /**
   * ARB version of glGenVertexArrays()
   * All arrays will be required to live in VBOs.
   */
   void GLAPIENTRY
   _mesa_GenVertexArrays_no_error(GLsizei n, GLuint *arrays)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GenVertexArrays(GLsizei n, GLuint *arrays)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * ARB_direct_state_access
   * Generates ID's and creates the array objects.
   */
   void GLAPIENTRY
   _mesa_CreateVertexArrays_no_error(GLsizei n, GLuint *arrays)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_CreateVertexArrays(GLsizei n, GLuint *arrays)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * Determine if ID is the name of an array object.
   *
   * \param id  ID of the potential array object.
   * \return  \c GL_TRUE if \c id is the name of a array object,
   *          \c GL_FALSE otherwise.
   */
   GLboolean GLAPIENTRY
   _mesa_IsVertexArray( GLuint id )
   {
      struct gl_vertex_array_object * obj;
   GET_CURRENT_CONTEXT(ctx);
                        }
         /**
   * Sets the element array buffer binding of a vertex array object.
   *
   * This is the ARB_direct_state_access equivalent of
   * glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer).
   */
   static ALWAYS_INLINE void
   vertex_array_element_buffer(struct gl_context *ctx, GLuint vaobj, GLuint buffer,
         {
      struct gl_vertex_array_object *vao;
                     if (!no_error) {
      /* The GL_ARB_direct_state_access specification says:
   *
   *    "An INVALID_OPERATION error is generated by
   *     VertexArrayElementBuffer if <vaobj> is not [compatibility profile:
   *     zero or] the name of an existing vertex array object."
   */
   vao =_mesa_lookup_vao_err(ctx, vaobj, false, "glVertexArrayElementBuffer");
   if (!vao)
      } else {
                  if (buffer != 0) {
      if (!no_error) {
      /* The GL_ARB_direct_state_access specification says:
   *
   *    "An INVALID_OPERATION error is generated if <buffer> is not zero
   *     or the name of an existing buffer object."
   */
   bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
      } else {
                  if (!bufObj)
      } else {
                     }
         void GLAPIENTRY
   _mesa_VertexArrayElementBuffer_no_error(GLuint vaobj, GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_VertexArrayElementBuffer(GLuint vaobj, GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param)
   {
      GET_CURRENT_CONTEXT(ctx);
                     /* The GL_ARB_direct_state_access specification says:
   *
   *   "An INVALID_OPERATION error is generated if <vaobj> is not
   *    [compatibility profile: zero or] the name of an existing
   *    vertex array object."
   */
   vao = _mesa_lookup_vao_err(ctx, vaobj, false, "glGetVertexArrayiv");
   if (!vao)
            /* The GL_ARB_direct_state_access specification says:
   *
   *   "An INVALID_ENUM error is generated if <pname> is not
   *    ELEMENT_ARRAY_BUFFER_BINDING."
   */
   if (pname != GL_ELEMENT_ARRAY_BUFFER_BINDING) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                              }
