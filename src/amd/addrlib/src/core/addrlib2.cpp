   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
         /**
   ************************************************************************************************************************
   * @file  addrlib2.cpp
   * @brief Contains the implementation for the AddrLib2 base class.
   ************************************************************************************************************************
   */
      #include "addrinterface.h"
   #include "addrlib2.h"
   #include "addrcommon.h"
      namespace Addr
   {
   namespace V2
   {
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Static Const Member
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      const Dim2d Lib::Block256_2d[] = {{16, 16}, {16, 8}, {8, 8}, {8, 4}, {4, 4}};
      const Dim3d Lib::Block1K_3d[]  = {{16, 8, 8}, {8, 8, 8}, {8, 8, 4}, {8, 4, 4}, {4, 4, 4}};
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Constructor/Destructor
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ************************************************************************************************************************
   *   Lib::Lib
   *
   *   @brief
   *       Constructor for the Addr::V2::Lib class
   *
   ************************************************************************************************************************
   */
   Lib::Lib()
      :
   Addr::Lib(),
   m_se(0),
   m_rbPerSe(0),
   m_maxCompFrag(0),
   m_banksLog2(0),
   m_pipesLog2(0),
   m_seLog2(0),
   m_rbPerSeLog2(0),
   m_maxCompFragLog2(0),
   m_pipeInterleaveLog2(0),
   m_blockVarSizeLog2(0),
      {
   }
      /**
   ************************************************************************************************************************
   *   Lib::Lib
   *
   *   @brief
   *       Constructor for the AddrLib2 class with hClient as parameter
   *
   ************************************************************************************************************************
   */
   Lib::Lib(const Client* pClient)
      :
   Addr::Lib(pClient),
   m_se(0),
   m_rbPerSe(0),
   m_maxCompFrag(0),
   m_banksLog2(0),
   m_pipesLog2(0),
   m_seLog2(0),
   m_rbPerSeLog2(0),
   m_maxCompFragLog2(0),
   m_pipeInterleaveLog2(0),
   m_blockVarSizeLog2(0),
      {
   }
      /**
   ************************************************************************************************************************
   *   Lib::~Lib
   *
   *   @brief
   *       Destructor for the AddrLib2 class
   *
   ************************************************************************************************************************
   */
   Lib::~Lib()
   {
   }
      /**
   ************************************************************************************************************************
   *   Lib::GetLib
   *
   *   @brief
   *       Get Addr::V2::Lib pointer
   *
   *   @return
   *      An Addr::V2::Lib class pointer
   ************************************************************************************************************************
   */
   Lib* Lib::GetLib(
         {
      Addr::Lib* pAddrLib = Addr::Lib::GetLib(hLib);
   if ((pAddrLib != NULL) &&
         {
      // only valid and GFX9+ ASIC can use AddrLib2 function.
   ADDR_ASSERT_ALWAYS();
      }
      }
         ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Surface Methods
   ////////////////////////////////////////////////////////////////////////////////////////////////////
         /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeSurfaceInfo.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceInfo(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR2_COMPUTE_SURFACE_INFO_INPUT)) ||
         {
                     // Adjust coming parameters.
   ADDR2_COMPUTE_SURFACE_INFO_INPUT localIn = *pIn;
   localIn.width        = Max(pIn->width, 1u);
   localIn.height       = Max(pIn->height, 1u);
   localIn.numMipLevels = Max(pIn->numMipLevels, 1u);
   localIn.numSlices    = Max(pIn->numSlices, 1u);
   localIn.numSamples   = Max(pIn->numSamples, 1u);
            UINT_32  expandX  = 1;
   UINT_32  expandY  = 1;
            if (returnCode == ADDR_OK)
   {
      // Set format to INVALID will skip this conversion
   if (localIn.format != ADDR_FMT_INVALID)
   {
         // Get compression/expansion factors and element mode which indicates compression/expansion
   localIn.bpp = GetElemLib()->GetBitsPerPixel(localIn.format,
                        // Special flag for 96 bit surface. 96 (or 48 if we support) bit surface's width is
   // pre-multiplied by 3 and bpp is divided by 3. So pitch alignment for linear-
   // aligned does not meet 64-pixel in real. We keep special handling in hwl since hw
   // restrictions are different.
                  if ((elemMode == ADDR_EXPANDED) && (expandX > 1))
   {
                  UINT_32 basePitch = 0;
   GetElemLib()->AdjustSurfaceInfo(elemMode,
                                                   if (localIn.bpp != 0)
   {
         localIn.width  = Max(localIn.width, 1u);
   }
   else // Rule out some invalid parameters
   {
                              if (returnCode == ADDR_OK)
   {
                  if (returnCode == ADDR_OK)
   {
               if (IsLinear(pIn->swizzleMode))
   {
         // linear mode
   }
   else
   {
         // tiled mode
            if (returnCode == ADDR_OK)
   {
         pOut->bpp = localIn.bpp;
   pOut->pixelPitch = pOut->pitch;
   pOut->pixelHeight = pOut->height;
   pOut->pixelMipChainPitch = pOut->mipChainPitch;
                  if (localIn.format != ADDR_FMT_INVALID)
                     GetElemLib()->RestoreSurfaceInfo(elemMode,
                                    GetElemLib()->RestoreSurfaceInfo(elemMode,
                                    if ((localIn.numMipLevels > 1) && (pOut->pMipInfo != NULL))
   {
      for (UINT_32 i = 0; i < localIn.numMipLevels; i++)
                              GetElemLib()->RestoreSurfaceInfo(elemMode,
                                          if (localIn.flags.needEquation && (Log2(localIn.numFrags) == 0))
   {
      pOut->equationIndex = GetEquationIndex(&localIn, pOut);
   if ((localIn.flags.allowExtEquation == 0) &&
      (pOut->equationIndex != ADDR_INVALID_EQUATION_INDEX) &&
      {
                     if (localIn.flags.qbStereo)
   {
         #if DEBUG
         #endif
                                                      }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeSurfaceInfo.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceAddrFromCoord(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT)) ||
         {
                     ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT localIn = *pIn;
   localIn.unalignedWidth  = Max(pIn->unalignedWidth, 1u);
   localIn.unalignedHeight = Max(pIn->unalignedHeight, 1u);
   localIn.numMipLevels    = Max(pIn->numMipLevels, 1u);
   localIn.numSlices       = Max(pIn->numSlices, 1u);
   localIn.numSamples      = Max(pIn->numSamples, 1u);
            if ((localIn.bpp < 8)        ||
      (localIn.bpp > 128)      ||
   ((localIn.bpp % 8) != 0) ||
   (localIn.sample >= localIn.numSamples)  ||
   (localIn.slice >= localIn.numSlices)    ||
   (localIn.mipId >= localIn.numMipLevels) ||
   (IsTex3d(localIn.resourceType) &&
      {
                  if (returnCode == ADDR_OK)
   {
      if (IsLinear(localIn.swizzleMode))
   {
         }
   else
   {
                  if (returnCode == ADDR_OK)
   {
                        }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceCoordFromAddr
   *
   *   @brief
   *       Interface function stub of ComputeSurfaceCoordFromAddr.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceCoordFromAddr(
      const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut    ///< [out] output structure
      {
               if (GetFillSizeFieldsFlags() == TRUE)
   {
      if ((pIn->size != sizeof(ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT)) ||
         {
                     if ((pIn->bpp < 8)        ||
      (pIn->bpp > 128)      ||
   ((pIn->bpp % 8) != 0) ||
      {
                  if (returnCode == ADDR_OK)
   {
      if (IsLinear(pIn->swizzleMode))
   {
         }
   else
   {
                        }
         ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               CMASK/HTILE
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ************************************************************************************************************************
   *   Lib::ComputeHtileInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeHtilenfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeHtileInfo(
      const ADDR2_COMPUTE_HTILE_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_HTILE_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_HTILE_INFO_INPUT)) ||
      {
         }
   else
   {
                              }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeHtileAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeHtileAddrFromCoord(
      const ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeHtileCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeHtileCoordFromAddr(
      const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT*   pIn,    ///< [in] input structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeCmaskInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskInfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskInfo(
      const ADDR2_COMPUTE_CMASK_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_CMASK_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_CMASK_INFO_INPUT)) ||
      {
         }
   else if (pIn->cMaskFlags.linear)
   {
         }
   else
   {
                              }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskAddrFromCoord(
      const ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeCmaskCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeCmaskCoordFromAddr(
      const ADDR2_COMPUTE_CMASK_COORDFROMADDR_INPUT*   pIn,    ///< [in] input structure
   ADDR2_COMPUTE_CMASK_COORDFROMADDR_OUTPUT*        pOut    ///< [out] output structure
      {
                           }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeFmaskInfo
   *
   *   @brief
   *       Interface function stub of ComputeFmaskInfo.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeFmaskInfo(
      const ADDR2_COMPUTE_FMASK_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_FMASK_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               BOOL_32 valid = (IsZOrderSwizzle(pIn->swizzleMode) == TRUE) &&
            if (GetFillSizeFieldsFlags())
   {
      if ((pIn->size != sizeof(ADDR2_COMPUTE_FMASK_INFO_INPUT)) ||
         {
                     if (valid == FALSE)
   {
         }
   else
   {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn = {0};
            localIn.size = sizeof(ADDR2_COMPUTE_SURFACE_INFO_INPUT);
            localIn.swizzleMode  = pIn->swizzleMode;
   localIn.numSlices    = Max(pIn->numSlices, 1u);
   localIn.width        = Max(pIn->unalignedWidth, 1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.bpp          = GetFmaskBpp(pIn->numSamples, pIn->numFrags);
   localIn.flags.fmask  = 1;
   localIn.numFrags     = 1;
   localIn.numSamples   = 1;
            if (localIn.bpp == 8)
   {
         }
   else if (localIn.bpp == 16)
   {
         }
   else if (localIn.bpp == 32)
   {
         }
   else
   {
                           if (returnCode == ADDR_OK)
   {
         pOut->pitch      = localOut.pitch;
   pOut->height     = localOut.height;
   pOut->baseAlign  = localOut.baseAlign;
   pOut->numSlices  = localOut.numSlices;
   pOut->fmaskBytes = static_cast<UINT_32>(localOut.surfSize);
   pOut->sliceSize  = static_cast<UINT_32>(localOut.sliceSize);
   pOut->bpp        = localIn.bpp;
                           }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeFmaskAddrFromCoord
   *
   *   @brief
   *       Interface function stub of ComputeFmaskAddrFromCoord.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeFmaskAddrFromCoord(
      const ADDR2_COMPUTE_FMASK_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
   ADDR2_COMPUTE_FMASK_ADDRFROMCOORD_OUTPUT*        pOut    ///< [out] output structure
      {
                           }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeFmaskCoordFromAddr
   *
   *   @brief
   *       Interface function stub of ComputeFmaskAddrFromCoord.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeFmaskCoordFromAddr(
      const ADDR2_COMPUTE_FMASK_COORDFROMADDR_INPUT*  pIn,     ///< [in] input structure
   ADDR2_COMPUTE_FMASK_COORDFROMADDR_OUTPUT*       pOut     ///< [out] output structure
      {
                           }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeDccInfo
   *
   *   @brief
   *       Interface function to compute DCC key info
   *
   *   @return
   *       return code of HwlComputeDccInfo
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeDccInfo(
      const ADDR2_COMPUTE_DCCINFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_DCCINFO_OUTPUT*         pOut    ///< [out] output structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_DCCINFO_INPUT)) ||
      {
         }
   else
   {
                              }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeDccAddrFromCoord
   *
   *   @brief
   *       Interface function stub of ComputeDccAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeDccAddrFromCoord(
      const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT)) ||
      {
         }
   else
   {
               if (returnCode == ADDR_OK)
   {
                        }
      /**
   ************************************************************************************************************************
   *   Lib::ComputePipeBankXor
   *
   *   @brief
   *       Interface function stub of Addr2ComputePipeBankXor.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputePipeBankXor(
      const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_PIPEBANKXOR_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSlicePipeBankXor
   *
   *   @brief
   *       Interface function stub of Addr2ComputeSlicePipeBankXor.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSlicePipeBankXor(
      const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT)) ||
      {
         }
   else if ((IsThin(pIn->resourceType, pIn->swizzleMode) == FALSE) ||
               {
         }
   else if ((pIn->bpe != 0) &&
            (pIn->bpe != 8) &&
   (pIn->bpe != 16) &&
   (pIn->bpe != 32) &&
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSubResourceOffsetForSwizzlePattern
   *
   *   @brief
   *       Interface function stub of Addr2ComputeSubResourceOffsetForSwizzlePattern.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSubResourceOffsetForSwizzlePattern(
      const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeNonBlockCompressedView
   *
   *   @brief
   *       Interface function stub of Addr2ComputeNonBlockCompressedView.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeNonBlockCompressedView(
      const ADDR2_COMPUTE_NONBLOCKCOMPRESSEDVIEW_INPUT* pIn,
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_COMPUTE_NONBLOCKCOMPRESSEDVIEW_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ExtractPipeBankXor
   *
   *   @brief
   *       Internal function to extract bank and pipe xor bits from combined xor bits.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ExtractPipeBankXor(
      UINT_32  pipeBankXor,
   UINT_32  bankBits,
   UINT_32  pipeBits,
   UINT_32* pBankX,
      {
               if (pipeBankXor < (1u << (pipeBits + bankBits)))
   {
      *pPipeX = pipeBankXor % (1 << pipeBits);
   *pBankX = pipeBankXor >> pipeBits;
      }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceInfoSanityCheck
   *
   *   @brief
   *       Internal function to do basic sanity check before compute surface info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceInfoSanityCheck(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT*  pIn   ///< [in] input structure
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
         {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ApplyCustomizedPitchHeight
   *
   *   @brief
   *       Helper function to override hw required row pitch/slice pitch by customrized one
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ApplyCustomizedPitchHeight(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   UINT_32  elementBytes,                          ///< [in] element bytes per element
   UINT_32  pitchAlignInElement,                   ///< [in] pitch alignment in element
   UINT_32* pPitch,                                ///< [in/out] pitch
   UINT_32* pHeight                                ///< [in/out] height
      {
               if (pIn->numMipLevels <= 1)
   {
      if (pIn->pitchInElement > 0)
   {
         if ((pIn->pitchInElement % pitchAlignInElement) != 0)
   {
         }
   else if (pIn->pitchInElement < (*pPitch))
   {
         }
   else
   {
                  if (returnCode == ADDR_OK)
   {
         if (pIn->sliceAlign > 0)
                     if (customizedHeight * elementBytes * (*pPitch) != pIn->sliceAlign)
   {
         }
   else if ((pIn->numSlices > 1) && ((*pHeight) != customizedHeight))
   {
         }
   else
   {
                           }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceInfoLinear
   *
   *   @brief
   *       Internal function to calculate alignment for linear swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceInfoLinear(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
         }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceInfoTiled
   *
   *   @brief
   *       Internal function to calculate alignment for tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceInfoTiled(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
         }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceAddrFromCoordLinear
   *
   *   @brief
   *       Internal function to calculate address from coord for linear swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceAddrFromCoordLinear(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR_E_RETURNCODE returnCode = ADDR_OK;
            if (valid)
   {
      if (IsTex1d(pIn->resourceType))
   {
                     if (valid)
   {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn  = {0};
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {0};
   ADDR2_MIP_INFO                    mipInfo[MaxMipLevels];
            localIn.bpp          = pIn->bpp;
   localIn.flags        = pIn->flags;
   localIn.width        = Max(pIn->unalignedWidth, 1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices, 1u);
   localIn.numMipLevels = Max(pIn->numMipLevels, 1u);
            if (localIn.numMipLevels <= 1)
   {
                                    if (returnCode == ADDR_OK)
   {
         pOut->addr        = (localOut.sliceSize * pIn->slice) +
               }
   else
   {
                     if (valid == FALSE)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceAddrFromCoordTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceAddrFromCoordTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
         }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceCoordFromAddrLinear
   *
   *   @brief
   *       Internal function to calculate coord from address for linear swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceCoordFromAddrLinear(
      const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut    ///< [out] output structure
      {
                        if (valid)
   {
      if (IsTex1d(pIn->resourceType))
   {
                     if (valid)
   {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn  = {0};
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {0};
   localIn.bpp          = pIn->bpp;
   localIn.flags        = pIn->flags;
   localIn.width        = Max(pIn->unalignedWidth, 1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices, 1u);
   localIn.numMipLevels = Max(pIn->numMipLevels, 1u);
   localIn.resourceType = pIn->resourceType;
   if (localIn.numMipLevels <= 1)
   {
         }
            if (returnCode == ADDR_OK)
   {
                        UINT_32 offsetInSlice = static_cast<UINT_32>(pIn->addr % localOut.sliceSize);
   UINT_32 elementBytes = pIn->bpp >> 3;
   UINT_32 mipOffsetInSlice = 0;
   UINT_32 mipSize = 0;
   UINT_32 mipId = 0;
   for (; mipId < pIn->numMipLevels ; mipId++)
   {
      if (IsTex1d(pIn->resourceType))
   {
         }
   else
   {
                        if (mipSize == 0)
   {
      valid = FALSE;
      }
   else if ((mipSize + mipOffsetInSlice) > offsetInSlice)
   {
         }
   else
   {
      mipOffsetInSlice += mipSize;
   if ((mipId == (pIn->numMipLevels - 1)) ||
         {
                        if (valid)
                     UINT_32 elemOffsetInMip = (offsetInSlice - mipOffsetInSlice) / elementBytes;
   if (IsTex1d(pIn->resourceType))
   {
      if (elemOffsetInMip < localOut.pitch)
   {
         pOut->x = elemOffsetInMip;
   }
   else
   {
            }
   else
   {
                        if ((pOut->slice >= pIn->numSlices)    ||
      (pOut->mipId >= pIn->numMipLevels) ||
   (pOut->x >= Max((pIn->unalignedWidth >> pOut->mipId), 1u))  ||
   (pOut->y >= Max((pIn->unalignedHeight >> pOut->mipId), 1u)) ||
   (IsTex3d(pIn->resourceType) &&
      (FALSE == Valid3DMipSliceIdConstraint(pIn->numSlices,
         {
            }
   else
   {
                     if (valid == FALSE)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurfaceCoordFromAddrTiled
   *
   *   @brief
   *       Internal function to calculate coord from address for tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeSurfaceCoordFromAddrTiled(
      const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut    ///< [out] output structure
      {
                           }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeBlockDimensionForSurf
   *
   *   @brief
   *       Internal function to get block width/height/depth in element from surface input params.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeBlockDimensionForSurf(
      UINT_32*         pWidth,
   UINT_32*         pHeight,
   UINT_32*         pDepth,
   UINT_32          bpp,
   UINT_32          numSamples,
   AddrResourceType resourceType,
      {
               if (IsThick(resourceType, swizzleMode))
   {
         }
   else if (IsThin(resourceType, swizzleMode))
   {
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeThinBlockDimension
   *
   *   @brief
   *       Internal function to get thin block width/height/depth in element from surface input params.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Lib::ComputeThinBlockDimension(
      UINT_32*         pWidth,
   UINT_32*         pHeight,
   UINT_32*         pDepth,
   UINT_32          bpp,
   UINT_32          numSamples,
   AddrResourceType resourceType,
      {
               // GFX9/GFX10 use different dimension amplifying logic: say for 128KB block + 1xAA + 1BPE, the dimension of thin
   // swizzle mode will be [256W * 512H] on GFX9 ASICs and [512W * 256H] on GFX10 ASICs. Since GFX10 is newer HWL so we
   // make its implementation into base class (in order to save future change on new HWLs)
   const UINT_32 log2BlkSize  = GetBlockSizeLog2(swizzleMode);
   const UINT_32 log2EleBytes = Log2(bpp >> 3);
   const UINT_32 log2Samples  = Log2(Max(numSamples, 1u));
            // For "1xAA/4xAA cases" or "2xAA/8xAA + odd log2BlkSize cases", width == height or width == 2 * height;
   // For other cases, height == width or height == 2 * width
   const BOOL_32 widthPrecedent = ((log2Samples & 1) == 0) || ((log2BlkSize & 1) != 0);
            *pWidth  = 1u << log2Width;
   *pHeight = 1u << (log2NumEle - log2Width);
      }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeBlockDimension
   *
   *   @brief
   *       Internal function to get block width/height/depth in element without considering MSAA case
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeBlockDimension(
      UINT_32*         pWidth,
   UINT_32*         pHeight,
   UINT_32*         pDepth,
   UINT_32          bpp,
   AddrResourceType resourceType,
      {
               if (IsThick(resourceType, swizzleMode))
   {
         }
   else if (IsThin(resourceType, swizzleMode))
   {
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeThickBlockDimension
   *
   *   @brief
   *       Internal function to get block width/height/depth in element for thick swizzle mode
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Lib::ComputeThickBlockDimension(
      UINT_32*         pWidth,
   UINT_32*         pHeight,
   UINT_32*         pDepth,
   UINT_32          bpp,
   AddrResourceType resourceType,
      {
               const UINT_32 log2BlkSize              = GetBlockSizeLog2(swizzleMode);
   const UINT_32 eleBytes                 = bpp >> 3;
                     const UINT_32 log2blkSizeIn1KB = log2BlkSize - 10;
   const UINT_32 averageAmp       = log2blkSizeIn1KB / 3;
            *pWidth  = Block1K_3d[microBlockSizeTableIndex].w << averageAmp;
   *pHeight = Block1K_3d[microBlockSizeTableIndex].h << (averageAmp + (restAmp / 2));
      }
      /**
   ************************************************************************************************************************
   *   Lib::GetMipTailDim
   *
   *   @brief
   *       Internal function to get out max dimension of first level in mip tail
   *
   *   @return
   *       Max Width/Height/Depth value of the first mip fitted in mip tail
   ************************************************************************************************************************
   */
   Dim3d Lib::GetMipTailDim(
      AddrResourceType  resourceType,
   AddrSwizzleMode   swizzleMode,
   UINT_32           blockWidth,
   UINT_32           blockHeight,
      {
      Dim3d   out         = {blockWidth, blockHeight, blockDepth};
            if (IsThick(resourceType, swizzleMode))
   {
               if (dim == 0)
   {
         }
   else if (dim == 1)
   {
         }
   else
   {
            }
   else
   {
         #if DEBUG
         // GFX9/GFX10 use different dimension shrinking logic for mipmap tail: say for 128KB block + 2BPE, the maximum
   // dimension of mipmap tail level will be [256W * 128H] on GFX9 ASICs and [128W * 256H] on GFX10 ASICs. Since
   // GFX10 is newer HWL so we make its implementation into base class, in order to save future change on new HWLs.
   // And assert log2BlkSize will always be an even value on GFX9, so we never need the logic wrapped by DEBUG...
   if ((log2BlkSize & 1) && (m_chipFamily == ADDR_CHIP_FAMILY_AI))
   {
                        }
   #endif
         {
                        }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurface2DMicroBlockOffset
   *
   *   @brief
   *       Internal function to calculate micro block (256B) offset from coord for 2D resource
   *
   *   @return
   *       micro block (256B) offset for 2D resource
   ************************************************************************************************************************
   */
   UINT_32 Lib::ComputeSurface2DMicroBlockOffset(
         {
               UINT_32 log2ElementBytes = Log2(pIn->bpp >> 3);
   UINT_32 microBlockOffset = 0;
   if (IsStandardSwizzle(pIn->resourceType, pIn->swizzleMode))
   {
      UINT_32 xBits = pIn->x << log2ElementBytes;
   microBlockOffset = (xBits & 0xf) | ((pIn->y & 0x3) << 4);
   if (log2ElementBytes < 3)
   {
         microBlockOffset |= (pIn->y & 0x4) << 4;
   if (log2ElementBytes == 0)
   {
         }
   else
   {
         }
   else
   {
            }
   else if (IsDisplaySwizzle(pIn->resourceType, pIn->swizzleMode))
   {
      if (log2ElementBytes == 4)
   {
         microBlockOffset = (GetBit(pIn->x, 0) << 4) |
               }
   else
   {
         microBlockOffset = GetBits(pIn->x, 0, 3, log2ElementBytes)     |
                     microBlockOffset = GetBits(microBlockOffset, 0, 4, 0) |
            }
   else if (IsRotateSwizzle(pIn->swizzleMode))
   {
      microBlockOffset = GetBits(pIn->y, 0, 3, log2ElementBytes) |
                     microBlockOffset = GetBits(microBlockOffset, 0, 4, 0) |
               if (log2ElementBytes == 3)
   {
      microBlockOffset = GetBits(microBlockOffset, 0, 6, 0) |
                     }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeSurface3DMicroBlockOffset
   *
   *   @brief
   *       Internal function to calculate micro block (1KB) offset from coord for 3D resource
   *
   *   @return
   *       micro block (1KB) offset for 3D resource
   ************************************************************************************************************************
   */
   UINT_32 Lib::ComputeSurface3DMicroBlockOffset(
         {
               UINT_32 log2ElementBytes = Log2(pIn->bpp >> 3);
   UINT_32 microBlockOffset = 0;
   if (IsStandardSwizzle(pIn->resourceType, pIn->swizzleMode))
   {
      if (log2ElementBytes == 0)
   {
         }
   else if (log2ElementBytes == 1)
   {
         }
   else if (log2ElementBytes == 2)
   {
         }
   else if (log2ElementBytes == 3)
   {
         }
   else
   {
                           UINT_32 xBits = pIn->x << log2ElementBytes;
      }
   else if (IsZOrderSwizzle(pIn->swizzleMode))
   {
               if (log2ElementBytes == 0)
   {
         microBlockOffset =
                  xh = pIn->x >> 3;
   yh = pIn->y >> 2;
   }
   else if (log2ElementBytes == 1)
   {
         microBlockOffset =
                  xh = pIn->x >> 2;
   yh = pIn->y >> 2;
   }
   else if (log2ElementBytes == 2)
   {
         microBlockOffset =
                  xh = pIn->x >> 2;
   yh = pIn->y >> 2;
   }
   else if (log2ElementBytes == 3)
   {
         microBlockOffset =
                  xh = pIn->x >> 2;
   yh = pIn->y >> 1;
   }
   else
   {
                        xh = pIn->x >> 1;
   yh = pIn->y >> 1;
                           }
      /**
   ************************************************************************************************************************
   *   Lib::GetPipeXorBits
   *
   *   @brief
   *       Internal function to get bits number for pipe/se xor operation
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   UINT_32 Lib::GetPipeXorBits(
         {
               // Total available xor bits
            // Pipe/Se xor bits
               }
      /**
   ************************************************************************************************************************
   *   Lib::Addr2GetPreferredSurfaceSetting
   *
   *   @brief
   *       Internal function to get suggested surface information for cliet to use
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::Addr2GetPreferredSurfaceSetting(
      const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,
      {
               if ((GetFillSizeFieldsFlags() == TRUE) &&
      ((pIn->size != sizeof(ADDR2_GET_PREFERRED_SURF_SETTING_INPUT)) ||
      {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Lib::GetPossibleSwizzleModes
   *
   *   @brief
   *       Returns a list of swizzle modes that are valid from the hardware's perspective for the client to choose from
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::GetPossibleSwizzleModes(
      const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,
      {
         }
      /**
   ************************************************************************************************************************
   *   Lib::GetAllowedBlockSet
   *
   *   @brief
   *       Returns the set of allowed block sizes given the allowed swizzle modes and resource type
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::GetAllowedBlockSet(
      ADDR2_SWMODE_SET allowedSwModeSet,
   AddrResourceType rsrcType,
      {
         }
      /**
   ************************************************************************************************************************
   *   Lib::GetAllowedSwSet
   *
   *   @brief
   *       Returns the set of allowed swizzle types given the allowed swizzle modes
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::GetAllowedSwSet(
      ADDR2_SWMODE_SET  allowedSwModeSet,
      {
         }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeBlock256Equation
   *
   *   @brief
   *       Compute equation for block 256B
   *
   *   @return
   *       If equation computed successfully
   *
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeBlock256Equation(
      AddrResourceType rsrcType,
   AddrSwizzleMode swMode,
   UINT_32 elementBytesLog2,
      {
               if (IsBlock256b(swMode))
   {
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeThinEquation
   *
   *   @brief
   *       Compute equation for 2D/3D resource which use THIN mode
   *
   *   @return
   *       If equation computed successfully
   *
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeThinEquation(
      AddrResourceType rsrcType,
   AddrSwizzleMode swMode,
   UINT_32 elementBytesLog2,
      {
               if (IsThin(rsrcType, swMode))
   {
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeThickEquation
   *
   *   @brief
   *       Compute equation for 3D resource which use THICK mode
   *
   *   @return
   *       If equation computed successfully
   *
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Lib::ComputeThickEquation(
      AddrResourceType rsrcType,
   AddrSwizzleMode swMode,
   UINT_32 elementBytesLog2,
      {
               if (IsThick(rsrcType, swMode))
   {
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Lib::ComputeQbStereoInfo
   *
   *   @brief
   *       Get quad buffer stereo information
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Lib::ComputeQbStereoInfo(
      ADDR2_COMPUTE_SURFACE_INFO_OUTPUT* pOut    ///< [in,out] updated pOut+pStereoInfo
      {
      ADDR_ASSERT(pOut->bpp >= 8);
            // Save original height
            // Right offset
            // Double height
                              // Double size
   pOut->surfSize  <<= 1;
      }
      /**
   ************************************************************************************************************************
   *   Lib::FilterInvalidEqSwizzleMode
   *
   *   @brief
   *       Filter out swizzle mode(s) if it doesn't have valid equation index
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Lib::FilterInvalidEqSwizzleMode(
      ADDR2_SWMODE_SET& allowedSwModeSet,
   AddrResourceType  resourceType,
   UINT_32           elemLog2,
   UINT_32           maxComponents
      {
      if (resourceType != ADDR_RSRC_TEX_1D)
   {
      UINT_32       allowedSwModeSetVal = allowedSwModeSet.value;
   const UINT_32 rsrcTypeIdx         = static_cast<UINT_32>(resourceType) - 1;
            for (UINT_32 swModeIdx = 1; validSwModeSet != 0; swModeIdx++)
   {
         if (validSwModeSet & 1)
   {
      UINT_32 equation = m_equationLookupTable[rsrcTypeIdx][swModeIdx][elemLog2];
   if (equation == ADDR_INVALID_EQUATION_INDEX)
   {
         }
   else if (m_equationTable[equation].numBitComponents > maxComponents)
   {
                              // Only apply the filtering if at least one valid swizzle mode remains
   if (allowedSwModeSetVal != 0)
   {
               }
      #if DEBUG
   /**
   ************************************************************************************************************************
   *   Lib::ValidateStereoInfo
   *
   *   @brief
   *       Validate stereo info by checking a few typical cases
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Lib::ValidateStereoInfo(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT*  pIn,   ///< [in] input structure
   const ADDR2_COMPUTE_SURFACE_INFO_OUTPUT* pOut   ///< [in] output structure
      {
      ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT addrIn = {};
   addrIn.size            = sizeof(addrIn);
   addrIn.swizzleMode     = pIn->swizzleMode;
   addrIn.flags           = pIn->flags;
   addrIn.flags.qbStereo  = 0;
   addrIn.resourceType    = pIn->resourceType;
   addrIn.bpp             = pIn->bpp;
   addrIn.unalignedWidth  = pIn->width;
   addrIn.numSlices       = pIn->numSlices;
   addrIn.numMipLevels    = pIn->numMipLevels;
   addrIn.numSamples      = pIn->numSamples;
            // Call Addr2ComputePipeBankXor() and validate different pbXor value if necessary...
            ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT addrOut = {};
            // Make the array to be {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096} for full test
            for (UINT_32 xIdx = 0; xIdx < sizeof(TestCoord) / sizeof(TestCoord[0]); xIdx++)
   {
      if (TestCoord[xIdx] < pIn->width)
   {
                  for (UINT_32 yIdx = 0; yIdx  < sizeof(TestCoord) / sizeof(TestCoord[0]); yIdx++)
   {
      if (TestCoord[yIdx] < pIn->height)
   {
                                                                                                            }
   #endif
      } // V2
   } // Addr
   