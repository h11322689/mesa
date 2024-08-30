   /*
   * Copyright © 2003 Felix Kuehling
   * Copyright © 2018 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
   * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   */
      #include "util/os_misc.h"
   #include "u_process.h"
   #include "detect_os.h"
   #include "macros.h"
   #include <string.h>
   #include <errno.h>
   #include <stdlib.h>
      #if DETECT_OS_WINDOWS
   #include <windows.h>
   #else
   #include <unistd.h>
   #endif
      #if DETECT_OS_APPLE
   #include <mach-o/dyld.h>
   #endif
      #if DETECT_OS_BSD
   #include <sys/types.h>
   #include <sys/sysctl.h>
   #endif
      #if DETECT_OS_LINUX
   #include <fcntl.h>
   #endif
      #include "util/u_call_once.h"
      #undef GET_PROGRAM_NAME_NOT_AVAILABLE
      #if defined(__linux__) && defined(HAVE_PROGRAM_INVOCATION_NAME)
   static char *
   __getProgramName()
   {
      char * arg = strrchr(program_invocation_name, '/');
   if (arg) {
      char *program_name = NULL;
   /* If the / character was found this is likely a linux path or
   * an invocation path for a 64-bit wine program.
   *
   * However, some programs pass command line arguments into argv[0].
   * Strip these arguments out by using the realpath only if it was
   * a prefix of the invocation name.
   */
            if (path && strncmp(path, program_invocation_name, strlen(path)) == 0) {
      /* This shouldn't be null because path is a a prefix,
   * but check it anyway since path is static. */
   char * name = strrchr(path, '/');
   if (name)
      }
   if (path) {
         }
   if (!program_name) {
         }
               /* If there was no '/' at all we likely have a windows like path from
   * a wine application.
   */
   arg = strrchr(program_invocation_name, '\\');
   if (arg)
               }
   #elif defined(HAVE_PROGRAM_INVOCATION_NAME)
   static char *
   __getProgramName()
   {
         }
   #elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__) || defined(ANDROID) || defined(__NetBSD__)
   #if defined(__NetBSD__)
   #    include <sys/param.h>
   #endif
   #if defined(__NetBSD_Version__) && (__NetBSD_Version__ < 106000100)
   #define GET_PROGRAM_NAME_NOT_AVAILABLE
   #else /* !(defined(__NetBSD_Version__) && (__NetBSD_Version__ < 106000100)) */
   static char *
   __getProgramName()
   {
      const char *program_name = getprogname();
   if (program_name) {
         }
      }
   #endif /* defined(__NetBSD_Version__) && (__NetBSD_Version__ < 106000100) */
   #elif defined(__sun)
   /* Solaris has getexecname() which returns the full path - return just
         #    include <libgen.h>
      static char *
   __getProgramName()
   {
      char *progname = NULL;
   const char *e = getexecname();
   if (e != NULL) {
      /* Have to make a copy since getexecname can return a readonly
         char *n = strdup(e);
   if (n != NULL) {
      progname = strdup(basename(n));
         }
      }
   #elif DETECT_OS_WINDOWS
   static char *
   __getProgramName()
   {
      char *progname;
   static char buf[MAX_PATH];
   GetModuleFileNameA(NULL, buf, sizeof(buf));
   progname = strrchr(buf, '\\');
   if (progname)
         else
            }
   #elif DETECT_OS_HAIKU
   #  include <kernel/OS.h>
   #  include <kernel/image.h>
   static char *
   __getProgramName()
   {
      image_info info;
   get_image_info(B_CURRENT_TEAM, &info);
      }
   #else
   #define GET_PROGRAM_NAME_NOT_AVAILABLE
   #endif
      #if defined(GET_PROGRAM_NAME_NOT_AVAILABLE)
   #if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__UCLIBC__) || defined(ANDROID)
   /* This is a hack. It's said to work on OpenBSD, NetBSD and GNU.
   * Rogelio M.Serrano Jr. reported it's also working with UCLIBC. It's
   * used as a last resort, if there is no documented facility available. */
   static char *
   __getProgramName()
   {
      extern const char *__progname;
   char * arg = strrchr(__progname, '/');
   if (arg)
         else
      }
   #else
   #pragma message ( "Warning: Per application configuration won't work with your OS version." )
   static char *
   __getProgramName()
   {
         }
   #endif
   #endif /* GET_PROGRAM_NAME_NOT_AVAILABLE */
      static char *program_name;
      static void
   free_program_name(void)
   {
      free(program_name);
      }
      static void
   util_get_process_name_callback(void)
   {
      const char *override_name = os_get_option("MESA_PROCESS_NAME");
            if (program_name)
      }
      const char *
   util_get_process_name(void)
   {
      static util_once_flag once_state = UTIL_ONCE_FLAG_INIT;
   util_call_once(&once_state, util_get_process_name_callback);
      }
      size_t
   util_get_process_exec_path(char* process_path, size_t len)
   {
   #if DETECT_OS_WINDOWS
         #elif DETECT_OS_APPLE
      uint32_t bufSize = len;
               #elif DETECT_OS_FREEBSD
               (void) sysctl(mib, 4, process_path, &len, NULL, 0);
               #elif DETECT_OS_UNIX
               if ((r = readlink("/proc/self/exe", process_path, len)) > 0)
         if ((r = readlink("/proc/curproc/exe", process_path, len)) > 0)
         if ((r = readlink("/proc/curproc/file", process_path, len)) > 0)
               success:
      if (r == len)
            process_path[r] = '\0';
         #endif
         }
      bool
   util_get_command_line(char *cmdline, size_t size)
   {
   #if DETECT_OS_WINDOWS
      const char *args = GetCommandLineA();
   if (args) {
      strncpy(cmdline, args, size);
   // make sure we terminate the string
   cmdline[size - 1] = 0;
         #elif DETECT_OS_LINUX
      int f = open("/proc/self/cmdline", O_RDONLY);
   if (f != -1) {
      const int n = read(f, cmdline, size - 1);
   int i;
   assert(n < size);
   // The arguments are separated by '\0' chars.  Convert them to spaces.
   for (i = 0; i < n; i++) {
      if (cmdline[i] == 0) {
            }
   // terminate the string
   cmdline[n] = 0;
   close(f);
         #elif DETECT_OS_BSD
      int mib[] = {
      #if DETECT_OS_NETBSD || DETECT_OS_OPENBSD
         KERN_PROC_ARGS,
   getpid(),
   #else
         KERN_PROC,
   KERN_PROC_ARGS,
   #endif
               /* Like /proc/pid/cmdline each argument is separated by NUL byte */
   if (sysctl(mib, ARRAY_SIZE(mib), cmdline, &size, NULL, 0) == -1) {
                  /* Replace NUL with space except terminating NUL */
   for (size_t i = 0; i < (size - 1); i++) {
      if (cmdline[i] == '\0')
                  #endif
                  cmdline[0] = 0;
      }
