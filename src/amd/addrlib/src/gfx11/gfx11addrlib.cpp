   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ************************************************************************************************************************
   * @file  gfx11addrlib.cpp
   * @brief Contain the implementation for the Gfx11Lib class.
   ************************************************************************************************************************
   */
      #include "gfx11addrlib.h"
   #include "gfx11_gb_reg.h"
      #include "amdgpu_asic_addr.h"
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      namespace Addr
   {
   /**
   ************************************************************************************************************************
   *   Gfx11HwlInit
   *
   *   @brief
   *       Creates an Gfx11Lib object.
   *
   *   @return
   *       Returns an Gfx11Lib object pointer.
   ************************************************************************************************************************
   */
   Addr::Lib* Gfx11HwlInit(const Client* pClient)
   {
         }
      namespace V2
   {
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Static Const Member
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      const SwizzleModeFlags Gfx11Lib::SwizzleModeTable[ADDR_SW_MAX_TYPE] =
   {//Linear 256B  4KB  64KB  256KB   Z    Std   Disp  Rot   XOR    T     RtOpt Reserved
      {{1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_LINEAR
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    1,    0,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0}}, // ADDR_SW_256B_D
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    1,    0,    0,    0,    1,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_4KB_S
   {{0,    0,    1,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0}}, // ADDR_SW_4KB_D
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    1,    0,    0,    1,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_64KB_S
   {{0,    0,    0,    1,    0,    0,    0,    1,    0,    0,    0,    0,    0}}, // ADDR_SW_64KB_D
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    1,    0,    0,    1,    0,    0,    1,    1,    0,    0}}, // ADDR_SW_64KB_S_T
   {{0,    0,    0,    1,    0,    0,    0,    1,    0,    1,    1,    0,    0}}, // ADDR_SW_64KB_D_T
            {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    1,    0,    0,    0,    1,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_4KB_S_X
   {{0,    0,    1,    0,    0,    0,    0,    1,    0,    1,    0,    0,    0}}, // ADDR_SW_4KB_D_X
            {{0,    0,    0,    1,    0,    1,    0,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_64KB_Z_X
   {{0,    0,    0,    1,    0,    0,    1,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_64KB_S_X
   {{0,    0,    0,    1,    0,    0,    0,    1,    0,    1,    0,    0,    0}}, // ADDR_SW_64KB_D_X
            {{0,    0,    0,    0,    1,    1,    0,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_256KB_Z_X
   {{0,    0,    0,    0,    1,    0,    1,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_256KB_S_X
   {{0,    0,    0,    0,    1,    0,    0,    1,    0,    1,    0,    0,    0}}, // ADDR_SW_256KB_D_X
   {{0,    0,    0,    0,    1,    0,    0,    0,    0,    1,    0,    1,    0}}, // ADDR_SW_256KB_R_X
      };
      const Dim3d Gfx11Lib::Block256_3d[] = {{8, 4, 8}, {4, 4, 8}, {4, 4, 4}, {4, 2, 4}, {2, 2, 4}};
      const Dim3d Gfx11Lib::Block256K_Log2_3d[] = {{6, 6, 6}, {5, 6, 6}, {5, 6, 5}, {5, 5, 5}, {4, 5, 5}};
   const Dim3d Gfx11Lib::Block64K_Log2_3d[]  = {{6, 5, 5}, {5, 5, 5}, {5, 5, 4}, {5, 4, 4}, {4, 4, 4}};
   const Dim3d Gfx11Lib::Block4K_Log2_3d[]   = {{4, 4, 4}, {3, 4, 4}, {3, 4, 3}, {3, 3, 3}, {2, 3, 3}};
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::Gfx11Lib
   *
   *   @brief
   *       Constructor
   *
   ************************************************************************************************************************
   */
   Gfx11Lib::Gfx11Lib(const Client* pClient)
      :
   Lib(pClient),
   m_numPkrLog2(0),
   m_numSaLog2(0),
   m_colorBaseIndex(0),
   m_htileBaseIndex(0),
      {
      memset(&m_settings, 0, sizeof(m_settings));
      }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::~Gfx11Lib
   *
   *   @brief
   *       Destructor
   ************************************************************************************************************************
   */
   Gfx11Lib::~Gfx11Lib()
   {
   }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeHtileInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeHtilenfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeHtileInfo(
      const ADDR2_COMPUTE_HTILE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_HTILE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if ((pIn->swizzleMode != ADDR_SW_64KB_Z_X)  &&
      (pIn->swizzleMode != ADDR_SW_256KB_Z_X) &&
      {
         }
   else
   {
      Dim3d         metaBlk     = {};
   const UINT_32 metaBlkSize = GetMetaBlkSize(Gfx11DataDepthStencil,
                                          pOut->pitch         = PowTwoAlign(pIn->unalignedWidth,  metaBlk.w);
   pOut->height        = PowTwoAlign(pIn->unalignedHeight, metaBlk.h);
   pOut->baseAlign     = Max(metaBlkSize, 1u << (m_pipesLog2 + 11u));
   pOut->metaBlkWidth  = metaBlk.w;
            if (pIn->numMipLevels > 1)
   {
                           for (INT_32 i = static_cast<INT_32>(pIn->firstMipIdInTail) - 1; i >=0; i--)
                                                                  if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[i].inMiptail = FALSE;
                                 pOut->sliceSize          = offset;
                  if (pOut->pMipInfo != NULL)
   {
      for (UINT_32 i = pIn->firstMipIdInTail; i < pIn->numMipLevels; i++)
   {
      pOut->pMipInfo[i].inMiptail = TRUE;
                     if (pIn->firstMipIdInTail != pIn->numMipLevels)
   {
            }
   else
   {
                        pOut->metaBlkNumPerSlice    = pitchInM * heightInM;
                  if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].inMiptail = FALSE;
   pOut->pMipInfo[0].offset    = 0;
               // Get the HTILE address equation (copied from HtileAddrFromCoord).
   // HTILE addressing depends on the number of samples, but this code doesn't support it yet.
   const UINT_32  index         = m_htileBaseIndex;
            ADDR_C_ASSERT(sizeof(GFX11_HTILE_SW_PATTERN[patIdxTable[index]]) == 72 * 2);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeDccInfo
   *
   *   @brief
   *       Interface function to compute DCC key info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeDccInfo(
      const ADDR2_COMPUTE_DCCINFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_DCCINFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if (IsLinear(pIn->swizzleMode) || IsBlock256b(pIn->swizzleMode))
   {
         }
   else
   {
      const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 numFragLog2 = Log2(Max(pIn->numFrags, 1u));
            GetCompressedBlockSizeLog2(Gfx11DataColor,
                                 pOut->compressBlkWidth  = 1 << compBlock.w;
   pOut->compressBlkHeight = 1 << compBlock.h;
            if (ret == ADDR_OK)
   {
         Dim3d         metaBlk     = {};
   const UINT_32 metaBlkSize = GetMetaBlkSize(Gfx11DataColor,
                                          pOut->dccRamBaseAlign   = metaBlkSize;
   pOut->metaBlkWidth      = metaBlk.w;
   pOut->metaBlkHeight     = metaBlk.h;
                  pOut->pitch             = PowTwoAlign(pIn->unalignedWidth,     metaBlk.w);
                  if (pIn->numMipLevels > 1)
                                                                                                   if (pOut->pMipInfo != NULL)
   {
                                                               if (pOut->pMipInfo != NULL)
   {
      for (UINT_32 i = pIn->firstMipIdInTail; i < pIn->numMipLevels; i++)
   {
                              if (pIn->firstMipIdInTail != pIn->numMipLevels)
   {
               }
   else
   {
                                          if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].inMiptail = FALSE;
   pOut->pMipInfo[0].offset    = 0;
                  // Get the DCC address equation (copied from DccAddrFromCoord)
   const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 numPipeLog2 = m_pipesLog2;
   UINT_32       index       = m_dccBaseIndex + elemLog2;
                  if (pIn->dccKeyFlags.pipeAligned)
                     if (m_numPkrLog2 < 2)
   {
         }
   else
                                    index += (m_numPkrLog2 - 2) * dccPipePerPkr * MaxNumOfBpp +
                  ADDR_C_ASSERT(sizeof(GFX11_DCC_R_X_SW_PATTERN[patIdxTable[index]]) == 68 * 2);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeHtileAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeHtileAddrFromCoord(
      const ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
      {
               if (pIn->numMipLevels > 1)
   {
         }
   else
   {
      ADDR2_COMPUTE_HTILE_INFO_INPUT input = {};
   input.size            = sizeof(input);
   input.hTileFlags      = pIn->hTileFlags;
   input.depthFlags      = pIn->depthflags;
   input.swizzleMode     = pIn->swizzleMode;
   input.unalignedWidth  = Max(pIn->unalignedWidth,  1u);
   input.unalignedHeight = Max(pIn->unalignedHeight, 1u);
   input.numSlices       = Max(pIn->numSlices,       1u);
            ADDR2_COMPUTE_HTILE_INFO_OUTPUT output = {};
                     if (returnCode == ADDR_OK)
   {
         const UINT_32  numSampleLog2 = Log2(pIn->numSamples);
   const UINT_32  pipeMask      = (1 << m_pipesLog2) - 1;
   const UINT_32  index         = m_htileBaseIndex + numSampleLog2;
   const UINT_8*  patIdxTable   = GFX11_HTILE_PATIDX;
   const UINT_32  blkSizeLog2   = Log2(output.metaBlkWidth) + Log2(output.metaBlkHeight) - 4;
   const UINT_32  blkMask       = (1 << blkSizeLog2) - 1;
   const UINT_32  blkOffset     = ComputeOffsetFromSwizzlePattern(GFX11_HTILE_SW_PATTERN[patIdxTable[index]],
                                 const UINT_32 xb       = pIn->x / output.metaBlkWidth;
   const UINT_32 yb       = pIn->y / output.metaBlkHeight;
   const UINT_32 pb       = output.pitch / output.metaBlkWidth;
                  pOut->addr = (static_cast<UINT_64>(output.sliceSize) * pIn->slice) +
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeHtileCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeHtileCoordFromAddr(
      const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
      {
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlSupportComputeDccAddrFromCoord
   *
   *   @brief
   *       Check whether HwlComputeDccAddrFromCoord() can be done for the input parameter
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlSupportComputeDccAddrFromCoord(
         {
               if ((pIn->resourceType != ADDR_RSRC_TEX_2D) ||
      ((pIn->swizzleMode != ADDR_SW_64KB_R_X) &&
         (pIn->dccKeyFlags.linear == TRUE) ||
   (pIn->numFrags > 1) ||
   (pIn->numMipLevels > 1) ||
      {
         }
   else if ((pIn->pitch == 0)         ||
            (pIn->metaBlkWidth == 0)  ||
      {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeDccAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeDccAddrFromCoord
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx11Lib::HwlComputeDccAddrFromCoord(
      const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT* pIn,  ///< [in] input structure
      {
      const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 numPipeLog2 = m_pipesLog2;
   const UINT_32 pipeMask    = (1 << numPipeLog2) - 1;
   UINT_32       index       = m_dccBaseIndex + elemLog2;
   const UINT_8* patIdxTable = (pIn->swizzleMode == ADDR_SW_64KB_R_X) ?
            if (pIn->dccKeyFlags.pipeAligned)
   {
               if (m_numPkrLog2 < 2)
   {
         }
   else
   {
                                 index += (m_numPkrLog2 - 2) * dccPipePerPkr * MaxNumOfBpp +
               const UINT_32  blkSizeLog2 = Log2(pIn->metaBlkWidth) + Log2(pIn->metaBlkHeight) + elemLog2 - 8;
   const UINT_32  blkMask     = (1 << blkSizeLog2) - 1;
   const UINT_32  blkOffset   = ComputeOffsetFromSwizzlePattern(GFX11_DCC_R_X_SW_PATTERN[patIdxTable[index]],
                                 const UINT_32 xb       = pIn->x / pIn->metaBlkWidth;
   const UINT_32 yb       = pIn->y / pIn->metaBlkHeight;
   const UINT_32 pb       = pIn->pitch / pIn->metaBlkWidth;
   const UINT_32 blkIndex = (yb * pb) + xb;
            pOut->addr = (static_cast<UINT_64>(pIn->dccRamSliceSize) * pIn->slice) +
            }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlInitGlobalParams
   *
   *   @brief
   *       Initializes global parameters
   *
   *   @return
   *       TRUE if all settings are valid
   *
   ************************************************************************************************************************
   */
   BOOL_32 Gfx11Lib::HwlInitGlobalParams(
         {
      BOOL_32              valid = TRUE;
                     switch (gbAddrConfig.bits.NUM_PIPES)
   {
      case ADDR_CONFIG_1_PIPE:
         m_pipes     = 1;
   m_pipesLog2 = 0;
   case ADDR_CONFIG_2_PIPE:
         m_pipes     = 2;
   m_pipesLog2 = 1;
   case ADDR_CONFIG_4_PIPE:
         m_pipes     = 4;
   m_pipesLog2 = 2;
   case ADDR_CONFIG_8_PIPE:
         m_pipes     = 8;
   m_pipesLog2 = 3;
   case ADDR_CONFIG_16_PIPE:
         m_pipes     = 16;
   m_pipesLog2 = 4;
   case ADDR_CONFIG_32_PIPE:
         m_pipes     = 32;
   m_pipesLog2 = 5;
   case ADDR_CONFIG_64_PIPE:
         m_pipes     = 64;
   m_pipesLog2 = 6;
   default:
         ADDR_ASSERT_ALWAYS();
               switch (gbAddrConfig.bits.PIPE_INTERLEAVE_SIZE)
   {
      case ADDR_CONFIG_PIPE_INTERLEAVE_256B:
         m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_256B;
   m_pipeInterleaveLog2  = 8;
   case ADDR_CONFIG_PIPE_INTERLEAVE_512B:
         m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_512B;
   m_pipeInterleaveLog2  = 9;
   case ADDR_CONFIG_PIPE_INTERLEAVE_1KB:
         m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_1KB;
   m_pipeInterleaveLog2  = 10;
   case ADDR_CONFIG_PIPE_INTERLEAVE_2KB:
         m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_2KB;
   m_pipeInterleaveLog2  = 11;
   default:
         ADDR_ASSERT_ALWAYS();
               // Addr::V2::Lib::ComputePipeBankXor()/ComputeSlicePipeBankXor() requires pipe interleave to be exactly 8 bits, and
   // any larger value requires a post-process (left shift) on the output pipeBankXor bits.
   // And more importantly, SW AddrLib doesn't support sw equation/pattern for PI != 256 case.
            // These fields are deprecated on GFX11; they do nothing on HW.
   m_maxCompFrag     = 1;
            // Skip unaligned case
            m_htileBaseIndex += m_pipesLog2 * MaxNumOfAA;
            m_numPkrLog2 = gbAddrConfig.bits.NUM_PKRS;
                     if (m_numPkrLog2 >= 2)
   {
      m_colorBaseIndex += (2 * m_numPkrLog2 - 2) * MaxNumOfBpp;
               // There is no so-called VAR swizzle mode on GFX11 and instead there are 4 256KB swizzle modes. Here we treat 256KB
   // swizzle mode as "VAR" swizzle mode for reusing exising facilities (e.g GetBlockSizeLog2()) provided by base class
            if (valid)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlConvertChipFamily
   *
   *   @brief
   *       Convert familyID defined in atiid.h to ChipFamily and set m_chipFamily/m_chipRevision
   *   @return
   *       ChipFamily
   ************************************************************************************************************************
   */
   ChipFamily Gfx11Lib::HwlConvertChipFamily(
      UINT_32 chipFamily,        ///< [in] chip family defined in atiih.h
      {
               switch (chipFamily)
   {
      case FAMILY_NV3:
         if (ASICREV_IS_NAVI31_P(chipRevision))
   {
   }
   if (ASICREV_IS_NAVI32_P(chipRevision))
   {
   }
   if (ASICREV_IS_NAVI33_P(chipRevision))
   {
   }
   case FAMILY_GFX1150:
         if (ASICREV_IS_GFX1150(chipRevision))
   {
         }
   case FAMILY_GFX1103:
         m_settings.isGfx1103 = 1;
   default:
                                 }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetBlk256SizeLog2
   *
   *   @brief
   *       Get block 256 size
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   void Gfx11Lib::GetBlk256SizeLog2(
      AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2,    ///< [in] number of samples
   Dim3d*           pBlock             ///< [out] block size
      {
      if (IsThin(resourceType, swizzleMode))
   {
               // On GFX11, Z and R modes are the same thing.
   if (IsZOrderSwizzle(swizzleMode) || IsRtOptSwizzle(swizzleMode))
   {
                  pBlock->w = (blockBits >> 1) + (blockBits & 1);
   pBlock->h = (blockBits >> 1);
      }
   else
   {
                        pBlock->d = (blockBits / 3) + (((blockBits % 3) > 0) ? 1 : 0);
   pBlock->w = (blockBits / 3) + (((blockBits % 3) > 1) ? 1 : 0);
         }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetCompressedBlockSizeLog2
   *
   *   @brief
   *       Get compress block size
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   void Gfx11Lib::GetCompressedBlockSizeLog2(
      Gfx11DataType    dataType,          ///< [in] Data type
   AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2,    ///< [in] number of samples
   Dim3d*           pBlock             ///< [out] block size
      {
      if (dataType == Gfx11DataColor)
   {
         }
   else
   {
      ADDR_ASSERT(dataType == Gfx11DataDepthStencil);
   pBlock->w = 3;
   pBlock->h = 3;
         }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetMetaOverlapLog2
   *
   *   @brief
   *       Get meta block overlap
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   INT_32 Gfx11Lib::GetMetaOverlapLog2(
      Gfx11DataType    dataType,          ///< [in] Data type
   AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2     ///< [in] number of samples
      {
      Dim3d compBlock;
            GetCompressedBlockSizeLog2(dataType, resourceType, swizzleMode, elemLog2, numSamplesLog2, &compBlock);
            const INT_32 compSizeLog2   = compBlock.w  + compBlock.h  + compBlock.d;
   const INT_32 blk256SizeLog2 = microBlock.w + microBlock.h + microBlock.d;
   const INT_32 maxSizeLog2    = Max(compSizeLog2, blk256SizeLog2);
   const INT_32 numPipesLog2   = GetEffectiveNumPipes();
            if (numPipesLog2 > 1)
   {
                  // In 16Bpp 8xaa, we lose 1 overlap bit because the block size reduction eats into a pipe anchor bit (y4)
   if ((elemLog2 == 4) && (numSamplesLog2 == 3))
   {
         }
   overlap = Max(overlap, 0);
      }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::Get3DMetaOverlapLog2
   *
   *   @brief
   *       Get 3d meta block overlap
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   INT_32 Gfx11Lib::Get3DMetaOverlapLog2(
      AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2           ///< [in] element size log2
      {
      Dim3d microBlock;
                              if ((overlap < 0) || (IsStandardSwizzle(resourceType, swizzleMode) == TRUE))
   {
         }
      }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetPipeRotateAmount
   *
   *   @brief
   *       Get pipe rotate amount
   *
   *   @return
   *       Pipe rotate amount
   ************************************************************************************************************************
   */
      INT_32 Gfx11Lib::GetPipeRotateAmount(
      AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode        ///< [in] Swizzle mode
      {
               if ((m_pipesLog2 >= (m_numSaLog2 + 1)) && (m_pipesLog2 > 1))
   {
      amount = ((m_pipesLog2 == (m_numSaLog2 + 1)) && IsRbAligned(resourceType, swizzleMode)) ?
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetMetaBlkSize
   *
   *   @brief
   *       Get metadata block size
   *
   *   @return
   *       Meta block size
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::GetMetaBlkSize(
      Gfx11DataType    dataType,          ///< [in] Data type
   AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2,    ///< [in] number of samples
   BOOL_32          pipeAlign,         ///< [in] pipe align
   Dim3d*           pBlock             ///< [out] block size
      {
               const INT_32 metaElemSizeLog2   = GetMetaElementSizeLog2(dataType);
   const INT_32 metaCacheSizeLog2  = GetMetaCacheSizeLog2(dataType);
   const INT_32 compBlkSizeLog2    = (dataType == Gfx11DataColor) ? 8 : 6 + numSamplesLog2 + elemLog2;
   const INT_32 metaBlkSamplesLog2 = numSamplesLog2;
   const INT_32 dataBlkSizeLog2    = GetBlockSizeLog2(swizzleMode);
            if (IsThin(resourceType, swizzleMode))
   {
      if ((pipeAlign == FALSE) ||
         (IsStandardSwizzle(resourceType, swizzleMode) == TRUE) ||
   {
         if (pipeAlign)
   {
      metablkSizeLog2 = Max(static_cast<INT_32>(m_pipeInterleaveLog2) + numPipesLog2, 12);
      }
   else
   {
         }
   else
   {
         if ((m_pipesLog2 == m_numSaLog2 + 1) && (m_pipesLog2 > 1))
   {
                           if (numPipesLog2 >= 4)
                     // In 16Bpe 8xaa, we have an extra overlap bit
   if ((pipeRotateLog2 > 0)  &&
      (elemLog2 == 4)       &&
   (numSamplesLog2 == 3) &&
                           metablkSizeLog2 = metaCacheSizeLog2 + overlapLog2 + numPipesLog2;
      }
   else
   {
                  if (dataType == Gfx11DataDepthStencil)
   {
                                 if  (IsRtOptSwizzle(swizzleMode) && (compFragLog2 > 1) && (pipeRotateLog2 >= 1))
                                 const INT_32 metablkBitsLog2 =
         pBlock->w = 1 << ((metablkBitsLog2 >> 1) + (metablkBitsLog2 & 1));
   pBlock->h = 1 << (metablkBitsLog2 >> 1);
      }
   else
   {
               if (pipeAlign)
   {
         if ((m_pipesLog2 == m_numSaLog2 + 1) &&
      (m_pipesLog2 > 1)                &&
      {
                           metablkSizeLog2 = metaCacheSizeLog2 + overlapLog2 + numPipesLog2;
   metablkSizeLog2 = Max(metablkSizeLog2, static_cast<INT_32>(m_pipeInterleaveLog2) + numPipesLog2);
   }
   else
   {
                  const INT_32 metablkBitsLog2 =
         pBlock->w = 1 << ((metablkBitsLog2 / 3) + (((metablkBitsLog2 % 3) > 0) ? 1 : 0));
   pBlock->h = 1 << ((metablkBitsLog2 / 3) + (((metablkBitsLog2 % 3) > 1) ? 1 : 0));
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ConvertSwizzlePatternToEquation
   *
   *   @brief
   *       Convert swizzle pattern to equation.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx11Lib::ConvertSwizzlePatternToEquation(
      UINT_32                elemLog2,  ///< [in] element bytes log2
   AddrResourceType       rsrcType,  ///< [in] resource type
   AddrSwizzleMode        swMode,    ///< [in] swizzle mode
   const ADDR_SW_PATINFO* pPatInfo,  ///< [in] swizzle pattern infor
   ADDR_EQUATION*         pEquation) ///< [out] equation converted from swizzle pattern
      {
      ADDR_BIT_SETTING fullSwizzlePattern[ADDR_MAX_EQUATION_BIT];
            const ADDR_BIT_SETTING* pSwizzle      = fullSwizzlePattern;
   const UINT_32           blockSizeLog2 = GetBlockSizeLog2(swMode);
   memset(pEquation, 0, sizeof(ADDR_EQUATION));
   pEquation->numBits            = blockSizeLog2;
   pEquation->numBitComponents   = pPatInfo->maxItemCount;
            for (UINT_32 i = 0; i < elemLog2; i++)
   {
      pEquation->addr[i].channel = 0;
   pEquation->addr[i].valid   = 1;
               if (IsXor(swMode) == FALSE)
   {
      // Use simplified logic when we only have one bit-component
   for (UINT_32 i = elemLog2; i < blockSizeLog2; i++)
   {
                  if (pSwizzle[i].x != 0)
                     pEquation->addr[i].channel = 0;
   pEquation->addr[i].valid   = 1;
      }
   else if (pSwizzle[i].y != 0)
                     pEquation->addr[i].channel = 1;
   pEquation->addr[i].valid   = 1;
      }
   else
   {
                     pEquation->addr[i].channel = 2;
   pEquation->addr[i].valid   = 1;
         }
   else
   {
      Dim3d dim;
            const UINT_32 blkXLog2 = Log2(dim.w);
   const UINT_32 blkYLog2 = Log2(dim.h);
   const UINT_32 blkZLog2 = Log2(dim.d);
   const UINT_32 blkXMask = dim.w - 1;
   const UINT_32 blkYMask = dim.h - 1;
            ADDR_BIT_SETTING swizzle[ADDR_MAX_EQUATION_BIT] = {};
   memcpy(&swizzle, pSwizzle, sizeof(swizzle));
   UINT_32          xMask = 0;
   UINT_32          yMask = 0;
            for (UINT_32 i = elemLog2; i < blockSizeLog2; i++)
   {
         for (UINT_32 bitComp = 0; bitComp < ADDR_MAX_EQUATION_COMP; bitComp++)
   {
      if (swizzle[i].value == 0)
   {
      ADDR_ASSERT(bitComp != 0); // Bits above element size must have at least one addr-bit
                     if (swizzle[i].x != 0)
   {
                           pEquation->comps[bitComp][i].channel = 0;
   pEquation->comps[bitComp][i].valid   = 1;
      }
   else if (swizzle[i].y != 0)
   {
                           pEquation->comps[bitComp][i].channel = 1;
   pEquation->comps[bitComp][i].valid   = 1;
      }
   else if (swizzle[i].z != 0)
   {
                           pEquation->comps[bitComp][i].channel = 2;
   pEquation->comps[bitComp][i].valid   = 1;
      }
   else
   {
      // This function doesn't handle MSAA (must update block dims, here, and consumers)
         }
            // We missed an address bit for coords inside the block?
   // That means two coords will land on the same addr, which is bad.
   ADDR_ASSERT(((xMask & blkXMask) == blkXMask) &&
               // We're sourcing from outside our block? That won't fly for PRTs, which need to be movable.
   // Non-xor modes can also be used for 2D PRTs but they're handled in the simplified logic above.
   ADDR_ASSERT((IsPrt(swMode) == false) ||
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::InitEquationTable
   *
   *   @brief
   *       Initialize Equation table.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx11Lib::InitEquationTable()
   {
               for (UINT_32 rsrcTypeIdx = 0; rsrcTypeIdx < MaxRsrcType; rsrcTypeIdx++)
   {
               for (UINT_32 swModeIdx = 0; swModeIdx < MaxSwModeType; swModeIdx++)
   {
                  for (UINT_32 elemLog2 = 0; elemLog2 < MaxElementBytesLog2; elemLog2++)
   {
                     if (pPatInfo != NULL)
                                                                                 }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlGetEquationIndex
   *
   *   @brief
   *       Interface function stub of GetEquationIndex
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::HwlGetEquationIndex(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if ((pIn->resourceType == ADDR_RSRC_TEX_2D) ||
         {
      const UINT_32 rsrcTypeIdx = static_cast<UINT_32>(pIn->resourceType) - 1;
   const UINT_32 swModeIdx   = static_cast<UINT_32>(pIn->swizzleMode);
                        if (pOut->pMipInfo != NULL)
   {
      for (UINT_32 i = 0; i < pIn->numMipLevels; i++)
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetValidDisplaySwizzleModes
   *
   *   @brief
   *       Get valid swizzle modes mask for displayable surface
   *
   *   @return
   *       Valid swizzle modes mask for displayable surface
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::GetValidDisplaySwizzleModes(
      UINT_32 bpp
      {
               if (bpp <= 64)
   {
                        if (false
         || (m_settings.isGfx1103)
         {
         // Not all GPUs support displaying with 256kB swizzle modes.
   swModeMask &= ~((1u << ADDR_SW_256KB_D_X) |
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::IsValidDisplaySwizzleMode
   *
   *   @brief
   *       Check if a swizzle mode is supported by display engine
   *
   *   @return
   *       TRUE is swizzle mode is supported by display engine
   ************************************************************************************************************************
   */
   BOOL_32 Gfx11Lib::IsValidDisplaySwizzleMode(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn     ///< [in] input structure
      {
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetMaxNumMipsInTail
   *
   *   @brief
   *       Return max number of mips in tails
   *
   *   @return
   *       Max number of mips in tails
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::GetMaxNumMipsInTail(
      UINT_32 blockSizeLog2,     ///< block size log2
   BOOL_32 isThin             ///< is thin or thick
      {
               if (isThin == FALSE)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputePipeBankXor
   *
   *   @brief
   *       Generate a PipeBankXor value to be ORed into bits above pipeInterleaveBits of address
   *
   *   @return
   *       PipeBankXor value
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputePipeBankXor(
      const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,     ///< [in] input structure
   ADDR2_COMPUTE_PIPEBANKXOR_OUTPUT*      pOut     ///< [out] output structure
      {
      if (IsNonPrtXor(pIn->swizzleMode))
   {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeSlicePipeBankXor
   *
   *   @brief
   *       Generate slice PipeBankXor value based on base PipeBankXor value and slice id
   *
   *   @return
   *       PipeBankXor value
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeSlicePipeBankXor(
      const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,   ///< [in] input structure
   ADDR2_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT*      pOut   ///< [out] output structure
      {
               if (IsNonPrtXor(pIn->swizzleMode))
   {
      if (pIn->bpe == 0)
   {
                  // Require a valid bytes-per-element value passed from client...
   }
   else
   {
         const ADDR_SW_PATINFO* pPatInfo = GetSwizzlePatternInfo(pIn->swizzleMode,
                        if (pPatInfo != NULL)
   {
                     const UINT_32 pipeBankXorOffset =
      ComputeOffsetFromSwizzlePattern(reinterpret_cast<const UINT_64*>(fullSwizzlePattern),
                                                            }
   else
   {
                           }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeSubResourceOffsetForSwizzlePattern
   *
   *   @brief
   *       Compute sub resource offset to support swizzle pattern
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeSubResourceOffsetForSwizzlePattern(
      const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT*      pOut    ///< [out] output structure
      {
                           }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeNonBlockCompressedView
   *
   *   @brief
   *       Compute non-block-compressed view for a given mipmap level/slice.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeNonBlockCompressedView(
      const ADDR2_COMPUTE_NONBLOCKCOMPRESSEDVIEW_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_NONBLOCKCOMPRESSEDVIEW_OUTPUT*      pOut    ///< [out] output structure
      {
               if (IsThin(pIn->resourceType, pIn->swizzleMode) == FALSE)
   {
      // Only thin swizzle mode can have a NonBC view...
      }
   else if (((pIn->format < ADDR_FMT_ASTC_4x4) || (pIn->format > ADDR_FMT_ETC2_128BPP)) &&
         {
      // Only support BC1~BC7, ASTC, or ETC2 for now...
      }
   else
   {
      UINT_32 bcWidth, bcHeight;
            ADDR2_COMPUTE_SURFACE_INFO_INPUT infoIn = {};
   infoIn.flags        = pIn->flags;
   infoIn.swizzleMode  = pIn->swizzleMode;
   infoIn.resourceType = pIn->resourceType;
   infoIn.bpp          = bpp;
   infoIn.width        = RoundUpQuotient(pIn->width, bcWidth);
   infoIn.height       = RoundUpQuotient(pIn->height, bcHeight);
   infoIn.numSlices    = pIn->numSlices;
   infoIn.numMipLevels = pIn->numMipLevels;
   infoIn.numSamples   = 1;
                     ADDR2_COMPUTE_SURFACE_INFO_OUTPUT infoOut = {};
                     if (tiled)
   {
         }
   else
   {
                  if (returnCode == ADDR_OK)
   {
         ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT subOffIn = {};
   subOffIn.swizzleMode      = infoIn.swizzleMode;
   subOffIn.resourceType     = infoIn.resourceType;
   subOffIn.slice            = pIn->slice;
   subOffIn.sliceSize        = infoOut.sliceSize;
                           // For any mipmap level, move nonBc view base address by offset
                  ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT slicePbXorIn = {};
   slicePbXorIn.bpe             = infoIn.bpp;
   slicePbXorIn.swizzleMode     = infoIn.swizzleMode;
   slicePbXorIn.resourceType    = infoIn.resourceType;
                           // For any mipmap level, nonBc view should use computed pbXor
                  const BOOL_32 inTail           = tiled && (pIn->mipId >= infoOut.firstMipIdInTail) ? TRUE : FALSE;
                  if (inTail)
   {
                                                                        // - (mip0) height = requestMipHeight << mipId, the value can't exceed mip tail dimension threshold
      }
   // This check should cover at least mipId == 0
   else if (requestMipWidth << pIn->mipId == infoIn.width)
   {
      // For mipmap level [N] that is not in mip tail block and downgraded without losing element:
                                       // (mip0) height = requestMipHeight
      }
   else
   {
      // For mipmap level [N] that is not in mip tail block and downgraded with element losing,
   // We have to make it a multiple mipmap view (2 levels view here), add one extra element if needed,
   // because single mip view may have different pitch value than original (multiple) mip view...
   // A simple case would be:
   // - 64KB block swizzle mode, 8 Bytes-Per-Element. Block dim = [0x80, 0x40]
   // - 2 mipmap levels with API mip0 width = 0x401/mip1 width = 0x200 and non-BC view
                                                                                                const BOOL_32 needExtraWidth =
      ((upperMipWidth < requestMipWidth * 2) ||
                     const BOOL_32 needExtraHeight =
      ((upperMipHeight < requestMipHeight * 2) ||
                                                      // Assert the downgrading from this mip[0] width would still generate correct mip[N] width
   ADDR_ASSERT(ShiftRight(pOut->unalignedWidth, pOut->mipId) == requestMipWidth);
   // Assert the downgrading from this mip[0] height would still generate correct mip[N] height
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ValidateNonSwModeParams
   *
   *   @brief
   *       Validate compute surface info params except swizzle mode
   *
   *   @return
   *       TRUE if parameters are valid, FALSE otherwise
   ************************************************************************************************************************
   */
   BOOL_32 Gfx11Lib::ValidateNonSwModeParams(
         {
               if ((pIn->bpp == 0) || (pIn->bpp > 128) || (pIn->width == 0) || (pIn->numFrags > 8))
   {
      ADDR_ASSERT_ALWAYS();
      }
   else if (pIn->flags.fmask == 1)
   {
      // There is no FMASK for GFX11 ASICs
   ADDR_ASSERT_ALWAYS();
      }
   else if (pIn->numSamples > 8)
   {
      // There is no EQAA support for GFX11 ASICs, so the max number of sample is 8
   ADDR_ASSERT_ALWAYS();
      }
   else if ((pIn->numFrags != 0) && (pIn->numSamples != pIn->numFrags))
   {
      // There is no EQAA support for GFX11 ASICs, so the number of sample has to be same as number of fragment
   ADDR_ASSERT_ALWAYS();
               const ADDR2_SURFACE_FLAGS flags    = pIn->flags;
   const AddrResourceType    rsrcType = pIn->resourceType;
   const BOOL_32             mipmap   = (pIn->numMipLevels > 1);
   const BOOL_32             msaa     = (pIn->numSamples > 1);
   const BOOL_32             display  = flags.display;
   const BOOL_32             tex3d    = IsTex3d(rsrcType);
   const BOOL_32             tex2d    = IsTex2d(rsrcType);
   const BOOL_32             tex1d    = IsTex1d(rsrcType);
            // Resource type check
   if (tex1d)
   {
      if (msaa || display || stereo)
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
      if (msaa || display || stereo)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ValidateSwModeParams
   *
   *   @brief
   *       Validate compute surface info related to swizzle mode
   *
   *   @return
   *       TRUE if parameters are valid, FALSE otherwise
   ************************************************************************************************************************
   */
   BOOL_32 Gfx11Lib::ValidateSwModeParams(
         {
               if (pIn->swizzleMode >= ADDR_SW_MAX_TYPE)
   {
      ADDR_ASSERT_ALWAYS();
      }
   else if (IsValidSwMode(pIn->swizzleMode) == FALSE)
   {
      ADDR_ASSERT_ALWAYS();
               const ADDR2_SURFACE_FLAGS flags       = pIn->flags;
   const AddrResourceType    rsrcType    = pIn->resourceType;
   const AddrSwizzleMode     swizzle     = pIn->swizzleMode;
   const BOOL_32             msaa        = (pIn->numSamples > 1);
   const BOOL_32             zbuffer     = flags.depth || flags.stencil;
   const BOOL_32             color       = flags.color;
   const BOOL_32             display     = flags.display;
   const BOOL_32             tex3d       = IsTex3d(rsrcType);
   const BOOL_32             tex2d       = IsTex2d(rsrcType);
   const BOOL_32             tex1d       = IsTex1d(rsrcType);
   const BOOL_32             thin3d      = flags.view3dAs2dArray;
   const BOOL_32             linear      = IsLinear(swizzle);
   const BOOL_32             blk256B     = IsBlock256b(swizzle);
   const BOOL_32             isNonPrtXor = IsNonPrtXor(swizzle);
            // Misc check
   if (msaa && (GetBlockSize(swizzle) < (m_pipeInterleaveBytes * pIn->numSamples)))
   {
      // MSAA surface must have blk_bytes/pipe_interleave >= num_samples
   ADDR_ASSERT_ALWAYS();
               if (display && (IsValidDisplaySwizzleMode(pIn) == FALSE))
   {
      ADDR_ASSERT_ALWAYS();
               if ((pIn->bpp == 96) && (linear == FALSE))
   {
      ADDR_ASSERT_ALWAYS();
                        // Resource type check
   if (tex1d)
   {
      if ((swizzleMask & Gfx11Rsrc1dSwModeMask) == 0)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (tex2d)
   {
      if ((swizzleMask & Gfx11Rsrc2dSwModeMask) == 0)
   {
         ADDR_ASSERT_ALWAYS();
   }
   else if (prt && ((swizzleMask & Gfx11Rsrc2dPrtSwModeMask) == 0))
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (tex3d)
   {
      if (((swizzleMask & Gfx11Rsrc3dSwModeMask) == 0) ||
         (prt && ((swizzleMask & Gfx11Rsrc3dPrtSwModeMask) == 0)) ||
   {
         ADDR_ASSERT_ALWAYS();
               // Swizzle type check
   if (linear)
   {
      if (zbuffer || msaa || (pIn->bpp == 0) || ((pIn->bpp % 8) != 0))
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsZOrderSwizzle(swizzle))
   {
      if ((pIn->bpp > 64)                         ||
         (msaa && (color || (pIn->bpp > 32)))    ||
   ElemLib::IsBlockCompressed(pIn->format) ||
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsStandardSwizzle(rsrcType, swizzle))
   {
      if (zbuffer || msaa)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsDisplaySwizzle(rsrcType, swizzle))
   {
      if (zbuffer || msaa)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (IsRtOptSwizzle(swizzle))
   {
      if (zbuffer)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else
   {
      ADDR_ASSERT_ALWAYS();
               // Block type check
   if (blk256B)
   {
      if (zbuffer || tex3d || msaa)
   {
         ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeSurfaceInfoSanityCheck
   *
   *   @brief
   *       Compute surface info sanity check
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeSurfaceInfoSanityCheck(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn     ///< [in] input structure
      {
         }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlGetPreferredSurfaceSetting
   *
   *   @brief
   *       Internal function to get suggested surface information for cliet to use
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlGetPreferredSurfaceSetting(
      const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,  ///< [in] input structure
   ADDR2_GET_PREFERRED_SURF_SETTING_OUTPUT*      pOut  ///< [out] output structure
      {
               if (pIn->flags.fmask)
   {
      // There is no FMASK for GFX11 ASICs.
               }
   else
   {
      UINT_32 bpp    = pIn->bpp;
   UINT_32 width  = Max(pIn->width, 1u);
            // Set format to INVALID will skip this conversion
   if (pIn->format != ADDR_FMT_INVALID)
   {
                        // Get compression/expansion factors and element mode which indicates compression/expansion
   bpp = GetElemLib()->GetBitsPerPixel(pIn->format,
                        UINT_32 basePitch = 0;
   GetElemLib()->AdjustSurfaceInfo(elemMode,
                                          const UINT_32 numSlices    = Max(pIn->numSlices,    1u);
   const UINT_32 numMipLevels = Max(pIn->numMipLevels, 1u);
   const UINT_32 numSamples   = Max(pIn->numSamples,   1u);
            // Pre sanity check on non swizzle mode parameters
   ADDR2_COMPUTE_SURFACE_INFO_INPUT localIn = {};
   localIn.flags        = pIn->flags;
   localIn.resourceType = pIn->resourceType;
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
   allowedSwModeSet.value |= pIn->forbiddenBlock.linear ? 0 : Gfx11LinearSwModeMask;
   allowedSwModeSet.value |= pIn->forbiddenBlock.micro  ? 0 : Gfx11Blk256BSwModeMask;
   allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThin4KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThick4KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThin64KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThick64KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.gfx11.thin256KB ? 0 :
      allowedSwModeSet.value |=
                  if (pIn->preferredSwSet.value != 0)
   {
      allowedSwModeSet.value &= pIn->preferredSwSet.sw_Z ? ~0 : ~Gfx11ZSwModeMask;
   allowedSwModeSet.value &= pIn->preferredSwSet.sw_S ? ~0 : ~Gfx11StandardSwModeMask;
                     if (pIn->noXor)
   {
                  if (pIn->maxAlign > 0)
   {
      if (pIn->maxAlign < Size256K)
                        if (pIn->maxAlign < Size64K)
                        if (pIn->maxAlign < Size4K)
                        if (pIn->maxAlign < Size256)
   {
                     // Filter out invalid swizzle mode(s) by image attributes and HW restrictions
   switch (pIn->resourceType)
   {
                                                                  if (pIn->flags.view3dAs2dArray)
   {
                     default:
      ADDR_ASSERT_ALWAYS();
                  if (ElemLib::IsBlockCompressed(pIn->format)  ||
      ElemLib::IsMacroPixelPacked(pIn->format) ||
   (bpp > 64)                               ||
      {
                  if (pIn->format == ADDR_FMT_32_32_32)
   {
                  if (msaa)
   {
                  if (pIn->flags.depth || pIn->flags.stencil)
   {
                  if (pIn->flags.display)
   {
                  #if DEBUG
                                 for (UINT_32 i = 0; validateSwModeSet != 0; i++)
   {
      if (validateSwModeSet & 1)
   {
                  #endif
                                                                  if (pOut->clientPreferredSwSet.value == 0)
                        // Apply optional restrictions
   if (pIn->flags.needEquation)
   {
      UINT_32 components = pIn->flags.allowExtEquation ?  ADDR_MAX_EQUATION_COMP :
                     if (allowedSwModeSet.value == Gfx11LinearSwModeMask)
   {
         }
                           if ((height > 1) && (computeMinSize == FALSE))
   {
         // Always ignore linear swizzle mode if:
                                       // Determine block size if there are 2 or more block type candidates
                                       if (pOut->resourceType == ADDR_RSRC_TEX_3D)
   {
      swMode[AddrBlockThick4KB]   = ADDR_SW_4KB_S_X;
   swMode[AddrBlockThin64KB]   = ADDR_SW_64KB_R_X;
   swMode[AddrBlockThick64KB]  = ADDR_SW_64KB_S_X;
   swMode[AddrBlockThin256KB]  = ADDR_SW_256KB_R_X;
      }
   else
   {
                                             const UINT_32 ratioLow           = computeMinSize ? 1 : (pIn->flags.opt4space ? 3 : 2);
                                       for (UINT_32 i = AddrBlockLinear; i < AddrBlockMaxTiledType; i++)
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
      case AddrBlockThick256KB:
         case AddrBlockThin256KB:
         case AddrBlockThick64KB:
         case AddrBlockThin64KB:
         case AddrBlockThick4KB:
         case AddrBlockThin4KB:
                                                            for (UINT_32 i = AddrBlockMicro; i < AddrBlockMaxTiledType; i++)
   {
      if ((i != minSizeBlk) &&
         {
         if (Addr2BlockTypeWithinMemoryBudget(minSize, padSize[i], 0, 0, pIn->memoryBudget) == FALSE)
                                                                                                               switch (minSizeBlk)
                                                                                                                                                                                                                              default:
                                                               // Determine swizzle type if there are 2 or more swizzle type candidates
   if ((allowedSwSet.value != 0) && (IsPow2(allowedSwSet.value) == FALSE))
   {
         if (ElemLib::IsBlockCompressed(pIn->format))
   {
      if (allowedSwSet.sw_D)
   {
         }
   else if (allowedSwSet.sw_S)
   {
         }
   else
   {
      ADDR_ASSERT(allowedSwSet.sw_R);
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
   else if (pIn->resourceType == ADDR_RSRC_TEX_3D)
   {
      if (pIn->flags.color && allowedSwSet.sw_R)
   {
         }
   else if (allowedSwSet.sw_S)
   {
         }
   else if (allowedSwSet.sw_D)
   {
         }
   else
   {
      ADDR_ASSERT(allowedSwSet.sw_Z);
         }
   else
   {
      if (allowedSwSet.sw_R)
   {
         }
   else if (allowedSwSet.sw_D)
   {
         }
   else if (allowedSwSet.sw_Z)
   {
         }
                                                      // Determine swizzle mode now. Always select the "largest" swizzle mode for a given block type +
   // swizzle type combination. E.g, for AddrBlockThin64KB + ADDR_SW_S, select SW_64KB_S_X(25) if it's
   // available, or otherwise select SW_64KB_S_T(17) if it's available, or otherwise select SW_64KB_S(9).
         }
   else
   {
      // Invalid combination...
   ADDR_ASSERT_ALWAYS();
      }
   else
   {
         // Invalid combination...
   ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlGetPossibleSwizzleModes
   *
   *   @brief
   *       Returns a list of swizzle modes that are valid from the hardware's perspective for the client to choose from
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlGetPossibleSwizzleModes(
      const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,  ///< [in] input structure
   ADDR2_GET_PREFERRED_SURF_SETTING_OUTPUT*      pOut  ///< [out] output structure
      {
               if (pIn->flags.fmask)
   {
      // There is no FMASK for GFX11 ASICs.
               }
   else
   {
      UINT_32 bpp    = pIn->bpp;
   UINT_32 width  = Max(pIn->width, 1u);
            // Set format to INVALID will skip this conversion
   if (pIn->format != ADDR_FMT_INVALID)
   {
                        // Get compression/expansion factors and element mode which indicates compression/expansion
   bpp = GetElemLib()->GetBitsPerPixel(pIn->format,
                        UINT_32 basePitch = 0;
   GetElemLib()->AdjustSurfaceInfo(elemMode,
      expandX,
   expandY,
   &bpp,
   &basePitch,
               const UINT_32 numSlices    = Max(pIn->numSlices, 1u);
   const UINT_32 numMipLevels = Max(pIn->numMipLevels, 1u);
   const UINT_32 numSamples   = Max(pIn->numSamples, 1u);
            // Pre sanity check on non swizzle mode parameters
   ADDR2_COMPUTE_SURFACE_INFO_INPUT localIn = {};
   localIn.flags = pIn->flags;
   localIn.resourceType = pIn->resourceType;
   localIn.format = pIn->format;
   localIn.bpp = bpp;
   localIn.width = width;
   localIn.height = height;
   localIn.numSlices = numSlices;
   localIn.numMipLevels = numMipLevels;
   localIn.numSamples = numSamples;
            if (ValidateNonSwModeParams(&localIn))
   {
         // Allow appropriate swizzle modes by default
   ADDR2_SWMODE_SET allowedSwModeSet = {};
   allowedSwModeSet.value |= Gfx11LinearSwModeMask | Gfx11Blk256BSwModeMask;
   if (pIn->resourceType == ADDR_RSRC_TEX_3D)
   {
      allowedSwModeSet.value |= Gfx11Rsrc3dThick4KBSwModeMask  |
                        }
   else
   {
                  // Filter out invalid swizzle mode(s) by image attributes and HW restrictions
   switch (pIn->resourceType)
   {
   case ADDR_RSRC_TEX_1D:
                  case ADDR_RSRC_TEX_2D:
                                    if (pIn->flags.view3dAs2dArray)
   {
                     default:
      ADDR_ASSERT_ALWAYS();
                     // TODO: figure out if following restrictions are correct on GFX11...
   if (ElemLib::IsBlockCompressed(pIn->format) ||
      ElemLib::IsMacroPixelPacked(pIn->format) ||
   (bpp > 64) ||
      {
                  if (pIn->format == ADDR_FMT_32_32_32)
   {
                  if (msaa)
   {
                  if (pIn->flags.depth || pIn->flags.stencil)
   {
                  if (pIn->flags.display)
   {
                  #if DEBUG
                                 for (UINT_32 i = 0; validateSwModeSet != 0; i++)
   {
      if (validateSwModeSet & 1)
   {
                  #endif
                                    if (pOut->clientPreferredSwSet.value == 0)
                        if (pIn->flags.needEquation)
   {
      UINT_32 components = pIn->flags.allowExtEquation ?  ADDR_MAX_EQUATION_COMP :
                     pOut->validSwModeSet = allowedSwModeSet;
      }
   else
   {
      // Invalid combination...
   ADDR_ASSERT_ALWAYS();
      }
   else
   {
         // Invalid combination...
   ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlGetAllowedBlockSet
   *
   *   @brief
   *       Returns the set of allowed block sizes given the allowed swizzle modes and resource type
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlGetAllowedBlockSet(
      ADDR2_SWMODE_SET allowedSwModeSet,  ///< [in] allowed swizzle modes
   AddrResourceType rsrcType,          ///< [in] resource type
   ADDR2_BLOCK_SET* pAllowedBlockSet   ///< [out] allowed block sizes
      {
               allowedBlockSet.micro  = (allowedSwModeSet.value & Gfx11Blk256BSwModeMask) ? TRUE : FALSE;
            if (rsrcType == ADDR_RSRC_TEX_3D)
   {
      allowedBlockSet.macroThick4KB    = (allowedSwModeSet.value & Gfx11Rsrc3dThick4KBSwModeMask)   ? TRUE : FALSE;
   allowedBlockSet.macroThin64KB    = (allowedSwModeSet.value & Gfx11Rsrc3dThin64KBSwModeMask)   ? TRUE : FALSE;
   allowedBlockSet.macroThick64KB   = (allowedSwModeSet.value & Gfx11Rsrc3dThick64KBSwModeMask)  ? TRUE : FALSE;
   allowedBlockSet.gfx11.thin256KB  = (allowedSwModeSet.value & Gfx11Rsrc3dThin256KBSwModeMask)  ? TRUE : FALSE;
      }
   else
   {
      allowedBlockSet.macroThin4KB    = (allowedSwModeSet.value & Gfx11Blk4KBSwModeMask)   ? TRUE : FALSE;
   allowedBlockSet.macroThin64KB   = (allowedSwModeSet.value & Gfx11Blk64KBSwModeMask)  ? TRUE : FALSE;
               *pAllowedBlockSet = allowedBlockSet;
      }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlGetAllowedSwSet
   *
   *   @brief
   *       Returns the set of allowed swizzle types given the allowed swizzle modes
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlGetAllowedSwSet(
      ADDR2_SWMODE_SET  allowedSwModeSet, ///< [in] allowed swizzle modes
   ADDR2_SWTYPE_SET* pAllowedSwSet     ///< [out] allowed swizzle types
      {
               allowedSwSet.sw_Z = (allowedSwModeSet.value & Gfx11ZSwModeMask)        ? TRUE : FALSE;
   allowedSwSet.sw_S = (allowedSwModeSet.value & Gfx11StandardSwModeMask) ? TRUE : FALSE;
   allowedSwSet.sw_D = (allowedSwModeSet.value & Gfx11DisplaySwModeMask)  ? TRUE : FALSE;
            *pAllowedSwSet = allowedSwSet;
      }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeStereoInfo
   *
   *   @brief
   *       Compute height alignment and right eye pipeBankXor for stereo surface
   *
   *   @return
   *       Error code
   *
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::ComputeStereoInfo(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,        ///< Compute surface info
   UINT_32*                                pAlignY,    ///< Stereo requested additional alignment in Y
   UINT_32*                                pRightXor   ///< Right eye xor
      {
                        if (IsNonPrtXor(pIn->swizzleMode))
   {
      const UINT_32 blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);
   const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 rsrcType    = static_cast<UINT_32>(pIn->resourceType) - 1;
   const UINT_32 swMode      = static_cast<UINT_32>(pIn->swizzleMode);
            if (eqIndex != ADDR_INVALID_EQUATION_INDEX)
   {
                        // First get "max y bit"
   for (UINT_32 i = m_pipeInterleaveLog2; i < blkSizeLog2; i++)
                     if ((m_equationTable[eqIndex].addr[i].channel == 1) &&
                              if ((m_equationTable[eqIndex].xor1[i].valid == 1) &&
      (m_equationTable[eqIndex].xor1[i].channel == 1) &&
                           if ((m_equationTable[eqIndex].xor2[i].valid == 1) &&
      (m_equationTable[eqIndex].xor2[i].channel == 1) &&
      {
                     // Then loop again for populating a position mask of "max Y bit"
   for (UINT_32 i = m_pipeInterleaveLog2; i < blkSizeLog2; i++)
   {
      if ((m_equationTable[eqIndex].addr[i].channel == 1) &&
         {
         }
   else if ((m_equationTable[eqIndex].xor1[i].valid == 1) &&
               {
         }
   else if ((m_equationTable[eqIndex].xor2[i].valid == 1) &&
               {
                              if (additionalAlign >= *pAlignY)
                              if ((alignedHeight >> yMax) & 1)
   {
            }
   else
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeSurfaceInfoTiled
   *
   *   @brief
   *       Internal function to calculate alignment for tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeSurfaceInfoTiled(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               // Mip chain dimesion and epitch has no meaning in GFX11, set to default value
   pOut->mipChainPitch    = 0;
   pOut->mipChainHeight   = 0;
   pOut->mipChainSlice    = 0;
            // Following information will be provided in ComputeSurfaceInfoMacroTiled() if necessary
   pOut->mipChainInTail   = FALSE;
            if (IsBlock256b(pIn->swizzleMode))
   {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeSurfaceInfoMicroTiled
   *
   *   @brief
   *       Internal function to calculate alignment for micro tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::ComputeSurfaceInfoMicroTiled(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR_E_RETURNCODE ret = ComputeBlockDimensionForSurf(&pOut->blockWidth,
                                          if (ret == ADDR_OK)
   {
               pOut->pitch     = PowTwoAlign(pIn->width,  pOut->blockWidth);
   pOut->height    = PowTwoAlign(pIn->height, pOut->blockHeight);
   pOut->numSlices = pIn->numSlices;
            if (pIn->numMipLevels > 1)
   {
         const UINT_32 mip0Width    = pIn->width;
                  for (INT_32 i = static_cast<INT_32>(pIn->numMipLevels) - 1; i >= 0; i--)
                                             if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[i].pitch            = mipActualWidth;
   pOut->pMipInfo[i].height           = mipActualHeight;
   pOut->pMipInfo[i].depth            = 1;
   pOut->pMipInfo[i].offset           = mipSliceSize;
                                 pOut->sliceSize = mipSliceSize;
   }
   else
   {
                        if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].pitch            = pOut->pitch;
   pOut->pMipInfo[0].height           = pOut->height;
   pOut->pMipInfo[0].depth            = 1;
   pOut->pMipInfo[0].offset           = 0;
   pOut->pMipInfo[0].mipTailOffset    = 0;
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeSurfaceInfoMacroTiled
   *
   *   @brief
   *       Internal function to calculate alignment for macro tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::ComputeSurfaceInfoMacroTiled(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR_E_RETURNCODE returnCode = ComputeBlockDimensionForSurf(&pOut->blockWidth,
                                          if (returnCode == ADDR_OK)
   {
               if (pIn->flags.qbStereo)
   {
                           if (returnCode == ADDR_OK)
   {
                  if (returnCode == ADDR_OK)
   {
                        pOut->pitch     = PowTwoAlign(pIn->width,     pOut->blockWidth);
   pOut->height    = PowTwoAlign(pIn->height,    heightAlign);
                  if (pIn->numMipLevels > 1)
   {
      const Dim3d  tailMaxDim         = GetMipTailDim(pIn->resourceType,
                           const UINT_32 mip0Width         = pIn->width;
   const UINT_32 mip0Height        = pIn->height;
   const BOOL_32 isThin            = IsThin(pIn->resourceType, pIn->swizzleMode);
   const UINT_32 mip0Depth         = isThin ? 1 : pIn->numSlices;
   const UINT_32 maxMipsInTail     = GetMaxNumMipsInTail(blockSizeLog2, isThin);
   const UINT_32 index             = Log2(pIn->bpp >> 3);
   UINT_32       firstMipInTail    = pIn->numMipLevels;
                        // For htile, we need to make z16 and stencil enter the mip tail at the same time as z32 would
   Dim3d fixedTailMaxDim = tailMaxDim;
   if (IsZOrderSwizzle(pIn->swizzleMode) && (index <= 1))
   {
                                                         if (IsInMipTail(fixedTailMaxDim, maxMipsInTail, mipWidth, mipHeight, pIn->numMipLevels - i))
   {
         firstMipInTail     = i;
   mipChainSliceSize += blockSize / pOut->blockSlices;
   }
   else
   {
                                                         if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[i].pitch  = pitch;
                        pOut->sliceSize        = mipChainSliceSize;
                        if (pOut->pMipInfo != NULL)
   {
                                                                                    for (INT_32 i = firstMipInTail - 1; i >= 0; i--)
   {
                                                                           for (UINT_32 i = firstMipInTail; i < pIn->numMipLevels; i++)
                                                                        UINT_32 mipX = ((mipOffset >> 9)  & 1)  |
                  ((mipOffset >> 10) & 2)  |
   ((mipOffset >> 11) & 4)  |
      UINT_32 mipY = ((mipOffset >> 8)  & 1)  |
                                    if (blockSizeLog2 & 1)
                                 if (index & 1)
                                 if (isThin)
                                 pitch  = Max(pitch  >> 1, Block256_2d[index].w);
   height = Max(height >> 1, Block256_2d[index].h);
      }
   else
                                 pitch  = Max(pitch  >> 1, Block256_3d[index].w);
   height = Max(height >> 1, Block256_3d[index].h);
            }
   else
   {
                     if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].pitch            = pOut->pitch;
   pOut->pMipInfo[0].height           = pOut->height;
   pOut->pMipInfo[0].depth            = IsTex3d(pIn->resourceType)? pOut->numSlices : 1;
   pOut->pMipInfo[0].offset           = 0;
   pOut->pMipInfo[0].mipTailOffset    = 0;
   pOut->pMipInfo[0].macroBlockOffset = 0;
   pOut->pMipInfo[0].mipTailCoordX    = 0;
   pOut->pMipInfo[0].mipTailCoordY    = 0;
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeSurfaceAddrFromCoordTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeSurfaceAddrFromCoordTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
               if (IsBlock256b(pIn->swizzleMode))
   {
         }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeOffsetFromEquation
   *
   *   @brief
   *       Compute offset from equation
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::ComputeOffsetFromEquation(
      const ADDR_EQUATION* pEq,   ///< Equation
   UINT_32              x,     ///< x coord in bytes
   UINT_32              y,     ///< y coord in pixel
   UINT_32              z      ///< z coord in slice
      {
               for (UINT_32 i = 0; i < pEq->numBits; i++)
   {
               for (UINT_32 c = 0; c < pEq->numBitComponents; c++)
   {
         if (pEq->comps[c][i].valid)
   {
      if (pEq->comps[c][i].channel == 0)
   {
         }
   else if (pEq->comps[c][i].channel == 1)
   {
         }
   else
   {
      ADDR_ASSERT(pEq->comps[c][i].channel == 2);
                                 }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeOffsetFromSwizzlePattern
   *
   *   @brief
   *       Compute offset from swizzle pattern
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::ComputeOffsetFromSwizzlePattern(
      const UINT_64* pPattern,    ///< Swizzle pattern
   UINT_32        numBits,     ///< Number of bits in pattern
   UINT_32        x,           ///< x coord in pixel
   UINT_32        y,           ///< y coord in pixel
   UINT_32        z,           ///< z coord in slice
   UINT_32        s            ///< sample id
      {
      UINT_32                 offset          = 0;
            for (UINT_32 i = 0; i < numBits; i++)
   {
               if (pSwizzlePattern[i].x != 0)
   {
                        while (mask != 0)
   {
      if (mask & 1)
                        xBits >>= 1;
               if (pSwizzlePattern[i].y != 0)
   {
                        while (mask != 0)
   {
      if (mask & 1)
                        yBits >>= 1;
               if (pSwizzlePattern[i].z != 0)
   {
                        while (mask != 0)
   {
      if (mask & 1)
                        zBits >>= 1;
               if (pSwizzlePattern[i].s != 0)
   {
                        while (mask != 0)
   {
      if (mask & 1)
                        sBits >>= 1;
                              }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetSwizzlePatternInfo
   *
   *   @brief
   *       Get swizzle pattern
   *
   *   @return
   *       Swizzle pattern information
   ************************************************************************************************************************
   */
   const ADDR_SW_PATINFO* Gfx11Lib::GetSwizzlePatternInfo(
      AddrSwizzleMode  swizzleMode,       ///< Swizzle mode
   AddrResourceType resourceType,      ///< Resource type
   UINT_32          elemLog2,          ///< Element size in bytes log2
   UINT_32          numFrag            ///< Number of fragment
      {
      const UINT_32          index       = IsXor(swizzleMode) ? (m_colorBaseIndex + elemLog2) : elemLog2;
   const ADDR_SW_PATINFO* patInfo     = NULL;
   const UINT_32          swizzleMask = 1 << swizzleMode;
   const BOOL_32          isBlock256k = IsBlock256kb(swizzleMode);
            if (IsLinear(swizzleMode) == FALSE)
   {
      if (resourceType == ADDR_RSRC_TEX_3D)
   {
                  if ((swizzleMask & Gfx11Rsrc3dSwModeMask) != 0)
   {
      if (IsZOrderSwizzle(swizzleMode) || IsRtOptSwizzle(swizzleMode))
   {
      if (isBlock256k)
   {
         ADDR_ASSERT((swizzleMode == ADDR_SW_256KB_Z_X) || (swizzleMode == ADDR_SW_256KB_R_X));
   }
   else if (isBlock64K)
   {
         ADDR_ASSERT((swizzleMode == ADDR_SW_64KB_Z_X) || (swizzleMode == ADDR_SW_64KB_R_X));
   }
   else
   {
            }
   else if (IsDisplaySwizzle(resourceType, swizzleMode))
   {
      if (isBlock256k)
   {
         ADDR_ASSERT(swizzleMode == ADDR_SW_256KB_D_X);
   }
   else if (isBlock64K)
   {
         ADDR_ASSERT(swizzleMode == ADDR_SW_64KB_D_X);
   }
   else
   {
            }
                           if (isBlock256k)
   {
         ADDR_ASSERT(swizzleMode == ADDR_SW_256KB_S_X);
   }
   else if (isBlock64K)
   {
         if (swizzleMode == ADDR_SW_64KB_S)
   {
         }
   else if (swizzleMode == ADDR_SW_64KB_S_X)
   {
         }
   else if (swizzleMode == ADDR_SW_64KB_S_T)
   {
         }
   else
   {
         }
   else if (IsBlock4kb(swizzleMode))
   {
         if (swizzleMode == ADDR_SW_4KB_S)
   {
         }
   else if (swizzleMode == ADDR_SW_4KB_S_X)
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
         if ((swizzleMask & Gfx11Rsrc2dSwModeMask) != 0)
   {
      if (IsBlock256b(swizzleMode))
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_256B_D);
      }
   else if (IsBlock4kb(swizzleMode))
   {
      if (swizzleMode == ADDR_SW_4KB_D)
   {
         }
   else if (swizzleMode == ADDR_SW_4KB_D_X)
   {
         }
   else
   {
            }
   else if (isBlock64K)
   {
      if (IsZOrderSwizzle(swizzleMode) || IsRtOptSwizzle(swizzleMode))
   {
         if (numFrag == 1)
   {
         }
   else if (numFrag == 2)
   {
         }
   else if (numFrag == 4)
   {
         }
   else if (numFrag == 8)
   {
         }
   else
   {
         }
   else if (IsDisplaySwizzle(resourceType, swizzleMode))
   {
         if (swizzleMode == ADDR_SW_64KB_D)
   {
         }
   else if (swizzleMode == ADDR_SW_64KB_D_X)
   {
         }
   else if (swizzleMode == ADDR_SW_64KB_D_T)
   {
         }
   else
   {
         }
   else
   {
            }
   else if (isBlock256k)
   {
      if (IsZOrderSwizzle(swizzleMode) || IsRtOptSwizzle(swizzleMode))
   {
         if (numFrag == 1)
   {
         }
   else if (numFrag == 2)
   {
         }
   else if (numFrag == 4)
   {
         }
   else if (numFrag == 8)
   {
         }
   else
   {
         }
   else if (IsDisplaySwizzle(resourceType, swizzleMode))
   {
         ADDR_ASSERT(swizzleMode == ADDR_SW_256KB_D_X);
   }
   else
   {
            }
   else
   {
                           }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeSurfaceAddrFromCoordMicroTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for micro tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::ComputeSurfaceAddrFromCoordMicroTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn  = {};
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {};
            localIn.swizzleMode  = pIn->swizzleMode;
   localIn.flags        = pIn->flags;
   localIn.resourceType = pIn->resourceType;
   localIn.bpp          = pIn->bpp;
   localIn.width        = Max(pIn->unalignedWidth,  1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices,       1u);
   localIn.numMipLevels = Max(pIn->numMipLevels,    1u);
   localIn.numSamples   = Max(pIn->numSamples,      1u);
   localIn.numFrags     = localIn.numSamples;
                     if (ret == ADDR_OK)
   {
      const UINT_32 elemLog2 = Log2(pIn->bpp >> 3);
   const UINT_32 rsrcType = static_cast<UINT_32>(pIn->resourceType) - 1;
   const UINT_32 swMode   = static_cast<UINT_32>(pIn->swizzleMode);
            if (eqIndex != ADDR_INVALID_EQUATION_INDEX)
   {
         const UINT_32 pb           = mipInfo[pIn->mipId].pitch / localOut.blockWidth;
   const UINT_32 yb           = pIn->y / localOut.blockHeight;
   const UINT_32 xb           = pIn->x / localOut.blockWidth;
   const UINT_32 blockIndex   = yb * pb + xb;
   const UINT_32 blockSize    = 256;
   const UINT_32 blk256Offset = ComputeOffsetFromEquation(&m_equationTable[eqIndex],
                     pOut->addr = localOut.sliceSize * pIn->slice +
               }
   else
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::ComputeSurfaceAddrFromCoordMacroTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for macro tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::ComputeSurfaceAddrFromCoordMacroTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn  = {};
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {};
            localIn.swizzleMode  = pIn->swizzleMode;
   localIn.flags        = pIn->flags;
   localIn.resourceType = pIn->resourceType;
   localIn.bpp          = pIn->bpp;
   localIn.width        = Max(pIn->unalignedWidth,  1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices,       1u);
   localIn.numMipLevels = Max(pIn->numMipLevels,    1u);
   localIn.numSamples   = Max(pIn->numSamples,      1u);
   localIn.numFrags     = localIn.numSamples;
                     if (ret == ADDR_OK)
   {
      const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);
   const UINT_32 blkMask     = (1 << blkSizeLog2) - 1;
   const UINT_32 pipeMask    = (1 << m_pipesLog2) - 1;
   const UINT_32 bankMask    = ((1 << GetBankXorBits(blkSizeLog2)) - 1) << (m_pipesLog2 + ColumnBits);
   const UINT_32 pipeBankXor = IsXor(pIn->swizzleMode) ?
            if (localIn.numSamples > 1)
   {
         const ADDR_SW_PATINFO* pPatInfo = GetSwizzlePatternInfo(pIn->swizzleMode,
                        if (pPatInfo != NULL)
   {
      const UINT_32 pb     = localOut.pitch / localOut.blockWidth;
                                       const UINT_32 blkOffset =
      ComputeOffsetFromSwizzlePattern(reinterpret_cast<const UINT_64*>(fullSwizzlePattern),
                                 pOut->addr = (localOut.sliceSize * pIn->slice) +
            }
   else
   {
         }
   else
   {
         const UINT_32 rsrcIdx = (pIn->resourceType == ADDR_RSRC_TEX_3D) ? 1 : 0;
                  if (eqIndex != ADDR_INVALID_EQUATION_INDEX)
   {
      const BOOL_32 inTail    = (mipInfo[pIn->mipId].mipTailOffset != 0) ? TRUE : FALSE;
   const BOOL_32 isThin    = IsThin(pIn->resourceType, pIn->swizzleMode);
   const UINT_64 sliceSize = isThin ? localOut.sliceSize : (localOut.sliceSize * localOut.blockSlices);
   const UINT_32 sliceId   = isThin ? pIn->slice : (pIn->slice / localOut.blockSlices);
   const UINT_32 x         = inTail ? (pIn->x     + mipInfo[pIn->mipId].mipTailCoordX) : pIn->x;
   const UINT_32 y         = inTail ? (pIn->y     + mipInfo[pIn->mipId].mipTailCoordY) : pIn->y;
   const UINT_32 z         = inTail ? (pIn->slice + mipInfo[pIn->mipId].mipTailCoordZ) : pIn->slice;
   const UINT_32 pb        = mipInfo[pIn->mipId].pitch / localOut.blockWidth;
   const UINT_32 yb        = pIn->y / localOut.blockHeight;
   const UINT_32 xb        = pIn->x / localOut.blockWidth;
   const UINT_64 blkIdx    = yb * pb + xb;
   const UINT_32 blkOffset = ComputeOffsetFromEquation(&m_equationTable[eqIndex],
                     pOut->addr = sliceSize * sliceId +
                  }
   else
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeMaxBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments
   *   @return
   *       maximum alignments
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::HwlComputeMaxBaseAlignments() const
   {
         }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeMaxMetaBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments for metadata
   *   @return
   *       maximum alignments for metadata
   ************************************************************************************************************************
   */
   UINT_32 Gfx11Lib::HwlComputeMaxMetaBaseAlignments() const
   {
               // Max base alignment for Htile
   const AddrSwizzleMode ValidSwizzleModeForHtile[] =
   {
      ADDR_SW_64KB_Z_X,
                        for (UINT_32 swIdx = 0; swIdx < sizeof(ValidSwizzleModeForHtile) / sizeof(ValidSwizzleModeForHtile[0]); swIdx++)
   {
      for (UINT_32 bppLog2 = 0; bppLog2 < 3; bppLog2++)
   {
         for (UINT_32 numFragLog2 = 0; numFragLog2 < 4; numFragLog2++)
   {
      const UINT_32 metaBlkSizeHtile = GetMetaBlkSize(Gfx11DataDepthStencil,
                                                         // Max base alignment for 2D Dcc
   // swizzle mode support DCC...
   const AddrSwizzleMode ValidSwizzleModeForDcc2D[] =
   {
      ADDR_SW_64KB_R_X,
                        for (UINT_32 swIdx = 0; swIdx < sizeof(ValidSwizzleModeForDcc2D) / sizeof(ValidSwizzleModeForDcc2D[0]); swIdx++)
   {
      for (UINT_32 bppLog2 = 0; bppLog2 < MaxNumOfBpp; bppLog2++)
   {
         for (UINT_32 numFragLog2 = 0; numFragLog2 < 4; numFragLog2++)
   {
      const UINT_32 metaBlkSize2D = GetMetaBlkSize(Gfx11DataColor,
                                                         // Max base alignment for 3D Dcc
   const AddrSwizzleMode ValidSwizzleModeForDcc3D[] =
   {
      ADDR_SW_64KB_S_X,
   ADDR_SW_64KB_D_X,
   ADDR_SW_64KB_R_X,
   ADDR_SW_256KB_S_X,
   ADDR_SW_256KB_D_X,
                        for (UINT_32 swIdx = 0; swIdx < sizeof(ValidSwizzleModeForDcc3D) / sizeof(ValidSwizzleModeForDcc3D[0]); swIdx++)
   {
      for (UINT_32 bppLog2 = 0; bppLog2 < MaxNumOfBpp; bppLog2++)
   {
         const UINT_32 metaBlkSize3D = GetMetaBlkSize(Gfx11DataColor,
                                                         }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetMetaElementSizeLog2
   *
   *   @brief
   *       Gets meta data element size log2
   *   @return
   *       Meta data element size log2
   ************************************************************************************************************************
   */
   INT_32 Gfx11Lib::GetMetaElementSizeLog2(
         {
               if (dataType == Gfx11DataColor)
   {
         }
   else
   {
      ADDR_ASSERT(dataType == Gfx11DataDepthStencil);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::GetMetaCacheSizeLog2
   *
   *   @brief
   *       Gets meta data cache line size log2
   *   @return
   *       Meta data cache line size log2
   ************************************************************************************************************************
   */
   INT_32 Gfx11Lib::GetMetaCacheSizeLog2(
         {
               if (dataType == Gfx11DataColor)
   {
         }
   else
   {
      ADDR_ASSERT(dataType == Gfx11DataDepthStencil);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx11Lib::HwlComputeSurfaceInfoLinear
   *
   *   @brief
   *       Internal function to calculate alignment for linear surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx11Lib::HwlComputeSurfaceInfoLinear(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if (IsTex1d(pIn->resourceType) && (pIn->height > 1))
   {
         }
   else
   {
      const UINT_32 elementBytes = pIn->bpp >> 3;
   const UINT_32 pitchAlign   = (pIn->swizzleMode == ADDR_SW_LINEAR_GENERAL) ? 1 : (256 / elementBytes);
   const UINT_32 mipDepth     = (pIn->resourceType == ADDR_RSRC_TEX_3D) ? pIn->numSlices : 1;
   UINT_32       pitch        = PowTwoAlign(pIn->width, pitchAlign);
   UINT_32       actualHeight = pIn->height;
            if (pIn->numMipLevels > 1)
   {
         for (INT_32 i = static_cast<INT_32>(pIn->numMipLevels) - 1; i >= 0; i--)
                                       if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[i].pitch            = mipActualWidth;
   pOut->pMipInfo[i].height           = mipHeight;
   pOut->pMipInfo[i].depth            = mipDepth;
   pOut->pMipInfo[i].offset           = sliceSize;
                        }
   else
   {
                  if (returnCode == ADDR_OK)
                     if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].pitch            = pitch;
   pOut->pMipInfo[0].height           = actualHeight;
   pOut->pMipInfo[0].depth            = mipDepth;
   pOut->pMipInfo[0].offset           = 0;
   pOut->pMipInfo[0].mipTailOffset    = 0;
                  if (returnCode == ADDR_OK)
   {
         pOut->pitch          = pitch;
   pOut->height         = actualHeight;
   pOut->numSlices      = pIn->numSlices;
   pOut->sliceSize      = sliceSize;
   pOut->surfSize       = sliceSize * pOut->numSlices;
   pOut->baseAlign      = (pIn->swizzleMode == ADDR_SW_LINEAR_GENERAL) ? elementBytes : 256;
   pOut->blockWidth     = pitchAlign;
                  // Following members are useless on GFX11
   pOut->mipChainPitch  = 0;
   pOut->mipChainHeight = 0;
                  // Post calculation validate
                  }
      } // V2
   } // Addr
