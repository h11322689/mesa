   //
   // Copyright 2012 Francisco Jerez
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
      #include "core/sampler.hpp"
   #include "pipe/p_state.h"
      using namespace clover;
      sampler::sampler(clover::context &ctx, bool norm_mode,
                  context(ctx), _norm_mode(norm_mode),
      }
      bool
   sampler::norm_mode() {
         }
      cl_addressing_mode
   sampler::addr_mode() {
         }
      cl_filter_mode
   sampler::filter_mode() {
         }
      void *
   sampler::bind(command_queue &q) {
                        info.wrap_s = info.wrap_t = info.wrap_r =
      (addr_mode() == CL_ADDRESS_CLAMP_TO_EDGE ? PIPE_TEX_WRAP_CLAMP_TO_EDGE :
   addr_mode() == CL_ADDRESS_CLAMP ? PIPE_TEX_WRAP_CLAMP_TO_BORDER :
   addr_mode() == CL_ADDRESS_REPEAT ? PIPE_TEX_WRAP_REPEAT :
   addr_mode() == CL_ADDRESS_MIRRORED_REPEAT ? PIPE_TEX_WRAP_MIRROR_REPEAT :
         info.min_img_filter = info.mag_img_filter =
      (filter_mode() == CL_FILTER_LINEAR ? PIPE_TEX_FILTER_LINEAR :
            }
      void
   sampler::unbind(command_queue &q, void *st) {
         }
