   #include <vndk/window.h>
      extern "C" {
      AHardwareBuffer *
   ANativeWindowBuffer_getHardwareBuffer(ANativeWindowBuffer *anwb)
   {
         }
      void
   AHardwareBuffer_acquire(AHardwareBuffer *buffer)
   {
   }
      void
   AHardwareBuffer_release(AHardwareBuffer *buffer)
   {
   }
      void
   AHardwareBuffer_describe(const AHardwareBuffer *buffer,
         {
   }
      int
   AHardwareBuffer_allocate(const AHardwareBuffer_Desc *desc,
         {
         }
      const native_handle_t *
   AHardwareBuffer_getNativeHandle(const AHardwareBuffer *buffer)
   {
         }
      void
   ANativeWindow_acquire(ANativeWindow *window)
   {
   }
      void
   ANativeWindow_release(ANativeWindow *window)
   {
   }
      int32_t
   ANativeWindow_getFormat(ANativeWindow *window)
   {
         }
      int
   ANativeWindow_setSwapInterval(ANativeWindow *window, int interval)
   {
         }
      int
   ANativeWindow_query(const ANativeWindow *window,
               {
         }
      int
   ANativeWindow_dequeueBuffer(ANativeWindow *window,
               {
         }
      int
   ANativeWindow_queueBuffer(ANativeWindow *window,
               {
         }
      int ANativeWindow_cancelBuffer(ANativeWindow* window,
                     }
      int
   ANativeWindow_setUsage(ANativeWindow *window, uint64_t usage)
   {
         }
      int
   ANativeWindow_setSharedBufferMode(ANativeWindow *window,
         {
         }
   }
