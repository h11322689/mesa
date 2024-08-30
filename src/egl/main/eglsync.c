   /**************************************************************************
   *
   * Copyright 2010 LunarG, Inc.
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
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <inttypes.h>
   #include <string.h>
      #include "eglcurrent.h"
   #include "egldriver.h"
   #include "egllog.h"
   #include "eglsync.h"
      /**
   * Parse the list of sync attributes and return the proper error code.
   */
   static EGLint
   _eglParseSyncAttribList(_EGLSync *sync, const EGLAttrib *attrib_list)
   {
               if (!attrib_list)
            for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLAttrib attr = attrib_list[i++];
   EGLAttrib val = attrib_list[i];
            switch (attr) {
   case EGL_CL_EVENT_HANDLE_KHR:
      if (sync->Type == EGL_SYNC_CL_EVENT_KHR) {
         } else {
         }
      case EGL_SYNC_NATIVE_FENCE_FD_ANDROID:
      if (sync->Type == EGL_SYNC_NATIVE_FENCE_ANDROID) {
      /* we take ownership of the native fd, so no dup(): */
      } else {
         }
      default:
      err = EGL_BAD_ATTRIBUTE;
               if (err != EGL_SUCCESS) {
      _eglLog(_EGL_DEBUG, "bad sync attribute 0x%" PRIxPTR, attr);
                     }
      EGLBoolean
   _eglInitSync(_EGLSync *sync, _EGLDisplay *disp, EGLenum type,
         {
               _eglInitResource(&sync->Resource, sizeof(*sync), disp);
   sync->Type = type;
   sync->SyncStatus = EGL_UNSIGNALED_KHR;
                     switch (type) {
   case EGL_SYNC_CL_EVENT_KHR:
      sync->SyncCondition = EGL_SYNC_CL_EVENT_COMPLETE_KHR;
      case EGL_SYNC_NATIVE_FENCE_ANDROID:
      if (sync->SyncFd == EGL_NO_NATIVE_FENCE_FD_ANDROID)
         else
            default:
                  if (err != EGL_SUCCESS)
            if (type == EGL_SYNC_CL_EVENT_KHR && !sync->CLEvent)
               }
      EGLBoolean
   _eglGetSyncAttrib(_EGLDisplay *disp, _EGLSync *sync, EGLint attribute,
         {
      switch (attribute) {
   case EGL_SYNC_TYPE_KHR:
      *value = sync->Type;
      case EGL_SYNC_STATUS_KHR:
      /* update the sync status */
   if (sync->SyncStatus != EGL_SIGNALED_KHR &&
      (sync->Type == EGL_SYNC_FENCE_KHR ||
   sync->Type == EGL_SYNC_CL_EVENT_KHR ||
   sync->Type == EGL_SYNC_REUSABLE_KHR ||
               *value = sync->SyncStatus;
      case EGL_SYNC_CONDITION_KHR:
      if (sync->Type != EGL_SYNC_FENCE_KHR &&
      sync->Type != EGL_SYNC_CL_EVENT_KHR &&
   sync->Type != EGL_SYNC_NATIVE_FENCE_ANDROID)
      *value = sync->SyncCondition;
         default:
      return _eglError(EGL_BAD_ATTRIBUTE, "eglGetSyncAttribKHR");
                  }
