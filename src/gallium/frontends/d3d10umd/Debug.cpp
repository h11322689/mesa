   #include "Debug.h"
      #include <stdarg.h>
   #include <stdio.h>
         #ifdef DEBUG
      unsigned st_debug = 0;
      static const
   struct debug_named_value st_debug_flags[] = {
      {"oldtexops", ST_DEBUG_OLD_TEX_OPS, "oldtexops"},
   {"tgsi", ST_DEBUG_TGSI, "tgsi"},
      };
   void
   st_debug_parse(void)
   {
         }
      #endif
         void
   DebugPrintf(const char *format, ...)
   {
               va_list ap;
   va_start(ap, format);
   vsnprintf(buf, sizeof buf, format, ap);
               }
         /**
   * Produce a human readable message from HRESULT.
   *
   * @sa http://msdn.microsoft.com/en-us/library/ms679351(VS.85).aspx
   */
   void
   CheckHResult(HRESULT hr, const char *function, unsigned line)
   {
      if (FAILED(hr)) {
               FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM,
   NULL,
   hr,
                              }
         void
   AssertFail(const char *expr,
            const char *file,
      {
         #if defined(__GNUC__)
         #elif defined(_MSC_VER)
         #else
         #endif
   }
