   /*
   * Copyright © 2017 Intel Corporation
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
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_state.c
   *
   * ============================= GENXML CODE =============================
   *              [This file is compiled once per generation.]
   * =======================================================================
   *
   * This is the main state upload code.
   *
   * Gallium uses Constant State Objects, or CSOs, for most state.  Large,
   * complex, or highly reusable state can be created once, and bound and
   * rebound multiple times.  This is modeled with the pipe->create_*_state()
   * and pipe->bind_*_state() hooks.  Highly dynamic or inexpensive state is
   * streamed out on the fly, via pipe->set_*_state() hooks.
   *
   * OpenGL involves frequently mutating context state, which is mirrored in
   * core Mesa by highly mutable data structures.  However, most applications
   * typically draw the same things over and over - from frame to frame, most
   * of the same objects are still visible and need to be redrawn.  So, rather
   * than inventing new state all the time, applications usually mutate to swap
   * between known states that we've seen before.
   *
   * Gallium isolates us from this mutation by tracking API state, and
   * distilling it into a set of Constant State Objects, or CSOs.  Large,
   * complex, or typically reusable state can be created once, then reused
   * multiple times.  Drivers can create and store their own associated data.
   * This create/bind model corresponds to the pipe->create_*_state() and
   * pipe->bind_*_state() driver hooks.
   *
   * Some state is cheap to create, or expected to be highly dynamic.  Rather
   * than creating and caching piles of CSOs for these, Gallium simply streams
   * them out, via the pipe->set_*_state() driver hooks.
   *
   * To reduce draw time overhead, we try to compute as much state at create
   * time as possible.  Wherever possible, we translate the Gallium pipe state
   * to 3DSTATE commands, and store those commands in the CSO.  At draw time,
   * we can simply memcpy them into a batch buffer.
   *
   * No hardware matches the abstraction perfectly, so some commands require
   * information from multiple CSOs.  In this case, we can store two copies
   * of the packet (one in each CSO), and simply | together their DWords at
   * draw time.  Sometimes the second set is trivial (one or two fields), so
   * we simply pack it at draw time.
   *
   * There are two main components in the file below.  First, the CSO hooks
   * create/bind/track state.  The second are the draw-time upload functions,
   * iris_upload_render_state() and iris_upload_compute_state(), which read
   * the context state and emit the commands into the actual batch.
   */
      #include <stdio.h>
   #include <errno.h>
      #ifdef HAVE_VALGRIND
   #include <valgrind.h>
   #include <memcheck.h>
   #define VG(x) x
   #else
   #define VG(x)
   #endif
      #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_dual_blend.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_framebuffer.h"
   #include "util/u_transfer.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_viewport.h"
   #include "util/u_memory.h"
   #include "util/u_trace_gallium.h"
   #include "drm-uapi/i915_drm.h"
   #include "nir.h"
   #include "intel/compiler/brw_compiler.h"
   #include "intel/common/intel_aux_map.h"
   #include "intel/common/intel_l3_config.h"
   #include "intel/common/intel_sample_positions.h"
   #include "intel/ds/intel_tracepoints.h"
   #include "iris_batch.h"
   #include "iris_context.h"
   #include "iris_defines.h"
   #include "iris_pipe.h"
   #include "iris_resource.h"
   #include "iris_utrace.h"
      #include "iris_genx_macros.h"
   #include "intel/common/intel_genX_state.h"
   #include "intel/common/intel_guardband.h"
   #include "intel/common/intel_pixel_hash.h"
      /**
   * Statically assert that PIPE_* enums match the hardware packets.
   * (As long as they match, we don't need to translate them.)
   */
   UNUSED static void pipe_asserts()
   {
   #define PIPE_ASSERT(x) STATIC_ASSERT((int)x)
         /* pipe_logicop happens to match the hardware. */
   PIPE_ASSERT(PIPE_LOGICOP_CLEAR == LOGICOP_CLEAR);
   PIPE_ASSERT(PIPE_LOGICOP_NOR == LOGICOP_NOR);
   PIPE_ASSERT(PIPE_LOGICOP_AND_INVERTED == LOGICOP_AND_INVERTED);
   PIPE_ASSERT(PIPE_LOGICOP_COPY_INVERTED == LOGICOP_COPY_INVERTED);
   PIPE_ASSERT(PIPE_LOGICOP_AND_REVERSE == LOGICOP_AND_REVERSE);
   PIPE_ASSERT(PIPE_LOGICOP_INVERT == LOGICOP_INVERT);
   PIPE_ASSERT(PIPE_LOGICOP_XOR == LOGICOP_XOR);
   PIPE_ASSERT(PIPE_LOGICOP_NAND == LOGICOP_NAND);
   PIPE_ASSERT(PIPE_LOGICOP_AND == LOGICOP_AND);
   PIPE_ASSERT(PIPE_LOGICOP_EQUIV == LOGICOP_EQUIV);
   PIPE_ASSERT(PIPE_LOGICOP_NOOP == LOGICOP_NOOP);
   PIPE_ASSERT(PIPE_LOGICOP_OR_INVERTED == LOGICOP_OR_INVERTED);
   PIPE_ASSERT(PIPE_LOGICOP_COPY == LOGICOP_COPY);
   PIPE_ASSERT(PIPE_LOGICOP_OR_REVERSE == LOGICOP_OR_REVERSE);
   PIPE_ASSERT(PIPE_LOGICOP_OR == LOGICOP_OR);
            /* pipe_blend_func happens to match the hardware. */
   PIPE_ASSERT(PIPE_BLENDFACTOR_ONE == BLENDFACTOR_ONE);
   PIPE_ASSERT(PIPE_BLENDFACTOR_SRC_COLOR == BLENDFACTOR_SRC_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_SRC_ALPHA == BLENDFACTOR_SRC_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_DST_ALPHA == BLENDFACTOR_DST_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_DST_COLOR == BLENDFACTOR_DST_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE == BLENDFACTOR_SRC_ALPHA_SATURATE);
   PIPE_ASSERT(PIPE_BLENDFACTOR_CONST_COLOR == BLENDFACTOR_CONST_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_CONST_ALPHA == BLENDFACTOR_CONST_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_SRC1_COLOR == BLENDFACTOR_SRC1_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_SRC1_ALPHA == BLENDFACTOR_SRC1_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_ZERO == BLENDFACTOR_ZERO);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_SRC_COLOR == BLENDFACTOR_INV_SRC_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_SRC_ALPHA == BLENDFACTOR_INV_SRC_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_DST_ALPHA == BLENDFACTOR_INV_DST_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_DST_COLOR == BLENDFACTOR_INV_DST_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_CONST_COLOR == BLENDFACTOR_INV_CONST_COLOR);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_CONST_ALPHA == BLENDFACTOR_INV_CONST_ALPHA);
   PIPE_ASSERT(PIPE_BLENDFACTOR_INV_SRC1_COLOR == BLENDFACTOR_INV_SRC1_COLOR);
            /* pipe_blend_func happens to match the hardware. */
   PIPE_ASSERT(PIPE_BLEND_ADD == BLENDFUNCTION_ADD);
   PIPE_ASSERT(PIPE_BLEND_SUBTRACT == BLENDFUNCTION_SUBTRACT);
   PIPE_ASSERT(PIPE_BLEND_REVERSE_SUBTRACT == BLENDFUNCTION_REVERSE_SUBTRACT);
   PIPE_ASSERT(PIPE_BLEND_MIN == BLENDFUNCTION_MIN);
            /* pipe_stencil_op happens to match the hardware. */
   PIPE_ASSERT(PIPE_STENCIL_OP_KEEP == STENCILOP_KEEP);
   PIPE_ASSERT(PIPE_STENCIL_OP_ZERO == STENCILOP_ZERO);
   PIPE_ASSERT(PIPE_STENCIL_OP_REPLACE == STENCILOP_REPLACE);
   PIPE_ASSERT(PIPE_STENCIL_OP_INCR == STENCILOP_INCRSAT);
   PIPE_ASSERT(PIPE_STENCIL_OP_DECR == STENCILOP_DECRSAT);
   PIPE_ASSERT(PIPE_STENCIL_OP_INCR_WRAP == STENCILOP_INCR);
   PIPE_ASSERT(PIPE_STENCIL_OP_DECR_WRAP == STENCILOP_DECR);
            /* pipe_sprite_coord_mode happens to match 3DSTATE_SBE */
   PIPE_ASSERT(PIPE_SPRITE_COORD_UPPER_LEFT == UPPERLEFT);
      #undef PIPE_ASSERT
   }
      static unsigned
   translate_prim_type(enum mesa_prim prim, uint8_t verts_per_patch)
   {
      static const unsigned map[] = {
      [MESA_PRIM_POINTS]                   = _3DPRIM_POINTLIST,
   [MESA_PRIM_LINES]                    = _3DPRIM_LINELIST,
   [MESA_PRIM_LINE_LOOP]                = _3DPRIM_LINELOOP,
   [MESA_PRIM_LINE_STRIP]               = _3DPRIM_LINESTRIP,
   [MESA_PRIM_TRIANGLES]                = _3DPRIM_TRILIST,
   [MESA_PRIM_TRIANGLE_STRIP]           = _3DPRIM_TRISTRIP,
   [MESA_PRIM_TRIANGLE_FAN]             = _3DPRIM_TRIFAN,
   [MESA_PRIM_QUADS]                    = _3DPRIM_QUADLIST,
   [MESA_PRIM_QUAD_STRIP]               = _3DPRIM_QUADSTRIP,
   [MESA_PRIM_POLYGON]                  = _3DPRIM_POLYGON,
   [MESA_PRIM_LINES_ADJACENCY]          = _3DPRIM_LINELIST_ADJ,
   [MESA_PRIM_LINE_STRIP_ADJACENCY]     = _3DPRIM_LINESTRIP_ADJ,
   [MESA_PRIM_TRIANGLES_ADJACENCY]      = _3DPRIM_TRILIST_ADJ,
   [MESA_PRIM_TRIANGLE_STRIP_ADJACENCY] = _3DPRIM_TRISTRIP_ADJ,
                  }
      static unsigned
   translate_compare_func(enum pipe_compare_func pipe_func)
   {
      static const unsigned map[] = {
      [PIPE_FUNC_NEVER]    = COMPAREFUNCTION_NEVER,
   [PIPE_FUNC_LESS]     = COMPAREFUNCTION_LESS,
   [PIPE_FUNC_EQUAL]    = COMPAREFUNCTION_EQUAL,
   [PIPE_FUNC_LEQUAL]   = COMPAREFUNCTION_LEQUAL,
   [PIPE_FUNC_GREATER]  = COMPAREFUNCTION_GREATER,
   [PIPE_FUNC_NOTEQUAL] = COMPAREFUNCTION_NOTEQUAL,
   [PIPE_FUNC_GEQUAL]   = COMPAREFUNCTION_GEQUAL,
      };
      }
      static unsigned
   translate_shadow_func(enum pipe_compare_func pipe_func)
   {
      /* Gallium specifies the result of shadow comparisons as:
   *
   *    1 if ref <op> texel,
   *    0 otherwise.
   *
   * The hardware does:
   *
   *    0 if texel <op> ref,
   *    1 otherwise.
   *
   * So we need to flip the operator and also negate.
   */
   static const unsigned map[] = {
      [PIPE_FUNC_NEVER]    = PREFILTEROP_ALWAYS,
   [PIPE_FUNC_LESS]     = PREFILTEROP_LEQUAL,
   [PIPE_FUNC_EQUAL]    = PREFILTEROP_NOTEQUAL,
   [PIPE_FUNC_LEQUAL]   = PREFILTEROP_LESS,
   [PIPE_FUNC_GREATER]  = PREFILTEROP_GEQUAL,
   [PIPE_FUNC_NOTEQUAL] = PREFILTEROP_EQUAL,
   [PIPE_FUNC_GEQUAL]   = PREFILTEROP_GREATER,
      };
      }
      static unsigned
   translate_cull_mode(unsigned pipe_face)
   {
      static const unsigned map[4] = {
      [PIPE_FACE_NONE]           = CULLMODE_NONE,
   [PIPE_FACE_FRONT]          = CULLMODE_FRONT,
   [PIPE_FACE_BACK]           = CULLMODE_BACK,
      };
      }
      static unsigned
   translate_fill_mode(unsigned pipe_polymode)
   {
      static const unsigned map[4] = {
      [PIPE_POLYGON_MODE_FILL]           = FILL_MODE_SOLID,
   [PIPE_POLYGON_MODE_LINE]           = FILL_MODE_WIREFRAME,
   [PIPE_POLYGON_MODE_POINT]          = FILL_MODE_POINT,
      };
      }
      static unsigned
   translate_mip_filter(enum pipe_tex_mipfilter pipe_mip)
   {
      static const unsigned map[] = {
      [PIPE_TEX_MIPFILTER_NEAREST] = MIPFILTER_NEAREST,
   [PIPE_TEX_MIPFILTER_LINEAR]  = MIPFILTER_LINEAR,
      };
      }
      static uint32_t
   translate_wrap(unsigned pipe_wrap)
   {
      static const unsigned map[] = {
      [PIPE_TEX_WRAP_REPEAT]                 = TCM_WRAP,
   [PIPE_TEX_WRAP_CLAMP]                  = TCM_HALF_BORDER,
   [PIPE_TEX_WRAP_CLAMP_TO_EDGE]          = TCM_CLAMP,
   [PIPE_TEX_WRAP_CLAMP_TO_BORDER]        = TCM_CLAMP_BORDER,
   [PIPE_TEX_WRAP_MIRROR_REPEAT]          = TCM_MIRROR,
            /* These are unsupported. */
   [PIPE_TEX_WRAP_MIRROR_CLAMP]           = -1,
      };
      }
      /**
   * Allocate space for some indirect state.
   *
   * Return a pointer to the map (to fill it out) and a state ref (for
   * referring to the state in GPU commands).
   */
   static void *
   upload_state(struct u_upload_mgr *uploader,
               struct iris_state_ref *ref,
   {
      void *p = NULL;
   u_upload_alloc(uploader, 0, size, alignment, &ref->offset, &ref->res, &p);
      }
      /**
   * Stream out temporary/short-lived state.
   *
   * This allocates space, pins the BO, and includes the BO address in the
   * returned offset (which works because all state lives in 32-bit memory
   * zones).
   */
   static uint32_t *
   stream_state(struct iris_batch *batch,
               struct u_upload_mgr *uploader,
   struct pipe_resource **out_res,
   unsigned size,
   {
                        struct iris_bo *bo = iris_resource_bo(*out_res);
            iris_record_state_size(batch->state_sizes,
                        }
      /**
   * stream_state() + memcpy.
   */
   static uint32_t
   emit_state(struct iris_batch *batch,
            struct u_upload_mgr *uploader,
   struct pipe_resource **out_res,
   const void *data,
      {
      unsigned offset = 0;
   uint32_t *map =
            if (map)
               }
      /**
   * Did field 'x' change between 'old_cso' and 'new_cso'?
   *
   * (If so, we may want to set some dirty flags.)
   */
   #define cso_changed(x) (!old_cso || (old_cso->x != new_cso->x))
   #define cso_changed_memcmp(x) \
         #define cso_changed_memcmp_elts(x, n) \
            static void
   flush_before_state_base_change(struct iris_batch *batch)
   {
      /* Wa_14014427904 - We need additional invalidate/flush when
   * emitting NP state commands with ATS-M in compute mode.
   */
   bool atsm_compute = intel_device_info_is_atsm(batch->screen->devinfo) &&
         uint32_t np_state_wa_bits =
      PIPE_CONTROL_CS_STALL |
   PIPE_CONTROL_STATE_CACHE_INVALIDATE |
   PIPE_CONTROL_CONST_CACHE_INVALIDATE |
   PIPE_CONTROL_UNTYPED_DATAPORT_CACHE_FLUSH |
   PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
   PIPE_CONTROL_INSTRUCTION_INVALIDATE |
         /* Flush before emitting STATE_BASE_ADDRESS.
   *
   * This isn't documented anywhere in the PRM.  However, it seems to be
   * necessary prior to changing the surface state base address.  We've
   * seen issues in Vulkan where we get GPU hangs when using multi-level
   * command buffers which clear depth, reset state base address, and then
   * go render stuff.
   *
   * Normally, in GL, we would trust the kernel to do sufficient stalls
   * and flushes prior to executing our batch.  However, it doesn't seem
   * as if the kernel's flushing is always sufficient and we don't want to
   * rely on it.
   *
   * We make this an end-of-pipe sync instead of a normal flush because we
   * do not know the current status of the GPU.  On Haswell at least,
   * having a fast-clear operation in flight at the same time as a normal
   * rendering operation can cause hangs.  Since the kernel's flushing is
   * insufficient, we need to ensure that any rendering operations from
   * other processes are definitely complete before we try to do our own
   * rendering.  It's a bit of a big hammer but it appears to work.
   */
   iris_emit_end_of_pipe_sync(batch,
                              }
      static void
   flush_after_state_base_change(struct iris_batch *batch)
   {
      const struct intel_device_info *devinfo = batch->screen->devinfo;
   /* After re-setting the surface state base address, we have to do some
   * cache flusing so that the sampler engine will pick up the new
   * SURFACE_STATE objects and binding tables. From the Broadwell PRM,
   * Shared Function > 3D Sampler > State > State Caching (page 96):
   *
   *    Coherency with system memory in the state cache, like the texture
   *    cache is handled partially by software. It is expected that the
   *    command stream or shader will issue Cache Flush operation or
   *    Cache_Flush sampler message to ensure that the L1 cache remains
   *    coherent with system memory.
   *
   *    [...]
   *
   *    Whenever the value of the Dynamic_State_Base_Addr,
   *    Surface_State_Base_Addr are altered, the L1 state cache must be
   *    invalidated to ensure the new surface or sampler state is fetched
   *    from system memory.
   *
   * The PIPE_CONTROL command has a "State Cache Invalidation Enable" bit
   * which, according the PIPE_CONTROL instruction documentation in the
   * Broadwell PRM:
   *
   *    Setting this bit is independent of any other bit in this packet.
   *    This bit controls the invalidation of the L1 and L2 state caches
   *    at the top of the pipe i.e. at the parsing time.
   *
   * Unfortunately, experimentation seems to indicate that state cache
   * invalidation through a PIPE_CONTROL does nothing whatsoever in
   * regards to surface state and binding tables.  In stead, it seems that
   * invalidating the texture cache is what is actually needed.
   *
   * XXX:  As far as we have been able to determine through
   * experimentation, shows that flush the texture cache appears to be
   * sufficient.  The theory here is that all of the sampling/rendering
   * units cache the binding table in the texture cache.  However, we have
   * yet to be able to actually confirm this.
   *
   * Wa_16013000631:
   *
   *  "DG2 128/256/512-A/B: S/W must program STATE_BASE_ADDRESS command twice
   *   or program pipe control with Instruction cache invalidate post
   *   STATE_BASE_ADDRESS command"
   */
   iris_emit_end_of_pipe_sync(batch,
                              "change STATE_BASE_ADDRESS (invalidates)",
   }
      static void
   iris_load_register_reg32(struct iris_batch *batch, uint32_t dst,
         {
      struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
      }
      static void
   iris_load_register_reg64(struct iris_batch *batch, uint32_t dst,
         {
      struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
      }
      static void
   iris_load_register_imm32(struct iris_batch *batch, uint32_t reg,
         {
      struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
      }
      static void
   iris_load_register_imm64(struct iris_batch *batch, uint32_t reg,
         {
      struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
      }
      /**
   * Emit MI_LOAD_REGISTER_MEM to load a 32-bit MMIO register from a buffer.
   */
   static void
   iris_load_register_mem32(struct iris_batch *batch, uint32_t reg,
         {
      iris_batch_sync_region_start(batch);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value src = mi_mem32(ro_bo(bo, offset));
   mi_store(&b, mi_reg32(reg), src);
      }
      /**
   * Load a 64-bit value from a buffer into a MMIO register via
   * two MI_LOAD_REGISTER_MEM commands.
   */
   static void
   iris_load_register_mem64(struct iris_batch *batch, uint32_t reg,
         {
      iris_batch_sync_region_start(batch);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value src = mi_mem64(ro_bo(bo, offset));
   mi_store(&b, mi_reg64(reg), src);
      }
      static void
   iris_store_register_mem32(struct iris_batch *batch, uint32_t reg,
               {
      iris_batch_sync_region_start(batch);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value dst = mi_mem32(rw_bo(bo, offset, IRIS_DOMAIN_OTHER_WRITE));
   struct mi_value src = mi_reg32(reg);
   if (predicated)
         else
            }
      static void
   iris_store_register_mem64(struct iris_batch *batch, uint32_t reg,
               {
      iris_batch_sync_region_start(batch);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value dst = mi_mem64(rw_bo(bo, offset, IRIS_DOMAIN_OTHER_WRITE));
   struct mi_value src = mi_reg64(reg);
   if (predicated)
         else
            }
      static void
   iris_store_data_imm32(struct iris_batch *batch,
               {
      iris_batch_sync_region_start(batch);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value dst = mi_mem32(rw_bo(bo, offset, IRIS_DOMAIN_OTHER_WRITE));
   struct mi_value src = mi_imm(imm);
   mi_store(&b, dst, src);
      }
      static void
   iris_store_data_imm64(struct iris_batch *batch,
               {
      iris_batch_sync_region_start(batch);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value dst = mi_mem64(rw_bo(bo, offset, IRIS_DOMAIN_OTHER_WRITE));
   struct mi_value src = mi_imm(imm);
   mi_store(&b, dst, src);
      }
      static void
   iris_copy_mem_mem(struct iris_batch *batch,
                     {
      /* MI_COPY_MEM_MEM operates on DWords. */
   assert(bytes % 4 == 0);
   assert(dst_offset % 4 == 0);
   assert(src_offset % 4 == 0);
            for (unsigned i = 0; i < bytes; i += 4) {
      iris_emit_cmd(batch, GENX(MI_COPY_MEM_MEM), cp) {
      cp.DestinationMemoryAddress = rw_bo(dst_bo, dst_offset + i,
                           }
      static void
   iris_rewrite_compute_walker_pc(struct iris_batch *batch,
                     {
   #if GFX_VERx10 >= 125
      struct iris_screen *screen = batch->screen;
                     _iris_pack_command(batch, GENX(COMPUTE_WALKER), dwords, cw) {
      cw.PostSync.Operation          = WriteTimestamp;
   cw.PostSync.DestinationAddress = addr;
               for (uint32_t i = 0; i < GENX(COMPUTE_WALKER_length); i++)
      #else
         #endif
   }
      static void
   emit_pipeline_select(struct iris_batch *batch, uint32_t pipeline)
   {
   #if GFX_VER >= 8 && GFX_VER < 10
      /* From the Broadwell PRM, Volume 2a: Instructions, PIPELINE_SELECT:
   *
   *   Software must clear the COLOR_CALC_STATE Valid field in
   *   3DSTATE_CC_STATE_POINTERS command prior to send a PIPELINE_SELECT
   *   with Pipeline Select set to GPGPU.
   *
   * The internal hardware docs recommend the same workaround for Gfx9
   * hardware too.
   */
   if (pipeline == GPGPU)
      #endif
      #if GFX_VER >= 12
      /* From Tigerlake PRM, Volume 2a, PIPELINE_SELECT:
   *
   *   "Software must ensure Render Cache, Depth Cache and HDC Pipeline flush
   *   are flushed through a stalling PIPE_CONTROL command prior to
   *   programming of PIPELINE_SELECT command transitioning Pipeline Select
   *   from 3D to GPGPU/Media.
   *   Software must ensure HDC Pipeline flush and Generic Media State Clear
   *   is issued through a stalling PIPE_CONTROL command prior to programming
   *   of PIPELINE_SELECT command transitioning Pipeline Select from
   *   GPGPU/Media to 3D."
   *
   * Note: Issuing PIPE_CONTROL_MEDIA_STATE_CLEAR causes GPU hangs, probably
   * because PIPE was not in MEDIA mode?!
   */
   enum pipe_control_flags flags = PIPE_CONTROL_CS_STALL |
            if (pipeline == GPGPU && batch->name == IRIS_BATCH_RENDER) {
      flags |= PIPE_CONTROL_RENDER_TARGET_FLUSH |
      } else {
         }
   /* Wa_16013063087 -  State Cache Invalidate must be issued prior to
   * PIPELINE_SELECT when switching from 3D to Compute.
   *
   * SW must do this by programming of PIPECONTROL with “CS Stall” followed
   * by a PIPECONTROL with State Cache Invalidate bit set.
   */
   if (pipeline == GPGPU &&
      intel_needs_workaround(batch->screen->devinfo, 16013063087))
            #else
      /* From "BXML » GT » MI » vol1a GPU Overview » [Instruction]
   * PIPELINE_SELECT [DevBWR+]":
   *
   *    "Project: DEVSNB+
   *
   *     Software must ensure all the write caches are flushed through a
   *     stalling PIPE_CONTROL command followed by another PIPE_CONTROL
   *     command to invalidate read only caches prior to programming
   *     MI_PIPELINE_SELECT command to change the Pipeline Select Mode."
   */
   iris_emit_pipe_control_flush(batch,
                                          iris_emit_pipe_control_flush(batch,
                              #endif
            #if GFX_VER >= 9
         sel.MaskBits = GFX_VER >= 12 ? 0x13 : 3;
   #endif
               }
      UNUSED static void
   init_glk_barrier_mode(struct iris_batch *batch, uint32_t value)
   {
   #if GFX_VER == 9
      /* Project: DevGLK
   *
   *    "This chicken bit works around a hardware issue with barrier
   *     logic encountered when switching between GPGPU and 3D pipelines.
   *     To workaround the issue, this mode bit should be set after a
   *     pipeline is selected."
   */
   iris_emit_reg(batch, GENX(SLICE_COMMON_ECO_CHICKEN1), reg) {
      reg.GLKBarrierMode = value;
         #endif
   }
      static void
   init_state_base_address(struct iris_batch *batch)
   {
      struct isl_device *isl_dev = &batch->screen->isl_dev;
   uint32_t mocs = isl_mocs(isl_dev, 0, false);
            /* We program most base addresses once at context initialization time.
   * Each base address points at a 4GB memory zone, and never needs to
   * change.  See iris_bufmgr.h for a description of the memory zones.
   *
   * The one exception is Surface State Base Address, which needs to be
   * updated occasionally.  See iris_binder.c for the details there.
   */
   iris_emit_cmd(batch, GENX(STATE_BASE_ADDRESS), sba) {
      sba.GeneralStateMOCS            = mocs;
   sba.StatelessDataPortAccessMOCS = mocs;
   sba.DynamicStateMOCS            = mocs;
   sba.IndirectObjectMOCS          = mocs;
   sba.InstructionMOCS             = mocs;
   #if GFX_VER >= 9
         #endif
            sba.GeneralStateBaseAddressModifyEnable   = true;
   sba.DynamicStateBaseAddressModifyEnable   = true;
   sba.IndirectObjectBaseAddressModifyEnable = true;
   sba.InstructionBaseAddressModifyEnable    = true;
   sba.GeneralStateBufferSizeModifyEnable    = true;
   sba.DynamicStateBufferSizeModifyEnable    = true;
   #if GFX_VER >= 11
         #endif
         sba.IndirectObjectBufferSizeModifyEnable  = true;
            sba.InstructionBaseAddress  = ro_bo(NULL, IRIS_MEMZONE_SHADER_START);
   sba.DynamicStateBaseAddress = ro_bo(NULL, IRIS_MEMZONE_DYNAMIC_START);
            sba.GeneralStateBufferSize   = 0xfffff;
   sba.IndirectObjectBufferSize = 0xfffff;
   sba.InstructionBufferSize    = 0xfffff;
   #if GFX_VERx10 >= 125
         #endif
                  }
      static void
   iris_emit_l3_config(struct iris_batch *batch,
         {
            #if GFX_VER >= 12
   #define L3_ALLOCATION_REG GENX(L3ALLOC)
   #define L3_ALLOCATION_REG_num GENX(L3ALLOC_num)
   #else
   #define L3_ALLOCATION_REG GENX(L3CNTLREG)
   #define L3_ALLOCATION_REG_num GENX(L3CNTLREG_num)
   #endif
            #if GFX_VER < 11
         #endif
   #if GFX_VER == 11
         /* Wa_1406697149: Bit 9 "Error Detection Behavior Control" must be set
   * in L3CNTLREG register. The default setting of the bit is not the
   * desirable behavior.
   */
   reg.ErrorDetectionBehaviorControl = true;
   #endif
         if (GFX_VER < 12 || cfg) {
      reg.URBAllocation = cfg->n[INTEL_L3P_URB];
   reg.ROAllocation = cfg->n[INTEL_L3P_RO];
   reg.DCAllocation = cfg->n[INTEL_L3P_DC];
      #if GFX_VER >= 12
         #endif
               }
      #if GFX_VER == 9
   static void
   iris_enable_obj_preemption(struct iris_batch *batch, bool enable)
   {
      /* A fixed function pipe flush is required before modifying this field */
   iris_emit_end_of_pipe_sync(batch, enable ? "enable preemption"
                  /* enable object level preemption */
   iris_emit_reg(batch, GENX(CS_CHICKEN1), reg) {
      reg.ReplayMode = enable;
         }
   #endif
      static void
   upload_pixel_hashing_tables(struct iris_batch *batch)
   {
      UNUSED const struct intel_device_info *devinfo = batch->screen->devinfo;
   UNUSED struct iris_context *ice = batch->ice;
         #if GFX_VER == 11
      /* Gfx11 hardware has two pixel pipes at most. */
   for (unsigned i = 2; i < ARRAY_SIZE(devinfo->ppipe_subslices); i++)
            if (devinfo->ppipe_subslices[0] == devinfo->ppipe_subslices[1])
            unsigned size = GENX(SLICE_HASH_TABLE_length) * 4;
   uint32_t hash_address;
   struct pipe_resource *tmp = NULL;
   uint32_t *map =
      stream_state(batch, ice->state.dynamic_uploader, &tmp,
               const bool flip = devinfo->ppipe_subslices[0] < devinfo->ppipe_subslices[1];
   struct GENX(SLICE_HASH_TABLE) table;
                     iris_emit_cmd(batch, GENX(3DSTATE_SLICE_TABLE_STATE_POINTERS), ptr) {
      ptr.SliceHashStatePointerValid = true;
               iris_emit_cmd(batch, GENX(3DSTATE_3D_MODE), mode) {
               #elif GFX_VERx10 == 120
      /* For each n calculate ppipes_of[n], equal to the number of pixel pipes
   * present with n active dual subslices.
   */
            for (unsigned n = 0; n < ARRAY_SIZE(ppipes_of); n++) {
      for (unsigned p = 0; p < 3; p++)
               /* Gfx12 has three pixel pipes. */
   for (unsigned p = 3; p < ARRAY_SIZE(devinfo->ppipe_subslices); p++)
            if (ppipes_of[2] == 3 || ppipes_of[0] == 2) {
      /* All three pixel pipes have the maximum number of active dual
   * subslices, or there is only one active pixel pipe: Nothing to do.
   */
               iris_emit_cmd(batch, GENX(3DSTATE_SUBSLICE_HASH_TABLE), p) {
               if (ppipes_of[2] == 2 && ppipes_of[0] == 1)
         else if (ppipes_of[2] == 1 && ppipes_of[1] == 1 && ppipes_of[0] == 1)
            if (ppipes_of[2] == 2 && ppipes_of[1] == 1)
         else if (ppipes_of[2] == 2 && ppipes_of[0] == 1)
         else if (ppipes_of[2] == 1 && ppipes_of[1] == 1 && ppipes_of[0] == 1)
         else
               iris_emit_cmd(batch, GENX(3DSTATE_3D_MODE), p) {
      p.SubsliceHashingTableEnable = true;
            #elif GFX_VERx10 == 125
      struct pipe_screen *pscreen = &batch->screen->base;
   const unsigned size = GENX(SLICE_HASH_TABLE_length) * 4;
   const struct pipe_resource tmpl = {
   .target = PIPE_BUFFER,
   .format = PIPE_FORMAT_R8_UNORM,
   .bind = PIPE_BIND_CUSTOM,
   .usage = PIPE_USAGE_IMMUTABLE,
   .flags = IRIS_RESOURCE_FLAG_DYNAMIC_MEMZONE,
   .width0 = size,
   .height0 = 1,
   .depth0 = 1,
   .array_size = 1
            pipe_resource_reference(&ice->state.pixel_hashing_tables, NULL);
            struct iris_resource *res = (struct iris_resource *)ice->state.pixel_hashing_tables;
   struct pipe_transfer *transfer = NULL;
   uint32_t *map = pipe_buffer_map_range(&ice->ctx, ice->state.pixel_hashing_tables,
                  uint32_t ppipe_mask = 0;
   for (unsigned p = 0; p < ARRAY_SIZE(devinfo->ppipe_subslices); p++) {
      if (devinfo->ppipe_subslices[p])
      }
                     /* Note that the hardware expects an array with 7 tables, each
   * table is intended to specify the pixel pipe hashing behavior for
   * every possible slice count between 2 and 8, however that doesn't
   * actually work, among other reasons due to hardware bugs that
   * will cause the GPU to erroneously access the table at the wrong
   * index in some cases, so in practice all 7 tables need to be
   * initialized to the same value.
   */
   for (unsigned i = 0; i < 7; i++)
                              iris_use_pinned_bo(batch, res->bo, false, IRIS_DOMAIN_NONE);
            iris_emit_cmd(batch, GENX(3DSTATE_SLICE_TABLE_STATE_POINTERS), ptr) {
      ptr.SliceHashStatePointerValid = true;
   ptr.SliceHashTableStatePointer = iris_bo_offset_from_base_address(res->bo) +
               iris_emit_cmd(batch, GENX(3DSTATE_3D_MODE), mode) {
      mode.SliceHashingTableEnable = true;
   mode.SliceHashingTableEnableMask = true;
   mode.CrossSliceHashingMode = (util_bitcount(ppipe_mask) > 1 ?
               #endif
   }
      static void
   iris_alloc_push_constants(struct iris_batch *batch)
   {
               /* For now, we set a static partitioning of the push constant area,
   * assuming that all stages could be in use.
   *
   * TODO: Try lazily allocating the HS/DS/GS sections as needed, and
   *       see if that improves performance by offering more space to
   *       the VS/FS when those aren't in use.  Also, try dynamically
   *       enabling/disabling it like i965 does.  This would be more
   *       stalls and may not actually help; we don't know yet.
            /* Divide as equally as possible with any remainder given to FRAGMENT. */
   const unsigned push_constant_kb = devinfo->max_constant_urb_size_kb;
   const unsigned stage_size = push_constant_kb / 5;
            for (int i = 0; i <= MESA_SHADER_FRAGMENT; i++) {
      iris_emit_cmd(batch, GENX(3DSTATE_PUSH_CONSTANT_ALLOC_VS), alloc) {
      alloc._3DCommandSubOpcode = 18 + i;
   alloc.ConstantBufferOffset = stage_size * i;
               #if GFX_VERx10 == 125
      /* DG2: Wa_22011440098
   * MTL: Wa_18022330953
   *
   * In 3D mode, after programming push constant alloc command immediately
   * program push constant command(ZERO length) without any commit between
   * them.
   */
   iris_emit_cmd(batch, GENX(3DSTATE_CONSTANT_ALL), c) {
      /* Update empty push constants for all stages (bitmask = 11111b) */
   c.ShaderUpdateEnable = 0x1f;
         #endif
   }
      #if GFX_VER >= 12
   static void
   init_aux_map_state(struct iris_batch *batch);
   #endif
      /* This updates a register. Caller should stall the pipeline as needed. */
   static void
   iris_disable_rhwo_optimization(struct iris_batch *batch, bool disable)
   {
         #if GFX_VERx10 == 120
      iris_emit_reg(batch, GENX(COMMON_SLICE_CHICKEN1), c1) {
      c1.RCCRHWOOptimizationDisable = disable;
         #endif
   }
      /**
   * Upload initial GPU state for any kind of context.
   *
   * These need to happen for both render and compute.
   */
   static void
   iris_init_common_context(struct iris_batch *batch)
   {
   #if GFX_VER == 11
      iris_emit_reg(batch, GENX(SAMPLER_MODE), reg) {
      reg.HeaderlessMessageforPreemptableContexts = 1;
               /* Bit 1 must be set in HALF_SLICE_CHICKEN7. */
   iris_emit_reg(batch, GENX(HALF_SLICE_CHICKEN7), reg) {
      reg.EnabledTexelOffsetPrecisionFix = 1;
         #endif
         /* Select 256B-aligned binding table mode on Icelake through Tigerlake,
   * which gives us larger binding table pointers, at the cost of higher
   * alignment requirements (bits 18:8 are valid instead of 15:5).  When
   * using this mode, we have to shift binding table pointers by 3 bits,
   * as they're still stored in the same bit-location in the field.
      #if GFX_VER >= 11 && GFX_VERx10 < 125
      iris_emit_reg(batch, GENX(GT_MODE), reg) {
      reg.BindingTableAlignment = BTP_18_8;
         #define IRIS_BT_OFFSET_SHIFT 3
   #else
   #define IRIS_BT_OFFSET_SHIFT 0
   #endif
      #if GFX_VERx10 == 125
      /* Even though L3 partial write merging is supposed to be enabled
   * by default on Gfx12.5 according to the hardware spec, i915
   * appears to accidentally clear the enables during context
   * initialization, so make sure to enable them here since partial
   * write merging has a large impact on rendering performance.
   */
   iris_emit_reg(batch, GENX(L3SQCREG5), reg) {
      reg.L3CachePartialWriteMergeTimerInitialValue = 0x7f;
   reg.CompressiblePartialWriteMergeEnable = true;
   reg.CoherentPartialWriteMergeEnable = true;
         #endif
   }
      static void
   toggle_protected(struct iris_batch *batch)
   {
               if (batch->name == IRIS_BATCH_RENDER)
         else if (batch->name == IRIS_BATCH_COMPUTE)
         else
            if (!ice->protected)
         #if GFX_VER >= 12
      iris_emit_cmd(batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
   pc.RenderTargetCacheFlushEnable = true;
      }
   iris_emit_cmd(batch, GENX(MI_SET_APPID), appid) {
      /* Default value for single session. */
   appid.ProtectedMemoryApplicationID = 0xf;
      }
   iris_emit_cmd(batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
   pc.RenderTargetCacheFlushEnable = true;
         #else
         #endif
   }
      /**
   * Upload the initial GPU state for a render context.
   *
   * This sets some invariant state that needs to be programmed a particular
   * way, but we never actually change.
   */
   static void
   iris_init_render_context(struct iris_batch *batch)
   {
                                                                  #if GFX_VER >= 9
      iris_emit_reg(batch, GENX(CS_DEBUG_MODE2), reg) {
      reg.CONSTANT_BUFFERAddressOffsetDisable = true;
         #else
      iris_emit_reg(batch, GENX(INSTPM), reg) {
      reg.CONSTANT_BUFFERAddressOffsetDisable = true;
         #endif
      #if GFX_VER == 9
      iris_emit_reg(batch, GENX(CACHE_MODE_1), reg) {
      reg.FloatBlendOptimizationEnable = true;
   reg.FloatBlendOptimizationEnableMask = true;
   reg.MSCRAWHazardAvoidanceBit = true;
   reg.MSCRAWHazardAvoidanceBitMask = true;
   reg.PartialResolveDisableInVC = true;
               if (devinfo->platform == INTEL_PLATFORM_GLK)
      #endif
      #if GFX_VER == 11
      iris_emit_reg(batch, GENX(TCCNTLREG), reg) {
      reg.L3DataPartialWriteMergingEnable = true;
   reg.ColorZPartialWriteMergingEnable = true;
   reg.URBPartialWriteMergingEnable = true;
               /* Hardware specification recommends disabling repacking for the
   * compatibility with decompression mechanism in display controller.
   */
   if (devinfo->disable_ccs_repack) {
      iris_emit_reg(batch, GENX(CACHE_MODE_0), reg) {
      reg.DisableRepackingforCompression = true;
            #endif
      #if GFX_VER == 12
      iris_emit_reg(batch, GENX(FF_MODE2), reg) {
      /* On Alchemist, the FF_MODE2 docs for the GS timer say:
   *
   *    "The timer value must be set to 224."
   *
   * and Wa_16011163337 indicates this is the case for all Gfx12 parts,
   * and that this is necessary to avoid hanging the HS/DS units.  It
   * also clarifies that 224 is literally 0xE0 in the bits, not 7*32=224.
   *
   * The HS timer docs also have the same quote for Alchemist.  I am
   * unaware of a reason it needs to be set to 224 on Tigerlake, but
   * we do so for consistency if nothing else.
   *
   * For the TDS timer value, the docs say:
   *
   *    "For best performance, a value of 4 should be programmed."
   *
   * i915 also sets it this way on Tigerlake due to workarounds.
   *
   * The default VS timer appears to be 0, so we leave it at that.
   */
   reg.GSTimerValue  = 224;
   reg.HSTimerValue  = 224;
   reg.TDSTimerValue = 4;
         #endif
      #if INTEL_NEEDS_WA_1508744258
      /* The suggested workaround is:
   *
   *    Disable RHWO by setting 0x7010[14] by default except during resolve
   *    pass.
   *
   * We implement global disabling of the optimization here and we toggle it
   * in iris_resolve_color.
   *
   * iris_init_compute_context is unmodified because we don't expect to
   * access the RCC in the compute context. iris_mcs_partial_resolve is
   * unmodified because that pass doesn't use a HW bit to perform the
   * resolve (related HSDs specifically call out the RenderTargetResolveType
   * field in the 3DSTATE_PS instruction).
   */
      #endif
      #if GFX_VERx10 == 120
      /* Wa_1806527549 says to disable the following HiZ optimization when the
   * depth buffer is D16_UNORM. We've found the WA to help with more depth
   * buffer configurations however, so we always disable it just to be safe.
   */
   iris_emit_reg(batch, GENX(HIZ_CHICKEN), reg) {
      reg.HZDepthTestLEGEOptimizationDisable = true;
         #endif
                  /* 3DSTATE_DRAWING_RECTANGLE is non-pipelined, so we want to avoid
   * changing it dynamically.  We set it to the maximum size here, and
   * instead include the render target dimensions in the viewport, so
   * viewport extents clipping takes care of pruning stray geometry.
   */
   iris_emit_cmd(batch, GENX(3DSTATE_DRAWING_RECTANGLE), rect) {
      rect.ClippedDrawingRectangleXMax = UINT16_MAX;
               /* Set the initial MSAA sample positions. */
   iris_emit_cmd(batch, GENX(3DSTATE_SAMPLE_PATTERN), pat) {
      INTEL_SAMPLE_POS_1X(pat._1xSample);
   INTEL_SAMPLE_POS_2X(pat._2xSample);
   INTEL_SAMPLE_POS_4X(pat._4xSample);
   #if GFX_VER >= 9
         #endif
               /* Use the legacy AA line coverage computation. */
            /* Disable chromakeying (it's for media) */
            /* We want regular rendering, not special HiZ operations. */
            /* No polygon stippling offsets are necessary. */
   /* TODO: may need to set an offset for origin-UL framebuffers */
         #if GFX_VERx10 >= 125
      iris_emit_cmd(batch, GENX(3DSTATE_MESH_CONTROL), foo);
      #endif
                  #if GFX_VER >= 12
         #endif
            }
      static void
   iris_init_compute_context(struct iris_batch *batch)
   {
                        /* Wa_1607854226:
   *
   *  Start with pipeline in 3D mode to set the STATE_BASE_ADDRESS.
      #if GFX_VERx10 == 120
         #else
         #endif
                                          #if GFX_VERx10 == 120
         #endif
      #if GFX_VER == 9
      if (devinfo->platform == INTEL_PLATFORM_GLK)
      #endif
      #if GFX_VER >= 12
         #endif
      #if GFX_VERx10 >= 125
      iris_emit_cmd(batch, GENX(CFE_STATE), cfe) {
      cfe.MaximumNumberofThreads =
         #endif
            }
      struct iris_vertex_buffer_state {
      /** The VERTEX_BUFFER_STATE hardware structure. */
            /** The resource to source vertex data from. */
               };
      struct iris_depth_buffer_state {
      /* Depth/HiZ/Stencil related hardware packets. */
   uint32_t packets[GENX(3DSTATE_DEPTH_BUFFER_length) +
                  };
      #if INTEL_NEEDS_WA_1808121037
   enum iris_depth_reg_mode {
      IRIS_DEPTH_REG_MODE_HW_DEFAULT = 0,
   IRIS_DEPTH_REG_MODE_D16_1X_MSAA,
      };
   #endif
      /**
   * Generation-specific context state (ice->state.genx->...).
   *
   * Most state can go in iris_context directly, but these encode hardware
   * packets which vary by generation.
   */
   struct iris_genx_state {
      struct iris_vertex_buffer_state vertex_buffers[33];
                           #if GFX_VER == 8
         #endif
         /* Is object level preemption enabled? */
         #if INTEL_NEEDS_WA_1808121037
         #endif
            #if GFX_VER == 8
         #endif
         };
      /**
   * The pipe->set_blend_color() driver hook.
   *
   * This corresponds to our COLOR_CALC_STATE.
   */
   static void
   iris_set_blend_color(struct pipe_context *ctx,
         {
               /* Our COLOR_CALC_STATE is exactly pipe_blend_color, so just memcpy */
   memcpy(&ice->state.blend_color, state, sizeof(struct pipe_blend_color));
      }
      /**
   * Gallium CSO for blend state (see pipe_blend_state).
   */
   struct iris_blend_state {
      /** Partial 3DSTATE_PS_BLEND */
            /** Partial BLEND_STATE */
   uint32_t blend_state[GENX(BLEND_STATE_length) +
                     /** Bitfield of whether blending is enabled for RT[i] - for aux resolves */
            /** Bitfield of whether color writes are enabled for RT[i] */
            /** Does RT[0] use dual color blending? */
            int ps_dst_blend_factor[BRW_MAX_DRAW_BUFFERS];
      };
      static enum pipe_blendfactor
   fix_blendfactor(enum pipe_blendfactor f, bool alpha_to_one)
   {
      if (alpha_to_one) {
      if (f == PIPE_BLENDFACTOR_SRC1_ALPHA)
            if (f == PIPE_BLENDFACTOR_INV_SRC1_ALPHA)
                  }
      /**
   * The pipe->create_blend_state() driver hook.
   *
   * Translates a pipe_blend_state into iris_blend_state.
   */
   static void *
   iris_create_blend_state(struct pipe_context *ctx,
         {
      struct iris_blend_state *cso = malloc(sizeof(struct iris_blend_state));
            cso->blend_enables = 0;
   cso->color_write_enables = 0;
                              for (int i = 0; i < BRW_MAX_DRAW_BUFFERS; i++) {
      const struct pipe_rt_blend_state *rt =
            enum pipe_blendfactor src_rgb =
         enum pipe_blendfactor src_alpha =
         enum pipe_blendfactor dst_rgb =
         enum pipe_blendfactor dst_alpha =
            /* Stored separately in cso for dynamic emission. */
   cso->ps_dst_blend_factor[i] = (int) dst_rgb;
            if (rt->rgb_func != rt->alpha_func ||
                  if (rt->blend_enable)
            if (rt->colormask)
            iris_pack_state(GENX(BLEND_STATE_ENTRY), blend_entry, be) {
                     be.PreBlendSourceOnlyClampEnable = false;
   be.ColorClampRange = COLORCLAMP_RTFORMAT;
                                          /* The casts prevent warnings about implicit enum type conversions. */
                  be.WriteDisableRed   = !(rt->colormask & PIPE_MASK_R);
   be.WriteDisableGreen = !(rt->colormask & PIPE_MASK_G);
   be.WriteDisableBlue  = !(rt->colormask & PIPE_MASK_B);
      }
               iris_pack_command(GENX(3DSTATE_PS_BLEND), cso->ps_blend, pb) {
      /* pb.HasWriteableRT is filled in at draw time.
   * pb.AlphaTestEnable is filled in at draw time.
   *
   * pb.ColorBufferBlendEnable is filled in at draw time so we can avoid
   * setting it when dual color blending without an appropriate shader.
            pb.AlphaToCoverageEnable = state->alpha_to_coverage;
            /* The casts prevent warnings about implicit enum type conversions. */
   pb.SourceBlendFactor =
         pb.SourceAlphaBlendFactor =
               iris_pack_state(GENX(BLEND_STATE), cso->blend_state, bs) {
      bs.AlphaToCoverageEnable = state->alpha_to_coverage;
   bs.IndependentAlphaBlendEnable = indep_alpha_blend;
   bs.AlphaToOneEnable = state->alpha_to_one;
   bs.AlphaToCoverageDitherEnable = state->alpha_to_coverage_dither;
   bs.ColorDitherEnable = state->dither;
                           }
      /**
   * The pipe->bind_blend_state() driver hook.
   *
   * Bind a blending CSO and flag related dirty bits.
   */
   static void
   iris_bind_blend_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *) ctx;
                     ice->state.dirty |= IRIS_DIRTY_PS_BLEND;
   ice->state.dirty |= IRIS_DIRTY_BLEND_STATE;
            if (GFX_VER == 8)
      }
      /**
   * Return true if the FS writes to any color outputs which are not disabled
   * via color masking.
   */
   static bool
   has_writeable_rt(const struct iris_blend_state *cso_blend,
         {
      if (!fs_info)
                     if (fs_info->outputs_written & BITFIELD64_BIT(FRAG_RESULT_COLOR))
               }
      /**
   * Gallium CSO for depth, stencil, and alpha testing state.
   */
   struct iris_depth_stencil_alpha_state {
      /** Partial 3DSTATE_WM_DEPTH_STENCIL. */
         #if GFX_VER >= 12
         #endif
         /** Outbound to BLEND_STATE, 3DSTATE_PS_BLEND, COLOR_CALC_STATE. */
   unsigned alpha_enabled:1;
   unsigned alpha_func:3;     /**< PIPE_FUNC_x */
            /** Outbound to resolve and cache set tracking. */
   bool depth_writes_enabled;
            /** Outbound to Gfx8-9 PMA stall equations */
            /** Tracking state of DS writes for Wa_18019816803. */
      };
      /**
   * The pipe->create_depth_stencil_alpha_state() driver hook.
   *
   * We encode most of 3DSTATE_WM_DEPTH_STENCIL, and just save off the alpha
   * testing state since we need pieces of it in a variety of places.
   */
   static void *
   iris_create_zsa_state(struct pipe_context *ctx,
         {
      struct iris_depth_stencil_alpha_state *cso =
                     bool depth_write_enabled = false;
            /* Depth writes enabled? */
   if (state->depth_writemask &&
      ((!state->depth_enabled) ||
   ((state->depth_func != PIPE_FUNC_NEVER) &&
   (state->depth_func != PIPE_FUNC_EQUAL))))
         bool stencil_all_keep =
      state->stencil[0].fail_op == PIPE_STENCIL_OP_KEEP &&
   state->stencil[0].zfail_op == PIPE_STENCIL_OP_KEEP &&
   state->stencil[0].zpass_op == PIPE_STENCIL_OP_KEEP &&
   (!two_sided_stencil ||
   (state->stencil[1].fail_op == PIPE_STENCIL_OP_KEEP &&
   state->stencil[1].zfail_op == PIPE_STENCIL_OP_KEEP &&
         bool stencil_mask_zero =
      state->stencil[0].writemask == 0 ||
         bool stencil_func_never =
      state->stencil[0].func == PIPE_FUNC_NEVER &&
   state->stencil[0].fail_op == PIPE_STENCIL_OP_KEEP &&
   (!two_sided_stencil ||
   (state->stencil[1].func == PIPE_FUNC_NEVER &&
         /* Stencil writes enabled? */
   if (state->stencil[0].writemask != 0 ||
      ((two_sided_stencil && state->stencil[1].writemask != 0) &&
   (!stencil_all_keep &&
   !stencil_mask_zero &&
   !stencil_func_never)))
                  cso->alpha_enabled = state->alpha_enabled;
   cso->alpha_func = state->alpha_func;
   cso->alpha_ref_value = state->alpha_ref_value;
   cso->depth_writes_enabled = state->depth_writemask;
   cso->depth_test_enabled = state->depth_enabled;
   cso->stencil_writes_enabled =
      state->stencil[0].writemask != 0 ||
         /* gallium frontends need to optimize away EQUAL writes for us. */
            iris_pack_command(GENX(3DSTATE_WM_DEPTH_STENCIL), cso->wmds, wmds) {
      wmds.StencilFailOp = state->stencil[0].fail_op;
   wmds.StencilPassDepthFailOp = state->stencil[0].zfail_op;
   wmds.StencilPassDepthPassOp = state->stencil[0].zpass_op;
   wmds.StencilTestFunction =
         wmds.BackfaceStencilFailOp = state->stencil[1].fail_op;
   wmds.BackfaceStencilPassDepthFailOp = state->stencil[1].zfail_op;
   wmds.BackfaceStencilPassDepthPassOp = state->stencil[1].zpass_op;
   wmds.BackfaceStencilTestFunction =
         wmds.DepthTestFunction = translate_compare_func(state->depth_func);
   wmds.DoubleSidedStencilEnable = two_sided_stencil;
   wmds.StencilTestEnable = state->stencil[0].enabled;
   wmds.StencilBufferWriteEnable =
      state->stencil[0].writemask != 0 ||
      wmds.DepthTestEnable = state->depth_enabled;
   wmds.DepthBufferWriteEnable = state->depth_writemask;
   wmds.StencilTestMask = state->stencil[0].valuemask;
   wmds.StencilWriteMask = state->stencil[0].writemask;
   wmds.BackfaceStencilTestMask = state->stencil[1].valuemask;
   wmds.BackfaceStencilWriteMask = state->stencil[1].writemask;
   #if GFX_VER >= 12
         #endif
            #if GFX_VER >= 12
      iris_pack_command(GENX(3DSTATE_DEPTH_BOUNDS), cso->depth_bounds, depth_bounds) {
      depth_bounds.DepthBoundsTestValueModifyDisable = false;
   depth_bounds.DepthBoundsTestEnableModifyDisable = false;
   depth_bounds.DepthBoundsTestEnable = state->depth_bounds_test;
   depth_bounds.DepthBoundsTestMinValue = state->depth_bounds_min;
         #endif
            }
      /**
   * The pipe->bind_depth_stencil_alpha_state() driver hook.
   *
   * Bind a depth/stencil/alpha CSO and flag related dirty bits.
   */
   static void
   iris_bind_zsa_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_depth_stencil_alpha_state *old_cso = ice->state.cso_zsa;
            if (new_cso) {
      if (cso_changed(alpha_ref_value))
            if (cso_changed(alpha_enabled))
            if (cso_changed(alpha_func))
            if (cso_changed(depth_writes_enabled) || cso_changed(stencil_writes_enabled))
            ice->state.depth_writes_enabled = new_cso->depth_writes_enabled;
            /* State ds_write_enable changed, need to flag dirty DS. */
   if (!old_cso || (ice->state.ds_write_state != new_cso->ds_write_state)) {
      ice->state.dirty |= IRIS_DIRTY_DS_WRITE_ENABLE;
         #if GFX_VER >= 12
         if (cso_changed(depth_bounds))
   #endif
               ice->state.cso_zsa = new_cso;
   ice->state.dirty |= IRIS_DIRTY_CC_VIEWPORT;
   ice->state.dirty |= IRIS_DIRTY_WM_DEPTH_STENCIL;
   ice->state.stage_dirty |=
            if (GFX_VER == 8)
      }
      #if GFX_VER == 8
   static bool
   want_pma_fix(struct iris_context *ice)
   {
      UNUSED struct iris_screen *screen = (void *) ice->ctx.screen;
   UNUSED const struct intel_device_info *devinfo = screen->devinfo;
   const struct brw_wm_prog_data *wm_prog_data = (void *)
         const struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   const struct iris_depth_stencil_alpha_state *cso_zsa = ice->state.cso_zsa;
            /* In very specific combinations of state, we can instruct Gfx8-9 hardware
   * to avoid stalling at the pixel mask array.  The state equations are
   * documented in these places:
   *
   * - Gfx8 Depth PMA Fix:   CACHE_MODE_1::NP_PMA_FIX_ENABLE
   * - Gfx9 Stencil PMA Fix: CACHE_MODE_0::STC PMA Optimization Enable
   *
   * Both equations share some common elements:
   *
   *    no_hiz_op =
   *       !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
   *         3DSTATE_WM_HZ_OP::DepthBufferResolve ||
   *         3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
   *         3DSTATE_WM_HZ_OP::StencilBufferClear) &&
   *
   *    killpixels =
   *       3DSTATE_WM::ForceKillPix != ForceOff &&
   *       (3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
   *        3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
   *        3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
   *        3DSTATE_PS_BLEND::AlphaTestEnable ||
   *        3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable)
   *
   *    (Technically the stencil PMA treats ForceKillPix differently,
   *     but I think this is a documentation oversight, and we don't
   *     ever use it in this way, so it doesn't matter).
   *
   *    common_pma_fix =
   *       3DSTATE_WM::ForceThreadDispatch != 1 &&
   *       3DSTATE_RASTER::ForceSampleCount == NUMRASTSAMPLES_0 &&
   *       3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL &&
   *       3DSTATE_DEPTH_BUFFER::HIZ Enable &&
   *       3DSTATE_WM::EDSC_Mode != EDSC_PREPS &&
   *       3DSTATE_PS_EXTRA::PixelShaderValid &&
   *       no_hiz_op
   *
   * These are always true:
   *
   *    3DSTATE_RASTER::ForceSampleCount == NUMRASTSAMPLES_0
   *    3DSTATE_PS_EXTRA::PixelShaderValid
   *
   * Also, we never use the normal drawing path for HiZ ops; these are true:
   *
   *    !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
   *      3DSTATE_WM_HZ_OP::DepthBufferResolve ||
   *      3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
   *      3DSTATE_WM_HZ_OP::StencilBufferClear)
   *
   * This happens sometimes:
   *
   *    3DSTATE_WM::ForceThreadDispatch != 1
   *
   * However, we choose to ignore it as it either agrees with the signal
   * (dispatch was already enabled, so nothing out of the ordinary), or
   * there are no framebuffer attachments (so no depth or HiZ anyway,
   * meaning the PMA signal will already be disabled).
            if (!cso_fb->zsbuf)
            struct iris_resource *zres, *sres;
            /* 3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL &&
   * 3DSTATE_DEPTH_BUFFER::HIZ Enable &&
   */
   if (!zres ||
      !iris_resource_level_has_hiz(devinfo, zres, cso_fb->zsbuf->u.tex.level))
         /* 3DSTATE_WM::EDSC_Mode != EDSC_PREPS */
   if (wm_prog_data->early_fragment_tests)
            /* 3DSTATE_WM::ForceKillPix != ForceOff &&
   * (3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
   *  3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
   *  3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
   *  3DSTATE_PS_BLEND::AlphaTestEnable ||
   *  3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable)
   */
   bool killpixels = wm_prog_data->uses_kill || wm_prog_data->uses_omask ||
            /* The Gfx8 depth PMA equation becomes:
   *
   *    depth_writes =
   *       3DSTATE_WM_DEPTH_STENCIL::DepthWriteEnable &&
   *       3DSTATE_DEPTH_BUFFER::DEPTH_WRITE_ENABLE
   *
   *    stencil_writes =
   *       3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
   *       3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE &&
   *       3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE
   *
   *    Z_PMA_OPT =
   *       common_pma_fix &&
   *       3DSTATE_WM_DEPTH_STENCIL::DepthTestEnable &&
   *       ((killpixels && (depth_writes || stencil_writes)) ||
   *        3DSTATE_PS_EXTRA::PixelShaderComputedDepthMode != PSCDEPTH_OFF)
   *
   */
   if (!cso_zsa->depth_test_enabled)
            return wm_prog_data->computed_depth_mode != PSCDEPTH_OFF ||
            }
   #endif
      void
   genX(update_pma_fix)(struct iris_context *ice,
               {
   #if GFX_VER == 8
               if (genx->pma_fix_enabled == enable)
                     /* According to the Broadwell PIPE_CONTROL documentation, software should
   * emit a PIPE_CONTROL with the CS Stall and Depth Cache Flush bits set
   * prior to the LRI.  If stencil buffer writes are enabled, then a Render        * Cache Flush is also necessary.
   *
   * The Gfx9 docs say to use a depth stall rather than a command streamer
   * stall.  However, the hardware seems to violently disagree.  A full
   * command streamer stall seems to be needed in both cases.
   */
   iris_emit_pipe_control_flush(batch, "PMA fix change (1/2)",
                        iris_emit_reg(batch, GENX(CACHE_MODE_1), reg) {
      reg.NPPMAFixEnable = enable;
   reg.NPEarlyZFailsDisable = enable;
   reg.NPPMAFixEnableMask = true;
               /* After the LRI, a PIPE_CONTROL with both the Depth Stall and Depth Cache
   * Flush bits is often necessary.  We do it regardless because it's easier.
   * The render cache flush is also necessary if stencil writes are enabled.
   *
   * Again, the Gfx9 docs give a different set of flushes but the Broadwell
   * flushes seem to work just as well.
   */
   iris_emit_pipe_control_flush(batch, "PMA fix change (1/2)",
                  #endif
   }
      /**
   * Gallium CSO for rasterizer state.
   */
   struct iris_rasterizer_state {
      uint32_t sf[GENX(3DSTATE_SF_length)];
   uint32_t clip[GENX(3DSTATE_CLIP_length)];
   uint32_t raster[GENX(3DSTATE_RASTER_length)];
   uint32_t wm[GENX(3DSTATE_WM_length)];
            uint8_t num_clip_plane_consts;
   bool clip_halfz; /* for CC_VIEWPORT */
   bool depth_clip_near; /* for CC_VIEWPORT */
   bool depth_clip_far; /* for CC_VIEWPORT */
   bool flatshade; /* for shader state */
   bool flatshade_first; /* for stream output */
   bool clamp_fragment_color; /* for shader state */
   bool light_twoside; /* for shader state */
   bool rasterizer_discard; /* for 3DSTATE_STREAMOUT and 3DSTATE_CLIP */
   bool half_pixel_center; /* for 3DSTATE_MULTISAMPLE */
   bool line_smooth;
   bool line_stipple_enable;
   bool poly_stipple_enable;
   bool multisample;
   bool force_persample_interp;
   bool conservative_rasterization;
   bool fill_mode_point;
   bool fill_mode_line;
   bool fill_mode_point_or_line;
   enum pipe_sprite_coord_mode sprite_coord_mode; /* PIPE_SPRITE_* */
      };
      static float
   get_line_width(const struct pipe_rasterizer_state *state)
   {
               /* From the OpenGL 4.4 spec:
   *
   * "The actual width of non-antialiased lines is determined by rounding
   *  the supplied width to the nearest integer, then clamping it to the
   *  implementation-dependent maximum non-antialiased line width."
   */
   if (!state->multisample && !state->line_smooth)
            if (!state->multisample && state->line_smooth && line_width < 1.5f) {
      /* For 1 pixel line thickness or less, the general anti-aliasing
   * algorithm gives up, and a garbage line is generated.  Setting a
   * Line Width of 0.0 specifies the rasterization of the "thinnest"
   * (one-pixel-wide), non-antialiased lines.
   *
   * Lines rendered with zero Line Width are rasterized using the
   * "Grid Intersection Quantization" rules as specified by the
   * "Zero-Width (Cosmetic) Line Rasterization" section of the docs.
   */
                  }
      /**
   * The pipe->create_rasterizer_state() driver hook.
   */
   static void *
   iris_create_rasterizer_state(struct pipe_context *ctx,
         {
      struct iris_rasterizer_state *cso =
            cso->multisample = state->multisample;
   cso->force_persample_interp = state->force_persample_interp;
   cso->clip_halfz = state->clip_halfz;
   cso->depth_clip_near = state->depth_clip_near;
   cso->depth_clip_far = state->depth_clip_far;
   cso->flatshade = state->flatshade;
   cso->flatshade_first = state->flatshade_first;
   cso->clamp_fragment_color = state->clamp_fragment_color;
   cso->light_twoside = state->light_twoside;
   cso->rasterizer_discard = state->rasterizer_discard;
   cso->half_pixel_center = state->half_pixel_center;
   cso->sprite_coord_mode = state->sprite_coord_mode;
   cso->sprite_coord_enable = state->sprite_coord_enable;
   cso->line_smooth = state->line_smooth;
   cso->line_stipple_enable = state->line_stipple_enable;
   cso->poly_stipple_enable = state->poly_stipple_enable;
   cso->conservative_rasterization =
            cso->fill_mode_point =
      state->fill_front == PIPE_POLYGON_MODE_POINT ||
      cso->fill_mode_line =
      state->fill_front == PIPE_POLYGON_MODE_LINE ||
      cso->fill_mode_point_or_line =
      cso->fill_mode_point ||
         if (state->clip_plane_enable != 0)
         else
                     iris_pack_command(GENX(3DSTATE_SF), cso->sf, sf) {
      sf.StatisticsEnable = true;
   sf.AALineDistanceMode = AALINEDISTANCE_TRUE;
   sf.LineEndCapAntialiasingRegionWidth =
         sf.LastPixelEnable = state->line_last_pixel;
   sf.LineWidth = line_width;
   sf.SmoothPointEnable = (state->point_smooth || state->multisample) &&
         sf.PointWidthSource = state->point_size_per_vertex ? Vertex : State;
            if (state->flatshade_first) {
         } else {
      sf.TriangleStripListProvokingVertexSelect = 2;
   sf.TriangleFanProvokingVertexSelect = 2;
                  iris_pack_command(GENX(3DSTATE_RASTER), cso->raster, rr) {
      rr.FrontWinding = state->front_ccw ? CounterClockwise : Clockwise;
   rr.CullMode = translate_cull_mode(state->cull_face);
   rr.FrontFaceFillMode = translate_fill_mode(state->fill_front);
   rr.BackFaceFillMode = translate_fill_mode(state->fill_back);
   rr.DXMultisampleRasterizationEnable = state->multisample;
   rr.GlobalDepthOffsetEnableSolid = state->offset_tri;
   rr.GlobalDepthOffsetEnableWireframe = state->offset_line;
   rr.GlobalDepthOffsetEnablePoint = state->offset_point;
   rr.GlobalDepthOffsetConstant = state->offset_units * 2;
   rr.GlobalDepthOffsetScale = state->offset_scale;
   rr.GlobalDepthOffsetClamp = state->offset_clamp;
   rr.SmoothPointEnable = state->point_smooth;
   #if GFX_VER >= 9
         rr.ViewportZNearClipTestEnable = state->depth_clip_near;
   rr.ViewportZFarClipTestEnable = state->depth_clip_far;
   rr.ConservativeRasterizationEnable =
   #else
         #endif
               iris_pack_command(GENX(3DSTATE_CLIP), cso->clip, cl) {
      /* cl.NonPerspectiveBarycentricEnable is filled in at draw time from
   * the FS program; cl.ForceZeroRTAIndexEnable is filled in from the FB.
   */
   cl.EarlyCullEnable = true;
   cl.UserClipDistanceClipTestEnableBitmask = state->clip_plane_enable;
   cl.ForceUserClipDistanceClipTestEnableBitmask = true;
   cl.APIMode = state->clip_halfz ? APIMODE_D3D : APIMODE_OGL;
   cl.GuardbandClipTestEnable = true;
   cl.ClipEnable = true;
   cl.MinimumPointWidth = 0.125;
            if (state->flatshade_first) {
         } else {
      cl.TriangleStripListProvokingVertexSelect = 2;
   cl.TriangleFanProvokingVertexSelect = 2;
                  iris_pack_command(GENX(3DSTATE_WM), cso->wm, wm) {
      /* wm.BarycentricInterpolationMode and wm.EarlyDepthStencilControl are
   * filled in at draw time from the FS program.
   */
   wm.LineAntialiasingRegionWidth = _10pixels;
   wm.LineEndCapAntialiasingRegionWidth = _05pixels;
   wm.PointRasterizationRule = RASTRULE_UPPER_RIGHT;
   wm.LineStippleEnable = state->line_stipple_enable;
               /* Remap from 0..255 back to 1..256 */
            iris_pack_command(GENX(3DSTATE_LINE_STIPPLE), cso->line_stipple, line) {
      if (state->line_stipple_enable) {
      line.LineStipplePattern = state->line_stipple_pattern;
   line.LineStippleInverseRepeatCount = 1.0f / line_stipple_factor;
                     }
      /**
   * The pipe->bind_rasterizer_state() driver hook.
   *
   * Bind a rasterizer CSO and flag related dirty bits.
   */
   static void
   iris_bind_rasterizer_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_rasterizer_state *old_cso = ice->state.cso_rast;
            if (new_cso) {
      /* Try to avoid re-emitting 3DSTATE_LINE_STIPPLE, it's non-pipelined */
   if (cso_changed_memcmp(line_stipple))
            if (cso_changed(half_pixel_center))
            if (cso_changed(line_stipple_enable) || cso_changed(poly_stipple_enable))
            if (cso_changed(rasterizer_discard))
            if (cso_changed(flatshade_first))
            if (cso_changed(depth_clip_near) || cso_changed(depth_clip_far) ||
                  if (cso_changed(sprite_coord_enable) ||
      cso_changed(sprite_coord_mode) ||
               if (cso_changed(conservative_rasterization))
               ice->state.cso_rast = new_cso;
   ice->state.dirty |= IRIS_DIRTY_RASTER;
   ice->state.dirty |= IRIS_DIRTY_CLIP;
   ice->state.stage_dirty |=
      }
      /**
   * Return true if the given wrap mode requires the border color to exist.
   *
   * (We can skip uploading it if the sampler isn't going to use it.)
   */
   static bool
   wrap_mode_needs_border_color(unsigned wrap_mode)
   {
         }
      /**
   * Gallium CSO for sampler state.
   */
   struct iris_sampler_state {
      union pipe_color_union border_color;
                  #if GFX_VERx10 == 125
      /* Sampler state structure to use for 3D textures in order to
   * implement Wa_14014414195.
   */
      #endif
   };
      static void
   fill_sampler_state(uint32_t *sampler_state,
               {
      float min_lod = state->min_lod;
            // XXX: explain this code ported from ilo...I don't get it at all...
   if (state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE &&
      state->min_lod > 0.0f) {
   min_lod = 0.0f;
               iris_pack_state(GENX(SAMPLER_STATE), sampler_state, samp) {
      samp.TCXAddressControlMode = translate_wrap(state->wrap_s);
   samp.TCYAddressControlMode = translate_wrap(state->wrap_t);
   samp.TCZAddressControlMode = translate_wrap(state->wrap_r);
   samp.CubeSurfaceControlMode = state->seamless_cube_map;
   samp.NonnormalizedCoordinateEnable = state->unnormalized_coords;
   samp.MinModeFilter = state->min_img_filter;
   samp.MagModeFilter = mag_img_filter;
   samp.MipModeFilter = translate_mip_filter(state->min_mip_filter);
            if (max_anisotropy >= 2) {
      if (state->min_img_filter == PIPE_TEX_FILTER_LINEAR) {
      samp.MinModeFilter = MAPFILTER_ANISOTROPIC;
                              samp.MaximumAnisotropy =
               /* Set address rounding bits if not using nearest filtering. */
   if (state->min_img_filter != PIPE_TEX_FILTER_NEAREST) {
      samp.UAddressMinFilterRoundingEnable = true;
   samp.VAddressMinFilterRoundingEnable = true;
               if (state->mag_img_filter != PIPE_TEX_FILTER_NEAREST) {
      samp.UAddressMagFilterRoundingEnable = true;
   samp.VAddressMagFilterRoundingEnable = true;
               if (state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE)
                     samp.LODPreClampMode = CLAMP_MODE_OGL;
   samp.MinLOD = CLAMP(min_lod, 0, hw_max_lod);
   samp.MaxLOD = CLAMP(state->max_lod, 0, hw_max_lod);
                  }
      /**
   * The pipe->create_sampler_state() driver hook.
   *
   * We fill out SAMPLER_STATE (except for the border color pointer), and
   * store that on the CPU.  It doesn't make sense to upload it to a GPU
   * buffer object yet, because 3DSTATE_SAMPLER_STATE_POINTERS requires
   * all bound sampler states to be in contiguous memor.
   */
   static void *
   iris_create_sampler_state(struct pipe_context *ctx,
         {
      UNUSED struct iris_screen *screen = (void *)ctx->screen;
   UNUSED const struct intel_device_info *devinfo = screen->devinfo;
            if (!cso)
            STATIC_ASSERT(PIPE_TEX_FILTER_NEAREST == MAPFILTER_NEAREST);
            unsigned wrap_s = translate_wrap(state->wrap_s);
   unsigned wrap_t = translate_wrap(state->wrap_t);
                     cso->needs_border_color = wrap_mode_needs_border_color(wrap_s) ||
                        #if GFX_VERx10 == 125
      /* Fill an extra sampler state structure with anisotropic filtering
   * disabled used to implement Wa_14014414195.
   */
   if (intel_needs_workaround(screen->devinfo, 14014414195))
      #endif
            }
      /**
   * The pipe->bind_sampler_states() driver hook.
   */
   static void
   iris_bind_sampler_states(struct pipe_context *ctx,
                     {
      struct iris_context *ice = (struct iris_context *) ctx;
   gl_shader_stage stage = stage_from_pipe(p_stage);
                              for (int i = 0; i < count; i++) {
      struct iris_sampler_state *state = states ? states[i] : NULL;
   if (shs->samplers[start + i] != state) {
      shs->samplers[start + i] = state;
                  if (dirty)
      }
      /**
   * Upload the sampler states into a contiguous area of GPU memory, for
   * for 3DSTATE_SAMPLER_STATE_POINTERS_*.
   *
   * Also fill out the border color state pointers.
   */
   static void
   iris_upload_sampler_states(struct iris_context *ice, gl_shader_stage stage)
   {
      struct iris_screen *screen = (struct iris_screen *) ice->ctx.screen;
   struct iris_compiled_shader *shader = ice->shaders.prog[stage];
   struct iris_shader_state *shs = &ice->state.shaders[stage];
   struct iris_border_color_pool *border_color_pool =
            /* We assume gallium frontends will call pipe->bind_sampler_states()
   * if the program's number of textures changes.
   */
            if (!count)
            /* Assemble the SAMPLER_STATEs into a contiguous table that lives
   * in the dynamic state memory zone, so we can point to it via the
   * 3DSTATE_SAMPLER_STATE_POINTERS_* commands.
   */
   unsigned size = count * 4 * GENX(SAMPLER_STATE_length);
   uint32_t *map =
         if (unlikely(!map))
            struct pipe_resource *res = shs->sampler_table.res;
            iris_record_state_size(ice->state.sizes,
                              for (int i = 0; i < count; i++) {
      struct iris_sampler_state *state = shs->samplers[i];
            if (!state) {
         } else {
      #if GFX_VERx10 == 125
            if (intel_needs_workaround(screen->devinfo, 14014414195) &&
      tex && tex->res->base.b.target == PIPE_TEXTURE_3D) {
   #endif
               if (!state->needs_border_color) {
                           /* We may need to swizzle the border color for format faking.
   * A/LA formats are faked as R/RG with 000R or R00G swizzles.
   * This means we need to move the border color's A channel into
   * the R or G channels so that those read swizzles will move it
   * back into A.
   */
   union pipe_color_union *color = &state->border_color;
   union pipe_color_union tmp;
                     if (util_format_is_alpha(internal_format)) {
      unsigned char swz[4] = {
      PIPE_SWIZZLE_W, PIPE_SWIZZLE_0,
      };
   util_format_apply_color_swizzle(&tmp, color, swz, true);
      } else if (util_format_is_luminance_alpha(internal_format) &&
            unsigned char swz[4] = {
      PIPE_SWIZZLE_X, PIPE_SWIZZLE_W,
      };
   util_format_apply_color_swizzle(&tmp, color, swz, true);
                  /* Stream out the border color and merge the pointer. */
                  uint32_t dynamic[GENX(SAMPLER_STATE_length)];
   iris_pack_state(GENX(SAMPLER_STATE), dynamic, dyns) {
                  for (uint32_t j = 0; j < GENX(SAMPLER_STATE_length); j++)
                        }
      static enum isl_channel_select
   fmt_swizzle(const struct iris_format_info *fmt, enum pipe_swizzle swz)
   {
      switch (swz) {
   case PIPE_SWIZZLE_X: return fmt->swizzle.r;
   case PIPE_SWIZZLE_Y: return fmt->swizzle.g;
   case PIPE_SWIZZLE_Z: return fmt->swizzle.b;
   case PIPE_SWIZZLE_W: return fmt->swizzle.a;
   case PIPE_SWIZZLE_1: return ISL_CHANNEL_SELECT_ONE;
   case PIPE_SWIZZLE_0: return ISL_CHANNEL_SELECT_ZERO;
   default: unreachable("invalid swizzle");
      }
      static void
   fill_buffer_surface_state(struct isl_device *isl_dev,
                           struct iris_resource *res,
   void *map,
   enum isl_format format,
   {
      const struct isl_format_layout *fmtl = isl_format_get_layout(format);
            /* The ARB_texture_buffer_specification says:
   *
   *    "The number of texels in the buffer texture's texel array is given by
   *
   *       floor(<buffer_size> / (<components> * sizeof(<base_type>)),
   *
   *     where <buffer_size> is the size of the buffer object, in basic
   *     machine units and <components> and <base_type> are the element count
   *     and base data type for elements, as specified in Table X.1.  The
   *     number of texels in the texel array is then clamped to the
   *     implementation-dependent limit MAX_TEXTURE_BUFFER_SIZE_ARB."
   *
   * We need to clamp the size in bytes to MAX_TEXTURE_BUFFER_SIZE * stride,
   * so that when ISL divides by stride to obtain the number of texels, that
   * texel count is clamped to MAX_TEXTURE_BUFFER_SIZE.
   */
   unsigned final_size =
      MIN3(size, res->bo->size - res->offset - offset,
         isl_buffer_fill_state(isl_dev, map,
                        .address = res->bo->address + res->offset + offset,
   .size_B = final_size,
   }
      #define SURFACE_STATE_ALIGNMENT 64
      /**
   * Allocate several contiguous SURFACE_STATE structures, one for each
   * supported auxiliary surface mode.  This only allocates the CPU-side
   * copy, they will need to be uploaded later after they're filled in.
   */
   static void
   alloc_surface_states(struct iris_surface_state *surf_state,
         {
               /* If this changes, update this to explicitly align pointers */
                     /* In case we're re-allocating them... */
            surf_state->aux_usages = aux_usages;
   surf_state->num_states = util_bitcount(aux_usages);
   surf_state->cpu = calloc(surf_state->num_states, surf_size);
   surf_state->ref.offset = 0;
               }
      /**
   * Upload the CPU side SURFACE_STATEs into a GPU buffer.
   */
   static void
   upload_surface_states(struct u_upload_mgr *mgr,
         {
      const unsigned surf_size = 4 * GENX(RENDER_SURFACE_STATE_length);
            void *map =
            surf_state->ref.offset +=
            if (map)
      }
      /**
   * Update resource addresses in a set of SURFACE_STATE descriptors,
   * and re-upload them if necessary.
   */
   static bool
   update_surface_state_addrs(struct u_upload_mgr *mgr,
               {
      if (surf_state->bo_address == bo->address)
            STATIC_ASSERT(GENX(RENDER_SURFACE_STATE_SurfaceBaseAddress_start) % 64 == 0);
                     /* First, update the CPU copies.  We assume no other fields exist in
   * the QWord containing Surface Base Address.
   */
   for (unsigned i = 0; i < surf_state->num_states; i++) {
      *ss_addr = *ss_addr - surf_state->bo_address + bo->address;
               /* Next, upload the updated copies to a GPU buffer. */
                        }
      /* We should only use this function when it's needed to fill out
   * surf with information provided by the pipe_(image|sampler)_view.
   * This is only necessary for CL extension cl_khr_image2d_from_buffer.
   * This is the reason why ISL_SURF_DIM_2D is hardcoded on dim field.
   */
   static void
   fill_surf_for_tex2d_from_buffer(struct isl_device *isl_dev,
                                 enum isl_format format,
   {
      const struct isl_format_layout *fmtl = isl_format_get_layout(format);
            const struct isl_surf_init_info init_info = {
      .dim = ISL_SURF_DIM_2D,
   .format = format,
   .width = width,
   .height = height,
   .depth = 1,
   .levels = 1,
   .array_len = 1,
   .samples = 1,
   .min_alignment_B = 4,
   .row_pitch_B = row_stride * cpp,
   .usage = usage,
               const bool isl_surf_created_successfully =
               }
      static void
   fill_surface_state(struct isl_device *isl_dev,
                     void *map,
   struct iris_resource *res,
   struct isl_surf *surf,
   struct isl_view *view,
   unsigned aux_usage,
   {
      struct isl_surf_fill_state_info f = {
      .surf = surf,
   .view = view,
   .mocs = iris_mocs(res->bo, isl_dev, view->usage),
   .address = res->bo->address + res->offset + extra_main_offset,
   .x_offset_sa = tile_x_sa,
               if (aux_usage != ISL_AUX_USAGE_NONE) {
      f.aux_surf = &res->aux.surf;
   f.aux_usage = aux_usage;
            if (aux_usage == ISL_AUX_USAGE_MC)
      f.mc_format = iris_format_for_usage(isl_dev->info,
               if (res->aux.bo)
            if (res->aux.clear_color_bo) {
      f.clear_address = res->aux.clear_color_bo->address +
                           }
      static void
   fill_surface_states(struct isl_device *isl_dev,
                     struct iris_surface_state *surf_state,
   struct iris_resource *res,
   struct isl_surf *surf,
   struct isl_view *view,
   {
      void *map = surf_state->cpu;
            while (aux_modes) {
               fill_surface_state(isl_dev, map, res, surf, view, aux_usage,
                  }
      /**
   * The pipe->create_sampler_view() driver hook.
   */
   static struct pipe_sampler_view *
   iris_create_sampler_view(struct pipe_context *ctx,
               {
      struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (!isv)
            /* initialize base object */
   isv->base = *tmpl;
   isv->base.context = ctx;
   isv->base.texture = NULL;
   pipe_reference_init(&isv->base.reference, 1);
            if (util_format_is_depth_or_stencil(tmpl->format)) {
      struct iris_resource *zres, *sres;
   const struct util_format_description *desc =
                                                   if (isv->base.target == PIPE_TEXTURE_CUBE ||
      isv->base.target == PIPE_TEXTURE_CUBE_ARRAY)
         const struct iris_format_info fmt =
                     isv->view = (struct isl_view) {
      .format = fmt.fmt,
   .swizzle = (struct isl_swizzle) {
      .r = fmt_swizzle(&fmt, tmpl->swizzle_r),
   .g = fmt_swizzle(&fmt, tmpl->swizzle_g),
   .b = fmt_swizzle(&fmt, tmpl->swizzle_b),
      },
                        if ((isv->res->aux.usage == ISL_AUX_USAGE_CCS_D ||
      isv->res->aux.usage == ISL_AUX_USAGE_CCS_E ||
   isv->res->aux.usage == ISL_AUX_USAGE_FCV_CCS_E) &&
   !isl_format_supports_ccs_e(devinfo, isv->view.format)) {
      } else if (isl_aux_usage_has_hiz(isv->res->aux.usage) &&
               } else {
      aux_usages = 1 << ISL_AUX_USAGE_NONE |
               alloc_surface_states(&isv->surface_state, aux_usages);
            /* Fill out SURFACE_STATE for this view. */
   if (tmpl->target != PIPE_BUFFER) {
      isv->view.base_level = tmpl->u.tex.first_level;
            if (tmpl->target == PIPE_TEXTURE_3D) {
      isv->view.base_array_layer = 0;
      #if GFX_VER < 9
               #endif
            isv->view.base_array_layer = tmpl->u.tex.first_layer;
   isv->view.array_len =
               fill_surface_states(&screen->isl_dev, &isv->surface_state, isv->res,
      } else if (isv->base.is_tex2d_from_buf) {
      /* In case it's a 2d image created from a buffer, we should
   * use fill_surface_states function with image parameters provided
   * by the CL application
   */
   isv->view.base_array_layer = 0;
            /* Create temp_surf and fill with values provided by CL application */
   struct isl_surf temp_surf;
   fill_surf_for_tex2d_from_buffer(&screen->isl_dev, fmt.fmt,
                                    fill_surface_states(&screen->isl_dev, &isv->surface_state, isv->res,
      } else {
      fill_buffer_surface_state(&screen->isl_dev, isv->res,
                                    }
      static void
   iris_sampler_view_destroy(struct pipe_context *ctx,
         {
      struct iris_sampler_view *isv = (void *) state;
   pipe_resource_reference(&state->texture, NULL);
   pipe_resource_reference(&isv->surface_state.ref.res, NULL);
   free(isv->surface_state.cpu);
      }
      /**
   * The pipe->create_surface() driver hook.
   *
   * In Gallium nomenclature, "surfaces" are a view of a resource that
   * can be bound as a render target or depth/stencil buffer.
   */
   static struct pipe_surface *
   iris_create_surface(struct pipe_context *ctx,
               {
      struct iris_screen *screen = (struct iris_screen *)ctx->screen;
            isl_surf_usage_flags_t usage = 0;
   if (tmpl->writable)
         else if (util_format_is_depth_or_stencil(tmpl->format))
         else
            const struct iris_format_info fmt =
            if ((usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) &&
      !isl_format_supports_rendering(devinfo, fmt.fmt)) {
   /* Framebuffer validation will reject this invalid case, but it
   * hasn't had the opportunity yet.  In the meantime, we need to
   * avoid hitting ISL asserts about unsupported formats below.
   */
               struct iris_surface *surf = calloc(1, sizeof(struct iris_surface));
            if (!surf)
                     struct isl_view *view = &surf->view;
   *view = (struct isl_view) {
      .format = fmt.fmt,
   .base_level = tmpl->u.tex.level,
   .levels = 1,
   .base_array_layer = tmpl->u.tex.first_layer,
   .array_len = array_len,
   .swizzle = ISL_SWIZZLE_IDENTITY,
            #if GFX_VER == 8
      struct isl_view *read_view = &surf->read_view;
   *read_view = (struct isl_view) {
      .format = fmt.fmt,
   .base_level = tmpl->u.tex.level,
   .levels = 1,
   .base_array_layer = tmpl->u.tex.first_layer,
   .array_len = array_len,
   .swizzle = ISL_SWIZZLE_IDENTITY,
               struct isl_surf read_surf = res->surf;
   uint64_t read_surf_offset_B = 0;
   uint32_t read_surf_tile_x_sa = 0, read_surf_tile_y_sa = 0;
   if (tex->target == PIPE_TEXTURE_3D && array_len == 1) {
      /* The minimum array element field of the surface state structure is
   * ignored by the sampler unit for 3D textures on some hardware.  If the
   * render buffer is a single slice of a 3D texture, create a 2D texture
   * covering that slice.
   *
   * TODO: This only handles the case where we're rendering to a single
   * slice of an array texture.  If we have layered rendering combined
   * with non-coherent FB fetch and a non-zero base_array_layer, then
   * we're going to run into problems.
   *
   * See https://gitlab.freedesktop.org/mesa/mesa/-/issues/4904
   */
   isl_surf_get_image_surf(&screen->isl_dev, &res->surf,
                           read_view->base_level = 0;
   read_view->base_array_layer = 0;
      } else if (tex->target == PIPE_TEXTURE_1D_ARRAY) {
      /* Convert 1D array textures to 2D arrays because shaders always provide
   * the array index coordinate at the Z component to avoid recompiles
   * when changing the texture target of the framebuffer.
   */
   assert(read_surf.dim_layout == ISL_DIM_LAYOUT_GFX4_2D);
         #endif
         struct isl_surf isl_surf = res->surf;
   uint64_t offset_B = 0;
   uint32_t tile_x_el = 0, tile_y_el = 0;
   if (isl_format_is_compressed(res->surf.format)) {
      /* The resource has a compressed format, which is not renderable, but we
   * have a renderable view format.  We must be attempting to upload
   * blocks of compressed data via an uncompressed view.
   *
   * In this case, we can assume there are no auxiliary buffers, a single
   * miplevel, and that the resource is single-sampled.  Gallium may try
   * and create an uncompressed view with multiple layers, however.
   */
   assert(res->aux.usage == ISL_AUX_USAGE_NONE);
   assert(res->surf.samples == 1);
            bool ok = isl_surf_get_uncompressed_surf(&screen->isl_dev,
                        /* On Broadwell, HALIGN and VALIGN are specified in pixels and are
   * hard-coded to align to exactly the block size of the compressed
   * texture. This means that, when reinterpreted as a non-compressed
   * texture, the tile offsets may be anything.
   *
   * We need them to be multiples of 4 to be usable in RENDER_SURFACE_STATE,
   * so force the state tracker to take fallback paths if they're not.
   #if GFX_VER == 8
         if (tile_x_el % 4 != 0 || tile_y_el % 4 != 0) {
         #endif
            if (!ok) {
      free(surf);
                           struct pipe_surface *psurf = &surf->base;
   pipe_reference_init(&psurf->reference, 1);
   pipe_resource_reference(&psurf->texture, tex);
   psurf->context = ctx;
   psurf->format = tmpl->format;
   psurf->width = isl_surf.logical_level0_px.width;
   psurf->height = isl_surf.logical_level0_px.height;
   psurf->texture = tex;
   psurf->u.tex.first_layer = tmpl->u.tex.first_layer;
   psurf->u.tex.last_layer = tmpl->u.tex.last_layer;
            /* Bail early for depth/stencil - we don't want SURFACE_STATE for them. */
   if (res->surf.usage & (ISL_SURF_USAGE_DEPTH_BIT |
                  /* Fill out a SURFACE_STATE for each possible auxiliary surface mode and
   * return the pipe_surface.
   */
            if ((res->aux.usage == ISL_AUX_USAGE_CCS_E ||
      res->aux.usage == ISL_AUX_USAGE_FCV_CCS_E) &&
   !isl_format_supports_ccs_e(devinfo, view->format)) {
      } else {
      aux_usages = 1 << ISL_AUX_USAGE_NONE |
               alloc_surface_states(&surf->surface_state, aux_usages);
   surf->surface_state.bo_address = res->bo->address;
   fill_surface_states(&screen->isl_dev, &surf->surface_state, res,
         #if GFX_VER == 8
      alloc_surface_states(&surf->surface_state_read, aux_usages);
   surf->surface_state_read.bo_address = res->bo->address;
   fill_surface_states(&screen->isl_dev, &surf->surface_state_read, res,
            #endif
            }
      #if GFX_VER < 9
   static void
   fill_default_image_param(struct brw_image_param *param)
   {
      memset(param, 0, sizeof(*param));
   /* Set the swizzling shifts to all-ones to effectively disable swizzling --
   * See emit_address_calculation() in brw_fs_surface_builder.cpp for a more
   * detailed explanation of these parameters.
   */
   param->swizzling[0] = 0xff;
      }
      static void
   fill_buffer_image_param(struct brw_image_param *param,
               {
               fill_default_image_param(param);
   param->size[0] = size / cpp;
      }
   #else
   #define isl_surf_fill_image_param(x, ...)
   #define fill_default_image_param(x, ...)
   #define fill_buffer_image_param(x, ...)
   #endif
      /**
   * The pipe->set_shader_images() driver hook.
   */
   static void
   iris_set_shader_images(struct pipe_context *ctx,
                           {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   gl_shader_stage stage = stage_from_pipe(p_stage);
      #if GFX_VER == 8
      struct iris_genx_state *genx = ice->state.genx;
      #endif
         shs->bound_image_views &=
            for (unsigned i = 0; i < count; i++) {
               if (p_images && p_images[i].resource) {
                                                                        /* Gfx12+ supports render compression for images */
                                 if (res->base.b.target != PIPE_BUFFER) {
      struct isl_view view = {
      .format = isl_fmt,
   .base_level = img->u.tex.level,
   .levels = 1,
   .base_array_layer = img->u.tex.first_layer,
   .array_len = img->u.tex.last_layer - img->u.tex.first_layer + 1,
                     /* If using untyped fallback. */
   if (isl_fmt == ISL_FORMAT_RAW) {
      fill_buffer_surface_state(&screen->isl_dev, res,
                        } else {
                        isl_surf_fill_image_param(&screen->isl_dev,
            } else if (img->access & PIPE_IMAGE_ACCESS_TEX2D_FROM_BUFFER) {
      /* In case it's a 2d image created from a buffer, we should
   * use fill_surface_states function with image parameters provided
   * by the CL application
   */
   isl_surf_usage_flags_t usage =  ISL_SURF_USAGE_STORAGE_BIT;
   struct isl_view view = {
      .format = isl_fmt,
   .base_level = 0,
   .levels = 1,
   .base_array_layer = 0,
   .array_len = 1,
                     /* Create temp_surf and fill with values provided by CL application */
   struct isl_surf temp_surf;
   enum isl_format fmt = iris_image_view_get_format(ice, img);
   fill_surf_for_tex2d_from_buffer(&screen->isl_dev, fmt,
                                    fill_surface_states(&screen->isl_dev, &iv->surface_state, res,
         isl_surf_fill_image_param(&screen->isl_dev,
            } else {
                     fill_buffer_surface_state(&screen->isl_dev, res,
                           fill_buffer_image_param(&image_params[start_slot + i],
                  } else {
      pipe_resource_reference(&iv->base.resource, NULL);
   pipe_resource_reference(&iv->surface_state.ref.res, NULL);
                  ice->state.stage_dirty |= IRIS_STAGE_DIRTY_BINDINGS_VS << stage;
   ice->state.dirty |=
      stage == MESA_SHADER_COMPUTE ? IRIS_DIRTY_COMPUTE_RESOLVES_AND_FLUSHES
         /* Broadwell also needs brw_image_params re-uploaded */
   if (GFX_VER < 9) {
      ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_VS << stage;
               if (unbind_num_trailing_slots) {
      iris_set_shader_images(ctx, p_stage, start_slot + count,
         }
      UNUSED static bool
   is_sampler_view_3d(const struct iris_sampler_view *view)
   {
         }
      /**
   * The pipe->set_sampler_views() driver hook.
   */
   static void
   iris_set_sampler_views(struct pipe_context *ctx,
                        enum pipe_shader_type p_stage,
      {
      struct iris_context *ice = (struct iris_context *) ctx;
   UNUSED struct iris_screen *screen = (void *) ctx->screen;
   UNUSED const struct intel_device_info *devinfo = screen->devinfo;
   gl_shader_stage stage = stage_from_pipe(p_stage);
   struct iris_shader_state *shs = &ice->state.shaders[stage];
            if (count == 0 && unbind_num_trailing_slots == 0)
            BITSET_CLEAR_RANGE(shs->bound_sampler_views, start,
            for (i = 0; i < count; i++) {
      struct pipe_sampler_view *pview = views ? views[i] : NULL;
      #if GFX_VERx10 == 125
         if (intel_needs_workaround(screen->devinfo, 14014414195)) {
      if (is_sampler_view_3d(shs->textures[start + i]) !=
      is_sampler_view_3d(view))
   #endif
            if (take_ownership) {
      pipe_sampler_view_reference((struct pipe_sampler_view **)
            } else {
      pipe_sampler_view_reference((struct pipe_sampler_view **)
      }
   if (view) {
                              update_surface_state_addrs(ice->state.surface_uploader,
         }
   for (; i < count + unbind_num_trailing_slots; i++) {
      pipe_sampler_view_reference((struct pipe_sampler_view **)
               ice->state.stage_dirty |= (IRIS_STAGE_DIRTY_BINDINGS_VS << stage);
   ice->state.dirty |=
      stage == MESA_SHADER_COMPUTE ? IRIS_DIRTY_COMPUTE_RESOLVES_AND_FLUSHES
   }
      static void
   iris_set_compute_resources(struct pipe_context *ctx,
               {
         }
      static void
   iris_set_global_binding(struct pipe_context *ctx,
                     {
               assert(start_slot + count <= IRIS_MAX_GLOBAL_BINDINGS);
   for (unsigned i = 0; i < count; i++) {
      if (resources && resources[i]) {
                     struct iris_resource *res = (void *) resources[i];
   assert(res->base.b.target == PIPE_BUFFER);
                  uint64_t addr = 0;
   memcpy(&addr, handles[i], sizeof(addr));
   addr += res->bo->address + res->offset;
      } else {
      pipe_resource_reference(&ice->state.global_bindings[start_slot + i],
                     }
      /**
   * The pipe->set_tess_state() driver hook.
   */
   static void
   iris_set_tess_state(struct pipe_context *ctx,
               {
      struct iris_context *ice = (struct iris_context *) ctx;
            memcpy(&ice->state.default_outer_level[0], &default_outer_level[0], 4 * sizeof(float));
            ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_TCS;
      }
      static void
   iris_set_patch_vertices(struct pipe_context *ctx, uint8_t patch_vertices)
   {
                  }
      static void
   iris_surface_destroy(struct pipe_context *ctx, struct pipe_surface *p_surf)
   {
      struct iris_surface *surf = (void *) p_surf;
   pipe_resource_reference(&p_surf->texture, NULL);
   pipe_resource_reference(&surf->surface_state.ref.res, NULL);
   pipe_resource_reference(&surf->surface_state_read.ref.res, NULL);
   free(surf->surface_state.cpu);
   free(surf->surface_state_read.cpu);
      }
      static void
   iris_set_clip_state(struct pipe_context *ctx,
         {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_VERTEX];
   struct iris_shader_state *gshs = &ice->state.shaders[MESA_SHADER_GEOMETRY];
                     ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_VS |
               shs->sysvals_need_upload = true;
   gshs->sysvals_need_upload = true;
      }
      /**
   * The pipe->set_polygon_stipple() driver hook.
   */
   static void
   iris_set_polygon_stipple(struct pipe_context *ctx,
         {
      struct iris_context *ice = (struct iris_context *) ctx;
   memcpy(&ice->state.poly_stipple, state, sizeof(*state));
      }
      /**
   * The pipe->set_sample_mask() driver hook.
   */
   static void
   iris_set_sample_mask(struct pipe_context *ctx, unsigned sample_mask)
   {
               /* We only support 16x MSAA, so we have 16 bits of sample maks.
   * st/mesa may pass us 0xffffffff though, meaning "enable all samples".
   */
   ice->state.sample_mask = sample_mask & 0xffff;
      }
      /**
   * The pipe->set_scissor_states() driver hook.
   *
   * This corresponds to our SCISSOR_RECT state structures.  It's an
   * exact match, so we just store them, and memcpy them out later.
   */
   static void
   iris_set_scissor_states(struct pipe_context *ctx,
                     {
               for (unsigned i = 0; i < num_scissors; i++) {
      if (rects[i].minx == rects[i].maxx || rects[i].miny == rects[i].maxy) {
      /* If the scissor was out of bounds and got clamped to 0 width/height
   * at the bounds, the subtraction of 1 from maximums could produce a
   * negative number and thus not clip anything.  Instead, just provide
   * a min > max scissor inside the bounds, which produces the expected
   * no rendering.
   */
   ice->state.scissors[start_slot + i] = (struct pipe_scissor_state) {
            } else {
      ice->state.scissors[start_slot + i] = (struct pipe_scissor_state) {
      .minx = rects[i].minx,     .miny = rects[i].miny,
                        }
      /**
   * The pipe->set_stencil_ref() driver hook.
   *
   * This is added to 3DSTATE_WM_DEPTH_STENCIL dynamically at draw time.
   */
   static void
   iris_set_stencil_ref(struct pipe_context *ctx,
         {
      struct iris_context *ice = (struct iris_context *) ctx;
   memcpy(&ice->state.stencil_ref, &state, sizeof(state));
   if (GFX_VER >= 12)
         else if (GFX_VER >= 9)
         else
      }
      static float
   viewport_extent(const struct pipe_viewport_state *state, int axis, float sign)
   {
         }
      /**
   * The pipe->set_viewport_states() driver hook.
   *
   * This corresponds to our SF_CLIP_VIEWPORT states.  We can't calculate
   * the guardband yet, as we need the framebuffer dimensions, but we can
   * at least fill out the rest.
   */
   static void
   iris_set_viewport_states(struct pipe_context *ctx,
                     {
      struct iris_context *ice = (struct iris_context *) ctx;
                     /* Fix depth test misrenderings by lowering translated depth range */
   if (screen->driconf.lower_depth_range_rate != 1.0f)
      ice->state.viewports[start_slot].translate[2] *=
                  if (ice->state.cso_rast && (!ice->state.cso_rast->depth_clip_near ||
            }
      /**
   * The pipe->set_framebuffer_state() driver hook.
   *
   * Sets the current draw FBO, including color render targets, depth,
   * and stencil buffers.
   */
   static void
   iris_set_framebuffer_state(struct pipe_context *ctx,
         {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
   struct isl_device *isl_dev = &screen->isl_dev;
   struct pipe_framebuffer_state *cso = &ice->state.framebuffer;
   struct iris_resource *zres;
            unsigned samples = util_framebuffer_get_num_samples(state);
            if (cso->samples != samples) {
               /* We need to toggle 3DSTATE_PS::32 Pixel Dispatch Enable */
   if (GFX_VER >= 9 && (cso->samples == 16 || samples == 16))
            /* We may need to emit blend state for Wa_14018912822. */
   if ((cso->samples > 1) != (samples > 1) &&
      intel_needs_workaround(devinfo, 14018912822)) {
   ice->state.dirty |= IRIS_DIRTY_BLEND_STATE;
                  if (cso->nr_cbufs != state->nr_cbufs) {
                  if ((cso->layers == 0) != (layers == 0)) {
                  if (cso->width != state->width || cso->height != state->height) {
                  if (cso->zsbuf || state->zsbuf) {
                  bool has_integer_rt = false;
   for (unsigned i = 0; i < state->nr_cbufs; i++) {
      if (state->cbufs[i]) {
      enum isl_format ifmt =
                        /* 3DSTATE_RASTER::AntialiasingEnable */
   if (has_integer_rt != ice->state.has_integer_rt ||
      cso->samples != samples) {
               util_copy_framebuffer_state(cso, state);
   cso->samples = samples;
                              struct isl_view view = {
      .base_level = 0,
   .levels = 1,
   .base_array_layer = 0,
   .array_len = 1,
               struct isl_depth_stencil_hiz_emit_info info = {
      .view = &view,
               if (cso->zsbuf) {
      iris_get_depth_stencil_resources(cso->zsbuf->texture, &zres,
            view.base_level = cso->zsbuf->u.tex.level;
   view.base_array_layer = cso->zsbuf->u.tex.first_layer;
   view.array_len =
            if (zres) {
               info.depth_surf = &zres->surf;
                           if (iris_resource_level_has_hiz(devinfo, zres, view.base_level)) {
      info.hiz_usage = zres->aux.usage;
   info.hiz_surf = &zres->aux.surf;
                           if (stencil_res) {
      view.usage |= ISL_SURF_USAGE_STENCIL_BIT;
   info.stencil_aux_usage = stencil_res->aux.usage;
   info.stencil_surf = &stencil_res->surf;
   info.stencil_address = stencil_res->bo->address + stencil_res->offset;
   if (!zres) {
      view.format = stencil_res->surf.format;
                              /* Make a null surface for unbound buffers */
   void *null_surf_map =
      upload_state(ice->state.surface_uploader, &ice->state.null_fb,
      isl_null_fill_state(&screen->isl_dev, null_surf_map,
                     ice->state.null_fb.offset +=
            /* Render target change */
                              ice->state.stage_dirty |=
            if (GFX_VER == 8)
      }
      /**
   * The pipe->set_constant_buffer() driver hook.
   *
   * This uploads any constant data in user buffers, and references
   * any UBO resources containing constant data.
   */
   static void
   iris_set_constant_buffer(struct pipe_context *ctx,
                     {
      struct iris_context *ice = (struct iris_context *) ctx;
   gl_shader_stage stage = stage_from_pipe(p_stage);
   struct iris_shader_state *shs = &ice->state.shaders[stage];
            /* TODO: Only do this if the buffer changes? */
            if (input && input->buffer_size && (input->buffer || input->user_buffer)) {
               if (input->user_buffer) {
      void *map = NULL;
   pipe_resource_reference(&cbuf->buffer, NULL);
                  if (!cbuf->buffer) {
      /* Allocation was unsuccessful - just unbind */
   iris_set_constant_buffer(ctx, p_stage, index, false, NULL);
               assert(map);
      } else if (input->buffer) {
      if (cbuf->buffer != input->buffer) {
      ice->state.dirty |= (IRIS_DIRTY_RENDER_MISC_BUFFER_FLUSHES |
                     if (take_ownership) {
      pipe_resource_reference(&cbuf->buffer, NULL);
      } else {
                              cbuf->buffer_size =
                  struct iris_resource *res = (void *) cbuf->buffer;
   res->bind_history |= PIPE_BIND_CONSTANT_BUFFER;
      } else {
      shs->bound_cbufs &= ~(1u << index);
                  }
      static void
   upload_sysvals(struct iris_context *ice,
               {
      UNUSED struct iris_genx_state *genx = ice->state.genx;
            struct iris_compiled_shader *shader = ice->shaders.prog[stage];
   if (!shader || (shader->num_system_values == 0 &&
                           unsigned sysval_cbuf_index = shader->num_cbufs - 1;
   struct pipe_shader_buffer *cbuf = &shs->constbuf[sysval_cbuf_index];
   unsigned system_values_start =
         unsigned upload_size = system_values_start +
                  assert(sysval_cbuf_index < PIPE_MAX_CONSTANT_BUFFERS);
   u_upload_alloc(ice->ctx.const_uploader, 0, upload_size, 64,
            if (shader->kernel_input_size > 0)
            uint32_t *sysval_map = map + system_values_start;
   for (int i = 0; i < shader->num_system_values; i++) {
      uint32_t sysval = shader->system_values[i];
            #if GFX_VER == 8
            unsigned img = BRW_PARAM_IMAGE_IDX(sysval);
   unsigned offset = BRW_PARAM_IMAGE_OFFSET(sysval);
                     #endif
         } else if (sysval == BRW_PARAM_BUILTIN_ZERO) {
         } else if (BRW_PARAM_BUILTIN_IS_CLIP_PLANE(sysval)) {
      int plane = BRW_PARAM_BUILTIN_CLIP_PLANE_IDX(sysval);
   int comp  = BRW_PARAM_BUILTIN_CLIP_PLANE_COMP(sysval);
      } else if (sysval == BRW_PARAM_BUILTIN_PATCH_VERTICES_IN) {
      if (stage == MESA_SHADER_TESS_CTRL) {
         } else {
      assert(stage == MESA_SHADER_TESS_EVAL);
   const struct shader_info *tcs_info =
         if (tcs_info)
         else
         } else if (sysval >= BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X &&
            unsigned i = sysval - BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X;
      } else if (sysval == BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_X) {
         } else if (sysval == BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_Y) {
         } else if (sysval >= BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_X &&
            unsigned i = sysval - BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_X;
      } else if (sysval == BRW_PARAM_BUILTIN_WORK_DIM) {
         } else {
                              cbuf->buffer_size = upload_size;
   iris_upload_ubo_ssbo_surf_state(ice, cbuf,
                     }
      /**
   * The pipe->set_shader_buffers() driver hook.
   *
   * This binds SSBOs and ABOs.  Unfortunately, we need to stream out
   * SURFACE_STATE here, as the buffer offset may change each time.
   */
   static void
   iris_set_shader_buffers(struct pipe_context *ctx,
                           {
      struct iris_context *ice = (struct iris_context *) ctx;
   gl_shader_stage stage = stage_from_pipe(p_stage);
                     shs->bound_ssbos &= ~modified_bits;
   shs->writable_ssbos &= ~modified_bits;
            for (unsigned i = 0; i < count; i++) {
      if (buffers && buffers[i].buffer) {
      struct iris_resource *res = (void *) buffers[i].buffer;
   struct pipe_shader_buffer *ssbo = &shs->ssbo[start_slot + i];
   struct iris_state_ref *surf_state =
         pipe_resource_reference(&ssbo->buffer, &res->base.b);
   ssbo->buffer_offset = buffers[i].buffer_offset;
                                                            util_range_add(&res->base.b, &res->valid_buffer_range, ssbo->buffer_offset,
      } else {
      pipe_resource_reference(&shs->ssbo[start_slot + i].buffer, NULL);
   pipe_resource_reference(&shs->ssbo_surf_state[start_slot + i].res,
                  ice->state.dirty |= (IRIS_DIRTY_RENDER_MISC_BUFFER_FLUSHES |
            }
      static void
   iris_delete_state(struct pipe_context *ctx, void *state)
   {
         }
      /**
   * The pipe->set_vertex_buffers() driver hook.
   *
   * This translates pipe_vertex_buffer to our 3DSTATE_VERTEX_BUFFERS packet.
   */
   static void
   iris_set_vertex_buffers(struct pipe_context *ctx,
                           {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_screen *screen = (struct iris_screen *)ctx->screen;
            ice->state.bound_vertex_buffers &=
            for (unsigned i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *buffer = buffers ? &buffers[i] : NULL;
   struct iris_vertex_buffer_state *state =
            if (!buffer) {
      pipe_resource_reference(&state->resource, NULL);
               /* We may see user buffers that are NULL bindings. */
            if (buffer->buffer.resource &&
                  if (take_ownership) {
      pipe_resource_reference(&state->resource, NULL);
      } else {
         }
                     if (res) {
      ice->state.bound_vertex_buffers |= 1ull << i;
               iris_pack_state(GENX(VERTEX_BUFFER_STATE), state->state, vb) {
      vb.VertexBufferIndex = i;
   vb.AddressModifyEnable = true;
   /* vb.BufferPitch is merged in dynamically from VE state later */
   if (res) {
      vb.BufferSize = res->base.b.width0 - (int) buffer->buffer_offset;
   vb.BufferStartingAddress =
         #if GFX_VER >= 12
         #endif
            } else {
      vb.NullVertexBuffer = true;
   vb.MOCS = iris_mocs(NULL, &screen->isl_dev,
                     for (unsigned i = 0; i < unbind_num_trailing_slots; i++) {
      struct iris_vertex_buffer_state *state =
                           }
      /**
   * Gallium CSO for vertex elements.
   */
   struct iris_vertex_element_state {
      uint32_t vertex_elements[1 + 33 * GENX(VERTEX_ELEMENT_STATE_length)];
   uint32_t vf_instancing[33 * GENX(3DSTATE_VF_INSTANCING_length)];
   uint32_t edgeflag_ve[GENX(VERTEX_ELEMENT_STATE_length)];
   uint32_t edgeflag_vfi[GENX(3DSTATE_VF_INSTANCING_length)];
   uint32_t stride[PIPE_MAX_ATTRIBS];
   unsigned vb_count;
      };
      /**
   * The pipe->create_vertex_elements_state() driver hook.
   *
   * This translates pipe_vertex_element to our 3DSTATE_VERTEX_ELEMENTS
   * and 3DSTATE_VF_INSTANCING commands. The vertex_elements and vf_instancing
   * arrays are ready to be emitted at draw time if no EdgeFlag or SGVs are
   * needed. In these cases we will need information available at draw time.
   * We setup edgeflag_ve and edgeflag_vfi as alternatives last
   * 3DSTATE_VERTEX_ELEMENT and 3DSTATE_VF_INSTANCING that can be used at
   * draw time if we detect that EdgeFlag is needed by the Vertex Shader.
   */
   static void *
   iris_create_vertex_elements(struct pipe_context *ctx,
               {
      struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
   struct iris_vertex_element_state *cso =
            cso->count = count;
            iris_pack_command(GENX(3DSTATE_VERTEX_ELEMENTS), cso->vertex_elements, ve) {
      ve.DWordLength =
               uint32_t *ve_pack_dest = &cso->vertex_elements[1];
            if (count == 0) {
      iris_pack_state(GENX(VERTEX_ELEMENT_STATE), ve_pack_dest, ve) {
      ve.Valid = true;
   ve.SourceElementFormat = ISL_FORMAT_R32G32B32A32_FLOAT;
   ve.Component0Control = VFCOMP_STORE_0;
   ve.Component1Control = VFCOMP_STORE_0;
   ve.Component2Control = VFCOMP_STORE_0;
               iris_pack_command(GENX(3DSTATE_VF_INSTANCING), vfi_pack_dest, vi) {
               for (int i = 0; i < count; i++) {
      const struct iris_format_info fmt =
         unsigned comp[4] = { VFCOMP_STORE_SRC, VFCOMP_STORE_SRC,
            switch (isl_format_get_num_channels(fmt.fmt)) {
   case 0: comp[0] = VFCOMP_STORE_0; FALLTHROUGH;
   case 1: comp[1] = VFCOMP_STORE_0; FALLTHROUGH;
   case 2: comp[2] = VFCOMP_STORE_0; FALLTHROUGH;
   case 3:
      comp[3] = isl_format_has_int_channel(fmt.fmt) ? VFCOMP_STORE_1_INT
            }
   iris_pack_state(GENX(VERTEX_ELEMENT_STATE), ve_pack_dest, ve) {
      ve.EdgeFlagEnable = false;
   ve.VertexBufferIndex = state[i].vertex_buffer_index;
   ve.Valid = true;
   ve.SourceElementOffset = state[i].src_offset;
   ve.SourceElementFormat = fmt.fmt;
   ve.Component0Control = comp[0];
   ve.Component1Control = comp[1];
   ve.Component2Control = comp[2];
               iris_pack_command(GENX(3DSTATE_VF_INSTANCING), vfi_pack_dest, vi) {
      vi.VertexElementIndex = i;
   vi.InstancingEnable = state[i].instance_divisor > 0;
               ve_pack_dest += GENX(VERTEX_ELEMENT_STATE_length);
   vfi_pack_dest += GENX(3DSTATE_VF_INSTANCING_length);
   cso->stride[state[i].vertex_buffer_index] = state[i].src_stride;
               /* An alternative version of the last VE and VFI is stored so it
   * can be used at draw time in case Vertex Shader uses EdgeFlag
   */
   if (count) {
      const unsigned edgeflag_index = count - 1;
   const struct iris_format_info fmt =
         iris_pack_state(GENX(VERTEX_ELEMENT_STATE), cso->edgeflag_ve, ve) {
      ve.EdgeFlagEnable = true ;
   ve.VertexBufferIndex = state[edgeflag_index].vertex_buffer_index;
   ve.Valid = true;
   ve.SourceElementOffset = state[edgeflag_index].src_offset;
   ve.SourceElementFormat = fmt.fmt;
   ve.Component0Control = VFCOMP_STORE_SRC;
   ve.Component1Control = VFCOMP_STORE_0;
   ve.Component2Control = VFCOMP_STORE_0;
      }
   iris_pack_command(GENX(3DSTATE_VF_INSTANCING), cso->edgeflag_vfi, vi) {
      /* The vi.VertexElementIndex of the EdgeFlag Vertex Element is filled
   * at draw time, as it should change if SGVs are emitted.
   */
   vi.InstancingEnable = state[edgeflag_index].instance_divisor > 0;
                     }
      /**
   * The pipe->bind_vertex_elements_state() driver hook.
   */
   static void
   iris_bind_vertex_elements_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_vertex_element_state *old_cso = ice->state.cso_vertex_elements;
            /* 3DSTATE_VF_SGVs overrides the last VE, so if the count is changing,
   * we need to re-emit it to ensure we're overriding the right one.
   */
   if (new_cso && cso_changed(count))
            ice->state.cso_vertex_elements = state;
   ice->state.dirty |= IRIS_DIRTY_VERTEX_ELEMENTS;
   if (new_cso) {
      /* re-emit vertex buffer state if stride changes */
   if (cso_changed(vb_count) ||
      cso_changed_memcmp_elts(stride, new_cso->vb_count))
      }
      /**
   * The pipe->create_stream_output_target() driver hook.
   *
   * "Target" here refers to a destination buffer.  We translate this into
   * a 3DSTATE_SO_BUFFER packet.  We can handle most fields, but don't yet
   * know which buffer this represents, or whether we ought to zero the
   * write-offsets, or append.  Those are handled in the set() hook.
   */
   static struct pipe_stream_output_target *
   iris_create_stream_output_target(struct pipe_context *ctx,
                     {
      struct iris_resource *res = (void *) p_res;
   struct iris_stream_output_target *cso = calloc(1, sizeof(*cso));
   if (!cso)
                     pipe_reference_init(&cso->base.reference, 1);
   pipe_resource_reference(&cso->base.buffer, p_res);
   cso->base.buffer_offset = buffer_offset;
   cso->base.buffer_size = buffer_size;
            util_range_add(&res->base.b, &res->valid_buffer_range, buffer_offset,
               }
      static void
   iris_stream_output_target_destroy(struct pipe_context *ctx,
         {
               pipe_resource_reference(&cso->base.buffer, NULL);
               }
      /**
   * The pipe->set_stream_output_targets() driver hook.
   *
   * At this point, we know which targets are bound to a particular index,
   * and also whether we want to append or start over.  We can finish the
   * 3DSTATE_SO_BUFFER packets we started earlier.
   */
   static void
   iris_set_stream_output_targets(struct pipe_context *ctx,
                     {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_genx_state *genx = ice->state.genx;
   uint32_t *so_buffers = genx->so_buffers;
            const bool active = num_targets > 0;
   if (ice->state.streamout_active != active) {
      ice->state.streamout_active = active;
            /* We only emit 3DSTATE_SO_DECL_LIST when streamout is active, because
   * it's a non-pipelined command.  If we're switching streamout on, we
   * may have missed emitting it earlier, so do so now.  (We're already
   * taking a stall to update 3DSTATE_SO_BUFFERS anyway...)
   */
   if (active) {
         } else {
      for (int i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
                     if (tgt)
                     for (int i = 0; i < 4; i++) {
      pipe_so_target_reference(&ice->state.so_target[i],
               /* No need to update 3DSTATE_SO_BUFFER unless SOL is active. */
   if (!active)
            for (unsigned i = 0; i < 4; i++,
               struct iris_stream_output_target *tgt = (void *) ice->state.so_target[i];
            if (!tgt) {
   #if GFX_VER < 12
         #else
               #endif
                  }
               if (!tgt->offset.res)
                     /* Note that offsets[i] will either be 0, causing us to zero
   * the value in the buffer, or 0xFFFFFFFF, which happens to mean
   * "continue appending at the existing offset."
   */
            /* When we're first called with an offset of 0, we want the next
   * 3DSTATE_SO_BUFFER packets to reset the offset to the beginning.
   * Any further times we emit those packets, we want to use 0xFFFFFFFF
   * to continue appending from the current offset.
   *
   * Note that we might be called by Begin (offset = 0), Pause, then
   * Resume (offset = 0xFFFFFFFF) before ever drawing (where these
   * commands will actually be sent to the GPU).  In this case, we
   * don't want to append - we still want to do our initial zeroing.
   */
   if (offset == 0)
            #if GFX_VER < 12
         #else
               #endif
            sob.SurfaceBaseAddress =
      rw_bo(NULL, res->bo->address + tgt->base.buffer_offset,
      sob.SOBufferEnable = true;
   sob.StreamOffsetWriteEnable = true;
   sob.StreamOutputBufferOffsetAddressEnable = true;
                  sob.SurfaceSize = MAX2(tgt->base.buffer_size / 4, 1) - 1;
   sob.StreamOutputBufferOffsetAddress =
      rw_bo(NULL, iris_resource_bo(tgt->offset.res)->address +
                        }
      /**
   * An iris-vtable helper for encoding the 3DSTATE_SO_DECL_LIST and
   * 3DSTATE_STREAMOUT packets.
   *
   * 3DSTATE_SO_DECL_LIST is a list of shader outputs we want the streamout
   * hardware to record.  We can create it entirely based on the shader, with
   * no dynamic state dependencies.
   *
   * 3DSTATE_STREAMOUT is an annoying mix of shader-based information and
   * state-based settings.  We capture the shader-related ones here, and merge
   * the rest in at draw time.
   */
   static uint32_t *
   iris_create_so_decl_list(const struct pipe_stream_output_info *info,
         {
      struct GENX(SO_DECL) so_decl[PIPE_MAX_VERTEX_STREAMS][128];
   int buffer_mask[PIPE_MAX_VERTEX_STREAMS] = {0, 0, 0, 0};
   int next_offset[PIPE_MAX_VERTEX_STREAMS] = {0, 0, 0, 0};
   int decls[PIPE_MAX_VERTEX_STREAMS] = {0, 0, 0, 0};
   int max_decls = 0;
                     /* Construct the list of SO_DECLs to be emitted.  The formatting of the
   * command feels strange -- each dword pair contains a SO_DECL per stream.
   */
   for (unsigned i = 0; i < info->num_outputs; i++) {
      const struct pipe_stream_output *output = &info->output[i];
   const int buffer = output->output_buffer;
   const int varying = output->register_index;
   const unsigned stream_id = output->stream;
                              /* Mesa doesn't store entries for gl_SkipComponents in the Outputs[]
   * array.  Instead, it simply increments DstOffset for the following
   * input by the number of components that should be skipped.
   *
   * Our hardware is unusual in that it requires us to program SO_DECLs
   * for fake "hole" components, rather than simply taking the offset
   * for each real varying.  Each hole can have size 1, 2, 3, or 4; we
   * program as many size = 4 holes as we can, then a final hole to
   * accommodate the final 1, 2, or 3 remaining.
   */
            while (skip_components > 0) {
      so_decl[stream_id][decls[stream_id]++] = (struct GENX(SO_DECL)) {
      .HoleFlag = 1,
   .OutputBufferSlot = output->output_buffer,
      };
                        so_decl[stream_id][decls[stream_id]++] = (struct GENX(SO_DECL)) {
      .OutputBufferSlot = output->output_buffer,
   .RegisterIndex = vue_map->varying_to_slot[varying],
   .ComponentMask =
               if (decls[stream_id] > max_decls)
               unsigned dwords = GENX(3DSTATE_STREAMOUT_length) + (3 + 2 * max_decls);
   uint32_t *map = ralloc_size(NULL, sizeof(uint32_t) * dwords);
            iris_pack_command(GENX(3DSTATE_STREAMOUT), map, sol) {
      int urb_entry_read_offset = 0;
   int urb_entry_read_length = (vue_map->num_slots + 1) / 2 -
            /* We always read the whole vertex.  This could be reduced at some
   * point by reading less and offsetting the register index in the
   * SO_DECLs.
   */
   sol.Stream0VertexReadOffset = urb_entry_read_offset;
   sol.Stream0VertexReadLength = urb_entry_read_length - 1;
   sol.Stream1VertexReadOffset = urb_entry_read_offset;
   sol.Stream1VertexReadLength = urb_entry_read_length - 1;
   sol.Stream2VertexReadOffset = urb_entry_read_offset;
   sol.Stream2VertexReadLength = urb_entry_read_length - 1;
   sol.Stream3VertexReadOffset = urb_entry_read_offset;
            /* Set buffer pitches; 0 means unbound. */
   sol.Buffer0SurfacePitch = 4 * info->stride[0];
   sol.Buffer1SurfacePitch = 4 * info->stride[1];
   sol.Buffer2SurfacePitch = 4 * info->stride[2];
               iris_pack_command(GENX(3DSTATE_SO_DECL_LIST), so_decl_map, list) {
      list.DWordLength = 3 + 2 * max_decls - 2;
   list.StreamtoBufferSelects0 = buffer_mask[0];
   list.StreamtoBufferSelects1 = buffer_mask[1];
   list.StreamtoBufferSelects2 = buffer_mask[2];
   list.StreamtoBufferSelects3 = buffer_mask[3];
   list.NumEntries0 = decls[0];
   list.NumEntries1 = decls[1];
   list.NumEntries2 = decls[2];
               for (int i = 0; i < max_decls; i++) {
      iris_pack_state(GENX(SO_DECL_ENTRY), so_decl_map + 3 + i * 2, entry) {
      entry.Stream0Decl = so_decl[0][i];
   entry.Stream1Decl = so_decl[1][i];
   entry.Stream2Decl = so_decl[2][i];
                     }
      static void
   iris_compute_sbe_urb_read_interval(uint64_t fs_input_slots,
                           {
      /* The compiler computes the first URB slot without considering COL/BFC
   * swizzling (because it doesn't know whether it's enabled), so we need
   * to do that here too.  This may result in a smaller offset, which
   * should be safe.
   */
   const unsigned first_slot =
            /* This becomes the URB read offset (counted in pairs of slots). */
   assert(first_slot % 2 == 0);
            /* We need to adjust the inputs read to account for front/back color
   * swizzling, as it can make the URB length longer.
   */
   for (int c = 0; c <= 1; c++) {
      if (fs_input_slots & (VARYING_BIT_COL0 << c)) {
      /* If two sided color is enabled, the fragment shader's gl_Color
   * (COL0) input comes from either the gl_FrontColor (COL0) or
   * gl_BackColor (BFC0) input varyings.  Mark BFC as used, too.
   */
                  /* If front color isn't written, we opt to give them back color
   * instead of an undefined value.  Switch from COL to BFC.
   */
   if (last_vue_map->varying_to_slot[VARYING_SLOT_COL0 + c] == -1) {
      fs_input_slots &= ~(VARYING_BIT_COL0 << c);
                     /* Compute the minimum URB Read Length necessary for the FS inputs.
   *
   * From the Sandy Bridge PRM, Volume 2, Part 1, documentation for
   * 3DSTATE_SF DWord 1 bits 15:11, "Vertex URB Entry Read Length":
   *
   * "This field should be set to the minimum length required to read the
   *  maximum source attribute.  The maximum source attribute is indicated
   *  by the maximum value of the enabled Attribute # Source Attribute if
   *  Attribute Swizzle Enable is set, Number of Output Attributes-1 if
   *  enable is not set.
   *  read_length = ceiling((max_source_attr + 1) / 2)
   *
   *  [errata] Corruption/Hang possible if length programmed larger than
   *  recommended"
   *
   * Similar text exists for Ivy Bridge.
   *
   * We find the last URB slot that's actually read by the FS.
   */
   unsigned last_read_slot = last_vue_map->num_slots - 1;
   while (last_read_slot > first_slot && !(fs_input_slots &
                  /* The URB read length is the difference of the two, counted in pairs. */
      }
      static void
   iris_emit_sbe_swiz(struct iris_batch *batch,
                     const struct iris_context *ice,
   {
      struct GENX(SF_OUTPUT_ATTRIBUTE_DETAIL) attr_overrides[16] = {};
   const struct brw_wm_prog_data *wm_prog_data = (void *)
                           for (uint8_t idx = 0; idx < wm_prog_data->urb_setup_attribs_count; idx++) {
      const uint8_t fs_attr = wm_prog_data->urb_setup_attribs[idx];
   const int input_index = wm_prog_data->urb_setup[fs_attr];
   if (input_index < 0 || input_index >= 16)
            struct GENX(SF_OUTPUT_ATTRIBUTE_DETAIL) *attr =
                  /* Viewport and Layer are stored in the VUE header.  We need to override
   * them to zero if earlier stages didn't write them, as GL requires that
   * they read back as zero when not explicitly set.
   */
   switch (fs_attr) {
   case VARYING_SLOT_VIEWPORT:
   case VARYING_SLOT_LAYER:
      attr->ComponentOverrideX = true;
                  if (!(vue_map->slots_valid & VARYING_BIT_LAYER))
         if (!(vue_map->slots_valid & VARYING_BIT_VIEWPORT))
               default:
                  if (sprite_coord_enables & (1 << input_index))
            /* If there was only a back color written but not front, use back
   * as the color instead of undefined.
   */
   if (slot == -1 && fs_attr == VARYING_SLOT_COL0)
         if (slot == -1 && fs_attr == VARYING_SLOT_COL1)
            /* Not written by the previous stage - undefined. */
   if (slot == -1) {
      attr->ComponentOverrideX = true;
   attr->ComponentOverrideY = true;
   attr->ComponentOverrideZ = true;
   attr->ComponentOverrideW = true;
   attr->ConstantSource = CONST_0001_FLOAT;
               /* Compute the location of the attribute relative to the read offset,
   * which is counted in 256-bit increments (two 128-bit VUE slots).
   */
   const int source_attr = slot - 2 * urb_read_offset;
   assert(source_attr >= 0 && source_attr <= 32);
            /* If we are doing two-sided color, and the VUE slot following this one
   * represents a back-facing color, then we need to instruct the SF unit
   * to do back-facing swizzling.
   */
   if (cso_rast->light_twoside &&
      ((vue_map->slot_to_varying[slot] == VARYING_SLOT_COL0 &&
         (vue_map->slot_to_varying[slot] == VARYING_SLOT_COL1 &&
                  iris_emit_cmd(batch, GENX(3DSTATE_SBE_SWIZ), sbes) {
      for (int i = 0; i < 16; i++)
         }
      static bool
   iris_is_drawing_points(const struct iris_context *ice)
   {
               if (cso_rast->fill_mode_point) {
                  if (ice->shaders.prog[MESA_SHADER_GEOMETRY]) {
      const struct brw_gs_prog_data *gs_prog_data =
            } else if (ice->shaders.prog[MESA_SHADER_TESS_EVAL]) {
      const struct brw_tes_prog_data *tes_data =
            } else {
            }
      static unsigned
   iris_calculate_point_sprite_overrides(const struct brw_wm_prog_data *prog_data,
         {
               if (prog_data->urb_setup[VARYING_SLOT_PNTC] != -1)
            for (int i = 0; i < 8; i++) {
      if ((cso->sprite_coord_enable & (1 << i)) &&
      prog_data->urb_setup[VARYING_SLOT_TEX0 + i] != -1)
               }
      static void
   iris_emit_sbe(struct iris_batch *batch, const struct iris_context *ice)
   {
      const struct iris_rasterizer_state *cso_rast = ice->state.cso_rast;
   const struct brw_wm_prog_data *wm_prog_data = (void *)
         const struct brw_vue_map *last_vue_map =
            unsigned urb_read_offset, urb_read_length;
   iris_compute_sbe_urb_read_interval(wm_prog_data->inputs,
                        unsigned sprite_coord_overrides =
      iris_is_drawing_points(ice) ?
         iris_emit_cmd(batch, GENX(3DSTATE_SBE), sbe) {
      sbe.AttributeSwizzleEnable = true;
   sbe.NumberofSFOutputAttributes = wm_prog_data->num_varying_inputs;
   sbe.PointSpriteTextureCoordinateOrigin = cso_rast->sprite_coord_mode;
   sbe.VertexURBEntryReadOffset = urb_read_offset;
   sbe.VertexURBEntryReadLength = urb_read_length;
   sbe.ForceVertexURBEntryReadOffset = true;
   sbe.ForceVertexURBEntryReadLength = true;
   sbe.ConstantInterpolationEnable = wm_prog_data->flat_inputs;
   #if GFX_VER >= 9
         for (int i = 0; i < 32; i++) {
         #endif
            /* Ask the hardware to supply PrimitiveID if the fragment shader
   * reads it but a previous stage didn't write one.
   */
   if ((wm_prog_data->inputs & VARYING_BIT_PRIMITIVE_ID) &&
      last_vue_map->varying_to_slot[VARYING_SLOT_PRIMITIVE_ID] == -1) {
   sbe.PrimitiveIDOverrideAttributeSelect =
         sbe.PrimitiveIDOverrideComponentX = true;
   sbe.PrimitiveIDOverrideComponentY = true;
   sbe.PrimitiveIDOverrideComponentZ = true;
                  iris_emit_sbe_swiz(batch, ice, last_vue_map, urb_read_offset,
      }
      /* ------------------------------------------------------------------- */
      /**
   * Populate VS program key fields based on the current state.
   */
   static void
   iris_populate_vs_key(const struct iris_context *ice,
                     {
               if (info->clip_distance_array_size == 0 &&
      (info->outputs_written & (VARYING_BIT_POS | VARYING_BIT_CLIP_VERTEX)) &&
   last_stage == MESA_SHADER_VERTEX)
   }
      /**
   * Populate TCS program key fields based on the current state.
   */
   static void
   iris_populate_tcs_key(const struct iris_context *ice,
         {
   }
      /**
   * Populate TES program key fields based on the current state.
   */
   static void
   iris_populate_tes_key(const struct iris_context *ice,
                     {
               if (info->clip_distance_array_size == 0 &&
      (info->outputs_written & (VARYING_BIT_POS | VARYING_BIT_CLIP_VERTEX)) &&
   last_stage == MESA_SHADER_TESS_EVAL)
   }
      /**
   * Populate GS program key fields based on the current state.
   */
   static void
   iris_populate_gs_key(const struct iris_context *ice,
                     {
               if (info->clip_distance_array_size == 0 &&
      (info->outputs_written & (VARYING_BIT_POS | VARYING_BIT_CLIP_VERTEX)) &&
   last_stage == MESA_SHADER_GEOMETRY)
   }
      /**
   * Populate FS program key fields based on the current state.
   */
   static void
   iris_populate_fs_key(const struct iris_context *ice,
               {
      struct iris_screen *screen = (void *) ice->ctx.screen;
   const struct pipe_framebuffer_state *fb = &ice->state.framebuffer;
   const struct iris_depth_stencil_alpha_state *zsa = ice->state.cso_zsa;
   const struct iris_rasterizer_state *rast = ice->state.cso_rast;
                                                key->flat_shade = rast->flatshade &&
            key->persample_interp = rast->force_persample_interp;
                     key->force_dual_color_blend =
      screen->driconf.dual_color_blend_by_location &&
   }
      static void
   iris_populate_cs_key(const struct iris_context *ice,
         {
   }
      static uint64_t
   KSP(const struct iris_compiled_shader *shader)
   {
      struct iris_resource *res = (void *) shader->assembly.res;
      }
      static uint32_t
   encode_sampler_count(const struct iris_compiled_shader *shader)
   {
      uint32_t count = util_last_bit64(shader->bt.samplers_used_mask);
            /* We can potentially have way more than 32 samplers and that's ok.
   * However, the 3DSTATE_XS packets only have 3 bits to specify how
   * many to pre-fetch and all values above 4 are marked reserved.
   */
      }
      #define INIT_THREAD_DISPATCH_FIELDS(pkt, prefix, stage)                   \
      pkt.KernelStartPointer = KSP(shader);                                  \
   pkt.BindingTableEntryCount = shader->bt.size_bytes / 4;                \
   pkt.SamplerCount = encode_sampler_count(shader);                       \
   pkt.FloatingPointMode = prog_data->use_alt_mode;                       \
         pkt.DispatchGRFStartRegisterForURBData =                               \
         pkt.prefix##URBEntryReadLength = vue_prog_data->urb_read_length;       \
   pkt.prefix##URBEntryReadOffset = 0;                                    \
         pkt.StatisticsEnable = true;                                           \
   pkt.Enable           = true;                                           \
         if (prog_data->total_scratch) {                                        \
               #if GFX_VERx10 >= 125
   #define INIT_THREAD_SCRATCH_SIZE(pkt)
   #define MERGE_SCRATCH_ADDR(name)                                          \
   {                                                                         \
      uint32_t pkt2[GENX(name##_length)] = {0};                              \
   _iris_pack_command(batch, GENX(name), pkt2, p) {                       \
         }                                                                      \
      }
   #else
   #define INIT_THREAD_SCRATCH_SIZE(pkt)                                     \
         #define MERGE_SCRATCH_ADDR(name)                                          \
   {                                                                         \
      uint32_t pkt2[GENX(name##_length)] = {0};                              \
   _iris_pack_command(batch, GENX(name), pkt2, p) {                       \
      p.ScratchSpaceBasePointer =                                         \
      }                                                                      \
      }
   #endif
         /**
   * Encode most of 3DSTATE_VS based on the compiled shader.
   */
   static void
   iris_store_vs_state(const struct intel_device_info *devinfo,
         {
      struct brw_stage_prog_data *prog_data = shader->prog_data;
            iris_pack_command(GENX(3DSTATE_VS), shader->derived_data, vs) {
      INIT_THREAD_DISPATCH_FIELDS(vs, Vertex, MESA_SHADER_VERTEX);
   vs.MaximumNumberofThreads = devinfo->max_vs_threads - 1;
   vs.SIMD8DispatchEnable = true;
   vs.UserClipDistanceCullTestEnableBitmask =
         }
      /**
   * Encode most of 3DSTATE_HS based on the compiled shader.
   */
   static void
   iris_store_tcs_state(const struct intel_device_info *devinfo,
         {
      struct brw_stage_prog_data *prog_data = shader->prog_data;
   struct brw_vue_prog_data *vue_prog_data = (void *) prog_data;
            iris_pack_command(GENX(3DSTATE_HS), shader->derived_data, hs) {
         #if GFX_VER >= 12
         /* Wa_1604578095:
   *
   *    Hang occurs when the number of max threads is less than 2 times
   *    the number of instance count. The number of max threads must be
   *    more than 2 times the number of instance count.
   */
   assert((devinfo->max_tcs_threads / 2) > tcs_prog_data->instances);
   hs.DispatchGRFStartRegisterForURBData = prog_data->dispatch_grf_start_reg & 0x1f;
   #endif
            hs.InstanceCount = tcs_prog_data->instances - 1;
   hs.MaximumNumberofThreads = devinfo->max_tcs_threads - 1;
      #if GFX_VER == 12
         /* Patch Count threshold specifies the maximum number of patches that
   * will be accumulated before a thread dispatch is forced.
   */
   #endif
      #if GFX_VER >= 9
         hs.DispatchMode = vue_prog_data->dispatch_mode;
   #endif
         }
      /**
   * Encode 3DSTATE_TE and most of 3DSTATE_DS based on the compiled shader.
   */
   static void
   iris_store_tes_state(const struct intel_device_info *devinfo,
         {
      struct brw_stage_prog_data *prog_data = shader->prog_data;
   struct brw_vue_prog_data *vue_prog_data = (void *) prog_data;
            uint32_t *ds_state = (void *) shader->derived_data;
            iris_pack_command(GENX(3DSTATE_DS), ds_state, ds) {
               ds.DispatchMode = DISPATCH_MODE_SIMD8_SINGLE_PATCH;
   ds.MaximumNumberofThreads = devinfo->max_tes_threads - 1;
   ds.ComputeWCoordinateEnable =
      #if GFX_VER >= 12
         #endif
         ds.UserClipDistanceCullTestEnableBitmask =
               iris_pack_command(GENX(3DSTATE_TE), te_state, te) {
      te.Partitioning = tes_prog_data->partitioning;
   te.OutputTopology = tes_prog_data->output_topology;
   te.TEDomain = tes_prog_data->domain;
   te.TEEnable = true;
   te.MaximumTessellationFactorOdd = 63.0;
   #if GFX_VERx10 >= 125
         STATIC_ASSERT(TEDMODE_OFF == 0);
   if (intel_needs_workaround(devinfo, 14015055625)) {
         } else if (intel_needs_workaround(devinfo, 22012699309)) {
         } else {
                  te.TessellationDistributionLevel = TEDLEVEL_PATCH;
   /* 64_TRIANGLES */
   te.SmallPatchThreshold = 3;
   /* 1K_TRIANGLES */
   te.TargetBlockSize = 8;
   /* 1K_TRIANGLES */
   #endif
         }
      /**
   * Encode most of 3DSTATE_GS based on the compiled shader.
   */
   static void
   iris_store_gs_state(const struct intel_device_info *devinfo,
         {
      struct brw_stage_prog_data *prog_data = shader->prog_data;
   struct brw_vue_prog_data *vue_prog_data = (void *) prog_data;
            iris_pack_command(GENX(3DSTATE_GS), shader->derived_data, gs) {
               gs.OutputVertexSize = gs_prog_data->output_vertex_size_hwords * 2 - 1;
   gs.OutputTopology = gs_prog_data->output_topology;
   gs.ControlDataHeaderSize =
         gs.InstanceControl = gs_prog_data->invocations - 1;
   gs.DispatchMode = DISPATCH_MODE_SIMD8;
   gs.IncludePrimitiveID = gs_prog_data->include_primitive_id;
   gs.ControlDataFormat = gs_prog_data->control_data_format;
   gs.ReorderMode = TRAILING;
   gs.ExpectedVertexCount = gs_prog_data->vertices_in;
   gs.MaximumNumberofThreads =
                  if (gs_prog_data->static_vertex_count != -1) {
      gs.StaticOutput = true;
      }
            gs.UserClipDistanceCullTestEnableBitmask =
            const int urb_entry_write_offset = 1;
   const uint32_t urb_entry_output_length =
                  gs.VertexURBEntryOutputReadOffset = urb_entry_write_offset;
         }
      /**
   * Encode most of 3DSTATE_PS and 3DSTATE_PS_EXTRA based on the shader.
   */
   static void
   iris_store_fs_state(const struct intel_device_info *devinfo,
         {
      struct brw_stage_prog_data *prog_data = shader->prog_data;
            uint32_t *ps_state = (void *) shader->derived_data;
            iris_pack_command(GENX(3DSTATE_PS), ps_state, ps) {
      ps.VectorMaskEnable = wm_prog_data->uses_vmask;
   ps.BindingTableEntryCount = shader->bt.size_bytes / 4;
   ps.SamplerCount = encode_sampler_count(shader);
   ps.FloatingPointMode = prog_data->use_alt_mode;
   ps.MaximumNumberofThreadsPerPSD =
                     /* From the documentation for this packet:
   * "If the PS kernel does not need the Position XY Offsets to
   *  compute a Position Value, then this field should be programmed
   *  to POSOFFSET_NONE."
   *
   * "SW Recommendation: If the PS kernel needs the Position Offsets
   *  to compute a Position XY value, this field should match Position
   *  ZW Interpolation Mode to ensure a consistent position.xyzw
   *  computation."
   *
   * We only require XY sample offsets. So, this recommendation doesn't
   * look useful at the moment.  We might need this in future.
   */
   ps.PositionXYOffsetSelect =
            if (prog_data->total_scratch) {
                     iris_pack_command(GENX(3DSTATE_PS_EXTRA), psx_state, psx) {
      psx.PixelShaderValid = true;
   psx.PixelShaderComputedDepthMode = wm_prog_data->computed_depth_mode;
   psx.PixelShaderKillsPixel = wm_prog_data->uses_kill;
   psx.AttributeEnable = wm_prog_data->num_varying_inputs != 0;
   psx.PixelShaderUsesSourceDepth = wm_prog_data->uses_src_depth;
   psx.PixelShaderUsesSourceW = wm_prog_data->uses_src_w;
   psx.PixelShaderIsPerSample =
            #if GFX_VER >= 9
         psx.PixelShaderPullsBary = wm_prog_data->pulls_bary;
   #endif
         }
      /**
   * Compute the size of the derived data (shader command packets).
   *
   * This must match the data written by the iris_store_xs_state() functions.
   */
   static void
   iris_store_cs_state(const struct intel_device_info *devinfo,
         {
      struct brw_cs_prog_data *cs_prog_data = (void *) shader->prog_data;
               #if GFX_VERx10 < 125
         desc.ConstantURBEntryReadLength = cs_prog_data->push.per_thread.regs;
   desc.CrossThreadConstantDataReadLength =
   #else
         assert(cs_prog_data->push.per_thread.regs == 0);
   #endif
         desc.BarrierEnable = cs_prog_data->uses_barrier;
   /* Typically set to 0 to avoid prefetching on every thread dispatch. */
   desc.BindingTableEntryCount = devinfo->verx10 == 125 ?
         #if GFX_VER >= 12
         /* TODO: Check if we are missing workarounds and enable mid-thread
   * preemption.
   *
   * We still have issues with mid-thread preemption (it was already
   * disabled by the kernel on gfx11, due to missing workarounds). It's
   * possible that we are just missing some workarounds, and could enable
   * it later, but for now let's disable it to fix a GPU in compute in Car
   * Chase (and possibly more).
   */
   #endif
         }
      static unsigned
   iris_derived_program_state_size(enum iris_program_cache_id cache_id)
   {
               static const unsigned dwords[] = {
      [IRIS_CACHE_VS] = GENX(3DSTATE_VS_length),
   [IRIS_CACHE_TCS] = GENX(3DSTATE_HS_length),
   [IRIS_CACHE_TES] = GENX(3DSTATE_TE_length) + GENX(3DSTATE_DS_length),
   [IRIS_CACHE_GS] = GENX(3DSTATE_GS_length),
   [IRIS_CACHE_FS] =
         [IRIS_CACHE_CS] = GENX(INTERFACE_DESCRIPTOR_DATA_length),
                  }
      /**
   * Create any state packets corresponding to the given shader stage
   * (i.e. 3DSTATE_VS) and save them as "derived data" in the shader variant.
   * This means that we can look up a program in the in-memory cache and
   * get most of the state packet without having to reconstruct it.
   */
   static void
   iris_store_derived_program_state(const struct intel_device_info *devinfo,
               {
      switch (cache_id) {
   case IRIS_CACHE_VS:
      iris_store_vs_state(devinfo, shader);
      case IRIS_CACHE_TCS:
      iris_store_tcs_state(devinfo, shader);
      case IRIS_CACHE_TES:
      iris_store_tes_state(devinfo, shader);
      case IRIS_CACHE_GS:
      iris_store_gs_state(devinfo, shader);
      case IRIS_CACHE_FS:
      iris_store_fs_state(devinfo, shader);
      case IRIS_CACHE_CS:
      iris_store_cs_state(devinfo, shader);
      case IRIS_CACHE_BLORP:
            }
      /* ------------------------------------------------------------------- */
      static const uint32_t push_constant_opcodes[] = {
      [MESA_SHADER_VERTEX]    = 21,
   [MESA_SHADER_TESS_CTRL] = 25, /* HS */
   [MESA_SHADER_TESS_EVAL] = 26, /* DS */
   [MESA_SHADER_GEOMETRY]  = 22,
   [MESA_SHADER_FRAGMENT]  = 23,
      };
      static uint32_t
   use_null_surface(struct iris_batch *batch, struct iris_context *ice)
   {
                           }
      static uint32_t
   use_null_fb_surface(struct iris_batch *batch, struct iris_context *ice)
   {
      /* If set_framebuffer_state() was never called, fall back to 1x1x1 */
   if (!ice->state.null_fb.res)
                                 }
      static uint32_t
   surf_state_offset_for_aux(unsigned aux_modes,
         {
      assert(aux_modes & (1 << aux_usage));
   return SURFACE_STATE_ALIGNMENT *
      }
      #if GFX_VER == 9
   static void
   surf_state_update_clear_value(struct iris_batch *batch,
                     {
      struct isl_device *isl_dev = &batch->screen->isl_dev;
   struct iris_bo *state_bo = iris_resource_bo(surf_state->ref.res);
   uint64_t real_offset = surf_state->ref.offset + IRIS_MEMZONE_BINDER_START;
   uint32_t offset_into_bo = real_offset - state_bo->address;
   uint32_t clear_offset = offset_into_bo +
      isl_dev->ss.clear_value_offset +
                        if (aux_usage == ISL_AUX_USAGE_HIZ) {
      iris_emit_pipe_control_write(batch, "update fast clear value (Z)",
            } else {
      iris_emit_pipe_control_write(batch, "update fast clear color (RG__)",
                           iris_emit_pipe_control_write(batch, "update fast clear color (__BA)",
                                 iris_emit_pipe_control_flush(batch,
                  }
   #endif
      static void
   update_clear_value(struct iris_context *ice,
                     struct iris_batch *batch,
   {
      UNUSED struct isl_device *isl_dev = &batch->screen->isl_dev;
            /* We only need to update the clear color in the surface state for gfx8 and
   * gfx9. Newer gens can read it directly from the clear color state buffer.
      #if GFX_VER == 9
      /* Skip updating the ISL_AUX_USAGE_NONE surface state */
            while (aux_modes) {
                     #elif GFX_VER == 8
      /* TODO: Could update rather than re-filling */
                        #endif
   }
      static uint32_t
   use_surface_state(struct iris_batch *batch,
               {
      iris_use_pinned_bo(batch, iris_resource_bo(surf_state->ref.res), false,
            return surf_state->ref.offset +
      }
      /**
   * Add a surface to the validation list, as well as the buffer containing
   * the corresponding SURFACE_STATE.
   *
   * Returns the binding table entry (offset to SURFACE_STATE).
   */
   static uint32_t
   use_surface(struct iris_context *ice,
               struct iris_batch *batch,
   struct pipe_surface *p_surf,
   bool writeable,
   enum isl_aux_usage aux_usage,
   {
      struct iris_surface *surf = (void *) p_surf;
            if (GFX_VER == 8 && is_read_surface && !surf->surface_state_read.ref.res) {
      upload_surface_states(ice->state.surface_uploader,
               if (!surf->surface_state.ref.res) {
      upload_surface_states(ice->state.surface_uploader,
               if (memcmp(&res->aux.clear_color, &surf->clear_color,
            update_clear_value(ice, batch, res, &surf->surface_state, &surf->view);
   if (GFX_VER == 8) {
      update_clear_value(ice, batch, res, &surf->surface_state_read,
      }
               if (res->aux.clear_color_bo)
            if (res->aux.bo)
                     if (GFX_VER == 8 && is_read_surface) {
         } else {
            }
      static uint32_t
   use_sampler_view(struct iris_context *ice,
               {
      enum isl_aux_usage aux_usage =
      iris_resource_texture_aux_usage(ice, isv->res, isv->view.format,
         if (!isv->surface_state.ref.res)
            if (memcmp(&isv->res->aux.clear_color, &isv->clear_color,
            update_clear_value(ice, batch, isv->res, &isv->surface_state,
                     if (isv->res->aux.clear_color_bo) {
      iris_use_pinned_bo(batch, isv->res->aux.clear_color_bo,
               if (isv->res->aux.bo) {
      iris_use_pinned_bo(batch, isv->res->aux.bo,
                           }
      static uint32_t
   use_ubo_ssbo(struct iris_batch *batch,
               struct iris_context *ice,
   struct pipe_shader_buffer *buf,
   {
      if (!buf->buffer || !surf_state->res)
            iris_use_pinned_bo(batch, iris_resource_bo(buf->buffer), writable, access);
   iris_use_pinned_bo(batch, iris_resource_bo(surf_state->res), false,
               }
      static uint32_t
   use_image(struct iris_batch *batch, struct iris_context *ice,
               {
      struct iris_image_view *iv = &shs->image[i];
            if (!res)
                              if (res->aux.bo)
            if (res->aux.clear_color_bo) {
      iris_use_pinned_bo(batch, res->aux.clear_color_bo, false,
                           }
      #define push_bt_entry(addr) \
      assert(addr >= surf_base_offset); \
   assert(s < shader->bt.size_bytes / sizeof(uint32_t)); \
         #define bt_assert(section) \
      if (!pin_only && shader->bt.used_mask[section] != 0) \
         /**
   * Populate the binding table for a given shader stage.
   *
   * This fills out the table of pointers to surfaces required by the shader,
   * and also adds those buffers to the validation list so the kernel can make
   * resident before running our batch.
   */
   static void
   iris_populate_binding_table(struct iris_context *ice,
                     {
      const struct iris_binder *binder = &ice->state.binder;
   struct iris_compiled_shader *shader = ice->shaders.prog[stage];
   if (!shader)
            struct iris_binding_table *bt = &shader->bt;
   UNUSED struct brw_stage_prog_data *prog_data = shader->prog_data;
   struct iris_shader_state *shs = &ice->state.shaders[stage];
            uint32_t *bt_map = binder->map + binder->bt_offset[stage];
            const struct shader_info *info = iris_get_shader_info(ice, stage);
   if (!info) {
      /* TCS passthrough doesn't need a binding table. */
   assert(stage == MESA_SHADER_TESS_CTRL);
               if (stage == MESA_SHADER_COMPUTE &&
      shader->bt.used_mask[IRIS_SURFACE_GROUP_CS_WORK_GROUPS]) {
   /* surface for gl_NumWorkGroups */
   struct iris_state_ref *grid_data = &ice->state.grid_size;
   struct iris_state_ref *grid_state = &ice->state.grid_surf_state;
   iris_use_pinned_bo(batch, iris_resource_bo(grid_data->res), false,
         iris_use_pinned_bo(batch, iris_resource_bo(grid_state->res), false,
                     if (stage == MESA_SHADER_FRAGMENT) {
      struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   /* Note that cso_fb->nr_cbufs == fs_key->nr_color_regions. */
   if (cso_fb->nr_cbufs) {
      for (unsigned i = 0; i < cso_fb->nr_cbufs; i++) {
      uint32_t addr;
   if (cso_fb->cbufs[i]) {
      addr = use_surface(ice, batch, cso_fb->cbufs[i], true,
            } else {
         }
         } else if (GFX_VER < 11) {
      uint32_t addr = use_null_fb_surface(batch, ice);
               #define foreach_surface_used(index, group) \
      bt_assert(group); \
   for (int index = 0; index < bt->sizes[group]; index++) \
      if (iris_group_index_to_bti(bt, group, index) != \
         foreach_surface_used(i, IRIS_SURFACE_GROUP_RENDER_TARGET_READ) {
      struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   uint32_t addr;
   if (cso_fb->cbufs[i]) {
      addr = use_surface(ice, batch, cso_fb->cbufs[i],
                              foreach_surface_used(i, IRIS_SURFACE_GROUP_TEXTURE_LOW64) {
      struct iris_sampler_view *view = shs->textures[i];
   uint32_t addr = view ? use_sampler_view(ice, batch, view)
                     foreach_surface_used(i, IRIS_SURFACE_GROUP_TEXTURE_HIGH64) {
      struct iris_sampler_view *view = shs->textures[64 + i];
   uint32_t addr = view ? use_sampler_view(ice, batch, view)
                     foreach_surface_used(i, IRIS_SURFACE_GROUP_IMAGE) {
      uint32_t addr = use_image(batch, ice, shs, info, i);
               foreach_surface_used(i, IRIS_SURFACE_GROUP_UBO) {
      uint32_t addr = use_ubo_ssbo(batch, ice, &shs->constbuf[i],
                           foreach_surface_used(i, IRIS_SURFACE_GROUP_SSBO) {
      uint32_t addr =
      use_ubo_ssbo(batch, ice, &shs->ssbo[i], &shs->ssbo_surf_state[i],
               #if 0
         /* XXX: YUV surfaces not implemented yet */
   bt_assert(plane_start[1], ...);
   #endif
   }
      static void
   iris_use_optional_res(struct iris_batch *batch,
                     {
      if (res) {
      struct iris_bo *bo = iris_resource_bo(res);
         }
      static void
   pin_depth_and_stencil_buffers(struct iris_batch *batch,
               {
      if (!zsbuf)
            struct iris_resource *zres, *sres;
            if (zres) {
      iris_use_pinned_bo(batch, zres->bo, cso_zsa->depth_writes_enabled,
         if (zres->aux.bo) {
      iris_use_pinned_bo(batch, zres->aux.bo,
                        if (sres) {
      iris_use_pinned_bo(batch, sres->bo, cso_zsa->stencil_writes_enabled,
         }
      static uint32_t
   pin_scratch_space(struct iris_context *ice,
                     {
               if (prog_data->total_scratch > 0) {
      struct iris_bo *scratch_bo =
            #if GFX_VERx10 >= 125
         const struct iris_state_ref *ref =
         iris_use_pinned_bo(batch, iris_resource_bo(ref->res),
         scratch_addr = ref->offset +
               #else
         #endif
                  }
      /* ------------------------------------------------------------------- */
      /**
   * Pin any BOs which were installed by a previous batch, and restored
   * via the hardware logical context mechanism.
   *
   * We don't need to re-emit all state every batch - the hardware context
   * mechanism will save and restore it for us.  This includes pointers to
   * various BOs...which won't exist unless we ask the kernel to pin them
   * by adding them to the validation list.
   *
   * We can skip buffers if we've re-emitted those packets, as we're
   * overwriting those stale pointers with new ones, and don't actually
   * refer to the old BOs.
   */
   static void
   iris_restore_render_saved_bos(struct iris_context *ice,
               {
               const uint64_t clean = ~ice->state.dirty;
            if (clean & IRIS_DIRTY_CC_VIEWPORT) {
      iris_use_optional_res(batch, ice->state.last_res.cc_vp, false,
               if (clean & IRIS_DIRTY_SF_CL_VIEWPORT) {
      iris_use_optional_res(batch, ice->state.last_res.sf_cl_vp, false,
               if (clean & IRIS_DIRTY_BLEND_STATE) {
      iris_use_optional_res(batch, ice->state.last_res.blend, false,
               if (clean & IRIS_DIRTY_COLOR_CALC_STATE) {
      iris_use_optional_res(batch, ice->state.last_res.color_calc, false,
               if (clean & IRIS_DIRTY_SCISSOR_RECT) {
      iris_use_optional_res(batch, ice->state.last_res.scissor, false,
               if (ice->state.streamout_active && (clean & IRIS_DIRTY_SO_BUFFERS)) {
      for (int i = 0; i < 4; i++) {
      struct iris_stream_output_target *tgt =
         if (tgt) {
      iris_use_pinned_bo(batch, iris_resource_bo(tgt->base.buffer),
         iris_use_pinned_bo(batch, iris_resource_bo(tgt->offset.res),
                     for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (!(stage_clean & (IRIS_STAGE_DIRTY_CONSTANTS_VS << stage)))
            struct iris_shader_state *shs = &ice->state.shaders[stage];
            if (!shader)
                     for (int i = 0; i < 4; i++) {
                              /* Range block is a binding table index, map back to UBO index. */
   unsigned block_index = iris_bti_to_group_index(
                                 if (res)
         else
      iris_use_pinned_bo(batch, batch->screen->workaround_bo, false,
               for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (stage_clean & (IRIS_STAGE_DIRTY_BINDINGS_VS << stage)) {
      /* Re-pin any buffers referred to by the binding table. */
                  for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      struct iris_shader_state *shs = &ice->state.shaders[stage];
   struct pipe_resource *res = shs->sampler_table.res;
   if (res)
      iris_use_pinned_bo(batch, iris_resource_bo(res), false,
            for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (stage_clean & (IRIS_STAGE_DIRTY_VS << stage)) {
               if (shader) {
                                       if ((clean & IRIS_DIRTY_DEPTH_BUFFER) &&
      (clean & IRIS_DIRTY_WM_DEPTH_STENCIL)) {
   struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
               iris_use_optional_res(batch, ice->state.last_res.index_buffer, false,
            if (clean & IRIS_DIRTY_VERTEX_BUFFERS) {
      uint64_t bound = ice->state.bound_vertex_buffers;
   while (bound) {
      const int i = u_bit_scan64(&bound);
   struct pipe_resource *res = genx->vertex_buffers[i].resource;
   iris_use_pinned_bo(batch, iris_resource_bo(res), false,
               #if GFX_VERx10 == 125
      iris_use_pinned_bo(batch, iris_resource_bo(ice->state.pixel_hashing_tables),
      #else
         #endif
   }
      static void
   iris_restore_compute_saved_bos(struct iris_context *ice,
               {
               const int stage = MESA_SHADER_COMPUTE;
            if (stage_clean & IRIS_STAGE_DIRTY_BINDINGS_CS) {
      /* Re-pin any buffers referred to by the binding table. */
               struct pipe_resource *sampler_res = shs->sampler_table.res;
   if (sampler_res)
      iris_use_pinned_bo(batch, iris_resource_bo(sampler_res), false,
         if ((stage_clean & IRIS_STAGE_DIRTY_SAMPLER_STATES_CS) &&
      (stage_clean & IRIS_STAGE_DIRTY_BINDINGS_CS) &&
   (stage_clean & IRIS_STAGE_DIRTY_CONSTANTS_CS) &&
   (stage_clean & IRIS_STAGE_DIRTY_CS)) {
   iris_use_optional_res(batch, ice->state.last_res.cs_desc, false,
               if (stage_clean & IRIS_STAGE_DIRTY_CS) {
               if (shader) {
                     if (GFX_VERx10 < 125) {
      struct iris_bo *curbe_bo =
                              }
      /**
   * Possibly emit STATE_BASE_ADDRESS to update Surface State Base Address.
   */
   static void
   iris_update_binder_address(struct iris_batch *batch,
         {
      if (batch->last_binder_address == binder->bo->address)
            struct isl_device *isl_dev = &batch->screen->isl_dev;
                  #if GFX_VER >= 11
            #if GFX_VERx10 == 120
      /* Wa_1607854226:
   *
   *  Workaround the non pipelined state not applying in MEDIA/GPGPU pipeline
   *  mode by putting the pipeline temporarily in 3D mode..
   */
   if (batch->name == IRIS_BATCH_COMPUTE)
      #endif
         iris_emit_pipe_control_flush(batch, "Stall for binder realloc",
            iris_emit_cmd(batch, GENX(3DSTATE_BINDING_TABLE_POOL_ALLOC), btpa) {
      btpa.BindingTablePoolBaseAddress = ro_bo(binder->bo, 0);
   #if GFX_VERx10 < 125
         #endif
                  #if GFX_VERx10 == 120
      /* Wa_1607854226:
   *
   *  Put the pipeline back into compute mode.
   */
   if (batch->name == IRIS_BATCH_COMPUTE)
      #endif
   #else
      /* Use STATE_BASE_ADDRESS on older platforms */
            iris_emit_cmd(batch, GENX(STATE_BASE_ADDRESS), sba) {
      sba.SurfaceStateBaseAddressModifyEnable = true;
            /* The hardware appears to pay attention to the MOCS fields even
   * if you don't set the "Address Modify Enable" bit for the base.
   */
   sba.GeneralStateMOCS            = mocs;
   sba.StatelessDataPortAccessMOCS = mocs;
   sba.DynamicStateMOCS            = mocs;
   sba.IndirectObjectMOCS          = mocs;
   sba.InstructionMOCS             = mocs;
   #if GFX_VER >= 9
         #endif
   #if GFX_VERx10 >= 125
         #endif
         #endif
         flush_after_state_base_change(batch);
               }
      static inline void
   iris_viewport_zmin_zmax(const struct pipe_viewport_state *vp, bool halfz,
         {
      if (window_space_position) {
      *zmin = 0.f;
   *zmax = 1.f;
      }
      }
      #if GFX_VER >= 12
   static void
   invalidate_aux_map_state_per_engine(struct iris_batch *batch)
   {
               switch (batch->name) {
   case IRIS_BATCH_RENDER: {
      /* HSD 1209978178: docs say that before programming the aux table:
   *
   *    "Driver must ensure that the engine is IDLE but ensure it doesn't
   *    add extra flushes in the case it knows that the engine is already
   *    IDLE."
   *
   * An end of pipe sync is needed here, otherwise we see GPU hangs in
   * dEQP-GLES31.functional.copy_image.* tests.
   *
   * HSD 22012751911: SW Programming sequence when issuing aux invalidation:
   *
   *    "Render target Cache Flush + L3 Fabric Flush + State Invalidation + CS Stall"
   *
   * Notice we don't set the L3 Fabric Flush here, because we have
   * PIPE_CONTROL_CS_STALL. The PIPE_CONTROL::L3 Fabric Flush
   * documentation says :
   *
   *    "L3 Fabric Flush will ensure all the pending transactions in the
   *     L3 Fabric are flushed to global observation point. HW does
   *     implicit L3 Fabric Flush on all stalling flushes (both explicit
   *     and implicit) and on PIPECONTROL having Post Sync Operation
   *     enabled."
   *
   * Therefore setting L3 Fabric Flush here would be redundant.
   *
   * From Bspec 43904 (Register_CCSAuxiliaryTableInvalidate):
   * RCS engine idle sequence:
   *
   *    Gfx125+:
   *       PIPE_CONTROL:- DC Flush + L3 Fabric Flush + CS Stall + Render
   *                      Target Cache Flush + Depth Cache + CCS flush
   *
   */
   iris_emit_end_of_pipe_sync(batch, "Invalidate aux map table",
                                    register_addr = GENX(GFX_CCS_AUX_INV_num);
      }
   case IRIS_BATCH_COMPUTE: {
      /*
   * Notice we don't set the L3 Fabric Flush here, because we have
   * PIPE_CONTROL_CS_STALL. The PIPE_CONTROL::L3 Fabric Flush
   * documentation says :
   *
   *    "L3 Fabric Flush will ensure all the pending transactions in the
   *     L3 Fabric are flushed to global observation point. HW does
   *     implicit L3 Fabric Flush on all stalling flushes (both explicit
   *     and implicit) and on PIPECONTROL having Post Sync Operation
   *     enabled."
   *
   * Therefore setting L3 Fabric Flush here would be redundant.
   *
   * From Bspec 43904 (Register_CCSAuxiliaryTableInvalidate):
   * Compute engine idle sequence:
   *
   *    Gfx125+:
   *       PIPE_CONTROL:- DC Flush + L3 Fabric Flush + CS Stall + CCS flush
   */
   iris_emit_end_of_pipe_sync(batch, "Invalidate aux map table",
                              register_addr = GENX(COMPCS0_CCS_AUX_INV_num);
      }
      #if GFX_VERx10 >= 125
         /*
   * Notice we don't set the L3 Fabric Flush here, because we have
   * PIPE_CONTROL_CS_STALL. The PIPE_CONTROL::L3 Fabric Flush
   * documentation says :
   *
   *    "L3 Fabric Flush will ensure all the pending transactions in the
   *     L3 Fabric are flushed to global observation point. HW does
   *     implicit L3 Fabric Flush on all stalling flushes (both explicit
   *     and implicit) and on PIPECONTROL having Post Sync Operation
   *     enabled."
   *
   * Therefore setting L3 Fabric Flush here would be redundant.
   *
   * From Bspec 43904 (Register_CCSAuxiliaryTableInvalidate):
   * Blitter engine idle sequence:
   *
   *    Gfx125+:
   *       MI_FLUSH_DW (dw0;b16 – flush CCS)
   */
   iris_emit_cmd(batch, GENX(MI_FLUSH_DW), fd) {
         }
   #endif
            }
   default:
      unreachable("Invalid batch for aux map invalidation");
               if (register_addr != 0) {
      /* If the aux-map state number increased, then we need to rewrite the
   * register. Rewriting the register is used to both set the aux-map
   * translation table address, and also to invalidate any previously
   * cached translations.
   */
            /* HSD 22012751911: SW Programming sequence when issuing aux invalidation:
   *
   *    "Poll Aux Invalidation bit once the invalidation is set (Register
   *     4208 bit 0)"
   */
   iris_emit_cmd(batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.CompareOperation = COMPARE_SAD_EQUAL_SDD;
   sem.WaitMode = PollingMode;
   sem.RegisterPollMode = true;
   sem.SemaphoreDataDword = 0x0;
            }
      void
   genX(invalidate_aux_map_state)(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
   void *aux_map_ctx = iris_bufmgr_get_aux_map_context(screen->bufmgr);
   if (!aux_map_ctx)
         uint32_t aux_map_state_num = intel_aux_map_get_state_num(aux_map_ctx);
   if (batch->last_aux_map_state != aux_map_state_num) {
      invalidate_aux_map_state_per_engine(batch);
         }
      static void
   init_aux_map_state(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
   void *aux_map_ctx = iris_bufmgr_get_aux_map_context(screen->bufmgr);
   if (!aux_map_ctx)
            uint64_t base_addr = intel_aux_map_get_base(aux_map_ctx);
   assert(base_addr != 0 && align64(base_addr, 32 * 1024) == base_addr);
   iris_load_register_imm64(batch, GENX(GFX_AUX_TABLE_BASE_ADDR_num),
      }
   #endif
      struct push_bos {
      struct {
      struct iris_address addr;
      } buffers[4];
   int buffer_count;
      };
      static void
   setup_constant_buffers(struct iris_context *ice,
                     {
      struct iris_shader_state *shs = &ice->state.shaders[stage];
   struct iris_compiled_shader *shader = ice->shaders.prog[stage];
                     int n = 0;
   for (int i = 0; i < 4; i++) {
               if (range->length == 0)
                     if (range->length > push_bos->max_length)
            /* Range block is a binding table index, map back to UBO index. */
   unsigned block_index = iris_bti_to_group_index(
                  struct pipe_shader_buffer *cbuf = &shs->constbuf[block_index];
                     if (res)
            push_bos->buffers[n].length = range->length;
   push_bos->buffers[n].addr =
      res ? ro_bo(res->bo, range->start * 32 + cbuf->buffer_offset)
                  /* From the 3DSTATE_CONSTANT_XS and 3DSTATE_CONSTANT_ALL programming notes:
   *
   *    "The sum of all four read length fields must be less than or
   *    equal to the size of 64."
   */
               }
      static void
   emit_push_constant_packets(struct iris_context *ice,
                     {
      UNUSED struct isl_device *isl_dev = &batch->screen->isl_dev;
   struct iris_compiled_shader *shader = ice->shaders.prog[stage];
            iris_emit_cmd(batch, GENX(3DSTATE_CONSTANT_VS), pkt) {
         #if GFX_VER >= 9
         #endif
            if (prog_data) {
      /* The Skylake PRM contains the following restriction:
   *
   *    "The driver must ensure The following case does not occur
   *     without a flush to the 3D engine: 3DSTATE_CONSTANT_* with
   *     buffer 3 read length equal to zero committed followed by a
   *     3DSTATE_CONSTANT_* with buffer 0 read length not equal to
   *     zero committed."
   *
   * To avoid this, we program the buffers in the highest slots.
   * This way, slot 0 is only used if slot 3 is also used.
   */
   int n = push_bos->buffer_count;
   assert(n <= 4);
   const unsigned shift = 4 - n;
   for (int i = 0; i < n; i++) {
      pkt.ConstantBody.ReadLength[i + shift] =
                     }
      #if GFX_VER >= 12
   static void
   emit_push_constant_packet_all(struct iris_context *ice,
                     {
               if (!push_bos) {
      iris_emit_cmd(batch, GENX(3DSTATE_CONSTANT_ALL), pc) {
      pc.ShaderUpdateEnable = shader_mask;
      }
               const uint32_t n = push_bos->buffer_count;
   const uint32_t max_pointers = 4;
   const uint32_t num_dwords = 2 + 2 * n;
   uint32_t const_all[2 + 2 * max_pointers];
            assert(n <= max_pointers);
   iris_pack_command(GENX(3DSTATE_CONSTANT_ALL), dw, all) {
      all.DWordLength = num_dwords - 2;
   all.MOCS = isl_mocs(isl_dev, 0, false);
   all.ShaderUpdateEnable = shader_mask;
      }
            for (int i = 0; i < n; i++) {
      _iris_pack_state(batch, GENX(3DSTATE_CONSTANT_ALL_DATA),
            data.PointerToConstantBuffer = push_bos->buffers[i].addr;
         }
      }
   #endif
      void
   genX(emit_depth_state_workarounds)(struct iris_context *ice,
               {
   #if INTEL_NEEDS_WA_1808121037
      const bool is_d16_1x_msaa = surf->format == ISL_FORMAT_R16_UNORM &&
            switch (ice->state.genx->depth_reg_mode) {
   case IRIS_DEPTH_REG_MODE_HW_DEFAULT:
      if (!is_d16_1x_msaa)
            case IRIS_DEPTH_REG_MODE_D16_1X_MSAA:
      if (is_d16_1x_msaa)
            case IRIS_DEPTH_REG_MODE_UNKNOWN:
                  /* We'll change some CHICKEN registers depending on the depth surface
   * format. Do a depth flush and stall so the pipeline is not using these
   * settings while we change the registers.
   */
   iris_emit_end_of_pipe_sync(batch,
                        /* Wa_1808121037
   *
   * To avoid sporadic corruptions “Set 0x7010[9] when Depth Buffer
   * Surface Format is D16_UNORM , surface type is not NULL & 1X_MSAA”.
   */
   iris_emit_reg(batch, GENX(COMMON_SLICE_CHICKEN1), reg) {
      reg.HIZPlaneOptimizationdisablebit = is_d16_1x_msaa;
               ice->state.genx->depth_reg_mode =
      is_d16_1x_msaa ? IRIS_DEPTH_REG_MODE_D16_1X_MSAA :
   #endif
   }
      static void
   iris_preemption_streamout_wa(struct iris_context *ice,
               {
   #if GFX_VERx10 >= 120
      if (!intel_needs_workaround(batch->screen->devinfo, 16013994831))
            iris_emit_reg(batch, GENX(CS_CHICKEN1), reg) {
      reg.DisablePreemptionandHighPriorityPausingdueto3DPRIMITIVECommand = !enable;
               /* Emit CS_STALL and 250 noops. */
   iris_emit_pipe_control_flush(batch, "workaround: Wa_16013994831",
         for (unsigned i = 0; i < 250; i++)
               #endif
   }
      static void
   shader_program_needs_wa_14015055625(struct iris_context *ice,
                           {
      if (!intel_needs_workaround(batch->screen->devinfo, 14015055625))
            switch (stage) {
   case MESA_SHADER_TESS_CTRL: {
      struct brw_tcs_prog_data *tcs_prog_data = (void *) prog_data;
   *program_needs_wa_14015055625 |= tcs_prog_data->include_primitive_id;
      }
   case MESA_SHADER_TESS_EVAL: {
      struct brw_tes_prog_data *tes_prog_data = (void *) prog_data;
   *program_needs_wa_14015055625 |= tes_prog_data->include_primitive_id;
      }
   default:
                  struct iris_compiled_shader *gs_shader =
         const struct brw_gs_prog_data *gs_prog_data =
            *program_needs_wa_14015055625 |=
      }
      static void
   iris_upload_dirty_render_state(struct iris_context *ice,
               {
      struct iris_screen *screen = batch->screen;
   struct iris_border_color_pool *border_color_pool =
            /* Re-emit 3DSTATE_DS before any 3DPRIMITIVE when tessellation is on */
   /* FIXME: WA framework doesn't know about 14019750404 yet.
   * if (intel_needs_workaround(batch->screen->devinfo, 14019750404) &&
   *     ice->shaders.prog[MESA_SHADER_TESS_EVAL])
   */
   if (batch->screen->devinfo->has_mesh_shading &&
      ice->shaders.prog[MESA_SHADER_TESS_EVAL])
         const uint64_t dirty = ice->state.dirty;
            if (!(dirty & IRIS_ALL_DIRTY_FOR_RENDER) &&
      !(stage_dirty & IRIS_ALL_STAGE_DIRTY_FOR_RENDER))
         struct iris_genx_state *genx = ice->state.genx;
   struct iris_binder *binder = &ice->state.binder;
   struct brw_wm_prog_data *wm_prog_data = (void *)
            /* When MSAA is enabled, instead of using BLENDFACTOR_ZERO use
   * CONST_COLOR, CONST_ALPHA and supply zero by using blend constants.
   */
   bool needs_wa_14018912822 =
      screen->driconf.intel_enable_wa_14018912822 &&
   intel_needs_workaround(batch->screen->devinfo, 14018912822) &&
         if (dirty & IRIS_DIRTY_CC_VIEWPORT) {
      const struct iris_rasterizer_state *cso_rast = ice->state.cso_rast;
            /* XXX: could avoid streaming for depth_clip [0,1] case. */
   uint32_t *cc_vp_map =
      stream_state(batch, ice->state.dynamic_uploader,
                  for (int i = 0; i < ice->state.num_viewports; i++) {
      float zmin, zmax;
   iris_viewport_zmin_zmax(&ice->state.viewports[i], cso_rast->clip_halfz,
               if (cso_rast->depth_clip_near)
                        iris_pack_state(GENX(CC_VIEWPORT), cc_vp_map, ccv) {
      ccv.MinimumDepth = zmin;
                           iris_emit_cmd(batch, GENX(3DSTATE_VIEWPORT_STATE_POINTERS_CC), ptr) {
                     if (dirty & IRIS_DIRTY_SF_CL_VIEWPORT) {
      struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   uint32_t sf_cl_vp_address;
   uint32_t *vp_map =
      stream_state(batch, ice->state.dynamic_uploader,
                     for (unsigned i = 0; i < ice->state.num_viewports; i++) {
                     float vp_xmin = viewport_extent(state, 0, -1.0f);
   float vp_xmax = viewport_extent(state, 0,  1.0f);
                  intel_calculate_guardband_size(0, cso_fb->width, 0, cso_fb->height,
                        iris_pack_state(GENX(SF_CLIP_VIEWPORT), vp_map, vp) {
      vp.ViewportMatrixElementm00 = state->scale[0];
   vp.ViewportMatrixElementm11 = state->scale[1];
   vp.ViewportMatrixElementm22 = state->scale[2];
   vp.ViewportMatrixElementm30 = state->translate[0];
   vp.ViewportMatrixElementm31 = state->translate[1];
   vp.ViewportMatrixElementm32 = state->translate[2];
   vp.XMinClipGuardband = gb_xmin;
   vp.XMaxClipGuardband = gb_xmax;
   vp.YMinClipGuardband = gb_ymin;
   vp.YMaxClipGuardband = gb_ymax;
   vp.XMinViewPort = MAX2(vp_xmin, 0);
   vp.XMaxViewPort = MIN2(vp_xmax, cso_fb->width) - 1;
   vp.YMinViewPort = MAX2(vp_ymin, 0);
                           iris_emit_cmd(batch, GENX(3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP), ptr) {
                     if (dirty & IRIS_DIRTY_URB) {
      for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_GEOMETRY; i++) {
      if (!ice->shaders.prog[i]) {
         } else {
      struct brw_vue_prog_data *vue_prog_data =
            }
               intel_get_urb_config(screen->devinfo,
                        screen->l3_config_3d,
   ice->shaders.prog[MESA_SHADER_TESS_EVAL] != NULL,
   ice->shaders.prog[MESA_SHADER_GEOMETRY] != NULL,
               for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_GEOMETRY; i++) {
      iris_emit_cmd(batch, GENX(3DSTATE_URB_VS), urb) {
      urb._3DCommandSubOpcode += i;
   urb.VSURBStartingAddress     = ice->shaders.urb.start[i];
   urb.VSURBEntryAllocationSize = ice->shaders.urb.size[i] - 1;
                     if (dirty & IRIS_DIRTY_BLEND_STATE) {
      struct iris_blend_state *cso_blend = ice->state.cso_blend;
   struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   struct iris_depth_stencil_alpha_state *cso_zsa = ice->state.cso_zsa;
            bool color_blend_zero = false;
            /* Always write at least one BLEND_STATE - the final RT message will
   * reference BLEND_STATE[0] even if there aren't color writes.  There
   * may still be alpha testing, computed depth, and so on.
   */
   const int rt_dwords =
            uint32_t blend_offset;
   uint32_t *blend_map =
      stream_state(batch, ice->state.dynamic_uploader,
               /* Copy of blend entries for merging dynamic changes. */
   uint32_t blend_entries[4 * rt_dwords];
                     uint32_t *blend_entry = blend_entries;
   for (unsigned i = 0; i < cbufs; i++) {
      int dst_blend_factor = cso_blend->ps_dst_blend_factor[i];
   int dst_alpha_blend_factor = cso_blend->ps_dst_alpha_blend_factor[i];
   uint32_t entry[GENX(BLEND_STATE_ENTRY_length)];
   iris_pack_state(GENX(BLEND_STATE_ENTRY), entry, be) {
      if (needs_wa_14018912822) {
      if (dst_blend_factor == BLENDFACTOR_ZERO) {
      dst_blend_factor = BLENDFACTOR_CONST_COLOR;
      }
   if (dst_alpha_blend_factor == BLENDFACTOR_ZERO) {
      dst_alpha_blend_factor = BLENDFACTOR_CONST_ALPHA;
         }
   be.DestinationBlendFactor = dst_blend_factor;
               /* Merge entry. */
   uint32_t *dst = blend_entry;
   uint32_t *src = entry;
                              /* Blend constants modified for Wa_14018912822. */
   if (ice->state.color_blend_zero != color_blend_zero) {
      ice->state.color_blend_zero = color_blend_zero;
      }
   if (ice->state.alpha_blend_zero != alpha_blend_zero) {
      ice->state.alpha_blend_zero = alpha_blend_zero;
               uint32_t blend_state_header;
   iris_pack_state(GENX(BLEND_STATE), &blend_state_header, bs) {
      bs.AlphaTestEnable = cso_zsa->alpha_enabled;
               blend_map[0] = blend_state_header | cso_blend->blend_state[0];
            iris_emit_cmd(batch, GENX(3DSTATE_BLEND_STATE_POINTERS), ptr) {
      ptr.BlendStatePointer = blend_offset;
                  if (dirty & IRIS_DIRTY_COLOR_CALC_STATE) {
      #if GFX_VER == 8
         #endif
         uint32_t cc_offset;
   void *cc_map =
      stream_state(batch, ice->state.dynamic_uploader,
                  iris_pack_state(GENX(COLOR_CALC_STATE), cc_map, cc) {
      cc.AlphaTestFormat = ALPHATEST_FLOAT32;
   cc.AlphaReferenceValueAsFLOAT32 = cso->alpha_ref_value;
   cc.BlendConstantColorRed   = ice->state.color_blend_zero ?
         cc.BlendConstantColorGreen = ice->state.color_blend_zero ?
         cc.BlendConstantColorBlue  = ice->state.color_blend_zero ?
            #if GFX_VER == 8
   cc.StencilReferenceValue = p_stencil_refs->ref_value[0];
   cc.BackfaceStencilReferenceValue = p_stencil_refs->ref_value[1];
   #endif
         }
   iris_emit_cmd(batch, GENX(3DSTATE_CC_STATE_POINTERS), ptr) {
      ptr.ColorCalcStatePointer = cc_offset;
                  /* Wa_1604061319
   *
   *    3DSTATE_CONSTANT_* needs to be programmed before BTP_*
   *
   * Testing shows that all the 3DSTATE_CONSTANT_XS need to be emitted if
   * any stage has a dirty binding table.
   */
   const bool emit_const_wa = GFX_VER >= 11 &&
      ((dirty & IRIS_DIRTY_RENDER_BUFFER) ||
      #if GFX_VER >= 12
         #endif
         for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (!(stage_dirty & (IRIS_STAGE_DIRTY_CONSTANTS_VS << stage)) &&
                  struct iris_shader_state *shs = &ice->state.shaders[stage];
            if (!shader)
            if (shs->sysvals_need_upload)
            struct push_bos push_bos = {};
      #if GFX_VER >= 12
         /* If this stage doesn't have any push constants, emit it later in a
   * single CONSTANT_ALL packet with all the other stages.
   */
   if (push_bos.buffer_count == 0) {
      nobuffer_stages |= 1 << stage;
               /* The Constant Buffer Read Length field from 3DSTATE_CONSTANT_ALL
   * contains only 5 bits, so we can only use it for buffers smaller than
   * 32.
   *
   * According to Wa_16011448509, Gfx12.0 misinterprets some address bits
   * in 3DSTATE_CONSTANT_ALL.  It should still be safe to use the command
   * for disabling stages, where all address bits are zero.  However, we
   * can't safely use it for general buffers with arbitrary addresses.
   * Just fall back to the individual 3DSTATE_CONSTANT_XS commands in that
   * case.
   */
   if (push_bos.max_length < 32 && GFX_VERx10 > 120) {
      emit_push_constant_packet_all(ice, batch, 1 << stage, &push_bos);
      #endif
                  #if GFX_VER >= 12
      if (nobuffer_stages)
      /* Wa_16011448509: all address bits are zero */
   #endif
         for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      /* Gfx9 requires 3DSTATE_BINDING_TABLE_POINTERS_XS to be re-emitted
   * in order to commit constants.  TODO: Investigate "Disable Gather
   * at Set Shader" to go back to legacy mode...
   */
   if (stage_dirty & ((IRIS_STAGE_DIRTY_BINDINGS_VS |
                  iris_emit_cmd(batch, GENX(3DSTATE_BINDING_TABLE_POINTERS_VS), ptr) {
      ptr._3DCommandSubOpcode = 38 + stage;
   ptr.PointertoVSBindingTable =
                     if (GFX_VER >= 11 && (dirty & IRIS_DIRTY_RENDER_BUFFER)) {
      // XXX: we may want to flag IRIS_DIRTY_MULTISAMPLE (or SAMPLE_MASK?)
            /* The PIPE_CONTROL command description says:
   *
   *   "Whenever a Binding Table Index (BTI) used by a Render Target
   *    Message points to a different RENDER_SURFACE_STATE, SW must issue a
   *    Render Target Cache Flush by enabling this bit. When render target
   *    flush is set due to new association of BTI, PS Scoreboard Stall bit
   *    must be set in this packet."
   */
   // XXX: does this need to happen at 3DSTATE_BTP_PS time?
   iris_emit_pipe_control_flush(batch, "workaround: RT BTI change [draw]",
                     if (dirty & IRIS_DIRTY_RENDER_BUFFER)
            for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (stage_dirty & (IRIS_STAGE_DIRTY_BINDINGS_VS << stage)) {
                     for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (!(stage_dirty & (IRIS_STAGE_DIRTY_SAMPLER_STATES_VS << stage)) ||
                           struct iris_shader_state *shs = &ice->state.shaders[stage];
   struct pipe_resource *res = shs->sampler_table.res;
   if (res)
                  iris_emit_cmd(batch, GENX(3DSTATE_SAMPLER_STATE_POINTERS_VS), ptr) {
      ptr._3DCommandSubOpcode = 43 + stage;
                  if (ice->state.need_border_colors)
            if (dirty & IRIS_DIRTY_MULTISAMPLE) {
      iris_emit_cmd(batch, GENX(3DSTATE_MULTISAMPLE), ms) {
      ms.PixelLocation =
         if (ice->state.framebuffer.samples > 0)
                  if (dirty & IRIS_DIRTY_SAMPLE_MASK) {
      iris_emit_cmd(batch, GENX(3DSTATE_SAMPLE_MASK), ms) {
                           #if INTEL_WA_14015055625_GFX_VER
      /* Check if FS stage will use primitive ID overrides for Wa_14015055625. */
   const struct brw_vue_map *last_vue_map =
         if ((wm_prog_data->inputs & VARYING_BIT_PRIMITIVE_ID) &&
      last_vue_map->varying_to_slot[VARYING_SLOT_PRIMITIVE_ID] == -1 &&
   intel_needs_workaround(batch->screen->devinfo, 14015055625)) {
         #endif
         for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      if (!(stage_dirty & (IRIS_STAGE_DIRTY_VS << stage)))
                     if (shader) {
      struct brw_stage_prog_data *prog_data = shader->prog_data;
                        #if INTEL_WA_14015055625_GFX_VER
               #endif
               if (stage == MESA_SHADER_FRAGMENT) {
                     uint32_t ps_state[GENX(3DSTATE_PS_length)] = {0};
   _iris_pack_command(batch, GENX(3DSTATE_PS), ps_state, ps) {
                           ps.DispatchGRFStartRegisterForConstantSetupData0 =
         ps.DispatchGRFStartRegisterForConstantSetupData1 =
                        ps.KernelStartPointer0 = KSP(shader) +
         ps.KernelStartPointer1 = KSP(shader) +
         #if GFX_VERx10 >= 125
         #else
               #endif
                        #if GFX_VER >= 9
                  if (!wm_prog_data->uses_sample_mask)
         else if (wm_prog_data->post_depth_coverage)
         else if (wm_prog_data->inner_coverage &&
            #else
               #endif
                        uint32_t *shader_ps = (uint32_t *) shader->derived_data;
   uint32_t *shader_psx = shader_ps + GENX(3DSTATE_PS_length);
   iris_emit_merge(batch, shader_ps, ps_state,
         iris_emit_merge(batch, shader_psx, psx_state,
      } else if (stage == MESA_SHADER_TESS_EVAL &&
            intel_needs_workaround(batch->screen->devinfo, 14015055625) &&
   /* This program doesn't require Wa_14015055625, so we can enable
   #if GFX_VERx10 >= 125
               uint32_t te_state[GENX(3DSTATE_TE_length)] = { 0 };
   iris_pack_command(GENX(3DSTATE_TE), te_state, te) {
      if (intel_needs_workaround(batch->screen->devinfo, 22012699309))
                           uint32_t ds_state[GENX(3DSTATE_DS_length)] = { 0 };
   iris_pack_command(GENX(3DSTATE_DS), ds_state, ds) {
                                       iris_emit_merge(batch, shader_ds, ds_state,
         #endif
            } else if (scratch_addr) {
      uint32_t *pkt = (uint32_t *) shader->derived_data;
   switch (stage) {
   case MESA_SHADER_VERTEX:    MERGE_SCRATCH_ADDR(3DSTATE_VS); break;
   case MESA_SHADER_TESS_CTRL: MERGE_SCRATCH_ADDR(3DSTATE_HS); break;
   case MESA_SHADER_TESS_EVAL: MERGE_SCRATCH_ADDR(3DSTATE_DS); break;
   case MESA_SHADER_GEOMETRY:  MERGE_SCRATCH_ADDR(3DSTATE_GS); break;
      } else {
      iris_batch_emit(batch, shader->derived_data,
         } else {
      if (stage == MESA_SHADER_TESS_EVAL) {
      iris_emit_cmd(batch, GENX(3DSTATE_HS), hs);
   iris_emit_cmd(batch, GENX(3DSTATE_TE), te);
      } else if (stage == MESA_SHADER_GEOMETRY) {
                        if (ice->state.streamout_active) {
      if (dirty & IRIS_DIRTY_SO_BUFFERS) {
      /* Wa_16011411144
   * SW must insert a PIPE_CONTROL cmd before and after the
   * 3dstate_so_buffer_index_0/1/2/3 states to ensure so_buffer_index_* state is
   * not combined with other state changes.
   */
   if (intel_device_info_is_dg2(batch->screen->devinfo)) {
      iris_emit_pipe_control_flush(batch,
                     for (int i = 0; i < 4; i++) {
      struct iris_stream_output_target *tgt =
         enum { dwords = GENX(3DSTATE_SO_BUFFER_length) };
                  if (tgt) {
      zero_offset = tgt->zero_offset;
   iris_use_pinned_bo(batch, iris_resource_bo(tgt->base.buffer),
                           if (zero_offset) {
      /* Skip the last DWord which contains "Stream Offset" of
   * 0xFFFFFFFF and instead emit a dword of zero directly.
   */
   STATIC_ASSERT(GENX(3DSTATE_SO_BUFFER_StreamOffset_start) ==
         const uint32_t zero = 0;
   iris_batch_emit(batch, so_buffers, 4 * (dwords - 1));
   iris_batch_emit(batch, &zero, sizeof(zero));
      } else {
                     /* Wa_16011411144 */
   if (intel_device_info_is_dg2(batch->screen->devinfo)) {
      iris_emit_pipe_control_flush(batch,
                        if ((dirty & IRIS_DIRTY_SO_DECL_LIST) && ice->state.streamout) {
      /* Wa_16011773973:
   * If SOL is enabled and SO_DECL state has to be programmed,
   *    1. Send 3D State SOL state with SOL disabled
   *    2. Send SO_DECL NP state
   *    3. Send 3D State SOL with SOL Enabled
   */
                  uint32_t *decl_list =
         #if GFX_VER >= 11
            /* ICL PRMs, Volume 2a - Command Reference: Instructions,
   * 3DSTATE_SO_DECL_LIST:
   *
   *    "Workaround: This command must be followed by a PIPE_CONTROL
   *     with CS Stall bit set."
   *
   * On DG2+ also known as Wa_1509820217.
   */
   iris_emit_pipe_control_flush(batch,
      #endif
                  if (dirty & IRIS_DIRTY_STREAMOUT) {
      #if GFX_VERx10 >= 120
            /* Wa_16013994831 - Disable preemption. */
      #endif
               uint32_t dynamic_sol[GENX(3DSTATE_STREAMOUT_length)];
   iris_pack_command(GENX(3DSTATE_STREAMOUT), dynamic_sol, sol) {
                     sol.RenderingDisable = cso_rast->rasterizer_discard &&
         #if INTEL_NEEDS_WA_18022508906
               /* Wa_14017076903 :
   *
   * SKL PRMs, Volume 7: 3D-Media-GPGPU, Stream Output Logic (SOL) Stage:
   *
   * SOL_INT::Render_Enable =
   *   (3DSTATE_STREAMOUT::Force_Rending == Force_On) ||
   *   (
   *     (3DSTATE_STREAMOUT::Force_Rending != Force_Off) &&
   *     !(3DSTATE_GS::Enable && 3DSTATE_GS::Output Vertex Size == 0) &&
   *     !3DSTATE_STREAMOUT::API_Render_Disable &&
   *     (
   *       3DSTATE_DEPTH_STENCIL_STATE::Stencil_TestEnable ||
   *       3DSTATE_DEPTH_STENCIL_STATE::Depth_TestEnable ||
   *       3DSTATE_DEPTH_STENCIL_STATE::Depth_WriteEnable ||
   *       3DSTATE_PS_EXTRA::PS_Valid ||
   *       3DSTATE_WM::Legacy Depth_Buffer_Clear ||
   *       3DSTATE_WM::Legacy Depth_Buffer_Resolve_Enable ||
   *       3DSTATE_WM::Legacy Hierarchical_Depth_Buffer_Resolve_Enable
   *     )
   *   )
   *
   * If SOL_INT::Render_Enable is false, the SO stage will not forward any
   * topologies down the pipeline. Which is not what we want for occlusion
   * queries.
   *
   * Here we force rendering to get SOL_INT::Render_Enable when occlusion
   * queries are active.
   */
   const struct iris_rasterizer_state *cso_rast = ice->state.cso_rast;
   #endif
                              iris_emit_merge(batch, ice->state.streamout, dynamic_sol,
         } else {
         #if GFX_VERx10 >= 120
            /* Wa_16013994831 - Enable preemption. */
      #endif
                              if (dirty & IRIS_DIRTY_CLIP) {
      struct iris_rasterizer_state *cso_rast = ice->state.cso_rast;
            bool gs_or_tes = ice->shaders.prog[MESA_SHADER_GEOMETRY] ||
         bool points_or_lines = cso_rast->fill_mode_point_or_line ||
                  uint32_t dynamic_clip[GENX(3DSTATE_CLIP_length)];
   iris_pack_command(GENX(3DSTATE_CLIP), &dynamic_clip, cl) {
      cl.StatisticsEnable = ice->state.statistics_counters_enabled;
   if (cso_rast->rasterizer_discard)
         else if (ice->state.window_space_position)
                                                cl.ForceZeroRTAIndexEnable = cso_fb->layers <= 1;
      }
   iris_emit_merge(batch, cso_rast->clip, dynamic_clip,
               if (dirty & (IRIS_DIRTY_RASTER | IRIS_DIRTY_URB)) {
      /* From the Browadwell PRM, Volume 2, documentation for
   * 3DSTATE_RASTER, "Antialiasing Enable":
   *
   * "This field must be disabled if any of the render targets
   * have integer (UINT or SINT) surface format."
   *
   * Additionally internal documentation for Gfx12+ states:
   *
   * "This bit MUST not be set when NUM_MULTISAMPLES > 1 OR
   *  FORCED_SAMPLE_COUNT > 1."
   */
   struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   unsigned samples = util_framebuffer_get_num_samples(cso_fb);
            bool aa_enable = cso->line_smooth &&
                  uint32_t dynamic_raster[GENX(3DSTATE_RASTER_length)];
   iris_pack_command(GENX(3DSTATE_RASTER), &dynamic_raster, raster) {
         }
   iris_emit_merge(batch, cso->raster, dynamic_raster,
            uint32_t dynamic_sf[GENX(3DSTATE_SF_length)];
   iris_pack_command(GENX(3DSTATE_SF), &dynamic_sf, sf) {
      #if GFX_VER >= 12
         #endif
         }
   iris_emit_merge(batch, cso->sf, dynamic_sf,
               if (dirty & IRIS_DIRTY_WM) {
      struct iris_rasterizer_state *cso = ice->state.cso_rast;
            iris_pack_command(GENX(3DSTATE_WM), &dynamic_wm, wm) {
                              if (wm_prog_data->early_fragment_tests)
         else if (wm_prog_data->has_side_effects)
                        /* We could skip this bit if color writes are enabled. */
   if (wm_prog_data->has_side_effects || wm_prog_data->uses_kill)
      }
               if (dirty & IRIS_DIRTY_SBE) {
                  if (dirty & IRIS_DIRTY_PS_BLEND) {
      struct iris_blend_state *cso_blend = ice->state.cso_blend;
   struct iris_depth_stencil_alpha_state *cso_zsa = ice->state.cso_zsa;
   const struct shader_info *fs_info =
            int dst_blend_factor = cso_blend->ps_dst_blend_factor[0];
            /* When MSAA is enabled, instead of using BLENDFACTOR_ZERO use
   * CONST_COLOR, CONST_ALPHA and supply zero by using blend constants.
   */
   if (needs_wa_14018912822) {
      if (ice->state.color_blend_zero)
         if (ice->state.alpha_blend_zero)
               uint32_t dynamic_pb[GENX(3DSTATE_PS_BLEND_length)];
   iris_pack_command(GENX(3DSTATE_PS_BLEND), &dynamic_pb, pb) {
                                    /* The dual source blending docs caution against using SRC1 factors
   * when the shader doesn't use a dual source render target write.
   * Empirically, this can lead to GPU hangs, and the results are
   * undefined anyway, so simply disable blending to avoid the hang.
   */
   pb.ColorBufferBlendEnable = (cso_blend->blend_enables & 1) &&
               iris_emit_merge(batch, cso_blend->ps_blend, dynamic_pb,
               if (dirty & IRIS_DIRTY_WM_DEPTH_STENCIL) {
      #if GFX_VER >= 9 && GFX_VER < 12
         struct pipe_stencil_ref *p_stencil_refs = &ice->state.stencil_ref;
   uint32_t stencil_refs[GENX(3DSTATE_WM_DEPTH_STENCIL_length)];
   iris_pack_command(GENX(3DSTATE_WM_DEPTH_STENCIL), &stencil_refs, wmds) {
      wmds.StencilReferenceValue = p_stencil_refs->ref_value[0];
      }
   #else
         /* Use modify disable fields which allow us to emit packets
   * directly instead of merging them later.
   */
   #endif
         /* Depth or stencil write changed in cso. */
   if (intel_needs_workaround(batch->screen->devinfo, 18019816803) &&
      (dirty & IRIS_DIRTY_DS_WRITE_ENABLE)) {
   iris_emit_pipe_control_flush(
      batch, "workaround: PSS stall after DS write enable change",
         #if GFX_VER >= 12
         #endif
                  #if GFX_VER >= 12
         /* Use modify disable fields which allow us to emit packets
   * directly instead of merging them later.
   */
   struct pipe_stencil_ref *p_stencil_refs = &ice->state.stencil_ref;
   uint32_t stencil_refs[GENX(3DSTATE_WM_DEPTH_STENCIL_length)];
   iris_pack_command(GENX(3DSTATE_WM_DEPTH_STENCIL), &stencil_refs, wmds) {
      wmds.StencilReferenceValue = p_stencil_refs->ref_value[0];
   wmds.BackfaceStencilReferenceValue = p_stencil_refs->ref_value[1];
   wmds.StencilTestMaskModifyDisable = true;
   wmds.StencilWriteMaskModifyDisable = true;
   wmds.StencilStateModifyDisable = true;
      }
   #endif
               if (dirty & IRIS_DIRTY_SCISSOR_RECT) {
      /* Wa_1409725701:
   *    "The viewport-specific state used by the SF unit (SCISSOR_RECT) is
   *    stored as an array of up to 16 elements. The location of first
   *    element of the array, as specified by Pointer to SCISSOR_RECT,
   *    should be aligned to a 64-byte boundary.
   */
   uint32_t alignment = 64;
   uint32_t scissor_offset =
      emit_state(batch, ice->state.dynamic_uploader,
            &ice->state.last_res.scissor,
            iris_emit_cmd(batch, GENX(3DSTATE_SCISSOR_STATE_POINTERS), ptr) {
                     if (dirty & IRIS_DIRTY_DEPTH_BUFFER) {
               /* Do not emit the cso yet. We may need to update clear params first. */
   struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   struct iris_resource *zres = NULL, *sres = NULL;
   if (cso_fb->zsbuf) {
      iris_get_depth_stencil_resources(cso_fb->zsbuf->texture,
               if (zres && ice->state.hiz_usage != ISL_AUX_USAGE_NONE) {
      uint32_t *clear_params =
                  iris_pack_command(GENX(3DSTATE_CLEAR_PARAMS), clear_params, clear) {
      clear.DepthClearValueValid = true;
                           /* Wa_14016712196:
   * Emit depth flush after state that sends implicit depth flush.
   */
   if (intel_needs_workaround(batch->screen->devinfo, 14016712196)) {
      iris_emit_pipe_control_flush(batch, "Wa_14016712196",
               if (zres)
            if (intel_needs_workaround(batch->screen->devinfo, 1408224581) ||
      intel_needs_workaround(batch->screen->devinfo, 14014097488)) {
   /* Wa_1408224581
   *
   * Workaround: Gfx12LP Astep only An additional pipe control with
   * post-sync = store dword operation would be required.( w/a is to
   * have an additional pipe control after the stencil state whenever
   * the surface state bits of this state is changing).
   *
   * This also seems sufficient to handle Wa_14014097488.
   */
   iris_emit_pipe_control_write(batch, "WA for stencil state",
                              if (dirty & (IRIS_DIRTY_DEPTH_BUFFER | IRIS_DIRTY_WM_DEPTH_STENCIL)) {
      /* Listen for buffer changes, and also write enable changes. */
   struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
               if (dirty & IRIS_DIRTY_POLYGON_STIPPLE) {
      iris_emit_cmd(batch, GENX(3DSTATE_POLY_STIPPLE_PATTERN), poly) {
      for (int i = 0; i < 32; i++) {
                        if (dirty & IRIS_DIRTY_LINE_STIPPLE) {
      struct iris_rasterizer_state *cso = ice->state.cso_rast;
   #if GFX_VER >= 11
         /* ICL PRMs, Volume 2a - Command Reference: Instructions,
   * 3DSTATE_LINE_STIPPLE:
   *
   *    "Workaround: This command must be followed by a PIPE_CONTROL with
   *     CS Stall bit set."
   */
   iris_emit_pipe_control_flush(batch,
         #endif
               if (dirty & IRIS_DIRTY_VF_TOPOLOGY) {
      iris_emit_cmd(batch, GENX(3DSTATE_VF_TOPOLOGY), topo) {
      topo.PrimitiveTopologyType =
                  if (dirty & IRIS_DIRTY_VERTEX_BUFFERS) {
      int count = util_bitcount64(ice->state.bound_vertex_buffers);
            if (ice->state.vs_uses_draw_params) {
               struct iris_vertex_buffer_state *state =
                        iris_pack_state(GENX(VERTEX_BUFFER_STATE), state->state, vb) {
      vb.VertexBufferIndex = count;
   vb.AddressModifyEnable = true;
   vb.BufferPitch = 0;
   vb.BufferSize = res->bo->size - ice->draw.draw_params.offset;
   vb.BufferStartingAddress =
      ro_bo(NULL, res->bo->address +
      #if GFX_VER >= 12
         #endif
            }
   dynamic_bound |= 1ull << count;
               if (ice->state.vs_uses_derived_draw_params) {
      struct iris_vertex_buffer_state *state =
         pipe_resource_reference(&state->resource,
                  iris_pack_state(GENX(VERTEX_BUFFER_STATE), state->state, vb) {
      vb.VertexBufferIndex = count;
   vb.AddressModifyEnable = true;
   vb.BufferPitch = 0;
   vb.BufferSize =
         vb.BufferStartingAddress =
      ro_bo(NULL, res->bo->address +
      #if GFX_VER >= 12
         #endif
            }
   dynamic_bound |= 1ull << count;
               #if GFX_VER >= 11
            /* Gfx11+ doesn't need the cache workaround below */
   uint64_t bound = dynamic_bound;
   while (bound) {
      const int i = u_bit_scan64(&bound);
   iris_use_optional_res(batch, genx->vertex_buffers[i].resource,
   #else
            /* The VF cache designers cut corners, and made the cache key's
   * <VertexBufferIndex, Memory Address> tuple only consider the bottom
   * 32 bits of the address.  If you have two vertex buffers which get
   * placed exactly 4 GiB apart and use them in back-to-back draw calls,
   * you can get collisions (even within a single batch).
   *
   * So, we need to do a VF cache invalidate if the buffer for a VB
   * slot slot changes [48:32] address bits from the previous time.
                  uint64_t bound = dynamic_bound;
   while (bound) {
                     struct iris_resource *res =
                           high_bits = res->bo->address >> 32ull;
   if (high_bits != ice->state.last_vbo_high_bits[i]) {
      flush_flags |= PIPE_CONTROL_VF_CACHE_INVALIDATE |
                           if (flush_flags) {
      iris_emit_pipe_control_flush(batch,
         #endif
                        uint32_t *map =
         _iris_pack_command(batch, GENX(3DSTATE_VERTEX_BUFFERS), map, vb) {
                                       bound = dynamic_bound;
                     uint32_t vb_stride[GENX(VERTEX_BUFFER_STATE_length)];
   struct iris_bo *bo =
         iris_pack_state(GENX(VERTEX_BUFFER_STATE), &vb_stride, vbs) {
      vbs.BufferPitch = cso_ve->stride[i];
   /* Unnecessary except to defeat the genxml nonzero checker */
   vbs.MOCS = iris_mocs(bo, &screen->isl_dev,
      }
                                    if (dirty & IRIS_DIRTY_VERTEX_ELEMENTS) {
      struct iris_vertex_element_state *cso = ice->state.cso_vertex_elements;
   const unsigned entries = MAX2(cso->count, 1);
   if (!(ice->state.vs_needs_sgvs_element ||
         ice->state.vs_uses_derived_draw_params ||
      iris_batch_emit(batch, cso->vertex_elements, sizeof(uint32_t) *
      } else {
      uint32_t dynamic_ves[1 + 33 * GENX(VERTEX_ELEMENT_STATE_length)];
   const unsigned dyn_count = cso->count +
                  iris_pack_command(GENX(3DSTATE_VERTEX_ELEMENTS),
            ve.DWordLength =
      }
   memcpy(&dynamic_ves[1], &cso->vertex_elements[1],
         (cso->count - ice->state.vs_needs_edge_flag) *
   uint32_t *ve_pack_dest =
                  if (ice->state.vs_needs_sgvs_element) {
      uint32_t base_ctrl = ice->state.vs_uses_draw_params ?
         iris_pack_state(GENX(VERTEX_ELEMENT_STATE), ve_pack_dest, ve) {
      ve.Valid = true;
   ve.VertexBufferIndex =
         ve.SourceElementFormat = ISL_FORMAT_R32G32_UINT;
   ve.Component0Control = base_ctrl;
   ve.Component1Control = base_ctrl;
   ve.Component2Control = VFCOMP_STORE_0;
      }
      }
   if (ice->state.vs_uses_derived_draw_params) {
      iris_pack_state(GENX(VERTEX_ELEMENT_STATE), ve_pack_dest, ve) {
      ve.Valid = true;
   ve.VertexBufferIndex =
      util_bitcount64(ice->state.bound_vertex_buffers) +
      ve.SourceElementFormat = ISL_FORMAT_R32G32_UINT;
   ve.Component0Control = VFCOMP_STORE_SRC;
   ve.Component1Control = VFCOMP_STORE_SRC;
   ve.Component2Control = VFCOMP_STORE_0;
      }
      }
   if (ice->state.vs_needs_edge_flag) {
      for (int i = 0; i < GENX(VERTEX_ELEMENT_STATE_length);  i++)
               iris_batch_emit(batch, &dynamic_ves, sizeof(uint32_t) *
               if (!ice->state.vs_needs_edge_flag) {
      iris_batch_emit(batch, cso->vf_instancing, sizeof(uint32_t) *
      } else {
      assert(cso->count > 0);
   const unsigned edgeflag_index = cso->count - 1;
   uint32_t dynamic_vfi[33 * GENX(3DSTATE_VF_INSTANCING_length)];
                  uint32_t *vfi_pack_dest = &dynamic_vfi[0] +
         iris_pack_command(GENX(3DSTATE_VF_INSTANCING), vfi_pack_dest, vi) {
      vi.VertexElementIndex = edgeflag_index +
      ice->state.vs_needs_sgvs_element +
   }
                  iris_batch_emit(batch, &dynamic_vfi[0], sizeof(uint32_t) *
                  if (dirty & IRIS_DIRTY_VF_SGVS) {
      const struct brw_vs_prog_data *vs_prog_data = (void *)
                  iris_emit_cmd(batch, GENX(3DSTATE_VF_SGVS), sgv) {
      if (vs_prog_data->uses_vertexid) {
      sgv.VertexIDEnable = true;
   sgv.VertexIDComponentNumber = 2;
   sgv.VertexIDElementOffset =
               if (vs_prog_data->uses_instanceid) {
      sgv.InstanceIDEnable = true;
   sgv.InstanceIDComponentNumber = 3;
   sgv.InstanceIDElementOffset =
                     if (dirty & IRIS_DIRTY_VF) {
      #if GFX_VERx10 >= 125
         #endif
            if (draw->primitive_restart) {
      vf.IndexedDrawCutIndexEnable = true;
                  #if GFX_VERx10 >= 125
      if (dirty & IRIS_DIRTY_VFG) {
      iris_emit_cmd(batch, GENX(3DSTATE_VFG), vfg) {
      /* If 3DSTATE_TE: TE Enable == 1 then RR_STRICT else RR_FREE*/
   vfg.DistributionMode =
      ice->shaders.prog[MESA_SHADER_TESS_EVAL] != NULL ? RR_STRICT :
      vfg.DistributionGranularity = BatchLevelGranularity;
   /* Wa_14014890652 */
   if (intel_device_info_is_dg2(batch->screen->devinfo))
         vfg.ListCutIndexEnable = draw->primitive_restart;
   /* 192 vertices for TRILIST_ADJ */
   vfg.ListNBatchSizeScale = 0;
   /* Batch size of 384 vertices */
   vfg.List3BatchSizeScale = 2;
   /* Batch size of 128 vertices */
   vfg.List2BatchSizeScale = 1;
   /* Batch size of 128 vertices */
   vfg.List1BatchSizeScale = 2;
   /* Batch size of 256 vertices for STRIP topologies */
   vfg.StripBatchSizeScale = 3;
   /* 192 control points for PATCHLIST_3 */
   vfg.PatchBatchSizeScale = 1;
   /* 192 control points for PATCHLIST_3 */
            #endif
         if (dirty & IRIS_DIRTY_VF_STATISTICS) {
      iris_emit_cmd(batch, GENX(3DSTATE_VF_STATISTICS), vf) {
                  #if GFX_VER == 8
      if (dirty & IRIS_DIRTY_PMA_FIX) {
      bool enable = want_pma_fix(ice);
         #endif
         if (ice->state.current_hash_scale != 1)
         #if GFX_VER >= 12
         #endif
   }
      static void
   flush_vbos(struct iris_context *ice, struct iris_batch *batch)
   {
      struct iris_genx_state *genx = ice->state.genx;
   uint64_t bound = ice->state.bound_vertex_buffers;
   while (bound) {
      const int i = u_bit_scan64(&bound);
   struct iris_bo *bo = iris_resource_bo(genx->vertex_buffers[i].resource);
         }
      static bool
   point_or_line_list(enum mesa_prim prim_type)
   {
      switch (prim_type) {
   case MESA_PRIM_POINTS:
   case MESA_PRIM_LINES:
   case MESA_PRIM_LINE_STRIP:
   case MESA_PRIM_LINES_ADJACENCY:
   case MESA_PRIM_LINE_STRIP_ADJACENCY:
   case MESA_PRIM_LINE_LOOP:
         default:
         }
      }
      void
   genX(emit_breakpoint)(struct iris_batch *batch, bool emit_before_draw)
   {
      struct iris_context *ice = batch->ice;
   uint32_t draw_count = emit_before_draw ?
                  if (((draw_count == intel_debug_bkp_before_draw_count &&
            (draw_count == intel_debug_bkp_after_draw_count &&
         iris_emit_cmd(batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.WaitMode            = PollingMode;
   sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD;
   sem.SemaphoreDataDword  = 0x1;
   sem.SemaphoreAddress    = rw_bo(batch->screen->breakpoint_bo, 0,
            }
      static void
   iris_upload_render_state(struct iris_context *ice,
                           struct iris_batch *batch,
   {
      UNUSED const struct intel_device_info *devinfo = batch->screen->devinfo;
                     if (ice->state.dirty & IRIS_DIRTY_VERTEX_BUFFER_FLUSHES)
                     /* Always pin the binder.  If we're emitting new binding table pointers,
   * we need it.  If not, we're probably inheriting old tables via the
   * context, and need it anyway.  Since true zero-bindings cases are
   * practically non-existent, just pin it and avoid last_res tracking.
   */
   iris_use_pinned_bo(batch, ice->state.binder.bo, false,
            if (!batch->contains_draw) {
      if (GFX_VER == 12) {
      /* Re-emit constants when starting a new batch buffer in order to
   * work around push constant corruption on context switch.
   *
   * XXX - Provide hardware spec quotation when available.
   */
   ice->state.stage_dirty |= (IRIS_STAGE_DIRTY_CONSTANTS_VS  |
                        }
               if (!batch->contains_draw_with_next_seqno) {
      iris_restore_render_saved_bos(ice, batch, draw);
               /* Wa_1306463417 - Send HS state for every primitive on gfx11.
   * Wa_16011107343 (same for gfx12)
   * We implement this by setting TCS dirty on each draw.
   */
   if ((INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343) &&
      ice->shaders.prog[MESA_SHADER_TESS_CTRL]) {
                        if (draw->index_size > 0) {
               if (draw->has_user_indices) {
               u_upload_data(ice->ctx.const_uploader, start_offset,
               sc->count * draw->index_size, 4,
      } else {
                     pipe_resource_reference(&ice->state.last_res.index_buffer,
                              struct iris_genx_state *genx = ice->state.genx;
            uint32_t ib_packet[GENX(3DSTATE_INDEX_BUFFER_length)];
   iris_pack_command(GENX(3DSTATE_INDEX_BUFFER), ib_packet, ib) {
      ib.IndexFormat = draw->index_size >> 1;
   ib.MOCS = iris_mocs(bo, &batch->screen->isl_dev,
            #if GFX_VER >= 12
         #endif
                  if (memcmp(genx->last_index_buffer, ib_packet, sizeof(ib_packet)) != 0) {
      memcpy(genx->last_index_buffer, ib_packet, sizeof(ib_packet));
   iris_batch_emit(batch, ib_packet, sizeof(ib_packet));
         #if GFX_VER < 11
         /* The VF cache key only uses 32-bits, see vertex buffer comment above */
   uint16_t high_bits = bo->address >> 32ull;
   if (high_bits != ice->state.last_index_bo_high_bits) {
      iris_emit_pipe_control_flush(batch,
                        #endif
               if (indirect) {
      struct mi_builder b;
   uint32_t mocs;
      #define _3DPRIM_END_OFFSET          0x2420
   #define _3DPRIM_START_VERTEX        0x2430
   #define _3DPRIM_VERTEX_COUNT        0x2434
   #define _3DPRIM_INSTANCE_COUNT      0x2438
   #define _3DPRIM_START_INSTANCE      0x243C
   #define _3DPRIM_BASE_VERTEX         0x2440
            if (!indirect->count_from_stream_output) {
                        struct iris_bo *draw_count_bo =
         unsigned draw_count_offset =
                        if (ice->state.predicate == IRIS_PREDICATE_STATE_USE_BIT) {
      /* comparison = draw id < draw count */
                        /* predicate = comparison & conditional rendering predicate */
   mi_store(&b, mi_reg32(MI_PREDICATE_RESULT),
                        /* Upload the id of the current primitive to MI_PREDICATE_SRC1. */
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(drawid_offset));
   /* Upload the current draw count from the draw parameters buffer
   * to MI_PREDICATE_SRC0. Zero the top 32-bits of
   * MI_PREDICATE_SRC0.
                        if (drawid_offset == 0) {
      mi_predicate = MI_PREDICATE | MI_PREDICATE_LOADOP_LOADINV |
            } else {
      /* While draw_index < draw_count the predicate's result will be
   *  (draw_index == draw_count) ^ TRUE = TRUE
   * When draw_index == draw_count the result is
   *  (TRUE) ^ TRUE = FALSE
   * After this all results will be:
   *  (FALSE) ^ FALSE = FALSE
   */
   mi_predicate = MI_PREDICATE | MI_PREDICATE_LOADOP_LOAD |
            }
         }
                                 mi_store(&b, mi_reg32(_3DPRIM_VERTEX_COUNT),
         mi_store(&b, mi_reg32(_3DPRIM_INSTANCE_COUNT),
         mi_store(&b, mi_reg32(_3DPRIM_START_VERTEX),
         if (draw->index_size) {
      mi_store(&b, mi_reg32(_3DPRIM_BASE_VERTEX),
         mi_store(&b, mi_reg32(_3DPRIM_START_INSTANCE),
      } else {
      mi_store(&b, mi_reg32(_3DPRIM_START_INSTANCE),
               } else if (indirect->count_from_stream_output) {
      struct iris_stream_output_target *so =
                                          struct iris_address addr = ro_bo(so_bo, so->offset.offset);
   struct mi_value offset =
         mi_store(&b, mi_reg32(_3DPRIM_VERTEX_COUNT),
         mi_store(&b, mi_reg32(_3DPRIM_START_VERTEX), mi_imm(0));
   mi_store(&b, mi_reg32(_3DPRIM_BASE_VERTEX), mi_imm(0));
   mi_store(&b, mi_reg32(_3DPRIM_START_INSTANCE), mi_imm(0));
   mi_store(&b, mi_reg32(_3DPRIM_INSTANCE_COUNT),
                                    iris_emit_cmd(batch, GENX(3DPRIMITIVE), prim) {
      prim.VertexAccessType = draw->index_size > 0 ? RANDOM : SEQUENTIAL;
            if (indirect) {
         } else {
      prim.StartInstanceLocation = draw->start_instance;
                           if (draw->index_size) {
                              #if GFX_VERx10 == 125
      if (intel_needs_workaround(devinfo, 22014412737) &&
      (point_or_line_list(ice->state.prim_mode) || indirect ||
   (sc->count == 1 || sc->count == 2))) {
      iris_emit_pipe_control_write(batch, "Wa_22014412737",
                        #endif
                  uint32_t count = (sc) ? sc->count : 0;
   count *= draw->instance_count ? draw->instance_count : 1;
      }
      static void
   iris_load_indirect_location(struct iris_context *ice,
               {
   #define GPGPU_DISPATCHDIMX 0x2500
   #define GPGPU_DISPATCHDIMY 0x2504
   #define GPGPU_DISPATCHDIMZ 0x2508
                  struct iris_state_ref *grid_size = &ice->state.grid_size;
   struct iris_bo *bo = iris_resource_bo(grid_size->res);
   struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   struct mi_value size_x = mi_mem32(ro_bo(bo, grid_size->offset + 0));
   struct mi_value size_y = mi_mem32(ro_bo(bo, grid_size->offset + 4));
   struct mi_value size_z = mi_mem32(ro_bo(bo, grid_size->offset + 8));
   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMX), size_x);
   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMY), size_y);
      }
      #if GFX_VERx10 >= 125
      static void
   iris_upload_compute_walker(struct iris_context *ice,
               {
      const uint64_t stage_dirty = ice->state.stage_dirty;
   struct iris_screen *screen = batch->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
   struct iris_binder *binder = &ice->state.binder;
   struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_COMPUTE];
   struct iris_compiled_shader *shader =
         struct brw_stage_prog_data *prog_data = shader->prog_data;
   struct brw_cs_prog_data *cs_prog_data = (void *) prog_data;
   const struct brw_cs_dispatch_info dispatch =
                     if (stage_dirty & IRIS_STAGE_DIRTY_CS) {
      iris_emit_cmd(batch, GENX(CFE_STATE), cfe) {
      cfe.MaximumNumberofThreads =
         uint32_t scratch_addr = pin_scratch_space(ice, batch, prog_data,
                        if (grid->indirect)
                     ice->utrace.last_compute_walker =
         _iris_pack_command(batch, GENX(COMPUTE_WALKER),
            cw.IndirectParameterEnable        = grid->indirect;
   cw.SIMDSize                       = dispatch.simd_size / 16;
   cw.LocalXMaximum                  = grid->block[0] - 1;
   cw.LocalYMaximum                  = grid->block[1] - 1;
   cw.LocalZMaximum                  = grid->block[2] - 1;
   cw.ThreadGroupIDXDimension        = grid->grid[0];
   cw.ThreadGroupIDYDimension        = grid->grid[1];
   cw.ThreadGroupIDZDimension        = grid->grid[2];
   cw.ExecutionMask                  = dispatch.right_mask;
            cw.InterfaceDescriptor = (struct GENX(INTERFACE_DESCRIPTOR_DATA)) {
      .KernelStartPointer = KSP(shader),
   .NumberofThreadsinGPGPUThreadGroup = dispatch.threads,
   .SharedLocalMemorySize =
         .PreferredSLMAllocationSize = preferred_slm_allocation_size(devinfo),
   .NumberOfBarriers = cs_prog_data->uses_barrier,
   .SamplerStatePointer = shs->sampler_table.offset,
   .SamplerCount = encode_sampler_count(shader),
   .BindingTablePointer = binder->bt_offset[MESA_SHADER_COMPUTE],
   /* Typically set to 0 to avoid prefetching on every thread dispatch. */
   .BindingTableEntryCount = devinfo->verx10 == 125 ?
                              }
      #else /* #if GFX_VERx10 >= 125 */
      static void
   iris_upload_gpgpu_walker(struct iris_context *ice,
               {
      const uint64_t stage_dirty = ice->state.stage_dirty;
   struct iris_screen *screen = batch->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
   struct iris_binder *binder = &ice->state.binder;
   struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_COMPUTE];
   struct iris_uncompiled_shader *ish =
         struct iris_compiled_shader *shader =
         struct brw_stage_prog_data *prog_data = shader->prog_data;
   struct brw_cs_prog_data *cs_prog_data = (void *) prog_data;
   const struct brw_cs_dispatch_info dispatch =
                     if ((stage_dirty & IRIS_STAGE_DIRTY_CS) ||
      cs_prog_data->local_size[0] == 0 /* Variable local group size */) {
   /* The MEDIA_VFE_STATE documentation for Gfx8+ says:
   *
   *   "A stalling PIPE_CONTROL is required before MEDIA_VFE_STATE unless
   *    the only bits that are changed are scoreboard related: Scoreboard
   *    Enable, Scoreboard Type, Scoreboard Mask, Scoreboard Delta.  For
   *    these scoreboard related states, a MEDIA_STATE_FLUSH is
   *    sufficient."
   */
   iris_emit_pipe_control_flush(batch,
                  iris_emit_cmd(batch, GENX(MEDIA_VFE_STATE), vfe) {
      if (prog_data->total_scratch) {
                     vfe.PerThreadScratchSpace = ffs(prog_data->total_scratch) - 11;
   vfe.ScratchSpaceBasePointer =
                  #if GFX_VER < 11
               #endif
   #if GFX_VER == 8
         #endif
                           vfe.CURBEAllocationSize =
      ALIGN(cs_prog_data->push.per_thread.regs * dispatch.threads +
               /* TODO: Combine subgroup-id with cbuf0 so we can push regular uniforms */
   if ((stage_dirty & IRIS_STAGE_DIRTY_CS) ||
      cs_prog_data->local_size[0] == 0 /* Variable local group size */) {
   uint32_t curbe_data_offset = 0;
   assert(cs_prog_data->push.cross_thread.dwords == 0 &&
         cs_prog_data->push.per_thread.dwords == 1 &&
   const unsigned push_const_size =
         uint32_t *curbe_data_map =
      stream_state(batch, ice->state.dynamic_uploader,
                  assert(curbe_data_map);
   memset(curbe_data_map, 0x5a, ALIGN(push_const_size, 64));
   iris_fill_cs_push_const_buffer(cs_prog_data, dispatch.threads,
            iris_emit_cmd(batch, GENX(MEDIA_CURBE_LOAD), curbe) {
      curbe.CURBETotalDataLength = ALIGN(push_const_size, 64);
                  for (unsigned i = 0; i < IRIS_MAX_GLOBAL_BINDINGS; i++) {
      struct pipe_resource *res = ice->state.global_bindings[i];
   if (!res)
            iris_use_pinned_bo(batch, iris_resource_bo(res),
               if (stage_dirty & (IRIS_STAGE_DIRTY_SAMPLER_STATES_CS |
                                 iris_pack_state(GENX(INTERFACE_DESCRIPTOR_DATA), desc, idd) {
      idd.SharedLocalMemorySize =
         idd.KernelStartPointer =
      KSP(shader) + brw_cs_prog_data_prog_offset(cs_prog_data,
      idd.SamplerStatePointer = shs->sampler_table.offset;
   idd.BindingTablePointer =
                     for (int i = 0; i < GENX(INTERFACE_DESCRIPTOR_DATA_length); i++)
            iris_emit_cmd(batch, GENX(MEDIA_INTERFACE_DESCRIPTOR_LOAD), load) {
      load.InterfaceDescriptorTotalLength =
         load.InterfaceDescriptorDataStartAddress =
      emit_state(batch, ice->state.dynamic_uploader,
               if (grid->indirect)
                     iris_emit_cmd(batch, GENX(GPGPU_WALKER), ggw) {
      ggw.IndirectParameterEnable    = grid->indirect != NULL;
   ggw.SIMDSize                   = dispatch.simd_size / 16;
   ggw.ThreadDepthCounterMaximum  = 0;
   ggw.ThreadHeightCounterMaximum = 0;
   ggw.ThreadWidthCounterMaximum  = dispatch.threads - 1;
   ggw.ThreadGroupIDXDimension    = grid->grid[0];
   ggw.ThreadGroupIDYDimension    = grid->grid[1];
   ggw.ThreadGroupIDZDimension    = grid->grid[2];
   ggw.RightExecutionMask         = dispatch.right_mask;
                           }
      #endif /* #if GFX_VERx10 >= 125 */
      static void
   iris_upload_compute_state(struct iris_context *ice,
               {
      struct iris_screen *screen = batch->screen;
   const uint64_t stage_dirty = ice->state.stage_dirty;
   struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_COMPUTE];
   struct iris_compiled_shader *shader =
         struct iris_border_color_pool *border_color_pool =
                     /* Always pin the binder.  If we're emitting new binding table pointers,
   * we need it.  If not, we're probably inheriting old tables via the
   * context, and need it anyway.  Since true zero-bindings cases are
   * practically non-existent, just pin it and avoid last_res tracking.
   */
            if (((stage_dirty & IRIS_STAGE_DIRTY_CONSTANTS_CS) &&
      shs->sysvals_need_upload) ||
   shader->kernel_input_size > 0)
         if (stage_dirty & IRIS_STAGE_DIRTY_BINDINGS_CS)
            if (stage_dirty & IRIS_STAGE_DIRTY_SAMPLER_STATES_CS)
            iris_use_optional_res(batch, shs->sampler_table.res, false,
         iris_use_pinned_bo(batch, iris_resource_bo(shader->assembly.res), false,
            if (ice->state.need_border_colors)
      iris_use_pinned_bo(batch, border_color_pool->bo, false,
      #if GFX_VER >= 12
         #endif
      #if GFX_VERx10 >= 125
         #else
         #endif
         if (!batch->contains_draw_with_next_seqno) {
      iris_restore_compute_saved_bos(ice, batch, grid);
                  }
      /**
   * State module teardown.
   */
   static void
   iris_destroy_state(struct iris_context *ice)
   {
                        pipe_resource_reference(&ice->draw.draw_params.res, NULL);
            /* Loop over all VBOs, including ones for draw parameters */
   for (unsigned i = 0; i < ARRAY_SIZE(genx->vertex_buffers); i++) {
                           for (int i = 0; i < 4; i++) {
                           for (int stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      struct iris_shader_state *shs = &ice->state.shaders[stage];
   pipe_resource_reference(&shs->sampler_table.res, NULL);
   for (int i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      pipe_resource_reference(&shs->constbuf[i].buffer, NULL);
      }
   for (int i = 0; i < PIPE_MAX_SHADER_IMAGES; i++) {
      pipe_resource_reference(&shs->image[i].base.resource, NULL);
   pipe_resource_reference(&shs->image[i].surface_state.ref.res, NULL);
      }
   for (int i = 0; i < PIPE_MAX_SHADER_BUFFERS; i++) {
      pipe_resource_reference(&shs->ssbo[i].buffer, NULL);
      }
   for (int i = 0; i < IRIS_MAX_TEXTURES; i++) {
      pipe_sampler_view_reference((struct pipe_sampler_view **)
                  pipe_resource_reference(&ice->state.grid_size.res, NULL);
            pipe_resource_reference(&ice->state.null_fb.res, NULL);
            pipe_resource_reference(&ice->state.last_res.cc_vp, NULL);
   pipe_resource_reference(&ice->state.last_res.sf_cl_vp, NULL);
   pipe_resource_reference(&ice->state.last_res.color_calc, NULL);
   pipe_resource_reference(&ice->state.last_res.scissor, NULL);
   pipe_resource_reference(&ice->state.last_res.blend, NULL);
   pipe_resource_reference(&ice->state.last_res.index_buffer, NULL);
   pipe_resource_reference(&ice->state.last_res.cs_thread_ids, NULL);
      }
      /* ------------------------------------------------------------------- */
      static void
   iris_rebind_buffer(struct iris_context *ice,
         {
      struct pipe_context *ctx = &ice->ctx;
                     /* Buffers can't be framebuffer attachments, nor display related,
   * and we don't have upstream Clover support.
   */
   assert(!(res->bind_history & (PIPE_BIND_DEPTH_STENCIL |
                                          if (res->bind_history & PIPE_BIND_VERTEX_BUFFER) {
      uint64_t bound_vbs = ice->state.bound_vertex_buffers;
   while (bound_vbs) {
                     /* Update the CPU struct */
   STATIC_ASSERT(GENX(VERTEX_BUFFER_STATE_BufferStartingAddress_start) == 32);
   STATIC_ASSERT(GENX(VERTEX_BUFFER_STATE_BufferStartingAddress_bits) == 64);
                  if (*addr != bo->address + state->offset) {
      *addr = bo->address + state->offset;
   ice->state.dirty |= IRIS_DIRTY_VERTEX_BUFFERS |
                     /* We don't need to handle PIPE_BIND_INDEX_BUFFER here: we re-emit
   * the 3DSTATE_INDEX_BUFFER packet whenever the address changes.
   *
   * There is also no need to handle these:
   * - PIPE_BIND_COMMAND_ARGS_BUFFER (emitted for every indirect draw)
   * - PIPE_BIND_QUERY_BUFFER (no persistent state references)
            if (res->bind_history & PIPE_BIND_STREAM_OUTPUT) {
      uint32_t *so_buffers = genx->so_buffers;
   for (unsigned i = 0; i < 4; i++,
               /* There are no other fields in bits 127:64 */
   uint64_t *addr = (uint64_t *) &so_buffers[2];
                  struct pipe_stream_output_target *tgt = ice->state.so_target[i];
   if (tgt) {
      struct iris_bo *bo = iris_resource_bo(tgt->buffer);
   if (*addr != bo->address + tgt->buffer_offset) {
      *addr = bo->address + tgt->buffer_offset;
                        for (int s = MESA_SHADER_VERTEX; s < MESA_SHADER_STAGES; s++) {
      struct iris_shader_state *shs = &ice->state.shaders[s];
            if (!(res->bind_stages & (1 << s)))
            if (res->bind_history & PIPE_BIND_CONSTANT_BUFFER) {
      /* Skip constant buffer 0, it's for regular uniforms, not UBOs */
   uint32_t bound_cbufs = shs->bound_cbufs & ~1u;
   while (bound_cbufs) {
      const int i = u_bit_scan(&bound_cbufs);
                  if (res->bo == iris_resource_bo(cbuf->buffer)) {
      pipe_resource_reference(&surf_state->res, NULL);
   shs->dirty_cbufs |= 1u << i;
   ice->state.dirty |= (IRIS_DIRTY_RENDER_MISC_BUFFER_FLUSHES |
                           if (res->bind_history & PIPE_BIND_SHADER_BUFFER) {
      uint32_t bound_ssbos = shs->bound_ssbos;
   while (bound_ssbos) {
                     if (res->bo == iris_resource_bo(ssbo->buffer)) {
      struct pipe_shader_buffer buf = {
      .buffer = &res->base.b,
   .buffer_offset = ssbo->buffer_offset,
      };
   iris_set_shader_buffers(ctx, p_stage, i, 1, &buf,
                     if (res->bind_history & PIPE_BIND_SAMPLER_VIEW) {
      int i;
   BITSET_FOREACH_SET(i, shs->bound_sampler_views, IRIS_MAX_TEXTURES) {
                     if (update_surface_state_addrs(ice->state.surface_uploader,
                              if (res->bind_history & PIPE_BIND_SHADER_IMAGE) {
      uint64_t bound_image_views = shs->bound_image_views;
   while (bound_image_views) {
      const int i = u_bit_scan64(&bound_image_views);
                  if (update_surface_state_addrs(ice->state.surface_uploader,
                           }
      /* ------------------------------------------------------------------- */
      /**
   * Introduce a batch synchronization boundary, and update its cache coherency
   * status to reflect the execution of a PIPE_CONTROL command with the
   * specified flags.
   */
   static void
   batch_mark_sync_for_pipe_control(struct iris_batch *batch, uint32_t flags)
   {
                        if ((flags & PIPE_CONTROL_CS_STALL)) {
      if ((flags & PIPE_CONTROL_RENDER_TARGET_FLUSH))
            if ((flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH))
            if ((flags & PIPE_CONTROL_TILE_CACHE_FLUSH)) {
      /* A tile cache flush makes any C/Z data in L3 visible to memory. */
   const unsigned c = IRIS_DOMAIN_RENDER_WRITE;
   const unsigned z = IRIS_DOMAIN_DEPTH_WRITE;
   batch->coherent_seqnos[c][c] = batch->l3_coherent_seqnos[c];
               if (flags & (PIPE_CONTROL_FLUSH_HDC | PIPE_CONTROL_DATA_CACHE_FLUSH)) {
      /* HDC and DC flushes both flush the data cache out to L3 */
               if ((flags & PIPE_CONTROL_DATA_CACHE_FLUSH)) {
      /* A DC flush also flushes L3 data cache lines out to memory. */
   const unsigned i = IRIS_DOMAIN_DATA_WRITE;
               if ((flags & PIPE_CONTROL_FLUSH_ENABLE))
            if ((flags & (PIPE_CONTROL_CACHE_FLUSH_BITS |
            iris_batch_mark_flush_sync(batch, IRIS_DOMAIN_VF_READ);
   iris_batch_mark_flush_sync(batch, IRIS_DOMAIN_SAMPLER_READ);
   iris_batch_mark_flush_sync(batch, IRIS_DOMAIN_PULL_CONSTANT_READ);
                  if ((flags & PIPE_CONTROL_RENDER_TARGET_FLUSH))
            if ((flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH))
            if (flags & (PIPE_CONTROL_FLUSH_HDC | PIPE_CONTROL_DATA_CACHE_FLUSH))
            if ((flags & PIPE_CONTROL_FLUSH_ENABLE))
            if ((flags & PIPE_CONTROL_VF_CACHE_INVALIDATE))
            if ((flags & PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE))
            /* Technically, to invalidate IRIS_DOMAIN_PULL_CONSTANT_READ, we need
   * both "Constant Cache Invalidate" and either "Texture Cache Invalidate"
   * or "Data Cache Flush" set, depending on the setting of
   * compiler->indirect_ubos_use_sampler.
   *
   * However, "Data Cache Flush" and "Constant Cache Invalidate" will never
   * appear in the same PIPE_CONTROL command, because one is bottom-of-pipe
   * while the other is top-of-pipe.  Because we only look at one flush at
   * a time, we won't see both together.
   *
   * To deal with this, we mark it as invalidated when the constant cache
   * is invalidated, and trust the callers to also flush the other related
   * cache correctly at the same time.
   */
   if ((flags & PIPE_CONTROL_CONST_CACHE_INVALIDATE))
                     if ((flags & PIPE_CONTROL_L3_RO_INVALIDATE_BITS) == PIPE_CONTROL_L3_RO_INVALIDATE_BITS) {
      /* If we just invalidated the read-only lines of L3, then writes from non-L3-coherent
   * domains will now be visible to those L3 clients.
   */
   for (unsigned i = 0; i < NUM_IRIS_DOMAINS; i++) {
      if (!iris_domain_is_l3_coherent(devinfo, i))
            }
      static unsigned
   flags_to_post_sync_op(uint32_t flags)
   {
      if (flags & PIPE_CONTROL_WRITE_IMMEDIATE)
            if (flags & PIPE_CONTROL_WRITE_DEPTH_COUNT)
            if (flags & PIPE_CONTROL_WRITE_TIMESTAMP)
               }
      /**
   * Do the given flags have a Post Sync or LRI Post Sync operation?
   */
   static enum pipe_control_flags
   get_post_sync_flags(enum pipe_control_flags flags)
   {
      flags &= PIPE_CONTROL_WRITE_IMMEDIATE |
            PIPE_CONTROL_WRITE_DEPTH_COUNT |
         /* Only one "Post Sync Op" is allowed, and it's mutually exclusive with
   * "LRI Post Sync Operation".  So more than one bit set would be illegal.
   */
               }
      #define IS_COMPUTE_PIPELINE(batch) (batch->name == IRIS_BATCH_COMPUTE)
      /**
   * Emit a series of PIPE_CONTROL commands, taking into account any
   * workarounds necessary to actually accomplish the caller's request.
   *
   * Unless otherwise noted, spec quotations in this function come from:
   *
   * Synchronization of the 3D Pipeline > PIPE_CONTROL Command > Programming
   * Restrictions for PIPE_CONTROL.
   *
   * You should not use this function directly.  Use the helpers in
   * iris_pipe_control.c instead, which may split the pipe control further.
   */
   static void
   iris_emit_raw_pipe_control(struct iris_batch *batch,
                                 {
      UNUSED const struct intel_device_info *devinfo = batch->screen->devinfo;
   enum pipe_control_flags post_sync_flags = get_post_sync_flags(flags);
   enum pipe_control_flags non_lri_post_sync_flags =
         #if GFX_VER >= 12
      if (batch->name == IRIS_BATCH_BLITTER) {
      batch_mark_sync_for_pipe_control(batch, flags);
                     /* The blitter doesn't actually use PIPE_CONTROL; rather it uses the
   * MI_FLUSH_DW command.  However, all of our code is set up to flush
   * via emitting a pipe control, so we just translate it at this point,
   * even if it is a bit hacky.
   */
   iris_emit_cmd(batch, GENX(MI_FLUSH_DW), fd) {
      fd.Address = rw_bo(bo, offset, IRIS_DOMAIN_OTHER_WRITE);
      #if GFX_VERx10 >= 125
               #endif
         }
   iris_batch_sync_region_end(batch);
         #endif
         /* The "L3 Read Only Cache Invalidation Bit" docs say it "controls the
   * invalidation of the Geometry streams cached in L3 cache at the top
   * of the pipe".  In other words, index & vertex data that gets cached
   * in L3 when VERTEX_BUFFER_STATE::L3BypassDisable is set.
   *
   * Normally, invalidating L1/L2 read-only caches also invalidate their
   * related L3 cachelines, but this isn't the case for the VF cache.
   * Emulate it by setting the L3 Read Only bit when doing a VF invalidate.
   */
   if (flags & PIPE_CONTROL_VF_CACHE_INVALIDATE)
            /* Recursive PIPE_CONTROL workarounds --------------------------------
   * (http://knowyourmeme.com/memes/xzibit-yo-dawg)
   *
   * We do these first because we want to look at the original operation,
   * rather than any workarounds we set.
   */
   if (GFX_VER == 9 && (flags & PIPE_CONTROL_VF_CACHE_INVALIDATE)) {
      /* The PIPE_CONTROL "VF Cache Invalidation Enable" bit description
   * lists several workarounds:
   *
   *    "Project: SKL, KBL, BXT
   *
   *     If the VF Cache Invalidation Enable is set to a 1 in a
   *     PIPE_CONTROL, a separate Null PIPE_CONTROL, all bitfields
   *     sets to 0, with the VF Cache Invalidation Enable set to 0
   *     needs to be sent prior to the PIPE_CONTROL with VF Cache
   *     Invalidation Enable set to a 1."
   */
   iris_emit_raw_pipe_control(batch,
                     if (GFX_VER == 9 && IS_COMPUTE_PIPELINE(batch) && post_sync_flags) {
      /* Project: SKL / Argument: LRI Post Sync Operation [23]
   *
   * "PIPECONTROL command with “Command Streamer Stall Enable” must be
   *  programmed prior to programming a PIPECONTROL command with "LRI
   *  Post Sync Operation" in GPGPU mode of operation (i.e when
   *  PIPELINE_SELECT command is set to GPGPU mode of operation)."
   *
   * The same text exists a few rows below for Post Sync Op.
   */
   iris_emit_raw_pipe_control(batch,
                     /* "Flush Types" workarounds ---------------------------------------------
   * We do these now because they may add post-sync operations or CS stalls.
            if (GFX_VER < 11 && flags & PIPE_CONTROL_VF_CACHE_INVALIDATE) {
      /* Project: BDW, SKL+ (stopping at CNL) / Argument: VF Invalidate
   *
   * "'Post Sync Operation' must be enabled to 'Write Immediate Data' or
   *  'Write PS Depth Count' or 'Write Timestamp'."
   */
   if (!bo) {
      flags |= PIPE_CONTROL_WRITE_IMMEDIATE;
   post_sync_flags |= PIPE_CONTROL_WRITE_IMMEDIATE;
   non_lri_post_sync_flags |= PIPE_CONTROL_WRITE_IMMEDIATE;
   bo = batch->screen->workaround_address.bo;
                  if (flags & PIPE_CONTROL_DEPTH_STALL) {
      /* From the PIPE_CONTROL instruction table, bit 13 (Depth Stall Enable):
   *
   *    "This bit must be DISABLED for operations other than writing
   *     PS_DEPTH_COUNT."
   *
   * This seems like nonsense.  An Ivybridge workaround requires us to
   * emit a PIPE_CONTROL with a depth stall and write immediate post-sync
   * operation.  Gfx8+ requires us to emit depth stalls and depth cache
   * flushes together.  So, it's hard to imagine this means anything other
   * than "we originally intended this to be used for PS_DEPTH_COUNT".
   *
   * We ignore the supposed restriction and do nothing.
               if (flags & (PIPE_CONTROL_RENDER_TARGET_FLUSH |
            /* From the PIPE_CONTROL instruction table, bit 12 and bit 1:
   *
   *    "This bit must be DISABLED for End-of-pipe (Read) fences,
   *     PS_DEPTH_COUNT or TIMESTAMP queries."
   *
   * TODO: Implement end-of-pipe checking.
   */
   assert(!(post_sync_flags & (PIPE_CONTROL_WRITE_DEPTH_COUNT |
               if (GFX_VER < 11 && (flags & PIPE_CONTROL_STALL_AT_SCOREBOARD)) {
      /* From the PIPE_CONTROL instruction table, bit 1:
   *
   *    "This bit is ignored if Depth Stall Enable is set.
   *     Further, the render cache is not flushed even if Write Cache
   *     Flush Enable bit is set."
   *
   * We assert that the caller doesn't do this combination, to try and
   * prevent mistakes.  It shouldn't hurt the GPU, though.
   *
   * We skip this check on Gfx11+ as the "Stall at Pixel Scoreboard"
   * and "Render Target Flush" combo is explicitly required for BTI
   * update workarounds.
   */
   assert(!(flags & (PIPE_CONTROL_DEPTH_STALL |
                        if (GFX_VER <= 8 && (flags & PIPE_CONTROL_STATE_CACHE_INVALIDATE)) {
      /* From the PIPE_CONTROL page itself:
   *
   *    "IVB, HSW, BDW
   *     Restriction: Pipe_control with CS-stall bit set must be issued
   *     before a pipe-control command that has the State Cache
   *     Invalidate bit set."
   */
               if (flags & PIPE_CONTROL_FLUSH_LLC) {
      /* From the PIPE_CONTROL instruction table, bit 26 (Flush LLC):
   *
   *    "Project: ALL
   *     SW must always program Post-Sync Operation to "Write Immediate
   *     Data" when Flush LLC is set."
   *
   * For now, we just require the caller to do it.
   */
               /* Emulate a HDC flush with a full Data Cache Flush on older hardware which
   * doesn't support the new lightweight flush.
      #if GFX_VER < 12
         if (flags & PIPE_CONTROL_FLUSH_HDC)
   #endif
                  /* Project: All / Argument: Global Snapshot Count Reset [19]
   *
   * "This bit must not be exercised on any product.
   *  Requires stall bit ([20] of DW1) set."
   *
   * We don't use this, so we just assert that it isn't used.  The
   * PIPE_CONTROL instruction page indicates that they intended this
   * as a debug feature and don't think it is useful in production,
   * but it may actually be usable, should we ever want to.
   */
            if (flags & (PIPE_CONTROL_MEDIA_STATE_CLEAR |
            /* Project: All / Arguments:
   *
   * - Generic Media State Clear [16]
   * - Indirect State Pointers Disable [16]
   *
   *    "Requires stall bit ([20] of DW1) set."
   *
   * Also, the PIPE_CONTROL instruction table, bit 16 (Generic Media
   * State Clear) says:
   *
   *    "PIPECONTROL command with “Command Streamer Stall Enable” must be
   *     programmed prior to programming a PIPECONTROL command with "Media
   *     State Clear" set in GPGPU mode of operation"
   *
   * This is a subset of the earlier rule, so there's nothing to do.
   */
               if (flags & PIPE_CONTROL_STORE_DATA_INDEX) {
      /* Project: All / Argument: Store Data Index
   *
   * "Post-Sync Operation ([15:14] of DW1) must be set to something other
   *  than '0'."
   *
   * For now, we just assert that the caller does this.  We might want to
   * automatically add a write to the workaround BO...
   */
               if (flags & PIPE_CONTROL_SYNC_GFDT) {
      /* Project: All / Argument: Sync GFDT
   *
   * "Post-Sync Operation ([15:14] of DW1) must be set to something other
   *  than '0' or 0x2520[13] must be set."
   *
   * For now, we just assert that the caller does this.
   */
               if (flags & PIPE_CONTROL_TLB_INVALIDATE) {
      /* Project: IVB+ / Argument: TLB inv
   *
   *    "Requires stall bit ([20] of DW1) set."
   *
   * Also, from the PIPE_CONTROL instruction table:
   *
   *    "Project: SKL+
   *     Post Sync Operation or CS stall must be set to ensure a TLB
   *     invalidation occurs.  Otherwise no cycle will occur to the TLB
   *     cache to invalidate."
   *
   * This is not a subset of the earlier rule, so there's nothing to do.
   */
               if (GFX_VER == 9 && devinfo->gt == 4) {
                           if (IS_COMPUTE_PIPELINE(batch)) {
      if ((GFX_VER == 9 || GFX_VER == 11) &&
      (flags & PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE)) {
   /* Project: SKL, ICL / Argument: Tex Invalidate
   * "Requires stall bit ([20] of DW) set for all GPGPU Workloads."
   */
               if (GFX_VER == 8 && (post_sync_flags ||
                        (flags & (PIPE_CONTROL_NOTIFY_ENABLE |
         /* Project: BDW / Arguments:
   *
   * - LRI Post Sync Operation   [23]
   * - Post Sync Op              [15:14]
   * - Notify En                 [8]
   * - Depth Stall               [13]
   * - Render Target Cache Flush [12]
   * - Depth Cache Flush         [0]
   * - DC Flush Enable           [5]
   *
   *    "Requires stall bit ([20] of DW) set for all GPGPU and Media
   *     Workloads."
                  /* Also, from the PIPE_CONTROL instruction table, bit 20:
   *
   *    "Project: BDW
   *     This bit must be always set when PIPE_CONTROL command is
   *     programmed by GPGPU and MEDIA workloads, except for the cases
   *     when only Read Only Cache Invalidation bits are set (State
   *     Cache Invalidation Enable, Instruction cache Invalidation
   *     Enable, Texture Cache Invalidation Enable, Constant Cache
   *     Invalidation Enable). This is to WA FFDOP CG issue, this WA
   *     need not implemented when FF_DOP_CG is disable via "Fixed
   *     Function DOP Clock Gate Disable" bit in RC_PSMI_CTRL register."
   *
   * It sounds like we could avoid CS stalls in some cases, but we
   * don't currently bother.  This list isn't exactly the list above,
   * either...
                  /* "Stall" workarounds ----------------------------------------------
   * These have to come after the earlier ones because we may have added
   * some additional CS stalls above.
            if (GFX_VER < 9 && (flags & PIPE_CONTROL_CS_STALL)) {
      /* Project: PRE-SKL, VLV, CHV
   *
   * "[All Stepping][All SKUs]:
   *
   *  One of the following must also be set:
   *
   *  - Render Target Cache Flush Enable ([12] of DW1)
   *  - Depth Cache Flush Enable ([0] of DW1)
   *  - Stall at Pixel Scoreboard ([1] of DW1)
   *  - Depth Stall ([13] of DW1)
   *  - Post-Sync Operation ([13] of DW1)
   *  - DC Flush Enable ([5] of DW1)"
   *
   * If we don't already have one of those bits set, we choose to add
   * "Stall at Pixel Scoreboard".  Some of the other bits require a
   * CS stall as a workaround (see above), which would send us into
   * an infinite recursion of PIPE_CONTROLs.  "Stall at Pixel Scoreboard"
   * appears to be safe, so we choose that.
   */
   const uint32_t wa_bits = PIPE_CONTROL_RENDER_TARGET_FLUSH |
                           PIPE_CONTROL_DEPTH_CACHE_FLUSH |
   PIPE_CONTROL_WRITE_IMMEDIATE |
   PIPE_CONTROL_WRITE_DEPTH_COUNT |
   if (!(flags & wa_bits))
               if (INTEL_NEEDS_WA_1409600907 && (flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH)) {
      /* Wa_1409600907:
   *
   * "PIPE_CONTROL with Depth Stall Enable bit must be set
   * with any PIPE_CONTROL with Depth Flush Enable bit set.
   */
                     #if INTEL_NEEDS_WA_14010840176
      /* "If the intention of “constant cache invalidate” is
   *  to invalidate the L1 cache (which can cache constants), use “HDC
   *  pipeline flush” instead of Constant Cache invalidate command."
   *
   * "If L3 invalidate is needed, the w/a should be to set state invalidate
   * in the pipe control command, in addition to the HDC pipeline flush."
   */
   if (flags & PIPE_CONTROL_CONST_CACHE_INVALIDATE) {
      flags &= ~PIPE_CONTROL_CONST_CACHE_INVALIDATE;
         #endif
                  if (INTEL_DEBUG(DEBUG_PIPE_CONTROL)) {
      fprintf(stderr,
         "  PC [%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%"PRIx64"]: %s\n",
   (flags & PIPE_CONTROL_FLUSH_ENABLE) ? "PipeCon " : "",
   (flags & PIPE_CONTROL_CS_STALL) ? "CS " : "",
   (flags & PIPE_CONTROL_STALL_AT_SCOREBOARD) ? "Scoreboard " : "",
   (flags & PIPE_CONTROL_VF_CACHE_INVALIDATE) ? "VF " : "",
   (flags & PIPE_CONTROL_RENDER_TARGET_FLUSH) ? "RT " : "",
   (flags & PIPE_CONTROL_CONST_CACHE_INVALIDATE) ? "Const " : "",
   (flags & PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE) ? "TC " : "",
   (flags & PIPE_CONTROL_DATA_CACHE_FLUSH) ? "DC " : "",
   (flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH) ? "ZFlush " : "",
   (flags & PIPE_CONTROL_TILE_CACHE_FLUSH) ? "Tile " : "",
   (flags & PIPE_CONTROL_CCS_CACHE_FLUSH) ? "CCS " : "",
   (flags & PIPE_CONTROL_DEPTH_STALL) ? "ZStall " : "",
   (flags & PIPE_CONTROL_STATE_CACHE_INVALIDATE) ? "State " : "",
   (flags & PIPE_CONTROL_TLB_INVALIDATE) ? "TLB " : "",
   (flags & PIPE_CONTROL_INSTRUCTION_INVALIDATE) ? "Inst " : "",
   (flags & PIPE_CONTROL_MEDIA_STATE_CLEAR) ? "MediaClear " : "",
   (flags & PIPE_CONTROL_NOTIFY_ENABLE) ? "Notify " : "",
   (flags & PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET) ?
         (flags & PIPE_CONTROL_INDIRECT_STATE_POINTERS_DISABLE) ?
         (flags & PIPE_CONTROL_WRITE_IMMEDIATE) ? "WriteImm " : "",
   (flags & PIPE_CONTROL_WRITE_DEPTH_COUNT) ? "WriteZCount " : "",
   (flags & PIPE_CONTROL_WRITE_TIMESTAMP) ? "WriteTimestamp " : "",
   (flags & PIPE_CONTROL_FLUSH_HDC) ? "HDC " : "",
   (flags & PIPE_CONTROL_PSS_STALL_SYNC) ? "PSS " : "",
                        const bool trace_pc =
            if (trace_pc)
               #if GFX_VERx10 >= 125
         #endif
   #if GFX_VER >= 12
         #endif
   #if GFX_VER > 11
         #endif
   #if GFX_VERx10 >= 125
         pc.UntypedDataPortCacheFlushEnable =
      (flags & (PIPE_CONTROL_UNTYPED_DATAPORT_CACHE_FLUSH |
                  pc.HDCPipelineFlushEnable |= pc.UntypedDataPortCacheFlushEnable;
   #endif
         pc.LRIPostSyncOperation = NoLRIOperation;
   pc.PipeControlFlushEnable = flags & PIPE_CONTROL_FLUSH_ENABLE;
   pc.DCFlushEnable = flags & PIPE_CONTROL_DATA_CACHE_FLUSH;
   pc.StoreDataIndex = 0;
   #if GFX_VERx10 < 125
         pc.GlobalSnapshotCountReset =
   #endif
         pc.TLBInvalidate = flags & PIPE_CONTROL_TLB_INVALIDATE;
   pc.GenericMediaStateClear = flags & PIPE_CONTROL_MEDIA_STATE_CLEAR;
   pc.StallAtPixelScoreboard = flags & PIPE_CONTROL_STALL_AT_SCOREBOARD;
   pc.RenderTargetCacheFlushEnable =
         pc.DepthCacheFlushEnable = flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH;
   pc.StateCacheInvalidationEnable =
   #if GFX_VER >= 12
         pc.L3ReadOnlyCacheInvalidationEnable =
   #endif
         pc.VFCacheInvalidationEnable = flags & PIPE_CONTROL_VF_CACHE_INVALIDATE;
   pc.ConstantCacheInvalidationEnable =
         pc.PostSyncOperation = flags_to_post_sync_op(flags);
   pc.DepthStallEnable = flags & PIPE_CONTROL_DEPTH_STALL;
   pc.InstructionCacheInvalidateEnable =
         pc.NotifyEnable = flags & PIPE_CONTROL_NOTIFY_ENABLE;
   pc.IndirectStatePointersDisable =
         pc.TextureCacheInvalidationEnable =
         pc.Address = rw_bo(bo, offset, IRIS_DOMAIN_OTHER_WRITE);
               if (trace_pc) {
      trace_intel_end_stall(&batch->trace, flags,
                        }
      #if GFX_VER == 9
   /**
   * Preemption on Gfx9 has to be enabled or disabled in various cases.
   *
   * See these workarounds for preemption:
   *  - WaDisableMidObjectPreemptionForGSLineStripAdj
   *  - WaDisableMidObjectPreemptionForTrifanOrPolygon
   *  - WaDisableMidObjectPreemptionForLineLoop
   *  - WA#0798
   *
   * We don't put this in the vtable because it's only used on Gfx9.
   */
   void
   gfx9_toggle_preemption(struct iris_context *ice,
               {
      struct iris_genx_state *genx = ice->state.genx;
            /* WaDisableMidObjectPreemptionForGSLineStripAdj
   *
   *    "WA: Disable mid-draw preemption when draw-call is a linestrip_adj
   *     and GS is enabled."
   */
   if (draw->mode == MESA_PRIM_LINE_STRIP_ADJACENCY &&
      ice->shaders.prog[MESA_SHADER_GEOMETRY])
         /* WaDisableMidObjectPreemptionForTrifanOrPolygon
   *
   *    "TriFan miscompare in Execlist Preemption test. Cut index that is
   *     on a previous context. End the previous, the resume another context
   *     with a tri-fan or polygon, and the vertex count is corrupted. If we
   *     prempt again we will cause corruption.
   *
   *     WA: Disable mid-draw preemption when draw-call has a tri-fan."
   */
   if (draw->mode == MESA_PRIM_TRIANGLE_FAN)
            /* WaDisableMidObjectPreemptionForLineLoop
   *
   *    "VF Stats Counters Missing a vertex when preemption enabled.
   *
   *     WA: Disable mid-draw preemption when the draw uses a lineloop
   *     topology."
   */
   if (draw->mode == MESA_PRIM_LINE_LOOP)
            /* WA#0798
   *
   *    "VF is corrupting GAFS data when preempted on an instance boundary
   *     and replayed with instancing enabled.
   *
   *     WA: Disable preemption when using instanceing."
   */
   if (draw->instance_count > 1)
            if (genx->object_preemption != object_preemption) {
      iris_enable_obj_preemption(batch, object_preemption);
         }
   #endif
      static void
   iris_lost_genx_state(struct iris_context *ice, struct iris_batch *batch)
   {
            #if INTEL_NEEDS_WA_1808121037
         #endif
            }
      static void
   iris_emit_mi_report_perf_count(struct iris_batch *batch,
                     {
      iris_batch_sync_region_start(batch);
   iris_emit_cmd(batch, GENX(MI_REPORT_PERF_COUNT), mi_rpc) {
      mi_rpc.MemoryAddress = rw_bo(bo, offset_in_bytes,
            }
      }
      /**
   * Update the pixel hashing modes that determine the balancing of PS threads
   * across subslices and slices.
   *
   * \param width Width bound of the rendering area (already scaled down if \p
   *              scale is greater than 1).
   * \param height Height bound of the rendering area (already scaled down if \p
   *               scale is greater than 1).
   * \param scale The number of framebuffer samples that could potentially be
   *              affected by an individual channel of the PS thread.  This is
   *              typically one for single-sampled rendering, but for operations
   *              like CCS resolves and fast clears a single PS invocation may
   *              update a huge number of pixels, in which case a finer
   *              balancing is desirable in order to maximally utilize the
   *              bandwidth available.  UINT_MAX can be used as shorthand for
   *              "finest hashing mode available".
   */
   void
   genX(emit_hashing_mode)(struct iris_context *ice, struct iris_batch *batch,
         {
   #if GFX_VER == 9
      const struct intel_device_info *devinfo = batch->screen->devinfo;
   const unsigned slice_hashing[] = {
      /* Because all Gfx9 platforms with more than one slice require
   * three-way subslice hashing, a single "normal" 16x16 slice hashing
   * block is guaranteed to suffer from substantial imbalance, with one
   * subslice receiving twice as much work as the other two in the
   * slice.
   *
   * The performance impact of that would be particularly severe when
   * three-way hashing is also in use for slice balancing (which is the
   * case for all Gfx9 GT4 platforms), because one of the slices
   * receives one every three 16x16 blocks in either direction, which
   * is roughly the periodicity of the underlying subslice imbalance
   * pattern ("roughly" because in reality the hardware's
   * implementation of three-way hashing doesn't do exact modulo 3
   * arithmetic, which somewhat decreases the magnitude of this effect
   * in practice).  This leads to a systematic subslice imbalance
   * within that slice regardless of the size of the primitive.  The
   * 32x32 hashing mode guarantees that the subslice imbalance within a
   * single slice hashing block is minimal, largely eliminating this
   * effect.
   */
   _32x32,
   /* Finest slice hashing mode available. */
      };
   const unsigned subslice_hashing[] = {
      /* 16x16 would provide a slight cache locality benefit especially
   * visible in the sampler L1 cache efficiency of low-bandwidth
   * non-LLC platforms, but it comes at the cost of greater subslice
   * imbalance for primitives of dimensions approximately intermediate
   * between 16x4 and 16x16.
   */
   _16x4,
   /* Finest subslice hashing mode available. */
      };
   /* Dimensions of the smallest hashing block of a given hashing mode.  If
   * the rendering area is smaller than this there can't possibly be any
   * benefit from switching to this mode, so we optimize out the
   * transition.
   */
   const unsigned min_size[][2] = {
      { 16, 4 },
      };
            if (width > min_size[idx][0] || height > min_size[idx][1]) {
      iris_emit_raw_pipe_control(batch,
                              iris_emit_reg(batch, GENX(GT_MODE), reg) {
      reg.SliceHashing = (devinfo->num_slices > 1 ? slice_hashing[idx] : 0);
   reg.SliceHashingMask = (devinfo->num_slices > 1 ? -1 : 0);
   reg.SubsliceHashing = subslice_hashing[idx];
                     #endif
   }
      static void
   iris_set_frontend_noop(struct pipe_context *ctx, bool enable)
   {
               if (iris_batch_prepare_noop(&ice->batches[IRIS_BATCH_RENDER], enable)) {
      ice->state.dirty |= IRIS_ALL_DIRTY_FOR_RENDER;
               if (iris_batch_prepare_noop(&ice->batches[IRIS_BATCH_COMPUTE], enable)) {
      ice->state.dirty |= IRIS_ALL_DIRTY_FOR_COMPUTE;
         }
      void
   genX(init_screen_state)(struct iris_screen *screen)
   {
      assert(screen->devinfo->verx10 == GFX_VERx10);
   screen->vtbl.destroy_state = iris_destroy_state;
   screen->vtbl.init_render_context = iris_init_render_context;
   screen->vtbl.init_compute_context = iris_init_compute_context;
   screen->vtbl.upload_render_state = iris_upload_render_state;
   screen->vtbl.update_binder_address = iris_update_binder_address;
   screen->vtbl.upload_compute_state = iris_upload_compute_state;
   screen->vtbl.emit_raw_pipe_control = iris_emit_raw_pipe_control;
   screen->vtbl.rewrite_compute_walker_pc = iris_rewrite_compute_walker_pc;
   screen->vtbl.emit_mi_report_perf_count = iris_emit_mi_report_perf_count;
   screen->vtbl.rebind_buffer = iris_rebind_buffer;
   screen->vtbl.load_register_reg32 = iris_load_register_reg32;
   screen->vtbl.load_register_reg64 = iris_load_register_reg64;
   screen->vtbl.load_register_imm32 = iris_load_register_imm32;
   screen->vtbl.load_register_imm64 = iris_load_register_imm64;
   screen->vtbl.load_register_mem32 = iris_load_register_mem32;
   screen->vtbl.load_register_mem64 = iris_load_register_mem64;
   screen->vtbl.store_register_mem32 = iris_store_register_mem32;
   screen->vtbl.store_register_mem64 = iris_store_register_mem64;
   screen->vtbl.store_data_imm32 = iris_store_data_imm32;
   screen->vtbl.store_data_imm64 = iris_store_data_imm64;
   screen->vtbl.copy_mem_mem = iris_copy_mem_mem;
   screen->vtbl.derived_program_state_size = iris_derived_program_state_size;
   screen->vtbl.store_derived_program_state = iris_store_derived_program_state;
   screen->vtbl.create_so_decl_list = iris_create_so_decl_list;
   screen->vtbl.populate_vs_key = iris_populate_vs_key;
   screen->vtbl.populate_tcs_key = iris_populate_tcs_key;
   screen->vtbl.populate_tes_key = iris_populate_tes_key;
   screen->vtbl.populate_gs_key = iris_populate_gs_key;
   screen->vtbl.populate_fs_key = iris_populate_fs_key;
   screen->vtbl.populate_cs_key = iris_populate_cs_key;
   screen->vtbl.lost_genx_state = iris_lost_genx_state;
      }
      void
   genX(init_state)(struct iris_context *ice)
   {
      struct pipe_context *ctx = &ice->ctx;
            ctx->create_blend_state = iris_create_blend_state;
   ctx->create_depth_stencil_alpha_state = iris_create_zsa_state;
   ctx->create_rasterizer_state = iris_create_rasterizer_state;
   ctx->create_sampler_state = iris_create_sampler_state;
   ctx->create_sampler_view = iris_create_sampler_view;
   ctx->create_surface = iris_create_surface;
   ctx->create_vertex_elements_state = iris_create_vertex_elements;
   ctx->bind_blend_state = iris_bind_blend_state;
   ctx->bind_depth_stencil_alpha_state = iris_bind_zsa_state;
   ctx->bind_sampler_states = iris_bind_sampler_states;
   ctx->bind_rasterizer_state = iris_bind_rasterizer_state;
   ctx->bind_vertex_elements_state = iris_bind_vertex_elements_state;
   ctx->delete_blend_state = iris_delete_state;
   ctx->delete_depth_stencil_alpha_state = iris_delete_state;
   ctx->delete_rasterizer_state = iris_delete_state;
   ctx->delete_sampler_state = iris_delete_state;
   ctx->delete_vertex_elements_state = iris_delete_state;
   ctx->set_blend_color = iris_set_blend_color;
   ctx->set_clip_state = iris_set_clip_state;
   ctx->set_constant_buffer = iris_set_constant_buffer;
   ctx->set_shader_buffers = iris_set_shader_buffers;
   ctx->set_shader_images = iris_set_shader_images;
   ctx->set_sampler_views = iris_set_sampler_views;
   ctx->set_compute_resources = iris_set_compute_resources;
   ctx->set_global_binding = iris_set_global_binding;
   ctx->set_tess_state = iris_set_tess_state;
   ctx->set_patch_vertices = iris_set_patch_vertices;
   ctx->set_framebuffer_state = iris_set_framebuffer_state;
   ctx->set_polygon_stipple = iris_set_polygon_stipple;
   ctx->set_sample_mask = iris_set_sample_mask;
   ctx->set_scissor_states = iris_set_scissor_states;
   ctx->set_stencil_ref = iris_set_stencil_ref;
   ctx->set_vertex_buffers = iris_set_vertex_buffers;
   ctx->set_viewport_states = iris_set_viewport_states;
   ctx->sampler_view_destroy = iris_sampler_view_destroy;
   ctx->surface_destroy = iris_surface_destroy;
   ctx->draw_vbo = iris_draw_vbo;
   ctx->launch_grid = iris_launch_grid;
   ctx->create_stream_output_target = iris_create_stream_output_target;
   ctx->stream_output_target_destroy = iris_stream_output_target_destroy;
   ctx->set_stream_output_targets = iris_set_stream_output_targets;
            ice->state.dirty = ~0ull;
                     ice->state.sample_mask = 0xffff;
   ice->state.num_viewports = 1;
   ice->state.prim_mode = MESA_PRIM_COUNT;
   ice->state.genx = calloc(1, sizeof(struct iris_genx_state));
         #if GFX_VERx10 >= 120
         #endif
         /* Make a 1x1x1 null surface for unbound textures */
   void *null_surf_map =
      upload_state(ice->state.surface_uploader, &ice->state.unbound_tex,
      isl_null_fill_state(&screen->isl_dev, null_surf_map,
         ice->state.unbound_tex.offset +=
            /* Default all scissor rectangles to be empty regions. */
   for (int i = 0; i < IRIS_MAX_VIEWPORTS; i++) {
      ice->state.scissors[i] = (struct pipe_scissor_state) {
               }
