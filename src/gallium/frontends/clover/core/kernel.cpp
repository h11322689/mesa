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
      #include "core/kernel.hpp"
   #include "core/resource.hpp"
   #include "util/factor.hpp"
   #include "util/u_math.h"
   #include "pipe/p_context.h"
      using namespace clover;
      kernel::kernel(clover::program &prog, const std::string &name,
            program(prog), _name(name), exec(*this),
   program_ref(prog._kernel_ref_counter) {
   for (auto &barg : bargs) {
      if (barg.semantic == binary::argument::general)
      }
   for (auto &dev : prog.devices()) {
      auto &b = prog.build(dev).bin;
   auto bsym = find(name_equals(name), b.syms);
   const auto f = id_type_equals(bsym.section, binary::section::data_constant);
   if (!any_of(f, b.secs))
            auto mconst = find(f, b.secs);
   auto rb = std::make_unique<root_buffer>(prog.context(), std::vector<cl_mem_properties>(),
                     }
      template<typename V>
   static inline std::vector<uint>
   pad_vector(command_queue &q, const V &v, uint x) {
      std::vector<uint> w { v.begin(), v.end() };
   w.resize(q.device().max_block_size().size(), x);
      }
      void
   kernel::launch(command_queue &q,
                  const std::vector<size_t> &grid_offset,
   const auto b = program().build(q.device()).bin;
   const auto reduced_grid_size =
            if (any_of(is_zero(), grid_size))
            void *st = exec.bind(&q, grid_offset);
            // The handles are created during exec_context::bind(), so we need make
   // sure to call exec_context::bind() before retrieving them.
   std::vector<uint32_t *> g_handles = map([&](size_t h) {
                  q.pipe->bind_compute_state(q.pipe, st);
   q.pipe->bind_sampler_states(q.pipe, PIPE_SHADER_COMPUTE,
                  q.pipe->set_sampler_views(q.pipe, PIPE_SHADER_COMPUTE, 0,
         q.pipe->set_shader_images(q.pipe, PIPE_SHADER_COMPUTE, 0,
         q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(),
         q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(),
            // Fill information for the launch_grid() call.
   info.work_dim = grid_size.size();
   copy(pad_vector(q, block_size, 1), info.block);
   copy(pad_vector(q, reduced_grid_size, 1), info.grid);
   info.pc = find(name_equals(_name), b.syms).offset;
   info.input = exec.input.data();
                     q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(), NULL, NULL);
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(), NULL);
   q.pipe->set_shader_images(q.pipe, PIPE_SHADER_COMPUTE, 0,
         q.pipe->set_sampler_views(q.pipe, PIPE_SHADER_COMPUTE, 0,
         q.pipe->bind_sampler_states(q.pipe, PIPE_SHADER_COMPUTE, 0,
            q.pipe->memory_barrier(q.pipe, PIPE_BARRIER_GLOBAL_BUFFER);
      }
      size_t
   kernel::mem_local() const {
               for (auto &arg : args()) {
      if (dynamic_cast<local_argument *>(&arg))
                  }
      size_t
   kernel::mem_private() const {
         }
      const std::string &
   kernel::name() const {
         }
      std::vector<size_t>
   kernel::optimal_block_size(const command_queue &q,
            if (any_of(is_zero(), grid_size))
            return factor::find_grid_optimal_factor<size_t>(
      q.device().max_threads_per_block(), q.device().max_block_size(),
   }
      std::vector<size_t>
   kernel::required_block_size() const {
         }
      kernel::argument_range
   kernel::args() {
         }
      kernel::const_argument_range
   kernel::args() const {
         }
      std::vector<clover::binary::arg_info>
   kernel::args_infos() {
      std::vector<clover::binary::arg_info> infos;
   for (auto &barg: find(name_equals(_name), program().symbols()).args)
      if (barg.semantic == clover::binary::argument::general)
            }
      const binary &
   kernel::binary(const command_queue &q) const {
         }
      kernel::exec_context::exec_context(kernel &kern) :
         }
      kernel::exec_context::~exec_context() {
      if (st)
      }
      void *
   kernel::exec_context::bind(intrusive_ptr<command_queue> _q,
                     // Bind kernel arguments.
   auto &b = kern.program().build(q->device()).bin;
   auto bsym = find(name_equals(kern.name()), b.syms);
   auto bargs = bsym.args;
   auto msec = find(id_type_equals(bsym.section, binary::section::text_executable), b.secs);
            for (auto &barg : bargs) {
      switch (barg.semantic) {
   case binary::argument::general:
                  case binary::argument::grid_dimension: {
                     arg->set(sizeof(dimension), &dimension);
   arg->bind(*this, barg);
      }
   case binary::argument::grid_offset: {
                        arg->set(sizeof(x), &x);
      }
      }
   case binary::argument::image_size: {
      auto img = dynamic_cast<image_argument &>(**(explicit_arg - 1)).get();
   std::vector<cl_uint> image_size{
         static_cast<cl_uint>(img->width()),
   static_cast<cl_uint>(img->height()),
                     arg->set(sizeof(x), &x);
      }
      }
   case binary::argument::image_format: {
      auto img = dynamic_cast<image_argument &>(**(explicit_arg - 1)).get();
   cl_image_format fmt = img->format();
   std::vector<cl_uint> image_format{
         static_cast<cl_uint>(fmt.image_channel_data_type),
                     arg->set(sizeof(x), &x);
      }
      }
   case binary::argument::constant_buffer: {
      auto arg = argument::create(barg);
   cl_mem buf = kern._constant_buffers.at(&q->device()).get();
   arg->set(sizeof(buf), &buf);
   arg->bind(*this, barg);
      }
   case binary::argument::printf_buffer: {
      print_handler = printf_handler::create(q, b.printf_infos,
                        auto arg = argument::create(barg);
   arg->set(sizeof(cl_mem), &print_mem);
   arg->bind(*this, barg);
      }
               // Create a new compute state if anything changed.
   if (!st || q != _q ||
      cs.req_input_mem != input.size()) {
   if (st)
            cs.ir_type = q->device().ir_format();
   cs.prog = &(msec.data[0]);
   // we only pass in NIRs or LLVMs and both IRs decode the size
   cs.static_shared_mem = 0;
   cs.req_input_mem = input.size();
   st = q->pipe->create_compute_state(q->pipe, &cs);
   if (!st) {
      unbind(); // Cleanup
                     }
      void
   kernel::exec_context::unbind() {
      if (print_handler)
            for (auto &arg : kern.args())
            input.clear();
   samplers.clear();
   sviews.clear();
   iviews.clear();
   resources.clear();
   g_buffers.clear();
   g_handles.clear();
      }
      namespace {
      template<typename T>
   std::vector<uint8_t>
   bytes(const T& x) {
                  ///
   /// Transform buffer \a v from the native byte order into the byte
   /// order specified by \a e.
   ///
   template<typename T>
   void
   byteswap(T &v, pipe_endian e) {
      if (PIPE_ENDIAN_NATIVE != e)
               ///
   /// Pad buffer \a v to the next multiple of \a n.
   ///
   template<typename T>
   void
   align(T &v, size_t n) {
                  bool
   msb(const std::vector<uint8_t> &s) {
      if (PIPE_ENDIAN_NATIVE == PIPE_ENDIAN_LITTLE)
         else
               ///
   /// Resize buffer \a v to size \a n using sign or zero extension
   /// according to \a ext.
   ///
   template<typename T>
   void
   extend(T &v, enum binary::argument::ext_type ext, size_t n) {
      const size_t m = std::min(v.size(), n);
   const bool sign_ext = (ext == binary::argument::sign_ext);
   const uint8_t fill = (sign_ext && msb(v) ? ~0 : 0);
            if (PIPE_ENDIAN_NATIVE == PIPE_ENDIAN_LITTLE)
         else
                        ///
   /// Append buffer \a w to \a v.
   ///
   template<typename T>
   void
   insert(T &v, const T &w) {
                  ///
   /// Append \a n elements to the end of buffer \a v.
   ///
   template<typename T>
   size_t
   allocate(T &v, size_t n) {
      size_t pos = v.size();
   v.resize(pos + n);
         }
      std::unique_ptr<kernel::argument>
   kernel::argument::create(const binary::argument &barg) {
      switch (barg.type) {
   case binary::argument::scalar:
            case binary::argument::global:
            case binary::argument::local:
            case binary::argument::constant:
            case binary::argument::image_rd:
            case binary::argument::image_wr:
            case binary::argument::sampler:
            }
      }
      kernel::argument::argument() : _set(false) {
   }
      bool
   kernel::argument::set() const {
         }
      size_t
   kernel::argument::storage() const {
         }
      kernel::scalar_argument::scalar_argument(size_t size) : size(size) {
   }
      void
   kernel::scalar_argument::set(size_t size, const void *value) {
      if (!value)
            if (size != this->size)
            v = { (uint8_t *)value, (uint8_t *)value + size };
      }
      void
   kernel::scalar_argument::bind(exec_context &ctx,
                     extend(w, barg.ext_type, barg.target_size);
   byteswap(w, ctx.q->device().endianness());
   align(ctx.input, barg.target_align);
      }
      void
   kernel::scalar_argument::unbind(exec_context &ctx) {
   }
      kernel::global_argument::global_argument() : buf(nullptr), svm(nullptr) {
   }
      void
   kernel::global_argument::set(size_t size, const void *value) {
      if (size != sizeof(cl_mem))
            buf = pobj<buffer>(value ? *(cl_mem *)value : NULL);
   svm = nullptr;
      }
      void
   kernel::global_argument::set_svm(const void *value) {
      svm = value;
   buf = nullptr;
      }
      void
   kernel::global_argument::bind(exec_context &ctx,
                     if (buf) {
      const resource &r = buf->resource_in(*ctx.q);
   ctx.g_handles.push_back(ctx.input.size());
            // How to handle multi-demensional offsets?
   // We don't need to.  Buffer offsets are always
   // one-dimensional.
   auto v = bytes(r.offset[0]);
   extend(v, barg.ext_type, barg.target_size);
   byteswap(v, ctx.q->device().endianness());
      } else if (svm) {
      auto v = bytes(svm);
   extend(v, barg.ext_type, barg.target_size);
   byteswap(v, ctx.q->device().endianness());
      } else {
      // Null pointer.
         }
      void
   kernel::global_argument::unbind(exec_context &ctx) {
   }
      size_t
   kernel::local_argument::storage() const {
         }
      void
   kernel::local_argument::set(size_t size, const void *value) {
      if (value)
            if (!size)
            _storage = size;
      }
      void
   kernel::local_argument::bind(exec_context &ctx,
            ctx.mem_local = ::align(ctx.mem_local, barg.target_align);
            extend(v, binary::argument::zero_ext, barg.target_size);
   byteswap(v, ctx.q->device().endianness());
   align(ctx.input, ctx.q->device().address_bits() / 8);
               }
      void
   kernel::local_argument::unbind(exec_context &ctx) {
   }
      kernel::constant_argument::constant_argument() : buf(nullptr), st(nullptr) {
   }
      void
   kernel::constant_argument::set(size_t size, const void *value) {
      if (size != sizeof(cl_mem))
            buf = pobj<buffer>(value ? *(cl_mem *)value : NULL);
      }
      void
   kernel::constant_argument::bind(exec_context &ctx,
                     if (buf) {
      resource &r = buf->resource_in(*ctx.q);
            extend(v, binary::argument::zero_ext, barg.target_size);
   byteswap(v, ctx.q->device().endianness());
            st = r.bind_surface(*ctx.q, false);
      } else {
      // Null pointer.
         }
      void
   kernel::constant_argument::unbind(exec_context &ctx) {
      if (buf)
      }
      kernel::image_rd_argument::image_rd_argument() : st(nullptr) {
   }
      void
   kernel::image_rd_argument::set(size_t size, const void *value) {
      if (!value)
            if (size != sizeof(cl_mem))
            img = &obj<image>(*(cl_mem *)value);
      }
      void
   kernel::image_rd_argument::bind(exec_context &ctx,
                     extend(v, binary::argument::zero_ext, barg.target_size);
   byteswap(v, ctx.q->device().endianness());
   align(ctx.input, barg.target_align);
            st = img->resource_in(*ctx.q).bind_sampler_view(*ctx.q);
      }
      void
   kernel::image_rd_argument::unbind(exec_context &ctx) {
         }
      void
   kernel::image_wr_argument::set(size_t size, const void *value) {
      if (!value)
            if (size != sizeof(cl_mem))
            img = &obj<image>(*(cl_mem *)value);
      }
      void
   kernel::image_wr_argument::bind(exec_context &ctx,
                     extend(v, binary::argument::zero_ext, barg.target_size);
   byteswap(v, ctx.q->device().endianness());
   align(ctx.input, barg.target_align);
   insert(ctx.input, v);
      }
      void
   kernel::image_wr_argument::unbind(exec_context &ctx) {
   }
      kernel::sampler_argument::sampler_argument() : s(nullptr), st(nullptr) {
   }
      void
   kernel::sampler_argument::set(size_t size, const void *value) {
      if (!value)
            if (size != sizeof(cl_sampler))
            s = &obj(*(cl_sampler *)value);
      }
      void
   kernel::sampler_argument::bind(exec_context &ctx,
            st = s->bind(*ctx.q);
      }
      void
   kernel::sampler_argument::unbind(exec_context &ctx) {
         }
