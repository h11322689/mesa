   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_defines.h"
   #include "util/format/u_format.h"
      #include "freedreno_util.h"
      int32_t marker_cnt;
      enum adreno_rb_depth_format
   fd_pipe2depth(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
         case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         default:
            }
      enum pc_di_index_size
   fd_pipe2index(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_R8_UINT:
         case PIPE_FORMAT_R16_UINT:
         case PIPE_FORMAT_R32_UINT:
         default:
            }
      /* we need to special case a bit the depth/stencil restore, because we are
   * using the texture sampler to blit into the depth/stencil buffer, *not*
   * into a color buffer.  Otherwise fdN_tex_swiz() will do the wrong thing,
   * as it is assuming that you are sampling into normal render target..
   */
   enum pipe_format
   fd_gmem_restore_format(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_S8_UINT:
         default:
            }
      enum adreno_rb_blend_factor
   fd_blend_factor(unsigned factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_DST_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         case PIPE_BLENDFACTOR_CONST_COLOR:
         case PIPE_BLENDFACTOR_CONST_ALPHA:
         case PIPE_BLENDFACTOR_ZERO:
   case 0:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         case PIPE_BLENDFACTOR_SRC1_COLOR:
         case PIPE_BLENDFACTOR_SRC1_ALPHA:
         default:
      DBG("invalid blend factor: %x", factor);
         }
      enum adreno_pa_su_sc_draw
   fd_polygon_mode(unsigned mode)
   {
      switch (mode) {
   case PIPE_POLYGON_MODE_POINT:
         case PIPE_POLYGON_MODE_LINE:
         case PIPE_POLYGON_MODE_FILL:
         default:
      DBG("invalid polygon mode: %u", mode);
         }
      enum adreno_stencil_op
   fd_stencil_op(unsigned op)
   {
      switch (op) {
   case PIPE_STENCIL_OP_KEEP:
         case PIPE_STENCIL_OP_ZERO:
         case PIPE_STENCIL_OP_REPLACE:
         case PIPE_STENCIL_OP_INCR:
         case PIPE_STENCIL_OP_DECR:
         case PIPE_STENCIL_OP_INCR_WRAP:
         case PIPE_STENCIL_OP_DECR_WRAP:
         case PIPE_STENCIL_OP_INVERT:
         default:
      DBG("invalid stencil op: %u", op);
         }
