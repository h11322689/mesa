   //
   // Copyright 2020 Serge Martin
   //
   // Permission is hereby granted, free of charge, to any person obtaining a
   // copy of this software and associated documentation files (the "Software"),
   // to deal in the Software without restriction, including without limitation
   // the rights to use, copy, modify, merge, publish, distribute, sublicense,
   // and/or sell copies of the Software, and to permit persons to whom the
   // Software is furnished to do so, subject to the following conditions:
   //
   // The above copyright notice and this permission notice shall be included in
   // all copies or substantial portions of the Software.
   //
   // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   // THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   // OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   // ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   // OTHER DEALINGS IN THE SOFTWARE.
   //
   // Extract from Serge's printf clover code by airlied.
      #include <assert.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdlib.h>
   #include <string.h>
      #include "macros.h"
   #include "strndup.h"
   #include "u_math.h"
   #include "u_printf.h"
      /* Some versions of MinGW are missing _vscprintf's declaration, although they
   * still provide the symbol in the import library. */
   #ifdef __MINGW32__
   _CRTIMP int _vscprintf(const char *format, va_list argptr);
   #endif
      static const char*
   util_printf_prev_tok(const char *str)
   {
      while (*str != '%')
            }
      size_t util_printf_next_spec_pos(const char *str, size_t pos)
   {
      if (str == NULL)
            const char *str_found = str + pos;
   do {
      str_found = strchr(str_found, '%');
   if (str_found == NULL)
            ++str_found;
   if (*str_found == '%') {
      ++str_found;
               char *spec_pos = strpbrk(str_found, "cdieEfFgGaAosuxXp%");
   if (spec_pos == NULL) {
         } else if (*spec_pos == '%') {
         } else {
               }
      size_t u_printf_length(const char *fmt, va_list untouched_args)
   {
      int size;
            /* Make a copy of the va_list so the original caller can still use it */
   va_list args;
         #ifdef _WIN32
      /* We need to use _vcsprintf to calculate the size as vsnprintf returns -1
   * if the number of characters to write is greater than count.
   */
   size = _vscprintf(fmt, args);
      #else
         #endif
                           }
      void u_printf(FILE *out, const char *buffer, size_t buffer_size,
         {
      for (size_t buf_pos = 0; buf_pos < buffer_size;) {
               /* the idx is 1 based */
   assert(fmt_idx > 0);
            /* The API allows more arguments than the format uses */
   if (fmt_idx >= info_size)
            const u_printf_info *fmt = &info[fmt_idx];
   const char *format = fmt->strings;
            if (!fmt->num_args) {
      fprintf(out, "%s", format);
               for (int i = 0; i < fmt->num_args; i++) {
                     if (spec_pos == -1) {
      fprintf(out, "%s", format);
                              /* print the part before the format token */
                  char *print_str = strndup(token, next_format - token);
                  /* print the formated part */
   if (print_str[spec_pos] == 's') {
      uint64_t idx;
               /* Never pass a 'n' spec to the host printf */
   } else if (print_str[spec_pos] != 'n') {
                     int component_count = 1;
   if (vec_pos != NULL) {
      /* non vector part of the format */
   size_t base = mod_pos ? mod_pos - print_str : spec_pos;
   size_t l = base - (vec_pos - print_str) - 1;
                                          /* in fact vec3 are vec4 */
   int men_components = component_count == 3 ? 4 : component_count;
                  for (int i = 0; i < component_count; i++) {
      size_t elmt_buf_pos = buf_pos + i * elmt_size;
   switch (elmt_size) {
   case 1: {
      uint8_t v;
   memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
   fprintf(out, print_str, v);
      }
   case 2: {
      uint16_t v;
   memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
   fprintf(out, print_str, v);
      }
   case 4: {
      if (is_float) {
      float v;
   memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
      } else {
      uint32_t v;
   memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
      }
      }
   case 8: {
      if (is_float) {
      double v;
   memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
      } else {
      uint64_t v;
   memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
      }
      }
   default:
                        if (i < component_count - 1)
                  /* rebase format */
                  buf_pos += arg_size;
               /* print remaining */
         }
