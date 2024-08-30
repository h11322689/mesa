   /*
   * Copyright 2014 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_binary.h"
      #include "ac_gpu_info.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include <sid.h>
   #include <stdio.h>
      #define SPILLED_SGPRS 0x4
   #define SPILLED_VGPRS 0x8
      /* Parse configuration data in .AMDGPU.config section format. */
   void ac_parse_shader_binary_config(const char *data, size_t nbytes, unsigned wave_size,
         {
      for (size_t i = 0; i < nbytes; i += 8) {
      unsigned reg = util_le32_to_cpu(*(uint32_t *)(data + i));
   unsigned value = util_le32_to_cpu(*(uint32_t *)(data + i + 4));
   switch (reg) {
   case R_00B028_SPI_SHADER_PGM_RSRC1_PS:
   case R_00B128_SPI_SHADER_PGM_RSRC1_VS:
   case R_00B228_SPI_SHADER_PGM_RSRC1_GS:
   case R_00B848_COMPUTE_PGM_RSRC1:
   case R_00B428_SPI_SHADER_PGM_RSRC1_HS:
      if (wave_size == 32 || info->wave64_vgpr_alloc_granularity == 8)
                        conf->num_sgprs = MAX2(conf->num_sgprs, (G_00B028_SGPRS(value) + 1) * 8);
   /* TODO: LLVM doesn't set FLOAT_MODE for non-compute shaders */
   conf->float_mode = G_00B028_FLOAT_MODE(value);
   conf->rsrc1 = value;
      case R_00B02C_SPI_SHADER_PGM_RSRC2_PS:
      conf->lds_size = MAX2(conf->lds_size, G_00B02C_EXTRA_LDS_SIZE(value));
   /* TODO: LLVM doesn't set SHARED_VGPR_CNT for all shader types */
   conf->num_shared_vgprs = G_00B02C_SHARED_VGPR_CNT(value);
   conf->rsrc2 = value;
      case R_00B12C_SPI_SHADER_PGM_RSRC2_VS:
      conf->num_shared_vgprs = G_00B12C_SHARED_VGPR_CNT(value);
   conf->rsrc2 = value;
      case R_00B22C_SPI_SHADER_PGM_RSRC2_GS:
      conf->num_shared_vgprs = G_00B22C_SHARED_VGPR_CNT(value);
   conf->rsrc2 = value;
      case R_00B42C_SPI_SHADER_PGM_RSRC2_HS:
      conf->num_shared_vgprs = G_00B42C_SHARED_VGPR_CNT(value);
   conf->rsrc2 = value;
      case R_00B84C_COMPUTE_PGM_RSRC2:
      conf->lds_size = MAX2(conf->lds_size, G_00B84C_LDS_SIZE(value));
   conf->rsrc2 = value;
      case R_00B8A0_COMPUTE_PGM_RSRC3:
      conf->num_shared_vgprs = G_00B8A0_SHARED_VGPR_CNT(value);
   conf->rsrc3 = value;
      case R_0286CC_SPI_PS_INPUT_ENA:
      conf->spi_ps_input_ena = value;
      case R_0286D0_SPI_PS_INPUT_ADDR:
      conf->spi_ps_input_addr = value;
      case R_0286E8_SPI_TMPRING_SIZE:
   case R_00B860_COMPUTE_TMPRING_SIZE:
      if (info->gfx_level >= GFX11)
         else
            case SPILLED_SGPRS:
      conf->spilled_sgprs = value;
      case SPILLED_VGPRS:
      conf->spilled_vgprs = value;
      default: {
               if (!printed) {
      fprintf(stderr,
         "Warning: LLVM emitted unknown "
   "config register: 0x%x\n",
         } break;
               if (!conf->spi_ps_input_addr)
            /* Enable 64-bit and 16-bit denormals, because there is no performance
   * cost.
   *
   * Don't enable denormals for 32-bit floats, because:
   * - denormals disable output modifiers
   * - denormals break v_mad_f32
   * - GFX6 & GFX7 would be very slow
   */
   conf->float_mode &= ~V_00B028_FP_32_DENORMS;
      }
      unsigned ac_align_shader_binary_for_prefetch(const struct radeon_info *info, unsigned size)
   {
      /* The SQ fetches up to N cache lines of 16 dwords
   * ahead of the PC, configurable by SH_MEM_CONFIG and
   * S_INST_PREFETCH. This can cause two issues:
   *
   * (1) Crossing a page boundary to an unmapped page. The logic
   *     does not distinguish between a required fetch and a "mere"
   *     prefetch and will fault.
   *
   * (2) Prefetching instructions that will be changed for a
   *     different shader.
   *
   * (2) is not currently an issue because we flush the I$ at IB
   * boundaries, but (1) needs to be addressed. Due to buffer
   * suballocation, we just play it safe.
   */
            if (!info->has_graphics && info->family >= CHIP_MI200)
         else if (info->gfx_level >= GFX10)
            if (prefetch_distance) {
      if (info->gfx_level >= GFX11)
         else
                  }
