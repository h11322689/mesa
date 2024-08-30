   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
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
      #include "tgsi/tgsi_from_mesa.h"
      #include "util/compiler.h"
      #include "util/compiler.h"
      /**
   * Determine the semantic index that is used when the given varying is mapped
   * to TGSI_SEMANTIC_GENERIC.
   */
   unsigned
   tgsi_get_generic_gl_varying_index(gl_varying_slot attr,
         {
      if (attr >= VARYING_SLOT_VAR0) {
      if (needs_texcoord_semantic)
         else
      }
   if (attr == VARYING_SLOT_PNTC) {
      assert(!needs_texcoord_semantic);
      }
   if (attr >= VARYING_SLOT_TEX0 && attr <= VARYING_SLOT_TEX7) {
      assert(!needs_texcoord_semantic);
               if (attr == VARYING_SLOT_CULL_PRIMITIVE)
            assert(0);
      }
      /**
   * Determine the semantic name and index used for the given varying.
   */
   void
   tgsi_get_gl_varying_semantic(gl_varying_slot attr,
                     {
      switch (attr) {
   case VARYING_SLOT_PRIMITIVE_ID:
      *semantic_name = TGSI_SEMANTIC_PRIMID;
   *semantic_index = 0;
      case VARYING_SLOT_POS:
      *semantic_name = TGSI_SEMANTIC_POSITION;
   *semantic_index = 0;
      case VARYING_SLOT_COL0:
      *semantic_name = TGSI_SEMANTIC_COLOR;
   *semantic_index = 0;
      case VARYING_SLOT_COL1:
      *semantic_name = TGSI_SEMANTIC_COLOR;
   *semantic_index = 1;
      case VARYING_SLOT_BFC0:
      *semantic_name = TGSI_SEMANTIC_BCOLOR;
   *semantic_index = 0;
      case VARYING_SLOT_BFC1:
      *semantic_name = TGSI_SEMANTIC_BCOLOR;
   *semantic_index = 1;
      case VARYING_SLOT_FOGC:
      *semantic_name = TGSI_SEMANTIC_FOG;
   *semantic_index = 0;
      case VARYING_SLOT_PSIZ:
      *semantic_name = TGSI_SEMANTIC_PSIZE;
   *semantic_index = 0;
      case VARYING_SLOT_CLIP_DIST0:
      *semantic_name = TGSI_SEMANTIC_CLIPDIST;
   *semantic_index = 0;
      case VARYING_SLOT_CLIP_DIST1:
      *semantic_name = TGSI_SEMANTIC_CLIPDIST;
   *semantic_index = 1;
      case VARYING_SLOT_CULL_DIST0:
   case VARYING_SLOT_CULL_DIST1:
      /* these should have been lowered by GLSL */
   assert(0);
      case VARYING_SLOT_EDGE:
      *semantic_name = TGSI_SEMANTIC_EDGEFLAG;
   *semantic_index = 0;
      case VARYING_SLOT_CLIP_VERTEX:
      *semantic_name = TGSI_SEMANTIC_CLIPVERTEX;
   *semantic_index = 0;
      case VARYING_SLOT_LAYER:
      *semantic_name = TGSI_SEMANTIC_LAYER;
   *semantic_index = 0;
      case VARYING_SLOT_VIEWPORT:
      *semantic_name = TGSI_SEMANTIC_VIEWPORT_INDEX;
   *semantic_index = 0;
      case VARYING_SLOT_FACE:
      *semantic_name = TGSI_SEMANTIC_FACE;
   *semantic_index = 0;
      case VARYING_SLOT_PNTC:
      *semantic_name = TGSI_SEMANTIC_PCOORD;
   *semantic_index = 0;
      case VARYING_SLOT_TESS_LEVEL_OUTER:
      *semantic_name = TGSI_SEMANTIC_TESSOUTER;
   *semantic_index = 0;
      case VARYING_SLOT_TESS_LEVEL_INNER:
      *semantic_name = TGSI_SEMANTIC_TESSINNER;
   *semantic_index = 0;
      case VARYING_SLOT_VIEWPORT_MASK:
      *semantic_name = TGSI_SEMANTIC_VIEWPORT_MASK;
   *semantic_index = 0;
      case VARYING_SLOT_CULL_PRIMITIVE:
      /* mesh only, just pick something random */
   *semantic_name = TGSI_SEMANTIC_PATCH;
   *semantic_index = 0;
      case VARYING_SLOT_TEX0:
   case VARYING_SLOT_TEX1:
   case VARYING_SLOT_TEX2:
   case VARYING_SLOT_TEX3:
   case VARYING_SLOT_TEX4:
   case VARYING_SLOT_TEX5:
   case VARYING_SLOT_TEX6:
   case VARYING_SLOT_TEX7:
      if (needs_texcoord_semantic) {
      *semantic_name = TGSI_SEMANTIC_TEXCOORD;
   *semantic_index = attr - VARYING_SLOT_TEX0;
      }
      case VARYING_SLOT_VAR0:
   default:
      assert(attr >= VARYING_SLOT_VAR0 ||
         if (attr >= VARYING_SLOT_PATCH0) {
      *semantic_name = TGSI_SEMANTIC_PATCH;
      } else {
      *semantic_name = TGSI_SEMANTIC_GENERIC;
   *semantic_index =
      }
         }
      /**
   * Determine the semantic name and index used for the given fragment shader
   * result.
   */
   void
   tgsi_get_gl_frag_result_semantic(gl_frag_result frag_result,
               {
      if (frag_result >= FRAG_RESULT_DATA0) {
      *semantic_name = TGSI_SEMANTIC_COLOR;
   *semantic_index = frag_result - FRAG_RESULT_DATA0;
                        switch (frag_result) {
   case FRAG_RESULT_DEPTH:
      *semantic_name = TGSI_SEMANTIC_POSITION;
      case FRAG_RESULT_STENCIL:
      *semantic_name = TGSI_SEMANTIC_STENCIL;
      case FRAG_RESULT_COLOR:
      *semantic_name = TGSI_SEMANTIC_COLOR;
      case FRAG_RESULT_SAMPLE_MASK:
      *semantic_name = TGSI_SEMANTIC_SAMPLEMASK;
      default:
            }
      /** Map Mesa's SYSTEM_VALUE_x to TGSI_SEMANTIC_x */
   enum tgsi_semantic
   tgsi_get_sysval_semantic(unsigned sysval)
   {
      switch (sysval) {
   /* Vertex shader */
   case SYSTEM_VALUE_VERTEX_ID:
         case SYSTEM_VALUE_INSTANCE_ID:
         case SYSTEM_VALUE_VERTEX_ID_ZERO_BASE:
         case SYSTEM_VALUE_BASE_VERTEX:
         case SYSTEM_VALUE_BASE_INSTANCE:
         case SYSTEM_VALUE_DRAW_ID:
            /* Geometry shader */
   case SYSTEM_VALUE_INVOCATION_ID:
            /* Fragment shader */
   case SYSTEM_VALUE_FRAG_COORD:
         case SYSTEM_VALUE_POINT_COORD:
         case SYSTEM_VALUE_FRONT_FACE:
         case SYSTEM_VALUE_SAMPLE_ID:
         case SYSTEM_VALUE_SAMPLE_POS:
         case SYSTEM_VALUE_SAMPLE_MASK_IN:
         case SYSTEM_VALUE_HELPER_INVOCATION:
            /* Tessellation shader */
   case SYSTEM_VALUE_TESS_COORD:
         case SYSTEM_VALUE_VERTICES_IN:
         case SYSTEM_VALUE_PRIMITIVE_ID:
         case SYSTEM_VALUE_TESS_LEVEL_OUTER:
         case SYSTEM_VALUE_TESS_LEVEL_INNER:
            /* Compute shader */
   case SYSTEM_VALUE_LOCAL_INVOCATION_ID:
         case SYSTEM_VALUE_WORKGROUP_ID:
         case SYSTEM_VALUE_NUM_WORKGROUPS:
         case SYSTEM_VALUE_WORKGROUP_SIZE:
            /* ARB_shader_ballot */
   case SYSTEM_VALUE_SUBGROUP_SIZE:
         case SYSTEM_VALUE_SUBGROUP_INVOCATION:
         case SYSTEM_VALUE_SUBGROUP_EQ_MASK:
         case SYSTEM_VALUE_SUBGROUP_GE_MASK:
         case SYSTEM_VALUE_SUBGROUP_GT_MASK:
         case SYSTEM_VALUE_SUBGROUP_LE_MASK:
         case SYSTEM_VALUE_SUBGROUP_LT_MASK:
            default:
            }
      enum tgsi_interpolate_mode
   tgsi_get_interp_mode(enum glsl_interp_mode mode, bool color)
   {
      switch (mode) {
   case INTERP_MODE_NONE:
         case INTERP_MODE_FLAT:
         case INTERP_MODE_NOPERSPECTIVE:
         case INTERP_MODE_SMOOTH:
         default:
            }
