   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dxil_enums.h"
      #include "nir.h"
   #include "nir_types.h"
      #include "util/u_debug.h"
      enum dxil_prog_sig_comp_type dxil_get_prog_sig_comp_type(const struct glsl_type *type)
   {
               switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_UINT: return DXIL_PROG_SIG_COMP_TYPE_UINT32;
   case GLSL_TYPE_INT: return DXIL_PROG_SIG_COMP_TYPE_SINT32;
   case GLSL_TYPE_FLOAT: return DXIL_PROG_SIG_COMP_TYPE_FLOAT32;
   case GLSL_TYPE_FLOAT16: return DXIL_PROG_SIG_COMP_TYPE_FLOAT16;
   case GLSL_TYPE_DOUBLE: return DXIL_PROG_SIG_COMP_TYPE_FLOAT64;
   case GLSL_TYPE_UINT16: return DXIL_PROG_SIG_COMP_TYPE_UINT16;
   case GLSL_TYPE_INT16: return DXIL_PROG_SIG_COMP_TYPE_SINT16;
   case GLSL_TYPE_UINT64: return DXIL_PROG_SIG_COMP_TYPE_UINT64;
   case GLSL_TYPE_INT64: return DXIL_PROG_SIG_COMP_TYPE_SINT64;
   case GLSL_TYPE_BOOL: return DXIL_PROG_SIG_COMP_TYPE_UINT32;
   case GLSL_TYPE_STRUCT: return DXIL_PROG_SIG_COMP_TYPE_UNKNOWN;
   default:
      debug_printf("unexpected type: %s\n", glsl_get_type_name(type));
         }
      enum dxil_component_type dxil_get_comp_type(const struct glsl_type *type)
   {
               enum glsl_base_type base_type = glsl_get_base_type(type);
   if (glsl_type_is_texture(type) || glsl_type_is_image(type))
         switch (base_type) {
   case GLSL_TYPE_UINT: return DXIL_COMP_TYPE_U32;
   case GLSL_TYPE_INT: return DXIL_COMP_TYPE_I32;
   case GLSL_TYPE_FLOAT: return DXIL_COMP_TYPE_F32;
   case GLSL_TYPE_FLOAT16: return DXIL_COMP_TYPE_F16;
   case GLSL_TYPE_DOUBLE: return DXIL_COMP_TYPE_F64;
   case GLSL_TYPE_UINT16: return DXIL_COMP_TYPE_U16;
   case GLSL_TYPE_INT16: return DXIL_COMP_TYPE_I16;
   case GLSL_TYPE_UINT64: return DXIL_COMP_TYPE_U64;
   case GLSL_TYPE_INT64: return DXIL_COMP_TYPE_I64;
            default:
      debug_printf("type: %s\n", glsl_get_type_name(type));
         }
      enum dxil_resource_kind dxil_sampler_dim_to_resource_kind(enum glsl_sampler_dim dim, bool is_array)
   {
      switch (dim) {
      case GLSL_SAMPLER_DIM_1D:
      return is_array ? DXIL_RESOURCE_KIND_TEXTURE1D_ARRAY
      case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_EXTERNAL:
      return is_array ? DXIL_RESOURCE_KIND_TEXTURE2D_ARRAY
      case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_CUBE:
      return is_array ? DXIL_RESOURCE_KIND_TEXTURECUBE_ARRAY
      case GLSL_SAMPLER_DIM_RECT:
         case GLSL_SAMPLER_DIM_BUF:
         case GLSL_SAMPLER_DIM_MS:
      return is_array ? DXIL_RESOURCE_KIND_TEXTURE2DMS_ARRAY
      case GLSL_SAMPLER_DIM_SUBPASS_MS:
            default:
         }
      enum dxil_resource_kind dxil_get_resource_kind(const struct glsl_type *type)
   {
               /* This looks weird, we strip the arrays but then we still test whether it's
   * an array, key is the first refers to sampler[] and the second to samplerArray */
            if (glsl_type_is_texture(type) || glsl_type_is_image(type))
            debug_printf("type: %s\n", glsl_get_type_name(type));
      }
      enum dxil_input_primitive dxil_get_input_primitive(unsigned primitive)
   {
      switch (primitive) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
         case MESA_PRIM_LINES_ADJACENCY:
         case MESA_PRIM_TRIANGLES:
         case MESA_PRIM_TRIANGLES_ADJACENCY:
         default:
            }
      enum dxil_primitive_topology dxil_get_primitive_topology(unsigned topology)
   {
      switch (topology) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
         case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_TRIANGLE_STRIP:
         default:
            }
      static const char *overload_str[DXIL_NUM_OVERLOADS] = {
      [DXIL_NONE] = "",
   [DXIL_I1] = "i1",
   [DXIL_I16] = "i16",
   [DXIL_I32] = "i32",
   [DXIL_I64] = "i64",
   [DXIL_F16] = "f16",
   [DXIL_F32] = "f32",
      };
      const char *dxil_overload_suffix( enum overload_type overload)
   {
      assert(overload < DXIL_NUM_OVERLOADS);
      }
