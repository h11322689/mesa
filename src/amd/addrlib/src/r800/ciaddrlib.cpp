   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ****************************************************************************************************
   * @file  ciaddrlib.cpp
   * @brief Contains the implementation for the CiLib class.
   ****************************************************************************************************
   */
      #include "ciaddrlib.h"
      #include "si_gb_reg.h"
      #include "amdgpu_asic_addr.h"
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      namespace Addr
   {
      /**
   ****************************************************************************************************
   *   CiHwlInit
   *
   *   @brief
   *       Creates an CiLib object.
   *
   *   @return
   *       Returns an CiLib object pointer.
   ****************************************************************************************************
   */
   Lib* CiHwlInit(const Client* pClient)
   {
         }
      namespace V1
   {
      /**
   ****************************************************************************************************
   *   Mask
   *
   *   @brief
   *       Gets a mask of "width"
   *   @return
   *       Bit mask
   ****************************************************************************************************
   */
   static UINT_64 Mask(
         {
               if (width >= sizeof(UINT_64)*8)
   {
         }
   else
   {
         }
      }
      /**
   ****************************************************************************************************
   *   GetBits
   *
   *   @brief
   *       Gets bits within a range of [msb, lsb]
   *   @return
   *       Bits of this range
   ****************************************************************************************************
   */
   static UINT_64 GetBits(
      UINT_64 bits,   ///< Source bits
   UINT_32 msb,    ///< Most signicant bit
      {
               if (msb >= lsb)
   {
         }
      }
      /**
   ****************************************************************************************************
   *   RemoveBits
   *
   *   @brief
   *       Removes bits within the range of [msb, lsb]
   *   @return
   *       Modified bits
   ****************************************************************************************************
   */
   static UINT_64 RemoveBits(
      UINT_64 bits,   ///< Source bits
   UINT_32 msb,    ///< Most signicant bit
      {
               if (msb >= lsb)
   {
      ret = GetBits(bits, lsb - 1, 0) // low bits
      }
      }
      /**
   ****************************************************************************************************
   *   InsertBits
   *
   *   @brief
   *       Inserts new bits into the range of [msb, lsb]
   *   @return
   *       Modified bits
   ****************************************************************************************************
   */
   static UINT_64 InsertBits(
      UINT_64 bits,       ///< Source bits
   UINT_64 newBits,    ///< New bits to be inserted
   UINT_32 msb,        ///< Most signicant bit
      {
               if (msb >= lsb)
   {
      ret = GetBits(bits, lsb - 1, 0) // old low bitss
            }
      }
      /**
   ****************************************************************************************************
   *   CiLib::CiLib
   *
   *   @brief
   *       Constructor
   *
   ****************************************************************************************************
   */
   CiLib::CiLib(const Client* pClient)
      :
   SiLib(pClient),
   m_noOfMacroEntries(0),
      {
   }
      /**
   ****************************************************************************************************
   *   CiLib::~CiLib
   *
   *   @brief
   *       Destructor
   ****************************************************************************************************
   */
   CiLib::~CiLib()
   {
   }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeDccInfo
   *
   *   @brief
   *       Compute DCC key size, base alignment
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE CiLib::HwlComputeDccInfo(
      const ADDR_COMPUTE_DCCINFO_INPUT*  pIn,
      {
               if (SupportDccAndTcCompatibility() && IsMacroTiled(pIn->tileMode))
   {
                        if (pIn->numSamples > 1)
   {
                        if (samplesPerSplit < pIn->numSamples)
   {
                                       if (0 != (dccFastClearSize & (fastClearBaseAlign - 1)))
   {
      // Disable dcc fast clear
   // if key size of fisrt sample split is not pipe*interleave aligned
                  pOut->dccRamSize          = pIn->colorSurfSize >> 8;
   pOut->dccRamBaseAlign     = pIn->tileInfo.banks *
               pOut->dccFastClearSize    = dccFastClearSize;
                     if (0 == (pOut->dccRamSize & (pOut->dccRamBaseAlign - 1)))
   {
         }
   else
   {
                  if (pOut->dccRamSize == pOut->dccFastClearSize)
   {
         }
   if ((pOut->dccRamSize & (dccRamSizeAlign - 1)) != 0)
   {
         }
   pOut->dccRamSize          = PowTwoAlign(pOut->dccRamSize, dccRamSizeAlign);
      }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Compute tc compatible Cmask address from fmask ram address
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE CiLib::HwlComputeCmaskAddrFromCoord(
      const ADDR_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*  pIn,  ///< [in] fmask addr/bpp/tile input
   ADDR_COMPUTE_CMASK_ADDRFROMCOORD_OUTPUT*       pOut  ///< [out] cmask address
      {
               if ((SupportDccAndTcCompatibility() == TRUE) &&
         {
      UINT_32 numOfPipes   = HwlGetPipes(pIn->pTileInfo);
   UINT_32 numOfBanks   = pIn->pTileInfo->banks;
   UINT_64 fmaskAddress = pIn->fmaskAddr;
   UINT_32 elemBits     = pIn->bpp;
   UINT_32 blockByte    = 64 * elemBits / 8;
   UINT_64 metaNibbleAddress = HwlComputeMetadataNibbleAddress(fmaskAddress,
                                                         pOut->addr = (metaNibbleAddress >> 1);
   pOut->bitPosition = (metaNibbleAddress % 2) ? 4 : 0;
                  }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeHtileAddrFromCoord
   *
   *   @brief
   *       Compute tc compatible Htile address from depth/stencil address
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE CiLib::HwlComputeHtileAddrFromCoord(
      const ADDR_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*  pIn,  ///< [in] depth/stencil addr/bpp/tile input
   ADDR_COMPUTE_HTILE_ADDRFROMCOORD_OUTPUT*       pOut  ///< [out] htile address
      {
               if ((SupportDccAndTcCompatibility() == TRUE) &&
         {
      UINT_32 numOfPipes   = HwlGetPipes(pIn->pTileInfo);
   UINT_32 numOfBanks   = pIn->pTileInfo->banks;
   UINT_64 zStencilAddr = pIn->zStencilAddr;
   UINT_32 elemBits     = pIn->bpp;
   UINT_32 blockByte    = 64 * elemBits / 8;
   UINT_64 metaNibbleAddress = HwlComputeMetadataNibbleAddress(zStencilAddr,
                                                         pOut->addr = (metaNibbleAddress >> 1);
   pOut->bitPosition = 0;
                  }
      /**
   ****************************************************************************************************
   *   CiLib::HwlConvertChipFamily
   *
   *   @brief
   *       Convert familyID defined in atiid.h to ChipFamily and set m_chipFamily/m_chipRevision
   *   @return
   *       ChipFamily
   ****************************************************************************************************
   */
   ChipFamily CiLib::HwlConvertChipFamily(
      UINT_32 uChipFamily,        ///< [in] chip family defined in atiih.h
      {
               switch (uChipFamily)
   {
      case FAMILY_CI:
         m_settings.isSeaIsland  = 1;
   m_settings.isBonaire    = ASICREV_IS_BONAIRE_M(uChipRevision);
   m_settings.isHawaii     = ASICREV_IS_HAWAII_P(uChipRevision);
   case FAMILY_KV:
         m_settings.isKaveri     = 1;
   m_settings.isSpectre    = ASICREV_IS_SPECTRE(uChipRevision);
   m_settings.isSpooky     = ASICREV_IS_SPOOKY(uChipRevision);
   m_settings.isKalindi    = ASICREV_IS_KALINDI(uChipRevision);
   case FAMILY_VI:
         m_settings.isVolcanicIslands = 1;
   m_settings.isIceland         = ASICREV_IS_ICELAND_M(uChipRevision);
   m_settings.isTonga           = ASICREV_IS_TONGA_P(uChipRevision);
   m_settings.isFiji            = ASICREV_IS_FIJI_P(uChipRevision);
   m_settings.isPolaris10       = ASICREV_IS_POLARIS10_P(uChipRevision);
   m_settings.isPolaris11       = ASICREV_IS_POLARIS11_M(uChipRevision);
   m_settings.isPolaris12       = ASICREV_IS_POLARIS12_V(uChipRevision);
   m_settings.isVegaM           = ASICREV_IS_VEGAM_P(uChipRevision);
   family = ADDR_CHIP_FAMILY_VI;
   case FAMILY_CZ:
         m_settings.isCarrizo         = 1;
   m_settings.isVolcanicIslands = 1;
   family = ADDR_CHIP_FAMILY_VI;
   default:
                        }
      /**
   ****************************************************************************************************
   *   CiLib::HwlInitGlobalParams
   *
   *   @brief
   *       Initializes global parameters
   *
   *   @return
   *       TRUE if all settings are valid
   *
   ****************************************************************************************************
   */
   BOOL_32 CiLib::HwlInitGlobalParams(
         {
                                 // The following assignments for m_pipes is only for fail-safe, InitTileSettingTable should
   // read the correct pipes from tile mode table
   if (m_settings.isHawaii)
   {
         }
   else if (m_settings.isBonaire || m_settings.isSpectre)
   {
         }
   else // Treat other KV asics to be 2-pipe
   {
                  // @todo: VI
   // Move this to VI code path once created
   if (m_settings.isTonga || m_settings.isPolaris10)
   {
         }
   else if (m_settings.isIceland)
   {
         }
   else if (m_settings.isFiji)
   {
         }
   else if (m_settings.isPolaris11 || m_settings.isPolaris12)
   {
         }
   else if (m_settings.isVegaM)
   {
                  if (valid)
   {
         }
   if (valid)
   {
                  if (valid)
   {
                     }
      /**
   ****************************************************************************************************
   *   CiLib::HwlPostCheckTileIndex
   *
   *   @brief
   *       Map a tile setting to index if curIndex is invalid, otherwise check if curIndex matches
   *       tile mode/type/info and change the index if needed
   *   @return
   *       Tile index.
   ****************************************************************************************************
   */
   INT_32 CiLib::HwlPostCheckTileIndex(
      const ADDR_TILEINFO* pInfo,     ///< [in] Tile Info
   AddrTileMode         mode,      ///< [in] Tile mode
   AddrTileType         type,      ///< [in] Tile type
   INT                  curIndex   ///< [in] Current index assigned in HwlSetupTileInfo
      {
               if (mode == ADDR_TM_LINEAR_GENERAL)
   {
         }
   else
   {
               // We need to find a new index if either of them is true
   // 1. curIndex is invalid
   // 2. tile mode is changed
   // 3. tile info does not match for macro tiled
   if ((index == TileIndexInvalid)         ||
         (mode != m_tileTable[index].mode)   ||
   {
         for (index = 0; index < static_cast<INT_32>(m_noOfEntries); index++)
   {
      if (macroTiled)
   {
      // macro tile modes need all to match
   if ((pInfo->pipeConfig == m_tileTable[index].info.pipeConfig) &&
         (mode == m_tileTable[index].mode) &&
   {
         // tileSplitBytes stored in m_tileTable is only valid for depth entries
   if (type == ADDR_DEPTH_SAMPLE_ORDER)
   {
      if (Min(m_tileTable[index].info.tileSplitBytes,
         {
            }
   else // other entries are determined by other 3 fields
   {
            }
   else if (mode == ADDR_TM_LINEAR_ALIGNED)
   {
      // linear mode only needs tile mode to match
   if (mode == m_tileTable[index].mode)
   {
            }
   else
   {
      // micro tile modes only need tile mode and tile type to match
   if (mode == m_tileTable[index].mode &&
         {
                                    if (index >= static_cast<INT_32>(m_noOfEntries))
   {
                     }
      /**
   ****************************************************************************************************
   *   CiLib::HwlSetupTileCfg
   *
   *   @brief
   *       Map tile index to tile setting.
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE CiLib::HwlSetupTileCfg(
      UINT_32         bpp,            ///< Bits per pixel
   INT_32          index,          ///< Tile index
   INT_32          macroModeIndex, ///< Index in macro tile mode table(CI)
   ADDR_TILEINFO*  pInfo,          ///< [out] Tile Info
   AddrTileMode*   pMode,          ///< [out] Tile mode
   AddrTileType*   pType           ///< [out] Tile type
      {
               // Global flag to control usage of tileIndex
   if (UseTileIndex(index))
   {
      if (index == TileIndexLinearGeneral)
   {
         pInfo->banks = 2;
   pInfo->bankWidth = 1;
   pInfo->bankHeight = 1;
   pInfo->macroAspectRatio = 1;
   pInfo->tileSplitBytes = 64;
   }
   else if (static_cast<UINT_32>(index) >= m_noOfEntries)
   {
         }
   else
   {
                  if (pInfo != NULL)
   {
      if (IsMacroTiled(pCfgTable->mode))
                                             if (pCfgTable->type == ADDR_DEPTH_SAMPLE_ORDER)
   {
         }
   else
   {
         if (bpp > 0)
   {
      UINT_32 thickness = Thickness(pCfgTable->mode);
   UINT_32 tileBytes1x = BITS_TO_BYTES(bpp * MicroTilePixels * thickness);
   // Non-depth entries store a split factor
   UINT_32 sampleSplit = m_tileTable[index].info.tileSplitBytes;
      }
   else
   {
                                          }
   else // 1D and linear modes, we return default value stored in table
   {
                     if (pMode != NULL)
   {
                  if (pType != NULL)
   {
                        }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeSurfaceInfo
   *
   *   @brief
   *       Entry of CI's ComputeSurfaceInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE CiLib::HwlComputeSurfaceInfo(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut    ///< [out] output structure
      {
      // If tileIndex is invalid, force macroModeIndex to be invalid, too
   if (pIn->tileIndex == TileIndexInvalid)
   {
                           if ((pIn->mipLevel > 0) &&
      (pOut->tcCompatible == TRUE) &&
   (pOut->tileMode != pIn->tileMode) &&
      {
                  if (pOut->macroModeIndex == TileIndexNoMacroIndex)
   {
                  if ((pIn->flags.matchStencilTileCfg == TRUE) &&
         {
               if ((MinDepth2DThinIndex <= pOut->tileIndex) &&
         {
                  if ((depthStencil2DTileConfigMatch == FALSE) &&
                           ADDR_COMPUTE_SURFACE_INFO_INPUT localIn = *pIn;
                                                      if ((depthStencil2DTileConfigMatch == FALSE) &&
                           ADDR_COMPUTE_SURFACE_INFO_INPUT localIn = *pIn;
                                    if (pOut->tileIndex == Depth1DThinIndex)
   {
                        }
      /**
   ****************************************************************************************************
   *   CiLib::HwlFmaskSurfaceInfo
   *   @brief
   *       Entry of r800's ComputeFmaskInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE CiLib::HwlComputeFmaskInfo(
      const ADDR_COMPUTE_FMASK_INFO_INPUT*    pIn,   ///< [in] input structure
   ADDR_COMPUTE_FMASK_INFO_OUTPUT*         pOut   ///< [out] output structure
      {
               ADDR_TILEINFO tileInfo = {0};
   ADDR_COMPUTE_FMASK_INFO_INPUT fmaskIn;
                     // Use internal tile info if pOut does not have a valid pTileInfo
   if (pOut->pTileInfo == NULL)
   {
                  ADDR_ASSERT(tileMode == ADDR_TM_2D_TILED_THIN1     ||
               tileMode == ADDR_TM_3D_TILED_THIN1     ||
            ADDR_ASSERT(m_tileTable[14].mode == ADDR_TM_2D_TILED_THIN1);
            // The only valid tile modes for fmask are 2D_THIN1 and 3D_THIN1 plus non-displayable
   INT_32 tileIndex = tileMode == ADDR_TM_2D_TILED_THIN1 ? 14 : 15;
   ADDR_SURFACE_FLAGS flags = {{0}};
                     UINT_32 numSamples = pIn->numSamples;
                     // EQAA needs one more bit
   if (numSamples > numFrags)
   {
                  if (bpp == 3)
   {
                                    fmaskIn.tileIndex = tileIndex;
   fmaskIn.pTileInfo = pOut->pTileInfo;
   pOut->macroModeIndex = macroModeIndex;
                     if (retCode == ADDR_OK)
   {
      pOut->tileIndex =
                     // Resets pTileInfo to NULL if the internal tile info is used
   if (pOut->pTileInfo == &tileInfo)
   {
                     }
      /**
   ****************************************************************************************************
   *   CiLib::HwlFmaskPreThunkSurfInfo
   *
   *   @brief
   *       Some preparation before thunking a ComputeSurfaceInfo call for Fmask
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   VOID CiLib::HwlFmaskPreThunkSurfInfo(
      const ADDR_COMPUTE_FMASK_INFO_INPUT*    pFmaskIn,   ///< [in] Input of fmask info
   const ADDR_COMPUTE_FMASK_INFO_OUTPUT*   pFmaskOut,  ///< [in] Output of fmask info
   ADDR_COMPUTE_SURFACE_INFO_INPUT*        pSurfIn,    ///< [out] Input of thunked surface info
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pSurfOut    ///< [out] Output of thunked surface info
      {
      pSurfIn->tileIndex = pFmaskIn->tileIndex;
      }
      /**
   ****************************************************************************************************
   *   CiLib::HwlFmaskPostThunkSurfInfo
   *
   *   @brief
   *       Copy hwl extra field after calling thunked ComputeSurfaceInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   VOID CiLib::HwlFmaskPostThunkSurfInfo(
      const ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pSurfOut,   ///< [in] Output of surface info
   ADDR_COMPUTE_FMASK_INFO_OUTPUT* pFmaskOut           ///< [out] Output of fmask info
      {
      pFmaskOut->tileIndex = pSurfOut->tileIndex;
      }
      /**
   ****************************************************************************************************
   *   CiLib::HwlDegradeThickTileMode
   *
   *   @brief
   *       Degrades valid tile mode for thick modes if needed
   *
   *   @return
   *       Suitable tile mode
   ****************************************************************************************************
   */
   AddrTileMode CiLib::HwlDegradeThickTileMode(
      AddrTileMode        baseTileMode,   ///< [in] base tile mode
   UINT_32             numSlices,      ///< [in] current number of slices
   UINT_32*            pBytesPerTile   ///< [in,out] pointer to bytes per slice
      {
         }
      /**
   ****************************************************************************************************
   *   CiLib::HwlOptimizeTileMode
   *
   *   @brief
   *       Optimize tile mode on CI
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID CiLib::HwlOptimizeTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT*    pInOut      ///< [in,out] input output structure
      {
               // Override 2D/3D macro tile mode to PRT_* tile mode if
   // client driver requests this surface is equation compatible
   if (IsMacroTiled(tileMode) == TRUE)
   {
      if ((pInOut->flags.needEquation == TRUE) &&
         (pInOut->numSamples <= 1) &&
   {
         if ((pInOut->numSlices > 1) && ((pInOut->maxBaseAlign == 0) || (pInOut->maxBaseAlign >= Block64K)))
                     if (thickness == 1)
   {
         }
   else
   {
      static const UINT_32 PrtTileBytes = 0x10000;
                        HwlComputeMacroModeIndex(PrtThickTileIndex,
                              UINT_32 macroTileBytes = ((pInOut->bpp) >> 3) * 64 * pInOut->numSamples *
                        if (macroTileBytes <= PrtTileBytes)
   {
         }
   else
   {
                        if (pInOut->maxBaseAlign != 0)
   {
                     if (tileMode != pInOut->tileMode)
   {
            }
      /**
   ****************************************************************************************************
   *   CiLib::HwlOverrideTileMode
   *
   *   @brief
   *       Override THICK to THIN, for specific formats on CI
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID CiLib::HwlOverrideTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT*    pInOut      ///< [in,out] input output structure
      {
      AddrTileMode tileMode = pInOut->tileMode;
            // currently, all CI/VI family do not
   // support ADDR_TM_PRT_2D_TILED_THICK,ADDR_TM_PRT_3D_TILED_THICK and
   // ADDR_TM_PRT_2D_TILED_THIN1, ADDR_TM_PRT_3D_TILED_THIN1
   switch (tileMode)
   {
      case ADDR_TM_PRT_2D_TILED_THICK:
   case ADDR_TM_PRT_3D_TILED_THICK:
         tileMode = ADDR_TM_PRT_TILED_THICK;
   case ADDR_TM_PRT_2D_TILED_THIN1:
   case ADDR_TM_PRT_3D_TILED_THIN1:
         tileMode = ADDR_TM_PRT_TILED_THIN1;
   default:
               // UBTS#404321, we do not need such overriding, as THICK+THICK entries removed from the tile-mode table
   if (!m_settings.isBonaire)
   {
               // tile_thickness = (array_mode == XTHICK) ? 8 : ((array_mode == THICK) ? 4 : 1)
   if (thickness > 1)
   {
         switch (pInOut->format)
   {
      // tcpError("Thick micro tiling is not supported for format...
   case ADDR_FMT_X24_8_32_FLOAT:
                        // packed formats
   case ADDR_FMT_GB_GR:
   case ADDR_FMT_BG_RG:
   case ADDR_FMT_1_REVERSED:
   case ADDR_FMT_1:
   case ADDR_FMT_BC1:
   case ADDR_FMT_BC2:
   case ADDR_FMT_BC3:
   case ADDR_FMT_BC4:
   case ADDR_FMT_BC5:
   case ADDR_FMT_BC6:
   case ADDR_FMT_BC7:
      switch (tileMode)
   {
                                                                                                                                                                     // Switch tile type from thick to thin
   if (tileMode != pInOut->tileMode)
   {
                           default:
                  if (tileMode != pInOut->tileMode)
   {
      pInOut->tileMode = tileMode;
         }
      /**
   ****************************************************************************************************
   *   CiLib::HwlSelectTileMode
   *
   *   @brief
   *       Select tile modes.
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID CiLib::HwlSelectTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT* pInOut     ///< [in,out] input output structure
      {
      AddrTileMode tileMode;
            if (pInOut->flags.rotateDisplay)
   {
      tileMode = ADDR_TM_2D_TILED_THIN1;
      }
   else if (pInOut->flags.volume)
   {
      BOOL_32 bThin = (m_settings.isBonaire == TRUE) ||
            if (pInOut->numSlices >= 8)
   {
         tileMode = ADDR_TM_2D_TILED_XTHICK;
   }
   else if (pInOut->numSlices >= 4)
   {
         tileMode = ADDR_TM_2D_TILED_THICK;
   }
   else
   {
         tileMode = ADDR_TM_2D_TILED_THIN1;
      }
   else
   {
               if (pInOut->flags.depth || pInOut->flags.stencil)
   {
         }
   else if ((pInOut->bpp <= 32) ||
               {
         }
   else
   {
                     if (pInOut->flags.prt)
   {
      if (Thickness(tileMode) > 1)
   {
         tileMode = ADDR_TM_PRT_TILED_THICK;
   }
   else
   {
                     pInOut->tileMode = tileMode;
            if ((pInOut->flags.dccCompatible == FALSE) &&
         {
      pInOut->flags.opt4Space = TRUE;
               // Optimize tile mode if possible
               }
      /**
   ****************************************************************************************************
   *   CiLib::HwlSetPrtTileMode
   *
   *   @brief
   *       Set PRT tile mode.
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID CiLib::HwlSetPrtTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT* pInOut     ///< [in,out] input output structure
      {
      AddrTileMode tileMode = pInOut->tileMode;
            if (Thickness(tileMode) > 1)
   {
      tileMode = ADDR_TM_PRT_TILED_THICK;
      }
   else
   {
      tileMode = ADDR_TM_PRT_TILED_THIN1;
               pInOut->tileMode = tileMode;
      }
      /**
   ****************************************************************************************************
   *   CiLib::HwlSetupTileInfo
   *
   *   @brief
   *       Setup default value of tile info for SI
   ****************************************************************************************************
   */
   VOID CiLib::HwlSetupTileInfo(
      AddrTileMode                        tileMode,       ///< [in] Tile mode
   ADDR_SURFACE_FLAGS                  flags,          ///< [in] Surface type flags
   UINT_32                             bpp,            ///< [in] Bits per pixel
   UINT_32                             pitch,          ///< [in] Pitch in pixels
   UINT_32                             height,         ///< [in] Height in pixels
   UINT_32                             numSamples,     ///< [in] Number of samples
   ADDR_TILEINFO*                      pTileInfoIn,    ///< [in] Tile info input: NULL for default
   ADDR_TILEINFO*                      pTileInfoOut,   ///< [out] Tile info output
   AddrTileType                        inTileType,     ///< [in] Tile type
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*   pOut            ///< [out] Output
      {
      UINT_32 thickness = Thickness(tileMode);
   ADDR_TILEINFO* pTileInfo = pTileInfoOut;
   INT index = TileIndexInvalid;
            // Fail-safe code
   if (IsLinear(tileMode) == FALSE)
   {
      // Thick tile modes must use thick micro tile mode but Bonaire does not support due to
   // old derived netlists (UBTS 404321)
   if (thickness > 1)
   {
         if (m_settings.isBonaire)
   {
         }
   else if ((m_allowNonDispThickModes == FALSE) ||
            (inTileType != ADDR_NON_DISPLAYABLE) ||
      {
         }
   // 128 bpp tiling must be non-displayable.
   // Fmask reuse color buffer's entry but bank-height field can be from another entry
   // To simplify the logic, fmask entry should be picked from non-displayable ones
   else if (bpp == 128 || flags.fmask)
   {
         }
   // These two modes only have non-disp entries though they can be other micro tile modes
   else if (tileMode == ADDR_TM_3D_TILED_THIN1 || tileMode == ADDR_TM_PRT_3D_TILED_THIN1)
   {
                  if (flags.depth || flags.stencil)
   {
                     // tcCompatible flag is only meaningful for gfx8.
   if (SupportDccAndTcCompatibility() == FALSE)
   {
                  if (IsTileInfoAllZero(pTileInfo))
   {
      // See table entries 0-4
   if (flags.depth || flags.stencil)
   {
                        // Turn off tc compatible if row_size is smaller than tile size (tile split occurs).
   if (m_rowSize < tileSize)
   {
                  if (flags.nonSplit | flags.tcCompatible | flags.needEquation)
   {
      // Texture readable depth surface should not be split
   switch (tileSize)
   {
      case 64:
         index = 0;
   case 128:
         index = 1;
   case 256:
         index = 2;
   case 512:
         index = 3;
   default:
               }
   else
   {
      // Depth and stencil need to use the same index, thus the pre-defined tile_split
   // can meet the requirement to choose the same macro mode index
   // uncompressed depth/stencil are not supported for now
   switch (numSamples)
   {
      case 1:
         index = 0;
   case 2:
   case 4:
         index = 1;
   case 8:
         index = 2;
   default:
                  // See table entries 5-6
   if (inTileType == ADDR_DEPTH_SAMPLE_ORDER)
   {
         switch (tileMode)
   {
      case ADDR_TM_1D_TILED_THIN1:
      index = 5;
      case ADDR_TM_PRT_TILED_THIN1:
      index = 6;
      default:
               // See table entries 8-12
   if (inTileType == ADDR_DISPLAYABLE)
   {
         switch (tileMode)
   {
      case ADDR_TM_1D_TILED_THIN1:
      index = 9;
      case ADDR_TM_2D_TILED_THIN1:
      index = 10;
      case ADDR_TM_PRT_TILED_THIN1:
      index = 11;
      default:
               // See table entries 13-18
   if (inTileType == ADDR_NON_DISPLAYABLE)
   {
         switch (tileMode)
   {
      case ADDR_TM_1D_TILED_THIN1:
      index = 13;
      case ADDR_TM_2D_TILED_THIN1:
      index = 14;
      case ADDR_TM_3D_TILED_THIN1:
      index = 15;
      case ADDR_TM_PRT_TILED_THIN1:
      index = 16;
      default:
               // See table entries 19-26
   if (thickness > 1)
   {
         switch (tileMode)
   {
      case ADDR_TM_1D_TILED_THICK:
      // special check for bonaire, for the compatablity between old KMD and new UMD
   index = ((inTileType == ADDR_THICK) || m_settings.isBonaire) ? 19 : 18;
      case ADDR_TM_2D_TILED_THICK:
      // special check for bonaire, for the compatablity between old KMD and new UMD
   index = ((inTileType == ADDR_THICK) || m_settings.isBonaire) ? 20 : 24;
      case ADDR_TM_3D_TILED_THICK:
      index = 21;
      case ADDR_TM_PRT_TILED_THICK:
      index = 22;
      case ADDR_TM_2D_TILED_XTHICK:
      index = 25;
      case ADDR_TM_3D_TILED_XTHICK:
      index = 26;
      default:
               // See table entries 27-30
   if (inTileType == ADDR_ROTATED)
   {
         switch (tileMode)
   {
      case ADDR_TM_1D_TILED_THIN1:
      index = 27;
      case ADDR_TM_2D_TILED_THIN1:
      index = 28;
      case ADDR_TM_PRT_TILED_THIN1:
      index = 29;
      case ADDR_TM_PRT_2D_TILED_THIN1:
      index = 30;
      default:
               if (m_pipes >= 8)
   {
         ADDR_ASSERT((index + 1) < static_cast<INT_32>(m_noOfEntries));
   // Only do this when tile mode table is updated.
   if (((tileMode == ADDR_TM_PRT_TILED_THIN1) || (tileMode == ADDR_TM_PRT_TILED_THICK)) &&
         {
                                                   if (macroTileBytes != PrtTileBytes)
                                                                  flags.tcCompatible = FALSE;
            }
   else
   {
      // A pre-filled tile info is ready
   index = pOut->tileIndex;
            // pass tile type back for post tile index compute
            if (flags.depth || flags.stencil)
   {
                        // Turn off tc compatible if row_size is smaller than tile size (tile split occurs).
   if (m_rowSize < tileSize)
   {
                           if (m_pipes != numPipes)
   {
                     // We only need to set up tile info if there is a valid index but macroModeIndex is invalid
   if ((index != TileIndexInvalid) && (macroModeIndex == TileIndexInvalid))
   {
               // Copy to pOut->tileType/tileIndex/macroModeIndex
   pOut->tileIndex = index;
   pOut->tileType = m_tileTable[index].type; // Or inTileType, the samea
      }
   else if (tileMode == ADDR_TM_LINEAR_GENERAL)
   {
               // Copy linear-aligned entry??
      }
   else if (tileMode == ADDR_TM_LINEAR_ALIGNED)
   {
      pOut->tileIndex = 8;
               if (flags.tcCompatible)
   {
                     }
      /**
   ****************************************************************************************************
   *   CiLib::ReadGbTileMode
   *
   *   @brief
   *       Convert GB_TILE_MODE HW value to ADDR_TILE_CONFIG.
   ****************************************************************************************************
   */
   VOID CiLib::ReadGbTileMode(
      UINT_32       regValue,   ///< [in] GB_TILE_MODE register
   TileConfig*   pCfg        ///< [out] output structure
      {
      GB_TILE_MODE gbTileMode;
            pCfg->type = static_cast<AddrTileType>(gbTileMode.f.micro_tile_mode_new);
   if (AltTilingEnabled() == TRUE)
   {
         }
   else
   {
                  if (pCfg->type == ADDR_DEPTH_SAMPLE_ORDER)
   {
         }
   else
   {
                                    switch (regArrayMode)
   {
      case 5:
         pCfg->mode = ADDR_TM_PRT_TILED_THIN1;
   case 6:
         pCfg->mode = ADDR_TM_PRT_2D_TILED_THIN1;
   case 8:
         pCfg->mode = ADDR_TM_2D_TILED_XTHICK;
   case 9:
         pCfg->mode = ADDR_TM_PRT_TILED_THICK;
   case 0xa:
         pCfg->mode = ADDR_TM_PRT_2D_TILED_THICK;
   case 0xb:
         pCfg->mode = ADDR_TM_PRT_3D_TILED_THIN1;
   case 0xe:
         pCfg->mode = ADDR_TM_3D_TILED_XTHICK;
   case 0xf:
         pCfg->mode = ADDR_TM_PRT_3D_TILED_THICK;
   default:
               // Fail-safe code for these always convert tile info, as the non-macro modes
   // return the entry of tile mode table directly without looking up macro mode table
   if (!IsMacroTiled(pCfg->mode))
   {
      pCfg->info.banks = 2;
   pCfg->info.bankWidth = 1;
   pCfg->info.bankHeight = 1;
   pCfg->info.macroAspectRatio = 1;
         }
      /**
   ****************************************************************************************************
   *   CiLib::InitTileSettingTable
   *
   *   @brief
   *       Initialize the ADDR_TILE_CONFIG table.
   *   @return
   *       TRUE if tile table is correctly initialized
   ****************************************************************************************************
   */
   BOOL_32 CiLib::InitTileSettingTable(
      const UINT_32*  pCfg,           ///< [in] Pointer to table of tile configs
   UINT_32         noOfEntries     ///< [in] Numbe of entries in the table above
      {
                                 if (noOfEntries != 0)
   {
         }
   else
   {
                  if (pCfg) // From Client
   {
      for (UINT_32 i = 0; i < m_noOfEntries; i++)
   {
            }
   else
   {
      ADDR_ASSERT_ALWAYS();
               if (initOk)
   {
               if (m_settings.isBonaire == FALSE)
   {
         // Check if entry 18 is "thick+thin" combination
   if ((m_tileTable[18].mode == ADDR_TM_1D_TILED_THICK) &&
         {
      m_allowNonDispThickModes = TRUE;
      }
   else
   {
                  // Assume the first entry is always programmed with full pipes
                  }
      /**
   ****************************************************************************************************
   *   CiLib::ReadGbMacroTileCfg
   *
   *   @brief
   *       Convert GB_MACRO_TILE_CFG HW value to ADDR_TILE_CONFIG.
   ****************************************************************************************************
   */
   VOID CiLib::ReadGbMacroTileCfg(
      UINT_32             regValue,   ///< [in] GB_MACRO_TILE_MODE register
   ADDR_TILEINFO*      pCfg        ///< [out] output structure
      {
      GB_MACROTILE_MODE gbTileMode;
            if (AltTilingEnabled() == TRUE)
   {
      pCfg->bankHeight       = 1 << gbTileMode.f.alt_bank_height;
   pCfg->banks            = 1 << (gbTileMode.f.alt_num_banks + 1);
      }
   else
   {
      pCfg->bankHeight       = 1 << gbTileMode.f.bank_height;
   pCfg->banks            = 1 << (gbTileMode.f.num_banks + 1);
      }
      }
      /**
   ****************************************************************************************************
   *   CiLib::InitMacroTileCfgTable
   *
   *   @brief
   *       Initialize the ADDR_MACRO_TILE_CONFIG table.
   *   @return
   *       TRUE if macro tile table is correctly initialized
   ****************************************************************************************************
   */
   BOOL_32 CiLib::InitMacroTileCfgTable(
      const UINT_32*  pCfg,           ///< [in] Pointer to table of tile configs
   UINT_32         noOfMacroEntries     ///< [in] Numbe of entries in the table above
      {
                                 if (noOfMacroEntries != 0)
   {
         }
   else
   {
                  if (pCfg) // From Client
   {
      for (UINT_32 i = 0; i < m_noOfMacroEntries; i++)
   {
                     }
   else
   {
      ADDR_ASSERT_ALWAYS();
      }
      }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeMacroModeIndex
   *
   *   @brief
   *       Computes macro tile mode index
   *   @return
   *       TRUE if macro tile table is correctly initialized
   ****************************************************************************************************
   */
   INT_32 CiLib::HwlComputeMacroModeIndex(
      INT_32              tileIndex,      ///< [in] Tile mode index
   ADDR_SURFACE_FLAGS  flags,          ///< [in] Surface flags
   UINT_32             bpp,            ///< [in] Bit per pixel
   UINT_32             numSamples,     ///< [in] Number of samples
   ADDR_TILEINFO*      pTileInfo,      ///< [out] Pointer to ADDR_TILEINFO
   AddrTileMode*       pTileMode,      ///< [out] Pointer to AddrTileMode
   AddrTileType*       pTileType       ///< [out] Pointer to AddrTileType
      {
               AddrTileMode tileMode = m_tileTable[tileIndex].mode;
   AddrTileType tileType = m_tileTable[tileIndex].type;
            if (!IsMacroTiled(tileMode))
   {
      *pTileInfo = m_tileTable[tileIndex].info;
      }
   else
   {
      UINT_32 tileBytes1x = BITS_TO_BYTES(bpp * MicroTilePixels * thickness);
            if (m_tileTable[tileIndex].type == ADDR_DEPTH_SAMPLE_ORDER)
   {
         // Depth entries store real tileSplitBytes
   }
   else
   {
         // Non-depth entries store a split factor
                           UINT_32 tileSplitC = Min(m_rowSize, tileSplit);
            if (flags.fmask)
   {
         }
   else
   {
                  if (tileBytes < 64)
   {
                           if (flags.prt || IsPrtTileMode(tileMode))
   {
         macroModeIndex += PrtMacroModeOffset;
   }
   else
   {
                                       if (NULL != pTileMode)
   {
                  if (NULL != pTileType)
   {
                     }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeTileDataWidthAndHeightLinear
   *
   *   @brief
   *       Compute the squared cache shape for per-tile data (CMASK and HTILE) for linear layout
   *
   *   @note
   *       MacroWidth and macroHeight are measured in pixels
   ****************************************************************************************************
   */
   VOID CiLib::HwlComputeTileDataWidthAndHeightLinear(
      UINT_32*        pMacroWidth,     ///< [out] macro tile width
   UINT_32*        pMacroHeight,    ///< [out] macro tile height
   UINT_32         bpp,             ///< [in] bits per pixel
   ADDR_TILEINFO*  pTileInfo        ///< [in] tile info
      {
                        switch (pTileInfo->pipeConfig)
   {
      case ADDR_PIPECFG_P16_32x32_8x16:
   case ADDR_PIPECFG_P16_32x32_16x16:
   case ADDR_PIPECFG_P8_32x64_32x32:
   case ADDR_PIPECFG_P8_32x32_16x32:
   case ADDR_PIPECFG_P8_32x32_16x16:
   case ADDR_PIPECFG_P8_32x32_8x16:
   case ADDR_PIPECFG_P4_32x32:
         numTiles = 8;
   default:
                     *pMacroWidth    = numTiles * MicroTileWidth;
      }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeMetadataNibbleAddress
   *
   *   @brief
   *        calculate meta data address based on input information
   *
   *   &parameter
   *        uncompressedDataByteAddress - address of a pixel in color surface
   *        dataBaseByteAddress         - base address of color surface
   *        metadataBaseByteAddress     - base address of meta ram
   *        metadataBitSize             - meta key size, 8 for DCC, 4 for cmask
   *        elementBitSize              - element size of color surface
   *        blockByteSize               - compression block size, 256 for DCC
   *        pipeInterleaveBytes         - pipe interleave size
   *        numOfPipes                  - number of pipes
   *        numOfBanks                  - number of banks
   *        numOfSamplesPerSplit        - number of samples per tile split
   *   @return
   *        meta data nibble address (nibble address is used to support DCC compatible cmask)
   *
   ****************************************************************************************************
   */
   UINT_64 CiLib::HwlComputeMetadataNibbleAddress(
      UINT_64 uncompressedDataByteAddress,
   UINT_64 dataBaseByteAddress,
   UINT_64 metadataBaseByteAddress,
   UINT_32 metadataBitSize,
   UINT_32 elementBitSize,
   UINT_32 blockByteSize,
   UINT_32 pipeInterleaveBytes,
   UINT_32 numOfPipes,
   UINT_32 numOfBanks,
      {
      ///--------------------------------------------------------------------------------------------
   /// Get pipe interleave, bank and pipe bits
   ///--------------------------------------------------------------------------------------------
   UINT_32 pipeInterleaveBits  = Log2(pipeInterleaveBytes);
   UINT_32 pipeBits            = Log2(numOfPipes);
            ///--------------------------------------------------------------------------------------------
   /// Clear pipe and bank swizzles
   ///--------------------------------------------------------------------------------------------
   UINT_32 dataMacrotileBits        = pipeInterleaveBits + pipeBits + bankBits;
            UINT_64 dataMacrotileClearMask     = ~((1L << dataMacrotileBits) - 1);
            UINT_64 dataBaseByteAddressNoSwizzle = dataBaseByteAddress & dataMacrotileClearMask;
            ///--------------------------------------------------------------------------------------------
   /// Modify metadata base before adding in so that when final address is divided by data ratio,
   /// the base address returns to where it should be
   ///--------------------------------------------------------------------------------------------
   ADDR_ASSERT((0 != metadataBitSize));
   UINT_64 metadataBaseShifted = metadataBaseByteAddressNoSwizzle * blockByteSize * 8 /
         UINT_64 offset = uncompressedDataByteAddress -
                  ///--------------------------------------------------------------------------------------------
   /// Save bank data bits
   ///--------------------------------------------------------------------------------------------
   UINT_32 lsb = pipeBits + pipeInterleaveBits;
                     ///--------------------------------------------------------------------------------------------
   /// Save pipe data bits
   ///--------------------------------------------------------------------------------------------
   lsb = pipeInterleaveBits;
                     ///--------------------------------------------------------------------------------------------
   /// Remove pipe and bank bits
   ///--------------------------------------------------------------------------------------------
   lsb = pipeInterleaveBits;
                     ADDR_ASSERT((0 != blockByteSize));
            UINT_32 tileSize = 8 * 8 * elementBitSize/8 * numOfSamplesPerSplit;
            if (0 == blocksInTile)
   {
         }
   else
   {
         }
                     /// NOTE *2 because we are converting to Nibble address in this step
               ///--------------------------------------------------------------------------------------------
   /// Reinsert pipe bits back into the final address
   ///--------------------------------------------------------------------------------------------
   lsb = pipeInterleaveBits + 1; ///<+1 due to Nibble address now gives interleave bits extra lsb.
   msb = pipeBits - 1 + lsb;
               }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeSurfaceAlignmentsMacroTiled
   *
   *   @brief
   *       Hardware layer function to compute alignment request for macro tile mode
   *
   ****************************************************************************************************
   */
   VOID CiLib::HwlComputeSurfaceAlignmentsMacroTiled(
      AddrTileMode                      tileMode,           ///< [in] tile mode
   UINT_32                           bpp,                ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS                flags,              ///< [in] surface flags
   UINT_32                           mipLevel,           ///< [in] mip level
   UINT_32                           numSamples,         ///< [in] number of samples
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pOut                ///< [in,out] Surface output
      {
      // This is to workaround a H/W limitation that DCC doesn't work when pipe config is switched to
   // P4. In theory, all asics that have such switching should be patched but we now only know what
   // to pad for Fiji.
   if ((m_settings.isFiji == TRUE) &&
      (flags.dccPipeWorkaround == TRUE) &&
   (flags.prt == FALSE) &&
   (mipLevel == 0) &&
   (tileMode == ADDR_TM_PRT_TILED_THIN1) &&
      {
      pOut->pitchAlign   = PowTwoAlign(pOut->pitchAlign, 256);
   // In case the client still requests DCC usage.
         }
      /**
   ****************************************************************************************************
   *   CiLib::HwlPadDimensions
   *
   *   @brief
   *       Helper function to pad dimensions
   *
   ****************************************************************************************************
   */
   VOID CiLib::HwlPadDimensions(
      AddrTileMode        tileMode,    ///< [in] tile mode
   UINT_32             bpp,         ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,       ///< [in] surface flags
   UINT_32             numSamples,  ///< [in] number of samples
   ADDR_TILEINFO*      pTileInfo,   ///< [in] tile info
   UINT_32             mipLevel,    ///< [in] mip level
   UINT_32*            pPitch,      ///< [in,out] pitch in pixels
   UINT_32*            pPitchAlign, ///< [in,out] pitch alignment
   UINT_32             height,      ///< [in] height in pixels
   UINT_32             heightAlign  ///< [in] height alignment
      {
      if ((SupportDccAndTcCompatibility() == TRUE) &&
      (flags.dccCompatible == TRUE) &&
   (numSamples > 1) &&
   (mipLevel == 0) &&
      {
      UINT_32 tileSizePerSample = BITS_TO_BYTES(bpp * MicroTileWidth * MicroTileHeight);
            if (samplesPerSplit < numSamples)
   {
                                 if (0 != (bytesPerSplit & (dccFastClearByteAlign - 1)))
   {
      UINT_32 dccFastClearPixelAlign = dccFastClearByteAlign /
                        if ((dccFastClearPixelAlign >= macroTilePixelAlign) &&
         {
                           while ((heightInMacroTile > 1) &&
            ((heightInMacroTile % 2) == 0) &&
      {
                                       if (IsPow2(dccFastClearPitchAlignInPixels))
   {
         }
   else
   {
                                          }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeMaxBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments
   *   @return
   *       maximum alignments
   ****************************************************************************************************
   */
   UINT_32 CiLib::HwlComputeMaxBaseAlignments() const
   {
               // Initial size is 64 KiB for PRT.
            for (UINT_32 i = 0; i < m_noOfMacroEntries; i++)
   {
      // The maximum tile size is 16 byte-per-pixel and either 8-sample or 8-slice.
            UINT_32 baseAlign = tileSize * pipes * m_macroTileTable[i].banks *
            if (baseAlign > maxBaseAlign)
   {
                        }
      /**
   ****************************************************************************************************
   *   CiLib::HwlComputeMaxMetaBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments for metadata
   *   @return
   *       maximum alignments for metadata
   ****************************************************************************************************
   */
   UINT_32 CiLib::HwlComputeMaxMetaBaseAlignments() const
   {
               for (UINT_32 i = 0; i < m_noOfMacroEntries; i++)
   {
      if (SupportDccAndTcCompatibility() && IsMacroTiled(m_tileTable[i].mode))
   {
                        }
      /**
   ****************************************************************************************************
   *   CiLib::DepthStencilTileCfgMatch
   *
   *   @brief
   *       Try to find a tile index for stencil which makes its tile config parameters matches to depth
   *   @return
   *       TRUE if such tile index for stencil can be found
   ****************************************************************************************************
   */
   BOOL_32 CiLib::DepthStencilTileCfgMatch(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut    ///< [out] output structure
      {
               for (INT_32 stencilTileIndex = MinDepth2DThinIndex;
         stencilTileIndex <= MaxDepth2DThinIndex;
   {
      ADDR_TILEINFO tileInfo = {0};
   INT_32 stencilMacroIndex = HwlComputeMacroModeIndex(stencilTileIndex,
                              if (stencilMacroIndex != TileIndexNoMacroIndex)
   {
         if ((m_macroTileTable[stencilMacroIndex].banks ==
      m_macroTileTable[pOut->macroModeIndex].banks) &&
   (m_macroTileTable[stencilMacroIndex].bankWidth ==
   m_macroTileTable[pOut->macroModeIndex].bankWidth) &&
   (m_macroTileTable[stencilMacroIndex].bankHeight ==
   m_macroTileTable[pOut->macroModeIndex].bankHeight) &&
   (m_macroTileTable[stencilMacroIndex].macroAspectRatio ==
   m_macroTileTable[pOut->macroModeIndex].macroAspectRatio) &&
   (m_macroTileTable[stencilMacroIndex].pipeConfig ==
      {
      if ((pOut->tcCompatible == FALSE) ||
         {
      depthStencil2DTileConfigMatch = TRUE;
   pOut->stencilTileIdx = stencilTileIndex;
         }
   else
   {
                        }
      /**
   ****************************************************************************************************
   *   CiLib::DepthStencilTileCfgMatch
   *
   *   @brief
   *       Check if tc compatibility is available
   *   @return
   *       If tc compatibility is not available
   ****************************************************************************************************
   */
   BOOL_32 CiLib::CheckTcCompatibility(
      const ADDR_TILEINFO*                    pTileInfo,    ///< [in] input tile info
   UINT_32                                 bpp,          ///< [in] Bits per pixel
   AddrTileMode                            tileMode,     ///< [in] input tile mode
   AddrTileType                            tileType,     ///< [in] input tile type
   const ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pOut          ///< [in] output surf info
      {
               if (IsMacroTiled(tileMode))
   {
      if (tileType != ADDR_DEPTH_SAMPLE_ORDER)
   {
         // Turn off tcCompatible for color surface if tileSplit happens. Depth/stencil
                  if ((tileIndex == TileIndexInvalid) && (IsTileInfoAllZero(pTileInfo) == FALSE))
   {
                  if (tileIndex != TileIndexInvalid)
                     ADDR_ASSERT(static_cast<UINT_32>(tileIndex) < TileTableSize);
   // Non-depth entries store a split factor
                        if (m_rowSize < colorTileSplit)
   {
               }
   else
   {
      // Client should not enable tc compatible for linear and 1D tile modes.
                  }
      } // V1
   } // Addr
