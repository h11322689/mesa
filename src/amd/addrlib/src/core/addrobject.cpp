   /*
   ************************************************************************************************************************
   *
   *  Copyright (C) 2007-2022 Advanced Micro Devices, Inc.  All rights reserved.
   *  SPDX-License-Identifier: MIT
   *
   ***********************************************************************************************************************/
         /**
   ****************************************************************************************************
   * @file  addrobject.cpp
   * @brief Contains the Object base class implementation.
   ****************************************************************************************************
   */
      #include "addrinterface.h"
   #include "addrobject.h"
      namespace Addr
   {
      /**
   ****************************************************************************************************
   *   Object::Object
   *
   *   @brief
   *       Constructor for the Object class.
   ****************************************************************************************************
   */
   Object::Object()
   {
      m_client.handle = NULL;
   m_client.callbacks.allocSysMem = NULL;
   m_client.callbacks.freeSysMem = NULL;
      }
      /**
   ****************************************************************************************************
   *   Object::Object
   *
   *   @brief
   *       Constructor for the Object class.
   ****************************************************************************************************
   */
   Object::Object(const Client* pClient)
   {
         }
      /**
   ****************************************************************************************************
   *   Object::~Object
   *
   *   @brief
   *       Destructor for the Object class.
   ****************************************************************************************************
   */
   Object::~Object()
   {
   }
      /**
   ****************************************************************************************************
   *   Object::ClientAlloc
   *
   *   @brief
   *       Calls instanced allocSysMem inside Client
   ****************************************************************************************************
   */
   VOID* Object::ClientAlloc(
      size_t         objSize,    ///< [in] Size to allocate
      {
               if (pClient->callbacks.allocSysMem != NULL)
   {
               allocInput.size        = sizeof(ADDR_ALLOCSYSMEM_INPUT);
   allocInput.flags.value = 0;
   allocInput.sizeInBytes = static_cast<UINT_32>(objSize);
                           }
      /**
   ****************************************************************************************************
   *   Object::Alloc
   *
   *   @brief
   *       A wrapper of ClientAlloc
   ****************************************************************************************************
   */
   VOID* Object::Alloc(
      size_t objSize      ///< [in] Size to allocate
      {
         }
      /**
   ****************************************************************************************************
   *   Object::ClientFree
   *
   *   @brief
   *       Calls freeSysMem inside Client
   ****************************************************************************************************
   */
   VOID Object::ClientFree(
      VOID*          pObjMem,    ///< [in] User virtual address to free.
      {
      if (pClient->callbacks.freeSysMem != NULL)
   {
      if (pObjMem != NULL)
   {
                  freeInput.size      = sizeof(ADDR_FREESYSMEM_INPUT);
                        }
      /**
   ****************************************************************************************************
   *   Object::Free
   *
   *   @brief
   *       A wrapper of ClientFree
   ****************************************************************************************************
   */
   VOID Object::Free(
      VOID* pObjMem       ///< [in] User virtual address to free.
      {
         }
      /**
   ****************************************************************************************************
   *   Object::operator new
   *
   *   @brief
   *       Placement new operator. (with pre-allocated memory pointer)
   *
   *   @return
   *       Returns pre-allocated memory pointer.
   ****************************************************************************************************
   */
   VOID* Object::operator new(
      size_t objSize,     ///< [in] Size to allocate
   VOID*  pMem        ///< [in] Pre-allocated pointer
      {
         }
      /**
   ****************************************************************************************************
   *   Object::operator delete
   *
   *   @brief
   *       Frees Object object memory.
   ****************************************************************************************************
   */
   VOID Object::operator delete(
         {
      Object* pObj = static_cast<Object*>(pObjMem);
      }
      /**
   ****************************************************************************************************
   *   Object::DebugPrint
   *
   *   @brief
   *       Print debug message
   *
   *   @return
   *       N/A
   ****************************************************************************************************
   */
   VOID Object::DebugPrint(
      const CHAR* pDebugString,     ///< [in] Debug string
   ...
      {
   #if DEBUG
      if (m_client.callbacks.debugPrint != NULL)
   {
                                 debugPrintInput.size         = sizeof(ADDR_DEBUGPRINT_INPUT);
   debugPrintInput.pDebugString = const_cast<CHAR*>(pDebugString);
   debugPrintInput.hClient      = m_client.handle;
                     va_end(ap);
         #endif
   }
      } // Addr
