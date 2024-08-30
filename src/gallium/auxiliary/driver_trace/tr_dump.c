   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
         /**
   * @file
   * Trace dumping functions.
   *
   * For now we just use standard XML for dumping the trace calls, as this is
   * simple to write, parse, and visually inspect, but the actual representation
   * is abstracted out of this file, so that we can switch to a binary
   * representation if/when it becomes justified.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include "util/detect.h"
      #include <stdio.h>
   #include <stdlib.h>
      /* for access() */
   #ifdef _WIN32
   # include <io.h>
   #endif
      #include "util/compiler.h"
   #include "util/u_thread.h"
   #include "util/os_time.h"
   #include "util/simple_mtx.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "compiler/nir/nir.h"
      #include "tr_dump.h"
   #include "tr_screen.h"
   #include "tr_texture.h"
         static bool close_stream = false;
   static FILE *stream = NULL;
   static simple_mtx_t call_mutex = SIMPLE_MTX_INITIALIZER;
   static long unsigned call_no = 0;
   static bool dumping = false;
   static long nir_count = 0;
      static bool trigger_active = true;
   static char *trigger_filename = NULL;
      void
   trace_dump_trigger_active(bool active)
   {
         }
      void
   trace_dump_check_trigger(void)
   {
      if (!trigger_filename)
            simple_mtx_lock(&call_mutex);
   if (trigger_active) {
         } else {
      if (!access(trigger_filename, 2 /* W_OK but compiles on Windows */)) {
      if (!unlink(trigger_filename)) {
         } else {
      fprintf(stderr, "error removing trigger file\n");
            }
      }
      bool
   trace_dump_is_triggered(void)
   {
         }
      static inline void
   trace_dump_write(const char *buf, size_t size)
   {
      if (stream && trigger_active) {
            }
         static inline void
   trace_dump_writes(const char *s)
   {
         }
         static inline void
   trace_dump_writef(const char *format, ...)
   {
      static char buf[1024];
   unsigned len;
   va_list ap;
   va_start(ap, format);
   len = vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);
      }
         static inline void
   trace_dump_escape(const char *str)
   {
      const unsigned char *p = (const unsigned char *)str;
   unsigned char c;
   while((c = *p++) != 0) {
      if(c == '<')
         else if(c == '>')
         else if(c == '&')
         else if(c == '\'')
         else if(c == '\"')
         else if(c >= 0x20 && c <= 0x7e)
         else
         }
         static inline void
   trace_dump_indent(unsigned level)
   {
      unsigned i;
   for(i = 0; i < level; ++i)
      }
         static inline void
   trace_dump_newline(void)
   {
         }
         static inline void
   trace_dump_tag_begin(const char *name)
   {
      trace_dump_writes("<");
   trace_dump_writes(name);
      }
      static inline void
   trace_dump_tag_begin1(const char *name,
         {
      trace_dump_writes("<");
   trace_dump_writes(name);
   trace_dump_writes(" ");
   trace_dump_writes(attr1);
   trace_dump_writes("='");
   trace_dump_escape(value1);
      }
         static inline void
   trace_dump_tag_end(const char *name)
   {
      trace_dump_writes("</");
   trace_dump_writes(name);
      }
      void
   trace_dump_trace_flush(void)
   {
      if (stream) {
            }
      static void
   trace_dump_trace_close(void)
   {
      if (stream) {
      trigger_active = true;
   trace_dump_writes("</trace>\n");
   if (close_stream) {
      fclose(stream);
   close_stream = false;
      }
   call_no = 0;
         }
         static void
   trace_dump_call_time(int64_t time)
   {
      if (stream) {
      trace_dump_indent(2);
   trace_dump_tag_begin("time");
   trace_dump_int(time);
   trace_dump_tag_end("time");
         }
         bool
   trace_dump_trace_begin(void)
   {
               filename = debug_get_option("GALLIUM_TRACE", NULL);
   if (!filename)
                                 if (strcmp(filename, "stderr") == 0) {
      close_stream = false;
      }
   else if (strcmp(filename, "stdout") == 0) {
      close_stream = false;
      }
   else {
      close_stream = true;
   stream = fopen(filename, "wt");
   if (!stream)
               trace_dump_writes("<?xml version='1.0' encoding='UTF-8'?>\n");
   trace_dump_writes("<?xml-stylesheet type='text/xsl' href='trace.xsl'?>\n");
            /* Many applications don't exit cleanly, others may create and destroy a
   * screen multiple times, so we only write </trace> tag and close at exit
   * time.
   */
            const char *trigger = debug_get_option("GALLIUM_TRACE_TRIGGER", NULL);
   if (trigger && __normal_user()) {
      trigger_filename = strdup(trigger);
      } else
                  }
      bool trace_dump_trace_enabled(void)
   {
         }
      /*
   * Call lock
   */
      void trace_dump_call_lock(void)
   {
         }
      void trace_dump_call_unlock(void)
   {
         }
      /*
   * Dumping control
   */
      void trace_dumping_start_locked(void)
   {
         }
      void trace_dumping_stop_locked(void)
   {
         }
      bool trace_dumping_enabled_locked(void)
   {
         }
      void trace_dumping_start(void)
   {
      simple_mtx_lock(&call_mutex);
   trace_dumping_start_locked();
      }
      void trace_dumping_stop(void)
   {
      simple_mtx_lock(&call_mutex);
   trace_dumping_stop_locked();
      }
      bool trace_dumping_enabled(void)
   {
      bool ret;
   simple_mtx_lock(&call_mutex);
   ret = trace_dumping_enabled_locked();
   simple_mtx_unlock(&call_mutex);
      }
      /*
   * Dump functions
   */
      static int64_t call_start_time = 0;
      void trace_dump_call_begin_locked(const char *klass, const char *method)
   {
      if (!dumping)
            ++call_no;
   trace_dump_indent(1);
   trace_dump_writes("<call no=\'");
   trace_dump_writef("%lu", call_no);
   trace_dump_writes("\' class=\'");
   trace_dump_escape(klass);
   trace_dump_writes("\' method=\'");
   trace_dump_escape(method);
   trace_dump_writes("\'>");
               }
      void trace_dump_call_end_locked(void)
   {
               if (!dumping)
                     trace_dump_call_time(call_end_time - call_start_time);
   trace_dump_indent(1);
   trace_dump_tag_end("call");
   trace_dump_newline();
      }
      void trace_dump_call_begin(const char *klass, const char *method)
   {
      simple_mtx_lock(&call_mutex);
      }
      void trace_dump_call_end(void)
   {
      trace_dump_call_end_locked();
      }
      void trace_dump_arg_begin(const char *name)
   {
      if (!dumping)
            trace_dump_indent(2);
      }
      void trace_dump_arg_end(void)
   {
      if (!dumping)
            trace_dump_tag_end("arg");
      }
      void trace_dump_ret_begin(void)
   {
      if (!dumping)
            trace_dump_indent(2);
      }
      void trace_dump_ret_end(void)
   {
      if (!dumping)
            trace_dump_tag_end("ret");
      }
      void trace_dump_bool(bool value)
   {
      if (!dumping)
               }
      void trace_dump_int(int64_t value)
   {
      if (!dumping)
               }
      void trace_dump_uint(uint64_t value)
   {
      if (!dumping)
               }
      void trace_dump_float(double value)
   {
      if (!dumping)
               }
      void trace_dump_bytes(const void *data,
         {
      static const char hex_table[16] = "0123456789ABCDEF";
   const uint8_t *p = data;
            if (!dumping)
            trace_dump_writes("<bytes>");
   for(i = 0; i < size; ++i) {
      uint8_t byte = *p++;
   char hex[2];
   hex[0] = hex_table[byte >> 4];
   hex[1] = hex_table[byte & 0xf];
      }
      }
      void trace_dump_box_bytes(const void *data,
            const struct pipe_box *box,
   unsigned stride,
      {
      enum pipe_format format = resource->format;
            assert(box->height > 0);
            size = util_format_get_nblocksx(format, box->width ) *
         (uint64_t)util_format_get_blocksize(format) +
            /*
   * Only dump buffer transfers to avoid huge files.
   * TODO: Make this run-time configurable
   */
   if (resource->target != PIPE_BUFFER) {
                  assert(size <= SIZE_MAX);
      }
      void trace_dump_string(const char *str)
   {
      if (!dumping)
            trace_dump_writes("<string>");
   trace_dump_escape(str);
      }
      void trace_dump_enum(const char *value)
   {
      if (!dumping)
            trace_dump_writes("<enum>");
   trace_dump_escape(value);
      }
      void trace_dump_array_begin(void)
   {
      if (!dumping)
               }
      void trace_dump_array_end(void)
   {
      if (!dumping)
               }
      void trace_dump_elem_begin(void)
   {
      if (!dumping)
               }
      void trace_dump_elem_end(void)
   {
      if (!dumping)
               }
      void trace_dump_struct_begin(const char *name)
   {
      if (!dumping)
               }
      void trace_dump_struct_end(void)
   {
      if (!dumping)
               }
      void trace_dump_member_begin(const char *name)
   {
      if (!dumping)
               }
      void trace_dump_member_end(void)
   {
      if (!dumping)
               }
      void trace_dump_null(void)
   {
      if (!dumping)
               }
      void trace_dump_ptr(const void *value)
   {
      if (!dumping)
            if(value)
         else
      }
      void trace_dump_surface_ptr(struct pipe_surface *_surface)
   {
      if (!dumping)
            if (_surface) {
      struct trace_surface *tr_surf = trace_surface(_surface);
      } else {
            }
      void trace_dump_transfer_ptr(struct pipe_transfer *_transfer)
   {
      if (!dumping)
            if (_transfer) {
      struct trace_transfer *tr_tran = trace_transfer(_transfer);
      } else {
            }
      void trace_dump_nir(void *nir)
   {
      if (!dumping)
            if (--nir_count < 0) {
      fputs("<string>...</string>", stream);
               // NIR doesn't have a print to string function.  Use CDATA and hope for the
   // best.
   if (stream) {
      fputs("<string><![CDATA[", stream);
   nir_print_shader(nir, stream);
         }
