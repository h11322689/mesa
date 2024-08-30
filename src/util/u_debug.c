   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * Copyright (c) 2008 VMware, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/u_atomic.h"
   #include "util/u_debug.h"
   #include "util/u_string.h"
   #include "util/u_math.h"
   #include <inttypes.h>
      #include <stdio.h>
   #include <limits.h> /* CHAR_BIT */
   #include <ctype.h> /* isalnum */
      #ifdef _WIN32
   #include <windows.h>
   #include <stdlib.h>
   #endif
         void
   _debug_vprintf(const char *format, va_list ap)
   {
         #if DETECT_OS_WINDOWS
      /* We buffer until we find a newline. */
   size_t len = strlen(buf);
   int ret = vsnprintf(buf + len, sizeof(buf) - len, format, ap);
   if (ret > (int)(sizeof(buf) - len - 1) || strchr(buf + len, '\n')) {
      os_log_message(buf);
         #else
      vsnprintf(buf, sizeof(buf), format, ap);
      #endif
   }
         void
   _util_debug_message(struct util_debug_callback *cb,
                     {
      if (!cb || !cb->debug_message)
         va_list args;
   va_start(args, fmt);
   cb->debug_message(cb->data, id, type, fmt, args);
      }
         #ifdef _WIN32
   void
   debug_disable_win32_error_dialogs(void)
   {
      /* When Windows' error message boxes are disabled for this process (as is
   * typically the case when running tests in an automated fashion) we disable
   * CRT message boxes too.
   */
   UINT uMode = SetErrorMode(0);
      #ifndef _GAMING_XBOX
      if (uMode & SEM_FAILCRITICALERRORS) {
      /* Disable assertion failure message box.
   * http://msdn.microsoft.com/en-us/library/sas1dkb2.aspx
   */
   #ifdef _MSC_VER
         /* Disable abort message box.
   * http://msdn.microsoft.com/en-us/library/e631wekh.aspx
   */
   #endif
         #endif /* _GAMING_XBOX */
   }
   #endif /* _WIN32 */
      bool
   debug_parse_bool_option(const char *str, bool dfault)
   {
               if (str == NULL)
         else if (!strcmp(str, "0"))
         else if (!strcasecmp(str, "n"))
         else if (!strcasecmp(str, "no"))
         else if (!strcasecmp(str, "f"))
         else if (!strcasecmp(str, "false"))
         else if (!strcmp(str, "1"))
         else if (!strcasecmp(str, "y"))
         else if (!strcasecmp(str, "yes"))
         else if (!strcasecmp(str, "t"))
         else if (!strcasecmp(str, "true"))
         else
            }
      static bool
   debug_get_option_should_print(void)
   {
      static bool initialized = false;
            if (unlikely(!p_atomic_read_relaxed(&initialized))) {
      bool parsed_value = debug_parse_bool_option(os_get_option("GALLIUM_PRINT_OPTIONS"), false);
   p_atomic_set(&value, parsed_value);
               /* We do not print value of GALLIUM_PRINT_OPTIONS intentionally. */
      }
         const char *
   debug_get_option(const char *name, const char *dfault)
   {
               result = os_get_option(name);
   if (!result)
            if (debug_get_option_should_print())
      debug_printf("%s: %s = %s\n", __func__, name,
            }
         const char *
   debug_get_option_cached(const char *name, const char *dfault)
   {
               result = os_get_option_cached(name);
   if (!result)
            if (debug_get_option_should_print())
      debug_printf("%s: %s = %s\n", __FUNCTION__, name,
            }
         /**
   * Reads an environment variable and interprets its value as a boolean.
   * Recognizes 0/n/no/f/false case insensitive as false.
   * Recognizes 1/y/yes/t/true case insensitive as true.
   * Other values result in the default value.
   */
   bool
   debug_get_bool_option(const char *name, bool dfault)
   {
      bool result = debug_parse_bool_option(os_get_option(name), dfault);
   if (debug_get_option_should_print())
      debug_printf("%s: %s = %s\n", __func__, name,
            }
         int64_t
   debug_parse_num_option(const char *str, int64_t dfault)
   {
      int64_t result;
   if (!str) {
         } else {
               result = strtoll(str, &endptr, 0);
   if (str == endptr) {
      /* Restore the default value when no digits were found. */
         }
      }
      int64_t
   debug_get_num_option(const char *name, int64_t dfault)
   {
               if (debug_get_option_should_print())
               }
      void
   debug_get_version_option(const char *name, unsigned *major, unsigned *minor)
   {
               str = os_get_option(name);
   if (str) {
      unsigned v_maj, v_min;
            n = sscanf(str, "%u.%u", &v_maj, &v_min);
   if (n != 2) {
      debug_printf("Illegal version specified for %s : %s\n", name, str);
      }
   *major = v_maj;
               if (debug_get_option_should_print())
               }
      static bool
   str_has_option(const char *str, const char *name)
   {
      /* Empty string. */
   if (!*str) {
                  /* OPTION=all */
   if (!strcmp(str, "all")) {
                  /* Find 'name' in 'str' surrounded by non-alphanumeric characters. */
   {
      const char *start = str;
            /* 'start' is the beginning of the currently-parsed word,
   * we increment 'str' each iteration.
   * if we find either the end of string or a non-alphanumeric character,
            while (1) {
      if (!*str || !(isalnum(*str) || *str == '_')) {
      if (str-start == name_len &&
                        if (!*str) {
                                                }
         uint64_t
   debug_parse_flags_option(const char *name,
                     {
      uint64_t result;
   const struct debug_named_value *orig = flags;
            if (!str)
         else if (!strcmp(str, "help")) {
      result = dfault;
   _debug_printf("%s: help for %s:\n", __func__, name);
   for (; flags->name; ++flags)
         for (flags = orig; flags->name; ++flags)
      _debug_printf("| %*s [0x%0*"PRIx64"]%s%s\n", namealign, flags->name,
         }
   else {
      result = 0;
   while (flags->name) {
      if (str_has_option(str, flags->name))
                           }
      uint64_t
   debug_get_flags_option(const char *name,
               {
      const char *str = os_get_option(name);
            if (debug_get_option_should_print()) {
      if (str) {
      debug_printf("%s: %s = 0x%"PRIx64" (%s)\n",
      } else {
                        }
         const char *
   debug_dump_enum(const struct debug_named_value *names,
         {
               while (names->name) {
      if (names->value == value)
                     snprintf(rest, sizeof(rest), "0x%08"PRIx64, value);
      }
         const char *
   debug_dump_flags(const struct debug_named_value *names, uint64_t value)
   {
      static char output[4096];
   static char rest[256];
                     while (names->name) {
      if ((names->value & value) == names->value) {
      if (!first)
         else
         strncat(output, names->name, sizeof(output) - strlen(output) - 1);
   output[sizeof(output) - 1] = '\0';
      }
               if (value) {
      if (!first)
         else
            snprintf(rest, sizeof(rest), "0x%08"PRIx64, value);
   strncat(output, rest, sizeof(output) - strlen(output) - 1);
               if (first)
               }
         uint64_t
   parse_debug_string(const char *debug,
         {
               if (debug != NULL) {
      for (; control->string != NULL; control++) {
                     } else {
                     for (; n = strcspn(s, ", "), *s; s += MAX2(1, n)) {
      if (strlen(control->string) == n &&
      !strncmp(control->string, s, n))
                        }
         uint64_t
   parse_enable_string(const char *debug,
               {
               if (debug != NULL) {
      for (; control->string != NULL; control++) {
                     } else {
                     for (; n = strcspn(s, ", "), *s; s += MAX2(1, n)) {
      bool enable;
   if (s[0] == '+') {
      enable = true;
      } else if (s[0] == '-') {
      enable = false;
      } else {
         }
   if (strlen(control->string) == n &&
      !strncmp(control->string, s, n)) {
   if (enable)
         else
                              }
         bool
   comma_separated_list_contains(const char *list, const char *s)
   {
      assert(list);
            for (unsigned n; n = strcspn(list, ","), *list; list += MAX2(1, n)) {
      if (n == len && !strncmp(list, s, n))
                  }
