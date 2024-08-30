   /*
   * Copyright © 2014-2017 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      /**
   * @file v3dx_simulator.c
   *
   * Implements the actual HW interaction betweeh the GL driver's V3D simulator and the simulator.
   *
   * The register headers between V3D versions will have conflicting defines, so
   * all register interactions appear in this file and are compiled per V3D version
   * we support.
   */
      #ifdef USE_V3D_SIMULATOR
      #include <assert.h>
   #include <stdbool.h>
   #include <stdio.h>
      #include "v3d_simulator.h"
   #include "v3d_simulator_wrapper.h"
      #include "common/v3d_performance_counters.h"
      #include "util/macros.h"
   #include "util/bitscan.h"
   #include "drm-uapi/v3d_drm.h"
      #define HW_REGISTER_RO(x) (x)
   #define HW_REGISTER_RW(x) (x)
   #if V3D_VERSION == 71
   #include "libs/core/v3d/registers/7.1.6.0/v3d.h"
   #else
   #if V3D_VERSION == 41 || V3D_VERSION == 42
   #include "libs/core/v3d/registers/4.2.14.0/v3d.h"
   #else
   #include "libs/core/v3d/registers/3.3.0.0/v3d.h"
   #endif
   #endif
      #define V3D_WRITE(reg, val) v3d_hw_write_reg(v3d, reg, val)
   #define V3D_READ(reg) v3d_hw_read_reg(v3d, reg)
      static void
   v3d_invalidate_l3(struct v3d_hw *v3d)
   {
   #if V3D_VERSION < 40
                  V3D_WRITE(V3D_GCA_CACHE_CTRL, gca_ctrl | V3D_GCA_CACHE_CTRL_FLUSH_SET);
   #endif
   }
      /* Invalidates the L2C cache.  This is a read-only cache for uniforms and instructions. */
   static void
   v3d_invalidate_l2c(struct v3d_hw *v3d)
   {
         if (V3D_VERSION >= 33)
            V3D_WRITE(V3D_CTL_0_L2CACTL,
         }
      enum v3d_l2t_cache_flush_mode {
         V3D_CACHE_FLUSH_MODE_FLUSH,
   V3D_CACHE_FLUSH_MODE_CLEAR,
   };
      /* Invalidates texture L2 cachelines */
   static void
   v3d_invalidate_l2t(struct v3d_hw *v3d)
   {
         V3D_WRITE(V3D_CTL_0_L2TFLSTA, 0);
   V3D_WRITE(V3D_CTL_0_L2TFLEND, ~0);
   V3D_WRITE(V3D_CTL_0_L2TCACTL,
         }
      /*
   * Wait for l2tcactl, used for flushes.
   *
   * FIXME: for a multicore scenario we should pass here the core. All wrapper
   * assumes just one core, so would be better to handle that on that case.
   */
   static UNUSED void v3d_core_wait_l2tcactl(struct v3d_hw *v3d,
         {
               while (V3D_READ(V3D_CTL_0_L2TCACTL) & ctrl) {
            }
      /* Flushes dirty texture cachelines from the L1 write combiner */
   static void
   v3d_flush_l1td(struct v3d_hw *v3d)
   {
         V3D_WRITE(V3D_CTL_0_L2TCACTL,
            /* Note: here the kernel (and previous versions of the simulator
      * wrapper) is using V3D_CTL_0_L2TCACTL_L2TFLS_SET, as with l2t. We
   * understand that it makes more sense to do like this. We need to
   * confirm which one is doing it correctly. So far things work fine on
   * the simulator this way.
      }
      /* Flushes dirty texture L2 cachelines */
   static void
   v3d_flush_l2t(struct v3d_hw *v3d)
   {
         V3D_WRITE(V3D_CTL_0_L2TFLSTA, 0);
   V3D_WRITE(V3D_CTL_0_L2TFLEND, ~0);
   V3D_WRITE(V3D_CTL_0_L2TCACTL,
                  }
      /* Invalidates the slice caches.  These are read-only caches. */
   static void
   v3d_invalidate_slices(struct v3d_hw *v3d)
   {
         }
      static void
   v3d_invalidate_caches(struct v3d_hw *v3d)
   {
         v3d_invalidate_l3(v3d);
   v3d_invalidate_l2c(v3d);
   v3d_invalidate_l2t(v3d);
   }
      static uint32_t g_gmp_ofs;
   static void
   v3d_reload_gmp(struct v3d_hw *v3d)
   {
         /* Completely reset the GMP. */
   V3D_WRITE(V3D_GMP_CFG,
         V3D_WRITE(V3D_GMP_TABLE_ADDR, g_gmp_ofs);
   V3D_WRITE(V3D_GMP_CLEAR_LOAD, ~0);
   while (V3D_READ(V3D_GMP_STATUS) &
               }
      static UNUSED void
   v3d_flush_caches(struct v3d_hw *v3d)
   {
         v3d_flush_l1td(v3d);
   }
      #if V3D_VERSION < 71
   #define TFU_REG(NAME) V3D_TFU_ ## NAME
   #else
   #define TFU_REG(NAME) V3D_IFC_ ## NAME
   #endif
         int
   v3dX(simulator_submit_tfu_ioctl)(struct v3d_hw *v3d,
         {
                  V3D_WRITE(TFU_REG(IIA), args->iia);
   V3D_WRITE(TFU_REG(IIS), args->iis);
   V3D_WRITE(TFU_REG(ICA), args->ica);
   V3D_WRITE(TFU_REG(IUA), args->iua);
   #if V3D_VERSION >= 71
         #endif
         V3D_WRITE(TFU_REG(IOS), args->ios);
   V3D_WRITE(TFU_REG(COEF0), args->coef[0]);
   V3D_WRITE(TFU_REG(COEF1), args->coef[1]);
   V3D_WRITE(TFU_REG(COEF2), args->coef[2]);
                     while ((V3D_READ(TFU_REG(CS)) & V3D_TFU_CS_CVTCT_SET) == last_vtct) {
                  }
      int
   v3dX(simulator_submit_csd_ioctl)(struct v3d_hw *v3d,
               {
   #if V3D_VERSION >= 41
         int last_completed_jobs = (V3D_READ(V3D_CSD_0_STATUS) &
         g_gmp_ofs = gmp_ofs;
                     V3D_WRITE(V3D_CSD_0_QUEUED_CFG1, args->cfg[1]);
   V3D_WRITE(V3D_CSD_0_QUEUED_CFG2, args->cfg[2]);
   V3D_WRITE(V3D_CSD_0_QUEUED_CFG3, args->cfg[3]);
   V3D_WRITE(V3D_CSD_0_QUEUED_CFG4, args->cfg[4]);
   V3D_WRITE(V3D_CSD_0_QUEUED_CFG5, args->cfg[5]);
   #if V3D_VERSION >= 71
         #endif
         /* CFG0 kicks off the job */
            /* Now we wait for the dispatch to finish. The safest way is to check
      * if NUM_COMPLETED_JOBS has increased. Note that in spite of that
   * name that register field is about the number of completed
   * dispatches.
      while ((V3D_READ(V3D_CSD_0_STATUS) &
                                 #else
         #endif
   }
      int
   v3dX(simulator_get_param_ioctl)(struct v3d_hw *v3d,
         {
         static const uint32_t reg_map[] = {
            [DRM_V3D_PARAM_V3D_UIFCFG] = V3D_HUB_CTL_UIFCFG,
   [DRM_V3D_PARAM_V3D_HUB_IDENT1] = V3D_HUB_CTL_IDENT1,
   [DRM_V3D_PARAM_V3D_HUB_IDENT2] = V3D_HUB_CTL_IDENT2,
   [DRM_V3D_PARAM_V3D_HUB_IDENT3] = V3D_HUB_CTL_IDENT3,
   [DRM_V3D_PARAM_V3D_CORE0_IDENT0] = V3D_CTL_0_IDENT0,
               switch (args->param) {
   case DRM_V3D_PARAM_SUPPORTS_TFU:
               case DRM_V3D_PARAM_SUPPORTS_CSD:
               case DRM_V3D_PARAM_SUPPORTS_CACHE_FLUSH:
               case DRM_V3D_PARAM_SUPPORTS_PERFMON:
               case DRM_V3D_PARAM_SUPPORTS_MULTISYNC_EXT:
                        if (args->param < ARRAY_SIZE(reg_map) && reg_map[args->param]) {
                        fprintf(stderr, "Unknown DRM_IOCTL_V3D_GET_PARAM(%lld)\n",
         }
      static struct v3d_hw *v3d_isr_hw;
         static void
   v3d_isr_core(struct v3d_hw *v3d,
         {
         /* FIXME: so far we are assuming just one core, and using only the _0_
      * registers. If we add multiple-core on the simulator, we would need
   * to pass core as a parameter, and chose the proper registers.
      assert(core == 0);
   uint32_t core_status = V3D_READ(V3D_CTL_0_INT_STS);
            if (core_status & V3D_CTL_0_INT_STS_INT_OUTOMEM_SET) {
                                    V3D_WRITE(V3D_PTB_0_BPOA, offset);
         #if V3D_VERSION <= 42
         if (core_status & V3D_CTL_0_INT_STS_INT_GMPV_SET) {
               } else {
            fprintf(stderr,
      }
   #endif
   }
      static void
   handle_mmu_interruptions(struct v3d_hw *v3d,
         {
         bool wrv = hub_status & V3D_HUB_CTL_INT_STS_INT_MMU_WRV_SET;
   bool pti = hub_status & V3D_HUB_CTL_INT_STS_INT_MMU_PTI_SET;
            if (!(pti || cap || wrv))
            const char *client = "?";
   uint32_t axi_id = V3D_READ(V3D_MMU_VIO_ID);
      #if V3D_VERSION >= 41
         static const char *const v3d41_axi_ids[] = {
            "L2T",
   "PTB",
   "PSE",
   "TLB",
   "CLE",
   "TFU",
               axi_id = axi_id >> 5;
   if (axi_id < ARRAY_SIZE(v3d41_axi_ids))
                     va_width += ((mmu_debug & V3D_MMU_DEBUG_INFO_VA_WIDTH_SET)
   #endif
         /* Only the top bits (final number depends on the gen) of the virtual
      * address are reported in the MMU VIO_ADDR register.
      uint64_t vio_addr = ((uint64_t)V3D_READ(V3D_MMU_VIO_ADDR) <<
            /* Difference with the kernel: here were are going to abort after
      * logging, so we don't bother with some stuff that the kernel does,
               fprintf(stderr, "MMU error from client %s (%d) at 0x%llx%s%s%s\n",
            client, axi_id, (long long) vio_addr,
               }
      static void
   v3d_isr_hub(struct v3d_hw *v3d)
   {
                  /* Acknowledge the interrupts we're handling here */
            if (hub_status & V3D_HUB_CTL_INT_STS_INT_TFUC_SET) {
            /* FIXME: we were not able to raise this exception. We let the
   * unreachable here, so we could get one if it is raised on
   * the future. In any case, note that for this case we would
   * only be doing debugging log.
                  #if V3D_VERSION == 71
         if (hub_status & V3D_HUB_CTL_INT_STS_INT_GMPV_SET) {
               } else {
            fprintf(stderr,
      }
   #endif
   }
      static void
   v3d_isr(uint32_t hub_status)
   {
         struct v3d_hw *v3d = v3d_isr_hw;
            /* Check the hub_status bits */
   while (mask) {
                     if (core == v3d_hw_get_hub_core())
                     }
      void
   v3dX(simulator_init_regs)(struct v3d_hw *v3d)
   {
   #if V3D_VERSION == 33
         /* Set OVRTMUOUT to match kernel behavior.
      *
   * This means that the texture sampler uniform configuration's tmu
   * output type field is used, instead of using the hardware default
   * behavior based on the texture type.  If you want the default
   * behavior, you can still put "2" in the indirect texture state's
   * output_type field.
      #endif
            /* FIXME: the kernel captures some additional core interrupts here,
      * for tracing. Perhaps we should evaluate to do the same here and add
   * some debug options.
      #if V3D_VERSION <= 42
         #endif
            V3D_WRITE(V3D_CTL_0_INT_MSK_SET, ~core_interrupts);
            uint32_t hub_interrupts =
      (V3D_HUB_CTL_INT_STS_INT_MMU_WRV_SET |  /* write violation */
      V3D_HUB_CTL_INT_STS_INT_MMU_PTI_SET |  /* page table invalid */
      #if V3D_VERSION == 71
         #endif
         V3D_WRITE(V3D_HUB_CTL_INT_MSK_SET, ~hub_interrupts);
            v3d_isr_hw = v3d;
   }
      void
   v3dX(simulator_submit_cl_ioctl)(struct v3d_hw *v3d,
               {
         int last_bfc = (V3D_READ(V3D_CLE_0_BFC) &
            int last_rfc = (V3D_READ(V3D_CLE_0_RFC) &
            g_gmp_ofs = gmp_ofs;
                     if (submit->qma) {
               #if V3D_VERSION >= 41
         if (submit->qts) {
            V3D_WRITE(V3D_CLE_0_CT0QTS,
      #endif
         V3D_WRITE(V3D_CLE_0_CT0QBA, submit->bcl_start);
            /* Wait for bin to complete before firing render.  The kernel's
      * scheduler implements this using the GPU scheduler blocking on the
   * bin fence completing.  (We don't use HW semaphores).
      while ((V3D_READ(V3D_CLE_0_BFC) &
                                 V3D_WRITE(V3D_CLE_0_CT1QBA, submit->rcl_start);
            while ((V3D_READ(V3D_CLE_0_RFC) &
               }
      #if V3D_VERSION >= 41
   #define V3D_PCTR_0_PCTR_N(x) (V3D_PCTR_0_PCTR0 + 4 * (x))
   #define V3D_PCTR_0_SRC_N(x) (V3D_PCTR_0_SRC_0_3 + 4 * (x))
   #define V3D_PCTR_0_SRC_N_SHIFT(x) ((x) * 8)
   #define V3D_PCTR_0_SRC_N_MASK(x) (BITFIELD_RANGE(V3D_PCTR_0_SRC_N_SHIFT(x), \
               #endif
      void
   v3dX(simulator_perfmon_start)(struct v3d_hw *v3d,
               {
   #if V3D_VERSION >= 41
         int i, j;
   uint32_t source;
            for (i = 0; i < ncounters; i+=4) {
            source = i / 4;
   uint32_t channels = 0;
   for (j = 0; j < 4 && (i + j) < ncounters; j++)
      }
   V3D_WRITE(V3D_PCTR_0_CLR, mask);
   V3D_WRITE(V3D_PCTR_0_OVERFLOW, mask);
   #endif
   }
      void v3dX(simulator_perfmon_stop)(struct v3d_hw *v3d,
               {
   #if V3D_VERSION >= 41
                  for (i = 0; i < ncounters; i++)
            #endif
   }
      void v3dX(simulator_get_perfcnt_total)(uint32_t *count)
   {
         }
      #endif /* USE_V3D_SIMULATOR */
