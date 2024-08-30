   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ****************************************************************************************************
   * @file  siaddrlib.cpp
   * @brief Contains the implementation for the SiLib class.
   ****************************************************************************************************
   */
      #include "siaddrlib.h"
   #include "si_gb_reg.h"
      #include "amdgpu_asic_addr.h"
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////////////////
   namespace Addr
   {
      /**
   ****************************************************************************************************
   *   SiHwlInit
   *
   *   @brief
   *       Creates an SiLib object.
   *
   *   @return
   *       Returns an SiLib object pointer.
   ****************************************************************************************************
   */
   Lib* SiHwlInit(const Client* pClient)
   {
         }
      namespace V1
   {
      // We don't support MSAA for equation
   const BOOL_32 SiLib::m_EquationSupport[SiLib::TileTableSize][SiLib::MaxNumElementBytes] =
   {
      {TRUE,  TRUE,  TRUE,  FALSE, FALSE},    //  0, non-AA compressed depth or any stencil
   {FALSE, FALSE, FALSE, FALSE, FALSE},    //  1, 2xAA/4xAA compressed depth with or without stencil
   {FALSE, FALSE, FALSE, FALSE, FALSE},    //  2, 8xAA compressed depth with or without stencil
   {FALSE, TRUE,  FALSE, FALSE, FALSE},    //  3, 16 bpp depth PRT (non-MSAA), don't support uncompressed depth
   {TRUE,  TRUE,  TRUE,  FALSE, FALSE},    //  4, 1D depth
   {FALSE, FALSE, FALSE, FALSE, FALSE},    //  5, 16 bpp depth PRT (4xMSAA)
   {FALSE, FALSE, TRUE,  FALSE, FALSE},    //  6, 32 bpp depth PRT (non-MSAA)
   {FALSE, FALSE, FALSE, FALSE, FALSE},    //  7, 32 bpp depth PRT (4xMSAA)
   {TRUE,  TRUE,  TRUE,  TRUE,  TRUE },    //  8, Linear
   {TRUE,  TRUE,  TRUE,  TRUE,  TRUE },    //  9, 1D display
   {TRUE,  FALSE, FALSE, FALSE, FALSE},    // 10, 8 bpp color (displayable)
   {FALSE, TRUE,  FALSE, FALSE, FALSE},    // 11, 16 bpp color (displayable)
   {FALSE, FALSE, TRUE,  TRUE,  FALSE},    // 12, 32/64 bpp color (displayable)
   {TRUE,  TRUE,  TRUE,  TRUE,  TRUE },    // 13, 1D thin
   {TRUE,  FALSE, FALSE, FALSE, FALSE},    // 14, 8 bpp color non-displayable
   {FALSE, TRUE,  FALSE, FALSE, FALSE},    // 15, 16 bpp color non-displayable
   {FALSE, FALSE, TRUE,  FALSE, FALSE},    // 16, 32 bpp color non-displayable
   {FALSE, FALSE, FALSE, TRUE,  TRUE },    // 17, 64/128 bpp color non-displayable
   {TRUE,  TRUE,  TRUE,  TRUE,  TRUE },    // 18, 1D THICK
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 19, 2D XTHICK
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 20, 2D THICK
   {TRUE,  FALSE, FALSE, FALSE, FALSE},    // 21, 8 bpp 2D PRTs (non-MSAA)
   {FALSE, TRUE,  FALSE, FALSE, FALSE},    // 22, 16 bpp 2D PRTs (non-MSAA)
   {FALSE, FALSE, TRUE,  FALSE, FALSE},    // 23, 32 bpp 2D PRTs (non-MSAA)
   {FALSE, FALSE, FALSE, TRUE,  FALSE},    // 24, 64 bpp 2D PRTs (non-MSAA)
   {FALSE, FALSE, FALSE, FALSE, TRUE },    // 25, 128bpp 2D PRTs (non-MSAA)
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 26, none
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 27, none
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 28, none
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 29, none
   {FALSE, FALSE, FALSE, FALSE, FALSE},    // 30, 64bpp 2D PRTs (4xMSAA)
      };
      /**
   ****************************************************************************************************
   *   SiLib::SiLib
   *
   *   @brief
   *       Constructor
   *
   ****************************************************************************************************
   */
   SiLib::SiLib(const Client* pClient)
      :
   EgBasedLib(pClient),
   m_noOfEntries(0),
      {
         }
      /**
   ****************************************************************************************************
   *   SiLib::~SiLib
   *
   *   @brief
   *       Destructor
   ****************************************************************************************************
   */
   SiLib::~SiLib()
   {
   }
      /**
   ****************************************************************************************************
   *   SiLib::HwlGetPipes
   *
   *   @brief
   *       Get number pipes
   *   @return
   *       num pipes
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlGetPipes(
      const ADDR_TILEINFO* pTileInfo    ///< [in] Tile info
      {
               if (pTileInfo)
   {
         }
   else
   {
      ADDR_ASSERT_ALWAYS();
                  }
      /**
   ****************************************************************************************************
   *   SiLib::GetPipePerSurf
   *   @brief
   *       get pipe num base on inputing tileinfo->pipeconfig
   *   @return
   *       pipe number
   ****************************************************************************************************
   */
   UINT_32 SiLib::GetPipePerSurf(
      AddrPipeCfg pipeConfig   ///< [in] pipe config
      {
               switch (pipeConfig)
   {
      case ADDR_PIPECFG_P2:
         numPipes = 2;
   case ADDR_PIPECFG_P4_8x16:
   case ADDR_PIPECFG_P4_16x16:
   case ADDR_PIPECFG_P4_16x32:
   case ADDR_PIPECFG_P4_32x32:
         numPipes = 4;
   case ADDR_PIPECFG_P8_16x16_8x16:
   case ADDR_PIPECFG_P8_16x32_8x16:
   case ADDR_PIPECFG_P8_32x32_8x16:
   case ADDR_PIPECFG_P8_16x32_16x16:
   case ADDR_PIPECFG_P8_32x32_16x16:
   case ADDR_PIPECFG_P8_32x32_16x32:
   case ADDR_PIPECFG_P8_32x64_32x32:
         numPipes = 8;
   case ADDR_PIPECFG_P16_32x32_8x16:
   case ADDR_PIPECFG_P16_32x32_16x16:
         numPipes = 16;
   default:
            }
      }
      /**
   ****************************************************************************************************
   *   SiLib::ComputeBankEquation
   *
   *   @brief
   *       Compute bank equation
   *
   *   @return
   *       If equation can be computed
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE SiLib::ComputeBankEquation(
      UINT_32         log2BytesPP,    ///< [in] log2 of bytes per pixel
   UINT_32         threshX,        ///< [in] threshold for x channel
   UINT_32         threshY,        ///< [in] threshold for y channel
   ADDR_TILEINFO*  pTileInfo,      ///< [in] tile info
   ADDR_EQUATION*  pEquation       ///< [out] bank equation
      {
               UINT_32 pipes = HwlGetPipes(pTileInfo);
   UINT_32 bankXStart = 3 + Log2(pipes) + Log2(pTileInfo->bankWidth);
            ADDR_CHANNEL_SETTING x3 = InitChannel(1, 0, log2BytesPP + bankXStart);
   ADDR_CHANNEL_SETTING x4 = InitChannel(1, 0, log2BytesPP + bankXStart + 1);
   ADDR_CHANNEL_SETTING x5 = InitChannel(1, 0, log2BytesPP + bankXStart + 2);
   ADDR_CHANNEL_SETTING x6 = InitChannel(1, 0, log2BytesPP + bankXStart + 3);
   ADDR_CHANNEL_SETTING y3 = InitChannel(1, 1, bankYStart);
   ADDR_CHANNEL_SETTING y4 = InitChannel(1, 1, bankYStart + 1);
   ADDR_CHANNEL_SETTING y5 = InitChannel(1, 1, bankYStart + 2);
            x3.value = (threshX > bankXStart)     ? x3.value : 0;
   x4.value = (threshX > bankXStart + 1) ? x4.value : 0;
   x5.value = (threshX > bankXStart + 2) ? x5.value : 0;
   x6.value = (threshX > bankXStart + 3) ? x6.value : 0;
   y3.value = (threshY > bankYStart)     ? y3.value : 0;
   y4.value = (threshY > bankYStart + 1) ? y4.value : 0;
   y5.value = (threshY > bankYStart + 2) ? y5.value : 0;
            switch (pTileInfo->banks)
   {
      case 16:
         if (pTileInfo->macroAspectRatio == 1)
   {
      pEquation->addr[0] = y6;
   pEquation->xor1[0] = x3;
   pEquation->addr[1] = y5;
   pEquation->xor1[1] = y6;
   pEquation->xor2[1] = x4;
   pEquation->addr[2] = y4;
   pEquation->xor1[2] = x5;
   pEquation->addr[3] = y3;
      }
   else if (pTileInfo->macroAspectRatio == 2)
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y6;
   pEquation->addr[1] = y5;
   pEquation->xor1[1] = y6;
   pEquation->xor2[1] = x4;
   pEquation->addr[2] = y4;
   pEquation->xor1[2] = x5;
   pEquation->addr[3] = y3;
      }
   else if (pTileInfo->macroAspectRatio == 4)
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y6;
   pEquation->addr[1] = x4;
   pEquation->xor1[1] = y5;
   pEquation->xor2[1] = y6;
   pEquation->addr[2] = y4;
   pEquation->xor1[2] = x5;
   pEquation->addr[3] = y3;
      }
   else if (pTileInfo->macroAspectRatio == 8)
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y6;
   pEquation->addr[1] = x4;
   pEquation->xor1[1] = y5;
   pEquation->xor2[1] = y6;
   pEquation->addr[2] = x5;
   pEquation->xor1[2] = y4;
   pEquation->addr[3] = y3;
      }
   else
   {
         }
   pEquation->numBits = 4;
   case 8:
         if (pTileInfo->macroAspectRatio == 1)
   {
      pEquation->addr[0] = y5;
   pEquation->xor1[0] = x3;
   pEquation->addr[1] = y4;
   pEquation->xor1[1] = y5;
   pEquation->xor2[1] = x4;
   pEquation->addr[2] = y3;
      }
   else if (pTileInfo->macroAspectRatio == 2)
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y5;
   pEquation->addr[1] = y4;
   pEquation->xor1[1] = y5;
   pEquation->xor2[1] = x4;
   pEquation->addr[2] = y3;
      }
   else if (pTileInfo->macroAspectRatio == 4)
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y5;
   pEquation->addr[1] = x4;
   pEquation->xor1[1] = y4;
   pEquation->xor2[1] = y5;
   pEquation->addr[2] = y3;
      }
   else
   {
         }
   pEquation->numBits = 3;
   case 4:
         if (pTileInfo->macroAspectRatio == 1)
   {
      pEquation->addr[0] = y4;
   pEquation->xor1[0] = x3;
   pEquation->addr[1] = y3;
      }
   else if (pTileInfo->macroAspectRatio == 2)
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y4;
   pEquation->addr[1] = y3;
      }
   else
   {
      pEquation->addr[0] = x3;
   pEquation->xor1[0] = y4;
   pEquation->addr[1] = x4;
      }
   pEquation->numBits = 2;
   case 2:
         if (pTileInfo->macroAspectRatio == 1)
   {
      pEquation->addr[0] = y3;
      }
   else
   {
      pEquation->addr[0] = x3;
      }
   pEquation->numBits = 1;
   default:
         pEquation->numBits = 0;
   retCode = ADDR_NOTSUPPORTED;
               for (UINT_32 i = 0; i < pEquation->numBits; i++)
   {
      if (pEquation->addr[i].value == 0)
   {
         if (pEquation->xor1[i].value == 0)
   {
      // 00X -> X00
   pEquation->addr[i].value = pEquation->xor2[i].value;
      }
   else
                     if (pEquation->xor2[i].value != 0)
   {
      // 0XY -> XY0
   pEquation->xor1[i].value = pEquation->xor2[i].value;
      }
   else
   {
      // 0X0 -> X00
         }
   else if (pEquation->xor1[i].value == 0)
   {
         if (pEquation->xor2[i].value != 0)
   {
      // X0Y -> XY0
   pEquation->xor1[i].value = pEquation->xor2[i].value;
         }
            if ((pTileInfo->bankWidth == 1) &&
      ((pTileInfo->pipeConfig == ADDR_PIPECFG_P4_32x32) ||
      {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::ComputePipeEquation
   *
   *   @brief
   *       Compute pipe equation
   *
   *   @return
   *       If equation can be computed
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE SiLib::ComputePipeEquation(
      UINT_32        log2BytesPP, ///< [in] Log2 of bytes per pixel
   UINT_32        threshX,     ///< [in] Threshold for X channel
   UINT_32        threshY,     ///< [in] Threshold for Y channel
   ADDR_TILEINFO* pTileInfo,   ///< [in] Tile info
   ADDR_EQUATION* pEquation    ///< [out] Pipe configure
      {
               ADDR_CHANNEL_SETTING* pAddr = pEquation->addr;
   ADDR_CHANNEL_SETTING* pXor1 = pEquation->xor1;
            ADDR_CHANNEL_SETTING x3 = InitChannel(1, 0, 3 + log2BytesPP);
   ADDR_CHANNEL_SETTING x4 = InitChannel(1, 0, 4 + log2BytesPP);
   ADDR_CHANNEL_SETTING x5 = InitChannel(1, 0, 5 + log2BytesPP);
   ADDR_CHANNEL_SETTING x6 = InitChannel(1, 0, 6 + log2BytesPP);
   ADDR_CHANNEL_SETTING y3 = InitChannel(1, 1, 3);
   ADDR_CHANNEL_SETTING y4 = InitChannel(1, 1, 4);
   ADDR_CHANNEL_SETTING y5 = InitChannel(1, 1, 5);
            x3.value = (threshX > 3) ? x3.value : 0;
   x4.value = (threshX > 4) ? x4.value : 0;
   x5.value = (threshX > 5) ? x5.value : 0;
   x6.value = (threshX > 6) ? x6.value : 0;
   y3.value = (threshY > 3) ? y3.value : 0;
   y4.value = (threshY > 4) ? y4.value : 0;
   y5.value = (threshY > 5) ? y5.value : 0;
            switch (pTileInfo->pipeConfig)
   {
      case ADDR_PIPECFG_P2:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pEquation->numBits = 1;
   case ADDR_PIPECFG_P4_8x16:
         pAddr[0] = x4;
   pXor1[0] = y3;
   pAddr[1] = x3;
   pXor1[1] = y4;
   pEquation->numBits = 2;
   case ADDR_PIPECFG_P4_16x16:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x4;
   pAddr[1] = x4;
   pXor1[1] = y4;
   pEquation->numBits = 2;
   case ADDR_PIPECFG_P4_16x32:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x4;
   pAddr[1] = x4;
   pXor1[1] = y5;
   pEquation->numBits = 2;
   case ADDR_PIPECFG_P4_32x32:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x5;
   pAddr[1] = x5;
   pXor1[1] = y5;
   pEquation->numBits = 2;
   case ADDR_PIPECFG_P8_16x16_8x16:
         pAddr[0] = x4;
   pXor1[0] = y3;
   pXor2[0] = x5;
   pAddr[1] = x3;
   pXor1[1] = y5;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P8_16x32_8x16:
         pAddr[0] = x4;
   pXor1[0] = y3;
   pXor2[0] = x5;
   pAddr[1] = x3;
   pXor1[1] = y4;
   pAddr[2] = x4;
   pXor1[2] = y5;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P8_16x32_16x16:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x4;
   pAddr[1] = x5;
   pXor1[1] = y4;
   pAddr[2] = x4;
   pXor1[2] = y5;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P8_32x32_8x16:
         pAddr[0] = x4;
   pXor1[0] = y3;
   pXor2[0] = x5;
   pAddr[1] = x3;
   pXor1[1] = y4;
   pAddr[2] = x5;
   pXor1[2] = y5;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P8_32x32_16x16:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x4;
   pAddr[1] = x4;
   pXor1[1] = y4;
   pAddr[2] = x5;
   pXor1[2] = y5;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P8_32x32_16x32:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x4;
   pAddr[1] = x4;
   pXor1[1] = y6;
   pAddr[2] = x5;
   pXor1[2] = y5;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P8_32x64_32x32:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x5;
   pAddr[1] = x6;
   pXor1[1] = y5;
   pAddr[2] = x5;
   pXor1[2] = y6;
   pEquation->numBits = 3;
   case ADDR_PIPECFG_P16_32x32_8x16:
         pAddr[0] = x4;
   pXor1[0] = y3;
   pAddr[1] = x3;
   pXor1[1] = y4;
   pAddr[2] = x5;
   pXor1[2] = y6;
   pAddr[3] = x6;
   pXor1[3] = y5;
   pEquation->numBits = 4;
   case ADDR_PIPECFG_P16_32x32_16x16:
         pAddr[0] = x3;
   pXor1[0] = y3;
   pXor2[0] = x4;
   pAddr[1] = x4;
   pXor1[1] = y4;
   pAddr[2] = x5;
   pXor1[2] = y6;
   pAddr[3] = x6;
   pXor1[3] = y5;
   pEquation->numBits = 4;
   default:
         ADDR_UNHANDLED_CASE();
   pEquation->numBits = 0;
               if (m_settings.isVegaM && (pEquation->numBits == 4))
   {
      ADDR_CHANNEL_SETTING addeMsb = pAddr[0];
   ADDR_CHANNEL_SETTING xor1Msb = pXor1[0];
            pAddr[0] = pAddr[1];
   pXor1[0] = pXor1[1];
            pAddr[1] = pAddr[2];
   pXor1[1] = pXor1[2];
            pAddr[2] = pAddr[3];
   pXor1[2] = pXor1[3];
            pAddr[3] = addeMsb;
   pXor1[3] = xor1Msb;
               for (UINT_32 i = 0; i < pEquation->numBits; i++)
   {
      if (pAddr[i].value == 0)
   {
         if (pXor1[i].value == 0)
   {
         }
   else
   {
      pAddr[i].value = pXor1[i].value;
                     }
      /**
   ****************************************************************************************************
   *   SiLib::ComputePipeFromCoord
   *
   *   @brief
   *       Compute pipe number from coordinates
   *   @return
   *       Pipe number
   ****************************************************************************************************
   */
   UINT_32 SiLib::ComputePipeFromCoord(
      UINT_32         x,              ///< [in] x coordinate
   UINT_32         y,              ///< [in] y coordinate
   UINT_32         slice,          ///< [in] slice index
   AddrTileMode    tileMode,       ///< [in] tile mode
   UINT_32         pipeSwizzle,    ///< [in] pipe swizzle
   BOOL_32         ignoreSE,       ///< [in] TRUE if shader engines are ignored
   ADDR_TILEINFO*  pTileInfo       ///< [in] Tile info
      {
      UINT_32 pipe;
   UINT_32 pipeBit0 = 0;
   UINT_32 pipeBit1 = 0;
   UINT_32 pipeBit2 = 0;
   UINT_32 pipeBit3 = 0;
   UINT_32 sliceRotation;
            UINT_32 tx = x / MicroTileWidth;
   UINT_32 ty = y / MicroTileHeight;
   UINT_32 x3 = _BIT(tx,0);
   UINT_32 x4 = _BIT(tx,1);
   UINT_32 x5 = _BIT(tx,2);
   UINT_32 x6 = _BIT(tx,3);
   UINT_32 y3 = _BIT(ty,0);
   UINT_32 y4 = _BIT(ty,1);
   UINT_32 y5 = _BIT(ty,2);
            switch (pTileInfo->pipeConfig)
   {
      case ADDR_PIPECFG_P2:
         pipeBit0 = x3 ^ y3;
   numPipes = 2;
   case ADDR_PIPECFG_P4_8x16:
         pipeBit0 = x4 ^ y3;
   pipeBit1 = x3 ^ y4;
   numPipes = 4;
   case ADDR_PIPECFG_P4_16x16:
         pipeBit0 = x3 ^ y3 ^ x4;
   pipeBit1 = x4 ^ y4;
   numPipes = 4;
   case ADDR_PIPECFG_P4_16x32:
         pipeBit0 = x3 ^ y3 ^ x4;
   pipeBit1 = x4 ^ y5;
   numPipes = 4;
   case ADDR_PIPECFG_P4_32x32:
         pipeBit0 = x3 ^ y3 ^ x5;
   pipeBit1 = x5 ^ y5;
   numPipes = 4;
   case ADDR_PIPECFG_P8_16x16_8x16:
         pipeBit0 = x4 ^ y3 ^ x5;
   pipeBit1 = x3 ^ y5;
   numPipes = 8;
   case ADDR_PIPECFG_P8_16x32_8x16:
         pipeBit0 = x4 ^ y3 ^ x5;
   pipeBit1 = x3 ^ y4;
   pipeBit2 = x4 ^ y5;
   numPipes = 8;
   case ADDR_PIPECFG_P8_16x32_16x16:
         pipeBit0 = x3 ^ y3 ^ x4;
   pipeBit1 = x5 ^ y4;
   pipeBit2 = x4 ^ y5;
   numPipes = 8;
   case ADDR_PIPECFG_P8_32x32_8x16:
         pipeBit0 = x4 ^ y3 ^ x5;
   pipeBit1 = x3 ^ y4;
   pipeBit2 = x5 ^ y5;
   numPipes = 8;
   case ADDR_PIPECFG_P8_32x32_16x16:
         pipeBit0 = x3 ^ y3 ^ x4;
   pipeBit1 = x4 ^ y4;
   pipeBit2 = x5 ^ y5;
   numPipes = 8;
   case ADDR_PIPECFG_P8_32x32_16x32:
         pipeBit0 = x3 ^ y3 ^ x4;
   pipeBit1 = x4 ^ y6;
   pipeBit2 = x5 ^ y5;
   numPipes = 8;
   case ADDR_PIPECFG_P8_32x64_32x32:
         pipeBit0 = x3 ^ y3 ^ x5;
   pipeBit1 = x6 ^ y5;
   pipeBit2 = x5 ^ y6;
   numPipes = 8;
   case ADDR_PIPECFG_P16_32x32_8x16:
         pipeBit0 = x4 ^ y3;
   pipeBit1 = x3 ^ y4;
   pipeBit2 = x5 ^ y6;
   pipeBit3 = x6 ^ y5;
   numPipes = 16;
   case ADDR_PIPECFG_P16_32x32_16x16:
         pipeBit0 = x3 ^ y3 ^ x4;
   pipeBit1 = x4 ^ y4;
   pipeBit2 = x5 ^ y6;
   pipeBit3 = x6 ^ y5;
   numPipes = 16;
   default:
                     if (m_settings.isVegaM && (numPipes == 16))
   {
      UINT_32 pipeMsb = pipeBit0;
   pipeBit0 = pipeBit1;
   pipeBit1 = pipeBit2;
   pipeBit2 = pipeBit3;
                                 //
   // Apply pipe rotation for the slice.
   //
   switch (tileMode)
   {
      case ADDR_TM_3D_TILED_THIN1:    //fall through thin
   case ADDR_TM_3D_TILED_THICK:    //fall through thick
   case ADDR_TM_3D_TILED_XTHICK:
         sliceRotation =
         default:
            }
   pipeSwizzle += sliceRotation;
                        }
      /**
   ****************************************************************************************************
   *   SiLib::ComputeTileCoordFromPipeAndElemIdx
   *
   *   @brief
   *       Compute (x,y) of a tile within a macro tile from address
   *   @return
   *       Pipe number
   ****************************************************************************************************
   */
   VOID SiLib::ComputeTileCoordFromPipeAndElemIdx(
      UINT_32         elemIdx,          ///< [in] per pipe element index within a macro tile
   UINT_32         pipe,             ///< [in] pipe index
   AddrPipeCfg     pipeCfg,          ///< [in] pipe config
   UINT_32         pitchInMacroTile, ///< [in] surface pitch in macro tile
   UINT_32         x,                ///< [in] x coordinate of the (0,0) tile in a macro tile
   UINT_32         y,                ///< [in] y coordinate of the (0,0) tile in a macro tile
   UINT_32*        pX,               ///< [out] x coordinate
   UINT_32*        pY                ///< [out] y coordinate
      {
      UINT_32 pipebit0 = _BIT(pipe,0);
   UINT_32 pipebit1 = _BIT(pipe,1);
   UINT_32 pipebit2 = _BIT(pipe,2);
   UINT_32 pipebit3 = _BIT(pipe,3);
   UINT_32 elemIdx0 = _BIT(elemIdx,0);
   UINT_32 elemIdx1 = _BIT(elemIdx,1);
   UINT_32 elemIdx2 = _BIT(elemIdx,2);
   UINT_32 x3 = 0;
   UINT_32 x4 = 0;
   UINT_32 x5 = 0;
   UINT_32 x6 = 0;
   UINT_32 y3 = 0;
   UINT_32 y4 = 0;
   UINT_32 y5 = 0;
            switch(pipeCfg)
   {
      case ADDR_PIPECFG_P2:
         x4 = elemIdx2;
   y4 = elemIdx1 ^ x4;
   y3 = elemIdx0 ^ x4;
   x3 = pipebit0 ^ y3;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P4_8x16:
         x4 = elemIdx1;
   y4 = elemIdx0 ^ x4;
   x3 = pipebit1 ^ y4;
   y3 = pipebit0 ^ x4;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P4_16x16:
         x4 = elemIdx1;
   y3 = elemIdx0 ^ x4;
   y4 = pipebit1 ^ x4;
   x3 = pipebit0 ^ y3 ^ x4;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P4_16x32:
         x3 = elemIdx0 ^ pipebit0;
   y5 = _BIT(y,5);
   x4 = pipebit1 ^ y5;
   y3 = pipebit0 ^ x3 ^ x4;
   y4 = elemIdx1 ^ x4;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P4_32x32:
         x4 = elemIdx2;
   y3 = elemIdx0 ^ x4;
   y4 = elemIdx1 ^ x4;
   if((pitchInMacroTile % 2) == 0)
   {   //even
      y5 = _BIT(y,5);
   x5 = pipebit1 ^ y5;
   x3 = pipebit0 ^ y3 ^ x5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {   //odd
      x5 = _BIT(x,5);
   x3 = pipebit0 ^ y3 ^ x5;
   *pY = Bits2Number(2, y4, y3);
      }
   case ADDR_PIPECFG_P8_16x16_8x16:
         x4 = elemIdx0;
   y5 = _BIT(y,5);
   x5 = _BIT(x,5);
   x3 = pipebit1 ^ y5;
   y4 = pipebit2 ^ x4;
   y3 = pipebit0 ^ x5 ^ x4;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P8_16x32_8x16:
         x3 = elemIdx0;
   y4 = pipebit1 ^ x3;
   y5 = _BIT(y,5);
   x5 = _BIT(x,5);
   x4 = pipebit2 ^ y5;
   y3 = pipebit0 ^ x4 ^ x5;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P8_32x32_8x16:
         x4 = elemIdx1;
   y4 = elemIdx0 ^ x4;
   x3 = pipebit1 ^ y4;
   if((pitchInMacroTile % 2) == 0)
   {  //even
      y5 = _BIT(y,5);
   x5 = _BIT(x,5);
   x5 = pipebit2 ^ y5;
   y3 = pipebit0 ^ x4 ^ x5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {  //odd
      x5 = _BIT(x,5);
   y3 = pipebit0 ^ x4 ^ x5;
   *pY = Bits2Number(2, y4, y3);
      }
   case ADDR_PIPECFG_P8_16x32_16x16:
         x3 = elemIdx0;
   x5 = _BIT(x,5);
   y5 = _BIT(y,5);
   x4 = pipebit2 ^ y5;
   y4 = pipebit1 ^ x5;
   y3 = pipebit0 ^ x3 ^ x4;
   *pY = Bits2Number(2, y4, y3);
   *pX = Bits2Number(2, x4, x3);
   case ADDR_PIPECFG_P8_32x32_16x16:
         x4 = elemIdx1;
   y3 = elemIdx0 ^ x4;
   x3 = y3^x4^pipebit0;
   y4 = pipebit1 ^ x4;
   if((pitchInMacroTile % 2) == 0)
   {   //even
      y5 = _BIT(y,5);
   x5 = pipebit2 ^ y5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {   //odd
      *pY = Bits2Number(2, y4, y3);
      }
   case ADDR_PIPECFG_P8_32x32_16x32:
         if((pitchInMacroTile % 2) == 0)
   {   //even
      y5 = _BIT(y,5);
   y6 = _BIT(y,6);
   x4 = pipebit1 ^ y6;
   y3 = elemIdx0 ^ x4;
   y4 = elemIdx1 ^ x4;
   x3 = pipebit0 ^ y3 ^ x4;
   x5 = pipebit2 ^ y5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {   //odd
      y6 = _BIT(y,6);
   x4 = pipebit1 ^ y6;
   y3 = elemIdx0 ^ x4;
   y4 = elemIdx1 ^ x4;
   x3 = pipebit0 ^ y3 ^ x4;
   *pY = Bits2Number(2, y4, y3);
      }
   case ADDR_PIPECFG_P8_32x64_32x32:
         x4 = elemIdx2;
   y3 = elemIdx0 ^ x4;
   y4 = elemIdx1 ^ x4;
   if((pitchInMacroTile % 4) == 0)
   {   //multiple of 4
      y5 = _BIT(y,5);
   y6 = _BIT(y,6);
   x5 = pipebit2 ^ y6;
   x6 = pipebit1 ^ y5;
   x3 = pipebit0 ^ y3 ^ x5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {
      y6 = _BIT(y,6);
   x5 = pipebit2 ^ y6;
   x3 = pipebit0 ^ y3 ^ x5;
   *pY = Bits2Number(2, y4, y3);
      }
   case ADDR_PIPECFG_P16_32x32_8x16:
         x4 = elemIdx1;
   y4 = elemIdx0 ^ x4;
   y3 = pipebit0 ^ x4;
   x3 = pipebit1 ^ y4;
   if((pitchInMacroTile % 4) == 0)
   {   //multiple of 4
      y5 = _BIT(y,5);
   y6 = _BIT(y,6);
   x5 = pipebit2 ^ y6;
   x6 = pipebit3 ^ y5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {
      y6 = _BIT(y,6);
   x5 = pipebit2 ^ y6;
   *pY = Bits2Number(2, y4, y3);
      }
   case ADDR_PIPECFG_P16_32x32_16x16:
         x4 = elemIdx1;
   y3 = elemIdx0 ^ x4;
   y4 = pipebit1 ^ x4;
   x3 = pipebit0 ^ y3 ^ x4;
   if((pitchInMacroTile % 4) == 0)
   {   //multiple of 4
      y5 = _BIT(y,5);
   y6 = _BIT(y,6);
   x5 = pipebit2 ^ y6;
   x6 = pipebit3 ^ y5;
   *pY = Bits2Number(2, y4, y3);
      }
   else
   {
      y6 = _BIT(y,6);
   x5 = pipebit2 ^ y6;
   *pY = Bits2Number(2, y4, y3);
      }
   default:
         }
      /**
   ****************************************************************************************************
   *   SiLib::TileCoordToMaskElementIndex
   *
   *   @brief
   *       Compute element index from coordinates in tiles
   *   @return
   *       Element index
   ****************************************************************************************************
   */
   UINT_32 SiLib::TileCoordToMaskElementIndex(
      UINT_32         tx,                 ///< [in] x coord, in Tiles
   UINT_32         ty,                 ///< [in] y coord, in Tiles
   AddrPipeCfg     pipeConfig,         ///< [in] pipe config
   UINT_32*        macroShift,         ///< [out] macro shift
   UINT_32*        elemIdxBits         ///< [out] tile offset bits
      {
      UINT_32 elemIdx = 0;
   UINT_32 elemIdx0, elemIdx1, elemIdx2;
   UINT_32 tx0, tx1;
            tx0 = _BIT(tx,0);
   tx1 = _BIT(tx,1);
   ty0 = _BIT(ty,0);
            switch(pipeConfig)
   {
      case ADDR_PIPECFG_P2:
         *macroShift = 3;
   *elemIdxBits =3;
   elemIdx2 = tx1;
   elemIdx1 = tx1 ^ ty1;
   elemIdx0 = tx1 ^ ty0;
   elemIdx = Bits2Number(3,elemIdx2,elemIdx1,elemIdx0);
   case ADDR_PIPECFG_P4_8x16:
         *macroShift = 2;
   *elemIdxBits =2;
   elemIdx1 = tx1;
   elemIdx0 = tx1 ^ ty1;
   elemIdx = Bits2Number(2,elemIdx1,elemIdx0);
   case ADDR_PIPECFG_P4_16x16:
         *macroShift = 2;
   *elemIdxBits =2;
   elemIdx0 = tx1^ty0;
   elemIdx1 = tx1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P4_16x32:
         *macroShift = 2;
   *elemIdxBits =2;
   elemIdx0 = tx1^ty0;
   elemIdx1 = tx1^ty1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P4_32x32:
         *macroShift = 2;
   *elemIdxBits =3;
   elemIdx0 = tx1^ty0;
   elemIdx1 = tx1^ty1;
   elemIdx2 = tx1;
   elemIdx = Bits2Number(3, elemIdx2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P8_16x16_8x16:
         *macroShift = 1;
   *elemIdxBits =1;
   elemIdx0 = tx1;
   elemIdx = elemIdx0;
   case ADDR_PIPECFG_P8_16x32_8x16:
         *macroShift = 1;
   *elemIdxBits =1;
   elemIdx0 = tx0;
   elemIdx = elemIdx0;
   case ADDR_PIPECFG_P8_32x32_8x16:
         *macroShift = 1;
   *elemIdxBits =2;
   elemIdx1 = tx1;
   elemIdx0 = tx1^ty1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P8_16x32_16x16:
         *macroShift = 1;
   *elemIdxBits =1;
   elemIdx0 = tx0;
   elemIdx = elemIdx0;
   case ADDR_PIPECFG_P8_32x32_16x16:
         *macroShift = 1;
   *elemIdxBits =2;
   elemIdx0 = tx1^ty0;
   elemIdx1 = tx1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P8_32x32_16x32:
         *macroShift = 1;
   *elemIdxBits =2;
   elemIdx0 =  tx1^ty0;
   elemIdx1 = tx1^ty1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P8_32x64_32x32:
         *macroShift = 1;
   *elemIdxBits =3;
   elemIdx0 = tx1^ty0;
   elemIdx1 = tx1^ty1;
   elemIdx2 = tx1;
   elemIdx = Bits2Number(3, elemIdx2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P16_32x32_8x16:
         *macroShift = 0;
   *elemIdxBits =2;
   elemIdx0 = tx1^ty1;
   elemIdx1 = tx1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   case ADDR_PIPECFG_P16_32x32_16x16:
         *macroShift = 0;
   *elemIdxBits =2;
   elemIdx0 = tx1^ty0;
   elemIdx1 = tx1;
   elemIdx = Bits2Number(2, elemIdx1, elemIdx0);
   default:
                        }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeTileDataWidthAndHeightLinear
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
   VOID SiLib::HwlComputeTileDataWidthAndHeightLinear(
      UINT_32*        pMacroWidth,     ///< [out] macro tile width
   UINT_32*        pMacroHeight,    ///< [out] macro tile height
   UINT_32         bpp,             ///< [in] bits per pixel
   ADDR_TILEINFO*  pTileInfo        ///< [in] tile info
      {
      ADDR_ASSERT(pTileInfo != NULL);
   UINT_32 macroWidth;
            /// In linear mode, the htile or cmask buffer must be padded out to 4 tiles
   /// but for P8_32x64_32x32, it must be padded out to 8 tiles
   /// Actually there are more pipe configs which need 8-tile padding but SI family
   /// has a bug which is fixed in CI family
   if ((pTileInfo->pipeConfig == ADDR_PIPECFG_P8_32x64_32x32) ||
      (pTileInfo->pipeConfig == ADDR_PIPECFG_P16_32x32_8x16) ||
      {
      macroWidth  = 8*MicroTileWidth;
      }
   else
   {
      macroWidth  = 4*MicroTileWidth;
               *pMacroWidth    = macroWidth;
      }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeHtileBytes
   *
   *   @brief
   *       Compute htile size in bytes
   *
   *   @return
   *       Htile size in bytes
   ****************************************************************************************************
   */
   UINT_64 SiLib::HwlComputeHtileBytes(
      UINT_32     pitch,          ///< [in] pitch
   UINT_32     height,         ///< [in] height
   UINT_32     bpp,            ///< [in] bits per pixel
   BOOL_32     isLinear,       ///< [in] if it is linear mode
   UINT_32     numSlices,      ///< [in] number of slices
   UINT_64*    pSliceBytes,    ///< [out] bytes per slice
   UINT_32     baseAlign       ///< [in] base alignments
      {
         }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeXmaskAddrFromCoord
   *
   *   @brief
   *       Compute address from coordinates for htile/cmask
   *   @return
   *       Byte address
   ****************************************************************************************************
   */
   UINT_64 SiLib::HwlComputeXmaskAddrFromCoord(
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
      UINT_32 tx = x / MicroTileWidth;
   UINT_32 ty = y / MicroTileHeight;
   UINT_32 newPitch;
   UINT_32 newHeight;
   UINT_64 totalBytes;
   UINT_32 macroWidth;
   UINT_32 macroHeight;
   UINT_64 pSliceBytes;
   UINT_32 pBaseAlign;
   UINT_32 tileNumPerPipe;
            if (factor == 2) //CMASK
   {
                        ComputeCmaskInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   pTileInfo,
   &newPitch,
   &newHeight,
      }
   else //HTile
   {
                        ComputeHtileInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   TRUE,
   TRUE,
   pTileInfo,
   &newPitch,
   &newHeight,
   &totalBytes,
   &macroWidth,
               const UINT_32 pitchInTile = newPitch / MicroTileWidth;
   const UINT_32 heightInTile = newHeight / MicroTileWidth;
   UINT_64 macroOffset; // Per pipe starting offset of the macro tile in which this tile lies.
   UINT_64 microNumber; // Per pipe starting offset of the macro tile in which this tile lies.
   UINT_32 microX;
   UINT_32 microY;
   UINT_64 microOffset;
   UINT_32 microShift;
   UINT_64 totalOffset;
   UINT_32 elemIdxBits;
   UINT_32 elemIdx =
                     if (isLinear)
   {   //linear addressing
      // Linear addressing is extremelly wasting memory if slice > 1, since each pipe has the full
   // slice memory foot print instead of divided by numPipes.
   microX = tx / 4; // Macro Tile is 4x4
   microY = ty / 4 ;
                     // do htile single slice alignment if the flag is true
   if (m_configFlags.useHtileSliceAlign && (factor == 1))  //Htile
   {
         }
      }
   else
   {   //tiled addressing
      const UINT_32 macroWidthInTile = macroWidth / MicroTileWidth; // Now in unit of Tiles
   const UINT_32 macroHeightInTile = macroHeight / MicroTileHeight;
   const UINT_32 pitchInCL = pitchInTile / macroWidthInTile;
            const UINT_32 macroX = x / macroWidth;
   const UINT_32 macroY = y / macroHeight;
            // Per pipe starting offset of the cache line in which this tile lies.
   microX = (x % macroWidth) / MicroTileWidth / 4; // Macro Tile is 4x4
   microY = (y % macroHeight) / MicroTileHeight / 4 ;
                        if(elemIdxBits == microShift)
   {
         }
   else
   {
      microNumber >>= elemIdxBits;
   microNumber <<= elemIdxBits;
               microOffset = elemBits * microNumber;
            UINT_32 pipe = ComputePipeFromCoord(x, y, 0, ADDR_TM_2D_TILED_THIN1, 0, FALSE, pTileInfo);
   UINT_64 addrInBits = totalOffset % (m_pipeInterleaveBytes * 8) +
               *pBitPosition = static_cast<UINT_32>(addrInBits) % 8;
               }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeXmaskCoordFromAddr
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
   VOID SiLib::HwlComputeXmaskCoordFromAddr(
      UINT_64         addr,           ///< [in] address
   UINT_32         bitPosition,    ///< [in] bitPosition in a byte
   UINT_32         pitch,          ///< [in] pitch
   UINT_32         height,         ///< [in] height
   UINT_32         numSlices,      ///< [in] number of slices
   UINT_32         factor,         ///< [in] factor that indicates cmask or htile
   BOOL_32         isLinear,       ///< [in] linear or tiled HTILE layout
   BOOL_32         isWidth8,       ///< [in] Not used by SI
   BOOL_32         isHeight8,      ///< [in] Not used by SI
   ADDR_TILEINFO*  pTileInfo,      ///< [in] Tile info
   UINT_32*        pX,             ///< [out] x coord
   UINT_32*        pY,             ///< [out] y coord
   UINT_32*        pSlice          ///< [out] slice index
      {
      UINT_32 newPitch;
   UINT_32 newHeight;
   UINT_64 totalBytes;
   UINT_32 clWidth;
   UINT_32 clHeight;
   UINT_32 tileNumPerPipe;
            *pX = 0;
   *pY = 0;
            if (factor == 2) //CMASK
   {
                        ComputeCmaskInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   pTileInfo,
   &newPitch,
      }
   else //HTile
   {
                        ComputeHtileInfo(flags,
                     pitch,
   height,
   numSlices,
   isLinear,
   TRUE,
   TRUE,
   pTileInfo,
   &newPitch,
   &newHeight,
               const UINT_32 pitchInTile = newPitch / MicroTileWidth;
   const UINT_32 heightInTile = newHeight / MicroTileWidth;
   const UINT_32 pitchInMacroTile = pitchInTile / 4;
   UINT_32 macroShift;
   UINT_32 elemIdxBits;
   // get macroShift and elemIdxBits
            const UINT_32 numPipes = HwlGetPipes(pTileInfo);
   const UINT_32 pipe = (UINT_32)((addr / m_pipeInterleaveBytes) % numPipes);
   // per pipe
   UINT_64 localOffset = (addr % m_pipeInterleaveBytes) +
            UINT_32 tileIndex;
   if (factor == 2) //CMASK
   {
         }
   else
   {
                  UINT_32 macroOffset;
   if (isLinear)
   {
               // do htile single slice alignment if the flag is true
   if (m_configFlags.useHtileSliceAlign && (factor == 1))  //Htile
   {
         }
   *pSlice = tileIndex / (sliceSizeInTile / numPipes);
      }
   else
   {
      const UINT_32 clWidthInTile = clWidth / MicroTileWidth; // Now in unit of Tiles
   const UINT_32 clHeightInTile = clHeight / MicroTileHeight;
   const UINT_32 pitchInCL = pitchInTile / clWidthInTile;
   const UINT_32 heightInCL = heightInTile / clHeightInTile;
            UINT_32 clX = clIndex % pitchInCL;
            *pX = clX * clWidthInTile * MicroTileWidth;
   *pY = clY * clHeightInTile * MicroTileHeight;
                        UINT_32 elemIdx = macroOffset & 7;
            if (elemIdxBits != macroShift)
   {
               UINT_32 pipebit1 = _BIT(pipe,1);
   UINT_32 pipebit2 = _BIT(pipe,2);
   UINT_32 pipebit3 = _BIT(pipe,3);
   if (pitchInMacroTile % 2)
   {   //odd
         switch (pTileInfo->pipeConfig)
   {
      case ADDR_PIPECFG_P4_32x32:
      macroOffset |= pipebit1;
      case ADDR_PIPECFG_P8_32x32_8x16:
   case ADDR_PIPECFG_P8_32x32_16x16:
   case ADDR_PIPECFG_P8_32x32_16x32:
      macroOffset |= pipebit2;
                           if (pitchInMacroTile % 4)
   {
         if (pTileInfo->pipeConfig == ADDR_PIPECFG_P8_32x64_32x32)
   {
         }
   if ((pTileInfo->pipeConfig == ADDR_PIPECFG_P16_32x32_8x16) ||
      (pTileInfo->pipeConfig == ADDR_PIPECFG_P16_32x32_16x16)
      {
                     UINT_32 macroX;
            if (isLinear)
   {
      macroX = macroOffset % pitchInMacroTile;
      }
   else
   {
      const UINT_32 clWidthInMacroTile = clWidth / (MicroTileWidth * 4);
   macroX = macroOffset % clWidthInMacroTile;
               *pX += macroX * 4 * MicroTileWidth;
            UINT_32 microX;
   UINT_32 microY;
   ComputeTileCoordFromPipeAndElemIdx(elemIdx, pipe, pTileInfo->pipeConfig, pitchInMacroTile,
            *pX += microX * MicroTileWidth;
      }
      /**
   ****************************************************************************************************
   *   SiLib::HwlGetPitchAlignmentLinear
   *   @brief
   *       Get pitch alignment
   *   @return
   *       pitch alignment
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlGetPitchAlignmentLinear(
      UINT_32             bpp,    ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags   ///< [in] surface flags
      {
               // Interleaved access requires a 256B aligned pitch, so fall back to pre-SI alignment
   if (flags.interleaved)
   {
            }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlGetSizeAdjustmentLinear
   *
   *   @brief
   *       Adjust linear surface pitch and slice size
   *
   *   @return
   *       Logical slice size in bytes
   ****************************************************************************************************
   */
   UINT_64 SiLib::HwlGetSizeAdjustmentLinear(
      AddrTileMode        tileMode,       ///< [in] tile mode
   UINT_32             bpp,            ///< [in] bits per pixel
   UINT_32             numSamples,     ///< [in] number of samples
   UINT_32             baseAlign,      ///< [in] base alignment
   UINT_32             pitchAlign,     ///< [in] pitch alignment
   UINT_32*            pPitch,         ///< [in,out] pointer to pitch
   UINT_32*            pHeight,        ///< [in,out] pointer to height
   UINT_32*            pHeightAlign    ///< [in,out] pointer to height align
      {
      UINT_64 sliceSize;
   if (tileMode == ADDR_TM_LINEAR_GENERAL)
   {
         }
   else
   {
      UINT_32 pitch   = *pPitch;
            UINT_32 pixelsPerPipeInterleave = m_pipeInterleaveBytes / BITS_TO_BYTES(bpp);
            // numSamples should be 1 in real cases (no MSAA for linear but TGL may pass non 1 value)
            while (pixelPerSlice % sliceAlignInPixel)
   {
         pitch += pitchAlign;
                              while ((pitch * heightAlign) % sliceAlignInPixel)
   {
                                          }
      /**
   ****************************************************************************************************
   *   SiLib::HwlPreHandleBaseLvl3xPitch
   *
   *   @brief
   *       Pre-handler of 3x pitch (96 bit) adjustment
   *
   *   @return
   *       Expected pitch
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlPreHandleBaseLvl3xPitch(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,        ///< [in] input
   UINT_32                                 expPitch    ///< [in] pitch
      {
               // From SI, if pow2Pad is 1 the pitch is expanded 3x first, then padded to pow2, so nothing to
   // do here
   if (pIn->flags.pow2Pad == FALSE)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlPostHandleBaseLvl3xPitch
   *
   *   @brief
   *       Post-handler of 3x pitch adjustment
   *
   *   @return
   *       Expected pitch
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlPostHandleBaseLvl3xPitch(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,        ///< [in] input
   UINT_32                                 expPitch    ///< [in] pitch
      {
      /**
   * @note The pitch will be divided by 3 in the end so the value will look odd but h/w should
   *  be able to compute a correct pitch from it as h/w address library is doing the job.
   */
   // From SI, the pitch is expanded 3x first, then padded to pow2, so no special handler here
   if (pIn->flags.pow2Pad == FALSE)
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlGetPitchAlignmentMicroTiled
   *
   *   @brief
   *       Compute 1D tiled surface pitch alignment
   *
   *   @return
   *       pitch alignment
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlGetPitchAlignmentMicroTiled(
      AddrTileMode        tileMode,          ///< [in] tile mode
   UINT_32             bpp,               ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS  flags,             ///< [in] surface flags
   UINT_32             numSamples         ///< [in] number of samples
      {
               if (flags.qbStereo)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlGetSizeAdjustmentMicroTiled
   *
   *   @brief
   *       Adjust 1D tiled surface pitch and slice size
   *
   *   @return
   *       Logical slice size in bytes
   ****************************************************************************************************
   */
   UINT_64 SiLib::HwlGetSizeAdjustmentMicroTiled(
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
            // Pitch alignment is always 8, so if slice size is not padded to base alignment
   // (pipe_interleave_size), we need to increase pitch
   while ((physicalSliceSize % baseAlign) != 0)
   {
                                 #if !ALT_TEST
      //
   // Special workaround for depth/stencil buffer, use 8 bpp to align depth buffer again since
   // the stencil plane may have larger pitch if the slice size is smaller than base alignment.
   //
   // Note: this actually does not work for mipmap but mipmap depth texture is not really
   // sampled with mipmap.
   //
   if (flags.depth && (flags.noStencil == FALSE))
   {
                        while ((logicalSiceSizeStencil % baseAlign) != 0)
   {
                           if (pitch != *pPitch)
   {
         // If this is a mipmap, this padded one cannot be sampled as a whole mipmap!
         #endif
                           }
      /**
   ****************************************************************************************************
   *   SiLib::HwlConvertChipFamily
   *
   *   @brief
   *       Convert familyID defined in atiid.h to ChipFamily and set m_chipFamily/m_chipRevision
   *   @return
   *       ChipFamily
   ****************************************************************************************************
   */
   ChipFamily SiLib::HwlConvertChipFamily(
      UINT_32 uChipFamily,        ///< [in] chip family defined in atiih.h
      {
               switch (uChipFamily)
   {
      case FAMILY_SI:
         m_settings.isSouthernIsland = 1;
   m_settings.isTahiti     = ASICREV_IS_TAHITI_P(uChipRevision);
   m_settings.isPitCairn   = ASICREV_IS_PITCAIRN_PM(uChipRevision);
   m_settings.isCapeVerde  = ASICREV_IS_CAPEVERDE_M(uChipRevision);
   m_settings.isOland      = ASICREV_IS_OLAND_M(uChipRevision);
   m_settings.isHainan     = ASICREV_IS_HAINAN_V(uChipRevision);
   default:
                        }
      /**
   ****************************************************************************************************
   *   SiLib::HwlSetupTileInfo
   *
   *   @brief
   *       Setup default value of tile info for SI
   ****************************************************************************************************
   */
   VOID SiLib::HwlSetupTileInfo(
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
            // Fail-safe code
   if (IsLinear(tileMode) == FALSE)
   {
      // 128 bpp/thick tiling must be non-displayable.
   // Fmask reuse color buffer's entry but bank-height field can be from another entry
   // To simplify the logic, fmask entry should be picked from non-displayable ones
   if (bpp == 128 || thickness > 1 || flags.fmask || flags.prt)
   {
                  if (flags.depth || flags.stencil)
   {
                     // Partial valid fields are not allowed for SI.
   if (IsTileInfoAllZero(pTileInfo))
   {
      if (IsMacroTiled(tileMode))
   {
         if (flags.prt)
   {
      if (numSamples == 1)
   {
      if (flags.depth)
   {
         switch (bpp)
   {
      case 16:
      index = 3;
      case 32:
      index = 6;
      default:
      ADDR_ASSERT_ALWAYS();
   }
   else
   {
         switch (bpp)
   {
      case 8:
      index = 21;
      case 16:
      index = 22;
      case 32:
      index = 23;
      case 64:
      index = 24;
      case 128:
                                 if (thickness > 1)
   {
      ADDR_ASSERT(bpp != 128);
         }
                           if (flags.depth)
   {
         switch (bpp)
   {
      case 16:
      index = 5;
      case 32:
      index = 7;
      default:
      ADDR_ASSERT_ALWAYS();
   }
   else
   {
         switch (bpp)
   {
      case 8:
      index = 23;
      case 16:
      index = 24;
      case 32:
      index = 25;
      case 64:
      index = 30;
      default:
      ADDR_ASSERT_ALWAYS();
         }//end of PRT part
   // See table entries 0-7
   else if (flags.depth || flags.stencil)
   {
      if (flags.compressZ)
   {
      if (flags.stencil)
   {
         }
   else
   {
         // optimal tile index for compressed depth/stencil.
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
         }
   else // unCompressZ
   {
            }
   else //non PRT & non Depth & non Stencil
   {
      // See table entries 9-12
   if (inTileType == ADDR_DISPLAYABLE)
   {
      switch (bpp)
   {
         case 8:
      index = 10;
      case 16:
      index = 11;
      case 32:
      index = 12;
      case 64:
      index = 12;
      default:
      }
   else
   {
      // See table entries 13-17
   if (thickness == 1)
   {
                                 switch (fmaskPixelSize)
   {
      case 8:
         index = 14;
   case 16:
         index = 15;
   case 32:
         index = 16;
   case 64:
         index = 17;
   default:
         }
   else
   {
      switch (bpp)
   {
      case 8:
         index = 14;
   case 16:
         index = 15;
   case 32:
         index = 16;
   case 64:
         index = 17;
   case 128:
         index = 17;
   default:
         }
   else // thick tiling - entries 18-20
   {
         switch (thickness)
   {
      case 4:
      index = 20;
      case 8:
      index = 19;
      default:
            }
   else
   {
         if (tileMode == ADDR_TM_LINEAR_ALIGNED)
   {
         }
   else if (tileMode == ADDR_TM_LINEAR_GENERAL)
   {
         }
   else
   {
      if (flags.depth || flags.stencil)
   {
         }
   else if (inTileType == ADDR_DISPLAYABLE)
   {
         }
   else if (thickness == 1)
   {
         }
   else
   {
                     if (index >= 0 && index <= 31)
   {
         *pTileInfo      = m_tileTable[index].info;
            if (index == TileIndexLinearGeneral)
   {
         *pTileInfo      = m_tileTable[8].info;
      }
   else
   {
      if (pTileInfoIn)
   {
         if (flags.stencil && pTileInfoIn->tileSplitBytes == 0)
   {
      // Stencil always uses index 0
      }
   // Pass through tile type
               pOut->tileIndex = index;
      }
      /**
   ****************************************************************************************************
   *   SiLib::DecodeGbRegs
   *
   *   @brief
   *       Decodes GB_ADDR_CONFIG and noOfBanks/noOfRanks
   *
   *   @return
   *       TRUE if all settings are valid
   *
   ****************************************************************************************************
   */
   BOOL_32 SiLib::DecodeGbRegs(
         {
      GB_ADDR_CONFIG  reg;
                     switch (reg.f.pipe_interleave_size)
   {
      case ADDR_CONFIG_PIPE_INTERLEAVE_256B:
         m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_256B;
   case ADDR_CONFIG_PIPE_INTERLEAVE_512B:
         m_pipeInterleaveBytes = ADDR_PIPEINTERLEAVE_512B;
   default:
         valid = FALSE;
               switch (reg.f.row_size)
   {
      case ADDR_CONFIG_1KB_ROW:
         m_rowSize = ADDR_ROWSIZE_1KB;
   case ADDR_CONFIG_2KB_ROW:
         m_rowSize = ADDR_ROWSIZE_2KB;
   case ADDR_CONFIG_4KB_ROW:
         m_rowSize = ADDR_ROWSIZE_4KB;
   default:
         valid = FALSE;
               switch (pRegValue->noOfBanks)
   {
      case 0:
         m_banks = 4;
   case 1:
         m_banks = 8;
   case 2:
         m_banks = 16;
   default:
         valid = FALSE;
               switch (pRegValue->noOfRanks)
   {
      case 0:
         m_ranks = 1;
   case 1:
         m_ranks = 2;
   default:
         valid = FALSE;
                                    }
      /**
   ****************************************************************************************************
   *   SiLib::HwlInitGlobalParams
   *
   *   @brief
   *       Initializes global parameters
   *
   *   @return
   *       TRUE if all settings are valid
   *
   ****************************************************************************************************
   */
   BOOL_32 SiLib::HwlInitGlobalParams(
         {
      BOOL_32 valid = TRUE;
                     if (valid)
   {
      if (m_settings.isTahiti || m_settings.isPitCairn)
   {
         }
   else if (m_settings.isCapeVerde || m_settings.isOland)
   {
         }
   else
   {
         // Hainan is 2-pipe (m_settings.isHainan == 1)
                     if (valid)
   {
                                 }
      /**
   ****************************************************************************************************
   *   SiLib::HwlConvertTileInfoToHW
   *   @brief
   *       Entry of si's ConvertTileInfoToHW
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE SiLib::HwlConvertTileInfoToHW(
      const ADDR_CONVERT_TILEINFOTOHW_INPUT* pIn, ///< [in] input structure
   ADDR_CONVERT_TILEINFOTOHW_OUTPUT* pOut      ///< [out] output structure
      {
                        if (retCode == ADDR_OK)
   {
      if (pIn->reverse == FALSE)
   {
         if (pIn->pTileInfo->pipeConfig == ADDR_PIPECFG_INVALID)
   {
         }
   else
   {
      pOut->pTileInfo->pipeConfig =
      }
   else
   {
         pOut->pTileInfo->pipeConfig =
                  }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeXmaskCoordYFrom8Pipe
   *
   *   @brief
   *       Compute the Y coord which will be added to Xmask Y
   *       coord.
   *   @return
   *       Y coord
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlComputeXmaskCoordYFrom8Pipe(
      UINT_32         pipe,       ///< [in] pipe id
   UINT_32         x           ///< [in] tile coord x, which is original x coord / 8
      {
      // This function should never be called since it is 6xx/8xx specfic.
   // Keep this empty implementation to avoid any mis-use.
               }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeSurfaceCoord2DFromBankPipe
   *
   *   @brief
   *       Compute surface x,y coordinates from bank/pipe info
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID SiLib::HwlComputeSurfaceCoord2DFromBankPipe(
      AddrTileMode        tileMode,   ///< [in] tile mode
   UINT_32*            pX,         ///< [in,out] x coordinate
   UINT_32*            pY,         ///< [in,out] y coordinate
   UINT_32             slice,      ///< [in] slice index
   UINT_32             bank,       ///< [in] bank number
   UINT_32             pipe,       ///< [in] pipe number
   UINT_32             bankSwizzle,///< [in] bank swizzle
   UINT_32             pipeSwizzle,///< [in] pipe swizzle
   UINT_32             tileSlices, ///< [in] slices in a micro tile
   BOOL_32             ignoreSE,   ///< [in] TRUE if shader engines are ignored
   ADDR_TILEINFO*      pTileInfo   ///< [in] bank structure. **All fields to be valid on entry**
      {
      UINT_32 xBit;
   UINT_32 yBit;
   UINT_32 yBit3 = 0;
   UINT_32 yBit4 = 0;
   UINT_32 yBit5 = 0;
            UINT_32 xBit3 = 0;
   UINT_32 xBit4 = 0;
                     CoordFromBankPipe xyBits = {0};
   ComputeSurfaceCoord2DFromBankPipe(tileMode, *pX, *pY, slice, bank, pipe,
               yBit3 = xyBits.yBit3;
   yBit4 = xyBits.yBit4;
   yBit5 = xyBits.yBit5;
            xBit3 = xyBits.xBit3;
   xBit4 = xyBits.xBit4;
                              if ((pTileInfo->pipeConfig == ADDR_PIPECFG_P4_32x32) ||
         {
      ADDR_ASSERT(pTileInfo->bankWidth == 1 && pTileInfo->macroAspectRatio > 1);
                                          yBit = Bits2Number(4, yBit6, yBit5, yBit4, yBit3);
            *pY += yBit * pTileInfo->bankHeight * MicroTileHeight;
            //calculate the bank and pipe bits in x, y
   UINT_32 xTile; //x in micro tile
   UINT_32 x3 = 0;
   UINT_32 x4 = 0;
   UINT_32 x5 = 0;
   UINT_32 x6 = 0;
            UINT_32 pipeBit0 = _BIT(pipe,0);
   UINT_32 pipeBit1 = _BIT(pipe,1);
            UINT_32 y3 = _BIT(y, 3);
   UINT_32 y4 = _BIT(y, 4);
   UINT_32 y5 = _BIT(y, 5);
            // bankbit0 after ^x4^x5
   UINT_32 bankBit00 = _BIT(bank,0);
            switch (pTileInfo->pipeConfig)
   {
      case ADDR_PIPECFG_P2:
         x3 = pipeBit0 ^ y3;
   case ADDR_PIPECFG_P4_8x16:
         x4 = pipeBit0 ^ y3;
   x3 = pipeBit0 ^ y4;
   case ADDR_PIPECFG_P4_16x16:
         x4 = pipeBit1 ^ y4;
   x3 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P4_16x32:
         x4 = pipeBit1 ^ y4;
   x3 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P4_32x32:
         x5 = pipeBit1 ^ y5;
   x3 = pipeBit0 ^ y3 ^ x5;
   bankBit0 = yBitTemp ^ x5;
   x4 = bankBit00 ^ x5 ^ bankBit0;
   *pX += x5 * 4 * 1 * 8; // x5 * num_pipes * bank_width * 8;
   case ADDR_PIPECFG_P8_16x16_8x16:
         x3 = pipeBit1 ^ y5;
   x4 = pipeBit2 ^ y4;
   x5 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P8_16x32_8x16:
         x3 = pipeBit1 ^ y4;
   x4 = pipeBit2 ^ y5;
   x5 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P8_32x32_8x16:
         x3 = pipeBit1 ^ y4;
   x5 = pipeBit2 ^ y5;
   x4 = pipeBit0 ^ y3 ^ x5;
   case ADDR_PIPECFG_P8_16x32_16x16:
         x4 = pipeBit2 ^ y5;
   x5 = pipeBit1 ^ y4;
   x3 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P8_32x32_16x16:
         x5 = pipeBit2 ^ y5;
   x4 = pipeBit1 ^ y4;
   x3 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P8_32x32_16x32:
         x5 = pipeBit2 ^ y5;
   x4 = pipeBit1 ^ y6;
   x3 = pipeBit0 ^ y3 ^ x4;
   case ADDR_PIPECFG_P8_32x64_32x32:
         x6 = pipeBit1 ^ y5;
   x5 = pipeBit2 ^ y6;
   x3 = pipeBit0 ^ y3 ^ x5;
   bankBit0 = yBitTemp ^ x6;
   x4 = bankBit00 ^ x5 ^ bankBit0;
   *pX += x6 * 8 * 1 * 8; // x6 * num_pipes * bank_width * 8;
   default:
                           }
      /**
   ****************************************************************************************************
   *   SiLib::HwlPreAdjustBank
   *
   *   @brief
   *       Adjust bank before calculating address acoording to bank/pipe
   *   @return
   *       Adjusted bank
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlPreAdjustBank(
      UINT_32         tileX,      ///< [in] x coordinate in unit of tile
   UINT_32         bank,       ///< [in] bank
   ADDR_TILEINFO*  pTileInfo   ///< [in] tile info
      {
      if (((pTileInfo->pipeConfig == ADDR_PIPECFG_P4_32x32) ||
         {
      UINT_32 bankBit0 = _BIT(bank, 0);
   UINT_32 x4 = _BIT(tileX, 1);
            bankBit0 = bankBit0 ^ x4 ^ x5;
                           }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeSurfaceInfo
   *
   *   @brief
   *       Entry of si's ComputeSurfaceInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE SiLib::HwlComputeSurfaceInfo(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,    ///< [in] input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pOut    ///< [out] output structure
      {
                                 if (((pIn->flags.needEquation   == TRUE) ||
            (pIn->numSamples <= 1) &&
      {
               if ((pIn->numSlices > 1) &&
         (IsMacroTiled(pOut->tileMode) == TRUE) &&
   ((m_chipFamily == ADDR_CHIP_FAMILY_SI) ||
   {
         }
   else if ((pIn->flags.prt == FALSE) &&
               {
         }
   else
                           if (pOut->equationIndex != ADDR_INVALID_EQUATION_INDEX)
   {
                              }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeMipLevel
   *   @brief
   *       Compute MipLevel info (including level 0)
   *   @return
   *       TRUE if HWL's handled
   ****************************************************************************************************
   */
   BOOL_32 SiLib::HwlComputeMipLevel(
      ADDR_COMPUTE_SURFACE_INFO_INPUT* pIn ///< [in,out] Input structure
      {
      // basePitch is calculated from level 0 so we only check this for mipLevel > 0
   if (pIn->mipLevel > 0)
   {
      // Note: Don't check expand 3x formats(96 bit) as the basePitch is not pow2 even if
   // we explicity set pow2Pad flag. The 3x base pitch is padded to pow2 but after being
   // divided by expandX factor (3) - to program texture pitch, the basePitch is never pow2.
   if (ElemLib::IsExpand3x(pIn->format) == FALSE)
   {
         // Sublevel pitches are generated from base level pitch instead of width on SI
   // If pow2Pad is 0, we don't assert - as this is not really used for a mip chain
   ADDR_ASSERT((pIn->flags.pow2Pad == FALSE) ||
            if (pIn->basePitch != 0)
   {
                                 }
      /**
   ****************************************************************************************************
   *   SiLib::HwlCheckLastMacroTiledLvl
   *
   *   @brief
   *       Sets pOut->last2DLevel to TRUE if it is
   *   @note
   *
   ****************************************************************************************************
   */
   VOID SiLib::HwlCheckLastMacroTiledLvl(
      const ADDR_COMPUTE_SURFACE_INFO_INPUT* pIn, ///< [in] Input structure
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pOut      ///< [in,out] Output structure (used as input, too)
      {
      // pow2Pad covers all mipmap cases
   if (pIn->flags.pow2Pad)
   {
               UINT_32 nextPitch;
   UINT_32 nextHeight;
                     if (pIn->mipLevel == 0 || pIn->basePitch == 0)
   {
         // Base level or fail-safe case (basePitch == 0)
   }
   else
   {
         // Sub levels
            // nextHeight must be shifted from this level's original height rather than a pow2 padded
   // one but this requires original height stored somewhere (pOut->height)
            // next level's height is just current level's >> 1 in pixels
   nextHeight = pOut->height >> 1;
   // Special format such as FMT_1 and FMT_32_32_32 can be linear only so we consider block
   // compressed foramts
   if (ElemLib::IsBlockCompressed(pIn->format))
   {
         }
            // nextSlices may be 0 if this level's is 1
   if (pIn->flags.volume)
   {
         }
   else
   {
                  nextTileMode = ComputeSurfaceMipLevelTileMode(pIn->tileMode,
                                                            }
      /**
   ****************************************************************************************************
   *   SiLib::HwlDegradeThickTileMode
   *
   *   @brief
   *       Degrades valid tile mode for thick modes if needed
   *
   *   @return
   *       Suitable tile mode
   ****************************************************************************************************
   */
   AddrTileMode SiLib::HwlDegradeThickTileMode(
      AddrTileMode        baseTileMode,   ///< base tile mode
   UINT_32             numSlices,      ///< current number of slices
   UINT_32*            pBytesPerTile   ///< [in,out] pointer to bytes per slice
      {
         }
      /**
   ****************************************************************************************************
   *   SiLib::HwlTileInfoEqual
   *
   *   @brief
   *       Return TRUE if all field are equal
   *   @note
   *       Only takes care of current HWL's data
   ****************************************************************************************************
   */
   BOOL_32 SiLib::HwlTileInfoEqual(
      const ADDR_TILEINFO* pLeft, ///<[in] Left compare operand
   const ADDR_TILEINFO* pRight ///<[in] Right compare operand
      {
               if (pLeft->pipeConfig == pRight->pipeConfig)
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::GetTileSettings
   *
   *   @brief
   *       Get tile setting infos by index.
   *   @return
   *       Tile setting info.
   ****************************************************************************************************
   */
   const TileConfig* SiLib::GetTileSetting(
      UINT_32 index          ///< [in] Tile index
      {
      ADDR_ASSERT(index < m_noOfEntries);
      }
      /**
   ****************************************************************************************************
   *   SiLib::HwlPostCheckTileIndex
   *
   *   @brief
   *       Map a tile setting to index if curIndex is invalid, otherwise check if curIndex matches
   *       tile mode/type/info and change the index if needed
   *   @return
   *       Tile index.
   ****************************************************************************************************
   */
   INT_32 SiLib::HwlPostCheckTileIndex(
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
   if ((index == TileIndexInvalid         ||
         (mode != m_tileTable[index].mode)  ||
   {
         for (index = 0; index < static_cast<INT_32>(m_noOfEntries); index++)
   {
      if (macroTiled)
   {
      // macro tile modes need all to match
   if (HwlTileInfoEqual(pInfo, &m_tileTable[index].info) &&
         (mode == m_tileTable[index].mode)                 &&
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
   *   SiLib::HwlSetupTileCfg
   *
   *   @brief
   *       Map tile index to tile setting.
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE SiLib::HwlSetupTileCfg(
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
         if (pMode)
   {
                  if (pType)
   {
                  if (pInfo)
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
                  if (pInfo)
   {
         }
   else
   {
      if (IsMacroTiled(pCfgTable->mode))
   {
                     if (pMode)
   {
                  if (pType)
   {
                        }
      /**
   ****************************************************************************************************
   *   SiLib::ReadGbTileMode
   *
   *   @brief
   *       Convert GB_TILE_MODE HW value to TileConfig.
   *   @return
   *       NA.
   ****************************************************************************************************
   */
   VOID SiLib::ReadGbTileMode(
      UINT_32     regValue,   ///< [in] GB_TILE_MODE register
   TileConfig* pCfg        ///< [out] output structure
      {
      GB_TILE_MODE gbTileMode;
            pCfg->type = static_cast<AddrTileType>(gbTileMode.f.micro_tile_mode);
   pCfg->info.bankHeight = 1 << gbTileMode.f.bank_height;
   pCfg->info.bankWidth = 1 << gbTileMode.f.bank_width;
   pCfg->info.banks = 1 << (gbTileMode.f.num_banks + 1);
   pCfg->info.macroAspectRatio = 1 << gbTileMode.f.macro_tile_aspect;
   pCfg->info.tileSplitBytes = 64 << gbTileMode.f.tile_split;
                              if (regArrayMode == 8) //ARRAY_2D_TILED_XTHICK
   {
         }
   else if (regArrayMode >= 14) //ARRAY_3D_TILED_XTHICK
   {
            }
      /**
   ****************************************************************************************************
   *   SiLib::InitTileSettingTable
   *
   *   @brief
   *       Initialize the ADDR_TILE_CONFIG table.
   *   @return
   *       TRUE if tile table is correctly initialized
   ****************************************************************************************************
   */
   BOOL_32 SiLib::InitTileSettingTable(
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
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlGetTileIndex
   *
   *   @brief
   *       Return the virtual/real index for given mode/type/info
   *   @return
   *       ADDR_OK if successful.
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE SiLib::HwlGetTileIndex(
      const ADDR_GET_TILEINDEX_INPUT* pIn,
      {
                           }
      /**
   ****************************************************************************************************
   *   SiLib::HwlFmaskPreThunkSurfInfo
   *
   *   @brief
   *       Some preparation before thunking a ComputeSurfaceInfo call for Fmask
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   VOID SiLib::HwlFmaskPreThunkSurfInfo(
      const ADDR_COMPUTE_FMASK_INFO_INPUT*    pFmaskIn,   ///< [in] Input of fmask info
   const ADDR_COMPUTE_FMASK_INFO_OUTPUT*   pFmaskOut,  ///< [in] Output of fmask info
   ADDR_COMPUTE_SURFACE_INFO_INPUT*        pSurfIn,    ///< [out] Input of thunked surface info
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT*       pSurfOut    ///< [out] Output of thunked surface info
      {
         }
      /**
   ****************************************************************************************************
   *   SiLib::HwlFmaskPostThunkSurfInfo
   *
   *   @brief
   *       Copy hwl extra field after calling thunked ComputeSurfaceInfo
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   VOID SiLib::HwlFmaskPostThunkSurfInfo(
      const ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pSurfOut,   ///< [in] Output of surface info
   ADDR_COMPUTE_FMASK_INFO_OUTPUT* pFmaskOut           ///< [out] Output of fmask info
      {
      pFmaskOut->macroModeIndex = TileIndexInvalid;
      }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeFmaskBits
   *   @brief
   *       Computes fmask bits
   *   @return
   *       Fmask bits
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlComputeFmaskBits(
      const ADDR_COMPUTE_FMASK_INFO_INPUT* pIn,
   UINT_32* pNumSamples
      {
      UINT_32 numSamples = pIn->numSamples;
   UINT_32 numFrags = GetNumFragments(numSamples, pIn->numFrags);
            if (numFrags != numSamples) // EQAA
   {
               if (pIn->resolved == FALSE)
   {
         if (numFrags == 1)
   {
      bpp          = 1;
      }
   else if (numFrags == 2)
                     bpp          = 2;
      }
   else if (numFrags == 4)
                     bpp          = 4;
      }
   else // numFrags == 8
                     bpp          = 4;
      }
   else
   {
         if (numFrags == 1)
   {
      bpp          = (numSamples == 16) ? 16 : 8;
      }
   else if (numFrags == 2)
                     bpp          = numSamples*2;
      }
   else if (numFrags == 4)
                     bpp          = numSamples*4;
      }
   else // numFrags == 8
                     bpp          = 16*4;
         }
   else // Normal AA
   {
      if (pIn->resolved == FALSE)
   {
         bpp          = ComputeFmaskNumPlanesFromNumSamples(numSamples);
   }
   else
   {
         // The same as 8XX
   bpp          = ComputeFmaskResolvedBppFromNumSamples(numSamples);
                           }
      /**
   ****************************************************************************************************
   *   SiLib::HwlOptimizeTileMode
   *
   *   @brief
   *       Optimize tile mode on SI
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID SiLib::HwlOptimizeTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT*    pInOut      ///< [in,out] input output structure
      {
               if ((pInOut->flags.needEquation == TRUE) &&
      (IsMacroTiled(tileMode) == TRUE) &&
      {
               if (thickness > 1)
   {
         }
   else if (pInOut->numSlices > 1)
   {
         }
   else
   {
                     if (tileMode != pInOut->tileMode)
   {
            }
      /**
   ****************************************************************************************************
   *   SiLib::HwlOverrideTileMode
   *
   *   @brief
   *       Override tile modes (for PRT only, avoid client passes in an invalid PRT mode for SI.
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID SiLib::HwlOverrideTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT*    pInOut          ///< [in,out] input output structure
      {
               switch (tileMode)
   {
      case ADDR_TM_PRT_TILED_THIN1:
                  case ADDR_TM_PRT_TILED_THICK:
                  case ADDR_TM_PRT_2D_TILED_THICK:
                  case ADDR_TM_PRT_3D_TILED_THICK:
                  default:
               if (tileMode != pInOut->tileMode)
   {
      pInOut->tileMode  = tileMode;
   // Only PRT tile modes are overridden for now. Revisit this once new modes are added above.
         }
      /**
   ****************************************************************************************************
   *   SiLib::HwlSetPrtTileMode
   *
   *   @brief
   *       Set prt tile modes.
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID SiLib::HwlSetPrtTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT* pInOut     ///< [in,out] input output structure
      {
      pInOut->tileMode = ADDR_TM_2D_TILED_THIN1;
   pInOut->tileType = (pInOut->tileType == ADDR_DEPTH_SAMPLE_ORDER) ?
            }
      /**
   ****************************************************************************************************
   *   SiLib::HwlSelectTileMode
   *
   *   @brief
   *       Select tile modes.
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID SiLib::HwlSelectTileMode(
      ADDR_COMPUTE_SURFACE_INFO_INPUT* pInOut     ///< [in,out] input output structure
      {
      AddrTileMode tileMode;
            if (pInOut->flags.volume)
   {
      if (pInOut->numSlices >= 8)
   {
         }
   else if (pInOut->numSlices >= 4)
   {
         }
   else
   {
         }
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
      tileMode = ADDR_TM_2D_TILED_THIN1;
               pInOut->tileMode = tileMode;
            // Optimize tile mode if possible
            // Optimize tile mode if possible
               }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeMaxBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments
   *   @return
   *       maximum alignments
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlComputeMaxBaseAlignments() const
   {
               // Initial size is 64 KiB for PRT.
            for (UINT_32 i = 0; i < m_noOfEntries; i++)
   {
      if ((IsMacroTiled(m_tileTable[i].mode) == TRUE) &&
         {
         // The maximum tile size is 16 byte-per-pixel and either 8-sample or 8-slice.
                                 if (baseAlign > maxBaseAlign)
   {
                        }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeMaxMetaBaseAlignments
   *
   *   @brief
   *       Gets maximum alignments for metadata
   *   @return
   *       maximum alignments for metadata
   ****************************************************************************************************
   */
   UINT_32 SiLib::HwlComputeMaxMetaBaseAlignments() const
   {
               for (UINT_32 i = 0; i < m_noOfEntries; i++)
   {
                     }
      /**
   ****************************************************************************************************
   *   SiLib::HwlComputeSurfaceAlignmentsMacroTiled
   *
   *   @brief
   *       Hardware layer function to compute alignment request for macro tile mode
   *
   *   @return
   *       N/A
   *
   ****************************************************************************************************
   */
   VOID SiLib::HwlComputeSurfaceAlignmentsMacroTiled(
      AddrTileMode                      tileMode,           ///< [in] tile mode
   UINT_32                           bpp,                ///< [in] bits per pixel
   ADDR_SURFACE_FLAGS                flags,              ///< [in] surface flags
   UINT_32                           mipLevel,           ///< [in] mip level
   UINT_32                           numSamples,         ///< [in] number of samples
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT* pOut                ///< [in,out] Surface output
      {
      if ((mipLevel == 0) && (flags.prt))
   {
               if (macroTileSize < PrtTileSize)
   {
                           pOut->pitchAlign *= numMacroTiles;
         }
      /**
   ****************************************************************************************************
   *   SiLib::InitEquationTable
   *
   *   @brief
   *       Initialize Equation table.
   *
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID SiLib::InitEquationTable()
   {
      ADDR_EQUATION_KEY equationKeyTable[EquationTableSize];
                                                // Loop all possible bpp
   for (UINT_32 log2ElementBytes = 0; log2ElementBytes < MaxNumElementBytes; log2ElementBytes++)
   {
      // Get bits per pixel
            // Loop all possible tile index
   for (INT_32 tileIndex = 0; tileIndex < static_cast<INT_32>(m_noOfEntries); tileIndex++)
   {
                                    // Compute tile info, hardcode numSamples to 1 because MSAA is not supported
                  // Check if the input is supported
   if (IsEquationSupported(bpp, tileConfig, tileIndex, log2ElementBytes) == TRUE)
                     // Generate swizzle equation key from bpp and tile config
   key.fields.log2ElementBytes = log2ElementBytes;
   key.fields.tileMode         = tileConfig.mode;
   // Treat depth micro tile type and non-display micro tile type as the same key
   // because they have the same equation actually
   key.fields.microTileType    = (tileConfig.type == ADDR_DEPTH_SAMPLE_ORDER) ?
         key.fields.pipeConfig       = tileConfig.info.pipeConfig;
   key.fields.numBanksLog2     = Log2(tileConfig.info.banks);
   key.fields.bankWidth        = tileConfig.info.bankWidth;
   key.fields.bankHeight       = tileConfig.info.bankHeight;
                        // Find in the table if the equation has been built based on the key
   for (UINT_32 i = 0; i < m_numEquations; i++)
   {
      if (key.value == equationKeyTable[i].value)
   {
                           // If found, just fill the index into the lookup table and no need
   // to generate the equation again. Otherwise, generate the equation.
   if (equationIndex == ADDR_INVALID_EQUATION_INDEX)
                                    // Generate the equation
   if (IsMicroTiled(tileConfig.mode))
   {
         retCode = ComputeMicroTileEquation(log2ElementBytes,
               }
   else
   {
         retCode = ComputeMacroTileEquation(log2ElementBytes,
                     }
   // Only fill the equation into the table if the return code is ADDR_OK,
   // otherwise if the return code is not ADDR_OK, it indicates this is not
   // a valid input, we do nothing but just fill invalid equation index
   // into the lookup table.
   if (retCode == ADDR_OK)
                                       if (IsMicroTiled(tileConfig.mode))
   {
      m_blockWidth[equationIndex]  = MicroTileWidth;
                                    m_blockWidth[equationIndex]  =
                                                                                                                                                                                                                                 // Fill the index into the lookup table, if the combination is not supported
   // fill the invalid equation index
            if (m_chipFamily == ADDR_CHIP_FAMILY_SI)
   {
                        for (UINT_32 log2ElemBytes = 0; log2ElemBytes < MaxNumElementBytes; log2ElemBytes++)
   {
                                    retCode = ComputeMacroTileEquation(log2ElemBytes,
                              if (retCode == ADDR_OK)
                                             m_blockWidth[equationIndex]  =
         HwlGetPipes(pTileInfo) * MicroTileWidth * pTileInfo->bankWidth *
                                             }
      /**
   ****************************************************************************************************
   *   SiLib::IsEquationSupported
   *
   *   @brief
   *       Check if it is supported for given bpp and tile config to generate a equation.
   *
   *   @return
   *       TRUE if supported
   ****************************************************************************************************
   */
   BOOL_32 SiLib::IsEquationSupported(
      UINT_32    bpp,             ///< Bits per pixel
   TileConfig tileConfig,      ///< Tile config
   INT_32     tileIndex,       ///< Tile index
   UINT_32    elementBytesLog2 ///< Log2 of element bytes
      {
               // Linear tile mode is not supported in swizzle pattern equation
   if (IsLinear(tileConfig.mode))
   {
         }
   // These tile modes are for Tex2DArray and Tex3D which has depth (num_slice > 1) use,
   // which is not supported in swizzle pattern equation due to slice rotation
   else if ((tileConfig.mode == ADDR_TM_2D_TILED_THICK)  ||
            (tileConfig.mode == ADDR_TM_2D_TILED_XTHICK) ||
   (tileConfig.mode == ADDR_TM_3D_TILED_THIN1)  ||
      {
         }
   // Only 8bpp(stencil), 16bpp and 32bpp is supported for depth
   else if ((tileConfig.type == ADDR_DEPTH_SAMPLE_ORDER) && (bpp > 32))
   {
         }
   // Tile split is not supported in swizzle pattern equation
   else if (IsMacroTiled(tileConfig.mode))
   {
      UINT_32 thickness = Thickness(tileConfig.mode);
   if (((bpp >> 3) * MicroTilePixels * thickness) > tileConfig.info.tileSplitBytes)
   {
                  if ((supported == TRUE) && (m_chipFamily == ADDR_CHIP_FAMILY_SI))
   {
                        }
      } // V1
   } // Addr
