   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
   /**
   ****************************************************************************************************
   * @file  egbaddrlib.cpp
   * @brief Contains the EgBasedLib class implementation.
   ****************************************************************************************************
   */
      #include "egbaddrlib.h"
      namespace Addr
   {
   namespace V1
   {
      /**
   ****************************************************************************************************
   *   EgBasedLib::EgBasedLib
   *
   *   @brief
   *       Constructor
   *
   *   @note
   *
   ****************************************************************************************************
   */
   EgBasedLib::EgBasedLib(const Client* pClient)
      :
   Lib(pClient),
   m_ranks(0),
   m_logicalBanks(0),
      {
   }
      /**
   ****************************************************************************************************
   *   EgBasedLib::~EgBasedLib
   *
   *   @brief
   *       Destructor
   ****************************************************************************************************
   */
   EgBasedLib::~EgBasedLib()
   {
   }
      /**
   ****************************************************************************************************
   *   EgBasedLib::DispatchComputeSurfaceInfo
   *
   *   @brief
   *       Compute surface sizes include padded pitch,height,slices,total size in bytes,
   *       meanwhile output suitable tile mode and base alignment might be changed in this
   *       call as well. Results are returned through output parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::DispatchComputeSurfaceInfo(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut    ///< [out] output structure
      {
      AddrTileMode        tileMode      = pIn->tileMode;
   UINT_32             bpp           = pIn->bpp;
   UINT_32             numSamples    = pIn->numSamples;
   UINT_32             numFrags      = ((pIn->numFrags == 0) ? numSamples : pIn->numFrags);
   UINT_32             pitch         = pIn->width;
   UINT_32             height        = pIn->height;
   UINT_32             numSlices     = pIn->numSlices;
   UINT_32             mipLevel      = pIn->mipLevel;
            ADDR_TILEINFO       tileInfoDef   = {0};
   ADDR_TILEINFO*      pTileInfo     = &tileInfoDef;
   UINT_32             padDims       = 0;
            if (pIn->flags.disallowLargeThickDegrade == 0)
   {
                  // Only override numSamples for NI above
   if (m_chipFamily >= ADDR_CHIP_FAMILY_NI)
   {
      if (numFrags != numSamples) // This means EQAA
   {
         // The real surface size needed is determined by number of fragments
            // Save altered numSamples in pOut
               // Caller makes sure pOut->pTileInfo is not NULL, see HwlComputeSurfaceInfo
            if (pOut->pTileInfo != NULL)
   {
                  // Set default values
   if (pIn->pTileInfo != NULL)
   {
      if (pTileInfo != pIn->pTileInfo)
   {
            }
   else
   {
                  // For macro tile mode, we should calculate default tiling parameters
   HwlSetupTileInfo(tileMode,
                     flags,
   bpp,
   pitch,
   height,
   numSamples,
            if (flags.cube)
   {
      if (mipLevel == 0)
   {
                  if (numSlices == 1)
   {
         // This is calculating one face, remove cube flag
               switch (tileMode)
   {
      case ADDR_TM_LINEAR_GENERAL://fall through
   case ADDR_TM_LINEAR_ALIGNED:
                  case ADDR_TM_1D_TILED_THIN1://fall through
   case ADDR_TM_1D_TILED_THICK:
                  case ADDR_TM_2D_TILED_THIN1:    //fall through
   case ADDR_TM_2D_TILED_THICK:    //fall through
   case ADDR_TM_3D_TILED_THIN1:    //fall through
   case ADDR_TM_3D_TILED_THICK:    //fall through
   case ADDR_TM_2D_TILED_XTHICK:   //fall through
   case ADDR_TM_3D_TILED_XTHICK:   //fall through
   case ADDR_TM_PRT_TILED_THIN1:   //fall through
   case ADDR_TM_PRT_2D_TILED_THIN1://fall through
   case ADDR_TM_PRT_3D_TILED_THIN1://fall through
   case ADDR_TM_PRT_TILED_THICK:   //fall through
   case ADDR_TM_PRT_2D_TILED_THICK://fall through
   case ADDR_TM_PRT_3D_TILED_THICK:
                  default:
         valid = FALSE;
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceInfoLinear
   *
   *   @brief
   *       Compute linear surface sizes include padded pitch, height, slices, total size in
   *       bytes, meanwhile alignments as well. Since it is linear mode, so output tile mode
   *       will not be changed here. Results are returned through output parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::ComputeSurfaceInfoLinear(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,    ///< [in] Input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut,   ///< [out] Output structure
   UINT_32                                 padDims ///< [in] Dimensions to padd
      {
      UINT_32 expPitch = pIn->width;
   UINT_32 expHeight = pIn->height;
            // No linear MSAA on real H/W, keep this for TGL
                     //
   // Compute the surface alignments.
   //
   ComputeSurfaceAlignmentsLinear(pIn->tileMode,
                                    if ((pIn->tileMode == ADDR_TM_LINEAR_GENERAL) && pIn->flags.color && (pIn->height > 1))
      #if !ALT_TEST
         // When linear_general surface is accessed in multiple lines, it requires 8 pixels in pitch
   // alignment since PITCH_TILE_MAX is in unit of 8 pixels.
   // It is OK if it is accessed per line.
   #endif
                                 //
   // Pad pitch and height to the required granularities.
   //
   PadDimensions(pIn->tileMode,
                  pIn->bpp,
   pIn->flags,
   numSamples,
   pOut->pTileInfo,
   padDims,
   pIn->mipLevel,
                  //
   // Adjust per HWL
                     logicalSliceSize = HwlGetSizeAdjustmentLinear(pIn->tileMode,
                                                if ((pIn->pitchAlign != 0) || (pIn->heightAlign != 0))
   {
      if (pIn->pitchAlign != 0)
   {
                        if (IsPow2(pOut->pitchAlign))
   {
         }
   else
   {
      expPitch += pOut->pitchAlign - 1;
   expPitch /= pOut->pitchAlign;
               if (pIn->heightAlign != 0)
   {
                        if (IsPow2(pOut->heightAlign))
   {
         }
   else
   {
      expHeight += pOut->heightAlign - 1;
   expHeight /= pOut->heightAlign;
                           pOut->pitch = expPitch;
   pOut->height = expHeight;
                                 }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceInfoMicroTiled
   *
   *   @brief
   *       Compute 1D/Micro Tiled surface sizes include padded pitch, height, slices, total
   *       size in bytes, meanwhile alignments as well. Results are returned through output
   *       parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::ComputeSurfaceInfoMicroTiled(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,        ///< [in] Input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut,       ///< [out] Output structure
   UINT_32                                 padDims,    ///< [in] Dimensions to padd
   AddrTileMode                            expTileMode ///< [in] Expected tile mode
      {
               UINT_32 microTileThickness;
   UINT_32 expPitch = pIn->width;
   UINT_32 expHeight = pIn->height;
            // No 1D MSAA on real H/W, keep this for TGL
            //
   // Compute the micro tile thickness.
   //
            //
   // Extra override for mip levels
   //
   if (pIn->mipLevel > 0)
   {
      //
   // Reduce tiling mode from thick to thin if the number of slices is less than the
   // micro tile thickness.
   //
   if ((expTileMode == ADDR_TM_1D_TILED_THICK) &&
         {
         expTileMode = HwlDegradeThickTileMode(ADDR_TM_1D_TILED_THICK, expNumSlices, NULL);
   if (expTileMode != ADDR_TM_1D_TILED_THICK)
   {
                     //
   // Compute the surface restrictions.
   //
   ComputeSurfaceAlignmentsMicroTiled(expTileMode,
                                                         //
   // Pad pitch and height to the required granularities.
   // Compute surface size.
   // Return parameters.
   //
   PadDimensions(expTileMode,
                  pIn->bpp,
   pIn->flags,
   numSamples,
   pOut->pTileInfo,
   padDims,
   pIn->mipLevel,
         //
   // Get HWL specific pitch adjustment
   //
   UINT_64 logicalSliceSize = HwlGetSizeAdjustmentMicroTiled(microTileThickness,
                                                   pOut->pitch = expPitch;
   pOut->height = expHeight;
                                 }
         /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceInfoMacroTiled
   *
   *   @brief
   *       Compute 2D/macro tiled surface sizes include padded pitch, height, slices, total
   *       size in bytes, meanwhile output suitable tile mode and alignments might be changed
   *       in this call as well. Results are returned through output parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::ComputeSurfaceInfoMacroTiled(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,        ///< [in] Input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut,       ///< [out] Output structure
   UINT_32                                 padDims,    ///< [in] Dimensions to padd
   AddrTileMode                            expTileMode ///< [in] Expected tile mode
      {
               AddrTileMode origTileMode = expTileMode;
            UINT_32 paddedPitch;
   UINT_32 paddedHeight;
            UINT_32 expPitch     = pIn->width;
   UINT_32 expHeight    = pIn->height;
                     //
   // Compute the surface restrictions as base
   // SanityCheckMacroTiled is called in ComputeSurfaceAlignmentsMacroTiled
   //
   valid = ComputeSurfaceAlignmentsMacroTiled(expTileMode,
                                    if (valid)
   {
      //
   // Compute the micro tile thickness.
   //
            //
   // Find the correct tiling mode for mip levels
   //
   if (pIn->mipLevel > 0)
   {
         //
   // Try valid tile mode
   //
   expTileMode = ComputeSurfaceMipLevelTileMode(expTileMode,
                                                      if (!IsMacroTiled(expTileMode)) // Downgraded to micro-tiled
   {
         }
   else if (microTileThickness != Thickness(expTileMode))
   {
      //
   // Re-compute if thickness changed since bank-height may be changed!
   //
               paddedPitch     = expPitch;
            //
   // Re-cal alignment
   //
   if (expTileMode != origTileMode) // Tile mode is changed but still macro-tiled
   {
         valid = ComputeSurfaceAlignmentsMacroTiled(expTileMode,
                                    //
   // Do padding
   //
   PadDimensions(expTileMode,
                  pIn->bpp,
   pIn->flags,
   numSamples,
   pOut->pTileInfo,
   padDims,
               if (pIn->flags.qbStereo &&
         {
                  if (stereoHeightAlign != 0)
   {
                  if ((pIn->flags.needEquation == TRUE) &&
         (m_chipFamily == ADDR_CHIP_FAMILY_SI) &&
   (pIn->numMipLevels > 1) &&
   {
                           for (UINT_32 i = 1; i < pIn->numMipLevels; i++)
   {
      UINT_32 mipPitch = Max(1u, paddedPitch >> i);
   UINT_32 mipHeight = Max(1u, pIn->height >> i);
   UINT_32 mipSlices = pIn->flags.volume ?
         expTileMode = ComputeSurfaceMipLevelTileMode(expTileMode,
                                                      if (IsMacroTiled(expTileMode))
   {
      if (PowTwoAlign(mipPitch, pOut->blockWidth) !=
         {
         convertTo1D = TRUE;
      }
   else
   {
                     if (convertTo1D)
   {
                  pOut->pitch = paddedPitch;
   // Put this check right here to workaround special mipmap cases which the original height
   // is needed.
   // The original height is pre-stored in pOut->height in PostComputeMipLevel and
   // pOut->pitch is needed in HwlCheckLastMacroTiledLvl, too.
   if (m_configFlags.checkLast2DLevel && (numSamples == 1)) // Don't check MSAA
   {
         // Set a TRUE in pOut if next Level is the first 1D sub level
   }
                     //
   // Compute the size of a slice.
   //
   bytesPerSlice = BITS_TO_BYTES(static_cast<UINT_64>(paddedPitch) *
                                                }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceAlignmentsLinear
   *
   *   @brief
   *       Compute linear surface alignment, calculation results are returned through
   *       output parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::ComputeSurfaceAlignmentsLinear(
      AddrTileMode        tileMode,          ///< [in] tile mode
   UINT_32             bpp,               ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,             ///< [in] surface flags
   UINT_32*            pBaseAlign,        ///< [out] base address alignment in bytes
   UINT_32*            pPitchAlign,       ///< [out] pitch alignment in pixels
   UINT_32*            pHeightAlign       ///< [out] height alignment in pixels
      {
               switch (tileMode)
   {
      case ADDR_TM_LINEAR_GENERAL:
         //
   // The required base alignment and pitch and height granularities is to 1 element.
   //
   *pBaseAlign   = (bpp > 8) ? bpp / 8 : 1;
   *pPitchAlign  = 1;
   *pHeightAlign = 1;
   case ADDR_TM_LINEAR_ALIGNED:
         //
   // The required alignment for base is the pipe interleave size.
   // The required granularity for pitch is hwl dependent.
   // The required granularity for height is one row.
   //
   *pBaseAlign     = m_pipeInterleaveBytes;
   *pPitchAlign    = HwlGetPitchAlignmentLinear(bpp, flags);
   *pHeightAlign   = 1;
   default:
         *pBaseAlign     = 1;
   *pPitchAlign    = 1;
   *pHeightAlign   = 1;
                           }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceAlignmentsMicroTiled
   *
   *   @brief
   *       Compute 1D tiled surface alignment, calculation results are returned through
   *       output parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::ComputeSurfaceAlignmentsMicroTiled(
      AddrTileMode        tileMode,          ///< [in] tile mode
   UINT_32             bpp,               ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,             ///< [in] surface flags
   UINT_32             mipLevel,          ///< [in] mip level
   UINT_32             numSamples,        ///< [in] number of samples
   UINT_32*            pBaseAlign,        ///< [out] base address alignment in bytes
   UINT_32*            pPitchAlign,       ///< [out] pitch alignment in pixels
   UINT_32*            pHeightAlign       ///< [out] height alignment in pixels
      {
               //
   // The required alignment for base is the pipe interleave size.
   //
                                       if (flags.czDispCompatible && (mipLevel == 0))
   {
      *pBaseAlign  = PowTwoAlign(*pBaseAlign, 4096);                         //Base address MOD 4096 = 0
      }
               }
         /**
   ****************************************************************************************************
   *   EgBasedLib::HwlReduceBankWidthHeight
   *
   *   @brief
   *       Additional checks, reduce bankHeight/bankWidth if needed and possible
   *       tileSize*BANK_WIDTH*BANK_HEIGHT <= ROW_SIZE
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::HwlReduceBankWidthHeight(
      UINT_32             tileSize,           ///< [in] tile size
   UINT_32             bpp,                ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,              ///< [in] surface flags
   UINT_32             numSamples,         ///< [in] number of samples
   UINT_32             bankHeightAlign,    ///< [in] bank height alignment
   UINT_32             pipes,              ///< [in] pipes
   ADDR_TILEINFO*      pTileInfo           ///< [in,out] bank structure.
      {
      UINT_32 macroAspectAlign;
            if (tileSize * pTileInfo->bankWidth * pTileInfo->bankHeight > m_rowSize)
   {
               // Try reducing bankWidth first
   if (stillGreater && pTileInfo->bankWidth > 1)
   {
         while (stillGreater && pTileInfo->bankWidth > 0)
                     if (pTileInfo->bankWidth == 0)
   {
                                          // bankWidth is reduced above, so we need to recalculate bankHeight and ratio
   bankHeightAlign = Max(1u,
                                       if (numSamples == 1)
   {
      macroAspectAlign = Max(1u,
                     pTileInfo->macroAspectRatio = PowTwoAlign(pTileInfo->macroAspectRatio,
               // Early quit bank_height degradation for "64" bit z buffer
   if (flags.depth && bpp >= 64)
   {
                  // Then try reducing bankHeight
   if (stillGreater && pTileInfo->bankHeight > bankHeightAlign)
   {
         while (stillGreater && pTileInfo->bankHeight > bankHeightAlign)
                     if (pTileInfo->bankHeight < bankHeightAlign)
   {
                        stillGreater =
                        // Generate a warning if we still fail to meet this constraint
   if (valid == FALSE)
   {
         ADDR_WARN(
                        }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceAlignmentsMacroTiled
   *
   *   @brief
   *       Compute 2D tiled surface alignment, calculation results are returned through
   *       output parameters.
   *
   *   @return
   *       TRUE if no error occurs
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::ComputeSurfaceAlignmentsMacroTiled(
      AddrTileMode                      tileMode,           ///< [in] tile mode
   UINT_32                           bpp,                ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS                flags,              ///< [in] surface flags
   UINT_32                           mipLevel,           ///< [in] mip level
   UINT_32                           numSamples,         ///< [in] number of samples
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pOut                ///< [in,out] Surface output
      {
                        if (valid)
   {
      UINT_32 macroTileWidth;
            UINT_32 tileSize;
   UINT_32 bankHeightAlign;
            UINT_32 thickness = Thickness(tileMode);
            //
   // Align bank height first according to latest h/w spec
            // tile_size = MIN(tile_split, 64 * tile_thickness * element_bytes * num_samples)
   tileSize = Min(pTileInfo->tileSplitBytes,
            // bank_height_align =
   // MAX(1, (pipe_interleave_bytes * bank_interleave)/(tile_size*bank_width))
   bankHeightAlign = Max(1u,
                                 // num_pipes * bank_width * macro_tile_aspect >=
   // (pipe_interleave_size * bank_interleave) / tile_size
   if (numSamples == 1)
   {
         // this restriction is only for mipmap (mipmap's numSamples must be 1)
   macroAspectAlign = Max(1u,
                              valid = HwlReduceBankWidthHeight(tileSize,
                                          //
   // The required granularity for pitch is the macro tile width.
   //
   macroTileWidth = MicroTileWidth * pTileInfo->bankWidth * pipes *
            pOut->pitchAlign = macroTileWidth;
                     //
   // The required granularity for height is the macro tile height.
   //
   macroTileHeight = MicroTileHeight * pTileInfo->bankHeight * pTileInfo->banks /
            pOut->heightAlign = macroTileHeight;
            //
   // Compute base alignment
   //
   pOut->baseAlign =
                           }
      /**
   ****************************************************************************************************
   *   EgBasedLib::SanityCheckMacroTiled
   *
   *   @brief
   *       Check if macro-tiled parameters are valid
   *   @return
   *       TRUE if valid
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::SanityCheckMacroTiled(
      ADDR_TILEINFO* pTileInfo   ///< [in] macro-tiled parameters
      {
      BOOL_32 valid       = TRUE;
            switch (pTileInfo->banks)
   {
      case 2: //fall through
   case 4: //fall through
   case 8: //fall through
   case 16:
         default:
                        if (valid)
   {
      switch (pTileInfo->bankWidth)
   {
         case 1: //fall through
   case 2: //fall through
   case 4: //fall through
   case 8:
         default:
                     if (valid)
   {
      switch (pTileInfo->bankHeight)
   {
         case 1: //fall through
   case 2: //fall through
   case 4: //fall through
   case 8:
         default:
                     if (valid)
   {
      switch (pTileInfo->macroAspectRatio)
   {
         case 1: //fall through
   case 2: //fall through
   case 4: //fall through
   case 8:
         default:
                     if (valid)
   {
      if (pTileInfo->banks < pTileInfo->macroAspectRatio)
   {
         // This will generate macro tile height <= 1
               if (valid)
   {
      if (pTileInfo->tileSplitBytes > m_rowSize)
   {
                     if (valid)
   {
                           // Add this assert for guidance
               }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceMipLevelTileMode
   *
   *   @brief
   *       Compute valid tile mode for surface mipmap sub-levels
   *
   *   @return
   *       Suitable tile mode
   ****************************************************************************************************
   */
   AddrTileMode EgBasedLib::ComputeSurfaceMipLevelTileMode(
      AddrTileMode        baseTileMode,   ///< [in] base tile mode
   UINT_32             bpp,            ///< [in] bits per pixels
   UINT_32             pitch,          ///< [in] current level pitch
   UINT_32             height,         ///< [in] current level height
   UINT_32             numSlices,      ///< [in] current number of slices
   UINT_32             numSamples,     ///< [in] number of samples
   UINT_32             pitchAlign,     ///< [in] pitch alignment
   UINT_32             heightAlign,    ///< [in] height alignment
   ADDR_TILEINFO*      pTileInfo       ///< [in] ptr to bank structure
      {
      UINT_64 bytesPerSlice;
            AddrTileMode expTileMode = baseTileMode;
   UINT_32 microTileThickness = Thickness(expTileMode);
            //
   // Compute the size of a slice.
   //
   bytesPerSlice = BITS_TO_BYTES(static_cast<UINT_64>(pitch) * height * bpp * numSamples);
            //
   // Reduce tiling mode from thick to thin if the number of slices is less than the
   // micro tile thickness.
   //
   if (numSlices < microTileThickness)
   {
                  if (bytesPerTile > pTileInfo->tileSplitBytes)
   {
                  UINT_32 threshold1 =
            UINT_32 threshold2 =
            //
   // Reduce the tile mode from 2D/3D to 1D in following conditions
   //
   switch (expTileMode)
   {
      case ADDR_TM_2D_TILED_THIN1: //fall through
   case ADDR_TM_3D_TILED_THIN1:
   case ADDR_TM_PRT_TILED_THIN1:
   case ADDR_TM_PRT_2D_TILED_THIN1:
   case ADDR_TM_PRT_3D_TILED_THIN1:
         if ((pitch < pitchAlign) ||
      (height < heightAlign) ||
   (interleaveSize > threshold1) ||
      {
         }
   case ADDR_TM_2D_TILED_THICK: //fall through
   case ADDR_TM_3D_TILED_THICK:
   case ADDR_TM_2D_TILED_XTHICK:
   case ADDR_TM_3D_TILED_XTHICK:
   case ADDR_TM_PRT_TILED_THICK:
   case ADDR_TM_PRT_2D_TILED_THICK:
   case ADDR_TM_PRT_3D_TILED_THICK:
         if ((pitch < pitchAlign) ||
         {
         }
   default:
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlGetAlignmentInfoMacroTiled
   *   @brief
   *       Get alignment info for giving tile mode
   *   @return
   *       TRUE if getting alignment is OK
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::HwlGetAlignmentInfoMacroTiled(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT* pIn,             ///< [in] create surface info
   UINT_32*                               pPitchAlign,     ///< [out] pitch alignment
   UINT_32*                               pHeightAlign,    ///< [out] height alignment
   UINT_32*                               pSizeAlign       ///< [out] size alignment
      {
                                 ADDR_ASSERT(pIn->pTileInfo);
   ADDR_TILEINFO tileInfo = *pIn->pTileInfo;
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT out = {0};
            if (UseTileIndex(pIn->tileIndex))
   {
      out.tileIndex = pIn->tileIndex;
               HwlSetupTileInfo(pIn->tileMode,
                     pIn->flags,
   pIn->bpp,
   pIn->width,
   pIn->height,
   numSamples,
            valid = ComputeSurfaceAlignmentsMacroTiled(pIn->tileMode,
                                    if (valid)
   {
      *pPitchAlign  = out.pitchAlign;
   *pHeightAlign = out.heightAlign;
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlDegradeThickTileMode
   *
   *   @brief
   *       Degrades valid tile mode for thick modes if needed
   *
   *   @return
   *       Suitable tile mode
   ****************************************************************************************************
   */
   AddrTileMode EgBasedLib::HwlDegradeThickTileMode(
      AddrTileMode        baseTileMode,   ///< [in] base tile mode
   UINT_32             numSlices,      ///< [in] current number of slices
   UINT_32*            pBytesPerTile   ///< [in,out] pointer to bytes per slice
      {
      ADDR_ASSERT(numSlices < Thickness(baseTileMode));
   // if pBytesPerTile is NULL, this is a don't-care....
            AddrTileMode expTileMode = baseTileMode;
   switch (baseTileMode)
   {
      case ADDR_TM_1D_TILED_THICK:
         expTileMode = ADDR_TM_1D_TILED_THIN1;
   bytesPerTile >>= 2;
   case ADDR_TM_2D_TILED_THICK:
         expTileMode = ADDR_TM_2D_TILED_THIN1;
   bytesPerTile >>= 2;
   case ADDR_TM_3D_TILED_THICK:
         expTileMode = ADDR_TM_3D_TILED_THIN1;
   bytesPerTile >>= 2;
   case ADDR_TM_2D_TILED_XTHICK:
         if (numSlices < ThickTileThickness)
   {
      expTileMode = ADDR_TM_2D_TILED_THIN1;
      }
   else
   {
      expTileMode = ADDR_TM_2D_TILED_THICK;
      }
   case ADDR_TM_3D_TILED_XTHICK:
         if (numSlices < ThickTileThickness)
   {
      expTileMode = ADDR_TM_3D_TILED_THIN1;
      }
   else
   {
      expTileMode = ADDR_TM_3D_TILED_THICK;
      }
   default:
                     if (pBytesPerTile != NULL)
   {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::DispatchComputeSurfaceAddrFromCoord
   *
   *   @brief
   *       Compute surface address from given coord (x, y, slice,sample)
   *
   *   @return
   *       Address in bytes
   ****************************************************************************************************
   */
   UINT_64 EgBasedLib::DispatchComputeSurfaceAddrFromCoord(
      const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      UINT_32             x                  = pIn->x;
   UINT_32             y                  = pIn->y;
   UINT_32             slice              = pIn->slice;
   UINT_32             sample             = pIn->sample;
   UINT_32             bpp                = pIn->bpp;
   UINT_32             pitch              = pIn->pitch;
   UINT_32             height             = pIn->height;
   UINT_32             numSlices          = pIn->numSlices;
   UINT_32             numSamples         = ((pIn->numSamples == 0) ? 1 : pIn->numSamples);
   UINT_32             numFrags           = ((pIn->numFrags == 0) ? numSamples : pIn->numFrags);
   AddrTileMode        tileMode           = pIn->tileMode;
   AddrTileType        microTileType      = pIn->tileType;
   BOOL_32             ignoreSE           = pIn->ignoreSE;
   BOOL_32             isDepthSampleOrder = pIn->isDepth;
            UINT_32*            pBitPosition       = &pOut->bitPosition;
            // ADDR_DEPTH_SAMPLE_ORDER = non-disp + depth-sample-order
   if (microTileType == ADDR_DEPTH_SAMPLE_ORDER)
   {
                  if (m_chipFamily >= ADDR_CHIP_FAMILY_NI)
   {
      if (numFrags != numSamples)
   {
         numSamples = numFrags;
            /// @note
   /// 128 bit/thick tiled surface doesn't support display tiling and
   /// mipmap chain must have the same tileType, so please fill tileType correctly
   if (IsLinear(pIn->tileMode) == FALSE)
   {
         if (bpp >= 128 || Thickness(tileMode) > 1)
   {
                     switch (tileMode)
   {
      case ADDR_TM_LINEAR_GENERAL://fall through
   case ADDR_TM_LINEAR_ALIGNED:
         addr = ComputeSurfaceAddrFromCoordLinear(x,
                                          y,
      case ADDR_TM_1D_TILED_THIN1://fall through
   case ADDR_TM_1D_TILED_THICK:
         addr = ComputeSurfaceAddrFromCoordMicroTiled(x,
                                                y,
   slice,
   sample,
      case ADDR_TM_2D_TILED_THIN1:    //fall through
   case ADDR_TM_2D_TILED_THICK:    //fall through
   case ADDR_TM_3D_TILED_THIN1:    //fall through
   case ADDR_TM_3D_TILED_THICK:    //fall through
   case ADDR_TM_2D_TILED_XTHICK:   //fall through
   case ADDR_TM_3D_TILED_XTHICK:   //fall through
   case ADDR_TM_PRT_TILED_THIN1:   //fall through
   case ADDR_TM_PRT_2D_TILED_THIN1://fall through
   case ADDR_TM_PRT_3D_TILED_THIN1://fall through
   case ADDR_TM_PRT_TILED_THICK:   //fall through
   case ADDR_TM_PRT_2D_TILED_THICK://fall through
   case ADDR_TM_PRT_3D_TILED_THICK:
                        if (m_configFlags.useCombinedSwizzle)
   {
      ExtractBankPipeSwizzle(pIn->tileSwizzle, pIn->pTileInfo,
      }
   else
   {
                        addr = ComputeSurfaceAddrFromCoordMacroTiled(x,
                                                y,
   slice,
   sample,
   bpp,
   pitch,
   height,
   numSamples,
      default:
         addr = 0;
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeMacroTileEquation
   *
   *   @brief
   *       Computes the address equation in macro tile
   *   @return
   *       If equation can be computed
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::ComputeMacroTileEquation(
      UINT_32             log2BytesPP,            ///< [in] log2 of bytes per pixel
   AddrTileMode        tileMode,               ///< [in] tile mode
   AddrTileType        microTileType,          ///< [in] micro tiling type
   ADDR_TILEINFO*      pTileInfo,              ///< [in] bank structure
   ADDR_EQUATION*      pEquation               ///< [out] Equation for addressing in macro tile
      {
               // Element equation within a tile
            if (retCode == ADDR_OK)
   {
      // Tile equesiton with signle pipe bank
   UINT_32 numPipes              = HwlGetPipes(pTileInfo);
            for (UINT_32 i = 0; i < Log2(pTileInfo->bankWidth); i++)
   {
         pEquation->addr[pEquation->numBits].valid = 1;
   pEquation->addr[pEquation->numBits].channel = 0;
   pEquation->addr[pEquation->numBits].index = i + log2BytesPP + 3 + numPipeBits;
            for (UINT_32 i = 0; i < Log2(pTileInfo->bankHeight); i++)
   {
         pEquation->addr[pEquation->numBits].valid = 1;
   pEquation->addr[pEquation->numBits].channel = 1;
   pEquation->addr[pEquation->numBits].index = i + 3;
            ADDR_EQUATION equation;
            UINT_32 thresholdX = 32;
            if (IsPrtNoRotationTileMode(tileMode))
   {
         UINT_32 macroTilePitch  =
         UINT_32 macroTileHeight =
      (MicroTileHeight * pTileInfo->bankHeight * pTileInfo->banks) /
      thresholdX = Log2(macroTilePitch);
            // Pipe equation
            if (retCode == ADDR_OK)
   {
                  if (pEquation->numBits > pipeBitStart)
                     for (UINT_32 i = 0; i < numLeftShift; i++)
   {
      pEquation->addr[pEquation->numBits + equation.numBits - i - 1] =
         pEquation->xor1[pEquation->numBits + equation.numBits - i - 1] =
         pEquation->xor2[pEquation->numBits + equation.numBits - i - 1] =
                  for (UINT_32 i = 0; i < equation.numBits; i++)
   {
      pEquation->addr[pipeBitStart + i] = equation.addr[i];
   pEquation->xor1[pipeBitStart + i] = equation.xor1[i];
                                                   if (retCode == ADDR_OK)
                                             for (UINT_32 i = 0; i < numLeftShift; i++)
   {
         pEquation->addr[pEquation->numBits + equation.numBits - i - 1] =
         pEquation->xor1[pEquation->numBits + equation.numBits - i - 1] =
                           for (UINT_32 i = 0; i < equation.numBits; i++)
   {
      pEquation->addr[bankBitStart + i] = equation.addr[i];
   pEquation->xor1[bankBitStart + i] = equation.xor1[i];
                                       }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceAddrFromCoordMicroTiled
   *
   *   @brief
   *       Computes the surface address and bit position from a
   *       coordinate for 2D tilied (macro tiled)
   *   @return
   *       The byte address
   ****************************************************************************************************
   */
   UINT_64 EgBasedLib::ComputeSurfaceAddrFromCoordMacroTiled(
      UINT_32             x,                      ///< [in] x coordinate
   UINT_32             y,                      ///< [in] y coordinate
   UINT_32             slice,                  ///< [in] slice index
   UINT_32             sample,                 ///< [in] sample index
   UINT_32             bpp,                    ///< [in] bits per pixel
   UINT_32             pitch,                  ///< [in] surface pitch, in pixels
   UINT_32             height,                 ///< [in] surface height, in pixels
   UINT_32             numSamples,             ///< [in] number of samples
   AddrTileMode        tileMode,               ///< [in] tile mode
   AddrTileType        microTileType,          ///< [in] micro tiling type
   BOOL_32             ignoreSE,               ///< [in] TRUE if shader enginers can be ignored
   BOOL_32             isDepthSampleOrder,     ///< [in] TRUE if it depth sample ordering is used
   UINT_32             pipeSwizzle,            ///< [in] pipe swizzle
   UINT_32             bankSwizzle,            ///< [in] bank swizzle
   ADDR_TILEINFO*      pTileInfo,              ///< [in] bank structure
         UINT_32*            pBitPosition            ///< [out] bit position, e.g. FMT_1 will use this
      {
               UINT_32 microTileBytes;
   UINT_32 microTileBits;
   UINT_32 sampleOffset;
   UINT_32 pixelIndex;
   UINT_32 pixelOffset;
   UINT_32 elementOffset;
   UINT_32 tileSplitSlice;
   UINT_32 pipe;
   UINT_32 bank;
   UINT_64 sliceBytes;
   UINT_64 sliceOffset;
   UINT_32 macroTilePitch;
   UINT_32 macroTileHeight;
   UINT_32 macroTilesPerRow;
   UINT_32 macroTilesPerSlice;
   UINT_64 macroTileBytes;
   UINT_32 macroTileIndexX;
   UINT_32 macroTileIndexY;
   UINT_64 macroTileOffset;
   UINT_64 totalOffset;
   UINT_64 pipeInterleaveMask;
   UINT_64 bankInterleaveMask;
   UINT_64 pipeInterleaveOffset;
   UINT_32 bankInterleaveOffset;
   UINT_64 offset;
   UINT_32 tileRowIndex;
   UINT_32 tileColumnIndex;
   UINT_32 tileIndex;
                     //
   // Compute the number of group, pipe, and bank bits.
   //
   UINT_32 numPipes              = HwlGetPipes(pTileInfo);
   UINT_32 numPipeInterleaveBits = Log2(m_pipeInterleaveBytes);
   UINT_32 numPipeBits           = Log2(numPipes);
   UINT_32 numBankInterleaveBits = Log2(m_bankInterleave);
            //
   // Compute the micro tile size.
   //
            microTileBytes = microTileBits / 8;
   //
   // Compute the pixel index within the micro tile.
   //
   pixelIndex = ComputePixelIndexWithinMicroTile(x,
                                    //
   // Compute the sample offset and pixel offset.
   //
   if (isDepthSampleOrder)
   {
      //
   // For depth surfaces, samples are stored contiguously for each element, so the sample
   // offset is the sample number times the element size.
   //
   sampleOffset = sample * bpp;
      }
   else
   {
      //
   // For color surfaces, all elements for a particular sample are stored contiguously, so
   // the sample offset is the sample number times the micro tile size divided yBit the number
   // of samples.
   //
   sampleOffset = sample * (microTileBits / numSamples);
               //
   // Compute the element offset.
   //
                              //
   // Determine if tiles need to be split across slices.
   //
   // If the size of the micro tile is larger than the tile split size, then the tile will be
   // split across multiple slices.
   //
            if ((microTileBytes > pTileInfo->tileSplitBytes) && (microTileThickness == 1))
               //
   // Compute the number of slices per tile.
   //
            //
   // Compute the tile split slice number for use in rotating the bank.
   //
            //
   // Adjust the element offset to account for the portion of the tile that is being moved to
   // a new slice..
   //
            //
   // Adjust the microTileBytes size to tileSplitBytes size since
   // a new slice..
   //
      }
   else
   {
                  //
   // Compute macro tile pitch and height.
   //
   macroTilePitch  =
         macroTileHeight =
            //
   // Compute the number of bytes per macro tile. Note: bytes of the same bank/pipe actually
   //
   macroTileBytes =
      static_cast<UINT_64>(microTileBytes) *
   (macroTilePitch / MicroTileWidth) * (macroTileHeight / MicroTileHeight) /
         //
   // Compute the number of macro tiles per row.
   //
            //
   // Compute the offset to the macro tile containing the specified coordinate.
   //
   macroTileIndexX = x / macroTilePitch;
   macroTileIndexY = y / macroTileHeight;
            //
   // Compute the number of macro tiles per slice.
   //
            //
   // Compute the slice size.
   //
            //
   // Compute the slice offset.
   //
            //
   // Compute tile offest
   //
   tileRowIndex    = (y / MicroTileHeight) % pTileInfo->bankHeight;
   tileColumnIndex = ((x / MicroTileWidth) / numPipes) % pTileInfo->bankWidth;
   tileIndex        = (tileRowIndex * pTileInfo->bankWidth) + tileColumnIndex;
            //
   // Combine the slice offset and macro tile offset with the pixel and sample offsets, accounting
   // for the pipe and bank bits in the middle of the address.
   //
            //
   // Get the pipe and bank.
            // when the tileMode is PRT type, then adjust x and y coordinates
   if (IsPrtNoRotationTileMode(tileMode))
   {
      x = x % macroTilePitch;
               pipe = ComputePipeFromCoord(x,
                              y,
         bank = ComputeBankFromCoord(x,
                              y,
            //
   // Split the offset to put some bits below the pipe+bank bits and some above.
   //
   pipeInterleaveMask = (1 << numPipeInterleaveBits) - 1;
   bankInterleaveMask = (1 << numBankInterleaveBits) - 1;
   pipeInterleaveOffset = totalOffset & pipeInterleaveMask;
   bankInterleaveOffset = static_cast<UINT_32>((totalOffset >> numPipeInterleaveBits) &
                  //
   // Assemble the address from its components.
   //
   addr  = pipeInterleaveOffset;
   // This is to remove /analyze warnings
   UINT_32 pipeBits            = pipe                 <<  numPipeInterleaveBits;
   UINT_32 bankInterleaveBits  = bankInterleaveOffset << (numPipeInterleaveBits + numPipeBits);
   UINT_32 bankBits            = bank                 << (numPipeInterleaveBits + numPipeBits +
         UINT_64 offsetBits          = offset               << (numPipeInterleaveBits + numPipeBits +
            addr |= pipeBits;
   addr |= bankInterleaveBits;
   addr |= bankBits;
               }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceAddrFromCoordMicroTiled
   *
   *   @brief
   *       Computes the surface address and bit position from a coordinate for 1D tilied
   *       (micro tiled)
   *   @return
   *       The byte address
   ****************************************************************************************************
   */
   UINT_64 EgBasedLib::ComputeSurfaceAddrFromCoordMicroTiled(
      UINT_32             x,                      ///< [in] x coordinate
   UINT_32             y,                      ///< [in] y coordinate
   UINT_32             slice,                  ///< [in] slice index
   UINT_32             sample,                 ///< [in] sample index
   UINT_32             bpp,                    ///< [in] bits per pixel
   UINT_32             pitch,                  ///< [in] pitch, in pixels
   UINT_32             height,                 ///< [in] height, in pixels
   UINT_32             numSamples,             ///< [in] number of samples
   AddrTileMode        tileMode,               ///< [in] tile mode
   AddrTileType        microTileType,          ///< [in] micro tiling type
   BOOL_32             isDepthSampleOrder,     ///< [in] TRUE if depth sample ordering is used
   UINT_32*            pBitPosition            ///< [out] bit position, e.g. FMT_1 will use this
      {
               UINT_32 microTileBytes;
   UINT_64 sliceBytes;
   UINT_32 microTilesPerRow;
   UINT_32 microTileIndexX;
   UINT_32 microTileIndexY;
   UINT_32 microTileIndexZ;
   UINT_64 sliceOffset;
   UINT_64 microTileOffset;
   UINT_32 sampleOffset;
   UINT_32 pixelIndex;
                     //
   // Compute the micro tile size.
   //
            //
   // Compute the slice size.
   //
   sliceBytes =
            //
   // Compute the number of micro tiles per row.
   //
            //
   // Compute the micro tile index.
   //
   microTileIndexX = x     / MicroTileWidth;
   microTileIndexY = y     / MicroTileHeight;
            //
   // Compute the slice offset.
   //
            //
   // Compute the offset to the micro tile containing the specified coordinate.
   //
   microTileOffset = (static_cast<UINT_64>(microTileIndexY) * microTilesPerRow + microTileIndexX) *
            //
   // Compute the pixel index within the micro tile.
   //
   pixelIndex = ComputePixelIndexWithinMicroTile(x,
                                    // Compute the sample offset.
   //
   if (isDepthSampleOrder)
   {
      //
   // For depth surfaces, samples are stored contiguously for each element, so the sample
   // offset is the sample number times the element size.
   //
   sampleOffset = sample * bpp;
      }
   else
   {
      //
   // For color surfaces, all elements for a particular sample are stored contiguously, so
   // the sample offset is the sample number times the micro tile size divided yBit the number
   // of samples.
   //
   sampleOffset = sample * (microTileBytes*8 / numSamples);
               //
   // Compute the bit position of the pixel.  Each element is stored with one bit per sample.
                     *pBitPosition = elemOffset % 8;
            //
   // Combine the slice offset, micro tile offset, sample offset, and pixel offsets.
   //
               }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputePixelCoordFromOffset
   *
   *   @brief
   *       Compute pixel coordinate from offset inside a micro tile
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID EgBasedLib::HwlComputePixelCoordFromOffset(
      UINT_32         offset,             ///< [in] offset inside micro tile in bits
   UINT_32         bpp,                ///< [in] bits per pixel
   UINT_32         numSamples,         ///< [in] number of samples
   AddrTileMode    tileMode,           ///< [in] tile mode
   UINT_32         tileBase,           ///< [in] base offset within a tile
   UINT_32         compBits,           ///< [in] component bits actually needed(for planar surface)
   UINT_32*        pX,                 ///< [out] x coordinate
   UINT_32*        pY,                 ///< [out] y coordinate
   UINT_32*        pSlice,             ///< [out] slice index
   UINT_32*        pSample,            ///< [out] sample index
   AddrTileType    microTileType,      ///< [in] micro tiling type
   BOOL_32         isDepthSampleOrder  ///< [in] TRUE if depth sample order in microtile is used
      {
      UINT_32 x = 0;
   UINT_32 y = 0;
   UINT_32 z = 0;
            // For planar surface, we adjust offset acoording to tile base
   if ((bpp != compBits) && (compBits != 0) && isDepthSampleOrder)
   {
               ADDR_ASSERT(microTileType == ADDR_NON_DISPLAYABLE ||
                        UINT_32 sampleTileBits;
   UINT_32 samplePixelBits;
            if (isDepthSampleOrder)
   {
      samplePixelBits = bpp * numSamples;
   pixelIndex = offset / samplePixelBits;
      }
   else
   {
      sampleTileBits = MicroTilePixels * bpp * thickness;
   *pSample = offset / sampleTileBits;
               if (microTileType != ADDR_THICK)
   {
      if (microTileType == ADDR_DISPLAYABLE) // displayable
   {
         switch (bpp)
   {
      case 8:
      x = pixelIndex & 0x7;
   y = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,3),_BIT(pixelIndex,4));
      case 16:
      x = pixelIndex & 0x7;
   y = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,4),_BIT(pixelIndex,3));
      case 32:
      x = Bits2Number(3, _BIT(pixelIndex,3),_BIT(pixelIndex,1),_BIT(pixelIndex,0));
   y = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,4),_BIT(pixelIndex,2));
      case 64:
      x = Bits2Number(3, _BIT(pixelIndex,3),_BIT(pixelIndex,2),_BIT(pixelIndex,0));
   y = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,4),_BIT(pixelIndex,1));
      case 128:
      x = Bits2Number(3, _BIT(pixelIndex,3),_BIT(pixelIndex,2),_BIT(pixelIndex,1));
   y = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,4),_BIT(pixelIndex,0));
      default:
      }
   else if (microTileType == ADDR_NON_DISPLAYABLE || microTileType == ADDR_DEPTH_SAMPLE_ORDER)
   {
         x = Bits2Number(3, _BIT(pixelIndex,4),_BIT(pixelIndex,2),_BIT(pixelIndex,0));
   }
   else if (microTileType == ADDR_ROTATED)
   {
         /*
                                                   64-Bit Elements
      */
   switch(bpp)
   {
      case 8:
      x = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,3),_BIT(pixelIndex,4));
   y = pixelIndex & 0x7;
      case 16:
      x = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,4),_BIT(pixelIndex,3));
   y = pixelIndex & 0x7;
      case 32:
      x = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,4),_BIT(pixelIndex,2));
   y = Bits2Number(3, _BIT(pixelIndex,3),_BIT(pixelIndex,1),_BIT(pixelIndex,0));
      case 64:
      x = Bits2Number(3, _BIT(pixelIndex,4),_BIT(pixelIndex,3),_BIT(pixelIndex,1));
   y = Bits2Number(3, _BIT(pixelIndex,5),_BIT(pixelIndex,2),_BIT(pixelIndex,0));
      default:
      ADDR_ASSERT_ALWAYS();
            if (thickness > 1) // thick
   {
            }
   else
   {
      ADDR_ASSERT((m_chipFamily >= ADDR_CHIP_FAMILY_CI) && (thickness > 1));
   /*
                                                      The equation to compute the element index for the extra thick tile:
   */
   switch (bpp)
   {
         case 8:
   case 16: // fall-through
      x = Bits2Number(3, _BIT(pixelIndex,6),_BIT(pixelIndex,2),_BIT(pixelIndex,0));
   y = Bits2Number(3, _BIT(pixelIndex,7),_BIT(pixelIndex,3),_BIT(pixelIndex,1));
   z = Bits2Number(2, _BIT(pixelIndex,5),_BIT(pixelIndex,4));
      case 32:
      x = Bits2Number(3, _BIT(pixelIndex,6),_BIT(pixelIndex,2),_BIT(pixelIndex,0));
   y = Bits2Number(3, _BIT(pixelIndex,7),_BIT(pixelIndex,4),_BIT(pixelIndex,1));
   z = Bits2Number(2, _BIT(pixelIndex,5),_BIT(pixelIndex,3));
      case 64:
   case 128: // fall-through
      x = Bits2Number(3, _BIT(pixelIndex,6),_BIT(pixelIndex,3),_BIT(pixelIndex,0));
   y = Bits2Number(3, _BIT(pixelIndex,7),_BIT(pixelIndex,4),_BIT(pixelIndex,1));
   z = Bits2Number(2, _BIT(pixelIndex,5),_BIT(pixelIndex,2));
      default:
                  if (thickness == 8)
   {
                     *pX = x;
   *pY = y;
      }
         /**
   ****************************************************************************************************
   *   EgBasedLib::DispatchComputeSurfaceCoordFromAddrDispatch
   *
   *   @brief
   *       Compute (x,y,slice,sample) coordinates from surface address
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID EgBasedLib::DispatchComputeSurfaceCoordFromAddr(
      const ADDR_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut    ///< [out] output structure
      {
      UINT_64             addr               = pIn->addr;
   UINT_32             bitPosition        = pIn->bitPosition;
   UINT_32             bpp                = pIn->bpp;
   UINT_32             pitch              = pIn->pitch;
   UINT_32             height             = pIn->height;
   UINT_32             numSlices          = pIn->numSlices;
   UINT_32             numSamples         = ((pIn->numSamples == 0) ? 1 : pIn->numSamples);
   UINT_32             numFrags           = ((pIn->numFrags == 0) ? numSamples : pIn->numFrags);
   AddrTileMode        tileMode           = pIn->tileMode;
   UINT_32             tileBase           = pIn->tileBase;
   UINT_32             compBits           = pIn->compBits;
   AddrTileType        microTileType      = pIn->tileType;
   BOOL_32             ignoreSE           = pIn->ignoreSE;
   BOOL_32             isDepthSampleOrder = pIn->isDepth;
            UINT_32*            pX                 = &pOut->x;
   UINT_32*            pY                 = &pOut->y;
   UINT_32*            pSlice             = &pOut->slice;
            if (microTileType == ADDR_DEPTH_SAMPLE_ORDER)
   {
                  if (m_chipFamily >= ADDR_CHIP_FAMILY_NI)
   {
      if (numFrags != numSamples)
   {
                  /// @note
   /// 128 bit/thick tiled surface doesn't support display tiling and
   /// mipmap chain must have the same tileType, so please fill tileType correctly
   if (IsLinear(pIn->tileMode) == FALSE)
   {
         if (bpp >= 128 || Thickness(tileMode) > 1)
   {
                     switch (tileMode)
   {
      case ADDR_TM_LINEAR_GENERAL://fall through
   case ADDR_TM_LINEAR_ALIGNED:
         ComputeSurfaceCoordFromAddrLinear(addr,
                                    bitPosition,
   bpp,
   pitch,
      case ADDR_TM_1D_TILED_THIN1://fall through
   case ADDR_TM_1D_TILED_THICK:
         ComputeSurfaceCoordFromAddrMicroTiled(addr,
                                       bitPosition,
   bpp,
   pitch,
   height,
   numSamples,
   tileMode,
   tileBase,
   compBits,
   case ADDR_TM_2D_TILED_THIN1:    //fall through
   case ADDR_TM_2D_TILED_THICK:    //fall through
   case ADDR_TM_3D_TILED_THIN1:    //fall through
   case ADDR_TM_3D_TILED_THICK:    //fall through
   case ADDR_TM_2D_TILED_XTHICK:   //fall through
   case ADDR_TM_3D_TILED_XTHICK:   //fall through
   case ADDR_TM_PRT_TILED_THIN1:   //fall through
   case ADDR_TM_PRT_2D_TILED_THIN1://fall through
   case ADDR_TM_PRT_3D_TILED_THIN1://fall through
   case ADDR_TM_PRT_TILED_THICK:   //fall through
   case ADDR_TM_PRT_2D_TILED_THICK://fall through
   case ADDR_TM_PRT_3D_TILED_THICK:
                        if (m_configFlags.useCombinedSwizzle)
   {
      ExtractBankPipeSwizzle(pIn->tileSwizzle, pIn->pTileInfo,
      }
   else
   {
                        ComputeSurfaceCoordFromAddrMacroTiled(addr,
                                       bitPosition,
   bpp,
   pitch,
   height,
   numSamples,
   tileMode,
   tileBase,
   compBits,
   microTileType,
   ignoreSE,
   isDepthSampleOrder,
   pipeSwizzle,
   default:
         }
         /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceCoordFromAddrMacroTiled
   *
   *   @brief
   *       Compute surface coordinates from address for macro tiled surface
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID EgBasedLib::ComputeSurfaceCoordFromAddrMacroTiled(
      UINT_64             addr,               ///< [in] byte address
   UINT_32             bitPosition,        ///< [in] bit position
   UINT_32             bpp,                ///< [in] bits per pixel
   UINT_32             pitch,              ///< [in] pitch in pixels
   UINT_32             height,             ///< [in] height in pixels
   UINT_32             numSamples,         ///< [in] number of samples
   AddrTileMode        tileMode,           ///< [in] tile mode
   UINT_32             tileBase,           ///< [in] tile base offset
   UINT_32             compBits,           ///< [in] component bits (for planar surface)
   AddrTileType        microTileType,      ///< [in] micro tiling type
   BOOL_32             ignoreSE,           ///< [in] TRUE if shader engines can be ignored
   BOOL_32             isDepthSampleOrder, ///< [in] TRUE if depth sample order is used
   UINT_32             pipeSwizzle,        ///< [in] pipe swizzle
   UINT_32             bankSwizzle,        ///< [in] bank swizzle
   ADDR_TILEINFO*      pTileInfo,          ///< [in] bank structure.
         UINT_32*            pX,                 ///< [out] X coord
   UINT_32*            pY,                 ///< [out] Y coord
   UINT_32*            pSlice,             ///< [out] slice index
   UINT_32*            pSample             ///< [out] sample index
      {
      UINT_32 mx;
   UINT_32 my;
   UINT_64 tileBits;
   UINT_64 macroTileBits;
   UINT_32 slices;
   UINT_32 tileSlices;
   UINT_64 elementOffset;
   UINT_64 macroTileIndex;
   UINT_32 tileIndex;
               UINT_32 bank;
   UINT_32 pipe;
   UINT_32 groupBits = m_pipeInterleaveBytes << 3;
   UINT_32 pipes = HwlGetPipes(pTileInfo);
                              //
   // remove bits for bank and pipe
   //
   totalOffset = (addrBits % groupBits) +
      (((addrBits / groupBits / pipes) % bankInterleave) * groupBits) +
                           UINT_32 microTileBytes = BITS_TO_BYTES(microTileBits);
   //
   // Determine if tiles need to be split across slices.
   //
   // If the size of the micro tile is larger than the tile split size, then the tile will be
   // split across multiple slices.
   //
            if ((microTileBytes > pTileInfo->tileSplitBytes) && (microTileThickness == 1))
               //
   // Compute the number of slices per tile.
   //
                        // in micro tiles because not MicroTileWidth timed.
   UINT_32 macroWidth  = pTileInfo->bankWidth * pipes * pTileInfo->macroAspectRatio;
   // in micro tiles as well
                                       // pitchMacros * height / heightMacros;  macroTilesPerSlice == _State->SliceMacros
   UINT_32 macroTilesPerSlice = (pitch / (macroWidth * MicroTileWidth)) * height /
                              //
   // calculate element offset and x[2:0], y[2:0], z[1:0] for thick
   //
            elementOffset  = tileSlices * tileBits;
                     HwlComputePixelCoordFromOffset(static_cast<UINT_32>(elementOffset),
                                 bpp,
   numSamples,
   tileMode,
   tileBase,
   compBits,
            macroTileIndex = macroTileIndex % macroTilesPerSlice;
   *pY += static_cast<UINT_32>(macroTileIndex / pitchInMacroTiles * macroHeight * MicroTileHeight);
                              my = (tileIndex / pTileInfo->bankWidth) % pTileInfo->bankHeight * MicroTileHeight;
            *pY += my;
            bank = ComputeBankFromAddr(addr, banks, pipes);
            HwlComputeSurfaceCoord2DFromBankPipe(tileMode,
                                       pX,
   pY,
   *pSlice,
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSurfaceCoord2DFromBankPipe
   *
   *   @brief
   *       Compute surface x,y coordinates from bank/pipe info
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID EgBasedLib::ComputeSurfaceCoord2DFromBankPipe(
      AddrTileMode        tileMode,   ///< [in] tile mode
   UINT_32             x,          ///< [in] x coordinate
   UINT_32             y,          ///< [in] y coordinate
   UINT_32             slice,      ///< [in] slice index
   UINT_32             bank,       ///< [in] bank number
   UINT_32             pipe,       ///< [in] pipe number
   UINT_32             bankSwizzle,///< [in] bank swizzle
   UINT_32             pipeSwizzle,///< [in] pipe swizzle
   UINT_32             tileSlices, ///< [in] slices in a micro tile
   ADDR_TILEINFO*      pTileInfo,  ///< [in] bank structure. **All fields to be valid on entry**
   CoordFromBankPipe*  pOutput     ///< [out] pointer to extracted x/y bits
      {
      UINT_32 yBit3 = 0;
   UINT_32 yBit4 = 0;
   UINT_32 yBit5 = 0;
            UINT_32 xBit3 = 0;
   UINT_32 xBit4 = 0;
                              UINT_32 bankRotation = ComputeBankRotation(tileMode,
                     UINT_32 xBit = x / (MicroTileWidth * pTileInfo->bankWidth * numPipes);
                     switch (tileMode)
   {
      case ADDR_TM_2D_TILED_THIN1:  //fall through
   case ADDR_TM_2D_TILED_THICK:  //fall through
   case ADDR_TM_2D_TILED_XTHICK: //fall through
   case ADDR_TM_3D_TILED_THIN1:  //fall through
   case ADDR_TM_3D_TILED_THICK:  //fall through
   case ADDR_TM_3D_TILED_XTHICK:
         tileSplitRotation = ((pTileInfo->banks / 2) + 1);
   default:
                              bank ^= tileSplitRotation * tileSlices;
   if (pipeRotation == 0)
   {
      bank ^= bankRotation * (slice / microTileThickness) + bankSwizzle;
   bank %= pTileInfo->banks;
      }
   else
   {
      bank ^= bankRotation * (slice / microTileThickness) / numPipes + bankSwizzle;
   bank %= pTileInfo->banks;
               if (pTileInfo->macroAspectRatio == 1)
   {
      switch (pTileInfo->banks)
   {
         case 2:
      yBit3 = _BIT(bank, 0) ^ _BIT(xBit,0);
      case 4:
      yBit4 = _BIT(bank, 0) ^ _BIT(xBit,0);
   yBit3 = _BIT(bank, 1) ^ _BIT(xBit,1);
      case 8:
      yBit3 = _BIT(bank, 2) ^ _BIT(xBit,2);
   yBit5 = _BIT(bank, 0) ^ _BIT(xBit,0);
   yBit4 = _BIT(bank, 1) ^ _BIT(xBit,1) ^ yBit5;
      case 16:
      yBit3 = _BIT(bank, 3) ^ _BIT(xBit, 3);
   yBit4 = _BIT(bank, 2) ^ _BIT(xBit, 2);
   yBit6 = _BIT(bank, 0) ^ _BIT(xBit, 0);
   yBit5 = _BIT(bank, 1) ^ _BIT(xBit, 1) ^ yBit6;
      default:
         }
   else if (pTileInfo->macroAspectRatio == 2)
   {
      switch (pTileInfo->banks)
   {
         case 2: //xBit3 = yBit3^b0
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit,0);
      case 4: //xBit3=yBit4^b0; yBit3=xBit4^b1
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit,1);
   yBit3 = _BIT(bank, 1) ^ _BIT(xBit,1);
      case 8: //xBit4, xBit5, yBit5 are known
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit,2);
   yBit3 = _BIT(bank, 2) ^ _BIT(xBit,2);
   yBit4 = _BIT(bank, 1) ^ _BIT(xBit,1) ^ _BIT(yBit, 2);
      case 16://x4,x5,x6,y6 are known
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit, 3); //x3 = y6 ^ b0
   yBit3 = _BIT(bank, 3) ^ _BIT(xBit, 3); //y3 = x6 ^ b3
   yBit4 = _BIT(bank, 2) ^ _BIT(xBit, 2); //y4 = x5 ^ b2
   yBit5 = _BIT(bank, 1) ^ _BIT(xBit, 1) ^ _BIT(yBit, 3); //y5=x4^y6^b1
      default:
      }
   else if (pTileInfo->macroAspectRatio == 4)
   {
      switch (pTileInfo->banks)
   {
         case 4: //yBit3, yBit4
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit,1);
   xBit4 = _BIT(bank, 1) ^ _BIT(yBit,0);
      case 8: //xBit5, yBit4, yBit5
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit,2);
   yBit3 = _BIT(bank, 2) ^ _BIT(xBit,2);
   xBit4 = _BIT(bank, 1) ^ _BIT(yBit,1) ^  _BIT(yBit,2);
      case 16: //xBit5, xBit6, yBit5, yBit6
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit, 3);//x3 = b0 ^ y6
   xBit4 = _BIT(bank, 1) ^ _BIT(yBit, 2) ^ _BIT(yBit, 3);//x4 = b1 ^ y5 ^ y6;
   yBit3 = _BIT(bank, 3) ^ _BIT(xBit, 3); //y3 = b3 ^ x6;
   yBit4 = _BIT(bank, 2) ^ _BIT(xBit, 2); //y4 = b2 ^ x5;
      default:
      }
   else if (pTileInfo->macroAspectRatio == 8)
   {
      switch (pTileInfo->banks)
   {
         case 8: //yBit3, yBit4, yBit5
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit,2); //x3 = b0 ^ y5;
   xBit4 = _BIT(bank, 1) ^ _BIT(yBit,1) ^ _BIT(yBit, 2);//x4 = b1 ^ y4 ^ y5;
   xBit5 = _BIT(bank, 2) ^ _BIT(yBit,0);
      case 16: //xBit6, yBit4, yBit5, yBit6
      xBit3 = _BIT(bank, 0) ^ _BIT(yBit, 3);//x3 = y6 ^ b0
   xBit4 = _BIT(bank, 1) ^ _BIT(yBit, 2) ^ _BIT(yBit, 3);//x4 = y5 ^ y6 ^ b1
   xBit5 = _BIT(bank, 2) ^ _BIT(yBit, 1);//x5 = y4 ^ b2
   yBit3 = _BIT(bank, 3) ^ _BIT(xBit, 3); //y3 = x6 ^ b3
      default:
               pOutput->xBits = xBit;
            pOutput->xBit3 = xBit3;
   pOutput->xBit4 = xBit4;
   pOutput->xBit5 = xBit5;
   pOutput->yBit3 = yBit3;
   pOutput->yBit4 = yBit4;
   pOutput->yBit5 = yBit5;
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlExtractBankPipeSwizzle
   *   @brief
   *       Entry of EgBasedLib ExtractBankPipeSwizzle
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlExtractBankPipeSwizzle(
      const ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT*  pIn,   ///< [in] input structure
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT*       pOut   ///< [out] output structure
      {
      ExtractBankPipeSwizzle(pIn->base256b,
                           }
         /**
   ****************************************************************************************************
   *   EgBasedLib::HwlCombineBankPipeSwizzle
   *   @brief
   *       Combine bank/pipe swizzle
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlCombineBankPipeSwizzle(
      UINT_32         bankSwizzle,    ///< [in] bank swizzle
   UINT_32         pipeSwizzle,    ///< [in] pipe swizzle
   ADDR_TILEINFO*  pTileInfo,      ///< [in] tile info
   UINT_64         baseAddr,       ///< [in] base address
   UINT_32*        pTileSwizzle    ///< [out] combined swizzle
      {
               if (pTileSwizzle)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeBaseSwizzle
   *   @brief
   *       Compute base swizzle
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeBaseSwizzle(
      const ADDR_COMPUTE_BASE_SWIZZLE_INPUT* pIn,
   ADDR_COMPUTE_BASE_SWIZZLE_OUTPUT* pOut
      {
      UINT_32 bankSwizzle = 0;
   UINT_32 pipeSwizzle = 0;
            ADDR_ASSERT(IsMacroTiled(pIn->tileMode));
            /// This is a legacy misreading of h/w doc, use it as it doesn't hurt.
   static const UINT_8 bankRotationArray[4][16] = {
      { 0, 0,  0, 0,  0, 0,  0, 0, 0,  0, 0,  0, 0,  0, 0, 0 }, // ADDR_SURF_2_BANK
   { 0, 1,  2, 3,  0, 0,  0, 0, 0,  0, 0,  0, 0,  0, 0, 0 }, // ADDR_SURF_4_BANK
   { 0, 3,  6, 1,  4, 7,  2, 5, 0,  0, 0,  0, 0,  0, 0, 0 }, // ADDR_SURF_8_BANK
               UINT_32 pipes = HwlGetPipes(pTileInfo);
   UINT_32 banks = pTileInfo ? pTileInfo->banks : 2;
            // Uses less bank swizzle bits
   if (pIn->option.reduceBankBit && banks > 2)
   {
                  switch (banks)
   {
      case 2:
         hwNumBanks = 0;
   case 4:
         hwNumBanks = 1;
   case 8:
         hwNumBanks = 2;
   case 16:
         hwNumBanks = 3;
   default:
         ADDR_ASSERT_ALWAYS();
               if (pIn->option.genOption == ADDR_SWIZZLE_GEN_LINEAR)
   {
         }
   else // (pIn->option.genOption == ADDR_SWIZZLE_GEN_DEFAULT)
   {
                  if (IsMacro3dTiled(pIn->tileMode))
   {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ExtractBankPipeSwizzle
   *   @brief
   *       Extract bank/pipe swizzle from base256b
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID EgBasedLib::ExtractBankPipeSwizzle(
      UINT_32         base256b,       ///< [in] input base256b register value
   ADDR_TILEINFO*  pTileInfo,      ///< [in] 2D tile parameters. Client must provide all data
   UINT_32*        pBankSwizzle,   ///< [out] bank swizzle
   UINT_32*        pPipeSwizzle    ///< [out] pipe swizzle
      {
      UINT_32 bankSwizzle = 0;
            if (base256b != 0)
   {
      UINT_32 numPipes        = HwlGetPipes(pTileInfo);
   UINT_32 bankBits        = QLog2(pTileInfo->banks);
   UINT_32 pipeBits        = QLog2(numPipes);
   UINT_32 groupBytes      = m_pipeInterleaveBytes;
            pipeSwizzle =
            bankSwizzle =
               *pPipeSwizzle = pipeSwizzle;
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::GetBankPipeSwizzle
   *   @brief
   *       Combine bank/pipe swizzle
   *   @return
   *       Base256b bits (only filled bank/pipe bits)
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::GetBankPipeSwizzle(
      UINT_32         bankSwizzle,    ///< [in] bank swizzle
   UINT_32         pipeSwizzle,    ///< [in] pipe swizzle
   UINT_64         baseAddr,       ///< [in] base address
   ADDR_TILEINFO*  pTileInfo       ///< [in] tile info
      {
      UINT_32 pipeBits = QLog2(HwlGetPipes(pTileInfo));
   UINT_32 bankInterleaveBits = QLog2(m_bankInterleave);
            baseAddr ^= tileSwizzle * m_pipeInterleaveBytes;
               }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeSliceTileSwizzle
   *   @brief
   *       Compute cubemap/3d texture faces/slices tile swizzle
   *   @return
   *       Tile swizzle
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputeSliceTileSwizzle(
      AddrTileMode        tileMode,       ///< [in] Tile mode
   UINT_32             baseSwizzle,    ///< [in] Base swizzle
   UINT_32             slice,          ///< [in] Slice index, Cubemap face index, 0 means +X
   UINT_64             baseAddr,       ///< [in] Base address
   ADDR_TILEINFO* pTileInfo       ///< [in] Bank structure
      {
               if (IsMacroTiled(tileMode)) // Swizzle only for macro tile mode
   {
               UINT_32 numPipes = HwlGetPipes(pTileInfo);
            UINT_32 pipeRotation;
            UINT_32 bankSwizzle = 0;
            pipeRotation = ComputePipeRotation(tileMode, numPipes);
            if (baseSwizzle != 0)
   {
         ExtractBankPipeSwizzle(baseSwizzle,
                        if (pipeRotation == 0) //2D mode
   {
         bankSwizzle += firstSlice * bankRotation;
   }
   else //3D mode
   {
         pipeSwizzle += firstSlice * pipeRotation;
   pipeSwizzle %= numPipes;
   bankSwizzle += firstSlice * bankRotation / numPipes;
            tileSwizzle = GetBankPipeSwizzle(bankSwizzle,
                              }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeQbStereoRightSwizzle
   *
   *   @brief
   *       Compute right eye swizzle
   *   @return
   *       swizzle
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::HwlComputeQbStereoRightSwizzle(
      ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pInfo  ///< [in] Surface info, must be valid
      {
      UINT_32 bankBits    = 0;
            // The assumption is default swizzle for left eye is 0
   if (IsMacroTiled(pInfo->tileMode) && pInfo->pStereoInfo && pInfo->pTileInfo)
   {
      bankBits = ComputeBankFromCoord(0, pInfo->height, 0,
            if (bankBits)
   {
                        }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeBankFromCoord
   *
   *   @brief
   *       Compute bank number from coordinates
   *   @return
   *       Bank number
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputeBankFromCoord(
      UINT_32         x,              ///< [in] x coordinate
   UINT_32         y,              ///< [in] y coordinate
   UINT_32         slice,          ///< [in] slice index
   AddrTileMode    tileMode,       ///< [in] tile mode
   UINT_32         bankSwizzle,    ///< [in] bank swizzle
   UINT_32         tileSplitSlice, ///< [in] If the size of the pixel offset is larger than the
                     ADDR_TILEINFO*  pTileInfo       ///< [in] tile info
      {
      UINT_32 pipes = HwlGetPipes(pTileInfo);
   UINT_32 bankBit0 = 0;
   UINT_32 bankBit1 = 0;
   UINT_32 bankBit2 = 0;
   UINT_32 bankBit3 = 0;
   UINT_32 sliceRotation;
   UINT_32 tileSplitRotation;
   UINT_32 bank;
   UINT_32 numBanks    = pTileInfo->banks;
   UINT_32 bankWidth   = pTileInfo->bankWidth;
            UINT_32 tx = x / MicroTileWidth / (bankWidth * pipes);
            UINT_32 x3 = _BIT(tx,0);
   UINT_32 x4 = _BIT(tx,1);
   UINT_32 x5 = _BIT(tx,2);
   UINT_32 x6 = _BIT(tx,3);
   UINT_32 y3 = _BIT(ty,0);
   UINT_32 y4 = _BIT(ty,1);
   UINT_32 y5 = _BIT(ty,2);
            switch (numBanks)
   {
      case 16:
         bankBit0 = x3 ^ y6;
   bankBit1 = x4 ^ y5 ^ y6;
   bankBit2 = x5 ^ y4;
   bankBit3 = x6 ^ y3;
   case 8:
         bankBit0 = x3 ^ y5;
   bankBit1 = x4 ^ y4 ^ y5;
   bankBit2 = x5 ^ y3;
   case 4:
         bankBit0 = x3 ^ y4;
   bankBit1 = x4 ^ y3;
   case 2:
         bankBit0 = x3 ^ y3;
   default:
                                       bank = HwlPreAdjustBank((x / MicroTileWidth), bank, pTileInfo);
   //
   // Compute bank rotation for the slice.
   //
            switch (tileMode)
   {
      case ADDR_TM_2D_TILED_THIN1:  // fall through
   case ADDR_TM_2D_TILED_THICK:  // fall through
   case ADDR_TM_2D_TILED_XTHICK:
         sliceRotation = ((numBanks / 2) - 1) * (slice / microTileThickness);
   case ADDR_TM_3D_TILED_THIN1:  // fall through
   case ADDR_TM_3D_TILED_THICK:  // fall through
   case ADDR_TM_3D_TILED_XTHICK:
         sliceRotation =
         default:
                        //
   // Compute bank rotation for the tile split slice.
   //
   // The sample slice will be non-zero if samples must be split across multiple slices.
   // This situation arises when the micro tile size multiplied yBit the number of samples exceeds
   // the split size (set in GB_ADDR_CONFIG).
   //
   switch (tileMode)
   {
      case ADDR_TM_2D_TILED_THIN1: //fall through
   case ADDR_TM_3D_TILED_THIN1: //fall through
   case ADDR_TM_PRT_2D_TILED_THIN1: //fall through
   case ADDR_TM_PRT_3D_TILED_THIN1: //fall through
         tileSplitRotation = ((numBanks / 2) + 1) * tileSplitSlice;
   default:
                     //
   // Apply bank rotation for the slice and tile split slice.
   //
   bank ^= bankSwizzle + sliceRotation;
                        }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeBankFromAddr
   *
   *   @brief
   *       Compute the bank number from an address
   *   @return
   *       Bank number
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputeBankFromAddr(
      UINT_64 addr,       ///< [in] address
   UINT_32 numBanks,   ///< [in] number of banks
   UINT_32 numPipes    ///< [in] number of pipes
      {
               //
   // The LSBs of the address are arranged as follows:
   //   bank | bankInterleave | pipe | pipeInterleave
   //
   // To get the bank number, shift off the pipe interleave, pipe, and bank interlave bits and
   // mask the bank bits.
   //
   bank = static_cast<UINT_32>(
      (addr >> Log2(m_pipeInterleaveBytes * numPipes * m_bankInterleave)) &
   (numBanks - 1)
            }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputePipeRotation
   *
   *   @brief
   *       Compute pipe rotation value
   *   @return
   *       Pipe rotation
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputePipeRotation(
      AddrTileMode tileMode,  ///< [in] tile mode
   UINT_32      numPipes   ///< [in] number of pipes
      {
               switch (tileMode)
   {
      case ADDR_TM_3D_TILED_THIN1:        //fall through
   case ADDR_TM_3D_TILED_THICK:        //fall through
   case ADDR_TM_3D_TILED_XTHICK:       //fall through
   case ADDR_TM_PRT_3D_TILED_THIN1:    //fall through
   case ADDR_TM_PRT_3D_TILED_THICK:
         rotation = (numPipes < 4) ? 1 : (numPipes / 2 - 1);
   default:
                  }
            /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeBankRotation
   *
   *   @brief
   *       Compute bank rotation value
   *   @return
   *       Bank rotation
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputeBankRotation(
      AddrTileMode tileMode,  ///< [in] tile mode
   UINT_32      numBanks,  ///< [in] number of banks
   UINT_32      numPipes   ///< [in] number of pipes
      {
               switch (tileMode)
   {
      case ADDR_TM_2D_TILED_THIN1: // fall through
   case ADDR_TM_2D_TILED_THICK: // fall through
   case ADDR_TM_2D_TILED_XTHICK:
   case ADDR_TM_PRT_2D_TILED_THIN1:
   case ADDR_TM_PRT_2D_TILED_THICK:
         // Rotate banks per Z-slice yBit 1 for 4-bank or 3 for 8-bank
   rotation =  numBanks / 2 - 1;
   case ADDR_TM_3D_TILED_THIN1: // fall through
   case ADDR_TM_3D_TILED_THICK: // fall through
   case ADDR_TM_3D_TILED_XTHICK:
   case ADDR_TM_PRT_3D_TILED_THIN1:
   case ADDR_TM_PRT_3D_TILED_THICK:
         rotation = (numPipes < 4) ? 1 : (numPipes / 2 - 1);    // rotate pipes & banks
   default:
                  }
         /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeHtileBytes
   *
   *   @brief
   *       Compute htile size in bytes
   *
   *   @return
   *       Htile size in bytes
   ****************************************************************************************************
   */
   UINT_64 EgBasedLib::ComputeHtileBytes(
      UINT_32 pitch,        ///< [in] pitch
   UINT_32 height,       ///< [in] height
   UINT_32 bpp,          ///< [in] bits per pixel
   BOOL_32 isLinear,     ///< [in] if it is linear mode
   UINT_32 numSlices,    ///< [in] number of slices
   UINT_64* sliceBytes,  ///< [out] bytes per slice
   UINT_32 baseAlign     ///< [in] base alignments
      {
                                 if (m_configFlags.useHtileSliceAlign)
   {
      // Align the sliceSize to htilecachelinesize * pipes at first
   *sliceBytes = PowTwoAlign(*sliceBytes, HtileCacheLineSize * m_pipes);
      }
   else
   {
      // Align the surfSize to htilecachelinesize * pipes at last
   surfBytes  = *sliceBytes * numSlices;
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::DispatchComputeFmaskInfo
   *
   *   @brief
   *       Compute fmask sizes include padded pitch, height, slices, total size in bytes,
   *       meanwhile output suitable tile mode and alignments as well. Results are returned
   *       through output parameters.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::DispatchComputeFmaskInfo(
      const ADDR_COMPUTE_FMASK_INFO_INPUT*    pIn,   ///< [in] input structure
      {
               ADDR_COMPUTE_SURFACE_INFO_INPUT  surfIn     = {0};
            // Setup input structure
   surfIn.tileMode          = pIn->tileMode;
   surfIn.width             = pIn->pitch;
   surfIn.height            = pIn->height;
   surfIn.numSlices         = pIn->numSlices;
   surfIn.pTileInfo         = pIn->pTileInfo;
   surfIn.tileType          = ADDR_NON_DISPLAYABLE;
            // Setup output structure
            // Setup hwl specific fields
                     // ComputeSurfaceInfo needs numSamples in surfOut as surface routines need adjusted numSamples
                     // Save bpp field for surface dump support
            if (retCode == ADDR_OK)
   {
      pOut->bpp               = surfOut.bpp;
   pOut->pitch             = surfOut.pitch;
   pOut->height            = surfOut.height;
   pOut->numSlices         = surfOut.depth;
   pOut->fmaskBytes        = surfOut.surfSize;
   pOut->baseAlign         = surfOut.baseAlign;
   pOut->pitchAlign        = surfOut.pitchAlign;
            if (surfOut.depth > 1)
   {
         // For fmask, expNumSlices is stored in depth.
   }
   else
   {
                  // Save numSamples field for surface dump support
                           }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlFmaskSurfaceInfo
   *   @brief
   *       Entry of EgBasedLib ComputeFmaskInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeFmaskInfo(
      const ADDR_COMPUTE_FMASK_INFO_INPUT*    pIn,   ///< [in] input structure
   ADDR_COMPUTE_FMASK_INFO_OUTPUT*         pOut   ///< [out] output structure
      {
                        // Use internal tile info if pOut does not have a valid pTileInfo
   if (pOut->pTileInfo == NULL)
   {
                           if (retCode == ADDR_OK)
   {
      pOut->tileIndex =
                     // Resets pTileInfo to NULL if the internal tile info is used
   if (pOut->pTileInfo == &tileInfo)
   {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeFmaskAddrFromCoord
   *   @brief
   *       Entry of EgBasedLib ComputeFmaskAddrFromCoord
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeFmaskAddrFromCoord(
      const ADDR_COMPUTE_FMASK_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_FMASK_ADDRFROMCOORD_OUTPUT*        pOut    ///< [out] output structure
      {
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeFmaskCoordFromAddr
   *   @brief
   *       Entry of EgBasedLib ComputeFmaskCoordFromAddr
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeFmaskCoordFromAddr(
      const ADDR_COMPUTE_FMASK_COORDFROMADDR_INPUT*   pIn,    ///< [in] input structure
   ADDR_COMPUTE_FMASK_COORDFROMADDR_OUTPUT*        pOut    ///< [out] output structure
      {
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeFmaskNumPlanesFromNumSamples
   *
   *   @brief
   *       Compute fmask number of planes from number of samples
   *
   *   @return
   *       Number of planes
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputeFmaskNumPlanesFromNumSamples(
         {
               //
   // FMASK is stored such that each micro tile is composed of elements containing N bits, where
   // N is the number of samples.  There is a micro tile for each bit in the FMASK address, and
   // micro tiles for each address bit, sometimes referred to as a plane, are stored sequentially.
   // The FMASK for a 2-sample surface looks like a general surface with 2 bits per element.
   // The FMASK for a 4-sample surface looks like a general surface with 4 bits per element and
   // 2 samples.  The FMASK for an 8-sample surface looks like a general surface with 8 bits per
   // element and 4 samples.  R6xx and R7xx only stored 3 planes for 8-sample FMASK surfaces.
   // This was changed for R8xx to simplify the logic in the CB.
   //
   switch (numSamples)
   {
      case 2:
         numPlanes = 1;
   case 4:
         numPlanes = 2;
   case 8:
         numPlanes = 4;
   default:
         ADDR_UNHANDLED_CASE();
      }
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::ComputeFmaskResolvedBppFromNumSamples
   *
   *   @brief
   *       Compute resolved fmask effective bpp based on number of samples
   *
   *   @return
   *       bpp
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::ComputeFmaskResolvedBppFromNumSamples(
         {
               //
   // Resolved FMASK surfaces are generated yBit the CB and read yBit the texture unit
   // so that the texture unit can read compressed multi-sample color data.
   // These surfaces store each index value packed per element.
   // Each element contains at least num_samples * log2(num_samples) bits.
   // Resolved FMASK surfaces are addressed as follows:
   // 2-sample Addressed similarly to a color surface with 8 bits per element and 1 sample.
   // 4-sample Addressed similarly to a color surface with 8 bits per element and 1 sample.
            switch (numSamples)
   {
      case 2:
         bpp = 8;
   case 4:
         bpp = 8;
   case 8:
         bpp = 32;
   default:
         ADDR_UNHANDLED_CASE();
      }
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::IsTileInfoAllZero
   *
   *   @brief
   *       Return TRUE if all field are zero
   *   @note
   *       Since NULL input is consider to be all zero
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::IsTileInfoAllZero(
         {
               if (pTileInfo)
   {
      if ((pTileInfo->banks            != 0)  ||
         (pTileInfo->bankWidth        != 0)  ||
   (pTileInfo->bankHeight       != 0)  ||
   (pTileInfo->macroAspectRatio != 0)  ||
   (pTileInfo->tileSplitBytes   != 0)  ||
   (pTileInfo->pipeConfig       != 0)
   {
                        }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlTileInfoEqual
   *
   *   @brief
   *       Return TRUE if all field are equal
   *   @note
   *       Only takes care of current HWL's data
   ****************************************************************************************************
   */
   BOOL_32 EgBasedLib::HwlTileInfoEqual(
      const ADDR_TILEINFO* pLeft, ///<[in] Left compare operand
   const ADDR_TILEINFO* pRight ///<[in] Right compare operand
      {
               if (pLeft->banks == pRight->banks           &&
      pLeft->bankWidth == pRight->bankWidth   &&
   pLeft->bankHeight == pRight->bankHeight &&
   pLeft->macroAspectRatio == pRight->macroAspectRatio &&
      {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlConvertTileInfoToHW
   *   @brief
   *       Entry of EgBasedLib ConvertTileInfoToHW
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlConvertTileInfoToHW(
      const ADDR_CONVERT_TILEINFOTOHW_INPUT* pIn, ///< [in] input structure
   ADDR_CONVERT_TILEINFOTOHW_OUTPUT* pOut      ///< [out] output structure
      {
               ADDR_TILEINFO *pTileInfoIn  = pIn->pTileInfo;
            if ((pTileInfoIn != NULL) && (pTileInfoOut != NULL))
   {
      if (pIn->reverse == FALSE)
   {
         switch (pTileInfoIn->banks)
   {
      case 2:
      pTileInfoOut->banks = 0;
      case 4:
      pTileInfoOut->banks = 1;
      case 8:
      pTileInfoOut->banks = 2;
      case 16:
      pTileInfoOut->banks = 3;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->bankWidth)
   {
      case 1:
      pTileInfoOut->bankWidth = 0;
      case 2:
      pTileInfoOut->bankWidth = 1;
      case 4:
      pTileInfoOut->bankWidth = 2;
      case 8:
      pTileInfoOut->bankWidth = 3;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->bankHeight)
   {
      case 1:
      pTileInfoOut->bankHeight = 0;
      case 2:
      pTileInfoOut->bankHeight = 1;
      case 4:
      pTileInfoOut->bankHeight = 2;
      case 8:
      pTileInfoOut->bankHeight = 3;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->macroAspectRatio)
   {
      case 1:
      pTileInfoOut->macroAspectRatio = 0;
      case 2:
      pTileInfoOut->macroAspectRatio = 1;
      case 4:
      pTileInfoOut->macroAspectRatio = 2;
      case 8:
      pTileInfoOut->macroAspectRatio = 3;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->tileSplitBytes)
   {
      case 64:
      pTileInfoOut->tileSplitBytes = 0;
      case 128:
      pTileInfoOut->tileSplitBytes = 1;
      case 256:
      pTileInfoOut->tileSplitBytes = 2;
      case 512:
      pTileInfoOut->tileSplitBytes = 3;
      case 1024:
      pTileInfoOut->tileSplitBytes = 4;
      case 2048:
      pTileInfoOut->tileSplitBytes = 5;
      case 4096:
      pTileInfoOut->tileSplitBytes = 6;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
   pTileInfoOut->tileSplitBytes = 0;
   }
   else
   {
         switch (pTileInfoIn->banks)
   {
      case 0:
      pTileInfoOut->banks = 2;
      case 1:
      pTileInfoOut->banks = 4;
      case 2:
      pTileInfoOut->banks = 8;
      case 3:
      pTileInfoOut->banks = 16;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->bankWidth)
   {
      case 0:
      pTileInfoOut->bankWidth = 1;
      case 1:
      pTileInfoOut->bankWidth = 2;
      case 2:
      pTileInfoOut->bankWidth = 4;
      case 3:
      pTileInfoOut->bankWidth = 8;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->bankHeight)
   {
      case 0:
      pTileInfoOut->bankHeight = 1;
      case 1:
      pTileInfoOut->bankHeight = 2;
      case 2:
      pTileInfoOut->bankHeight = 4;
      case 3:
      pTileInfoOut->bankHeight = 8;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->macroAspectRatio)
   {
      case 0:
      pTileInfoOut->macroAspectRatio = 1;
      case 1:
      pTileInfoOut->macroAspectRatio = 2;
      case 2:
      pTileInfoOut->macroAspectRatio = 4;
      case 3:
      pTileInfoOut->macroAspectRatio = 8;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
                  switch (pTileInfoIn->tileSplitBytes)
   {
      case 0:
      pTileInfoOut->tileSplitBytes = 64;
      case 1:
      pTileInfoOut->tileSplitBytes = 128;
      case 2:
      pTileInfoOut->tileSplitBytes = 256;
      case 3:
      pTileInfoOut->tileSplitBytes = 512;
      case 4:
      pTileInfoOut->tileSplitBytes = 1024;
      case 5:
      pTileInfoOut->tileSplitBytes = 2048;
      case 6:
      pTileInfoOut->tileSplitBytes = 4096;
      default:
      ADDR_ASSERT_ALWAYS();
   retCode = ADDR_INVALIDPARAMS;
   pTileInfoOut->tileSplitBytes = 64;
            if (pTileInfoIn != pTileInfoOut)
   {
            }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeSurfaceInfo
   *   @brief
   *       Entry of EgBasedLib ComputeSurfaceInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeSurfaceInfo(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut    ///< [out] output structure
      {
               if (pIn->numSamples < pIn->numFrags)
   {
                           if (retCode == ADDR_OK)
   {
      // Uses internal tile info if pOut does not have a valid pTileInfo
   if (pOut->pTileInfo == NULL)
   {
                  if (DispatchComputeSurfaceInfo(pIn, pOut) == FALSE)
   {
                  // In case client uses tile info as input and would like to calculate a correct size and
   // alignment together with tile info as output when the tile info is not suppose to have any
   // matching indices in tile mode tables.
   if (pIn->flags.skipIndicesOutput == FALSE)
   {
         // Returns an index
   pOut->tileIndex = HwlPostCheckTileIndex(pOut->pTileInfo,
                        if (IsMacroTiled(pOut->tileMode) && (pOut->macroModeIndex == TileIndexInvalid))
   {
      pOut->macroModeIndex = HwlComputeMacroModeIndex(pOut->tileIndex,
                                 // Resets pTileInfo to NULL if the internal tile info is used
   if (pOut->pTileInfo == &tileInfo)
   #if DEBUG
               // Client does not pass in a valid pTileInfo
   if (IsMacroTiled(pOut->tileMode))
   {
                           if (IsTileInfoAllZero(pIn->pTileInfo) == FALSE)
   {
      // The initial value of pIn->pTileInfo is copied to tileInfo
   // We do not expect any of these value to be changed nor any 0 of inputs
   ADDR_ASSERT(tileInfo.banks == pIn->pTileInfo->banks);
   ADDR_ASSERT(tileInfo.bankWidth == pIn->pTileInfo->bankWidth);
   ADDR_ASSERT(tileInfo.bankHeight == pIn->pTileInfo->bankHeight);
   ADDR_ASSERT(tileInfo.macroAspectRatio == pIn->pTileInfo->macroAspectRatio);
   #endif
                              }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeSurfaceAddrFromCoord
   *   @brief
   *       Entry of EgBasedLib ComputeSurfaceAddrFromCoord
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeSurfaceAddrFromCoord(
      const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
                  #if !ALT_TEST // Overflow test needs this out-of-boundary coord
         (pIn->x > pIn->pitch)   ||
   #endif
            {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeSurfaceCoordFromAddr
   *   @brief
   *       Entry of EgBasedLib ComputeSurfaceCoordFromAddr
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeSurfaceCoordFromAddr(
      const ADDR_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut    ///< [out] output structure
      {
               if ((pIn->bitPosition >= 8) ||
         {
         }
   else
   {
         }
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeSliceTileSwizzle
   *   @brief
   *       Entry of EgBasedLib ComputeSurfaceCoordFromAddr
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE EgBasedLib::HwlComputeSliceTileSwizzle(
      const ADDR_COMPUTE_SLICESWIZZLE_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SLICESWIZZLE_OUTPUT*       pOut    ///< [out] output structure
      {
               if (pIn->pTileInfo && (pIn->pTileInfo->banks > 0))
               pOut->tileSwizzle = ComputeSliceTileSwizzle(pIn->tileMode,
                        }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeHtileBpp
   *
   *   @brief
   *       Compute htile bpp
   *
   *   @return
   *       Htile bpp
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::HwlComputeHtileBpp(
      BOOL_32 isWidth8,   ///< [in] TRUE if block width is 8
   BOOL_32 isHeight8   ///< [in] TRUE if block height is 8
      {
      // only support 8x8 mode
   ADDR_ASSERT(isWidth8 && isHeight8);
      }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlComputeHtileBaseAlign
   *
   *   @brief
   *       Compute htile base alignment
   *
   *   @return
   *       Htile base alignment
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::HwlComputeHtileBaseAlign(
      BOOL_32         isTcCompatible, ///< [in] if TC compatible
   BOOL_32         isLinear,       ///< [in] if it is linear mode
   ADDR_TILEINFO*  pTileInfo       ///< [in] Tile info
      {
               if (isTcCompatible)
   {
      ADDR_ASSERT(pTileInfo != NULL);
   if (pTileInfo)
   {
                        }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlGetPitchAlignmentMicroTiled
   *
   *   @brief
   *       Compute 1D tiled surface pitch alignment, calculation results are returned through
   *       output parameters.
   *
   *   @return
   *       pitch alignment
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::HwlGetPitchAlignmentMicroTiled(
      AddrTileMode        tileMode,          ///< [in] tile mode
   UINT_32             bpp,               ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,             ///< [in] surface flags
   UINT_32             numSamples         ///< [in] number of samples
      {
                        UINT_32 pixelsPerMicroTile;
   UINT_32 pixelsPerPipeInterleave;
            //
   // Special workaround for depth/stencil buffer, use 8 bpp to meet larger requirement for
   // stencil buffer since pitch alignment is related to bpp.
   // For a depth only buffer do not set this.
   //
   // Note: this actually does not work for mipmap but mipmap depth texture is not really
   // sampled with mipmap.
   //
   if (flags.depth && (flags.noStencil == FALSE))
   {
                  pixelsPerMicroTile = MicroTilePixels * microTileThickness;
   pixelsPerPipeInterleave = BYTES_TO_BITS(m_pipeInterleaveBytes) / (bpp * numSamples);
                        }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlGetSizeAdjustmentMicroTiled
   *
   *   @brief
   *       Adjust 1D tiled surface pitch and slice size
   *
   *   @return
   *       Logical slice size in bytes
   ****************************************************************************************************
   */
   UINT_64 EgBasedLib::HwlGetSizeAdjustmentMicroTiled(
      UINT_32             thickness,      ///< [in] thickness
   UINT_32             bpp,            ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,          ///< [in] surface flags
   UINT_32             numSamples,     ///< [in] number of samples
   UINT_32             baseAlign,      ///< [in] base alignment
   UINT_32             pitchAlign,     ///< [in] pitch alignment
   UINT_32*            pPitch,         ///< [in,out] pointer to pitch
   UINT_32*            pHeight         ///< [in,out] pointer to height
      {
      UINT_64 logicalSliceSize;
            UINT_32 pitch   = *pPitch;
            // Logical slice: pitch * height * bpp * numSamples (no 1D MSAA so actually numSamples == 1)
            // Physical slice: multiplied by thickness
            //
   // R800 will always pad physical slice size to baseAlign which is pipe_interleave_bytes
   //
               }
      /**
   ****************************************************************************************************
   *   EgBasedLib::HwlStereoCheckRightOffsetPadding
   *
   *   @brief
   *       check if the height needs extra padding for stereo right eye offset, to avoid swizzling
   *
   *   @return
   *       TRUE is the extra padding is needed
   *
   ****************************************************************************************************
   */
   UINT_32 EgBasedLib::HwlStereoCheckRightOffsetPadding(
      ADDR_TILEINFO* pTileInfo    ///< Tiling info
      {
               if (pTileInfo->macroAspectRatio > 2)
   {
      // Since 3D rendering treats right eye surface starting from y == "eye height" while
   // display engine treats it to be 0, so the bank bits may be different.
   // Additional padding in height is required to make sure it's possible
            static const UINT_32 StereoAspectRatio = 2;
   stereoHeightAlign = pTileInfo->banks *
         pTileInfo->bankHeight *
                  }
      } // V1
   } // Addr
