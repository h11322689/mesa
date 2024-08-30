   /*
   * Copyright © 2017 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "amd_family.h"
   #include "addrlib/src/amdgpu_asic_addr.h"
   #include "util/macros.h"
      const char *ac_get_family_name(enum radeon_family family)
   {
      switch (family) {
   case CHIP_TAHITI:
         case CHIP_PITCAIRN:
         case CHIP_VERDE:
         case CHIP_OLAND:
         case CHIP_HAINAN:
         case CHIP_BONAIRE:
         case CHIP_KABINI:
         case CHIP_KAVERI:
         case CHIP_HAWAII:
         case CHIP_TONGA:
         case CHIP_ICELAND:
         case CHIP_CARRIZO:
         case CHIP_FIJI:
         case CHIP_STONEY:
         case CHIP_POLARIS10:
         case CHIP_POLARIS11:
         case CHIP_POLARIS12:
         case CHIP_VEGAM:
         case CHIP_VEGA10:
         case CHIP_RAVEN:
         case CHIP_VEGA12:
         case CHIP_VEGA20:
         case CHIP_RAVEN2:
         case CHIP_RENOIR:
         case CHIP_MI100:
         case CHIP_MI200:
         case CHIP_GFX940:
         case CHIP_NAVI10:
         case CHIP_NAVI12:
         case CHIP_NAVI14:
         case CHIP_NAVI21:
         case CHIP_NAVI22:
         case CHIP_NAVI23:
         case CHIP_VANGOGH:
         case CHIP_NAVI24:
         case CHIP_REMBRANDT:
         case CHIP_RAPHAEL_MENDOCINO:
         case CHIP_NAVI31:
         case CHIP_NAVI32:
         case CHIP_NAVI33:
         case CHIP_GFX1103_R1:
         case CHIP_GFX1103_R2:
         case CHIP_GFX1150:
         default:
            }
      enum amd_gfx_level ac_get_gfx_level(enum radeon_family family)
   {
      if (family >= CHIP_NAVI31)
         if (family >= CHIP_NAVI21)
         if (family >= CHIP_NAVI10)
         if (family >= CHIP_VEGA10)
         if (family >= CHIP_TONGA)
         if (family >= CHIP_BONAIRE)
               }
      unsigned ac_get_family_id(enum radeon_family family)
   {
      if (family >= CHIP_NAVI31)
         if (family >= CHIP_NAVI21)
         if (family >= CHIP_NAVI10)
         if (family >= CHIP_VEGA10)
         if (family >= CHIP_TONGA)
         if (family >= CHIP_BONAIRE)
               }
      const char *ac_get_llvm_processor_name(enum radeon_family family)
   {
      switch (family) {
   case CHIP_TAHITI:
         case CHIP_PITCAIRN:
         case CHIP_VERDE:
         case CHIP_OLAND:
         case CHIP_HAINAN:
         case CHIP_BONAIRE:
         case CHIP_KABINI:
         case CHIP_KAVERI:
         case CHIP_HAWAII:
         case CHIP_TONGA:
         case CHIP_ICELAND:
         case CHIP_CARRIZO:
         case CHIP_FIJI:
         case CHIP_STONEY:
         case CHIP_POLARIS10:
         case CHIP_POLARIS11:
   case CHIP_POLARIS12:
   case CHIP_VEGAM:
         case CHIP_VEGA10:
         case CHIP_RAVEN:
         case CHIP_VEGA12:
         case CHIP_VEGA20:
         case CHIP_RAVEN2:
   case CHIP_RENOIR:
         case CHIP_MI100:
         case CHIP_MI200:
         case CHIP_GFX940:
         case CHIP_NAVI10:
         case CHIP_NAVI12:
         case CHIP_NAVI14:
         case CHIP_NAVI21:
         case CHIP_NAVI22:
         case CHIP_NAVI23:
         case CHIP_VANGOGH:
         case CHIP_NAVI24:
         case CHIP_REMBRANDT:
         case CHIP_RAPHAEL_MENDOCINO:
         case CHIP_NAVI31:
         case CHIP_NAVI32:
         case CHIP_NAVI33:
         case CHIP_GFX1103_R1:
   case CHIP_GFX1103_R2:
         case CHIP_GFX1150:
         default:
            }
