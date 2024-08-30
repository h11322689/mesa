   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ****************************************************************************************************
   * @file  addr1lib.cpp
   * @brief Contains the implementation for the Addr::V1::Lib base class.
   ****************************************************************************************************
   */
      #include "addrinterface.h"
   #include "addrlib1.h"
   #include "addrcommon.h"
      namespace Addr
   {
   namespace V1
   {
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Static Const Member
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      const TileModeFlags Lib::ModeFlags[ADDR_TM_COUNT] =
   {// T   L  1  2  3  P  Pr B
      {1, 1, 0, 0, 0, 0, 0, 0}, // ADDR_TM_LINEAR_GENERAL
   {1, 1, 0, 0, 0, 0, 0, 0}, // ADDR_TM_LINEAR_ALIGNED
   {1, 0, 1, 0, 0, 0, 0, 0}, // ADDR_TM_1D_TILED_THIN1
   {4, 0, 1, 0, 0, 0, 0, 0}, // ADDR_TM_1D_TILED_THICK
   {1, 0, 0, 1, 0, 0, 0, 0}, // ADDR_TM_2D_TILED_THIN1
   {1, 0, 0, 1, 0, 0, 0, 0}, // ADDR_TM_2D_TILED_THIN2
   {1, 0, 0, 1, 0, 0, 0, 0}, // ADDR_TM_2D_TILED_THIN4
   {4, 0, 0, 1, 0, 0, 0, 0}, // ADDR_TM_2D_TILED_THICK
   {1, 0, 0, 1, 0, 0, 0, 1}, // ADDR_TM_2B_TILED_THIN1
   {1, 0, 0, 1, 0, 0, 0, 1}, // ADDR_TM_2B_TILED_THIN2
   {1, 0, 0, 1, 0, 0, 0, 1}, // ADDR_TM_2B_TILED_THIN4
   {4, 0, 0, 1, 0, 0, 0, 1}, // ADDR_TM_2B_TILED_THICK
   {1, 0, 0, 1, 1, 0, 0, 0}, // ADDR_TM_3D_TILED_THIN1
   {4, 0, 0, 1, 1, 0, 0, 0}, // ADDR_TM_3D_TILED_THICK
   {1, 0, 0, 1, 1, 0, 0, 1}, // ADDR_TM_3B_TILED_THIN1
   {4, 0, 0, 1, 1, 0, 0, 1}, // ADDR_TM_3B_TILED_THICK
   {8, 0, 0, 1, 0, 0, 0, 0}, // ADDR_TM_2D_TILED_XTHICK
   {8, 0, 0, 1, 1, 0, 0, 0}, // ADDR_TM_3D_TILED_XTHICK
   {1, 0, 0, 0, 0, 0, 0, 0}, // ADDR_TM_POWER_SAVE
   {1, 0, 0, 1, 0, 1, 1, 0}, // ADDR_TM_PRT_TILED_THIN1
   {1, 0, 0, 1, 0, 1, 0, 0}, // ADDR_TM_PRT_2D_TILED_THIN1
   {1, 0, 0, 1, 1, 1, 0, 0}, // ADDR_TM_PRT_3D_TILED_THIN1
   {4, 0, 0, 1, 0, 1, 1, 0}, // ADDR_TM_PRT_TILED_THICK
   {4, 0, 0, 1, 0, 1, 0, 0}, // ADDR_TM_PRT_2D_TILED_THICK
   {4, 0, 0, 1, 1, 1, 0, 0}, // ADDR_TM_PRT_3D_TILED_THICK
      };
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Constructor/Destructor
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Lib::AddrLib1
   *
   *   @brief
   *       Constructor for the AddrLib1 class
   *
   ****************************************************************************************************
   */
   Lib::Lib()
      :
      {
   }
      /**
   ****************************************************************************************************
   *   Lib::Lib
   *
   *   @brief
   *       Constructor for the Addr::V1::Lib class with hClient as parameter
   *
   ****************************************************************************************************
   */
   Lib::Lib(const Client* pClient)
      :
      {
   }
      /**
   ****************************************************************************************************
   *   Lib::~AddrLib1
   *
   *   @brief
   *       Destructor for the AddrLib1 class
   *
   ****************************************************************************************************
   */
   Lib::~Lib()
   {
   }
      /**
   ****************************************************************************************************
   *   Lib::GetLib
   *
   *   @brief
   *       Get AddrLib1 pointer
   *
   *   @return
   *      An Addr::V1::Lib class pointer
   ****************************************************************************************************
   */
   Lib* Lib::GetLib(
         {
      Addr::Lib* pAddrLib = Addr::Lib::GetLib(hLib);
   if ((pAddrLib != NULL) &&
      ((pAddrLib->GetChipFamily() == ADDR_CHIP_FAMILY_IVLD) ||
      {
      // only valid and pre-VI ASIC can use AddrLib1 function.
   ADDR_ASSERT_ALWAYS();
      }
      }
         ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Surface Methods
   ////////////////////////////////////////////////////////////////////////////////////////////////////
         /**
   ****************************************************************************************************
   *   Lib::ComputeSurfaceInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeSurfaceInfo.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceInfo(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT)) ||
         {
                     // We suggest client do sanity check but a check here is also good
   if (pIn->bpp > 128)
   {
                  if ((pIn->tileMode == ADDR_TM_UNKNOWN) && (pIn->mipLevel > 0))
   {
                  // Thick modes don't support multisample
   if ((Thickness(pIn->tileMode) > 1) && (pIn->numSamples > 1))
   {
                  if (returnCode == ADDR_OK)
   {
      // Get a local copy of input structure and only reference pIn for unadjusted values
   ADDR_COMPUTE_SURFACE_INFO_INPUT localIn = *pIn;
            if (UseTileInfo())
   {
         // If the original input has a valid ADDR_TILEINFO pointer then copy its contents.
   // Otherwise the default 0's in tileInfoNull are used.
   if (pIn->pTileInfo)
   {
         }
                     // Do mipmap check first
   // If format is BCn, pre-pad dimension to power-of-two according to HWL
            if (m_configFlags.checkLast2DLevel)
   {
         // Save this level's original height in pixels
            UINT_32 expandX = 1;
   UINT_32 expandY = 1;
            // Save outputs that may not go through HWL
   pOut->pixelBits = localIn.bpp;
   pOut->numSamples = localIn.numSamples;
   pOut->last2DLevel = FALSE;
      #if !ALT_TEST
         if (localIn.numSamples > 1)
   {
         #endif
            if (localIn.format != ADDR_FMT_INVALID) // Set format to INVALID will skip this conversion
   {
         // Get compression/expansion factors and element mode
   // (which indicates compression/expansion
   localIn.bpp = GetElemLib()->GetBitsPerPixel(localIn.format,
                        // Special flag for 96 bit surface. 96 (or 48 if we support) bit surface's width is
   // pre-multiplied by 3 and bpp is divided by 3. So pitch alignment for linear-
   // aligned does not meet 64-pixel in real. We keep special handling in hwl since hw
   // restrictions are different.
                  if ((elemMode == ADDR_EXPANDED) && (expandX > 1))
   {
                  GetElemLib()->AdjustSurfaceInfo(elemMode,
                                          }
   else if (localIn.bpp != 0)
   {
         localIn.width  = (localIn.width != 0) ? localIn.width : 1;
   }
   else // Rule out some invalid parameters
   {
                           // Check mipmap after surface expansion
   if (returnCode == ADDR_OK)
   {
                  if (returnCode == ADDR_OK)
   {
         if (UseTileIndex(localIn.tileIndex))
   {
                                       if (localIn.tileIndex != TileIndexLinearGeneral)
   {
      // Try finding a macroModeIndex
   macroModeIndex = HwlComputeMacroModeIndex(localIn.tileIndex,
                                             // If macroModeIndex is not needed, then call HwlSetupTileCfg to get tile info
   if (macroModeIndex == TileIndexNoMacroIndex)
   {
      returnCode = HwlSetupTileCfg(localIn.bpp,
                  }
   // If macroModeIndex is invalid, then assert this is not macro tiled
   else if (macroModeIndex == TileIndexInvalid)
                                    if (returnCode == ADDR_OK)
   {
                  if (localIn.tileMode == ADDR_TM_UNKNOWN)
   {
      // HWL layer may override tile mode if necessary
      }
   else
   {
                     // Optimize tile mode if possible
               // Call main function to compute surface info
   if (returnCode == ADDR_OK)
   {
                  if (returnCode == ADDR_OK)
   {
                        // Also original width/height/bpp
      #if DEBUG
               if (localIn.flags.display)
   {
   #endif //DEBUG
                  if (localIn.format != ADDR_FMT_INVALID)
   {
      //
   // Note: For 96 bit surface, the pixelPitch returned might be an odd number, but it
   // is okay to program texture pitch as HW's mip calculator would multiply 3 first,
   // then do the appropriate paddings (linear alignment requirement and possible the
   // nearest power-of-two for mipmaps), which results in the original pitch.
   //
   GetElemLib()->RestoreSurfaceInfo(elemMode,
                                       if (localIn.flags.qbStereo)
   {
      if (pOut->pStereoInfo)
   {
                     if (localIn.flags.volume) // For volume sliceSize equals to all z-slices
   {
         }
   else // For array: sliceSize is likely to have slice-padding (the last one)
                     // array or cubemap
   if (pIn->numSlices > 1)
   {
      // If this is the last slice then add the padding size to this slice
   if (pIn->slice == (pIn->numSlices - 1))
   {
         }
   else if (m_configFlags.checkLast2DLevel)
   {
         // Reset last2DLevel flag if this is not the last array slice
                  pOut->pitchTileMax = pOut->pitch / 8 - 1;
   pOut->heightTileMax = pOut->height / 8 - 1;
                           }
      /**
   ****************************************************************************************************
   *   Lib::ComputeSurfaceInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeSurfaceInfo.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceAddrFromCoord(
      const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                                 // Try finding a macroModeIndex
   INT_32 macroModeIndex = HwlComputeMacroModeIndex(input.tileIndex,
                                          // If macroModeIndex is not needed, then call HwlSetupTileCfg to get tile info
   if (macroModeIndex == TileIndexNoMacroIndex)
   {
      returnCode = HwlSetupTileCfg(input.bpp, input.tileIndex, macroModeIndex,
      }
   // If macroModeIndex is invalid, then assert this is not macro tiled
   else if (macroModeIndex == TileIndexInvalid)
   {
                  // Change the input structure
            if (returnCode == ADDR_OK)
   {
                  if (returnCode == ADDR_OK)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeSurfaceCoordFromAddr
   *
   *   @brief
   *       Interface function stub of ComputeSurfaceCoordFromAddr.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceCoordFromAddr(
      const ADDR_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_SURFACE_COORDFROMADDR_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                                 // Try finding a macroModeIndex
   INT_32 macroModeIndex = HwlComputeMacroModeIndex(input.tileIndex,
                                          // If macroModeIndex is not needed, then call HwlSetupTileCfg to get tile info
   if (macroModeIndex == TileIndexNoMacroIndex)
   {
      returnCode = HwlSetupTileCfg(input.bpp, input.tileIndex, macroModeIndex,
      }
   // If macroModeIndex is invalid, then assert this is not macro tiled
   else if (macroModeIndex == TileIndexInvalid)
   {
                  // Change the input structure
            if (returnCode == ADDR_OK)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeSliceTileSwizzle
   *
   *   @brief
   *       Interface function stub of ComputeSliceTileSwizzle.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSliceTileSwizzle(
      const ADDR_COMPUTE_SLICESWIZZLE_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SLICESWIZZLE_OUTPUT*       pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_SLICESWIZZLE_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                  returnCode = HwlSetupTileCfg(0, input.tileIndex, input.macroModeIndex,
         // Change the input structure
            if (returnCode == ADDR_OK)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ExtractBankPipeSwizzle
   *
   *   @brief
   *       Interface function stub of AddrExtractBankPipeSwizzle.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ExtractBankPipeSwizzle(
      const ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT*  pIn,    ///< [in] input structure
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT*       pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                  returnCode = HwlSetupTileCfg(0, input.tileIndex, input.macroModeIndex, input.pTileInfo);
   // Change the input structure
            if (returnCode == ADDR_OK)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::CombineBankPipeSwizzle
   *
   *   @brief
   *       Interface function stub of AddrCombineBankPipeSwizzle.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::CombineBankPipeSwizzle(
      const ADDR_COMBINE_BANKPIPE_SWIZZLE_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMBINE_BANKPIPE_SWIZZLE_OUTPUT*       pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_FMASK_INFO_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                  returnCode = HwlSetupTileCfg(0, input.tileIndex, input.macroModeIndex, input.pTileInfo);
   // Change the input structure
            if (returnCode == ADDR_OK)
   {
         returnCode = HwlCombineBankPipeSwizzle(pIn->bankSwizzle,
                                    }
      /**
   ****************************************************************************************************
   *   Lib::ComputeBaseSwizzle
   *
   *   @brief
   *       Interface function stub of AddrCompueBaseSwizzle.
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeBaseSwizzle(
      const ADDR_COMPUTE_BASE_SWIZZLE_INPUT*  pIn,
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_BASE_SWIZZLE_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                  returnCode = HwlSetupTileCfg(0, input.tileIndex, input.macroModeIndex, input.pTileInfo);
   // Change the input structure
            if (returnCode == ADDR_OK)
   {
         if (IsMacroTiled(pIn->tileMode))
   {
         }
   else
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeFmaskInfo
   *
   *   @brief
   *       Interface function stub of ComputeFmaskInfo.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeFmaskInfo(
      const ADDR_COMPUTE_FMASK_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR_COMPUTE_FMASK_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_FMASK_INFO_INPUT)) ||
         {
                     // No thick MSAA
   if (Thickness(pIn->tileMode) > 1)
   {
                  if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
                  if (pOut->pTileInfo)
   {
      // Use temp tile info for calcalation
      }
   else
   {
                                 // Try finding a macroModeIndex
   INT_32 macroModeIndex = HwlComputeMacroModeIndex(pIn->tileIndex,
                                    // If macroModeIndex is not needed, then call HwlSetupTileCfg to get tile info
   if (macroModeIndex == TileIndexNoMacroIndex)
   {
                                 // Change the input structure
            if (returnCode == ADDR_OK)
   {
         if (pIn->numSamples > 1)
   {
         }
   else
                                                }
      /**
   ****************************************************************************************************
   *   Lib::ComputeFmaskAddrFromCoord
   *
   *   @brief
   *       Interface function stub of ComputeFmaskAddrFromCoord.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeFmaskAddrFromCoord(
      const ADDR_COMPUTE_FMASK_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_FMASK_ADDRFROMCOORD_OUTPUT*        pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_FMASK_ADDRFROMCOORD_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
               if (pIn->numSamples > 1)
   {
         }
   else
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeFmaskCoordFromAddr
   *
   *   @brief
   *       Interface function stub of ComputeFmaskAddrFromCoord.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeFmaskCoordFromAddr(
      const ADDR_COMPUTE_FMASK_COORDFROMADDR_INPUT*  pIn,     ///< [in] input structure
   ADDR_COMPUTE_FMASK_COORDFROMADDR_OUTPUT* pOut           ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_FMASK_COORDFROMADDR_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
               if (pIn->numSamples > 1)
   {
         }
   else
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ConvertTileInfoToHW
   *
   *   @brief
   *       Convert tile info from real value to HW register value in HW layer
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ConvertTileInfoToHW(
      const ADDR_CONVERT_TILEINFOTOHW_INPUT* pIn, ///< [in] input structure
   ADDR_CONVERT_TILEINFOTOHW_OUTPUT* pOut      ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_CONVERT_TILEINFOTOHW_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
   ADDR_CONVERT_TILEINFOTOHW_INPUT input;
   // if pIn->reverse is TRUE, indices are ignored
   if (pIn->reverse == FALSE && UseTileIndex(pIn->tileIndex))
   {
                                                if (returnCode == ADDR_OK)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ConvertTileIndex
   *
   *   @brief
   *       Convert tile index to tile mode/type/info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ConvertTileIndex(
      const ADDR_CONVERT_TILEINDEX_INPUT* pIn, ///< [in] input structure
   ADDR_CONVERT_TILEINDEX_OUTPUT* pOut      ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_CONVERT_TILEINDEX_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
               returnCode = HwlSetupTileCfg(pIn->bpp, pIn->tileIndex, pIn->macroModeIndex,
            if (returnCode == ADDR_OK && pIn->tileInfoHw)
   {
                        hwInput.pTileInfo = pOut->pTileInfo;
                                 }
      /**
   ****************************************************************************************************
   *   Lib::GetMacroModeIndex
   *
   *   @brief
   *       Get macro mode index based on input info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::GetMacroModeIndex(
      const ADDR_GET_MACROMODEINDEX_INPUT* pIn, ///< [in] input structure
   ADDR_GET_MACROMODEINDEX_OUTPUT*      pOut ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags())
   {
      if ((pIn->size != sizeof(ADDR_GET_MACROMODEINDEX_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfo = {0};
   pOut->macroModeIndex = HwlComputeMacroModeIndex(pIn->tileIndex, pIn->flags, pIn->bpp,
                  }
      /**
   ****************************************************************************************************
   *   Lib::ConvertTileIndex1
   *
   *   @brief
   *       Convert tile index to tile mode/type/info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ConvertTileIndex1(
      const ADDR_CONVERT_TILEINDEX1_INPUT* pIn,   ///< [in] input structure
   ADDR_CONVERT_TILEINDEX_OUTPUT* pOut         ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_CONVERT_TILEINDEX1_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
               HwlComputeMacroModeIndex(pIn->tileIndex, flags, pIn->bpp, pIn->numSamples,
            if (pIn->tileInfoHw)
   {
                        hwInput.pTileInfo = pOut->pTileInfo;
                                 }
      /**
   ****************************************************************************************************
   *   Lib::GetTileIndex
   *
   *   @brief
   *       Get tile index from tile mode/type/info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::GetTileIndex(
      const ADDR_GET_TILEINDEX_INPUT* pIn, ///< [in] input structure
   ADDR_GET_TILEINDEX_OUTPUT* pOut      ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_GET_TILEINDEX_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
                     }
      /**
   ****************************************************************************************************
   *   Lib::Thickness
   *
   *   @brief
   *       Get tile mode thickness
   *
   *   @return
   *       Tile mode thickness
   ****************************************************************************************************
   */
   UINT_32 Lib::Thickness(
         {
         }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               CMASK/HTILE
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Lib::ComputeHtileInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeHtilenfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeHtileInfo(
      const ADDR_COMPUTE_HTILE_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR_COMPUTE_HTILE_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               BOOL_32 isWidth8  = (pIn->blockWidth == 8) ? TRUE : FALSE;
            if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_HTILE_INFO_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                           // Change the input structure
            if (returnCode == ADDR_OK)
   {
         if (pIn->flags.tcCompatible)
   {
                                             pOut->sliceSize        = sliceSize;
   pOut->htileBytes       = pIn->flags.skipTcCompatSizeAlign ?
            }
   else
   {
      pOut->sliceSize        = pIn->flags.skipTcCompatSizeAlign ?
                                    pOut->pitch       = pIn->pitch;
   pOut->height      = pIn->height;
   pOut->baseAlign   = align;
   pOut->macroWidth  = 0;
   pOut->macroHeight = 0;
      }
   else
   {
      pOut->bpp = ComputeHtileInfo(pIn->flags,
                                 pIn->pitch,
   pIn->height,
   pIn->numSlices,
   pIn->isLinear,
   isWidth8,
   isHeight8,
   pIn->pTileInfo,
   &pOut->pitch,
                              }
      /**
   ****************************************************************************************************
   *   Lib::ComputeCmaskInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskInfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskInfo(
      const ADDR_COMPUTE_CMASK_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR_COMPUTE_CMASK_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_CMASK_INFO_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                           // Change the input structure
            if (returnCode == ADDR_OK)
   {
         returnCode = ComputeCmaskInfo(pIn->flags,
                                 pIn->pitch,
   pIn->height,
   pIn->numSlices,
   pIn->isLinear,
   pIn->pTileInfo,
   &pOut->pitch,
   &pOut->height,
                           }
      /**
   ****************************************************************************************************
   *   Lib::ComputeDccInfo
   *
   *   @brief
   *       Interface function to compute DCC key info
   *
   *   @return
   *       return code of HwlComputeDccInfo
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeDccInfo(
      const ADDR_COMPUTE_DCCINFO_INPUT*    pIn,    ///< [in] input structure
   ADDR_COMPUTE_DCCINFO_OUTPUT*         pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_DCCINFO_INPUT)) ||
         {
                     if (ret == ADDR_OK)
   {
               if (UseTileIndex(pIn->tileIndex))
   {
                                          if (ret == ADDR_OK)
   {
                                 }
      /**
   ****************************************************************************************************
   *   Lib::ComputeHtileAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeHtileAddrFromCoord(
      const ADDR_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_HTILE_ADDRFROMCOORD_OUTPUT*        pOut    ///< [out] output structure
      {
               BOOL_32 isWidth8  = (pIn->blockWidth == 8) ? TRUE : FALSE;
            if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_HTILE_ADDRFROMCOORD_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                           // Change the input structure
            if (returnCode == ADDR_OK)
   {
         if (pIn->flags.tcCompatible)
   {
         }
   else
   {
      pOut->addr = HwlComputeXmaskAddrFromCoord(pIn->pitch,
                                             pIn->height,
   pIn->x,
   pIn->y,
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeHtileCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeHtileCoordFromAddr(
      const ADDR_COMPUTE_HTILE_COORDFROMADDR_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_HTILE_COORDFROMADDR_OUTPUT*        pOut    ///< [out] output structure
      {
               BOOL_32 isWidth8  = (pIn->blockWidth == 8) ? TRUE : FALSE;
            if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_HTILE_COORDFROMADDR_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                           // Change the input structure
            if (returnCode == ADDR_OK)
   {
         HwlComputeXmaskCoordFromAddr(pIn->addr,
                              pIn->bitPosition,
   pIn->pitch,
   pIn->height,
   pIn->numSlices,
   1,
   pIn->isLinear,
                     }
      /**
   ****************************************************************************************************
   *   Lib::ComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskAddrFromCoord(
      const ADDR_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_CMASK_ADDRFROMCOORD_OUTPUT*        pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_CMASK_ADDRFROMCOORD_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                           // Change the input structure
            if (returnCode == ADDR_OK)
   {
         if (pIn->flags.tcCompatible == TRUE)
   {
         }
   else
   {
      pOut->addr = HwlComputeXmaskAddrFromCoord(pIn->pitch,
                                             pIn->height,
   pIn->x,
                              }
      /**
   ****************************************************************************************************
   *   Lib::ComputeCmaskCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskCoordFromAddr(
      const ADDR_COMPUTE_CMASK_COORDFROMADDR_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_CMASK_COORDFROMADDR_OUTPUT*        pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR_COMPUTE_CMASK_COORDFROMADDR_INPUT)) ||
         {
                     if (returnCode == ADDR_OK)
   {
      ADDR_TILEINFO tileInfoNull;
            if (UseTileIndex(pIn->tileIndex))
   {
         input = *pIn;
                           // Change the input structure
            if (returnCode == ADDR_OK)
   {
         HwlComputeXmaskCoordFromAddr(pIn->addr,
                              pIn->bitPosition,
   pIn->pitch,
   pIn->height,
   pIn->numSlices,
   2,
   pIn->isLinear,
                     }
      /**
   ****************************************************************************************************
   *   Lib::ComputeTileDataWidthAndHeight
   *
   *   @brief
   *       Compute the squared cache shape for per-tile data (CMASK and HTILE)
   *
   *   @return
   *       N/A
   *
   *   @note
   *       MacroWidth and macroHeight are measured in pixels
   ****************************************************************************************************
   */
   VOID Lib::ComputeTileDataWidthAndHeight(
      UINT_32         bpp,             ///< [in] bits per pixel
   UINT_32         cacheBits,       ///< [in] bits of cache
   ADDR_TILEINFO*  pTileInfo,       ///< [in] Tile info
   UINT_32*        pMacroWidth,     ///< [out] macro tile width
   UINT_32*        pMacroHeight     ///< [out] macro tile height
      {
      UINT_32 height = 1;
   UINT_32 width  = cacheBits / bpp;
            // Double height until the macro-tile is close to square
            while ((width > height * 2 * pipes) && !(width & 1))
   {
      width  /= 2;
               *pMacroWidth  = 8 * width;
            // Note: The above iterative comptuation is equivalent to the following
   //
   //int log2_height = ((log2(cacheBits)-log2(bpp)-log2(pipes))/2);
      }
      /**
   ****************************************************************************************************
   *   Lib::HwlComputeTileDataWidthAndHeightLinear
   *
   *   @brief
   *       Compute the squared cache shape for per-tile data (CMASK and HTILE) for linear layout
   *
   *   @return
   *       N/A
   *
   *   @note
   *       MacroWidth and macroHeight are measured in pixels
   ****************************************************************************************************
   */
   VOID Lib::HwlComputeTileDataWidthAndHeightLinear(
      UINT_32*        pMacroWidth,     ///< [out] macro tile width
   UINT_32*        pMacroHeight,    ///< [out] macro tile height
   UINT_32         bpp,             ///< [in] bits per pixel
   ADDR_TILEINFO*  pTileInfo        ///< [in] tile info
      {
      ADDR_ASSERT(bpp != 4);              // Cmask does not support linear layout prior to SI
   *pMacroWidth  = 8 * 512 / bpp;      // Align width to 512-bit memory accesses
      }
      /**
   ****************************************************************************************************
   *   Lib::ComputeHtileInfo
   *
   *   @brief
   *       Compute htile pitch,width, bytes per 2D slice
   *
   *   @return
   *       Htile bpp i.e. How many bits for an 8x8 tile
   *       Also returns by output parameters:
   *       *Htile pitch, height, total size in bytes, macro-tile dimensions and slice size*
   ****************************************************************************************************
   */
   UINT_32 Lib::ComputeHtileInfo(
      ADDR_HTILE_FLAGS flags,             ///< [in] htile flags
   UINT_32          pitchIn,           ///< [in] pitch input
   UINT_32          heightIn,          ///< [in] height input
   UINT_32          numSlices,         ///< [in] number of slices
   BOOL_32          isLinear,          ///< [in] if it is linear mode
   BOOL_32          isWidth8,          ///< [in] if htile block width is 8
   BOOL_32          isHeight8,         ///< [in] if htile block height is 8
   ADDR_TILEINFO*   pTileInfo,         ///< [in] Tile info
   UINT_32*         pPitchOut,         ///< [out] pitch output
   UINT_32*         pHeightOut,        ///< [out] height output
   UINT_64*         pHtileBytes,       ///< [out] bytes per 2D slice
   UINT_32*         pMacroWidth,       ///< [out] macro-tile width in pixels
   UINT_32*         pMacroHeight,      ///< [out] macro-tile width in pixels
   UINT_64*         pSliceSize,        ///< [out] slice size in bytes
   UINT_32*         pBaseAlign         ///< [out] base alignment
      {
         UINT_32 macroWidth;
   UINT_32 macroHeight;
   UINT_32 baseAlign;
   UINT_64 surfBytes;
                     const UINT_32 bpp = HwlComputeHtileBpp(isWidth8, isHeight8);
            if (isLinear)
   {
      HwlComputeTileDataWidthAndHeightLinear(&macroWidth,
                  }
   else
   {
      ComputeTileDataWidthAndHeight(bpp,
                                 *pPitchOut = PowTwoAlign(pitchIn,  macroWidth);
                     surfBytes = HwlComputeHtileBytes(*pPitchOut,
                                                   //
   // Use SafeAssign since they are optional
   //
                                          }
      /**
   ****************************************************************************************************
   *   Lib::ComputeCmaskBaseAlign
   *
   *   @brief
   *       Compute cmask base alignment
   *
   *   @return
   *       Cmask base alignment
   ****************************************************************************************************
   */
   UINT_32 Lib::ComputeCmaskBaseAlign(
      ADDR_CMASK_FLAGS flags,           ///< [in] Cmask flags
   ADDR_TILEINFO*   pTileInfo        ///< [in] Tile info
      {
               if (flags.tcCompatible)
   {
      ADDR_ASSERT(pTileInfo != NULL);
   if (pTileInfo)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeCmaskBytes
   *
   *   @brief
   *       Compute cmask size in bytes
   *
   *   @return
   *       Cmask size in bytes
   ****************************************************************************************************
   */
   UINT_64 Lib::ComputeCmaskBytes(
      UINT_32 pitch,        ///< [in] pitch
   UINT_32 height,       ///< [in] height
   UINT_32 numSlices     ///< [in] number of slices
      {
      return BITS_TO_BYTES(static_cast<UINT_64>(pitch) * height * numSlices * CmaskElemBits) /
      }
      /**
   ****************************************************************************************************
   *   Lib::ComputeCmaskInfo
   *
   *   @brief
   *       Compute cmask pitch,width, bytes per 2D slice
   *
   *   @return
   *       BlockMax. Also by output parameters: Cmask pitch,height, total size in bytes,
   *       macro-tile dimensions
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskInfo(
      ADDR_CMASK_FLAGS flags,            ///< [in] cmask flags
   UINT_32          pitchIn,           ///< [in] pitch input
   UINT_32          heightIn,          ///< [in] height input
   UINT_32          numSlices,         ///< [in] number of slices
   BOOL_32          isLinear,          ///< [in] is linear mode
   ADDR_TILEINFO*   pTileInfo,         ///< [in] Tile info
   UINT_32*         pPitchOut,         ///< [out] pitch output
   UINT_32*         pHeightOut,        ///< [out] height output
   UINT_64*         pCmaskBytes,       ///< [out] bytes per 2D slice
   UINT_32*         pMacroWidth,       ///< [out] macro-tile width in pixels
   UINT_32*         pMacroHeight,      ///< [out] macro-tile width in pixels
   UINT_64*         pSliceSize,        ///< [out] slice size in bytes
   UINT_32*         pBaseAlign,        ///< [out] base alignment
   UINT_32*         pBlockMax          ///< [out] block max == slice / 128 / 128 - 1
      {
      UINT_32 macroWidth;
   UINT_32 macroHeight;
   UINT_32 baseAlign;
   UINT_64 surfBytes;
                     const UINT_32 bpp = CmaskElemBits;
                     if (isLinear)
   {
      HwlComputeTileDataWidthAndHeightLinear(&macroWidth,
                  }
   else
   {
      ComputeTileDataWidthAndHeight(bpp,
                                 *pPitchOut = (pitchIn + macroWidth - 1) & ~(macroWidth - 1);
               sliceBytes = ComputeCmaskBytes(*pPitchOut,
                           while (sliceBytes % baseAlign)
   {
               sliceBytes = ComputeCmaskBytes(*pPitchOut,
                                       //
   // Use SafeAssign since they are optional
   //
                                       UINT_32 slice = (*pPitchOut) * (*pHeightOut);
         #if DEBUG
      if (slice % (64*256) != 0)
   {
            #endif //DEBUG
                  if (blockMax > maxBlockMax)
   {
      blockMax = maxBlockMax;
                           }
      /**
   ****************************************************************************************************
   *   Lib::ComputeXmaskCoordYFromPipe
   *
   *   @brief
   *       Compute the Y coord from pipe number for cmask/htile
   *
   *   @return
   *       Y coordinate
   *
   ****************************************************************************************************
   */
   UINT_32 Lib::ComputeXmaskCoordYFromPipe(
      UINT_32         pipe,       ///< [in] pipe number
   UINT_32         x           ///< [in] x coordinate
      {
      UINT_32 pipeBit0;
   UINT_32 pipeBit1;
   UINT_32 xBit0;
   UINT_32 xBit1;
   UINT_32 yBit0;
                     UINT_32 numPipes = m_pipes; // SI has its implementation
   //
   // Convert pipe + x to y coordinate.
   //
   switch (numPipes)
   {
      case 1:
         //
   // 1 pipe
   //
   // p0 = 0
   //
   y = 0;
   case 2:
         //
   // 2 pipes
   //
   // p0 = x0 ^ y0
   //
   // y0 = p0 ^ x0
                                    y = yBit0;
   case 4:
         //
   // 4 pipes
   //
   // p0 = x1 ^ y0
   // p1 = x0 ^ y1
   //
   // y0 = p0 ^ x1
   // y1 = p1 ^ x0
   //
                                                y = (yBit0 |
         case 8:
         //
   // 8 pipes
   //
   // r600 and r800 have different method
   //
   y = HwlComputeXmaskCoordYFrom8Pipe(pipe, x);
   default:
      }
      }
      /**
   ****************************************************************************************************
   *   Lib::HwlComputeXmaskCoordFromAddr
   *
   *   @brief
   *       Compute the coord from an address of a cmask/htile
   *
   *   @return
   *       N/A
   *
   *   @note
   *       This method is reused by htile, so rename to Xmask
   ****************************************************************************************************
   */
   VOID Lib::HwlComputeXmaskCoordFromAddr(
      UINT_64         addr,           ///< [in] address
   UINT_32         bitPosition,    ///< [in] bitPosition in a byte
   UINT_32         pitch,          ///< [in] pitch
   UINT_32         height,         ///< [in] height
   UINT_32         numSlices,      ///< [in] number of slices
   UINT_32         factor,         ///< [in] factor that indicates cmask or htile
   BOOL_32         isLinear,       ///< [in] linear or tiled HTILE layout
   BOOL_32         isWidth8,       ///< [in] TRUE if width is 8, FALSE means 4. It's register value
   BOOL_32         isHeight8,      ///< [in] TRUE if width is 8, FALSE means 4. It's register value
   ADDR_TILEINFO*  pTileInfo,      ///< [in] Tile info
   UINT_32*        pX,             ///< [out] x coord
   UINT_32*        pY,             ///< [out] y coord
   UINT_32*        pSlice          ///< [out] slice index
      {
      UINT_32 pipe;
   UINT_32 numPipes;
   UINT_32 numGroupBits;
   UINT_32 numPipeBits;
   UINT_32 macroTilePitch;
                                       UINT_32 pitchAligned = pitch;
   UINT_32 heightAligned = height;
                     UINT_64 macroIndex;
            UINT_64 macroNumber;
            UINT_32 macroX;
   UINT_32 macroY;
            UINT_32 microX;
            UINT_32 tilesPerMacro;
   UINT_32 macrosPerPitch;
            //
   // Extract pipe.
   //
   numPipes = HwlGetPipes(pTileInfo);
            //
   // Compute the number of group and pipe bits.
   //
   numGroupBits = Log2(m_pipeInterleaveBytes);
            UINT_32 groupBits = 8 * m_pipeInterleaveBytes;
               //
   // Compute the micro tile size, in bits. And macro tile pitch and height.
   //
   if (factor == 2) //CMASK
   {
                        ComputeCmaskInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   pTileInfo,
   &pitchAligned,
      }
   else  //HTILE
   {
               if (factor != 1)
   {
                           ComputeHtileInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   isWidth8,
   isHeight8,
   pTileInfo,
   &pitchAligned,
               // Should use aligned dims
   //
   pitch = pitchAligned;
               //
   // Convert byte address to bit address.
   //
               //
   // Remove pipe bits from address.
                                          macrosPerPitch = pitch / (macroTilePitch/factor);
            macroIndex = elemOffset / factor / tilesPerMacro;
            macroNumber = macroIndex * factor + microIndex % factor;
            macroX = static_cast<UINT_32>((macroNumber % macrosPerPitch));
   macroY = static_cast<UINT_32>((macroNumber % macrosPerSlice) / macrosPerPitch);
               microX = microNumber % (macroTilePitch / factor / MicroTileWidth);
            *pX = macroX * (macroTilePitch/factor) + microX * MicroTileWidth;
   *pY = macroY * macroTileHeight + (microY * MicroTileHeight << numPipeBits);
            microTileCoordY = ComputeXmaskCoordYFromPipe(pipe,
               //
   // Assemble final coordinates.
   //
         }
      /**
   ****************************************************************************************************
   *   Lib::HwlComputeXmaskAddrFromCoord
   *
   *   @brief
   *       Compute the address from an address of cmask (prior to si)
   *
   *   @return
   *       Address in bytes
   *
   ****************************************************************************************************
   */
   UINT_64 Lib::HwlComputeXmaskAddrFromCoord(
      UINT_32        pitch,          ///< [in] pitch
   UINT_32        height,         ///< [in] height
   UINT_32        x,              ///< [in] x coord
   UINT_32        y,              ///< [in] y coord
   UINT_32        slice,          ///< [in] slice/depth index
   UINT_32        numSlices,      ///< [in] number of slices
   UINT_32        factor,         ///< [in] factor that indicates cmask(2) or htile(1)
   BOOL_32        isLinear,       ///< [in] linear or tiled HTILE layout
   BOOL_32        isWidth8,       ///< [in] TRUE if width is 8, FALSE means 4. It's register value
   BOOL_32        isHeight8,      ///< [in] TRUE if width is 8, FALSE means 4. It's register value
   ADDR_TILEINFO* pTileInfo,      ///< [in] Tile info
   UINT_32*       pBitPosition    ///< [out] bit position inside a byte
      {
      UINT_64 addr;
   UINT_32 numGroupBits;
   UINT_32 numPipeBits;
   UINT_32 newPitch = 0;
   UINT_32 newHeight = 0;
   UINT_64 sliceBytes = 0;
   UINT_64 totalBytes = 0;
   UINT_64 sliceOffset;
   UINT_32 pipe;
   UINT_32 macroTileWidth;
   UINT_32 macroTileHeight;
   UINT_32 macroTilesPerRow;
   UINT_32 macroTileBytes;
   UINT_32 macroTileIndexX;
   UINT_32 macroTileIndexY;
   UINT_64 macroTileOffset;
   UINT_32 pixelBytesPerRow;
   UINT_32 pixelOffsetX;
   UINT_32 pixelOffsetY;
   UINT_32 pixelOffset;
   UINT_64 totalOffset;
   UINT_64 offsetLo;
   UINT_64 offsetHi;
                                 if (factor == 2) //CMASK
   {
               // For asics before SI, cmask is always tiled
      }
   else //HTILE
   {
      if (factor != 1) // Fix compile warning
   {
                              //
   // Compute the number of group bits and pipe bits.
   //
   numGroupBits = Log2(m_pipeInterleaveBytes);
            //
   // Compute macro tile dimensions.
   //
   if (factor == 2) // CMASK
   {
               ComputeCmaskInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   pTileInfo,
   &newPitch,
               }
   else // HTILE
   {
               ComputeHtileInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   isWidth8,
   isHeight8,
   pTileInfo,
   &newPitch,
   &newHeight,
                        //
   // Get the pipe.  Note that neither slice rotation nor pipe swizzling apply for CMASK.
   //
   pipe = ComputePipeFromCoord(x,
                              y,
         //
   // Compute the number of macro tiles per row.
   //
            //
   // Compute the number of bytes per macro tile.
   //
            //
   // Compute the offset to the macro tile containing the specified coordinate.
   //
   macroTileIndexX = x / macroTileWidth;
   macroTileIndexY = y / macroTileHeight;
            //
   // Compute the pixel offset within the macro tile.
   //
            //
   // The nibbles are interleaved (see below), so the part of the offset relative to the x
   // coordinate repeats halfway across the row. (Not for HTILE)
   //
   if (factor == 2)
   {
         }
   else
   {
                  //
   // Compute the y offset within the macro tile.
   //
                     //
   // Combine the slice offset and macro tile offset with the pixel offset, accounting for the
   // pipe bits in the middle of the address.
   //
            //
   // Split the offset to put some bits below the pipe bits and some above.
   //
   groupMask = (1 << numGroupBits) - 1;
   offsetLo  = totalOffset &  groupMask;
            //
   // Assemble the address from its components.
   //
   addr  = offsetLo;
   addr |= offsetHi;
   // This is to remove warning with /analyze option
   UINT_32 pipeBits = pipe << numGroupBits;
            //
   // Compute the bit position.  The lower nibble is used when the x coordinate within the macro
   // tile is less than half of the macro tile width, and the upper nibble is used when the x
   // coordinate within the macro tile is greater than or equal to half the macro tile width.
   //
               }
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Surface Addressing Shared
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Lib::ComputeSurfaceAddrFromCoordLinear
   *
   *   @brief
   *       Compute address from coord for linear surface
   *
   *   @return
   *       Address in bytes
   *
   ****************************************************************************************************
   */
   UINT_64 Lib::ComputeSurfaceAddrFromCoordLinear(
      UINT_32  x,              ///< [in] x coord
   UINT_32  y,              ///< [in] y coord
   UINT_32  slice,          ///< [in] slice/depth index
   UINT_32  sample,         ///< [in] sample index
   UINT_32  bpp,            ///< [in] bits per pixel
   UINT_32  pitch,          ///< [in] pitch
   UINT_32  height,         ///< [in] height
   UINT_32  numSlices,      ///< [in] number of slices
   UINT_32* pBitPosition    ///< [out] bit position inside a byte
      {
               UINT_64 sliceOffset = (slice + sample * numSlices)* sliceSize;
   UINT_64 rowOffset   = static_cast<UINT_64>(y) * pitch;
                     *pBitPosition = static_cast<UINT_32>(addr % 8);
               }
      /**
   ****************************************************************************************************
   *   Lib::ComputeSurfaceCoordFromAddrLinear
   *
   *   @brief
   *       Compute the coord from an address of a linear surface
   *
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID Lib::ComputeSurfaceCoordFromAddrLinear(
      UINT_64  addr,           ///< [in] address
   UINT_32  bitPosition,    ///< [in] bitPosition in a byte
   UINT_32  bpp,            ///< [in] bits per pixel
   UINT_32  pitch,          ///< [in] pitch
   UINT_32  height,         ///< [in] height
   UINT_32  numSlices,      ///< [in] number of slices
   UINT_32* pX,             ///< [out] x coord
   UINT_32* pY,             ///< [out] y coord
   UINT_32* pSlice,         ///< [out] slice/depth index
   UINT_32* pSample         ///< [out] sample index
      {
      const UINT_64 sliceSize = static_cast<UINT_64>(pitch) * height;
            *pX = static_cast<UINT_32>((linearOffset % sliceSize) % pitch);
   *pY = static_cast<UINT_32>((linearOffset % sliceSize) / pitch % height);
   *pSlice  = static_cast<UINT_32>((linearOffset / sliceSize) % numSlices);
      }
      /**
   ****************************************************************************************************
   *   Lib::ComputeSurfaceCoordFromAddrMicroTiled
   *
   *   @brief
   *       Compute the coord from an address of a micro tiled surface
   *
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID Lib::ComputeSurfaceCoordFromAddrMicroTiled(
      UINT_64         addr,               ///< [in] address
   UINT_32         bitPosition,        ///< [in] bitPosition in a byte
   UINT_32         bpp,                ///< [in] bits per pixel
   UINT_32         pitch,              ///< [in] pitch
   UINT_32         height,             ///< [in] height
   UINT_32         numSamples,         ///< [in] number of samples
   AddrTileMode    tileMode,           ///< [in] tile mode
   UINT_32         tileBase,           ///< [in] base offset within a tile
   UINT_32         compBits,           ///< [in] component bits actually needed(for planar surface)
   UINT_32*        pX,                 ///< [out] x coord
   UINT_32*        pY,                 ///< [out] y coord
   UINT_32*        pSlice,             ///< [out] slice/depth index
   UINT_32*        pSample,            ///< [out] sample index,
   AddrTileType    microTileType,      ///< [in] micro tiling order
   BOOL_32         isDepthSampleOrder  ///< [in] TRUE if in depth sample order
      {
      UINT_64 bitAddr;
   UINT_32 microTileThickness;
   UINT_32 microTileBits;
   UINT_64 sliceBits;
   UINT_64 rowBits;
   UINT_32 sliceIndex;
   UINT_32 microTileCoordX;
   UINT_32 microTileCoordY;
   UINT_32 pixelOffset;
   UINT_32 pixelCoordX = 0;
   UINT_32 pixelCoordY = 0;
   UINT_32 pixelCoordZ = 0;
            //
   // Convert byte address to bit address.
   //
            //
   // Compute the micro tile size, in bits.
   //
   switch (tileMode)
   {
      case ADDR_TM_1D_TILED_THICK:
         microTileThickness = ThickTileThickness;
   default:
                              //
   // Compute number of bits per slice and number of bits per row of micro tiles.
   //
                     //
   // Extract the slice index.
   //
   sliceIndex = static_cast<UINT_32>(bitAddr / sliceBits);
            //
   // Extract the y coordinate of the micro tile.
   //
   microTileCoordY = static_cast<UINT_32>(bitAddr / rowBits) * MicroTileHeight;
            //
   // Extract the x coordinate of the micro tile.
   //
            //
   // Compute the pixel offset within the micro tile.
   //
            //
   // Extract pixel coordinates from the offset.
   //
   HwlComputePixelCoordFromOffset(pixelOffset,
                                 bpp,
   numSamples,
   tileMode,
   tileBase,
   compBits,
            //
   // Assemble final coordinates.
   //
   *pX     = microTileCoordX + pixelCoordX;
   *pY     = microTileCoordY + pixelCoordY;
   *pSlice = (sliceIndex * microTileThickness) + pixelCoordZ;
            if (microTileThickness > 1)
   {
            }
      /**
   ****************************************************************************************************
   *   Lib::ComputePipeFromAddr
   *
   *   @brief
   *       Compute the pipe number from an address
   *
   *   @return
   *       Pipe number
   *
   ****************************************************************************************************
   */
   UINT_32 Lib::ComputePipeFromAddr(
      UINT_64 addr,        ///< [in] address
   UINT_32 numPipes     ///< [in] number of banks
      {
                        // R600
   // The LSBs of the address are arranged as follows:
   //   bank | pipe | group
   //
   // To get the pipe number, shift off the group bits and mask the pipe bits.
            // R800
   // The LSBs of the address are arranged as follows:
   //   bank | bankInterleave | pipe | pipeInterleave
   //
   // To get the pipe number, shift off the pipe interleave bits and mask the pipe bits.
                        }
      /**
   ****************************************************************************************************
   *   Lib::ComputeMicroTileEquation
   *
   *   @brief
   *       Compute micro tile equation
   *
   *   @return
   *       If equation can be computed
   *
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeMicroTileEquation(
      UINT_32         log2BytesPP,    ///< [in] log2 of bytes per pixel
   AddrTileMode    tileMode,       ///< [in] tile mode
   AddrTileType    microTileType,  ///< [in] pixel order in display/non-display mode
   ADDR_EQUATION*  pEquation       ///< [out] equation
      {
               for (UINT_32 i = 0; i < log2BytesPP; i++)
   {
      pEquation->addr[i].valid = 1;
   pEquation->addr[i].channel = 0;
                        ADDR_CHANNEL_SETTING x0 = InitChannel(1, 0, log2BytesPP + 0);
   ADDR_CHANNEL_SETTING x1 = InitChannel(1, 0, log2BytesPP + 1);
   ADDR_CHANNEL_SETTING x2 = InitChannel(1, 0, log2BytesPP + 2);
   ADDR_CHANNEL_SETTING y0 = InitChannel(1, 1, 0);
   ADDR_CHANNEL_SETTING y1 = InitChannel(1, 1, 1);
   ADDR_CHANNEL_SETTING y2 = InitChannel(1, 1, 2);
   ADDR_CHANNEL_SETTING z0 = InitChannel(1, 2, 0);
   ADDR_CHANNEL_SETTING z1 = InitChannel(1, 2, 1);
            UINT_32 thickness = Thickness(tileMode);
            if (microTileType != ADDR_THICK)
   {
      if (microTileType == ADDR_DISPLAYABLE)
   {
         switch (bpp)
   {
      case 8:
      pixelBit[0] = x0;
   pixelBit[1] = x1;
   pixelBit[2] = x2;
   pixelBit[3] = y1;
   pixelBit[4] = y0;
   pixelBit[5] = y2;
      case 16:
      pixelBit[0] = x0;
   pixelBit[1] = x1;
   pixelBit[2] = x2;
   pixelBit[3] = y0;
   pixelBit[4] = y1;
   pixelBit[5] = y2;
      case 32:
      pixelBit[0] = x0;
   pixelBit[1] = x1;
   pixelBit[2] = y0;
   pixelBit[3] = x2;
   pixelBit[4] = y1;
   pixelBit[5] = y2;
      case 64:
      pixelBit[0] = x0;
   pixelBit[1] = y0;
   pixelBit[2] = x1;
   pixelBit[3] = x2;
   pixelBit[4] = y1;
   pixelBit[5] = y2;
      case 128:
      pixelBit[0] = y0;
   pixelBit[1] = x0;
   pixelBit[2] = x1;
   pixelBit[3] = x2;
   pixelBit[4] = y1;
   pixelBit[5] = y2;
      default:
      ADDR_ASSERT_ALWAYS();
   }
   else if (microTileType == ADDR_NON_DISPLAYABLE || microTileType == ADDR_DEPTH_SAMPLE_ORDER)
   {
         pixelBit[0] = x0;
   pixelBit[1] = y0;
   pixelBit[2] = x1;
   pixelBit[3] = y1;
   pixelBit[4] = x2;
   }
   else if (microTileType == ADDR_ROTATED)
   {
                  switch (bpp)
   {
      case 8:
      pixelBit[0] = y0;
   pixelBit[1] = y1;
   pixelBit[2] = y2;
   pixelBit[3] = x1;
   pixelBit[4] = x0;
   pixelBit[5] = x2;
      case 16:
      pixelBit[0] = y0;
   pixelBit[1] = y1;
   pixelBit[2] = y2;
   pixelBit[3] = x0;
   pixelBit[4] = x1;
   pixelBit[5] = x2;
      case 32:
      pixelBit[0] = y0;
   pixelBit[1] = y1;
   pixelBit[2] = x0;
   pixelBit[3] = y2;
   pixelBit[4] = x1;
   pixelBit[5] = x2;
      case 64:
      pixelBit[0] = y0;
   pixelBit[1] = x0;
   pixelBit[2] = y1;
   pixelBit[3] = x1;
   pixelBit[4] = x2;
   pixelBit[5] = y2;
      default:
      retCode = ADDR_NOTSUPPORTED;
            if (thickness > 1)
   {
         pixelBit[6] = z0;
   pixelBit[7] = z1;
   }
   else
   {
            }
   else // ADDR_THICK
   {
               switch (bpp)
   {
         case 8:
   case 16:
      pixelBit[0] = x0;
   pixelBit[1] = y0;
   pixelBit[2] = x1;
   pixelBit[3] = y1;
   pixelBit[4] = z0;
   pixelBit[5] = z1;
      case 32:
      pixelBit[0] = x0;
   pixelBit[1] = y0;
   pixelBit[2] = x1;
   pixelBit[3] = z0;
   pixelBit[4] = y1;
   pixelBit[5] = z1;
      case 64:
   case 128:
      pixelBit[0] = x0;
   pixelBit[1] = y0;
   pixelBit[2] = z0;
   pixelBit[3] = x1;
   pixelBit[4] = y1;
   pixelBit[5] = z1;
      default:
                  pixelBit[6] = x2;
   pixelBit[7] = y2;
               if (thickness == 8)
   {
      pixelBit[8] = z2;
               // stackedDepthSlices is used for addressing mode that a tile block contains multiple slices,
   // which is not supported by our address lib
   pEquation->stackedDepthSlices = FALSE;
               }
      /**
   ****************************************************************************************************
   *   Lib::ComputePixelIndexWithinMicroTile
   *
   *   @brief
   *       Compute the pixel index inside a micro tile of surface
   *
   *   @return
   *       Pixel index
   *
   ****************************************************************************************************
   */
   UINT_32 Lib::ComputePixelIndexWithinMicroTile(
      UINT_32         x,              ///< [in] x coord
   UINT_32         y,              ///< [in] y coord
   UINT_32         z,              ///< [in] slice/depth index
   UINT_32         bpp,            ///< [in] bits per pixel
   AddrTileMode    tileMode,       ///< [in] tile mode
   AddrTileType    microTileType   ///< [in] pixel order in display/non-display mode
      {
      UINT_32 pixelBit0 = 0;
   UINT_32 pixelBit1 = 0;
   UINT_32 pixelBit2 = 0;
   UINT_32 pixelBit3 = 0;
   UINT_32 pixelBit4 = 0;
   UINT_32 pixelBit5 = 0;
   UINT_32 pixelBit6 = 0;
   UINT_32 pixelBit7 = 0;
   UINT_32 pixelBit8 = 0;
            UINT_32 x0 = _BIT(x, 0);
   UINT_32 x1 = _BIT(x, 1);
   UINT_32 x2 = _BIT(x, 2);
   UINT_32 y0 = _BIT(y, 0);
   UINT_32 y1 = _BIT(y, 1);
   UINT_32 y2 = _BIT(y, 2);
   UINT_32 z0 = _BIT(z, 0);
   UINT_32 z1 = _BIT(z, 1);
                              if (microTileType != ADDR_THICK)
   {
      if (microTileType == ADDR_DISPLAYABLE)
   {
         switch (bpp)
   {
      case 8:
      pixelBit0 = x0;
   pixelBit1 = x1;
   pixelBit2 = x2;
   pixelBit3 = y1;
   pixelBit4 = y0;
   pixelBit5 = y2;
      case 16:
      pixelBit0 = x0;
   pixelBit1 = x1;
   pixelBit2 = x2;
   pixelBit3 = y0;
   pixelBit4 = y1;
   pixelBit5 = y2;
      case 32:
      pixelBit0 = x0;
   pixelBit1 = x1;
   pixelBit2 = y0;
   pixelBit3 = x2;
   pixelBit4 = y1;
   pixelBit5 = y2;
      case 64:
      pixelBit0 = x0;
   pixelBit1 = y0;
   pixelBit2 = x1;
   pixelBit3 = x2;
   pixelBit4 = y1;
   pixelBit5 = y2;
      case 128:
      pixelBit0 = y0;
   pixelBit1 = x0;
   pixelBit2 = x1;
   pixelBit3 = x2;
   pixelBit4 = y1;
   pixelBit5 = y2;
      default:
      ADDR_ASSERT_ALWAYS();
   }
   else if (microTileType == ADDR_NON_DISPLAYABLE || microTileType == ADDR_DEPTH_SAMPLE_ORDER)
   {
         pixelBit0 = x0;
   pixelBit1 = y0;
   pixelBit2 = x1;
   pixelBit3 = y1;
   pixelBit4 = x2;
   }
   else if (microTileType == ADDR_ROTATED)
   {
                  switch (bpp)
   {
      case 8:
      pixelBit0 = y0;
   pixelBit1 = y1;
   pixelBit2 = y2;
   pixelBit3 = x1;
   pixelBit4 = x0;
   pixelBit5 = x2;
      case 16:
      pixelBit0 = y0;
   pixelBit1 = y1;
   pixelBit2 = y2;
   pixelBit3 = x0;
   pixelBit4 = x1;
   pixelBit5 = x2;
      case 32:
      pixelBit0 = y0;
   pixelBit1 = y1;
   pixelBit2 = x0;
   pixelBit3 = y2;
   pixelBit4 = x1;
   pixelBit5 = x2;
      case 64:
      pixelBit0 = y0;
   pixelBit1 = x0;
   pixelBit2 = y1;
   pixelBit3 = x1;
   pixelBit4 = x2;
   pixelBit5 = y2;
      default:
      ADDR_ASSERT_ALWAYS();
            if (thickness > 1)
   {
         pixelBit6 = z0;
      }
   else // ADDR_THICK
   {
               switch (bpp)
   {
         case 8:
   case 16:
      pixelBit0 = x0;
   pixelBit1 = y0;
   pixelBit2 = x1;
   pixelBit3 = y1;
   pixelBit4 = z0;
   pixelBit5 = z1;
      case 32:
      pixelBit0 = x0;
   pixelBit1 = y0;
   pixelBit2 = x1;
   pixelBit3 = z0;
   pixelBit4 = y1;
   pixelBit5 = z1;
      case 64:
   case 128:
      pixelBit0 = x0;
   pixelBit1 = y0;
   pixelBit2 = z0;
   pixelBit3 = x1;
   pixelBit4 = y1;
   pixelBit5 = z1;
      default:
                  pixelBit6 = x2;
               if (thickness == 8)
   {
                  pixelNumber = ((pixelBit0     ) |
                  (pixelBit1 << 1) |
   (pixelBit2 << 2) |
   (pixelBit3 << 3) |
   (pixelBit4 << 4) |
   (pixelBit5 << 5) |
            }
      /**
   ****************************************************************************************************
   *   Lib::AdjustPitchAlignment
   *
   *   @brief
   *       Adjusts pitch alignment for flipping surface
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID Lib::AdjustPitchAlignment(
      ADDR_SURFACE_FLAGS  flags,      ///< [in] Surface flags
   UINT_32*            pPitchAlign ///< [out] Pointer to pitch alignment
      {
      // Display engine hardwires lower 5 bit of GRPH_PITCH to ZERO which means 32 pixel alignment
   // Maybe it will be fixed in future but let's make it general for now.
   if (flags.display || flags.overlay)
   {
               if(flags.display)
   {
               }
      /**
   ****************************************************************************************************
   *   Lib::PadDimensions
   *
   *   @brief
   *       Helper function to pad dimensions
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID Lib::PadDimensions(
      AddrTileMode        tileMode,    ///< [in] tile mode
   UINT_32             bpp,         ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,       ///< [in] surface flags
   UINT_32             numSamples,  ///< [in] number of samples
   ADDR_TILEINFO*      pTileInfo,   ///< [in,out] bank structure.
   UINT_32             padDims,     ///< [in] Dimensions to pad valid value 1,2,3
   UINT_32             mipLevel,    ///< [in] MipLevel
   UINT_32*            pPitch,      ///< [in,out] pitch in pixels
   UINT_32*            pPitchAlign, ///< [in,out] pitch align could be changed in HwlPadDimensions
   UINT_32*            pHeight,     ///< [in,out] height in pixels
   UINT_32             heightAlign, ///< [in] height alignment
   UINT_32*            pSlices,     ///< [in,out] number of slices
   UINT_32             sliceAlign   ///< [in] number of slice alignment
      {
      UINT_32 pitchAlign = *pPitchAlign;
                     //
   // Override padding for mip levels
   //
   if (mipLevel > 0)
   {
      if (flags.cube)
   {
         // for cubemap, we only pad when client call with 6 faces as an identity
   if (*pSlices > 1)
   {
         }
   else
   {
                     // Any possibilities that padDims is 0?
   if (padDims == 0)
   {
                  if (IsPow2(pitchAlign))
   {
         }
   else // add this code to pass unit test, r600 linear mode is not align bpp to pow2 for linear
   {
      *pPitch += pitchAlign - 1;
   *pPitch /= pitchAlign;
               if (padDims > 1)
   {
      if (IsPow2(heightAlign))
   {
         }
   else
   {
         *pHeight += heightAlign - 1;
   *pHeight /= heightAlign;
               if (padDims > 2 || thickness > 1)
   {
      // for cubemap single face, we do not pad slices.
   // if we pad it, the slice number should be set to 6 and current mip level > 1
   if (flags.cube && (!m_configFlags.noCubeMipSlicesPad || flags.cubeAsArray))
   {
                  // normal 3D texture or arrays or cubemap has a thick mode? (Just pass unit test)
   if (thickness > 1)
   {
                        HwlPadDimensions(tileMode,
                     bpp,
   flags,
   numSamples,
   pTileInfo,
   mipLevel,
      }
         /**
   ****************************************************************************************************
   *   Lib::HwlPreHandleBaseLvl3xPitch
   *
   *   @brief
   *       Pre-handler of 3x pitch (96 bit) adjustment
   *
   *   @return
   *       Expected pitch
   ****************************************************************************************************
   */
   UINT_32 Lib::HwlPreHandleBaseLvl3xPitch(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,        ///< [in] input
   UINT_32                                 expPitch    ///< [in] pitch
      {
      ADDR_ASSERT(pIn->width == expPitch);
   //
   // If pitch is pre-multiplied by 3, we retrieve original one here to get correct miplevel size
   //
   if (ElemLib::IsExpand3x(pIn->format) &&
      pIn->mipLevel == 0 &&
      {
      expPitch /= 3;
                  }
      /**
   ****************************************************************************************************
   *   Lib::HwlPostHandleBaseLvl3xPitch
   *
   *   @brief
   *       Post-handler of 3x pitch adjustment
   *
   *   @return
   *       Expected pitch
   ****************************************************************************************************
   */
   UINT_32 Lib::HwlPostHandleBaseLvl3xPitch(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,        ///< [in] input
   UINT_32                                 expPitch    ///< [in] pitch
      {
      //
   // 96 bits surface of sub levels require element pitch of 32 bits instead
   // So we just return pitch in 32 bit pixels without timing 3
   //
   if (ElemLib::IsExpand3x(pIn->format) &&
      pIn->mipLevel == 0 &&
      {
                     }
         /**
   ****************************************************************************************************
   *   Lib::IsMacroTiled
   *
   *   @brief
   *       Check if the tile mode is macro tiled
   *
   *   @return
   *       TRUE if it is macro tiled (2D/2B/3D/3B)
   ****************************************************************************************************
   */
   BOOL_32 Lib::IsMacroTiled(
         {
         }
      /**
   ****************************************************************************************************
   *   Lib::IsMacro3dTiled
   *
   *   @brief
   *       Check if the tile mode is 3D macro tiled
   *
   *   @return
   *       TRUE if it is 3D macro tiled
   ****************************************************************************************************
   */
   BOOL_32 Lib::IsMacro3dTiled(
         {
         }
      /**
   ****************************************************************************************************
   *   Lib::IsMicroTiled
   *
   *   @brief
   *       Check if the tile mode is micro tiled
   *
   *   @return
   *       TRUE if micro tiled
   ****************************************************************************************************
   */
   BOOL_32 Lib::IsMicroTiled(
         {
         }
      /**
   ****************************************************************************************************
   *   Lib::IsLinear
   *
   *   @brief
   *       Check if the tile mode is linear
   *
   *   @return
   *       TRUE if linear
   ****************************************************************************************************
   */
   BOOL_32 Lib::IsLinear(
         {
         }
      /**
   ****************************************************************************************************
   *   Lib::IsPrtNoRotationTileMode
   *
   *   @brief
   *       Return TRUE if it is prt tile without rotation
   *   @note
   *       This function just used by CI
   ****************************************************************************************************
   */
   BOOL_32 Lib::IsPrtNoRotationTileMode(
         {
         }
      /**
   ****************************************************************************************************
   *   Lib::IsPrtTileMode
   *
   *   @brief
   *       Return TRUE if it is prt tile
   *   @note
   *       This function just used by CI
   ****************************************************************************************************
   */
   BOOL_32 Lib::IsPrtTileMode(
         {
         }
      /**
   ****************************************************************************************************
   *   Lib::ComputeMipLevel
   *
   *   @brief
   *       Compute mipmap level width/height/slices
   *   @return
   *      N/A
   ****************************************************************************************************
   */
   VOID Lib::ComputeMipLevel(
      ADDR_COMPUTE_SURFACE_INFO_INPUT* pIn ///< [in,out] Input structure
      {
      // Check if HWL has handled
            if (ElemLib::IsBlockCompressed(pIn->format))
   {
      if (pIn->mipLevel == 0)
   {
         // DXTn's level 0 must be multiple of 4
   // But there are exceptions:
   // 1. Internal surface creation in hostblt/vsblt/etc...
   // 2. Runtime doesn't reject ATI1/ATI2 whose width/height are not multiple of 4
   pIn->width = PowTwoAlign(pIn->width, 4);
                  }
      /**
   ****************************************************************************************************
   *   Lib::DegradeTo1D
   *
   *   @brief
   *       Check if surface can be degraded to 1D
   *   @return
   *       TRUE if degraded
   ****************************************************************************************************
   */
   BOOL_32 Lib::DegradeTo1D(
      UINT_32 width,                  ///< surface width
   UINT_32 height,                 ///< surface height
   UINT_32 macroTilePitchAlign,    ///< macro tile pitch align
   UINT_32 macroTileHeightAlign    ///< macro tile height align
      {
               // Check whether 2D tiling still has too much footprint
   if (degrade == FALSE)
   {
      // Only check width and height as slices are aligned to thickness
            UINT_32 alignedPitch = PowTwoAlign(width, macroTilePitchAlign);
   UINT_32 alignedHeight = PowTwoAlign(height, macroTileHeightAlign);
            // alignedSize > 1.5 * unalignedSize
   if (2 * alignedSize > 3 * unalignedSize)
   {
                        }
      /**
   ****************************************************************************************************
   *   Lib::OptimizeTileMode
   *
   *   @brief
   *       Check if base level's tile mode can be optimized (degraded)
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID Lib::OptimizeTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT*  pInOut     ///< [in, out] structure for surface info
      {
               BOOL_32 doOpt = (pInOut->flags.opt4Space == TRUE) ||
                           // Optimization can only be done on level 0 and samples <= 1
   if ((doOpt == TRUE)                     &&
      (pInOut->mipLevel == 0)             &&
   (IsPrtTileMode(tileMode) == FALSE)  &&
      {
      UINT_32 width = pInOut->width;
   UINT_32 height = pInOut->height;
   UINT_32 thickness = Thickness(tileMode);
   BOOL_32 macroTiledOK = TRUE;
   UINT_32 macroWidthAlign = 0;
   UINT_32 macroHeightAlign = 0;
            if (IsMacroTiled(tileMode))
   {
         macroTiledOK = HwlGetAlignmentInfoMacroTiled(pInOut,
                        if (macroTiledOK)
   {
         if ((pInOut->flags.display == FALSE) &&
      (pInOut->flags.opt4Space == TRUE) &&
      {
      // Check if linear mode is optimal
   if ((pInOut->height == 1) &&
      (IsLinear(tileMode) == FALSE) &&
   (ElemLib::IsBlockCompressed(pInOut->format) == FALSE) &&
   (pInOut->flags.depth == FALSE) &&
   (pInOut->flags.stencil == FALSE) &&
   (m_configFlags.disableLinearOpt == FALSE) &&
      {
         }
   else if (IsMacroTiled(tileMode) && (pInOut->flags.tcCompatible == FALSE))
   {
      if (DegradeTo1D(width, height, macroWidthAlign, macroHeightAlign))
   {
         tileMode = (thickness == 1) ?
   }
   else if ((thickness > 1) && (pInOut->flags.disallowLargeThickDegrade == 0))
   {
                                                                                                            if (macroTiledOK &&
         {
                           if (macroTiledOK)
   {
      if ((pInOut->flags.minimizeAlignment == TRUE) &&
      (pInOut->numSamples <= 1) &&
      {
      UINT_32 macroSize = PowTwoAlign(width, macroWidthAlign) *
                        if (macroSize > microSize)
   {
                           if ((pInOut->maxBaseAlign != 0) &&
         {
      if (macroSizeAlign > pInOut->maxBaseAlign)
   {
                                    }
   else if (pInOut->maxBaseAlign < Block64K)
   {
      tileMode = (thickness == 1) ?
      }
   else
   {
                           if (convertToPrt)
   {
      if ((pInOut->flags.matchStencilTileCfg == TRUE) && (pInOut->numSamples <= 1))
   {
         }
   else
   {
            }
   else if (tileMode != pInOut->tileMode)
   {
                     }
      /**
   ****************************************************************************************************
   *   Lib::DegradeLargeThickTile
   *
   *   @brief
   *       Check if the thickness needs to be reduced if a tile is too large
   *   @return
   *       The degraded tile mode (unchanged if not degraded)
   ****************************************************************************************************
   */
   AddrTileMode Lib::DegradeLargeThickTile(
      AddrTileMode tileMode,
      {
      // Override tilemode
   // When tile_width (8) * tile_height (8) * thickness * element_bytes is > row_size,
   // it is better to just use THIN mode in this case
            if (thickness > 1 && m_configFlags.allowLargeThickTile == 0)
   {
               if (tileSize > m_rowSize)
   {
         switch (tileMode)
   {
      case ADDR_TM_2D_TILED_XTHICK:
      if ((tileSize >> 1) <= m_rowSize)
   {
         tileMode = ADDR_TM_2D_TILED_THICK;
   }
                           case ADDR_TM_3D_TILED_XTHICK:
      if ((tileSize >> 1) <= m_rowSize)
   {
         tileMode = ADDR_TM_3D_TILED_THICK;
   }
                                                                                          default:
                     }
      /**
   ****************************************************************************************************
   *   Lib::PostComputeMipLevel
   *   @brief
   *       Compute MipLevel info (including level 0) after surface adjustment
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::PostComputeMipLevel(
      ADDR_COMPUTE_SURFACE_INFO_INPUT*    pIn,   ///< [in,out] Input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*   pOut   ///< [out] Output structure
      {
      // Mipmap including level 0 must be pow2 padded since either SI hw expects so or it is
   // required by CFX  for Hw Compatibility between NI and SI. Otherwise it is only needed for
            if (pIn->flags.pow2Pad)
   {
      pIn->width      = NextPow2(pIn->width);
   pIn->height     = NextPow2(pIn->height);
      }
   else if (pIn->mipLevel > 0)
   {
      pIn->width      = NextPow2(pIn->width);
            if (!pIn->flags.cube)
   {
                                 }
      /**
   ****************************************************************************************************
   *   Lib::HwlSetupTileCfg
   *
   *   @brief
   *       Map tile index to tile setting.
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::HwlSetupTileCfg(
      UINT_32         bpp,              ///< Bits per pixel
   INT_32          index,            ///< [in] Tile index
   INT_32          macroModeIndex,   ///< [in] Index in macro tile mode table(CI)
   ADDR_TILEINFO*  pInfo,            ///< [out] Tile Info
   AddrTileMode*   pMode,            ///< [out] Tile mode
   AddrTileType*   pType             ///< [out] Tile type
      {
         }
      /**
   ****************************************************************************************************
   *   Lib::HwlGetPipes
   *
   *   @brief
   *       Get number pipes
   *   @return
   *       num pipes
   ****************************************************************************************************
   */
   UINT_32 Lib::HwlGetPipes(
      const ADDR_TILEINFO* pTileInfo    ///< [in] Tile info
      {
      //pTileInfo can be NULL when asic is 6xx and 8xx.
      }
      /**
   ****************************************************************************************************
   *   Lib::ComputeQbStereoInfo
   *
   *   @brief
   *       Get quad buffer stereo information
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID Lib::ComputeQbStereoInfo(
      ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut    ///< [in,out] updated pOut+pStereoInfo
      {
      ADDR_ASSERT(pOut->bpp >= 8);
            // Save original height
            // Right offset
            pOut->pStereoInfo->rightSwizzle = HwlComputeQbStereoRightSwizzle(pOut);
   // Double height
   pOut->height <<= 1;
            // Double size
                        }
         /**
   ****************************************************************************************************
   *   Lib::ComputePrtInfo
   *
   *   @brief
   *       Compute prt surface related info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputePrtInfo(
      const ADDR_PRT_INFO_INPUT*  pIn,
      {
                        UINT_32     expandX = 1;
   UINT_32     expandY = 1;
            UINT_32     bpp = GetElemLib()->GetBitsPerPixel(pIn->format,
                        if (bpp <8 || bpp == 24 || bpp == 48 || bpp == 96)
   {
                  UINT_32     numFrags = pIn->numFrags;
            UINT_32     tileWidth = 0;
   UINT_32     tileHeight = 0;
   if (returnCode == ADDR_OK)
   {
      // 3D texture without depth or 2d texture
   if (pIn->baseMipDepth > 1 || pIn->baseMipHeight > 1)
   {
         if (bpp == 8)
   {
      tileWidth = 256;
      }
   else if (bpp == 16)
   {
      tileWidth = 256;
      }
   else if (bpp == 32)
   {
      tileWidth = 128;
      }
   else if (bpp == 64)
   {
                           if (elemMode == ADDR_UNCOMPRESSED)
   {
      tileWidth = 128;
         }
   else if (bpp == 128)
   {
                           if (elemMode == ADDR_UNCOMPRESSED)
   {
      tileWidth = 64;
                  if (numFrags == 2)
   {
         }
   else if (numFrags == 4)
   {
      tileWidth = tileWidth / 2;
      }
   else if (numFrags == 8)
   {
      tileWidth = tileWidth / 4;
      }
   else    // 1d
   {
         tileHeight = 1;
   if (bpp == 8)
   {
         }
   else if (bpp == 16)
   {
         }
   else if (bpp == 32)
   {
         }
   else if (bpp == 64)
   {
         }
   else if (bpp == 128)
   {
                     pOut->prtTileWidth = tileWidth;
               }
      } // V1
   } // Addr
