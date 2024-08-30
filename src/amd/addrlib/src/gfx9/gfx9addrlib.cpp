   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ************************************************************************************************************************
   * @file  gfx9addrlib.cpp
   * @brief Contgfx9ns the implementation for the Gfx9Lib class.
   ************************************************************************************************************************
   */
      #include "gfx9addrlib.h"
      #include "gfx9_gb_reg.h"
      #include "amdgpu_asic_addr.h"
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      namespace Addr
   {
      /**
   ************************************************************************************************************************
   *   Gfx9HwlInit
   *
   *   @brief
   *       Creates an Gfx9Lib object.
   *
   *   @return
   *       Returns an Gfx9Lib object pointer.
   ************************************************************************************************************************
   */
   Addr::Lib* Gfx9HwlInit(const Client* pClient)
   {
         }
      namespace V2
   {
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Static Const Member
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      const SwizzleModeFlags Gfx9Lib::SwizzleModeTable[ADDR_SW_MAX_TYPE] =
   {//Linear 256B  4KB  64KB   Var    Z    Std   Disp  Rot   XOR    T     RtOpt Reserved
      {{1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_LINEAR
   {{0,    1,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_256B_S
   {{0,    1,    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0}}, // ADDR_SW_256B_D
            {{0,    0,    1,    0,    0,    1,    0,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_4KB_Z
   {{0,    0,    1,    0,    0,    0,    1,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_4KB_S
   {{0,    0,    1,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0}}, // ADDR_SW_4KB_D
            {{0,    0,    0,    1,    0,    1,    0,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_64KB_Z
   {{0,    0,    0,    1,    0,    0,    1,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_64KB_S
   {{0,    0,    0,    1,    0,    0,    0,    1,    0,    0,    0,    0,    0}}, // ADDR_SW_64KB_D
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
            {{0,    0,    0,    1,    0,    1,    0,    0,    0,    1,    1,    0,    0}}, // ADDR_SW_64KB_Z_T
   {{0,    0,    0,    1,    0,    0,    1,    0,    0,    1,    1,    0,    0}}, // ADDR_SW_64KB_S_T
   {{0,    0,    0,    1,    0,    0,    0,    1,    0,    1,    1,    0,    0}}, // ADDR_SW_64KB_D_T
            {{0,    0,    1,    0,    0,    1,    0,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_4KB_Z_x
   {{0,    0,    1,    0,    0,    0,    1,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_4KB_S_x
   {{0,    0,    1,    0,    0,    0,    0,    1,    0,    1,    0,    0,    0}}, // ADDR_SW_4KB_D_x
            {{0,    0,    0,    1,    0,    1,    0,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_64KB_Z_X
   {{0,    0,    0,    1,    0,    0,    1,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_64KB_S_X
   {{0,    0,    0,    1,    0,    0,    0,    1,    0,    1,    0,    0,    0}}, // ADDR_SW_64KB_D_X
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
      };
      const UINT_32 Gfx9Lib::MipTailOffset256B[] = {2048, 1024, 512, 256, 128, 64, 32, 16, 8, 6, 5, 4, 3, 2, 1, 0};
      const Dim3d   Gfx9Lib::Block256_3dS[]  = {{16, 4, 4}, {8, 4, 4}, {4, 4, 4}, {2, 4, 4}, {1, 4, 4}};
      const Dim3d   Gfx9Lib::Block256_3dZ[]  = {{8, 4, 8}, {4, 4, 8}, {4, 4, 4}, {4, 2, 4}, {2, 2, 4}};
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::Gfx9Lib
   *
   *   @brief
   *       Constructor
   *
   ************************************************************************************************************************
   */
   Gfx9Lib::Gfx9Lib(const Client* pClient)
      :
      {
      memset(&m_settings, 0, sizeof(m_settings));
   memcpy(m_swizzleModeTable, SwizzleModeTable, sizeof(SwizzleModeTable));
   memset(m_cachedMetaEqKey, 0, sizeof(m_cachedMetaEqKey));
      }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::~Gfx9Lib
   *
   *   @brief
   *       Destructor
   ************************************************************************************************************************
   */
   Gfx9Lib::~Gfx9Lib()
   {
   }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeHtileInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeHtilenfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeHtileInfo(
      const ADDR2_COMPUTE_HTILE_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_HTILE_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
      UINT_32 numPipeTotal = GetPipeNumForMetaAddressing(pIn->hTileFlags.pipeAligned,
                              if ((numPipeTotal == 1) && (numRbTotal == 1))
   {
         }
   else
   {
      if (m_settings.applyAliasFix)
   {
         }
   else
   {
                              Dim3d   metaBlkDim   = {8, 8, 1};
   UINT_32 totalAmpBits = numCompressBlkPerMetaBlkLog2;
   UINT_32 widthAmp     = (pIn->numMipLevels > 1) ? (totalAmpBits >> 1) : RoundHalf(totalAmpBits);
   UINT_32 heightAmp    = totalAmpBits - widthAmp;
   metaBlkDim.w <<= widthAmp;
         #if DEBUG
      Dim3d metaBlkDimDbg = {8, 8, 1};
   for (UINT_32 index = 0; index < numCompressBlkPerMetaBlkLog2; index++)
   {
      if ((metaBlkDimDbg.h < metaBlkDimDbg.w) ||
         {
         }
   else
   {
            }
      #endif
         UINT_32 numMetaBlkX;
   UINT_32 numMetaBlkY;
            GetMetaMipInfo(pIn->numMipLevels, &metaBlkDim, FALSE, pOut->pMipInfo,
                  const UINT_32 metaBlkSize = numCompressBlkPerMetaBlk << 2;
            if ((IsXor(pIn->swizzleMode) == FALSE) && (numPipeTotal > 2))
   {
                           if (m_settings.metaBaseAlignFix)
   {
                  if (m_settings.htileAlignFix)
   {
      const INT_32 metaBlkSizeLog2        = numCompressBlkPerMetaBlkLog2 + 2;
   const INT_32 htileCachelineSizeLog2 = 11;
                                 pOut->pitch      = numMetaBlkX * metaBlkDim.w;
   pOut->height     = numMetaBlkY * metaBlkDim.h;
            pOut->metaBlkWidth       = metaBlkDim.w;
   pOut->metaBlkHeight      = metaBlkDim.h;
            pOut->baseAlign  = align;
               }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeCmaskInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskInfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeCmaskInfo(
      const ADDR2_COMPUTE_CMASK_INFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_CMASK_INFO_OUTPUT*         pOut    ///< [out] output structure
      {
               UINT_32 numPipeTotal = GetPipeNumForMetaAddressing(pIn->cMaskFlags.pipeAligned,
                              if ((numPipeTotal == 1) && (numRbTotal == 1))
   {
         }
   else
   {
      if (m_settings.applyAliasFix)
   {
         }
   else
   {
                                       Dim2d metaBlkDim = {8, 8};
   UINT_32 totalAmpBits = numCompressBlkPerMetaBlkLog2;
   UINT_32 heightAmp = totalAmpBits >> 1;
   UINT_32 widthAmp = totalAmpBits - heightAmp;
   metaBlkDim.w <<= widthAmp;
         #if DEBUG
      Dim2d metaBlkDimDbg = {8, 8};
   for (UINT_32 index = 0; index < numCompressBlkPerMetaBlkLog2; index++)
   {
      if (metaBlkDimDbg.h < metaBlkDimDbg.w)
   {
         }
   else
   {
            }
      #endif
         UINT_32 numMetaBlkX = (pIn->unalignedWidth  + metaBlkDim.w - 1) / metaBlkDim.w;
   UINT_32 numMetaBlkY = (pIn->unalignedHeight + metaBlkDim.h - 1) / metaBlkDim.h;
                     if (m_settings.metaBaseAlignFix)
   {
                  pOut->pitch      = numMetaBlkX * metaBlkDim.w;
   pOut->height     = numMetaBlkY * metaBlkDim.h;
   pOut->sliceSize  = (numMetaBlkX * numMetaBlkY * numCompressBlkPerMetaBlk) >> 1;
   pOut->cmaskBytes = PowTwoAlign(pOut->sliceSize * numMetaBlkZ, sizeAlign);
            pOut->metaBlkWidth = metaBlkDim.w;
                     // Get the CMASK address equation (copied from CmaskAddrFromCoord)
   UINT_32 fmaskBpp              = GetFmaskBpp(1, 1);
   UINT_32 fmaskElementBytesLog2 = Log2(fmaskBpp >> 3);
   UINT_32 metaBlkWidthLog2      = Log2(pOut->metaBlkWidth);
            MetaEqParams metaEqParams = {0, fmaskElementBytesLog2, 0, pIn->cMaskFlags,
                           // Generate the CMASK address equation.
   pOut->equation.gfx9.num_bits = Min(32u, eq->getsize());
   bool checked = false;
   for (unsigned b = 0; b < pOut->equation.gfx9.num_bits; b++) {
               unsigned c;
   for (c = 0; c < bit.getsize(); c++) {
      Coordinate &coord = bit[c];
   pOut->equation.gfx9.bit[b].coord[c].dim = coord.getdim();
      }
   for (; c < 5; c++)
               // Reduce num_bits because DIM_M fills the rest of the bits monotonically.
   for (int b = pOut->equation.gfx9.num_bits - 1; b >= 1; b--) {
      CoordTerm &prev = (*eq)[b - 1];
            if (cur.getsize() == 1 && cur[0].getdim() == DIM_M &&
      prev.getsize() == 1 && prev[0].getdim() == DIM_M &&
   prev[0].getord() + 1 == cur[0].getord())
      else
               pOut->equation.gfx9.numPipeBits = GetPipeLog2ForMetaAddressing(pIn->cMaskFlags.pipeAligned,
               }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetMetaMipInfo
   *
   *   @brief
   *       Get meta mip info
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::GetMetaMipInfo(
      UINT_32 numMipLevels,           ///< [in]  number of mip levels
   Dim3d* pMetaBlkDim,             ///< [in]  meta block dimension
   BOOL_32 dataThick,              ///< [in]  data surface is thick
   ADDR2_META_MIP_INFO* pInfo,     ///< [out] meta mip info
   UINT_32 mip0Width,              ///< [in]  mip0 width
   UINT_32 mip0Height,             ///< [in]  mip0 height
   UINT_32 mip0Depth,              ///< [in]  mip0 depth
   UINT_32* pNumMetaBlkX,          ///< [out] number of metablock X in mipchain
   UINT_32* pNumMetaBlkY,          ///< [out] number of metablock Y in mipchain
   UINT_32* pNumMetaBlkZ)          ///< [out] number of metablock Z in mipchain
      {
      UINT_32 numMetaBlkX = (mip0Width  + pMetaBlkDim->w - 1) / pMetaBlkDim->w;
   UINT_32 numMetaBlkY = (mip0Height + pMetaBlkDim->h - 1) / pMetaBlkDim->h;
   UINT_32 numMetaBlkZ = (mip0Depth  + pMetaBlkDim->d - 1) / pMetaBlkDim->d;
   UINT_32 tailWidth   = pMetaBlkDim->w;
   UINT_32 tailHeight  = pMetaBlkDim->h >> 1;
   UINT_32 tailDepth   = pMetaBlkDim->d;
   BOOL_32 inTail      = FALSE;
            if (numMipLevels > 1)
   {
      if (dataThick && (numMetaBlkZ > numMetaBlkX) && (numMetaBlkZ > numMetaBlkY))
   {
         // Z major
   }
   else if (numMetaBlkX >= numMetaBlkY)
   {
         // X major
   }
   else
   {
         // Y major
            inTail = ((mip0Width <= tailWidth) &&
                  if (inTail == FALSE)
   {
         UINT_32 orderLimit;
                  if (major == ADDR_MAJOR_Z)
   {
      // Z major
   pMipDim = &numMetaBlkY;
   pOrderDim = &numMetaBlkZ;
      }
   else if (major == ADDR_MAJOR_X)
   {
      // X major
   pMipDim = &numMetaBlkY;
   pOrderDim = &numMetaBlkX;
      }
   else
   {
      // Y major
   pMipDim = &numMetaBlkX;
                     if ((*pMipDim < 3) && (*pOrderDim > orderLimit) && (numMipLevels > 3))
   {
         }
   else
   {
                     if (pInfo != NULL)
   {
      UINT_32 mipWidth  = mip0Width;
   UINT_32 mipHeight = mip0Height;
   UINT_32 mipDepth  = mip0Depth;
            for (UINT_32 mip = 0; mip < numMipLevels; mip++)
   {
         if (inTail)
   {
      GetMetaMiptailInfo(&pInfo[mip], mipCoord, numMipLevels - mip,
            }
   else
   {
                           pInfo[mip].inMiptail = FALSE;
   pInfo[mip].startX = mipCoord.w;
   pInfo[mip].startY = mipCoord.h;
   pInfo[mip].startZ = mipCoord.d;
                        if ((mip >= 3) || (mip & 1))
   {
      switch (major)
   {
         case ADDR_MAJOR_X:
      mipCoord.w += mipWidth;
      case ADDR_MAJOR_Y:
      mipCoord.h += mipHeight;
      case ADDR_MAJOR_Z:
      mipCoord.d += mipDepth;
      default:
      }
   else
   {
      switch (major)
   {
         case ADDR_MAJOR_X:
      mipCoord.h += mipHeight;
      case ADDR_MAJOR_Y:
      mipCoord.w += mipWidth;
      case ADDR_MAJOR_Z:
      mipCoord.h += mipHeight;
                                             inTail = ((mipWidth <= tailWidth) &&
                        *pNumMetaBlkX = numMetaBlkX;
   *pNumMetaBlkY = numMetaBlkY;
      }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeDccInfo
   *
   *   @brief
   *       Interface function to compute DCC key info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeDccInfo(
      const ADDR2_COMPUTE_DCCINFO_INPUT*    pIn,    ///< [in] input structure
   ADDR2_COMPUTE_DCCINFO_OUTPUT*         pOut    ///< [out] output structure
      {
      BOOL_32 dataLinear = IsLinear(pIn->swizzleMode);
   BOOL_32 metaLinear = pIn->dccKeyFlags.linear;
            if (dataLinear)
   {
         }
   else if (metaLinear == TRUE)
   {
                           if (metaLinear)
   {
      // Linear metadata supporting was removed for GFX9! No one can use this feature on GFX9.
            pOut->dccRamBaseAlign = numPipeTotal * m_pipeInterleaveBytes;
      }
   else
   {
                        UINT_32 numFrags = Max(pIn->numFrags, 1u);
                                       if ((numPipeTotal > 1) || (numRbTotal > 1))
   {
                                 if (numCompressBlkPerMetaBlk > 65536 * pIn->bpp)
   {
                  Dim3d compressBlkDim = GetDccCompressBlk(pIn->resourceType, pIn->swizzleMode, pIn->bpp);
            for (UINT_32 index = 1; index < numCompressBlkPerMetaBlk; index <<= 1)
   {
         if ((metaBlkDim.h < metaBlkDim.w) ||
         {
      if ((dataThick == FALSE) || (metaBlkDim.h <= metaBlkDim.d))
   {
         }
   else
   {
            }
   else
   {
      if ((dataThick == FALSE) || (metaBlkDim.w <= metaBlkDim.d))
   {
         }
   else
   {
                     UINT_32 numMetaBlkX;
   UINT_32 numMetaBlkY;
            GetMetaMipInfo(pIn->numMipLevels, &metaBlkDim, dataThick, pOut->pMipInfo,
                           if (numFrags > m_maxCompFrag)
   {
                  if (m_settings.metaBaseAlignFix)
   {
                  pOut->dccRamSize = numMetaBlkX * numMetaBlkY * numMetaBlkZ *
         pOut->dccRamSize = PowTwoAlign(pOut->dccRamSize, sizeAlign);
            pOut->pitch = numMetaBlkX * metaBlkDim.w;
   pOut->height = numMetaBlkY * metaBlkDim.h;
            pOut->compressBlkWidth = compressBlkDim.w;
   pOut->compressBlkHeight = compressBlkDim.h;
            pOut->metaBlkWidth = metaBlkDim.w;
   pOut->metaBlkHeight = metaBlkDim.h;
   pOut->metaBlkDepth = metaBlkDim.d;
            pOut->metaBlkNumPerSlice = numMetaBlkX * numMetaBlkY;
   pOut->fastClearSizePerSlice =
            // Get the DCC address equation (copied from DccAddrFromCoord)
   UINT_32 elementBytesLog2  = Log2(pIn->bpp >> 3);
   UINT_32 numSamplesLog2    = Log2(pIn->numFrags);
   UINT_32 metaBlkWidthLog2  = Log2(pOut->metaBlkWidth);
   UINT_32 metaBlkHeightLog2 = Log2(pOut->metaBlkHeight);
   UINT_32 metaBlkDepthLog2  = Log2(pOut->metaBlkDepth);
   UINT_32 compBlkWidthLog2  = Log2(pOut->compressBlkWidth);
   UINT_32 compBlkHeightLog2 = Log2(pOut->compressBlkHeight);
            MetaEqParams metaEqParams = {0, elementBytesLog2, numSamplesLog2, pIn->dccKeyFlags,
                                 // Generate the DCC address equation.
   pOut->equation.gfx9.num_bits = Min(32u, eq->getsize());
   bool checked = false;
   for (unsigned b = 0; b < pOut->equation.gfx9.num_bits; b++) {
               unsigned c;
   for (c = 0; c < bit.getsize(); c++) {
      Coordinate &coord = bit[c];
   pOut->equation.gfx9.bit[b].coord[c].dim = coord.getdim();
      }
   for (; c < 5; c++)
               // Reduce num_bits because DIM_M fills the rest of the bits monotonically.
   for (int b = pOut->equation.gfx9.num_bits - 1; b >= 1; b--) {
                     if (cur.getsize() == 1 && cur[0].getdim() == DIM_M &&
         prev.getsize() == 1 && prev[0].getdim() == DIM_M &&
         else
               pOut->equation.gfx9.numPipeBits = GetPipeLog2ForMetaAddressing(pIn->dccKeyFlags.pipeAligned,
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeMaxBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments
   *   @return
   *       maximum alignments
   ************************************************************************************************************************
   */
   UINT_32 Gfx9Lib::HwlComputeMaxBaseAlignments() const
   {
         }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeMaxMetaBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments for metadata
   *   @return
   *       maximum alignments for metadata
   ************************************************************************************************************************
   */
   UINT_32 Gfx9Lib::HwlComputeMaxMetaBaseAlignments() const
   {
      // Max base alignment for Htile
   const UINT_32 maxNumPipeTotal = GetPipeNumForMetaAddressing(TRUE, ADDR_SW_64KB_Z);
            // If applyAliasFix was set, the extra bits should be MAX(10u, m_pipeInterleaveLog2),
   // but we never saw any ASIC whose m_pipeInterleaveLog2 != 8, so just put an assertion and simply the logic.
   ADDR_ASSERT((m_settings.applyAliasFix == FALSE) || (m_pipeInterleaveLog2 <= 10u));
                     if (maxNumPipeTotal > 2)
   {
                           if (m_settings.metaBaseAlignFix)
   {
                  if (m_settings.htileAlignFix)
   {
                           // Max base alignment for 2D Dcc will not be larger than that for 3D, no need to calculate
            if ((maxNumPipeTotal > 1) || (maxNumRbTotal > 1))
   {
                  // Max base alignment for Msaa Dcc
            if (m_settings.metaBaseAlignFix)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeCmaskAddrFromCoord(
      const ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
      {
      ADDR2_COMPUTE_CMASK_INFO_INPUT input = {0};
   input.size            = sizeof(input);
   input.cMaskFlags      = pIn->cMaskFlags;
   input.colorFlags      = pIn->colorFlags;
   input.unalignedWidth  = Max(pIn->unalignedWidth, 1u);
   input.unalignedHeight = Max(pIn->unalignedHeight, 1u);
   input.numSlices       = Max(pIn->numSlices, 1u);
   input.swizzleMode     = pIn->swizzleMode;
            ADDR2_COMPUTE_CMASK_INFO_OUTPUT output = {0};
                     if (returnCode == ADDR_OK)
   {
      UINT_32 fmaskBpp              = GetFmaskBpp(pIn->numSamples, pIn->numFrags);
   UINT_32 fmaskElementBytesLog2 = Log2(fmaskBpp >> 3);
   UINT_32 metaBlkWidthLog2      = Log2(output.metaBlkWidth);
            MetaEqParams metaEqParams = {0, fmaskElementBytesLog2, 0, pIn->cMaskFlags,
                           UINT_32 xb = pIn->x / output.metaBlkWidth;
   UINT_32 yb = pIn->y / output.metaBlkHeight;
            UINT_32 pitchInBlock     = output.pitch / output.metaBlkWidth;
   UINT_32 sliceSizeInBlock = (output.height / output.metaBlkHeight) * pitchInBlock;
            UINT_32 coords[] = {pIn->x, pIn->y, pIn->slice, 0, blockIndex};
            pOut->addr = address >> 1;
               UINT_32 numPipeBits = GetPipeLog2ForMetaAddressing(pIn->cMaskFlags.pipeAligned,
                                    }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeHtileAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeHtileAddrFromCoord(
      const ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*   pIn,    ///< [in] input structure
      {
               if (pIn->numMipLevels > 1)
   {
         }
   else
   {
      ADDR2_COMPUTE_HTILE_INFO_INPUT input = {0};
   input.size            = sizeof(input);
   input.hTileFlags      = pIn->hTileFlags;
   input.depthFlags      = pIn->depthflags;
   input.swizzleMode     = pIn->swizzleMode;
   input.unalignedWidth  = Max(pIn->unalignedWidth, 1u);
   input.unalignedHeight = Max(pIn->unalignedHeight, 1u);
   input.numSlices       = Max(pIn->numSlices, 1u);
            ADDR2_COMPUTE_HTILE_INFO_OUTPUT output = {0};
                     if (returnCode == ADDR_OK)
   {
         UINT_32 elementBytesLog2  = Log2(pIn->bpp >> 3);
   UINT_32 metaBlkWidthLog2  = Log2(output.metaBlkWidth);
                  MetaEqParams metaEqParams = {0, elementBytesLog2, numSamplesLog2, pIn->hTileFlags,
                           UINT_32 xb = pIn->x / output.metaBlkWidth;
                  UINT_32 pitchInBlock     = output.pitch / output.metaBlkWidth;
                                                                                 }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeHtileCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeHtileCoordFromAddr(
      const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT*   pIn,    ///< [in] input structure
      {
               if (pIn->numMipLevels > 1)
   {
         }
   else
   {
      ADDR2_COMPUTE_HTILE_INFO_INPUT input = {0};
   input.size            = sizeof(input);
   input.hTileFlags      = pIn->hTileFlags;
   input.swizzleMode     = pIn->swizzleMode;
   input.unalignedWidth  = Max(pIn->unalignedWidth, 1u);
   input.unalignedHeight = Max(pIn->unalignedHeight, 1u);
   input.numSlices       = Max(pIn->numSlices, 1u);
            ADDR2_COMPUTE_HTILE_INFO_OUTPUT output = {0};
                     if (returnCode == ADDR_OK)
   {
         UINT_32 elementBytesLog2  = Log2(pIn->bpp >> 3);
   UINT_32 metaBlkWidthLog2  = Log2(output.metaBlkWidth);
                  MetaEqParams metaEqParams = {0, elementBytesLog2, numSamplesLog2, pIn->hTileFlags,
                                                                                          pOut->slice = coords[DIM_M] / sliceSizeInBlock;
   pOut->y     = ((coords[DIM_M] % sliceSizeInBlock) / pitchInBlock) * output.metaBlkHeight + coords[DIM_Y];
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlSupportComputeDccAddrFromCoord
   *
   *   @brief
   *       Check whether HwlComputeDccAddrFromCoord() can be done for the input parameter
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlSupportComputeDccAddrFromCoord(
         {
               if ((pIn->numMipLevels > 1) || (pIn->mipId > 1) || pIn->dccKeyFlags.linear)
   {
         }
   else if ((pIn->pitch == 0)             ||
            (pIn->height == 0)            ||
   (pIn->compressBlkWidth == 0)  ||
   (pIn->compressBlkHeight == 0) ||
   (pIn->compressBlkDepth == 0)  ||
   (pIn->metaBlkWidth == 0)      ||
   (pIn->metaBlkHeight == 0)     ||
      {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeDccAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeDccAddrFromCoord
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::HwlComputeDccAddrFromCoord(
      const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT*  pIn,
      {
      UINT_32 elementBytesLog2  = Log2(pIn->bpp >> 3);
   UINT_32 numSamplesLog2    = Log2(pIn->numFrags);
   UINT_32 metaBlkWidthLog2  = Log2(pIn->metaBlkWidth);
   UINT_32 metaBlkHeightLog2 = Log2(pIn->metaBlkHeight);
   UINT_32 metaBlkDepthLog2  = Log2(pIn->metaBlkDepth);
   UINT_32 compBlkWidthLog2  = Log2(pIn->compressBlkWidth);
   UINT_32 compBlkHeightLog2 = Log2(pIn->compressBlkHeight);
            MetaEqParams metaEqParams = {pIn->mipId, elementBytesLog2, numSamplesLog2, pIn->dccKeyFlags,
                                 UINT_32 xb = pIn->x / pIn->metaBlkWidth;
   UINT_32 yb = pIn->y / pIn->metaBlkHeight;
            UINT_32 pitchInBlock     = pIn->pitch / pIn->metaBlkWidth;
   UINT_32 sliceSizeInBlock = (pIn->height / pIn->metaBlkHeight) * pitchInBlock;
            UINT_32 coords[] = {pIn->x, pIn->y, pIn->slice, pIn->sample, blockIndex};
                     UINT_32 numPipeBits = GetPipeLog2ForMetaAddressing(pIn->dccKeyFlags.pipeAligned,
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlInitGlobalParams
   *
   *   @brief
   *       Initializes global parameters
   *
   *   @return
   *       TRUE if all settings are valid
   *
   ************************************************************************************************************************
   */
   BOOL_32 Gfx9Lib::HwlInitGlobalParams(
         {
               if (m_settings.isArcticIsland)
   {
                        // These values are copied from CModel code
   switch (gbAddrConfig.bits.NUM_PIPES)
   {
         case ADDR_CONFIG_1_PIPE:
      m_pipes = 1;
   m_pipesLog2 = 0;
      case ADDR_CONFIG_2_PIPE:
      m_pipes = 2;
   m_pipesLog2 = 1;
      case ADDR_CONFIG_4_PIPE:
      m_pipes = 4;
   m_pipesLog2 = 2;
      case ADDR_CONFIG_8_PIPE:
      m_pipes = 8;
   m_pipesLog2 = 3;
      case ADDR_CONFIG_16_PIPE:
      m_pipes = 16;
   m_pipesLog2 = 4;
      case ADDR_CONFIG_32_PIPE:
      m_pipes = 32;
   m_pipesLog2 = 5;
      default:
                  switch (gbAddrConfig.bits.PIPE_INTERLEAVE_SIZE)
   {
         case ADDR_CONFIG_PIPE_INTERLEAVE_256B:
      m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_256B;
   m_pipeInterleaveLog2 = 8;
      case ADDR_CONFIG_PIPE_INTERLEAVE_512B:
      m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_512B;
   m_pipeInterleaveLog2 = 9;
      case ADDR_CONFIG_PIPE_INTERLEAVE_1KB:
      m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_1KB;
   m_pipeInterleaveLog2 = 10;
      case ADDR_CONFIG_PIPE_INTERLEAVE_2KB:
      m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_2KB;
   m_pipeInterleaveLog2 = 11;
      default:
                  // Addr::V2::Lib::ComputePipeBankXor()/ComputeSlicePipeBankXor() requires pipe interleave to be exactly 8 bits,
   // and any larger value requires a post-process (left shift) on the output pipeBankXor bits.
            switch (gbAddrConfig.bits.NUM_BANKS)
   {
         case ADDR_CONFIG_1_BANK:
      m_banks = 1;
   m_banksLog2 = 0;
      case ADDR_CONFIG_2_BANK:
      m_banks = 2;
   m_banksLog2 = 1;
      case ADDR_CONFIG_4_BANK:
      m_banks = 4;
   m_banksLog2 = 2;
      case ADDR_CONFIG_8_BANK:
      m_banks = 8;
   m_banksLog2 = 3;
      case ADDR_CONFIG_16_BANK:
      m_banks = 16;
   m_banksLog2 = 4;
      default:
                  switch (gbAddrConfig.bits.NUM_SHADER_ENGINES)
   {
         case ADDR_CONFIG_1_SHADER_ENGINE:
      m_se = 1;
   m_seLog2 = 0;
      case ADDR_CONFIG_2_SHADER_ENGINE:
      m_se = 2;
   m_seLog2 = 1;
      case ADDR_CONFIG_4_SHADER_ENGINE:
      m_se = 4;
   m_seLog2 = 2;
      case ADDR_CONFIG_8_SHADER_ENGINE:
      m_se = 8;
   m_seLog2 = 3;
      default:
                  switch (gbAddrConfig.bits.NUM_RB_PER_SE)
   {
         case ADDR_CONFIG_1_RB_PER_SHADER_ENGINE:
      m_rbPerSe = 1;
   m_rbPerSeLog2 = 0;
      case ADDR_CONFIG_2_RB_PER_SHADER_ENGINE:
      m_rbPerSe = 2;
   m_rbPerSeLog2 = 1;
      case ADDR_CONFIG_4_RB_PER_SHADER_ENGINE:
      m_rbPerSe = 4;
   m_rbPerSeLog2 = 2;
      default:
                  switch (gbAddrConfig.bits.MAX_COMPRESSED_FRAGS)
   {
         case ADDR_CONFIG_1_MAX_COMPRESSED_FRAGMENTS:
      m_maxCompFrag = 1;
   m_maxCompFragLog2 = 0;
      case ADDR_CONFIG_2_MAX_COMPRESSED_FRAGMENTS:
      m_maxCompFrag = 2;
   m_maxCompFragLog2 = 1;
      case ADDR_CONFIG_4_MAX_COMPRESSED_FRAGMENTS:
      m_maxCompFrag = 4;
   m_maxCompFragLog2 = 2;
      case ADDR_CONFIG_8_MAX_COMPRESSED_FRAGMENTS:
      m_maxCompFrag = 8;
   m_maxCompFragLog2 = 3;
      default:
                  if ((m_rbPerSeLog2 == 1) &&
         (((m_pipesLog2 == 1) && ((m_seLog2 == 2) || (m_seLog2 == 3))) ||
   {
                                    if (m_settings.isVega12)
   {
                  // For simplicity we never allow VAR swizzle mode for GFX9, the actural value is 18 on GFX9
      }
   else
   {
      valid = FALSE;
               if (valid)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlConvertChipFamily
   *
   *   @brief
   *       Convert familyID defined in atiid.h to ChipFamily and set m_chipFamily/m_chipRevision
   *   @return
   *       ChipFamily
   ************************************************************************************************************************
   */
   ChipFamily Gfx9Lib::HwlConvertChipFamily(
      UINT_32 uChipFamily,        ///< [in] chip family defined in atiih.h
      {
               switch (uChipFamily)
   {
      case FAMILY_AI:
         m_settings.isArcticIsland = 1;
   m_settings.isVega10 = ASICREV_IS_VEGA10_P(uChipRevision);
   m_settings.isVega12 = ASICREV_IS_VEGA12_P(uChipRevision);
                  if (m_settings.isVega10 == 0)
   {
                                 m_settings.depthPipeXorDisable = 1;
   case FAMILY_RV:
                  if (ASICREV_IS_RAVEN(uChipRevision))
                                 if (ASICREV_IS_RAVEN2(uChipRevision))
   {
                  if (m_settings.isRaven == 0)
   {
                                 if (ASICREV_IS_RENOIR(uChipRevision))
   {
                                 default:
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::InitRbEquation
   *
   *   @brief
   *       Init RB equation
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::GetRbEquation(
      CoordEq* pRbEq,             ///< [out] rb equation
   UINT_32  numRbPerSeLog2,    ///< [in] number of rb per shader engine
   UINT_32  numSeLog2)         ///< [in] number of shader engine
      {
      // RB's are distributed on 16x16, except when we have 1 rb per se, in which case its 32x32
   UINT_32 rbRegion = (numRbPerSeLog2 == 0) ? 5 : 4;
   Coordinate cx(DIM_X, rbRegion);
            UINT_32 start = 0;
            // Clear the rb equation
   pRbEq->resize(0);
            if ((numSeLog2 > 0) && (numRbPerSeLog2 == 1))
   {
      // Special case when more than 1 SE, and 2 RB per SE
   (*pRbEq)[0].add(cx);
   (*pRbEq)[0].add(cy);
   cx++;
            if (m_settings.applyAliasFix == false)
   {
                  (*pRbEq)[0].add(cy);
                        for (UINT_32 i = 0; i < numBits; i++)
   {
      UINT_32 idx =
            if ((i % 2) == 1)
   {
         (*pRbEq)[idx].add(cx);
   }
   else
   {
         (*pRbEq)[idx].add(cy);
         }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetDataEquation
   *
   *   @brief
   *       Get data equation for fmask and Z
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::GetDataEquation(
      CoordEq* pDataEq,               ///< [out] data surface equation
   Gfx9DataType dataSurfaceType,   ///< [in] data surface type
   AddrSwizzleMode swizzleMode,    ///< [in] data surface swizzle mode
   AddrResourceType resourceType,  ///< [in] data surface resource type
   UINT_32 elementBytesLog2,       ///< [in] data surface element bytes
   UINT_32 numSamplesLog2)         ///< [in] data surface sample count
      {
      Coordinate cx(DIM_X, 0);
   Coordinate cy(DIM_Y, 0);
   Coordinate cz(DIM_Z, 0);
            // Clear the equation
   pDataEq->resize(0);
            if (dataSurfaceType == Gfx9DataColor)
   {
      if (IsLinear(swizzleMode))
   {
                           for (UINT_32 i = 0; i < 49; i++)
   {
      (*pDataEq)[i].add(cm);
      }
   else if (IsThick(resourceType, swizzleMode))
   {
         // Color 3d_S and 3d_Z modes, 3d_D is same as color 2d
   UINT_32 i;
   if (IsStandardSwizzle(resourceType, swizzleMode))
   {
      // Standard 3d swizzle
   // Fill in bottom x bits
   for (i = elementBytesLog2; i < 4; i++)
   {
      (*pDataEq)[i].add(cx);
      }
   // Fill in 2 bits of y and then z
   for (i = 4; i < 6; i++)
   {
      (*pDataEq)[i].add(cy);
      }
   for (i = 6; i < 8; i++)
   {
      (*pDataEq)[i].add(cz);
      }
   if (elementBytesLog2 < 2)
   {
      // fill in z & y bit
   (*pDataEq)[8].add(cz);
   (*pDataEq)[9].add(cy);
   cz++;
      }
   else if (elementBytesLog2 == 2)
   {
      // fill in y and x bit
   (*pDataEq)[8].add(cy);
   (*pDataEq)[9].add(cx);
   cy++;
      }
   else
   {
      // fill in 2 x bits
   (*pDataEq)[8].add(cx);
   cx++;
   (*pDataEq)[9].add(cx);
         }
   else
   {
      // Z 3d swizzle
   UINT_32 m2dEnd = (elementBytesLog2 ==0) ? 3 : ((elementBytesLog2 < 4) ? 4 : 5);
   UINT_32 numZs = (elementBytesLog2 == 0 || elementBytesLog2 == 4) ?
         pDataEq->mort2d(cx, cy, elementBytesLog2, m2dEnd);
   for (i = m2dEnd + 1; i <= m2dEnd + numZs; i++)
   {
      (*pDataEq)[i].add(cz);
      }
   if ((elementBytesLog2 == 0) || (elementBytesLog2 == 3))
   {
      // add an x and z
   (*pDataEq)[6].add(cx);
   (*pDataEq)[7].add(cz);
   cx++;
      }
   else if (elementBytesLog2 == 2)
   {
      // add a y and z
   (*pDataEq)[6].add(cy);
   (*pDataEq)[7].add(cz);
   cy++;
      }
   // add y and x
   (*pDataEq)[8].add(cy);
   (*pDataEq)[9].add(cx);
   cy++;
      }
   // Fill in bit 10 and up
   }
   else if (IsThin(resourceType, swizzleMode))
   {
         UINT_32 blockSizeLog2 = GetBlockSizeLog2(swizzleMode);
   // Color 2D
   UINT_32 microYBits = (8 - elementBytesLog2) / 2;
   UINT_32 tileSplitStart = blockSizeLog2 - numSamplesLog2;
   UINT_32 i;
   // Fill in bottom x bits
   for (i = elementBytesLog2; i < 4; i++)
   {
      (*pDataEq)[i].add(cx);
      }
   // Fill in bottom y bits
   for (i = 4; i < 4 + microYBits; i++)
   {
      (*pDataEq)[i].add(cy);
      }
   // Fill in last of the micro_x bits
   for (i = 4 + microYBits; i < 8; i++)
   {
      (*pDataEq)[i].add(cx);
      }
   // Fill in x/y bits below sample split
   pDataEq->mort2d(cy, cx, 8, tileSplitStart - 1);
   // Fill in sample bits
   for (i = 0; i < numSamplesLog2; i++)
   {
      cs.set(DIM_S, i);
      }
   // Fill in x/y bits above sample split
   if ((numSamplesLog2 & 1) ^ (blockSizeLog2 & 1))
   {
         }
   else
   {
         }
   else
   {
            }
   else
   {
      // Fmask or depth
   UINT_32 sampleStart = elementBytesLog2;
   UINT_32 pixelStart = elementBytesLog2 + numSamplesLog2;
            for (UINT_32 s = 0; s < numSamplesLog2; s++)
   {
         cs.set(DIM_S, s);
            // Put in the x-major order pixel bits
   pDataEq->mort2d(cx, cy, pixelStart, ymajStart - 1);
   // Put in the y-major order pixel bits
         }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetPipeEquation
   *
   *   @brief
   *       Get pipe equation
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::GetPipeEquation(
      CoordEq*         pPipeEq,            ///< [out] pipe equation
   CoordEq*         pDataEq,            ///< [in] data equation
   UINT_32          pipeInterleaveLog2, ///< [in] pipe interleave
   UINT_32          numPipeLog2,        ///< [in] number of pipes
   UINT_32          numSamplesLog2,     ///< [in] data surface sample count
   Gfx9DataType     dataSurfaceType,    ///< [in] data surface type
   AddrSwizzleMode  swizzleMode,        ///< [in] data surface swizzle mode
   AddrResourceType resourceType        ///< [in] data surface resource type
      {
      UINT_32 blockSizeLog2 = GetBlockSizeLog2(swizzleMode);
                     if (dataSurfaceType == Gfx9DataColor)
   {
      INT_32 shift = static_cast<INT_32>(numSamplesLog2);
                        // This section should only apply to z/stencil, maybe fmask
   // If the pipe bit is below the comp block size,
   // then keep moving up the address until we find a bit that is above
            if (dataSurfaceType != Gfx9DataColor)
   {
               while (dataEq[pipeInterleaveLog2 + pipeStart][0] < tileMin)
   {
                  // if pipe is 0, then the first pipe bit is above the comp block size,
   // so we don't need to do anything
   // Note, this if condition is not necessary, since if we execute the loop when pipe==0,
   // we will get the same pipe equation
   if (pipeStart != 0)
   {
         for (UINT_32 i = 0; i < numPipeLog2; i++)
   {
      // Copy the jth bit above pipe interleave to the current pipe equation bit
                  if (IsPrt(swizzleMode))
   {
      // Clear out bits above the block size if prt's are enabled
   dataEq.resize(blockSizeLog2);
               if (IsXor(swizzleMode))
   {
               if (IsThick(resourceType, swizzleMode))
   {
                                    for (UINT_32 pipeIdx = 0; pipeIdx < numPipeLog2; pipeIdx++)
   {
      xorMask[pipeIdx].add(xorMask2[2 * pipeIdx]);
      }
   else
   {
                        if ((numSamplesLog2 == 0) && (IsPrt(swizzleMode) == FALSE))
   {
      Coordinate co;
   CoordEq xorMask2;
   // if 1xaa and not prt, then xor in the z bits
   xorMask2.resize(0);
   xorMask2.resize(numPipeLog2);
   for (UINT_32 pipeIdx = 0; pipeIdx < numPipeLog2; pipeIdx++)
   {
                                    xorMask.reverse();
         }
   /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetMetaEquation
   *
   *   @brief
   *       Get meta equation for cmask/htile/DCC
   *   @return
   *       Pointer to a calculated meta equation
   ************************************************************************************************************************
   */
   const CoordEq* Gfx9Lib::GetMetaEquation(
         {
               for (cachedMetaEqIndex = 0; cachedMetaEqIndex < MaxCachedMetaEq; cachedMetaEqIndex++)
   {
      if (memcmp(&metaEqParams,
               {
                              if (cachedMetaEqIndex < MaxCachedMetaEq)
   {
         }
   else
   {
                                 GenMetaEquation(pMetaEq,
                     metaEqParams.maxMip,
   metaEqParams.elementBytesLog2,
   metaEqParams.numSamplesLog2,
   metaEqParams.metaFlag,
   metaEqParams.dataSurfaceType,
   metaEqParams.swizzleMode,
   metaEqParams.resourceType,
   metaEqParams.metaBlkWidthLog2,
   metaEqParams.metaBlkHeightLog2,
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GenMetaEquation
   *
   *   @brief
   *       Get meta equation for cmask/htile/DCC
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::GenMetaEquation(
      CoordEq*         pMetaEq,               ///< [out] meta equation
   UINT_32          maxMip,                ///< [in] max mip Id
   UINT_32          elementBytesLog2,      ///< [in] data surface element bytes
   UINT_32          numSamplesLog2,        ///< [in] data surface sample count
   ADDR2_META_FLAGS metaFlag,              ///< [in] meta falg
   Gfx9DataType     dataSurfaceType,       ///< [in] data surface type
   AddrSwizzleMode  swizzleMode,           ///< [in] data surface swizzle mode
   AddrResourceType resourceType,          ///< [in] data surface resource type
   UINT_32          metaBlkWidthLog2,      ///< [in] meta block width
   UINT_32          metaBlkHeightLog2,     ///< [in] meta block height
   UINT_32          metaBlkDepthLog2,      ///< [in] meta block depth
   UINT_32          compBlkWidthLog2,      ///< [in] compress block width
   UINT_32          compBlkHeightLog2,     ///< [in] compress block height
   UINT_32          compBlkDepthLog2)      ///< [in] compress block depth
      {
      UINT_32 numPipeTotalLog2   = GetPipeLog2ForMetaAddressing(metaFlag.pipeAligned, swizzleMode);
            // Get the correct data address and rb equation
   CoordEq dataEq;
   GetDataEquation(&dataEq, dataSurfaceType, swizzleMode, resourceType,
            // Get pipe and rb equations
   CoordEq pipeEquation;
   GetPipeEquation(&pipeEquation, &dataEq, pipeInterleaveLog2, numPipeTotalLog2,
                  if (metaFlag.linear)
   {
      // Linear metadata supporting was removed for GFX9! No one can use this feature.
                              if (IsLinear(swizzleMode))
   {
         if (metaFlag.pipeAligned)
   {
      // Remove the pipe bits
   INT_32 shift = static_cast<INT_32>(numPipeTotalLog2);
      }
                  if (metaFlag.pipeAligned)
   {
                     for (UINT_32 i = 0; i < numPipeTotalLog2; i++)
   {
                        }
   else
   {
      UINT_32 maxCompFragLog2 = static_cast<INT_32>(m_maxCompFragLog2);
   UINT_32 compFragLog2 =
                           // Make sure the metaaddr is cleared
   pMetaEq->resize(0);
            if (IsThick(resourceType, swizzleMode))
   {
         Coordinate cx(DIM_X, 0);
                  if (maxMip > 0)
   {
         }
   else
   {
         }
   else
   {
         Coordinate cx(DIM_X, 0);
                  if (maxMip > 0)
   {
         }
   else
   {
                  //------------------------------------------------------------------------------------------------------------------------
   // Put the compressible fragments at the lsb
   // the uncompressible frags will be at the msb of the micro address
   //------------------------------------------------------------------------------------------------------------------------
   for (UINT_32 s = 0; s < compFragLog2; s++)
   {
      cs.set(DIM_S, s);
               // Keep a copy of the pipe equations
   CoordEq origPipeEquation;
            Coordinate co;
   // filter out everything under the compressed block size
   co.set(DIM_X, compBlkWidthLog2);
   pMetaEq->Filter('<', co, 0, DIM_X);
   co.set(DIM_Y, compBlkHeightLog2);
   pMetaEq->Filter('<', co, 0, DIM_Y);
   co.set(DIM_Z, compBlkDepthLog2);
            // For non-color, filter out sample bits
   if (dataSurfaceType != Gfx9DataColor)
   {
         co.set(DIM_X, 0);
            // filter out everything above the metablock size
   co.set(DIM_X, metaBlkWidthLog2 - 1);
   pMetaEq->Filter('>', co, 0, DIM_X);
   co.set(DIM_Y, metaBlkHeightLog2 - 1);
   pMetaEq->Filter('>', co, 0, DIM_Y);
   co.set(DIM_Z, metaBlkDepthLog2 - 1);
            // filter out everything above the metablock size for the channel bits
   co.set(DIM_X, metaBlkWidthLog2 - 1);
   pipeEquation.Filter('>', co, 0, DIM_X);
   co.set(DIM_Y, metaBlkHeightLog2 - 1);
   pipeEquation.Filter('>', co, 0, DIM_Y);
   co.set(DIM_Z, metaBlkDepthLog2 - 1);
            // Make sure we still have the same number of channel bits
   if (pipeEquation.getsize() != numPipeTotalLog2)
   {
                  // Loop through all channel and rb bits,
   // and make sure these components exist in the metadata address
   for (UINT_32 i = 0; i < numPipeTotalLog2; i++)
   {
         for (UINT_32 j = pipeEquation[i].getsize(); j > 0; j--)
   {
      if (pMetaEq->Exists(pipeEquation[i][j - 1]) == FALSE)
   {
                     const UINT_32 numSeLog2     = metaFlag.rbAligned ? m_seLog2      : 0;
   const UINT_32 numRbPeSeLog2 = metaFlag.rbAligned ? m_rbPerSeLog2 : 0;
   const UINT_32 numRbTotalLog2 = numRbPeSeLog2 + numSeLog2;
                              for (UINT_32 i = 0; i < numRbTotalLog2; i++)
   {
         for (UINT_32 j = rbEquation[i].getsize(); j > 0; j--)
   {
      if (pMetaEq->Exists(rbEquation[i][j - 1]) == FALSE)
   {
                     if (m_settings.applyAliasFix)
   {
                  // Loop through each rb id bit; if it is equal to any of the filtered channel bits, clear it
   for (UINT_32 i = 0; i < numRbTotalLog2; i++)
   {
         for (UINT_32 j = 0; j < numPipeTotalLog2; j++)
                     if (m_settings.applyAliasFix)
                                       }
   else
                        if (isRbEquationInPipeEquation)
   {
                              // Loop through each bit of the channel, get the smallest coordinate,
   // and remove it from the metaaddr, and rb_equation
   for (UINT_32 i = 0; i < numPipeTotalLog2; i++)
   {
                  UINT_32 old_size = pMetaEq->getsize();
   pMetaEq->Filter('=', co);
   UINT_32 new_size = pMetaEq->getsize();
   if (new_size != old_size-1)
   {
         }
   pipeEquation.remove(co);
   for (UINT_32 j = 0; j < numRbTotalLog2; j++)
   {
      if (rbEquation[j].remove(co))
   {
      // if we actually removed something from this bit, then add the remaining
   // channel bits, as these can be removed for this bit
   for (UINT_32 k = 0; k < pipeEquation[i].getsize(); k++)
   {
         if (pipeEquation[i][k] != co)
   {
      rbEquation[j].add(pipeEquation[i][k]);
                     // Loop through the rb bits and see what remain;
   // filter out the smallest coordinate if it remains
   UINT_32 rbBitsLeft = 0;
   for (UINT_32 i = 0; i < numRbTotalLog2; i++)
   {
                  if (m_settings.applyAliasFix)
   {
         }
   else
   {
                  if (isRbEqAppended)
   {
      rbBitsLeft++;
   rbEquation[i].getsmallest(co);
   UINT_32 old_size = pMetaEq->getsize();
   pMetaEq->Filter('=', co);
   UINT_32 new_size = pMetaEq->getsize();
   if (new_size != old_size - 1)
   {
         }
   for (UINT_32 j = i + 1; j < numRbTotalLog2; j++)
   {
      if (rbEquation[j].remove(co))
   {
         // if we actually removed something from this bit, then add the remaining
   // rb bits, as these can be removed for this bit
   for (UINT_32 k = 0; k < rbEquation[i].getsize(); k++)
   {
      if (rbEquation[i][k] != co)
   {
      rbEquation[j].add(rbEquation[i][k]);
                        // capture the size of the metaaddr
   UINT_32 metaSize = pMetaEq->getsize();
   // resize to 49 bits...make this a nibble address
   pMetaEq->resize(49);
   // Concatenate the macro address above the current address
   for (UINT_32 i = metaSize, j = 0; i < 49; i++, j++)
   {
         co.set(DIM_M, j);
            // Multiply by meta element size (in nibbles)
   if (dataSurfaceType == Gfx9DataColor)
   {
         }
   else if (dataSurfaceType == Gfx9DataDepthStencil)
   {
                  //------------------------------------------------------------------------------------------
   // Note the pipeInterleaveLog2+1 is because address is a nibble address
   // Shift up from pipe interleave number of channel
   // and rb bits left, and uncompressed fragments
                     // Put in the channel bits
   for (UINT_32 i = 0; i < numPipeTotalLog2; i++)
   {
                  // Put in remaining rb bits
   for (UINT_32 i = 0, j = 0; j < rbBitsLeft; i = (i + 1) % numRbTotalLog2)
   {
                  if (m_settings.applyAliasFix)
   {
         }
   else
   {
                  if (isRbEqAppended)
   {
      origRbEquation[i].copyto((*pMetaEq)[pipeInterleaveLog2 + 1 + numPipeTotalLog2 + j]);
   // Mark any rb bit we add in to the rb mask
               //------------------------------------------------------------------------------------------
   // Put in the uncompressed fragment bits
   //------------------------------------------------------------------------------------------
   for (UINT_32 i = 0; i < uncompFragLog2; i++)
   {
         co.set(DIM_S, compFragLog2 + i);
         }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::IsEquationSupported
   *
   *   @brief
   *       Check if equation is supported for given swizzle mode and resource type.
   *
   *   @return
   *       TRUE if supported
   ************************************************************************************************************************
   */
   BOOL_32 Gfx9Lib::IsEquationSupported(
      AddrResourceType rsrcType,
   AddrSwizzleMode  swMode,
      {
      BOOL_32 supported = (elementBytesLog2 < MaxElementBytesLog2) &&
                        (IsValidSwMode(swMode) == TRUE) &&
   (IsLinear(swMode) == FALSE) &&
   (((IsTex2d(rsrcType) == TRUE) &&
   ((elementBytesLog2 < 4) ||
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::InitEquationTable
   *
   *   @brief
   *       Initialize Equation table.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::InitEquationTable()
   {
               // Loop all possible resource type (2D/3D)
   for (UINT_32 rsrcTypeIdx = 0; rsrcTypeIdx < MaxRsrcType; rsrcTypeIdx++)
   {
               // Loop all possible swizzle mode
   for (UINT_32 swModeIdx = 0; swModeIdx < MaxSwModeType; swModeIdx++)
   {
                  // Loop all possible bpp
   for (UINT_32 bppIdx = 0; bppIdx < MaxElementBytesLog2; bppIdx++)
                     // Check if the input is supported
   if (IsEquationSupported(rsrcType, swMode, bppIdx))
                                    // Generate the equation
   if (IsBlock256b(swMode) && IsTex2d(rsrcType))
   {
         }
   else if (IsThin(rsrcType, swMode))
   {
         }
   else
                        // Only fill the equation into the table if the return code is ADDR_OK,
   // otherwise if the return code is not ADDR_OK, it indicates this is not
   // a valid input, we do nothing but just fill invalid equation index
   // into the lookup table.
   if (retCode == ADDR_OK)
                                       }
   else
   {
                     // Fill the index into the lookup table, if the combination is not supported
   // fill the invalid equation index
            }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlGetEquationIndex
   *
   *   @brief
   *       Interface function stub of GetEquationIndex
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   UINT_32 Gfx9Lib::HwlGetEquationIndex(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut
      {
      AddrResourceType rsrcType         = pIn->resourceType;
   AddrSwizzleMode  swMode           = pIn->swizzleMode;
   UINT_32          elementBytesLog2 = Log2(pIn->bpp >> 3);
            if (IsEquationSupported(rsrcType, swMode, elementBytesLog2))
   {
      UINT_32 rsrcTypeIdx = static_cast<UINT_32>(rsrcType) - 1;
                        if (pOut->pMipInfo != NULL)
   {
      for (UINT_32 i = 0; i < pIn->numMipLevels; i++)
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeBlock256Equation
   *
   *   @brief
   *       Interface function stub of ComputeBlock256Equation
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeBlock256Equation(
      AddrResourceType rsrcType,
   AddrSwizzleMode  swMode,
   UINT_32          elementBytesLog2,
      {
               pEquation->numBits = 8;
            UINT_32 i = 0;
   for (; i < elementBytesLog2; i++)
   {
                           const UINT_32 maxBitsUsed = 4;
   ADDR_CHANNEL_SETTING x[maxBitsUsed] = {};
            for (i = 0; i < maxBitsUsed; i++)
   {
      InitChannel(1, 0, elementBytesLog2 + i, &x[i]);
               if (IsStandardSwizzle(rsrcType, swMode))
   {
      switch (elementBytesLog2)
   {
         case 0:
      pixelBit[0] = x[0];
   pixelBit[1] = x[1];
   pixelBit[2] = x[2];
   pixelBit[3] = x[3];
   pixelBit[4] = y[0];
   pixelBit[5] = y[1];
   pixelBit[6] = y[2];
   pixelBit[7] = y[3];
      case 1:
      pixelBit[0] = x[0];
   pixelBit[1] = x[1];
   pixelBit[2] = x[2];
   pixelBit[3] = y[0];
   pixelBit[4] = y[1];
   pixelBit[5] = y[2];
   pixelBit[6] = x[3];
      case 2:
      pixelBit[0] = x[0];
   pixelBit[1] = x[1];
   pixelBit[2] = y[0];
   pixelBit[3] = y[1];
   pixelBit[4] = y[2];
   pixelBit[5] = x[2];
      case 3:
      pixelBit[0] = x[0];
   pixelBit[1] = y[0];
   pixelBit[2] = y[1];
   pixelBit[3] = x[1];
   pixelBit[4] = x[2];
      case 4:
      pixelBit[0] = y[0];
   pixelBit[1] = y[1];
   pixelBit[2] = x[0];
   pixelBit[3] = x[1];
      default:
      ADDR_ASSERT_ALWAYS();
         }
   else if (IsDisplaySwizzle(rsrcType, swMode))
   {
      switch (elementBytesLog2)
   {
         case 0:
      pixelBit[0] = x[0];
   pixelBit[1] = x[1];
   pixelBit[2] = x[2];
   pixelBit[3] = y[1];
   pixelBit[4] = y[0];
   pixelBit[5] = y[2];
   pixelBit[6] = x[3];
   pixelBit[7] = y[3];
      case 1:
      pixelBit[0] = x[0];
   pixelBit[1] = x[1];
   pixelBit[2] = x[2];
   pixelBit[3] = y[0];
   pixelBit[4] = y[1];
   pixelBit[5] = y[2];
   pixelBit[6] = x[3];
      case 2:
      pixelBit[0] = x[0];
   pixelBit[1] = x[1];
   pixelBit[2] = y[0];
   pixelBit[3] = x[2];
   pixelBit[4] = y[1];
   pixelBit[5] = y[2];
      case 3:
      pixelBit[0] = x[0];
   pixelBit[1] = y[0];
   pixelBit[2] = x[1];
   pixelBit[3] = x[2];
   pixelBit[4] = y[1];
      case 4:
      pixelBit[0] = x[0];
   pixelBit[1] = y[0];
   pixelBit[2] = x[1];
   pixelBit[3] = y[1];
      default:
      ADDR_ASSERT_ALWAYS();
         }
   else if (IsRotateSwizzle(swMode))
   {
      switch (elementBytesLog2)
   {
         case 0:
      pixelBit[0] = y[0];
   pixelBit[1] = y[1];
   pixelBit[2] = y[2];
   pixelBit[3] = x[1];
   pixelBit[4] = x[0];
   pixelBit[5] = x[2];
   pixelBit[6] = x[3];
   pixelBit[7] = y[3];
      case 1:
      pixelBit[0] = y[0];
   pixelBit[1] = y[1];
   pixelBit[2] = y[2];
   pixelBit[3] = x[0];
   pixelBit[4] = x[1];
   pixelBit[5] = x[2];
   pixelBit[6] = x[3];
      case 2:
      pixelBit[0] = y[0];
   pixelBit[1] = y[1];
   pixelBit[2] = x[0];
   pixelBit[3] = y[2];
   pixelBit[4] = x[1];
   pixelBit[5] = x[2];
      case 3:
      pixelBit[0] = y[0];
   pixelBit[1] = x[0];
   pixelBit[2] = y[1];
   pixelBit[3] = x[1];
   pixelBit[4] = x[2];
      default:
         case 4:
            }
   else
   {
      ADDR_ASSERT_ALWAYS();
               // Post validation
   if (ret == ADDR_OK)
   {
      Dim2d microBlockDim = Block256_2d[elementBytesLog2];
   ADDR_ASSERT((2u << GetMaxValidChannelIndex(pEquation->addr, 8, 0)) ==
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeThinEquation
   *
   *   @brief
   *       Interface function stub of ComputeThinEquation
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeThinEquation(
      AddrResourceType rsrcType,
   AddrSwizzleMode  swMode,
   UINT_32          elementBytesLog2,
      {
                        UINT_32 maxXorBits = blockSizeLog2;
   if (IsNonPrtXor(swMode))
   {
      // For non-prt-xor, maybe need to initialize some more bits for xor
   // The highest xor bit used in equation will be max the following 3 items:
   // 1. m_pipeInterleaveLog2 + 2 * pipeXorBits
   // 2. m_pipeInterleaveLog2 + pipeXorBits + 2 * bankXorBits
            maxXorBits = Max(maxXorBits, m_pipeInterleaveLog2 + 2 * GetPipeXorBits(blockSizeLog2));
   maxXorBits = Max(maxXorBits, m_pipeInterleaveLog2 +
                     const UINT_32 maxBitsUsed = 14;
   ADDR_ASSERT((2 * maxBitsUsed) >= maxXorBits);
   ADDR_CHANNEL_SETTING x[maxBitsUsed] = {};
            const UINT_32 extraXorBits = 16;
   ADDR_ASSERT(extraXorBits >= maxXorBits - blockSizeLog2);
            for (UINT_32 i = 0; i < maxBitsUsed; i++)
   {
      InitChannel(1, 0, elementBytesLog2 + i, &x[i]);
                        for (UINT_32 i = 0; i < elementBytesLog2; i++)
   {
                  UINT_32 xIdx = 0;
   UINT_32 yIdx = 0;
            if (IsZOrderSwizzle(swMode))
   {
      if (elementBytesLog2 <= 3)
   {
         for (UINT_32 i = elementBytesLog2; i < 6; i++)
   {
                  }
   else
   {
            }
   else
   {
               if (ret == ADDR_OK)
   {
         Dim2d microBlockDim = Block256_2d[elementBytesLog2];
   xIdx = Log2(microBlockDim.w);
   yIdx = Log2(microBlockDim.h);
               if (ret == ADDR_OK)
   {
      for (UINT_32 i = lowBits; i < blockSizeLog2; i++)
   {
                  for (UINT_32 i = blockSizeLog2; i < maxXorBits; i++)
   {
                  if (IsXor(swMode))
   {
         // Fill XOR bits
                                 for (UINT_32 i = 0; i < pipeXorBits; i++)
   {
                                       for (UINT_32 i = 0; i < bankXorBits; i++)
   {
                                       if (IsPrt(swMode) == FALSE)
   {
      for (UINT_32 i = 0; i < pipeXorBits; i++)
                        for (UINT_32 i = 0; i < bankXorBits; i++)
   {
                     FillEqBitComponents(pEquation);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeThickEquation
   *
   *   @brief
   *       Interface function stub of ComputeThickEquation
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeThickEquation(
      AddrResourceType rsrcType,
   AddrSwizzleMode  swMode,
   UINT_32          elementBytesLog2,
      {
                                 UINT_32 maxXorBits = blockSizeLog2;
   if (IsNonPrtXor(swMode))
   {
      // For non-prt-xor, maybe need to initialize some more bits for xor
   // The highest xor bit used in equation will be max the following 3:
   // 1. m_pipeInterleaveLog2 + 3 * pipeXorBits
   // 2. m_pipeInterleaveLog2 + pipeXorBits + 3 * bankXorBits
            maxXorBits = Max(maxXorBits, m_pipeInterleaveLog2 + 3 * GetPipeXorBits(blockSizeLog2));
   maxXorBits = Max(maxXorBits, m_pipeInterleaveLog2 +
                     for (UINT_32 i = 0; i < elementBytesLog2; i++)
   {
                           const UINT_32 maxBitsUsed = 12;
   ADDR_ASSERT((3 * maxBitsUsed) >= maxXorBits);
   ADDR_CHANNEL_SETTING x[maxBitsUsed] = {};
   ADDR_CHANNEL_SETTING y[maxBitsUsed] = {};
            const UINT_32 extraXorBits = 24;
   ADDR_ASSERT(extraXorBits >= maxXorBits - blockSizeLog2);
            for (UINT_32 i = 0; i < maxBitsUsed; i++)
   {
      InitChannel(1, 0, elementBytesLog2 + i, &x[i]);
   InitChannel(1, 1, i, &y[i]);
               if (IsZOrderSwizzle(swMode))
   {
      switch (elementBytesLog2)
   {
         case 0:
      pixelBit[0]  = x[0];
   pixelBit[1]  = y[0];
   pixelBit[2]  = x[1];
   pixelBit[3]  = y[1];
   pixelBit[4]  = z[0];
   pixelBit[5]  = z[1];
   pixelBit[6]  = x[2];
   pixelBit[7]  = z[2];
   pixelBit[8]  = y[2];
   pixelBit[9]  = x[3];
      case 1:
      pixelBit[0]  = x[0];
   pixelBit[1]  = y[0];
   pixelBit[2]  = x[1];
   pixelBit[3]  = y[1];
   pixelBit[4]  = z[0];
   pixelBit[5]  = z[1];
   pixelBit[6]  = z[2];
   pixelBit[7]  = y[2];
   pixelBit[8]  = x[2];
      case 2:
      pixelBit[0]  = x[0];
   pixelBit[1]  = y[0];
   pixelBit[2]  = x[1];
   pixelBit[3]  = z[0];
   pixelBit[4]  = y[1];
   pixelBit[5]  = z[1];
   pixelBit[6]  = y[2];
   pixelBit[7]  = x[2];
      case 3:
      pixelBit[0]  = x[0];
   pixelBit[1]  = y[0];
   pixelBit[2]  = z[0];
   pixelBit[3]  = x[1];
   pixelBit[4]  = z[1];
   pixelBit[5]  = y[1];
   pixelBit[6]  = x[2];
      case 4:
      pixelBit[0]  = x[0];
   pixelBit[1]  = y[0];
   pixelBit[2]  = z[0];
   pixelBit[3]  = z[1];
   pixelBit[4]  = y[1];
   pixelBit[5]  = x[1];
      default:
      ADDR_ASSERT_ALWAYS();
         }
   else if (IsStandardSwizzle(rsrcType, swMode))
   {
      switch (elementBytesLog2)
   {
         case 0:
      pixelBit[0]  = x[0];
   pixelBit[1]  = x[1];
   pixelBit[2]  = x[2];
   pixelBit[3]  = x[3];
   pixelBit[4]  = y[0];
   pixelBit[5]  = y[1];
   pixelBit[6]  = z[0];
   pixelBit[7]  = z[1];
   pixelBit[8]  = z[2];
   pixelBit[9]  = y[2];
      case 1:
      pixelBit[0]  = x[0];
   pixelBit[1]  = x[1];
   pixelBit[2]  = x[2];
   pixelBit[3]  = y[0];
   pixelBit[4]  = y[1];
   pixelBit[5]  = z[0];
   pixelBit[6]  = z[1];
   pixelBit[7]  = z[2];
   pixelBit[8]  = y[2];
      case 2:
      pixelBit[0]  = x[0];
   pixelBit[1]  = x[1];
   pixelBit[2]  = y[0];
   pixelBit[3]  = y[1];
   pixelBit[4]  = z[0];
   pixelBit[5]  = z[1];
   pixelBit[6]  = y[2];
   pixelBit[7]  = x[2];
      case 3:
      pixelBit[0]  = x[0];
   pixelBit[1]  = y[0];
   pixelBit[2]  = y[1];
   pixelBit[3]  = z[0];
   pixelBit[4]  = z[1];
   pixelBit[5]  = x[1];
   pixelBit[6]  = x[2];
      case 4:
      pixelBit[0]  = y[0];
   pixelBit[1]  = y[1];
   pixelBit[2]  = z[0];
   pixelBit[3]  = z[1];
   pixelBit[4]  = x[0];
   pixelBit[5]  = x[1];
      default:
      ADDR_ASSERT_ALWAYS();
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
               if (ret == ADDR_OK)
   {
      Dim3d microBlockDim = Block1K_3d[elementBytesLog2];
   UINT_32 xIdx = Log2(microBlockDim.w);
   UINT_32 yIdx = Log2(microBlockDim.h);
                     const UINT_32 lowBits = 10;
   ADDR_ASSERT(pEquation->addr[lowBits - 1].valid == 1);
            for (UINT_32 i = lowBits; i < blockSizeLog2; i++)
   {
         if ((i % 3) == 0)
   {
         }
   else if ((i % 3) == 1)
   {
         }
   else
   {
                  for (UINT_32 i = blockSizeLog2; i < maxXorBits; i++)
   {
         if ((i % 3) == 0)
   {
         }
   else if ((i % 3) == 1)
   {
         }
   else
   {
                  if (IsXor(swMode))
   {
         // Fill XOR bits
   UINT_32 pipeStart = m_pipeInterleaveLog2;
   UINT_32 pipeXorBits = GetPipeXorBits(blockSizeLog2);
   for (UINT_32 i = 0; i < pipeXorBits; i++)
   {
                                                                     UINT_32 bankStart = pipeStart + pipeXorBits;
   UINT_32 bankXorBits = GetBankXorBits(blockSizeLog2);
   for (UINT_32 i = 0; i < bankXorBits; i++)
   {
                                                                     FillEqBitComponents(pEquation);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::IsValidDisplaySwizzleMode
   *
   *   @brief
   *       Check if a swizzle mode is supported by display engine
   *
   *   @return
   *       TRUE is swizzle mode is supported by display engine
   ************************************************************************************************************************
   */
   BOOL_32 Gfx9Lib::IsValidDisplaySwizzleMode(
         {
                        if (m_settings.isDce12)
   {
      if (pIn->bpp == 32)
   {
         }
   else if (pIn->bpp <= 64)
   {
            }
   else if (m_settings.isDcn1)
   {
      if (pIn->bpp < 64)
   {
         }
   else if (pIn->bpp == 64)
   {
            }
   else if (m_settings.isDcn2)
   {
      if (pIn->bpp < 64)
   {
         }
   else if (pIn->bpp == 64)
   {
            }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputePipeBankXor
   *
   *   @brief
   *       Generate a PipeBankXor value to be ORed into bits above pipeInterleaveBits of address
   *
   *   @return
   *       PipeBankXor value
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputePipeBankXor(
      const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,
      {
      if (IsXor(pIn->swizzleMode))
   {
      UINT_32 macroBlockBits = GetBlockSizeLog2(pIn->swizzleMode);
   UINT_32 pipeBits       = GetPipeXorBits(macroBlockBits);
            UINT_32 pipeXor = 0;
            const UINT_32 bankMask = (1 << bankBits) - 1;
            const UINT_32 bpp      = pIn->flags.fmask ?
         if (bankBits == 4)
   {
                        }
   else if (bankBits > 0)
   {
         UINT_32 bankIncrease = (1 << (bankBits - 1)) - 1;
   bankIncrease = (bankIncrease == 0) ? 1 : bankIncrease;
               }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeSlicePipeBankXor
   *
   *   @brief
   *       Generate slice PipeBankXor value based on base PipeBankXor value and slice id
   *
   *   @return
   *       PipeBankXor value
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeSlicePipeBankXor(
      const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,
      {
      UINT_32 macroBlockBits = GetBlockSizeLog2(pIn->swizzleMode);
   UINT_32 pipeBits       = GetPipeXorBits(macroBlockBits);
            UINT_32 pipeXor        = ReverseBitVector(pIn->slice, pipeBits);
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeSubResourceOffsetForSwizzlePattern
   *
   *   @brief
   *       Compute sub resource offset to support swizzle pattern
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeSubResourceOffsetForSwizzlePattern(
      const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,
      {
               UINT_32 macroBlockBits = GetBlockSizeLog2(pIn->swizzleMode);
   UINT_32 pipeBits       = GetPipeXorBits(macroBlockBits);
   UINT_32 bankBits       = GetBankXorBits(macroBlockBits);
   UINT_32 pipeXor        = ReverseBitVector(pIn->slice, pipeBits);
   UINT_32 bankXor        = ReverseBitVector(pIn->slice >> pipeBits, bankBits);
            pOut->offset = pIn->slice * pIn->sliceSize +
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::ValidateNonSwModeParams
   *
   *   @brief
   *       Validate compute surface info params except swizzle mode
   *
   *   @return
   *       TRUE if parameters are valid, FALSE otherwise
   ************************************************************************************************************************
   */
   BOOL_32 Gfx9Lib::ValidateNonSwModeParams(
         {
               if ((pIn->bpp == 0) || (pIn->bpp > 128) || (pIn->width == 0) || (pIn->numFrags > 8) || (pIn->numSamples > 16))
   {
      ADDR_ASSERT_ALWAYS();
               if (pIn->resourceType >= ADDR_RSRC_MAX_TYPE)
   {
      ADDR_ASSERT_ALWAYS();
               const BOOL_32 mipmap = (pIn->numMipLevels > 1);
   const BOOL_32 msaa   = (pIn->numFrags > 1);
            const AddrResourceType rsrcType = pIn->resourceType;
   const BOOL_32          tex3d    = IsTex3d(rsrcType);
   const BOOL_32          tex2d    = IsTex2d(rsrcType);
            const ADDR2_SURFACE_FLAGS flags   = pIn->flags;
   const BOOL_32             zbuffer = flags.depth || flags.stencil;
   const BOOL_32             display = flags.display || flags.rotated;
   const BOOL_32             stereo  = flags.qbStereo;
            // Resource type check
   if (tex1d)
   {
      if (msaa || zbuffer || display || stereo || isBc || fmask)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (tex2d)
   {
      if ((msaa && mipmap) || (stereo && msaa) || (stereo && mipmap))
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (tex3d)
   {
      if (msaa || zbuffer || display || stereo || fmask)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::ValidateSwModeParams
   *
   *   @brief
   *       Validate compute surface info related to swizzle mode
   *
   *   @return
   *       TRUE if parameters are valid, FALSE otherwise
   ************************************************************************************************************************
   */
   BOOL_32 Gfx9Lib::ValidateSwModeParams(
         {
               if ((pIn->swizzleMode >= ADDR_SW_MAX_TYPE) || (IsValidSwMode(pIn->swizzleMode) == FALSE))
   {
      ADDR_ASSERT_ALWAYS();
               const BOOL_32 mipmap = (pIn->numMipLevels > 1);
   const BOOL_32 msaa   = (pIn->numFrags > 1);
   const BOOL_32 isBc   = ElemLib::IsBlockCompressed(pIn->format);
            const AddrResourceType rsrcType = pIn->resourceType;
   const BOOL_32          tex3d    = IsTex3d(rsrcType);
   const BOOL_32          tex2d    = IsTex2d(rsrcType);
            const AddrSwizzleMode  swizzle     = pIn->swizzleMode;
   const BOOL_32          linear      = IsLinear(swizzle);
   const BOOL_32          blk256B     = IsBlock256b(swizzle);
            const ADDR2_SURFACE_FLAGS flags   = pIn->flags;
   const BOOL_32             zbuffer = flags.depth || flags.stencil;
   const BOOL_32             color   = flags.color;
   const BOOL_32             texture = flags.texture;
   const BOOL_32             display = flags.display || flags.rotated;
   const BOOL_32             prt     = flags.prt;
            const BOOL_32             thin3d  = tex3d && flags.view3dAs2dArray;
   const BOOL_32             zMaxMip = tex3d && mipmap &&
            // Misc check
   if (msaa && (GetBlockSize(swizzle) < (m_pipeInterleaveBytes * pIn->numFrags)))
   {
      // MSAA surface must have blk_bytes/pipe_interleave >= num_samples
   ADDR_ASSERT_ALWAYS();
               if (display && (IsValidDisplaySwizzleMode(pIn) == FALSE))
   {
      ADDR_ASSERT_ALWAYS();
               if ((pIn->bpp == 96) && (linear == FALSE))
   {
      ADDR_ASSERT_ALWAYS();
               if (prt && isNonPrtXor)
   {
      ADDR_ASSERT_ALWAYS();
               // Resource type check
   if (tex1d)
   {
      if (linear == FALSE)
   {
         ADDR_ASSERT_ALWAYS();
               // Swizzle type check
   if (linear)
   {
      if (((tex1d == FALSE) && prt) || zbuffer || msaa || (pIn->bpp == 0) ||
         {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsZOrderSwizzle(swizzle))
   {
      if ((color && msaa) || thin3d || isBc || is422 || (tex2d && (pIn->bpp > 64)) || (msaa && (pIn->bpp > 32)))
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsStandardSwizzle(swizzle))
   {
      if (zbuffer || thin3d || (tex3d && (pIn->bpp == 128) && color) || fmask)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsDisplaySwizzle(swizzle))
   {
      if (zbuffer || (prt && tex3d) || fmask || zMaxMip)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsRotateSwizzle(swizzle))
   {
      if (zbuffer || (pIn->bpp > 64) || tex3d || isBc || fmask)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else
   {
      ADDR_ASSERT_ALWAYS();
               // Block type check
   if (blk256B)
   {
      if (prt || zbuffer || tex3d || mipmap || msaa)
   {
         ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeSurfaceInfoSanityCheck
   *
   *   @brief
   *       Compute surface info sanity check
   *
   *   @return
   *       ADDR_OK if parameters are valid, ADDR_INVALIDPARAMS otherwise
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeSurfaceInfoSanityCheck(
         {
         }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlGetPreferredSurfaceSetting
   *
   *   @brief
   *       Internal function to get suggested surface information for cliet to use
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlGetPreferredSurfaceSetting(
      const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,
      {
      ADDR_E_RETURNCODE returnCode = ADDR_INVALIDPARAMS;
            UINT_32 bpp        = pIn->bpp;
   UINT_32 width      = Max(pIn->width, 1u);
   UINT_32 height     = Max(pIn->height, 1u);
   UINT_32 numSamples = Max(pIn->numSamples, 1u);
            if (pIn->flags.fmask)
   {
      bpp                = GetFmaskBpp(numSamples, numFrags);
   numFrags           = 1;
   numSamples         = 1;
      }
   else
   {
      // Set format to INVALID will skip this conversion
   if (pIn->format != ADDR_FMT_INVALID)
   {
                                 // Get compression/expansion factors and element mode which indicates compression/expansion
   bpp = pElemLib->GetBitsPerPixel(pIn->format,
                        UINT_32 basePitch = 0;
   GetElemLib()->AdjustSurfaceInfo(elemMode,
                                          // The output may get changed for volume(3D) texture resource in future
               const UINT_32 numSlices    = Max(pIn->numSlices, 1u);
   const UINT_32 numMipLevels = Max(pIn->numMipLevels, 1u);
   const BOOL_32 msaa         = (numFrags > 1) || (numSamples > 1);
            // Pre sanity check on non swizzle mode parameters
   ADDR2_COMPUTE_SURFACE_INFO_INPUT localIn = {};
   localIn.flags        = pIn->flags;
   localIn.resourceType = pOut->resourceType;
   localIn.format       = pIn->format;
   localIn.bpp          = bpp;
   localIn.width        = width;
   localIn.height       = height;
   localIn.numSlices    = numSlices;
   localIn.numMipLevels = numMipLevels;
   localIn.numSamples   = numSamples;
            if (ValidateNonSwModeParams(&localIn))
   {
      // Forbid swizzle mode(s) by client setting
   ADDR2_SWMODE_SET allowedSwModeSet = {};
   allowedSwModeSet.value |= pIn->forbiddenBlock.linear ? 0 : Gfx9LinearSwModeMask;
   allowedSwModeSet.value |= pIn->forbiddenBlock.micro  ? 0 : Gfx9Blk256BSwModeMask;
   allowedSwModeSet.value |=
         pIn->forbiddenBlock.macroThin4KB ? 0 :
   allowedSwModeSet.value |=
         pIn->forbiddenBlock.macroThick4KB ? 0 :
   allowedSwModeSet.value |=
         pIn->forbiddenBlock.macroThin64KB ? 0 :
   allowedSwModeSet.value |=
                  if (pIn->preferredSwSet.value != 0)
   {
         allowedSwModeSet.value &= pIn->preferredSwSet.sw_Z ? ~0 : ~Gfx9ZSwModeMask;
   allowedSwModeSet.value &= pIn->preferredSwSet.sw_S ? ~0 : ~Gfx9StandardSwModeMask;
   allowedSwModeSet.value &= pIn->preferredSwSet.sw_D ? ~0 : ~Gfx9DisplaySwModeMask;
            if (pIn->noXor)
   {
                  if (pIn->maxAlign > 0)
   {
         if (pIn->maxAlign < Size64K)
   {
                  if (pIn->maxAlign < Size4K)
   {
                  if (pIn->maxAlign < Size256)
   {
                  // Filter out invalid swizzle mode(s) by image attributes and HW restrictions
   switch (pOut->resourceType)
   {
         case ADDR_RSRC_TEX_1D:
                                    if (bpp > 64)
   {
                                       if ((numMipLevels > 1) && (numSlices >= width) && (numSlices >= height))
   {
      // SW_*_D for 3D mipmaps (maxmip > 0) is only supported for Xmajor or Ymajor mipmap
   // When depth (Z) is the maximum dimension then must use one of the SW_*_S
                     if ((bpp == 128) && pIn->flags.color)
                        if (pIn->flags.view3dAs2dArray)
   {
                     default:
      ADDR_ASSERT_ALWAYS();
               if (pIn->format == ADDR_FMT_32_32_32)
   {
                  if (ElemLib::IsBlockCompressed(pIn->format))
   {
         if (pIn->flags.texture)
   {
         }
   else
   {
                  if (ElemLib::IsMacroPixelPacked(pIn->format) ||
         {
                  if (pIn->flags.fmask || pIn->flags.depth || pIn->flags.stencil)
   {
                  if (pIn->flags.noMetadata == FALSE)
   {
      if (pIn->flags.depth &&
      pIn->flags.texture &&
      {
      // When _X/_T swizzle mode was used for MSAA depth texture, TC will get zplane
   // equation from wrong address within memory range a tile covered and use the
                     if (m_settings.htileCacheRbConflict &&
      (pIn->flags.depth || pIn->flags.stencil) &&
   (numSlices > 1) &&
   (pIn->flags.metaRbUnaligned == FALSE) &&
      {
      // Z_X 2D array with Rb/Pipe aligned HTile won't have metadata cache coherency
                  if (msaa)
   {
                  if ((numFrags > 1) &&
         {
         // MSAA surface must have blk_bytes/pipe_interleave >= num_samples
            if (numMipLevels > 1)
   {
                  if (displayRsrc)
   {
         if (m_settings.isDce12)
   {
         }
   else if (m_settings.isDcn1)
   {
         }
   else if (m_settings.isDcn2)
   {
         }
   else
   {
                  if (allowedSwModeSet.value != 0)
   #if DEBUG
                              for (UINT_32 i = 0; validateSwModeSet != 0; i++)
   {
      if (validateSwModeSet & 1)
   {
                     #endif
                  pOut->validSwModeSet = allowedSwModeSet;
   pOut->canXor         = (allowedSwModeSet.value & Gfx9XorSwModeMask) ? TRUE : FALSE;
                           if (pOut->clientPreferredSwSet.value == 0)
   {
                  // Apply optional restrictions
   if (pIn->flags.needEquation)
   {
      UINT_32 components = pIn->flags.allowExtEquation ?  ADDR_MAX_EQUATION_COMP :
                     if (allowedSwModeSet.value == Gfx9LinearSwModeMask)
   {
         }
   else
                     if ((height > 1) && (computeMinSize == FALSE))
   {
      // Always ignore linear swizzle mode if:
   // 1. This is a (2D/3D) resource with height > 1
                              // Determine block size if there are 2 or more block type candidates
                           swMode[AddrBlockLinear]   = ADDR_SW_LINEAR;
                        if (pOut->resourceType == ADDR_RSRC_TEX_3D)
   {
                                 const UINT_32 ratioLow           = computeMinSize ? 1 : (pIn->flags.opt4space ? 3 : 2);
   const UINT_32 ratioHi            = computeMinSize ? 1 : (pIn->flags.opt4space ? 2 : 1);
                                 for (UINT_32 i = AddrBlockLinear; i < AddrBlockMaxTiledType; i++)
   {
                                 if (localIn.swizzleMode == ADDR_SW_LINEAR)
   {
                                                                  if ((minSize == 0) ||
         {
         minSize    = padSize[i];
      }
   else
   {
                           if (pIn->memoryBudget > 1.0)
   {
         // If minimum size is given by swizzle mode with bigger-block type, then don't ever check
   // smaller-block type again in coming loop
   switch (minSizeBlk)
   {
      case AddrBlockThick64KB:
         case AddrBlockThin64KB:
         case AddrBlockThick4KB:
         case AddrBlockThin4KB:
                                                            for (UINT_32 i = AddrBlockMicro; i < AddrBlockMaxTiledType; i++)
   {
      if ((i != minSizeBlk) &&
         {
      if (Addr2BlockTypeWithinMemoryBudget(minSize, padSize[i], 0, 0, pIn->memoryBudget) == FALSE)
   {
                              // Remove linear block type if 2 or more block types are allowed
                                             if (minSizeBlk == static_cast<UINT_32>(AddrBlockMaxTiledType))
                        switch (minSizeBlk)
   {
                                                                                                                                                                     default:
                                                   // Determine swizzle type if there are 2 or more swizzle type candidates
   if ((allowedSwSet.value != 0) && (IsPow2(allowedSwSet.value) == FALSE))
   {
      if (ElemLib::IsBlockCompressed(pIn->format))
   {
         if (allowedSwSet.sw_D)
   {
         }
   else
   {
      ADDR_ASSERT(allowedSwSet.sw_S);
      }
   else if (ElemLib::IsMacroPixelPacked(pIn->format))
   {
         if (allowedSwSet.sw_S)
   {
         }
   else if (allowedSwSet.sw_D)
   {
         }
   else
   {
      ADDR_ASSERT(allowedSwSet.sw_R);
      }
   else if (pOut->resourceType == ADDR_RSRC_TEX_3D)
   {
         if (pIn->flags.color && allowedSwSet.sw_D)
   {
         }
   else if (allowedSwSet.sw_Z)
   {
         }
   else
   {
      ADDR_ASSERT(allowedSwSet.sw_S);
      }
   else
   {
         if (pIn->flags.rotated && allowedSwSet.sw_R)
   {
         }
   else if (allowedSwSet.sw_D)
   {
         }
   else if (allowedSwSet.sw_S)
   {
         }
   else
   {
                                          // Determine swizzle mode now. Always select the "largest" swizzle mode for a given block type + swizzle
   // type combination. For example, for AddrBlockThin64KB + ADDR_SW_S, select SW_64KB_S_X(25) if it's
                     }
   else
   {
         // Invalid combination...
      }
   else
   {
      // Invalid combination...
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::ComputeStereoInfo
   *
   *   @brief
   *       Compute height alignment and right eye pipeBankXor for stereo surface
   *
   *   @return
   *       Error code
   *
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::ComputeStereoInfo(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut,
   UINT_32*                                pHeightAlign
      {
                        if (eqIndex < m_numEquations)
   {
      if (IsXor(pIn->swizzleMode))
   {
         const UINT_32        blkSizeLog2       = GetBlockSizeLog2(pIn->swizzleMode);
   const UINT_32        numPipeBits       = GetPipeXorBits(blkSizeLog2);
   const UINT_32        numBankBits       = GetBankXorBits(blkSizeLog2);
   const UINT_32        bppLog2           = Log2(pIn->bpp >> 3);
                                                                                                                              if (maxYCoordInPipeBankXor > maxYCoordInBaseEquation)
                                             if ((PowTwoAlign(pIn->height, *pHeightAlign) % (*pHeightAlign * 2)) != 0)
   {
                                    if (maxYCoordInBankXor == maxYCoordInPipeBankXor)
                              ADDR_ASSERT(pOut->pStereoInfo->rightSwizzle ==
                  }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeSurfaceInfoTiled
   *
   *   @brief
   *       Internal function to calculate alignment for tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeSurfaceInfoTiled(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR_E_RETURNCODE returnCode = ComputeBlockDimensionForSurf(&pOut->blockWidth,
                                          if (returnCode == ADDR_OK)
   {
               if ((IsTex2d(pIn->resourceType) == TRUE) &&
         (pIn->flags.display || pIn->flags.rotated) &&
   (pIn->numMipLevels <= 1) &&
   (pIn->numSamples <= 1) &&
   {
         // Display engine needs pitch align to be at least 32 pixels.
                     if ((pIn->numMipLevels <= 1) && (pIn->pitchInElement > 0))
   {
         if ((pIn->pitchInElement % pitchAlignInElement) != 0)
   {
         }
   else if (pIn->pitchInElement < pOut->pitch)
   {
         }
   else
   {
                           if (pIn->flags.qbStereo)
   {
                  if (returnCode == ADDR_OK)
   {
                  if (heightAlign > 1)
   {
                           pOut->epitchIsHeight   = FALSE;
                  pOut->mipChainPitch    = pOut->pitch;
                  if (pIn->numMipLevels > 1)
   {
      pOut->firstMipIdInTail = GetMipChainInfo(pIn->resourceType,
                                                                           if (endingMipId == 0)
   {
      const Dim3d tailMaxDim = GetMipTailDim(pIn->resourceType,
                              pOut->epitchIsHeight = TRUE;
   pOut->pitch          = tailMaxDim.w;
   pOut->height         = tailMaxDim.h;
   pOut->numSlices      = IsThick(pIn->resourceType, pIn->swizzleMode) ?
            }
   else
                           AddrMajorMode majorMode = GetMajorMode(pIn->resourceType,
                                                                                          }
                                                                                                      for (UINT_32 i = 0; i < pIn->numMipLevels; i++)
                              mipStartPos = GetMipStartPos(pIn->resourceType,
                              pIn->swizzleMode,
                                 UINT_32 pitchInBlock     =
         UINT_32 sliceInBlock     =
                                    pOut->pMipInfo[i].macroBlockOffset = macroBlockOffset;
         }
   else if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].pitch  = pOut->pitch;
   pOut->pMipInfo[0].height = pOut->height;
                     pOut->sliceSize = static_cast<UINT_64>(pOut->mipChainPitch) * pOut->mipChainHeight *
                        if ((IsBlock256b(pIn->swizzleMode) == FALSE) &&
      (pIn->flags.color || pIn->flags.depth || pIn->flags.stencil || pIn->flags.fmask) &&
   (pIn->flags.texture == TRUE) &&
   (pIn->flags.noMetadata == FALSE) &&
      {
      // Assume client requires pipe aligned metadata, which is TcCompatible and will be accessed by TC...
   // Then we need extra padding for base surface. Otherwise, metadata and data surface for same pixel will
   // be flushed to different pipes, but texture engine only uses pipe id of data surface to fetch both of
                     if (pIn->flags.prt)
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeSurfaceInfoLinear
   *
   *   @brief
   *       Internal function to calculate alignment for linear surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeSurfaceInfoLinear(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR_E_RETURNCODE returnCode   = ADDR_OK;
   UINT_32           pitch        = 0;
   UINT_32           actualHeight = 0;
   UINT_32           elementBytes = pIn->bpp >> 3;
            if (IsTex1d(pIn->resourceType))
   {
      if (pIn->height > 1)
   {
         }
   else
   {
                                 if (pIn->flags.prt == FALSE)
   {
                        if (returnCode == ADDR_OK)
   {
      if (pOut->pMipInfo != NULL)
   {
      for (UINT_32 i = 0; i < pIn->numMipLevels; i++)
   {
         pOut->pMipInfo[i].offset = pitch * elementBytes * i;
   pOut->pMipInfo[i].pitch  = pitch;
   pOut->pMipInfo[i].height = 1;
            }
   else
   {
                  if ((pitch == 0) || (actualHeight == 0))
   {
                  if (returnCode == ADDR_OK)
   {
      pOut->pitch          = pitch;
   pOut->height         = pIn->height;
   pOut->numSlices      = pIn->numSlices;
   pOut->mipChainPitch  = pitch;
   pOut->mipChainHeight = actualHeight;
   pOut->mipChainSlice  = pOut->numSlices;
   pOut->epitchIsHeight = (pIn->numMipLevels > 1) ? TRUE : FALSE;
   pOut->sliceSize      = static_cast<UINT_64>(pOut->pitch) * actualHeight * elementBytes;
   pOut->surfSize       = pOut->sliceSize * pOut->numSlices;
   pOut->baseAlign      = (pIn->swizzleMode == ADDR_SW_LINEAR_GENERAL) ? (pIn->bpp / 8) : alignment;
   pOut->blockWidth     = (pIn->swizzleMode == ADDR_SW_LINEAR_GENERAL) ? 1 : (256 / elementBytes);
   pOut->blockHeight    = 1;
               // Post calculation validate
               }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetMipChainInfo
   *
   *   @brief
   *       Internal function to get out information about mip chain
   *
   *   @return
   *       Smaller value between Id of first mip fitted in mip tail and max Id of mip being created
   ************************************************************************************************************************
   */
   UINT_32 Gfx9Lib::GetMipChainInfo(
      AddrResourceType  resourceType,
   AddrSwizzleMode   swizzleMode,
   UINT_32           bpp,
   UINT_32           mip0Width,
   UINT_32           mip0Height,
   UINT_32           mip0Depth,
   UINT_32           blockWidth,
   UINT_32           blockHeight,
   UINT_32           blockDepth,
   UINT_32           numMipLevel,
      {
      const Dim3d tailMaxDim =
            UINT_32 mipPitch         = mip0Width;
   UINT_32 mipHeight        = mip0Height;
   UINT_32 mipDepth         = IsTex3d(resourceType) ? mip0Depth : 1;
   UINT_32 offset           = 0;
   UINT_32 firstMipIdInTail = numMipLevel;
   BOOL_32 inTail           = FALSE;
   BOOL_32 finalDim         = FALSE;
   BOOL_32 is3dThick        = IsThick(resourceType, swizzleMode);
            for (UINT_32 mipId = 0; mipId < numMipLevel; mipId++)
   {
      if (inTail)
   {
         if (finalDim == FALSE)
                     if (is3dThick)
   {
         }
   else
                                                if (is3dThick)
   {
         mipPitch  = Block256_3dZ[index].w;
   mipHeight = Block256_3dZ[index].h;
   }
   else
   {
                              }
   else
   {
                        if (inTail)
   {
                           if (is3dThick)
   {
            }
   else
   {
                     if (is3dThick)
   {
                     if (pMipInfo != NULL)
   {
         pMipInfo[mipId].pitch  = mipPitch;
   pMipInfo[mipId].height = mipHeight;
   pMipInfo[mipId].depth  = mipDepth;
                     if (finalDim)
   {
         if (is3dThin)
   {
         }
   else
   {
                        if (is3dThick || is3dThin)
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetMetaMiptailInfo
   *
   *   @brief
   *       Get mip tail coordinate information.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::GetMetaMiptailInfo(
      ADDR2_META_MIP_INFO*    pInfo,          ///< [out] output structure to store per mip coord
   Dim3d                   mipCoord,       ///< [in] mip tail base coord
   UINT_32                 numMipInTail,   ///< [in] number of mips in tail
   Dim3d*                  pMetaBlkDim     ///< [in] meta block width/height/depth
      {
      BOOL_32 isThick   = (pMetaBlkDim->d > 1);
   UINT_32 mipWidth  = pMetaBlkDim->w;
   UINT_32 mipHeight = pMetaBlkDim->h >> 1;
   UINT_32 mipDepth  = pMetaBlkDim->d;
            if (isThick)
   {
         }
   else if (pMetaBlkDim->h >= 1024)
   {
         }
   else if (pMetaBlkDim->h == 512)
   {
         }
   else
   {
                           for (UINT_32 mip = 0; mip < numMipInTail; mip++)
   {
      pInfo[mip].inMiptail = TRUE;
   pInfo[mip].startX = mipCoord.w;
   pInfo[mip].startY = mipCoord.h;
   pInfo[mip].startZ = mipCoord.d;
   pInfo[mip].width = mipWidth;
   pInfo[mip].height = mipHeight;
            if (mipWidth <= 32)
   {
         if (blk32MipId == 0xFFFFFFFF)
   {
                  mipCoord.w = pInfo[blk32MipId].startX;
                  switch (mip - blk32MipId)
   {
      case 0:
      mipCoord.w += 32;       // 16x16
      case 1:
      mipCoord.h += 32;       // 8x8
      case 2:
      mipCoord.h += 32;       // 4x4
   mipCoord.w += 16;
      case 3:
      mipCoord.h += 32;       // 2x2
   mipCoord.w += 32;
      case 4:
      mipCoord.h += 32;       // 1x1
   mipCoord.w += 48;
      // The following are for BC/ASTC formats
   case 5:
      mipCoord.h += 48;       // 1/2 x 1/2
      case 6:
      mipCoord.h += 48;       // 1/4 x 1/4
   mipCoord.w += 16;
      case 7:
      mipCoord.h += 48;       // 1/8 x 1/8
   mipCoord.w += 32;
      case 8:
      mipCoord.h += 48;       // 1/16 x 1/16
   mipCoord.w += 48;
      default:
                                    if (isThick)
   {
         }
   else
   {
         if (mipWidth <= minInc)
   {
      // if we're below the minimal increment...
   if (isThick)
   {
      // For 3d, just go in z direction
      }
   else
   {
      // For 2d, first go across, then down
   if ((mipWidth * 2) == minInc)
   {
         // if we're 2 mips below, that's when we go back in x, and down in y
   mipCoord.w -= minInc;
   }
   else
   {
         // otherwise, just go across in x
         }
   else
   {
      // On even mip, go down, otherwise, go across
   if (mip & 1)
   {
         }
   else
   {
            }
   // Divide the width by 2
   mipWidth >>= 1;
   // After the first mip in tail, the mip is always a square
   mipHeight = mipWidth;
   // ...or for 3d, a cube
   if (isThick)
   {
               }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::GetMipStartPos
   *
   *   @brief
   *       Internal function to get out information about mip logical start position
   *
   *   @return
   *       logical start position in macro block width/heith/depth of one mip level within one slice
   ************************************************************************************************************************
   */
   Dim3d Gfx9Lib::GetMipStartPos(
      AddrResourceType  resourceType,
   AddrSwizzleMode   swizzleMode,
   UINT_32           width,
   UINT_32           height,
   UINT_32           depth,
   UINT_32           blockWidth,
   UINT_32           blockHeight,
   UINT_32           blockDepth,
   UINT_32           mipId,
   UINT_32           log2ElementBytes,
      {
      Dim3d       mipStartPos = {0};
            // Report mip in tail if Mip0 is already in mip tail
   BOOL_32 inMipTail      = IsInMipTail(resourceType, swizzleMode, tailMaxDim, width, height, depth);
   UINT_32 log2BlkSize    = GetBlockSizeLog2(swizzleMode);
            if (inMipTail == FALSE)
   {
      // Mip 0 dimension, unit in block
   UINT_32 mipWidthInBlk   = width  / blockWidth;
   UINT_32 mipHeightInBlk  = height / blockHeight;
   UINT_32 mipDepthInBlk   = depth  / blockDepth;
   AddrMajorMode majorMode = GetMajorMode(resourceType,
                                       for (UINT_32 i = 1; i <= mipId; i++)
   {
         if ((i == 1) || (i == 3))
   {
      if (majorMode == ADDR_MAJOR_Y)
   {
         }
   else
   {
            }
   else
   {
      if (majorMode == ADDR_MAJOR_X)
   {
         }
   else if (majorMode == ADDR_MAJOR_Y)
   {
         }
   else
   {
                              if (IsThick(resourceType, swizzleMode))
                     if (dim == 0)
   {
      inTail =
      }
   else if (dim == 1)
   {
      inTail =
      }
   else
   {
      inTail =
         }
   else
   {
      if (log2BlkSize & 1)
   {
         }
   else
   {
                     if (inTail)
   {
                        mipWidthInBlk  = RoundHalf(mipWidthInBlk);
   mipHeightInBlk = RoundHalf(mipHeightInBlk);
            if (mipId >= endingMip)
   {
         inMipTail      = TRUE;
               if (inMipTail)
   {
      UINT_32 index = mipIndexInTail + MaxMacroBits - log2BlkSize;
   ADDR_ASSERT(index < sizeof(MipTailOffset256B) / sizeof(UINT_32));
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::HwlComputeSurfaceAddrFromCoordTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::HwlComputeSurfaceAddrFromCoordTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT localIn = {0};
   localIn.swizzleMode  = pIn->swizzleMode;
   localIn.flags        = pIn->flags;
   localIn.resourceType = pIn->resourceType;
   localIn.bpp          = pIn->bpp;
   localIn.width        = Max(pIn->unalignedWidth, 1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices, 1u);
   localIn.numMipLevels = Max(pIn->numMipLevels, 1u);
   localIn.numSamples   = Max(pIn->numSamples, 1u);
   localIn.numFrags     = Max(pIn->numFrags, 1u);
   if (localIn.numMipLevels <= 1)
   {
                  ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {0};
            BOOL_32 valid = (returnCode == ADDR_OK) &&
                        if (valid)
   {
      UINT_32 log2ElementBytes   = Log2(pIn->bpp >> 3);
   Dim3d   mipStartPos        = {0};
            if (pIn->numMipLevels > 1)
   {
                        mipStartPos = GetMipStartPos(pIn->resourceType,
                              pIn->swizzleMode,
   localOut.pitch,
   localOut.height,
   localOut.numSlices,
               UINT_32 interleaveOffset = 0;
   UINT_32 pipeBits = 0;
   UINT_32 pipeXor = 0;
   UINT_32 bankBits = 0;
            if (IsThin(pIn->resourceType, pIn->swizzleMode))
   {
                        if (IsZOrderSwizzle(pIn->swizzleMode))
   {
      // Morton generation
   if ((log2ElementBytes == 0) || (log2ElementBytes == 2))
   {
      UINT_32 totalLowBits = 6 - log2ElementBytes;
   UINT_32 mortBits = totalLowBits / 2;
   UINT_32 lowBitsValue = MortonGen2d(pIn->y, pIn->x, mortBits);
   // Are 9 bits enough?
   UINT_32 highBitsValue =
         blockOffset = lowBitsValue | highBitsValue;
      }
   else
                        // Fill LSBs with sample bits
   if (pIn->numSamples > 1)
   {
                        // Shift according to BytesPP
      }
   else
   {
                           // Micro block dimension
   ADDR_ASSERT(log2ElementBytes < MaxNumOfBpp);
   Dim2d microBlockDim = Block256_2d[log2ElementBytes];
                        // Sample bits start location
   UINT_32 sampleStart = log2BlkSize - Log2(pIn->numSamples);
   // Join sample bits information to the highest Macro block bits
   if (IsNonPrtXor(pIn->swizzleMode))
   {
      // Non-prt-Xor : xor highest Macro block bits with sample bits
      }
   else
   {
      // Non-Xor or prt-Xor: replace highest Macro block bits with sample bits
   // after this op, the blockOffset only contains log2 Macro block size bits
   blockOffset %= (1 << sampleStart);
   blockOffset |= (pIn->sample << sampleStart);
                  if (IsXor(pIn->swizzleMode))
   {
      // Mask off bits above Macro block bits to keep page synonyms working for prt
   if (IsPrt(pIn->swizzleMode))
                                             // Pipe/Se xor bits
   pipeBits = GetPipeXorBits(log2BlkSize);
                        // Bank xor bits
   bankBits = GetBankXorBits(log2BlkSize);
                        // Put all the part back together
   blockOffset <<= bankBits;
   blockOffset |= bankXor;
   blockOffset <<= pipeBits;
   blockOffset |= pipeXor;
                                             if (IsNonPrtXor(pIn->swizzleMode) && (pIn->numSamples <= 1))
   {
      // Apply slice xor if not MSAA/PRT
   blockOffset ^= (ReverseBitVector(pIn->slice, pipeBits) << m_pipeInterleaveLog2);
                                             UINT_32 pitchInMacroBlock = localOut.mipChainPitch / localOut.blockWidth;
   UINT_32 paddedHeightInMacroBlock = localOut.mipChainHeight / localOut.blockHeight;
   UINT_32 sliceSizeInMacroBlock = pitchInMacroBlock * paddedHeightInMacroBlock;
   UINT_64 macroBlockIndex =
                        }
   else
   {
                           UINT_32 blockOffset = MortonGen3d((pIn->x / microBlockDim.w),
                                       if (IsXor(pIn->swizzleMode))
   {
      // Mask off bits above Macro block bits to keep page synonyms working for prt
   if (IsPrt(pIn->swizzleMode))
                                             // Pipe/Se xor bits
   pipeBits = GetPipeXorBits(log2BlkSize);
                        // Bank xor bits
   bankBits = GetBankXorBits(log2BlkSize);
                        // Put all the part back together
   blockOffset <<= bankBits;
   blockOffset |= bankXor;
   blockOffset <<= pipeBits;
   blockOffset |= pipeXor;
                     ADDR_ASSERT((blockOffset | mipTailBytesOffset) == (blockOffset + mipTailBytesOffset));
                                          UINT_32 xb = pIn->x / localOut.blockWidth  + mipStartPos.w;
                  UINT_32 pitchInBlock = localOut.mipChainPitch / localOut.blockWidth;
   UINT_32 sliceSizeInBlock =
                     }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::ComputeSurfaceInfoLinear
   *
   *   @brief
   *       Internal function to calculate padding for linear swizzle 2D/3D surface
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx9Lib::ComputeSurfaceLinearPadding(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,                    ///< [in] input srtucture
   UINT_32*                                pMipmap0PaddedWidth,    ///< [out] padded width in element
   UINT_32*                                pSlice0PaddedHeight,    ///< [out] padded height for HW
   ADDR2_MIP_INFO*                         pMipInfo                ///< [out] per mip information
      {
               UINT_32 elementBytes        = pIn->bpp >> 3;
            if (pIn->swizzleMode == ADDR_SW_LINEAR_GENERAL)
   {
      ADDR_ASSERT(pIn->numMipLevels <= 1);
   ADDR_ASSERT(pIn->numSlices <= 1);
      }
   else
   {
                  UINT_32 mipChainWidth      = PowTwoAlign(pIn->width, pitchAlignInElement);
            returnCode = ApplyCustomizedPitchHeight(pIn, elementBytes, pitchAlignInElement,
            if (returnCode == ADDR_OK)
   {
      UINT_32 mipChainHeight = 0;
   UINT_32 mipHeight      = pIn->height;
            for (UINT_32 i = 0; i < pIn->numMipLevels; i++)
   {
         if (pMipInfo != NULL)
   {
      pMipInfo[i].offset = mipChainWidth * mipChainHeight * elementBytes;
   pMipInfo[i].pitch  = mipChainWidth;
                     mipChainHeight += mipHeight;
   mipHeight = RoundHalf(mipHeight);
            *pMipmap0PaddedWidth = mipChainWidth;
                  }
      /**
   ************************************************************************************************************************
   *   Gfx9Lib::ComputeThinBlockDimension
   *
   *   @brief
   *       Internal function to get thin block width/height/depth in element from surface input params.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx9Lib::ComputeThinBlockDimension(
      UINT_32*         pWidth,
   UINT_32*         pHeight,
   UINT_32*         pDepth,
   UINT_32          bpp,
   UINT_32          numSamples,
   AddrResourceType resourceType,
      {
               const UINT_32 log2BlkSize              = GetBlockSizeLog2(swizzleMode);
   const UINT_32 eleBytes                 = bpp >> 3;
   const UINT_32 microBlockSizeTableIndex = Log2(eleBytes);
   const UINT_32 log2blkSizeIn256B        = log2BlkSize - 8;
   const UINT_32 widthAmp                 = log2blkSizeIn256B / 2;
                     *pWidth  = (Block256_2d[microBlockSizeTableIndex].w << widthAmp);
   *pHeight = (Block256_2d[microBlockSizeTableIndex].h << heightAmp);
            if (numSamples > 1)
   {
      const UINT_32 log2sample = Log2(numSamples);
   const UINT_32 q          = log2sample >> 1;
            if (log2BlkSize & 1)
   {
         *pWidth  >>= q;
   }
   else
   {
         *pWidth  >>= (q + r);
         }
      } // V2
   } // Addr
