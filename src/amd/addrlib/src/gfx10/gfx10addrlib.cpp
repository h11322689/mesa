   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ************************************************************************************************************************
   * @file  gfx10addrlib.cpp
   * @brief Contain the implementation for the Gfx10Lib class.
   ************************************************************************************************************************
   */
      #include "gfx10addrlib.h"
   #include "addrcommon.h"
   #include "gfx10_gb_reg.h"
      #include "amdgpu_asic_addr.h"
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      namespace Addr
   {
   /**
   ************************************************************************************************************************
   *   Gfx10HwlInit
   *
   *   @brief
   *       Creates an Gfx10Lib object.
   *
   *   @return
   *       Returns an Gfx10Lib object pointer.
   ************************************************************************************************************************
   */
   Addr::Lib* Gfx10HwlInit(const Client* pClient)
   {
         }
      namespace V2
   {
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Static Const Member
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      const SwizzleModeFlags Gfx10Lib::SwizzleModeTable[ADDR_SW_MAX_TYPE] =
   {//Linear 256B  4KB  64KB   Var    Z    Std   Disp  Rot   XOR    T     RtOpt Reserved
      {{1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_LINEAR
   {{0,    1,    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,    0}}, // ADDR_SW_256B_S
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
            {{0,    0,    0,    0,    1,    1,    0,    0,    0,    1,    0,    0,    0}}, // ADDR_SW_VAR_Z_X
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}}, // Reserved
   {{0,    0,    0,    0,    1,    0,    0,    0,    0,    1,    0,    1,    0}}, // ADDR_SW_VAR_R_X
      };
      const Dim3d Gfx10Lib::Block256_3d[] = {{8, 4, 8}, {4, 4, 8}, {4, 4, 4}, {4, 2, 4}, {2, 2, 4}};
      const Dim3d Gfx10Lib::Block64K_Log2_3d[] = {{6, 5, 5}, {5, 5, 5}, {5, 5, 4}, {5, 4, 4}, {4, 4, 4}};
   const Dim3d Gfx10Lib::Block4K_Log2_3d[]  = {{4, 4, 4}, {3, 4, 4}, {3, 4, 3}, {3, 3, 3}, {2, 3, 3}};
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::Gfx10Lib
   *
   *   @brief
   *       Constructor
   *
   ************************************************************************************************************************
   */
   Gfx10Lib::Gfx10Lib(const Client* pClient)
      :
   Lib(pClient),
   m_numPkrLog2(0),
   m_numSaLog2(0),
   m_colorBaseIndex(0),
   m_xmaskBaseIndex(0),
   m_htileBaseIndex(0),
      {
      memset(&m_settings, 0, sizeof(m_settings));
      }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::~Gfx10Lib
   *
   *   @brief
   *       Destructor
   ************************************************************************************************************************
   */
   Gfx10Lib::~Gfx10Lib()
   {
   }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeHtileInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeHtilenfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeHtileInfo(
      const ADDR2_COMPUTE_HTILE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_HTILE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if (((pIn->swizzleMode != ADDR_SW_64KB_Z_X) &&
               {
         }
   else
   {
      Dim3d         metaBlk     = {};
   const UINT_32 metaBlkSize = GetMetaBlkSize(Gfx10DataDepthStencil,
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
   const UINT_32 index = m_xmaskBaseIndex;
            ADDR_C_ASSERT(sizeof(GFX10_HTILE_SW_PATTERN[patIdxTable[index]]) == 72 * 2);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeCmaskInfo
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskInfo
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeCmaskInfo(
      const ADDR2_COMPUTE_CMASK_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_CMASK_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if ((pIn->resourceType != ADDR_RSRC_TEX_2D) ||
      (pIn->cMaskFlags.pipeAligned != TRUE)   ||
   ((pIn->swizzleMode != ADDR_SW_64KB_Z_X) &&
      {
         }
   else
   {
      Dim3d         metaBlk     = {};
   const UINT_32 metaBlkSize = GetMetaBlkSize(Gfx10DataFmask,
                                          pOut->pitch         = PowTwoAlign(pIn->unalignedWidth,  metaBlk.w);
   pOut->height        = PowTwoAlign(pIn->unalignedHeight, metaBlk.h);
   pOut->baseAlign     = metaBlkSize;
   pOut->metaBlkWidth  = metaBlk.w;
            if (pIn->numMipLevels > 1)
   {
                           for (INT_32 i = static_cast<INT_32>(pIn->firstMipIdInTail) - 1; i >= 0; i--)
                                                            if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[i].inMiptail = FALSE;
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
                                 if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].inMiptail = FALSE;
   pOut->pMipInfo[0].offset    = 0;
               pOut->sliceSize  = pOut->metaBlkNumPerSlice * metaBlkSize;
            // Get the CMASK address equation (copied from CmaskAddrFromCoord)
   const UINT_32  fmaskBpp      = GetFmaskBpp(1, 1);
   const UINT_32  fmaskElemLog2 = Log2(fmaskBpp >> 3);
   const UINT_32  index         = m_xmaskBaseIndex + fmaskElemLog2;
   const UINT_8*  patIdxTable   =
                  ADDR_C_ASSERT(sizeof(GFX10_CMASK_SW_PATTERN[patIdxTable[index]]) == 68 * 2);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeDccInfo
   *
   *   @brief
   *       Interface function to compute DCC key info
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeDccInfo(
      const ADDR2_COMPUTE_DCCINFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_DCCINFO_OUTPUT*      pOut    ///< [out] output structure
      {
               if (IsLinear(pIn->swizzleMode) || IsBlock256b(pIn->swizzleMode))
   {
      // Hardware support dcc for 256 swizzle mode, but address lib will not support it because we only
   // select 256 swizzle mode for small surface, and it's not helpful to enable dcc for small surface.
      }
   else if (m_settings.dccUnsup3DSwDis && IsTex3d(pIn->resourceType) && IsDisplaySwizzle(pIn->swizzleMode))
   {
      // DCC is not supported on 3D Display surfaces for GFX10.0 and GFX10.1
      }
   else
   {
               {
                                 pOut->compressBlkWidth  = isThick ? Block256_3d[elemLog2].w : Block256_2d[elemLog2].w;
   pOut->compressBlkHeight = isThick ? Block256_3d[elemLog2].h : Block256_2d[elemLog2].h;
            if (ret == ADDR_OK)
   {
         Dim3d         metaBlk     = {};
   const UINT_32 numFragLog2 = Log2(Max(pIn->numFrags, 1u));
   const UINT_32 metaBlkSize = GetMetaBlkSize(Gfx10DataColor,
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
                  if (m_settings.supportRbPlus)
                                             if (m_numPkrLog2 < 2)
   {
         }
   else
                                       index += (m_numPkrLog2 - 2) * dccPipePerPkr * MaxNumOfBpp +
         }
   else
                     if (pIn->dccKeyFlags.pipeAligned)
   {
         }
   else
   {
                     ADDR_C_ASSERT(sizeof(GFX10_DCC_64K_R_X_SW_PATTERN[patIdxTable[index]]) == 68 * 2);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeCmaskAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeCmaskAddrFromCoord(
      const ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
      {
      // Only support pipe aligned CMask
            ADDR2_COMPUTE_CMASK_INFO_INPUT input = {};
   input.size            = sizeof(input);
   input.cMaskFlags      = pIn->cMaskFlags;
   input.colorFlags      = pIn->colorFlags;
   input.unalignedWidth  = Max(pIn->unalignedWidth,  1u);
   input.unalignedHeight = Max(pIn->unalignedHeight, 1u);
   input.numSlices       = Max(pIn->numSlices,       1u);
   input.swizzleMode     = pIn->swizzleMode;
            ADDR2_COMPUTE_CMASK_INFO_OUTPUT output = {};
                     if (returnCode == ADDR_OK)
   {
      const UINT_32  fmaskBpp      = GetFmaskBpp(pIn->numSamples, pIn->numFrags);
   const UINT_32  fmaskElemLog2 = Log2(fmaskBpp >> 3);
   const UINT_32  pipeMask      = (1 << m_pipesLog2) - 1;
   const UINT_32  index         = m_xmaskBaseIndex + fmaskElemLog2;
   const UINT_8*  patIdxTable   =
                  const UINT_32  blkSizeLog2  = Log2(output.metaBlkWidth) + Log2(output.metaBlkHeight) - 7;
   const UINT_32  blkMask      = (1 << blkSizeLog2) - 1;
   const UINT_32  blkOffset    = ComputeOffsetFromSwizzlePattern(GFX10_CMASK_SW_PATTERN[patIdxTable[index]],
                                 const UINT_32 xb       = pIn->x / output.metaBlkWidth;
   const UINT_32 yb       = pIn->y / output.metaBlkHeight;
   const UINT_32 pb       = output.pitch / output.metaBlkWidth;
   const UINT_32 blkIndex = (yb * pb) + xb;
            pOut->addr = (output.sliceSize * pIn->slice) +
                              }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeHtileAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileAddrFromCoord
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeHtileAddrFromCoord(
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
                  const UINT_32  blkSizeLog2   = Log2(output.metaBlkWidth) + Log2(output.metaBlkHeight) - 4;
   const UINT_32  blkMask       = (1 << blkSizeLog2) - 1;
   const UINT_32  blkOffset     = ComputeOffsetFromSwizzlePattern(GFX10_HTILE_SW_PATTERN[patIdxTable[index]],
                                 const UINT_32 xb       = pIn->x / output.metaBlkWidth;
   const UINT_32 yb       = pIn->y / output.metaBlkHeight;
   const UINT_32 pb       = output.pitch / output.metaBlkWidth;
                  pOut->addr = (static_cast<UINT_64>(output.sliceSize) * pIn->slice) +
                        }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeHtileCoordFromAddr
   *
   *   @brief
   *       Interface function stub of AddrComputeHtileCoordFromAddr
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeHtileCoordFromAddr(
      const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT* pIn,    ///< [in] input structure
      {
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlSupportComputeDccAddrFromCoord
   *
   *   @brief
   *       Check whether HwlComputeDccAddrFromCoord() can be done for the input parameter
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlSupportComputeDccAddrFromCoord(
         {
               if ((pIn->resourceType       != ADDR_RSRC_TEX_2D) ||
      (pIn->swizzleMode        != ADDR_SW_64KB_R_X) ||
   (pIn->dccKeyFlags.linear == TRUE)             ||
   (pIn->numFrags           >  1)                ||
   (pIn->numMipLevels       >  1)                ||
      {
         }
   else if ((pIn->pitch == 0)         ||
            (pIn->metaBlkWidth == 0)  ||
      {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeDccAddrFromCoord
   *
   *   @brief
   *       Interface function stub of AddrComputeDccAddrFromCoord
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx10Lib::HwlComputeDccAddrFromCoord(
      const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT* pIn,  ///< [in] input structure
      {
      const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 numPipeLog2 = m_pipesLog2;
   const UINT_32 pipeMask    = (1 << numPipeLog2) - 1;
   UINT_32       index       = m_dccBaseIndex + elemLog2;
            if (m_settings.supportRbPlus)
   {
               if (pIn->dccKeyFlags.pipeAligned)
   {
                  if (m_numPkrLog2 < 2)
   {
         }
   else
   {
                              index += (m_numPkrLog2 - 2) * dccPipePerPkr * MaxNumOfBpp +
         }
   else
   {
               if (pIn->dccKeyFlags.pipeAligned)
   {
         }
   else
   {
                     const UINT_32  blkSizeLog2 = Log2(pIn->metaBlkWidth) + Log2(pIn->metaBlkHeight) + elemLog2 - 8;
   const UINT_32  blkMask     = (1 << blkSizeLog2) - 1;
   const UINT_32  blkOffset   =
      ComputeOffsetFromSwizzlePattern(GFX10_DCC_64K_R_X_SW_PATTERN[patIdxTable[index]],
                              const UINT_32 xb       = pIn->x / pIn->metaBlkWidth;
   const UINT_32 yb       = pIn->y / pIn->metaBlkHeight;
   const UINT_32 pb       = pIn->pitch / pIn->metaBlkWidth;
   const UINT_32 blkIndex = (yb * pb) + xb;
            pOut->addr = (static_cast<UINT_64>(pIn->dccRamSliceSize) * pIn->slice) +
            }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlInitGlobalParams
   *
   *   @brief
   *       Initializes global parameters
   *
   *   @return
   *       TRUE if all settings are valid
   *
   ************************************************************************************************************************
   */
   BOOL_32 Gfx10Lib::HwlInitGlobalParams(
         {
      BOOL_32              valid = TRUE;
                     // These values are copied from CModel code
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
            switch (gbAddrConfig.bits.MAX_COMPRESSED_FRAGS)
   {
      case ADDR_CONFIG_1_MAX_COMPRESSED_FRAGMENTS:
         m_maxCompFrag     = 1;
   m_maxCompFragLog2 = 0;
   case ADDR_CONFIG_2_MAX_COMPRESSED_FRAGMENTS:
         m_maxCompFrag     = 2;
   m_maxCompFragLog2 = 1;
   case ADDR_CONFIG_4_MAX_COMPRESSED_FRAGMENTS:
         m_maxCompFrag     = 4;
   m_maxCompFragLog2 = 2;
   case ADDR_CONFIG_8_MAX_COMPRESSED_FRAGMENTS:
         m_maxCompFrag     = 8;
   m_maxCompFragLog2 = 3;
   default:
         ADDR_ASSERT_ALWAYS();
               {
      // Skip unaligned case
   m_xmaskBaseIndex += MaxNumOfBppCMask;
            m_xmaskBaseIndex += m_pipesLog2 * MaxNumOfBppCMask;
   m_htileBaseIndex += m_pipesLog2 * MaxNumOfAA;
            if (m_settings.supportRbPlus)
   {
                                                if (m_numPkrLog2 >= 2)
   {
      m_colorBaseIndex += (2 * m_numPkrLog2 - 2) * MaxNumOfBpp;
   m_xmaskBaseIndex += (m_numPkrLog2 - 1) * 3 * MaxNumOfBppCMask;
      }
   else
   {
         const UINT_32 numPipeType = static_cast<UINT_32>(ADDR_CONFIG_64_PIPE) -
                  ADDR_C_ASSERT(sizeof(GFX10_HTILE_PATIDX) / sizeof(GFX10_HTILE_PATIDX[0]) == (numPipeType + 1) * MaxNumOfAA);
   ADDR_C_ASSERT(sizeof(GFX10_CMASK_64K_PATIDX) / sizeof(GFX10_CMASK_64K_PATIDX[0]) ==
               if (m_settings.supportRbPlus)
   {
      // VAR block size = 16K * num_pipes. For 4 pipe configuration, SW_VAR_* mode swizzle patterns are same as the
   // corresponding SW_64KB_* mode
               if (valid)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlConvertChipFamily
   *
   *   @brief
   *       Convert familyID defined in atiid.h to ChipFamily and set m_chipFamily/m_chipRevision
   *   @return
   *       ChipFamily
   ************************************************************************************************************************
   */
   ChipFamily Gfx10Lib::HwlConvertChipFamily(
      UINT_32 chipFamily,        ///< [in] chip family defined in atiih.h
      {
               m_settings.dccUnsup3DSwDis  = 1;
            switch (chipFamily)
   {
      case FAMILY_NV:
         if (ASICREV_IS_NAVI10_P(chipRevision))
   {
                        if (ASICREV_IS_NAVI12_P(chipRevision))
   {
                  if (ASICREV_IS_NAVI14_M(chipRevision))
   {
                  if (ASICREV_IS_NAVI21_M(chipRevision))
   {
                        if (ASICREV_IS_NAVI22_P(chipRevision))
   {
                        if (ASICREV_IS_NAVI23_P(chipRevision))
   {
                        if (ASICREV_IS_NAVI24_P(chipRevision))
   {
      m_settings.supportRbPlus   = 1;
               case FAMILY_VGH:
         if (ASICREV_IS_VANGOGH(chipRevision))
   {
      m_settings.supportRbPlus   = 1;
      }
   else
   {
                  case FAMILY_RMB:
         if (ASICREV_IS_REMBRANDT(chipRevision))
   {
      m_settings.supportRbPlus   = 1;
      }
   else
   {
         }
   case FAMILY_RPL:
         if (ASICREV_IS_RAPHAEL(chipRevision))
   {
      m_settings.supportRbPlus   = 1;
      }
   case FAMILY_MDN:
         if (ASICREV_IS_MENDOCINO(chipRevision))
   {
      m_settings.supportRbPlus   = 1;
      }
   else
   {
         }
   default:
                                 }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetBlk256SizeLog2
   *
   *   @brief
   *       Get block 256 size
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   void Gfx10Lib::GetBlk256SizeLog2(
      AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2,    ///< [in] number of samples
   Dim3d*           pBlock             ///< [out] block size
      {
      if (IsThin(resourceType, swizzleMode))
   {
               if (IsZOrderSwizzle(swizzleMode))
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
   *   Gfx10Lib::GetCompressedBlockSizeLog2
   *
   *   @brief
   *       Get compress block size
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   void Gfx10Lib::GetCompressedBlockSizeLog2(
      Gfx10DataType    dataType,          ///< [in] Data type
   AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2,    ///< [in] number of samples
   Dim3d*           pBlock             ///< [out] block size
      {
      if (dataType == Gfx10DataColor)
   {
         }
   else
   {
      ADDR_ASSERT((dataType == Gfx10DataDepthStencil) || (dataType == Gfx10DataFmask));
   pBlock->w = 3;
   pBlock->h = 3;
         }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetMetaOverlapLog2
   *
   *   @brief
   *       Get meta block overlap
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   INT_32 Gfx10Lib::GetMetaOverlapLog2(
      Gfx10DataType    dataType,          ///< [in] Data type
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
            if ((numPipesLog2 > 1) && m_settings.supportRbPlus)
   {
                  // In 16Bpp 8xaa, we lose 1 overlap bit because the block size reduction eats into a pipe anchor bit (y4)
   if ((elemLog2 == 4) && (numSamplesLog2 == 3))
   {
         }
   overlap = Max(overlap, 0);
      }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::Get3DMetaOverlapLog2
   *
   *   @brief
   *       Get 3d meta block overlap
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   INT_32 Gfx10Lib::Get3DMetaOverlapLog2(
      AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2           ///< [in] element size log2
      {
      Dim3d microBlock;
                     if (m_settings.supportRbPlus)
   {
                  if ((overlap < 0) || (IsStandardSwizzle(resourceType, swizzleMode) == TRUE))
   {
         }
      }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetPipeRotateAmount
   *
   *   @brief
   *       Get pipe rotate amount
   *
   *   @return
   *       Pipe rotate amount
   ************************************************************************************************************************
   */
      INT_32 Gfx10Lib::GetPipeRotateAmount(
      AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode        ///< [in] Swizzle mode
      {
               if (m_settings.supportRbPlus && (m_pipesLog2 >= (m_numSaLog2 + 1)) && (m_pipesLog2 > 1))
   {
      amount = ((m_pipesLog2 == (m_numSaLog2 + 1)) && IsRbAligned(resourceType, swizzleMode)) ?
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetMetaBlkSize
   *
   *   @brief
   *       Get metadata block size
   *
   *   @return
   *       Meta block size
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::GetMetaBlkSize(
      Gfx10DataType    dataType,          ///< [in] Data type
   AddrResourceType resourceType,      ///< [in] Resource type
   AddrSwizzleMode  swizzleMode,       ///< [in] Swizzle mode
   UINT_32          elemLog2,          ///< [in] element size log2
   UINT_32          numSamplesLog2,    ///< [in] number of samples
   BOOL_32          pipeAlign,         ///< [in] pipe align
   Dim3d*           pBlock             ///< [out] block size
      {
               {
      const INT_32 metaElemSizeLog2   = GetMetaElementSizeLog2(dataType);
   const INT_32 metaCacheSizeLog2  = GetMetaCacheSizeLog2(dataType);
   const INT_32 compBlkSizeLog2    = (dataType == Gfx10DataColor) ? 8 : 6 + numSamplesLog2 + elemLog2;
   const INT_32 metaBlkSamplesLog2 = (dataType == Gfx10DataDepthStencil) ?
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
      if (m_settings.supportRbPlus && (m_pipesLog2 == m_numSaLog2 + 1) && (m_pipesLog2 > 1))
                                                         // In 16Bpe 8xaa, we have an extra overlap bit
   if ((pipeRotateLog2 > 0)  &&
         (elemLog2 == 4)       &&
   (numSamplesLog2 == 3) &&
                                       if (m_settings.supportRbPlus    &&
         IsRtOptSwizzle(swizzleMode) &&
   (numPipesLog2 == 6)         &&
   (numSamplesLog2 == 3)       &&
   (m_maxCompFragLog2 == 3)    &&
   {
            }
   else
                        if (dataType == Gfx10DataDepthStencil)
   {
                                                                        const INT_32 metablkBitsLog2 =
         pBlock->w = 1 << ((metablkBitsLog2 >> 1) + (metablkBitsLog2 & 1));
   pBlock->h = 1 << (metablkBitsLog2 >> 1);
   }
   else
   {
                  if (pipeAlign)
   {
      if (m_settings.supportRbPlus         &&
      (m_pipesLog2 == m_numSaLog2 + 1) &&
   (m_pipesLog2 > 1)                &&
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
   *   Gfx10Lib::ConvertSwizzlePatternToEquation
   *
   *   @brief
   *       Convert swizzle pattern to equation.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx10Lib::ConvertSwizzlePatternToEquation(
      UINT_32                elemLog2,  ///< [in] element bytes log2
   AddrResourceType       rsrcType,  ///< [in] resource type
   AddrSwizzleMode        swMode,    ///< [in] swizzle mode
   const ADDR_SW_PATINFO* pPatInfo,  ///< [in] swizzle pattern infor
   ADDR_EQUATION*         pEquation) ///< [out] equation converted from swizzle pattern
      {
      // Get full swizzle pattern and store it as an ADDR_BIT_SETTING list
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
   *   Gfx10Lib::InitEquationTable
   *
   *   @brief
   *       Initialize Equation table.
   *
   *   @return
   *       N/A
   ************************************************************************************************************************
   */
   VOID Gfx10Lib::InitEquationTable()
   {
               // Iterate through resourceTypes, up to MaxRsrcType where a "resourceType" refers to AddrResourceType (1D/2D/3D)
   // resources. This starts with rsrcTypeIdx = 0, however there is an offset added that will start us off at
   // computing 2D resources.
   for (UINT_32 rsrcTypeIdx = 0; rsrcTypeIdx < MaxRsrcType; rsrcTypeIdx++)
   {
      // Add offset. Start iterating from ADDR_RSRC_TEX_2D
            // Iterate through the maximum number of swizzlemodes a type can hold
   for (UINT_32 swModeIdx = 0; swModeIdx < MaxSwModeType; swModeIdx++)
   {
                  // Iterate through the different bits-per-pixel settings (8bpp/16bpp/32bpp/64bpp/128bpp)
   for (UINT_32 elemLog2 = 0; elemLog2 < MaxElementBytesLog2; elemLog2++)
   {
      UINT_32                equationIndex = ADDR_INVALID_EQUATION_INDEX;
                        if (pPatInfo != NULL)
                                          equationIndex = m_numEquations;
   ADDR_ASSERT(equationIndex < EquationTableSize);
   // Updates m_equationTable[m_numEquations] to be the addr equation for this PatInfo
   m_equationTable[equationIndex] = equation;
   // Increment m_numEquations
      }
   // equationIndex, which is used to look up equations in m_equationTable, will be cached for every
   // iteration in this nested for-loop
            }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlGetEquationIndex
   *
   *   @brief
   *       Interface function stub of GetEquationIndex
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::HwlGetEquationIndex(
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
   *   Gfx10Lib::GetValidDisplaySwizzleModes
   *
   *   @brief
   *       Get valid swizzle modes mask for displayable surface
   *
   *   @return
   *       Valid swizzle modes mask for displayable surface
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::GetValidDisplaySwizzleModes(
      UINT_32 bpp
      {
               if (bpp <= 64)
   {
      if (m_settings.isDcn20)
   {
         }
   else
   {
                        }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::IsValidDisplaySwizzleMode
   *
   *   @brief
   *       Check if a swizzle mode is supported by display engine
   *
   *   @return
   *       TRUE is swizzle mode is supported by display engine
   ************************************************************************************************************************
   */
   BOOL_32 Gfx10Lib::IsValidDisplaySwizzleMode(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn     ///< [in] input structure
      {
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetMaxNumMipsInTail
   *
   *   @brief
   *       Return max number of mips in tails
   *
   *   @return
   *       Max number of mips in tails
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::GetMaxNumMipsInTail(
      UINT_32 blockSizeLog2,     ///< block size log2
   BOOL_32 isThin             ///< is thin or thick
      {
               if (isThin == FALSE)
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputePipeBankXor
   *
   *   @brief
   *       Generate a PipeBankXor value to be ORed into bits above pipeInterleaveBits of address
   *
   *   @return
   *       PipeBankXor value
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputePipeBankXor(
      const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,     ///< [in] input structure
   ADDR2_COMPUTE_PIPEBANKXOR_OUTPUT*      pOut     ///< [out] output structure
      {
      if (IsNonPrtXor(pIn->swizzleMode))
   {
               // No pipe xor...
   const UINT_32 pipeXor = 0;
            const UINT_32         XorPatternLen = 8;
   static const UINT_32  XorBankRot1b[XorPatternLen] = {0,  1,  0,  1,  0,  1,  0,  1};
   static const UINT_32  XorBankRot2b[XorPatternLen] = {0,  2,  1,  3,  2,  0,  3,  1};
   static const UINT_32  XorBankRot3b[XorPatternLen] = {0,  4,  2,  6,  1,  5,  3,  7};
   static const UINT_32  XorBankRot4b[XorPatternLen] = {0,  8,  4, 12,  2, 10,  6, 14};
            switch (bankBits)
   {
         case 1:
   case 2:
   case 3:
   case 4:
      bankXor = XorBankRotPat[bankBits - 1][pIn->surfIndex % XorPatternLen] << (m_pipesLog2 + ColumnBits);
      default:
      // valid bank bits should be 0~4
      case 0:
               }
   else
   {
                     }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeSlicePipeBankXor
   *
   *   @brief
   *       Generate slice PipeBankXor value based on base PipeBankXor value and slice id
   *
   *   @return
   *       PipeBankXor value
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeSlicePipeBankXor(
      const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,   ///< [in] input structure
   ADDR2_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT*      pOut   ///< [out] output structure
      {
      if (IsNonPrtXor(pIn->swizzleMode))
   {
      const UINT_32 blockBits = GetBlockSizeLog2(pIn->swizzleMode);
   const UINT_32 pipeBits  = GetPipeXorBits(blockBits);
                     if (pIn->bpe != 0)
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
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeSubResourceOffsetForSwizzlePattern
   *
   *   @brief
   *       Compute sub resource offset to support swizzle pattern
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeSubResourceOffsetForSwizzlePattern(
      const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT*      pOut    ///< [out] output structure
      {
                           }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeNonBlockCompressedView
   *
   *   @brief
   *       Compute non-block-compressed view for a given mipmap level/slice.
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeNonBlockCompressedView(
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
            ADDR2_MIP_INFO mipInfo[MaxMipLevels] = {};
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
   *   Gfx10Lib::ValidateNonSwModeParams
   *
   *   @brief
   *       Validate compute surface info params except swizzle mode
   *
   *   @return
   *       TRUE if parameters are valid, FALSE otherwise
   ************************************************************************************************************************
   */
   BOOL_32 Gfx10Lib::ValidateNonSwModeParams(
         {
               if ((pIn->bpp == 0) || (pIn->bpp > 128) || (pIn->width == 0) || (pIn->numFrags > 8) || (pIn->numSamples > 16))
   {
      ADDR_ASSERT_ALWAYS();
               if (pIn->resourceType >= ADDR_RSRC_MAX_TYPE)
   {
      ADDR_ASSERT_ALWAYS();
               const ADDR2_SURFACE_FLAGS flags    = pIn->flags;
   const AddrResourceType    rsrcType = pIn->resourceType;
   const BOOL_32             mipmap   = (pIn->numMipLevels > 1);
   const BOOL_32             msaa     = (pIn->numFrags > 1);
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
   *   Gfx10Lib::ValidateSwModeParams
   *
   *   @brief
   *       Validate compute surface info related to swizzle mode
   *
   *   @return
   *       TRUE if parameters are valid, FALSE otherwise
   ************************************************************************************************************************
   */
   BOOL_32 Gfx10Lib::ValidateSwModeParams(
         {
               if (pIn->swizzleMode >= ADDR_SW_MAX_TYPE)
   {
      ADDR_ASSERT_ALWAYS();
      }
   else if (IsValidSwMode(pIn->swizzleMode) == FALSE)
   {
      {
         ADDR_ASSERT_ALWAYS();
               const ADDR2_SURFACE_FLAGS flags       = pIn->flags;
   const AddrResourceType    rsrcType    = pIn->resourceType;
   const AddrSwizzleMode     swizzle     = pIn->swizzleMode;
   const BOOL_32             msaa        = (pIn->numFrags > 1);
   const BOOL_32             zbuffer     = flags.depth || flags.stencil;
   const BOOL_32             color       = flags.color;
   const BOOL_32             display     = flags.display;
   const BOOL_32             tex3d       = IsTex3d(rsrcType);
   const BOOL_32             tex2d       = IsTex2d(rsrcType);
   const BOOL_32             tex1d       = IsTex1d(rsrcType);
   const BOOL_32             thin3d      = flags.view3dAs2dArray;
   const BOOL_32             linear      = IsLinear(swizzle);
   const BOOL_32             blk256B     = IsBlock256b(swizzle);
   const BOOL_32             blkVar      = IsBlockVariable(swizzle);
   const BOOL_32             isNonPrtXor = IsNonPrtXor(swizzle);
   const BOOL_32             prt         = flags.prt;
            // Misc check
   if ((pIn->numFrags > 1) &&
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
      if ((swizzleMask & Gfx10Rsrc1dSwModeMask) == 0)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (tex2d)
   {
      if ((swizzleMask & Gfx10Rsrc2dSwModeMask) == 0)
   {
         {
      ADDR_ASSERT_ALWAYS();
      }
   else if ((prt && ((swizzleMask & Gfx10Rsrc2dPrtSwModeMask) == 0)) ||
         {
         ADDR_ASSERT_ALWAYS();
      }
   else if (tex3d)
   {
      if (((swizzleMask & Gfx10Rsrc3dSwModeMask) == 0) ||
         (prt && ((swizzleMask & Gfx10Rsrc3dPrtSwModeMask) == 0)) ||
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
      {
         ADDR_ASSERT_ALWAYS();
               // Block type check
   if (blk256B)
   {
      if (zbuffer || tex3d || msaa)
   {
         ADDR_ASSERT_ALWAYS();
      }
   else if (blkVar)
   {
      if (m_blockVarSizeLog2 == 0)
   {
         ADDR_ASSERT_ALWAYS();
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeSurfaceInfoSanityCheck
   *
   *   @brief
   *       Compute surface info sanity check
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeSurfaceInfoSanityCheck(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn     ///< [in] input structure
      {
         }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlGetPreferredSurfaceSetting
   *
   *   @brief
   *       Internal function to get suggested surface information for client to use
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlGetPreferredSurfaceSetting(
      const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,  ///< [in] input structure
   ADDR2_GET_PREFERRED_SURF_SETTING_OUTPUT*      pOut  ///< [out] output structure
      {
               if (pIn->flags.fmask)
   {
      const BOOL_32 forbid64KbBlockType = pIn->forbiddenBlock.macroThin64KB ? TRUE : FALSE;
            if (forbid64KbBlockType && forbidVarBlockType)
   {
         // Invalid combination...
   ADDR_ASSERT_ALWAYS();
   }
   else
   {
         pOut->resourceType                   = ADDR_RSRC_TEX_2D;
   pOut->validBlockSet.value            = 0;
   pOut->validBlockSet.macroThin64KB    = forbid64KbBlockType ? 0 : 1;
   pOut->validBlockSet.var              = forbidVarBlockType  ? 0 : 1;
   pOut->validSwModeSet.value           = 0;
   pOut->validSwModeSet.sw64KB_Z_X      = forbid64KbBlockType ? 0 : 1;
   pOut->validSwModeSet.gfx10.swVar_Z_X = forbidVarBlockType  ? 0 : 1;
   pOut->canXor                         = TRUE;
                           if ((forbid64KbBlockType == FALSE) && (forbidVarBlockType == FALSE))
   {
      const UINT_8  maxFmaskSwizzleModeType = 2;
   const UINT_32 ratioLow                = pIn->flags.minimizeAlign ? 1 : (pIn->flags.opt4space ? 3 : 2);
   const UINT_32 ratioHi                 = pIn->flags.minimizeAlign ? 1 : (pIn->flags.opt4space ? 2 : 1);
   const UINT_32 fmaskBpp                = GetFmaskBpp(pIn->numSamples, pIn->numFrags);
   const UINT_32 numSlices               = Max(pIn->numSlices, 1u);
                        AddrSwizzleMode swMode[maxFmaskSwizzleModeType]  = {ADDR_SW_64KB_Z_X, ADDR_SW_VAR_Z_X};
                        for (UINT_8 i = 0; i < maxFmaskSwizzleModeType; i++)
   {
      ComputeBlockDimensionForSurf(&blkDim[i].w,
                                                            if (Addr2BlockTypeWithinMemoryBudget(padSize[0],
                                 {
            }
   else if (forbidVarBlockType)
   {
                  if (use64KbBlockType)
   {
         }
   else
   {
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
   const UINT_32 numFrags     = (pIn->numFrags == 0) ? numSamples : pIn->numFrags;
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
   allowedSwModeSet.value |= pIn->forbiddenBlock.linear ? 0 : Gfx10LinearSwModeMask;
   allowedSwModeSet.value |= pIn->forbiddenBlock.micro  ? 0 : Gfx10Blk256BSwModeMask;
   allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThin4KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThick4KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThin64KB ? 0 :
      allowedSwModeSet.value |=
      pIn->forbiddenBlock.macroThick64KB ? 0 :
                     if (pIn->preferredSwSet.value != 0)
   {
      allowedSwModeSet.value &= pIn->preferredSwSet.sw_Z ? ~0 : ~Gfx10ZSwModeMask;
   allowedSwModeSet.value &= pIn->preferredSwSet.sw_S ? ~0 : ~Gfx10StandardSwModeMask;
                     if (pIn->noXor)
   {
                  if (pIn->maxAlign > 0)
   {
      if (pIn->maxAlign < (1u << m_blockVarSizeLog2))
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
                     pOut->resourceType   = pIn->resourceType;
   pOut->validSwModeSet = allowedSwModeSet;
                                 if (pOut->clientPreferredSwSet.value == 0)
                        // Apply optional restrictions
   if ((pIn->flags.depth || pIn->flags.stencil) && msaa && m_configFlags.nonPower2MemConfig)
   {
      if ((allowedSwModeSet.value &= ~Gfx10BlkVarSwModeMask) != 0)
   {
         // MSAA depth in non power of 2 memory configs would suffer from non-local channel accesses from
   // the GL2 in VAR mode, so it should be avoided.
   }
   else
   {
         // We should still be able to use VAR for non power of 2 memory configs with MSAA z/stencil.
                     if (pIn->flags.needEquation)
   {
      UINT_32 components = pIn->flags.allowExtEquation ?  ADDR_MAX_EQUATION_COMP :
                     if (allowedSwModeSet.value == Gfx10LinearSwModeMask)
   {
         }
                           if ((height > 1) && (computeMinSize == FALSE))
   {
         // Always ignore linear swizzle mode if:
                                       // Determine block size if there are 2 or more block type candidates
   if (IsPow2(allowedBlockSet.value) == FALSE)
                                                                  if (pOut->resourceType == ADDR_RSRC_TEX_3D)
   {
      swMode[AddrBlockThick4KB]  = ADDR_SW_4KB_S;
   swMode[AddrBlockThin64KB]  = ADDR_SW_64KB_R_X;
      }
   else
   {
                                             const UINT_32 ratioLow           = computeMinSize ? 1 : (pIn->flags.opt4space ? 3 : 2);
                                       // Iterate through all block types
   for (UINT_32 i = AddrBlockLinear; i < AddrBlockMaxTiledType; i++)
                                    if (localIn.swizzleMode == ADDR_SW_LINEAR)
   {
                                                                     if (minSize == 0)
   {
      minSize    = padSize[i];
      }
   else
   {
      // Checks if the block type is within the memory budget but favors larger blocks
   if (Addr2BlockTypeWithinMemoryBudget(
            minSize,
   padSize[i],
   ratioLow,
   ratioHi,
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
         case AddrBlockThinVar:
   case AddrBlockThin64KB:
         case AddrBlockThick4KB:
         case AddrBlockThin4KB:
                                                            for (UINT_32 i = AddrBlockMicro; i < AddrBlockMaxTiledType; i++)
   {
      if ((i != minSizeBlk) &&
         {
         if (Addr2BlockTypeWithinMemoryBudget(
         minSize,
   padSize[i],
   0,
   0,
   pIn->memoryBudget,
                                 // Remove VAR block type if bigger block type is allowed
   if (GetBlockSizeLog2(swMode[AddrBlockThinVar]) < GetBlockSizeLog2(ADDR_SW_64KB_R_X))
   {
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
      if (pIn->flags.color &&
      GetAllowedBlockSet(allowedSwModeSet, pOut->resourceType).macroThick64KB &&
      {
         }
   else if (allowedSwSet.sw_S)
   {
         }
   else if (allowedSwSet.sw_R)
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
   else if (allowedSwSet.sw_S)
   {
         }
   else
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
   *   Gfx10Lib::ComputeStereoInfo
   *
   *   @brief
   *       Compute height alignment and right eye pipeBankXor for stereo surface
   *
   *   @return
   *       Error code
   *
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::ComputeStereoInfo(
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
   *   Gfx10Lib::HwlComputeSurfaceInfoTiled
   *
   *   @brief
   *       Internal function to calculate alignment for tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeSurfaceInfoTiled(
      const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
      {
               // Mip chain dimesion and epitch has no meaning in GFX10, set to default value
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
   *   Gfx10Lib::ComputeSurfaceInfoMicroTiled
   *
   *   @brief
   *       Internal function to calculate alignment for micro tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::ComputeSurfaceInfoMicroTiled(
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
   *   Gfx10Lib::ComputeSurfaceInfoMacroTiled
   *
   *   @brief
   *       Internal function to calculate alignment for macro tiled surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::ComputeSurfaceInfoMacroTiled(
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
                                       if (m_settings.dsMipmapHtileFix && IsZOrderSwizzle(pIn->swizzleMode) && (index <= 1))
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
      }
   else
                                 pitch  = Max(pitch  >> 1, Block256_3d[index].w);
            }
   else
   {
                     if (pOut->pMipInfo != NULL)
   {
      pOut->pMipInfo[0].pitch            = pOut->pitch;
   pOut->pMipInfo[0].height           = pOut->height;
   pOut->pMipInfo[0].depth            = IsTex3d(pIn->resourceType) ? pOut->numSlices : 1;
   pOut->pMipInfo[0].offset           = 0;
   pOut->pMipInfo[0].mipTailOffset    = 0;
   pOut->pMipInfo[0].macroBlockOffset = 0;
   pOut->pMipInfo[0].mipTailCoordX    = 0;
   pOut->pMipInfo[0].mipTailCoordY    = 0;
                        }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeSurfaceAddrFromCoordTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeSurfaceAddrFromCoordTiled(
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
   *   Gfx10Lib::ComputeOffsetFromEquation
   *
   *   @brief
   *       Compute offset from equation
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::ComputeOffsetFromEquation(
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
   *   Gfx10Lib::ComputeOffsetFromSwizzlePattern
   *
   *   @brief
   *       Compute offset from swizzle pattern
   *
   *   @return
   *       Offset
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::ComputeOffsetFromSwizzlePattern(
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
   *   Gfx10Lib::GetSwizzlePatternInfo
   *
   *   @brief
   *       Get swizzle pattern
   *
   *   @return
   *       Swizzle pattern information
   ************************************************************************************************************************
   */
   const ADDR_SW_PATINFO* Gfx10Lib::GetSwizzlePatternInfo(
      AddrSwizzleMode  swizzleMode,       ///< Swizzle mode
   AddrResourceType resourceType,      ///< Resource type
   UINT_32          elemLog2,          ///< Element size in bytes log2
   UINT_32          numFrag            ///< Number of fragment
      {
      // Now elemLog2 is going to be used to access the correct index insode of the pPatInfo array so we will start from
   // the right location
   const UINT_32          index       = IsXor(swizzleMode) ? (m_colorBaseIndex + elemLog2) : elemLog2;
   const ADDR_SW_PATINFO* patInfo     = NULL;
            if (IsBlockVariable(swizzleMode))
   {
      if (m_blockVarSizeLog2 != 0)
   {
                  if (IsRtOptSwizzle(swizzleMode))
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
   else
   {
      ADDR_ASSERT(numFrag == 8);
         }
   else if (IsZOrderSwizzle(swizzleMode))
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
   else
   {
      ADDR_ASSERT(numFrag == 8);
            }
   else if (IsLinear(swizzleMode) == FALSE)
   {
      if (resourceType == ADDR_RSRC_TEX_3D)
   {
                  if ((swizzleMask & Gfx10Rsrc3dSwModeMask) != 0)
   {
      if (IsRtOptSwizzle(swizzleMode))
   {
      if (swizzleMode == ADDR_SW_4KB_R_X)
   {
         }
   else
   {
         patInfo = m_settings.supportRbPlus ?
      }
   else if (IsZOrderSwizzle(swizzleMode))
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (IsDisplaySwizzle(resourceType, swizzleMode))
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_64KB_D_X);
   patInfo = m_settings.supportRbPlus ?
      }
                           if (IsBlock4kb(swizzleMode))
   {
         if (swizzleMode == ADDR_SW_4KB_S)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_4KB_S_X);
   patInfo = m_settings.supportRbPlus ?
      }
   else
   {
         if (swizzleMode == ADDR_SW_64KB_S)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (swizzleMode == ADDR_SW_64KB_S_X)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_64KB_S_T);
   patInfo = m_settings.supportRbPlus ?
            }
   else
   {
         if ((swizzleMask & Gfx10Rsrc2dSwModeMask) != 0)
   {
      if (IsBlock256b(swizzleMode))
   {
      if (swizzleMode == ADDR_SW_256B_S)
   {
         patInfo = m_settings.supportRbPlus ?
   }
   else
   {
         ADDR_ASSERT(swizzleMode == ADDR_SW_256B_D);
   patInfo = m_settings.supportRbPlus ?
      }
   else if (IsBlock4kb(swizzleMode))
   {
      if (IsStandardSwizzle(resourceType, swizzleMode))
   {
         if (swizzleMode == ADDR_SW_4KB_S)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_4KB_S_X);
   patInfo = m_settings.supportRbPlus ?
      }
   else
   {
         if (swizzleMode == ADDR_SW_4KB_D)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (swizzleMode == ADDR_SW_4KB_R_X)
   {
         }
   else
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_4KB_D_X);
   patInfo = m_settings.supportRbPlus ?
         }
   else
   {
      if (IsRtOptSwizzle(swizzleMode))
   {
         if (numFrag == 1)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (numFrag == 2)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (numFrag == 4)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(numFrag == 8);
   patInfo = m_settings.supportRbPlus ?
      }
   else if (IsZOrderSwizzle(swizzleMode))
   {
         if (numFrag == 1)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (numFrag == 2)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (numFrag == 4)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(numFrag == 8);
   patInfo = m_settings.supportRbPlus ?
      }
   else if (IsDisplaySwizzle(resourceType, swizzleMode))
   {
         if (swizzleMode == ADDR_SW_64KB_D)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (swizzleMode == ADDR_SW_64KB_D_X)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_64KB_D_T);
   patInfo = m_settings.supportRbPlus ?
      }
   else
   {
         if (swizzleMode == ADDR_SW_64KB_S)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else if (swizzleMode == ADDR_SW_64KB_S_X)
   {
      patInfo = m_settings.supportRbPlus ?
      }
   else
   {
      ADDR_ASSERT(swizzleMode == ADDR_SW_64KB_S_T);
   patInfo = m_settings.supportRbPlus ?
                           }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::ComputeSurfaceAddrFromCoordMicroTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for micro tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::ComputeSurfaceAddrFromCoordMicroTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn  = {};
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {};
   ADDR2_MIP_INFO                    mipInfo[MaxMipLevels];
            localIn.swizzleMode  = pIn->swizzleMode;
   localIn.flags        = pIn->flags;
   localIn.resourceType = pIn->resourceType;
   localIn.bpp          = pIn->bpp;
   localIn.width        = Max(pIn->unalignedWidth,  1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices,       1u);
   localIn.numMipLevels = Max(pIn->numMipLevels,    1u);
   localIn.numSamples   = Max(pIn->numSamples,      1u);
   localIn.numFrags     = Max(pIn->numFrags,        1u);
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
   *   Gfx10Lib::ComputeSurfaceAddrFromCoordMacroTiled
   *
   *   @brief
   *       Internal function to calculate address from coord for macro tiled swizzle surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::ComputeSurfaceAddrFromCoordMacroTiled(
      const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
   ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
      {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT  localIn  = {};
   ADDR2_COMPUTE_SURFACE_INFO_OUTPUT localOut = {};
   ADDR2_MIP_INFO                    mipInfo[MaxMipLevels];
            localIn.swizzleMode  = pIn->swizzleMode;
   localIn.flags        = pIn->flags;
   localIn.resourceType = pIn->resourceType;
   localIn.bpp          = pIn->bpp;
   localIn.width        = Max(pIn->unalignedWidth,  1u);
   localIn.height       = Max(pIn->unalignedHeight, 1u);
   localIn.numSlices    = Max(pIn->numSlices,       1u);
   localIn.numMipLevels = Max(pIn->numMipLevels,    1u);
   localIn.numSamples   = Max(pIn->numSamples,      1u);
   localIn.numFrags     = Max(pIn->numFrags,        1u);
                     if (ret == ADDR_OK)
   {
      const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
   const UINT_32 blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);
   const UINT_32 blkMask     = (1 << blkSizeLog2) - 1;
   const UINT_32 pipeMask    = (1 << m_pipesLog2) - 1;
   const UINT_32 bankMask    = ((1 << GetBankXorBits(blkSizeLog2)) - 1) << (m_pipesLog2 + ColumnBits);
   const UINT_32 pipeBankXor = IsXor(pIn->swizzleMode) ?
            if (localIn.numFrags > 1)
   {
         const ADDR_SW_PATINFO* pPatInfo = GetSwizzlePatternInfo(pIn->swizzleMode,
                        if (pPatInfo != NULL)
   {
      const UINT_32 pb        = localOut.pitch / localOut.blockWidth;
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
   *   Gfx10Lib::HwlComputeMaxBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments
   *   @return
   *       maximum alignments
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::HwlComputeMaxBaseAlignments() const
   {
         }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeMaxMetaBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments for metadata
   *   @return
   *       maximum alignments for metadata
   ************************************************************************************************************************
   */
   UINT_32 Gfx10Lib::HwlComputeMaxMetaBaseAlignments() const
   {
               const AddrSwizzleMode ValidSwizzleModeForXmask[] =
   {
      ADDR_SW_64KB_Z_X,
               UINT_32 maxBaseAlignHtile = 0;
            for (UINT_32 swIdx = 0; swIdx < sizeof(ValidSwizzleModeForXmask) / sizeof(ValidSwizzleModeForXmask[0]); swIdx++)
   {
      for (UINT_32 bppLog2 = 0; bppLog2 < 3; bppLog2++)
   {
         for (UINT_32 numFragLog2 = 0; numFragLog2 < 4; numFragLog2++)
   {
      // Max base alignment for Htile
   const UINT_32 metaBlkSizeHtile = GetMetaBlkSize(Gfx10DataDepthStencil,
                                                      // Max base alignment for Cmask
   const UINT_32 metaBlkSizeCmask = GetMetaBlkSize(Gfx10DataFmask,
                                                      // Max base alignment for 2D Dcc
   const AddrSwizzleMode ValidSwizzleModeForDcc2D[] =
   {
      ADDR_SW_64KB_S_X,
   ADDR_SW_64KB_D_X,
   ADDR_SW_64KB_R_X,
                        for (UINT_32 swIdx = 0; swIdx < sizeof(ValidSwizzleModeForDcc2D) / sizeof(ValidSwizzleModeForDcc2D[0]); swIdx++)
   {
      for (UINT_32 bppLog2 = 0; bppLog2 < MaxNumOfBpp; bppLog2++)
   {
         for (UINT_32 numFragLog2 = 0; numFragLog2 < 4; numFragLog2++)
   {
      const UINT_32 metaBlkSize2D = GetMetaBlkSize(Gfx10DataColor,
                                                         // Max base alignment for 3D Dcc
   const AddrSwizzleMode ValidSwizzleModeForDcc3D[] =
   {
      ADDR_SW_64KB_Z_X,
   ADDR_SW_64KB_S_X,
   ADDR_SW_64KB_D_X,
   ADDR_SW_64KB_R_X,
                        for (UINT_32 swIdx = 0; swIdx < sizeof(ValidSwizzleModeForDcc3D) / sizeof(ValidSwizzleModeForDcc3D[0]); swIdx++)
   {
      for (UINT_32 bppLog2 = 0; bppLog2 < MaxNumOfBpp; bppLog2++)
   {
         const UINT_32 metaBlkSize3D = GetMetaBlkSize(Gfx10DataColor,
                                                         }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetMetaElementSizeLog2
   *
   *   @brief
   *       Gets meta data element size log2
   *   @return
   *       Meta data element size log2
   ************************************************************************************************************************
   */
   INT_32 Gfx10Lib::GetMetaElementSizeLog2(
         {
               if (dataType == Gfx10DataColor)
   {
         }
   else if (dataType == Gfx10DataDepthStencil)
   {
         }
   else
   {
      ADDR_ASSERT(dataType == Gfx10DataFmask);
                  }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::GetMetaCacheSizeLog2
   *
   *   @brief
   *       Gets meta data cache line size log2
   *   @return
   *       Meta data cache line size log2
   ************************************************************************************************************************
   */
   INT_32 Gfx10Lib::GetMetaCacheSizeLog2(
         {
               if (dataType == Gfx10DataColor)
   {
         }
   else if (dataType == Gfx10DataDepthStencil)
   {
         }
   else
   {
      ADDR_ASSERT(dataType == Gfx10DataFmask);
      }
      }
      /**
   ************************************************************************************************************************
   *   Gfx10Lib::HwlComputeSurfaceInfoLinear
   *
   *   @brief
   *       Internal function to calculate alignment for linear surface
   *
   *   @return
   *       ADDR_E_RETURNCODE
   ************************************************************************************************************************
   */
   ADDR_E_RETURNCODE Gfx10Lib::HwlComputeSurfaceInfoLinear(
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
                  // Following members are useless on GFX10
   pOut->mipChainPitch  = 0;
   pOut->mipChainHeight = 0;
                  // Post calculation validate
                  }
      } // V2
   } // Addr
