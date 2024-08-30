   /* -*- c++ -*- */
   /*
   * Copyright © 2010 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdio.h>
   #include <math.h>
   #include <string.h>
   #include <stdlib.h>
   #include "s_expression.h"
      s_symbol::s_symbol(const char *str, size_t n)
   {
      /* Assume the given string is already nul-terminated and in memory that
   * will live as long as this node.
   */
   assert(str[n] == '\0');
      }
      s_list::s_list()
   {
   }
      static void
   skip_whitespace(const char *&src, char *&symbol_buffer)
   {
      size_t n = strspn(src, " \v\t\r\n");
   src += n;
   symbol_buffer += n;
   /* Also skip Scheme-style comments: semi-colon 'til end of line */
   if (src[0] == ';') {
      n = strcspn(src, "\n");
   src += n;
   symbol_buffer += n;
         }
      static s_expression *
   read_atom(void *ctx, const char *&src, char *&symbol_buffer)
   {
                        size_t n = strcspn(src, "( \v\t\r\n);");
   if (n == 0)
            // Check for the special symbol '+INF', which means +Infinity.  Note: C99
   // requires strtof to parse '+INF' as +Infinity, but we still support some
   // non-C99-compliant compilers (e.g. MSVC).
   if (n == 4 && strncmp(src, "+INF", 4) == 0) {
         } else {
      // Check if the atom is a number.
   char *float_end = NULL;
   float f = _mesa_strtof(src, &float_end);
   if (float_end != src) {
      char *int_end = NULL;
   int i = strtol(src, &int_end, 10);
   // If strtof matched more characters, it must have a decimal part
   if (float_end > int_end)
         else
      } else {
      // Not a number; return a symbol.
   symbol_buffer[n] = '\0';
                  src += n;
               }
      static s_expression *
   __read_expression(void *ctx, const char *&src, char *&symbol_buffer)
   {
      s_expression *atom = read_atom(ctx, src, symbol_buffer);
   if (atom != NULL)
            skip_whitespace(src, symbol_buffer);
   if (src[0] == '(') {
      ++src;
            s_list *list = new(ctx) s_list;
            list->subexpressions.push_tail(expr);
         }
   skip_whitespace(src, symbol_buffer);
   printf("Unclosed expression (check your parenthesis).\n");
   return NULL;
         }
   ++src;
   ++symbol_buffer;
      }
      }
      s_expression *
   s_expression::read_expression(void *ctx, const char *&src)
   {
               /* When we encounter a Symbol, we need to save a nul-terminated copy of
   * the string.  However, ralloc_strndup'ing every individual Symbol is
   * extremely expensive.  We could avoid this by simply overwriting the
   * next character (guaranteed to be whitespace, parens, or semicolon) with
   * a nul-byte.  But overwriting non-whitespace would mess up parsing.
   *
   * So, just copy the whole buffer ahead of time.  Walk both, leaving the
   * original source string unmodified, and altering the copy to contain the
   * necessary nul-bytes whenever we encounter a symbol.
   */
   char *symbol_buffer = ralloc_strdup(ctx, src);
      }
      void s_int::print()
   {
         }
      void s_float::print()
   {
         }
      void s_symbol::print()
   {
         }
      void s_list::print()
   {
      printf("(");
   foreach_in_list(s_expression, expr, &this->subexpressions) {
      expr->print();
   printf(" ");
      }
      }
      // --------------------------------------------------
      bool
   s_pattern::match(s_expression *expr)
   {
      switch (type)
   {
   case EXPR:   *p_expr = expr; break;
   case LIST:   if (expr->is_list())   *p_list   = (s_list *)   expr; break;
   case SYMBOL: if (expr->is_symbol()) *p_symbol = (s_symbol *) expr; break;
   case NUMBER: if (expr->is_number()) *p_number = (s_number *) expr; break;
   case INT:    if (expr->is_int())    *p_int    = (s_int *)    expr; break;
   case STRING:
      s_symbol *sym = SX_AS_SYMBOL(expr);
   return true;
                        }
      bool
   s_match(s_expression *top, unsigned n, s_pattern *pattern, bool partial)
   {
      s_list *list = SX_AS_LIST(top);
   if (list == NULL)
            unsigned i = 0;
   foreach_in_list(s_expression, expr, &list->subexpressions) {
      return partial; /* More actual items than the pattern expected */
            return false;
                        if (i < n)
               }
