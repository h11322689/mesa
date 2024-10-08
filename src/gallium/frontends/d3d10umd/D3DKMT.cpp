   /**************************************************************************
   *
   * Copyright 2012-2021 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
      /*
   * D3DKMT.cpp --
   *    Implement kernel mode thunks, so that this can be loaded as a
   *    software DLL (D3D_DRIVER_TYPE_SOFTWARE).
   */
         #include "DriverIncludes.h"
      #include "Debug.h"
         #ifndef STATUS_NOT_IMPLEMENTED
   #define STATUS_NOT_IMPLEMENTED 0xC0000002
   #endif
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateAllocation(D3DKMT_CREATEALLOCATION *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateAllocation2(D3DKMT_CREATEALLOCATION *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTQueryResourceInfo(D3DKMT_QUERYRESOURCEINFO *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenResource(D3DKMT_OPENRESOURCE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenResource2(D3DKMT_OPENRESOURCE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroyAllocation(CONST D3DKMT_DESTROYALLOCATION *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetAllocationPriority(CONST D3DKMT_SETALLOCATIONPRIORITY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTQueryAllocationResidency(CONST D3DKMT_QUERYALLOCATIONRESIDENCY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateDevice(D3DKMT_CREATEDEVICE *pData)
   {
      LOG_ENTRYPOINT();
   pData->hDevice = 1;
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroyDevice(CONST D3DKMT_DESTROYDEVICE *pData)
   {
      LOG_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateContext(D3DKMT_CREATECONTEXT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroyContext(CONST D3DKMT_DESTROYCONTEXT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateSynchronizationObject(D3DKMT_CREATESYNCHRONIZATIONOBJECT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateSynchronizationObject2(D3DKMT_CREATESYNCHRONIZATIONOBJECT2 *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenSynchronizationObject(D3DKMT_OPENSYNCHRONIZATIONOBJECT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroySynchronizationObject(CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTWaitForSynchronizationObject(CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTWaitForSynchronizationObject2(CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT2 *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSignalSynchronizationObject(CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSignalSynchronizationObject2(CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT2 *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTLock(D3DKMT_LOCK *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTUnlock(CONST D3DKMT_UNLOCK *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetDisplayModeList(D3DKMT_GETDISPLAYMODELIST *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetDisplayMode(CONST D3DKMT_SETDISPLAYMODE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetMultisampleMethodList(D3DKMT_GETMULTISAMPLEMETHODLIST *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTPresent(D3DKMT_PRESENT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTRender(D3DKMT_RENDER *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetRuntimeData(CONST D3DKMT_GETRUNTIMEDATA *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTQueryAdapterInfo(CONST D3DKMT_QUERYADAPTERINFO *pData)
   {
               switch (pData->Type) {
   case KMTQAITYPE_UMDRIVERNAME:
      {
      D3DKMT_UMDFILENAMEINFO *pResult =
         if (pResult->Version != KMTUMDVERSION_DX10 &&
         DebugPrintf("%s: unsupported UMD version (%u)\n",
               }
   HMODULE hModule = 0;
   BOOL bRet;
   DWORD dwRet;
   bRet = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
               assert(bRet);
   dwRet = GetModuleFileNameW(hModule, pResult->UmdFileName, MAX_PATH);
   assert(dwRet);
      }
      case KMTQAITYPE_GETSEGMENTSIZE:
      {
      D3DKMT_SEGMENTSIZEINFO *pResult =
         pResult->DedicatedVideoMemorySize = 0;
   pResult->DedicatedSystemMemorySize = 0;
   pResult->SharedSystemMemorySize = 3ULL*1024ULL*1024ULL*1024ULL;
      }
      case KMTQAITYPE_CHECKDRIVERUPDATESTATUS:
      {
      BOOL *pResult = (BOOL *)pData->pPrivateDriverData;
   *pResult = false;
         case KMTQAITYPE_DRIVERVERSION:
      {
      D3DKMT_DRIVERVERSION *pResult = (D3DKMT_DRIVERVERSION *)pData->pPrivateDriverData;
   *pResult = KMT_DRIVERVERSION_WDDM_1_0;
         case KMTQAITYPE_XBOX:
      {
      BOOL *pResult = (BOOL *)pData->pPrivateDriverData;
   *pResult = false;
         case KMTQAITYPE_PHYSICALADAPTERCOUNT:
      {
      UINT *pResult = (UINT *)pData->pPrivateDriverData;
   *pResult = 1;
         case KMTQAITYPE_PHYSICALADAPTERDEVICEIDS:
      ZeroMemory(pData->pPrivateDriverData, pData->PrivateDriverDataSize);
      default:
      DebugPrintf("%s: unsupported query type (Type=%u, PrivateDriverDataSize=%u)\n",
         ZeroMemory(pData->pPrivateDriverData, pData->PrivateDriverDataSize);
         }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenAdapterFromHdc(D3DKMT_OPENADAPTERFROMHDC *pData)
   {
      LOG_ENTRYPOINT();
   pData->hAdapter = 1;
   pData->AdapterLuid.LowPart = 0;
   pData->AdapterLuid.HighPart = 0;
   pData->VidPnSourceId = 1;
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenAdapterFromGdiDisplayName(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *pData)
   {
      LOG_ENTRYPOINT();
   pData->hAdapter = 1;
   pData->AdapterLuid.LowPart = 0;
   pData->AdapterLuid.HighPart = 0;
   pData->VidPnSourceId = 1;
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenAdapterFromDeviceName(D3DKMT_OPENADAPTERFROMDEVICENAME *pData)
   {
      LOG_ENTRYPOINT();
   pData->hAdapter = 1;
   pData->AdapterLuid.LowPart = 0;
   pData->AdapterLuid.HighPart = 0;
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCloseAdapter(CONST D3DKMT_CLOSEADAPTER *pData)
   {
      LOG_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetSharedPrimaryHandle(D3DKMT_GETSHAREDPRIMARYHANDLE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTEscape(CONST D3DKMT_ESCAPE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetVidPnSourceOwner(CONST D3DKMT_SETVIDPNSOURCEOWNER *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetVidPnSourceOwner1(CONST D3DKMT_SETVIDPNSOURCEOWNER1 *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetPresentHistory(D3DKMT_GETPRESENTHISTORY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetPresentQueueEvent(D3DKMT_HANDLE hAdapter, HANDLE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateOverlay(D3DKMT_CREATEOVERLAY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTUpdateOverlay(CONST D3DKMT_UPDATEOVERLAY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTFlipOverlay(CONST D3DKMT_FLIPOVERLAY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroyOverlay(CONST D3DKMT_DESTROYOVERLAY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTWaitForVerticalBlankEvent(CONST D3DKMT_WAITFORVERTICALBLANKEVENT *pData)
   {
      LOG_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetGammaRamp(CONST D3DKMT_SETGAMMARAMP *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetDeviceState(D3DKMT_GETDEVICESTATE *pData)
   {
      LOG_ENTRYPOINT();
   switch (pData->StateType) {
   case D3DKMT_DEVICESTATE_EXECUTION:
      pData->ExecutionState = D3DKMT_DEVICEEXECUTION_ACTIVE;
      case D3DKMT_DEVICESTATE_PRESENT:
      pData->PresentState.PresentStats.PresentCount = 0;
   pData->PresentState.PresentStats.PresentRefreshCount = 0;
   pData->PresentState.PresentStats.SyncRefreshCount = 0;
   pData->PresentState.PresentStats.SyncQPCTime.QuadPart = 0;
   pData->PresentState.PresentStats.SyncGPUTime.QuadPart = 0;
      case D3DKMT_DEVICESTATE_RESET:
      pData->ResetState.Value = 0;
      default:
            }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateDCFromMemory(D3DKMT_CREATEDCFROMMEMORY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroyDCFromMemory(CONST D3DKMT_DESTROYDCFROMMEMORY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetContextSchedulingPriority(CONST D3DKMT_SETCONTEXTSCHEDULINGPRIORITY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetContextSchedulingPriority(D3DKMT_GETCONTEXTSCHEDULINGPRIORITY *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetProcessSchedulingPriorityClass(HANDLE hProcess, D3DKMT_SCHEDULINGPRIORITYCLASS Priority)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetProcessSchedulingPriorityClass(HANDLE hProcess, D3DKMT_SCHEDULINGPRIORITYCLASS *pPriority)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTReleaseProcessVidPnSourceOwners(HANDLE hProcess)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetScanLine(D3DKMT_GETSCANLINE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTChangeSurfacePointer(CONST D3DKMT_CHANGESURFACEPOINTER *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetQueuedLimit(CONST D3DKMT_SETQUEUEDLIMIT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTPollDisplayChildren(CONST D3DKMT_POLLDISPLAYCHILDREN *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTInvalidateActiveVidPn(CONST D3DKMT_INVALIDATEACTIVEVIDPN *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCheckOcclusion(CONST D3DKMT_CHECKOCCLUSION *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTWaitForIdle(CONST D3DKMT_WAITFORIDLE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCheckMonitorPowerState(CONST D3DKMT_CHECKMONITORPOWERSTATE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C BOOLEAN APIENTRY
   D3DKMTCheckExclusiveOwnership(VOID)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCheckVidPnExclusiveOwnership(CONST D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSetDisplayPrivateDriverFormat(CONST D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSharedPrimaryLockNotification(CONST D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTSharedPrimaryUnLockNotification(CONST D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCreateKeyedMutex(D3DKMT_CREATEKEYEDMUTEX *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTOpenKeyedMutex(D3DKMT_OPENKEYEDMUTEX *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTDestroyKeyedMutex(CONST D3DKMT_DESTROYKEYEDMUTEX *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTAcquireKeyedMutex(D3DKMT_ACQUIREKEYEDMUTEX *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTReleaseKeyedMutex(D3DKMT_RELEASEKEYEDMUTEX *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTConfigureSharedResource(CONST D3DKMT_CONFIGURESHAREDRESOURCE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTGetOverlayState(D3DKMT_GETOVERLAYSTATE *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         EXTERN_C NTSTATUS APIENTRY
   D3DKMTCheckSharedResourceAccess(CONST D3DKMT_CHECKSHAREDRESOURCEACCESS *pData)
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
