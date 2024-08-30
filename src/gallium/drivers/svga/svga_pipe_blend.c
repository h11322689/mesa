   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "util/u_inlines.h"
   #include "pipe/p_defines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
      #include "svga_context.h"
   #include "svga_hw_reg.h"
   #include "svga_cmd.h"
         static inline unsigned
   svga_translate_blend_factor(const struct svga_context *svga, unsigned factor)
   {
      /* Note: there is no SVGA3D_BLENDOP_[INV]BLENDFACTORALPHA so
   * we can't translate PIPE_BLENDFACTOR_[INV_]CONST_ALPHA properly.
   */
   switch (factor) {
   case PIPE_BLENDFACTOR_ZERO:            return SVGA3D_BLENDOP_ZERO;
   case PIPE_BLENDFACTOR_SRC_ALPHA:       return SVGA3D_BLENDOP_SRCALPHA;
   case PIPE_BLENDFACTOR_ONE:             return SVGA3D_BLENDOP_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR:       return SVGA3D_BLENDOP_SRCCOLOR;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:   return SVGA3D_BLENDOP_INVSRCCOLOR;
   case PIPE_BLENDFACTOR_DST_COLOR:       return SVGA3D_BLENDOP_DESTCOLOR;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:   return SVGA3D_BLENDOP_INVDESTCOLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:   return SVGA3D_BLENDOP_INVSRCALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA:       return SVGA3D_BLENDOP_DESTALPHA;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:   return SVGA3D_BLENDOP_INVDESTALPHA;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return SVGA3D_BLENDOP_SRCALPHASAT;
   case PIPE_BLENDFACTOR_CONST_COLOR:     return SVGA3D_BLENDOP_BLENDFACTOR;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR: return SVGA3D_BLENDOP_INVBLENDFACTOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      if (svga_have_vgpu10(svga))
         else
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      if (svga_have_vgpu10(svga))
         else
      case PIPE_BLENDFACTOR_SRC1_COLOR:      return SVGA3D_BLENDOP_SRC1COLOR;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:  return SVGA3D_BLENDOP_INVSRC1COLOR;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:      return SVGA3D_BLENDOP_SRC1ALPHA;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:  return SVGA3D_BLENDOP_INVSRC1ALPHA;
   case 0:                                return SVGA3D_BLENDOP_ONE;
   default:
      assert(0);
         }
      static inline unsigned
   svga_translate_blend_func(unsigned mode)
   {
      switch (mode) {
   case PIPE_BLEND_ADD:              return SVGA3D_BLENDEQ_ADD;
   case PIPE_BLEND_SUBTRACT:         return SVGA3D_BLENDEQ_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT: return SVGA3D_BLENDEQ_REVSUBTRACT;
   case PIPE_BLEND_MIN:              return SVGA3D_BLENDEQ_MINIMUM;
   case PIPE_BLEND_MAX:              return SVGA3D_BLENDEQ_MAXIMUM;
   default:
      assert(0);
         }
         /**
   * Translate gallium logicop mode to SVGA3D logicop mode.
   */
   static int
   translate_logicop(enum pipe_logicop op)
   {
      switch (op) {
   case PIPE_LOGICOP_CLEAR:
         case PIPE_LOGICOP_NOR:
         case PIPE_LOGICOP_AND_INVERTED:
         case PIPE_LOGICOP_COPY_INVERTED:
         case PIPE_LOGICOP_AND_REVERSE:
         case PIPE_LOGICOP_INVERT:
         case PIPE_LOGICOP_XOR:
         case PIPE_LOGICOP_NAND:
         case PIPE_LOGICOP_AND:
         case PIPE_LOGICOP_EQUIV:
         case PIPE_LOGICOP_NOOP:
         case PIPE_LOGICOP_OR_INVERTED:
         case PIPE_LOGICOP_COPY:
         case PIPE_LOGICOP_OR_REVERSE:
         case PIPE_LOGICOP_OR:
         case PIPE_LOGICOP_SET:
         default:
            };
         /**
   * Define a vgpu10 blend state object for the given
   * svga blend state.
   */
   static void
   define_blend_state_object(struct svga_context *svga,
         {
      SVGA3dDXBlendStatePerRT perRT[SVGA3D_MAX_RENDER_TARGETS];
                              for (i = 0; i < SVGA3D_DX_MAX_RENDER_TARGETS; i++) {
      perRT[i].blendEnable = bs->rt[i].blend_enable;
   perRT[i].srcBlend = bs->rt[i].srcblend;
   perRT[i].destBlend = bs->rt[i].dstblend;
   perRT[i].blendOp = bs->rt[i].blendeq;
   perRT[i].srcBlendAlpha = bs->rt[i].srcblend_alpha;
   perRT[i].destBlendAlpha = bs->rt[i].dstblend_alpha;
   perRT[i].blendOpAlpha = bs->rt[i].blendeq_alpha;
   perRT[i].renderTargetWriteMask = bs->rt[i].writemask;
   perRT[i].logicOpEnable = bs->logicop_enabled;
               SVGA_RETRY(svga, SVGA3D_vgpu10_DefineBlendState(svga->swc,
                        }
         /**
   * If SVGA3D_DEVCAP_LOGIC_BLENDOPS is false, we can't directly implement
   * GL's logicops.  But we can emulate some of them.  We set up the blending
   * state for that here.
   */
   static void
   emulate_logicop(struct svga_context *svga,
                     {
      switch (logicop_func) {
   case PIPE_LOGICOP_XOR:
   case PIPE_LOGICOP_INVERT:
      blend->need_white_fragments = true;
   blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_ONE;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_ONE;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_SUBTRACT;
      case PIPE_LOGICOP_CLEAR:
      blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_ZERO;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_ZERO;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
      case PIPE_LOGICOP_COPY:
      blend->rt[buffer].blend_enable = false;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_ONE;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_ZERO;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_ADD;
      case PIPE_LOGICOP_COPY_INVERTED:
      blend->rt[buffer].blend_enable   = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_INVSRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_ZERO;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_ADD;
      case PIPE_LOGICOP_NOOP:
      blend->rt[buffer].blend_enable   = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_ZERO;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_ADD;
      case PIPE_LOGICOP_SET:
      blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_ONE;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_ONE;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
      case PIPE_LOGICOP_AND:
      /* Approximate with minimum - works for the 0 & anything case: */
   blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
      case PIPE_LOGICOP_AND_REVERSE:
      blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_INVDESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
      case PIPE_LOGICOP_AND_INVERTED:
      blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_INVSRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MINIMUM;
      case PIPE_LOGICOP_OR:
      /* Approximate with maximum - works for the 1 | anything case: */
   blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
      case PIPE_LOGICOP_OR_REVERSE:
      blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_SRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_INVDESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
      case PIPE_LOGICOP_OR_INVERTED:
      blend->rt[buffer].blend_enable = true;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_INVSRCCOLOR;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_DESTCOLOR;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_MAXIMUM;
      case PIPE_LOGICOP_NAND:
   case PIPE_LOGICOP_NOR:
   case PIPE_LOGICOP_EQUIV:
      /* Fill these in with plausible values */
   blend->rt[buffer].blend_enable = false;
   blend->rt[buffer].srcblend       = SVGA3D_BLENDOP_ONE;
   blend->rt[buffer].dstblend       = SVGA3D_BLENDOP_ZERO;
   blend->rt[buffer].blendeq        = SVGA3D_BLENDEQ_ADD;
      default:
      assert(0);
      }
   blend->rt[buffer].srcblend_alpha = blend->rt[buffer].srcblend;
   blend->rt[buffer].dstblend_alpha = blend->rt[buffer].dstblend;
            if (logicop_func == PIPE_LOGICOP_XOR) {
      util_debug_message(&svga->debug.callback, CONFORMANCE,
      }
   else if (logicop_func != PIPE_LOGICOP_COPY) {
      util_debug_message(&svga->debug.callback, CONFORMANCE,
         }
            static void *
   svga_create_blend_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_blend_state *blend = CALLOC_STRUCT( svga_blend_state );
            if (!blend)
            /* Find index of first target with blending enabled.  If no blending is
   * enabled at all, first_enabled will be zero.
   */
   unsigned first_enabled = 0;
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (templ->rt[i].blend_enable) {
      first_enabled = i;
                  /* Fill in the per-rendertarget blend state.  We currently only
   * support independent blend enable and colormask per render target.
   */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      /* No way to set this in SVGA3D, and no way to correctly implement it on
   * top of D3D9 API.  Instead we try to simulate with various blend modes.
   */
   if (templ->logicop_enable) {
      if (ss->haveBlendLogicops) {
      blend->logicop_enabled = true;
   blend->logicop_mode = translate_logicop(templ->logicop_func);
   blend->rt[i].blendeq = SVGA3D_BLENDEQ_ADD;
   blend->rt[i].blendeq_alpha = SVGA3D_BLENDEQ_ADD;
   blend->rt[i].srcblend = SVGA3D_BLENDOP_ZERO;
   blend->rt[i].dstblend = SVGA3D_BLENDOP_ZERO;
   blend->rt[i].srcblend_alpha = SVGA3D_BLENDOP_ZERO;
      }
   else {
            }
   else {
      /* Note: per-target blend terms are only supported for sm4_1
   * device. For vgpu10 device, the blending terms must be identical
   * for all targets (this is why we need the first_enabled index).
   */
   const unsigned j =
      svga_have_sm4_1(svga) && templ->independent_blend_enable
      if (templ->independent_blend_enable || templ->rt[j].blend_enable) {
      blend->rt[i].srcblend =
         blend->rt[i].dstblend =
         blend->rt[i].blendeq =
         blend->rt[i].srcblend_alpha =
         blend->rt[i].dstblend_alpha =
                        if (blend->rt[i].srcblend_alpha != blend->rt[i].srcblend ||
      blend->rt[i].dstblend_alpha != blend->rt[i].dstblend ||
   blend->rt[i].blendeq_alpha  != blend->rt[i].blendeq) {
         }
   else {
      /* disabled - default blend terms */
   blend->rt[i].srcblend = SVGA3D_BLENDOP_ONE;
   blend->rt[i].dstblend = SVGA3D_BLENDOP_ZERO;
   blend->rt[i].blendeq = SVGA3D_BLENDEQ_ADD;
   blend->rt[i].srcblend_alpha = SVGA3D_BLENDOP_ONE;
   blend->rt[i].dstblend_alpha = SVGA3D_BLENDOP_ZERO;
               if (templ->independent_blend_enable) {
         }
   else {
                     /* Some GL blend modes are not supported by the VGPU9 device (there's
   * no equivalent of PIPE_BLENDFACTOR_[INV_]CONST_ALPHA).
   * When we set this flag, we copy the constant blend alpha value
   * to the R, G, B components.
   * This works as long as the src/dst RGB blend factors doesn't use
   * PIPE_BLENDFACTOR_CONST_COLOR and PIPE_BLENDFACTOR_CONST_ALPHA
   * at the same time.  There's no work-around for that.
   */
   if (!svga_have_vgpu10(svga)) {
      if (templ->rt[0].rgb_src_factor == PIPE_BLENDFACTOR_CONST_ALPHA ||
      templ->rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_CONST_ALPHA ||
   templ->rt[0].rgb_src_factor == PIPE_BLENDFACTOR_INV_CONST_ALPHA ||
   templ->rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_INV_CONST_ALPHA) {
                  if (templ->independent_blend_enable) {
         }
   else {
                              blend->alpha_to_coverage = templ->alpha_to_coverage;
            if (svga_have_vgpu10(svga)) {
                  svga->hud.num_blend_objects++;
   SVGA_STATS_COUNT_INC(svga_screen(svga->pipe.screen)->sws,
               }
         static void svga_bind_blend_state(struct pipe_context *pipe,
         {
               svga->curr.blend = (struct svga_blend_state*)blend;
      }
      static void svga_delete_blend_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
   struct svga_blend_state *bs =
            if (svga_have_vgpu10(svga) && bs->id != SVGA3D_INVALID_ID) {
               if (bs->id == svga->state.hw_draw.blend_id)
            util_bitmask_clear(svga->blend_object_id_bm, bs->id);
               FREE(blend);
      }
      static void svga_set_blend_color( struct pipe_context *pipe,
         {
                           }
         void svga_init_blend_functions( struct svga_context *svga )
   {
      svga->pipe.create_blend_state = svga_create_blend_state;
   svga->pipe.bind_blend_state = svga_bind_blend_state;
               }
