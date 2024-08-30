   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
      /**
   ****************************************************************************************************
   * @file  addrinterface.cpp
   * @brief Contains the addrlib interface functions
   ****************************************************************************************************
   */
   #include "addrinterface.h"
   #include "addrlib1.h"
   #include "addrlib2.h"
      #include "addrcommon.h"
      using namespace Addr;
      ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                               Create/Destroy/Config functions
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrCreate
   *
   *   @brief
   *       Create address lib object
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrCreate(
      const ADDR_CREATE_INPUT*    pAddrCreateIn,  ///< [in] infomation for creating address lib object
      {
      ADDR_E_RETURNCODE returnCode = ADDR_OK;
   {
                     }
            /**
   ****************************************************************************************************
   *   AddrDestroy
   *
   *   @brief
   *       Destroy address lib object
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrDestroy(
         {
               if (hLib)
   {
      Lib* pLib = Lib::GetLib(hLib);
      }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                    Surface functions
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrComputeSurfaceInfo
   *
   *   @brief
   *       Calculate surface width/height/depth/alignments and suitable tiling mode
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeSurfaceInfo(
      ADDR_HANDLE                             hLib, ///< address lib handle
   const ADDR_COMPUTE_SURFACE_INFO_INPUT*  pIn,  ///< [in] surface information
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            /**
   ****************************************************************************************************
   *   AddrComputeSurfaceAddrFromCoord
   *
   *   @brief
   *       Compute surface address according to coordinates
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeSurfaceAddrFromCoord(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,  ///< [in] surface info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeSurfaceCoordFromAddr
   *
   *   @brief
   *       Compute coordinates according to surface address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeSurfaceCoordFromAddr(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,  ///< [in] surface info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                   HTile functions
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrComputeHtileInfo
   *
   *   @brief
   *       Compute Htile pitch, height, base alignment and size in bytes
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeHtileInfo(
      ADDR_HANDLE                             hLib, ///< address lib handle
   const ADDR_COMPUTE_HTILE_INFO_INPUT*    pIn,  ///< [in] Htile information
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeHtileAddrFromCoord
   *
   *   @brief
   *       Compute Htile address according to coordinates (of depth buffer)
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeHtileAddrFromCoord(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*   pIn,  ///< [in] Htile info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeHtileCoordFromAddr
   *
   *   @brief
   *       Compute coordinates within depth buffer (1st pixel of a micro tile) according to
   *       Htile address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeHtileCoordFromAddr(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_HTILE_COORDFROMADDR_INPUT*   pIn,  ///< [in] Htile info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                     C-mask functions
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrComputeCmaskInfo
   *
   *   @brief
   *       Compute Cmask pitch, height, base alignment and size in bytes from color buffer
   *       info
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeCmaskInfo(
      ADDR_HANDLE                             hLib, ///< address lib handle
   const ADDR_COMPUTE_CMASK_INFO_INPUT*    pIn,  ///< [in] Cmask pitch and height
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Compute Cmask address according to coordinates (of MSAA color buffer)
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeCmaskAddrFromCoord(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*   pIn,  ///< [in] Cmask info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeCmaskCoordFromAddr
   *
   *   @brief
   *       Compute coordinates within color buffer (1st pixel of a micro tile) according to
   *       Cmask address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeCmaskCoordFromAddr(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_CMASK_COORDFROMADDR_INPUT*   pIn,  ///< [in] Cmask info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                     F-mask functions
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrComputeFmaskInfo
   *
   *   @brief
   *       Compute Fmask pitch/height/depth/alignments and size in bytes
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeFmaskInfo(
      ADDR_HANDLE                             hLib, ///< address lib handle
   const ADDR_COMPUTE_FMASK_INFO_INPUT*    pIn,  ///< [in] Fmask information
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeFmaskAddrFromCoord
   *
   *   @brief
   *       Compute Fmask address according to coordinates (x,y,slice,sample,plane)
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeFmaskAddrFromCoord(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_FMASK_ADDRFROMCOORD_INPUT*   pIn,  ///< [in] Fmask info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeFmaskCoordFromAddr
   *
   *   @brief
   *       Compute coordinates (x,y,slice,sample,plane) according to Fmask address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeFmaskCoordFromAddr(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR_COMPUTE_FMASK_COORDFROMADDR_INPUT*   pIn,  ///< [in] Fmask info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                     DCC key functions
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrComputeDccInfo
   *
   *   @brief
   *       Compute DCC key size, base alignment based on color surface size, tile info or tile index
   *
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeDccInfo(
      ADDR_HANDLE                             hLib,   ///< handle of addrlib
   const ADDR_COMPUTE_DCCINFO_INPUT*       pIn,    ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ///////////////////////////////////////////////////////////////////////////////
   // Below functions are element related or helper functions
   ///////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   AddrGetVersion
   *
   *   @brief
   *       Get AddrLib version number. Client may check this return value against ADDRLIB_VERSION
   *       defined in addrinterface.h to see if there is a mismatch.
   ****************************************************************************************************
   */
   UINT_32 ADDR_API AddrGetVersion(ADDR_HANDLE hLib)
   {
                                 if (pLib)
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrUseTileIndex
   *
   *   @brief
   *       Return TRUE if tileIndex is enabled in this address library
   ****************************************************************************************************
   */
   BOOL_32 ADDR_API AddrUseTileIndex(ADDR_HANDLE hLib)
   {
                                 if (pLib)
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrUseCombinedSwizzle
   *
   *   @brief
   *       Return TRUE if combined swizzle is enabled in this address library
   ****************************************************************************************************
   */
   BOOL_32 ADDR_API AddrUseCombinedSwizzle(ADDR_HANDLE hLib)
   {
                                 if (pLib)
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrExtractBankPipeSwizzle
   *
   *   @brief
   *       Extract Bank and Pipe swizzle from base256b
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrExtractBankPipeSwizzle(
      ADDR_HANDLE                                 hLib,     ///< addrlib handle
   const ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT*  pIn,      ///< [in] input structure
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrCombineBankPipeSwizzle
   *
   *   @brief
   *       Combine Bank and Pipe swizzle
   *   @return
   *       ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrCombineBankPipeSwizzle(
      ADDR_HANDLE                                 hLib,
   const ADDR_COMBINE_BANKPIPE_SWIZZLE_INPUT*  pIn,
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeSliceSwizzle
   *
   *   @brief
   *       Compute a swizzle for slice from a base swizzle
   *   @return
   *       ADDR_OK if no error
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeSliceSwizzle(
      ADDR_HANDLE                                 hLib,
   const ADDR_COMPUTE_SLICESWIZZLE_INPUT*      pIn,
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputeBaseSwizzle
   *
   *   @brief
   *       Return a Combined Bank and Pipe swizzle base on surface based on surface type/index
   *   @return
   *       ADDR_OK if no error
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputeBaseSwizzle(
      ADDR_HANDLE                             hLib,
   const ADDR_COMPUTE_BASE_SWIZZLE_INPUT*  pIn,
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   ElemFlt32ToDepthPixel
   *
   *   @brief
   *       Convert a FLT_32 value to a depth/stencil pixel value
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   *
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API ElemFlt32ToDepthPixel(
      ADDR_HANDLE                         hLib,    ///< addrlib handle
   const ELEM_FLT32TODEPTHPIXEL_INPUT* pIn,     ///< [in] per-component value
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   ElemFlt32ToColorPixel
   *
   *   @brief
   *       Convert a FLT_32 value to a red/green/blue/alpha pixel value
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   *
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API ElemFlt32ToColorPixel(
      ADDR_HANDLE                         hLib,    ///< addrlib handle
   const ELEM_FLT32TOCOLORPIXEL_INPUT* pIn,     ///< [in] format, surface number and swap value
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   ElemGetExportNorm
   *
   *   @brief
   *       Helper function to check one format can be EXPORT_NUM,
   *       which is a register CB_COLOR_INFO.SURFACE_FORMAT.
   *       FP16 can be reported as EXPORT_NORM for rv770 in r600
   *       family
   *
   ****************************************************************************************************
   */
   BOOL_32 ADDR_API ElemGetExportNorm(
      ADDR_HANDLE                     hLib, ///< addrlib handle
      {
      Addr::Lib* pLib = Lib::GetLib(hLib);
                     if (pLib != NULL)
   {
         }
   else
   {
                              }
      /**
   ****************************************************************************************************
   *   ElemSize
   *
   *   @brief
   *       Get bits-per-element for specified format
   *
   *   @return
   *       Bits-per-element of specified format
   *
   ****************************************************************************************************
   */
   UINT_32 ADDR_API ElemSize(
      ADDR_HANDLE hLib,
      {
                        if (pLib != NULL)
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrConvertTileInfoToHW
   *
   *   @brief
   *       Convert tile info from real value to hardware register value
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrConvertTileInfoToHW(
      ADDR_HANDLE                             hLib, ///< address lib handle
   const ADDR_CONVERT_TILEINFOTOHW_INPUT*  pIn,  ///< [in] tile info with real value
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrConvertTileIndex
   *
   *   @brief
   *       Convert tile index to tile mode/type/info
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrConvertTileIndex(
      ADDR_HANDLE                          hLib, ///< address lib handle
   const ADDR_CONVERT_TILEINDEX_INPUT*  pIn,  ///< [in] input - tile index
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrGetMacroModeIndex
   *
   *   @brief
   *       Get macro mode index based on input parameters
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrGetMacroModeIndex(
      ADDR_HANDLE                          hLib, ///< address lib handle
   const ADDR_GET_MACROMODEINDEX_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrConvertTileIndex1
   *
   *   @brief
   *       Convert tile index to tile mode/type/info
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrConvertTileIndex1(
      ADDR_HANDLE                          hLib, ///< address lib handle
   const ADDR_CONVERT_TILEINDEX1_INPUT* pIn,  ///< [in] input - tile index
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrGetTileIndex
   *
   *   @brief
   *       Get tile index from tile mode/type/info
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   *
   *   @note
   *       Only meaningful for SI (and above)
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrGetTileIndex(
      ADDR_HANDLE                     hLib,
   const ADDR_GET_TILEINDEX_INPUT* pIn,
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrComputePrtInfo
   *
   *   @brief
   *       Interface function for ComputePrtInfo
   *
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrComputePrtInfo(
      ADDR_HANDLE                 hLib,
   const ADDR_PRT_INFO_INPUT*  pIn,
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrGetMaxAlignments
   *
   *   @brief
   *       Convert maximum alignments
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrGetMaxAlignments(
      ADDR_HANDLE                     hLib, ///< address lib handle
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   AddrGetMaxMetaAlignments
   *
   *   @brief
   *       Convert maximum alignments for metadata
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API AddrGetMaxMetaAlignments(
      ADDR_HANDLE                     hLib, ///< address lib handle
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                    Surface functions for Addr2
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Addr2ComputeSurfaceInfo
   *
   *   @brief
   *       Calculate surface width/height/depth/alignments and suitable tiling mode
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeSurfaceInfo(
      ADDR_HANDLE                                hLib, ///< address lib handle
   const ADDR2_COMPUTE_SURFACE_INFO_INPUT*    pIn,  ///< [in] surface information
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeSurfaceAddrFromCoord
   *
   *   @brief
   *       Compute surface address according to coordinates
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeSurfaceAddrFromCoord(
      ADDR_HANDLE                                         hLib, ///< address lib handle
   const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT*    pIn,  ///< [in] surface info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeSurfaceCoordFromAddr
   *
   *   @brief
   *       Compute coordinates according to surface address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeSurfaceCoordFromAddr(
      ADDR_HANDLE                                         hLib, ///< address lib handle
   const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT*    pIn,  ///< [in] surface info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                   HTile functions for Addr2
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Addr2ComputeHtileInfo
   *
   *   @brief
   *       Compute Htile pitch, height, base alignment and size in bytes
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeHtileInfo(
      ADDR_HANDLE                              hLib, ///< address lib handle
   const ADDR2_COMPUTE_HTILE_INFO_INPUT*    pIn,  ///< [in] Htile information
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeHtileAddrFromCoord
   *
   *   @brief
   *       Compute Htile address according to coordinates (of depth buffer)
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeHtileAddrFromCoord(
      ADDR_HANDLE                                       hLib, ///< address lib handle
   const ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*    pIn,  ///< [in] Htile info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeHtileCoordFromAddr
   *
   *   @brief
   *       Compute coordinates within depth buffer (1st pixel of a micro tile) according to
   *       Htile address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeHtileCoordFromAddr(
      ADDR_HANDLE                                       hLib, ///< address lib handle
   const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT*    pIn,  ///< [in] Htile info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                     C-mask functions for Addr2
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Addr2ComputeCmaskInfo
   *
   *   @brief
   *       Compute Cmask pitch, height, base alignment and size in bytes from color buffer
   *       info
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeCmaskInfo(
      ADDR_HANDLE                              hLib, ///< address lib handle
   const ADDR2_COMPUTE_CMASK_INFO_INPUT*    pIn,  ///< [in] Cmask pitch and height
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeCmaskAddrFromCoord
   *
   *   @brief
   *       Compute Cmask address according to coordinates (of MSAA color buffer)
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeCmaskAddrFromCoord(
      ADDR_HANDLE                                       hLib, ///< address lib handle
   const ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*    pIn,  ///< [in] Cmask info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeCmaskCoordFromAddr
   *
   *   @brief
   *       Compute coordinates within color buffer (1st pixel of a micro tile) according to
   *       Cmask address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeCmaskCoordFromAddr(
      ADDR_HANDLE                                       hLib, ///< address lib handle
   const ADDR2_COMPUTE_CMASK_COORDFROMADDR_INPUT*    pIn,  ///< [in] Cmask info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                     F-mask functions for Addr2
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Addr2ComputeFmaskInfo
   *
   *   @brief
   *       Compute Fmask pitch/height/depth/alignments and size in bytes
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeFmaskInfo(
      ADDR_HANDLE                              hLib, ///< address lib handle
   const ADDR2_COMPUTE_FMASK_INFO_INPUT*    pIn,  ///< [in] Fmask information
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeFmaskAddrFromCoord
   *
   *   @brief
   *       Compute Fmask address according to coordinates (x,y,slice,sample,plane)
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeFmaskAddrFromCoord(
      ADDR_HANDLE                                       hLib, ///< address lib handle
   const ADDR2_COMPUTE_FMASK_ADDRFROMCOORD_INPUT*    pIn,  ///< [in] Fmask info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
         /**
   ****************************************************************************************************
   *   Addr2ComputeFmaskCoordFromAddr
   *
   *   @brief
   *       Compute coordinates (x,y,slice,sample,plane) according to Fmask address
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeFmaskCoordFromAddr(
      ADDR_HANDLE                                       hLib, ///< address lib handle
   const ADDR2_COMPUTE_FMASK_COORDFROMADDR_INPUT*    pIn,  ///< [in] Fmask info and address
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
            ////////////////////////////////////////////////////////////////////////////////////////////////////
   //                                     DCC key functions for Addr2
   ////////////////////////////////////////////////////////////////////////////////////////////////////
      /**
   ****************************************************************************************************
   *   Addr2ComputeDccInfo
   *
   *   @brief
   *       Compute DCC key size, base alignment based on color surface size, tile info or tile index
   *
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeDccInfo(
      ADDR_HANDLE                           hLib,   ///< handle of addrlib
   const ADDR2_COMPUTE_DCCINFO_INPUT*    pIn,    ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2ComputeDccAddrFromCoord
   *
   *   @brief
   *       Compute DCC key address according to coordinates
   *
   *   @return
   *       ADDR_OK if successful, otherwise an error code of ADDR_E_RETURNCODE
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeDccAddrFromCoord(
      ADDR_HANDLE                                     hLib, ///< address lib handle
   const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT*    pIn,  ///< [in] Dcc info and coordinates
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2ComputePipeBankXor
   *
   *   @brief
   *       Calculate a valid bank pipe xor value for client to use.
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputePipeBankXor(
      ADDR_HANDLE                            hLib, ///< handle of addrlib
   const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2ComputeSlicePipeBankXor
   *
   *   @brief
   *       Calculate slice pipe bank xor value based on base pipe bank xor and slice id.
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeSlicePipeBankXor(
      ADDR_HANDLE                                  hLib, ///< handle of addrlib
   const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2ComputeSubResourceOffsetForSwizzlePattern
   *
   *   @brief
   *       Calculate sub resource offset for swizzle pattern.
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeSubResourceOffsetForSwizzlePattern(
      ADDR_HANDLE                                                     hLib, ///< handle of addrlib
   const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2ComputeNonBlockCompressedView
   *
   *   @brief
   *       Compute non-block-compressed view for a given mipmap level/slice.
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2ComputeNonBlockCompressedView(
      ADDR_HANDLE                                       hLib, ///< handle of addrlib
   const ADDR2_COMPUTE_NONBLOCKCOMPRESSEDVIEW_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2GetPreferredSurfaceSetting
   *
   *   @brief
   *       Suggest a preferred setting for client driver to program HW register
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2GetPreferredSurfaceSetting(
      ADDR_HANDLE                                   hLib, ///< handle of addrlib
   const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2IsValidDisplaySwizzleMode
   *
   *   @brief
   *       Return whether the swizzle mode is supported by display engine
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2IsValidDisplaySwizzleMode(
      ADDR_HANDLE     hLib,
   AddrSwizzleMode swizzleMode,
   UINT_32         bpp,
      {
                        if (pLib != NULL)
   {
      ADDR2_COMPUTE_SURFACE_INFO_INPUT in = {};
   in.resourceType = ADDR_RSRC_TEX_2D;
   in.swizzleMode  = swizzleMode;
            *pResult   = pLib->IsValidDisplaySwizzleMode(&in);
      }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2GetPossibleSwizzleModes
   *
   *   @brief
   *       Returns a list of swizzle modes that are valid from the hardware's perspective for the
   *       client to choose from
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2GetPossibleSwizzleModes(
      ADDR_HANDLE                                   hLib, ///< handle of addrlib
   const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,  ///< [in] input
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
   /**
   ****************************************************************************************************
   *   Addr2GetAllowedBlockSet
   *
   *   @brief
   *       Returns the set of allowed block sizes given the allowed swizzle modes and resource type
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2GetAllowedBlockSet(
      ADDR_HANDLE      hLib,              ///< handle of addrlib
   ADDR2_SWMODE_SET allowedSwModeSet,  ///< [in] allowed swizzle modes
   AddrResourceType rsrcType,          ///< [in] resource type
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2GetAllowedSwSet
   *
   *   @brief
   *       Returns the set of allowed swizzle types given the allowed swizzle modes
   ****************************************************************************************************
   */
   ADDR_E_RETURNCODE ADDR_API Addr2GetAllowedSwSet(
      ADDR_HANDLE       hLib,              ///< handle of addrlib
   ADDR2_SWMODE_SET  allowedSwModeSet,  ///< [in] allowed swizzle modes
      {
                        if (pLib != NULL)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2IsBlockTypeAvailable
   *
   *   @brief
   *       Determine whether a block type is allowed in a given blockSet
   ****************************************************************************************************
   */
   BOOL_32 Addr2IsBlockTypeAvailable(
      ADDR2_BLOCK_SET blockSet,
      {
               if (blockType == AddrBlockLinear)
   {
         }
   else
   {
                     }
      /**
   ****************************************************************************************************
   *   Addr2BlockTypeWithinMemoryBudget
   *
   *   @brief
   *       Determine whether a new block type is acceptable based on memory waste ratio. Will favor
   *       larger block types.
   ****************************************************************************************************
   */
   BOOL_32 Addr2BlockTypeWithinMemoryBudget(
      UINT_64 minSize,
   UINT_64 newBlockTypeSize,
   UINT_32 ratioLow,
   UINT_32 ratioHi,
   DOUBLE  memoryBudget,
      {
               if (memoryBudget >= 1.0)
   {
      if (newBlockTypeBigger)
   {
         if ((static_cast<DOUBLE>(newBlockTypeSize) / minSize) <= memoryBudget)
   {
         }
   else
   {
         if ((static_cast<DOUBLE>(minSize) / newBlockTypeSize) > memoryBudget)
   {
            }
   else
   {
      if (newBlockTypeBigger)
   {
         if ((newBlockTypeSize * ratioHi) <= (minSize * ratioLow))
   {
         }
   else
   {
         if ((newBlockTypeSize * ratioLow) < (minSize * ratioHi))
   {
                        }
