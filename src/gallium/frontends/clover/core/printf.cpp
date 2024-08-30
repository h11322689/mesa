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
      #include <cstring>
   #include <cstdio>
   #include <string>
   #include <iostream>
      #include "util/u_math.h"
   #include "core/printf.hpp"
      #include "util/u_printf.h"
   using namespace clover;
      namespace {
         const cl_uint hdr_dwords = 2;
            /* all valid chars that can appear in CL C printf string. */
            void
   print_formatted(std::vector<binary::printf_info> &formatters,
                              if (buffer.empty() && !warn_count++)
            std::vector<u_printf_info> infos;
   for (auto &f : formatters) {
               info.num_args = f.arg_sizes.size();
   info.arg_sizes = f.arg_sizes.data();
                                    }
      std::unique_ptr<printf_handler>
   printf_handler::create(const intrusive_ptr<command_queue> &q,
                        return std::unique_ptr<printf_handler>(
      }
      printf_handler::printf_handler(const intrusive_ptr<command_queue> &q,
                                 if (_size) {
      std::string data;
   data.reserve(_size);
            header[0] = initial_buffer_offset;
            data.append((char *)header, (char *)(header+hdr_dwords));
   _buffer = std::unique_ptr<root_buffer>(new root_buffer(_q->context,
                     }
      cl_mem
   printf_handler::get_mem() {
         }
      void
   printf_handler::print() {
      if (!_buffer)
            mapping src = { *_q, _buffer->resource_in(*_q), CL_MAP_READ, true,
            cl_uint header[2] = { 0 };
   std::memcpy(header,
                  cl_uint buffer_size = header[0];
   buffer_size -= initial_buffer_offset;
   std::vector<char> buf;
            std::memcpy(buf.data(),
                  // mixed endian isn't going to work, sort it out if anyone cares later.
   assert(_q->device().endianness() == PIPE_ENDIAN_NATIVE);
      }
