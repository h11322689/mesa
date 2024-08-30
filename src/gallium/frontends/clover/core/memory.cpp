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
      #include "core/memory.hpp"
   #include "core/resource.hpp"
   #include "util/format/u_format.h"
      using namespace clover;
      memory_obj::memory_obj(clover::context &ctx,
                        context(ctx), _properties(properties), _flags(flags),
   _size(size), _host_ptr(host_ptr) {
   if (flags & CL_MEM_COPY_HOST_PTR)
      }
      memory_obj::~memory_obj() {
      while (_destroy_notify.size()) {
      _destroy_notify.top()();
         }
      bool
   memory_obj::operator==(const memory_obj &obj) const {
         }
      void
   memory_obj::destroy_notify(std::function<void ()> f) {
         }
      std::vector<cl_mem_properties>
   memory_obj::properties() const {
         }
      cl_mem_flags
   memory_obj::flags() const {
         }
      size_t
   memory_obj::size() const {
         }
      void *
   memory_obj::host_ptr() const {
         }
      buffer::buffer(clover::context &ctx,
                  std::vector<cl_mem_properties> properties,
      }
      cl_mem_object_type
   buffer::type() const {
         }
      root_buffer::root_buffer(clover::context &ctx,
                           }
      resource &
   root_buffer::resource_in(command_queue &q) {
      const void *data_ptr = NULL;
   if (flags() & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))
               }
      resource &
   root_buffer::resource_undef(command_queue &q) {
         }
      resource &
   root_buffer::resource(command_queue &q, const void *data_ptr) {
      std::lock_guard<std::mutex> lock(resources_mtx);
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q.device())) {
      auto r = (!resources.empty() ?
                        resources.insert(std::make_pair(&q.device(),
                        }
      void
   root_buffer::resource_out(command_queue &q) {
      std::lock_guard<std::mutex> lock(resources_mtx);
      }
      sub_buffer::sub_buffer(root_buffer &parent, cl_mem_flags flags,
            buffer(parent.context(), std::vector<cl_mem_properties>(), flags, size,
            }
      resource &
   sub_buffer::resource_in(command_queue &q) {
      std::lock_guard<std::mutex> lock(resources_mtx);
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q.device())) {
               resources.insert(std::make_pair(&q.device(),
                  }
      resource &
   sub_buffer::resource_undef(command_queue &q) {
         }
      void
   sub_buffer::resource_out(command_queue &q) {
      std::lock_guard<std::mutex> lock(resources_mtx);
      }
      size_t
   sub_buffer::offset() const {
         }
      image::image(clover::context &ctx,
               std::vector<cl_mem_properties> properties,
   cl_mem_flags flags,
   const cl_image_format *format,
   size_t width, size_t height, size_t depth, size_t array_size,
      memory_obj(ctx, properties, flags, size, host_ptr),
   _format(*format), _width(width), _height(height), _depth(depth),
   _row_pitch(row_pitch), _slice_pitch(slice_pitch), _array_size(array_size),
      }
      resource &
   image::resource_in(command_queue &q) {
      const void *data_ptr = !data.empty() ? data.data() : NULL;
      }
      resource &
   image::resource_undef(command_queue &q) {
         }
      resource &
   image::resource(command_queue &q, const void *data_ptr) {
      std::lock_guard<std::mutex> lock(resources_mtx);
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q.device())) {
      auto r = (!resources.empty() ?
                        resources.insert(std::make_pair(&q.device(),
                        }
      void
   image::resource_out(command_queue &q) {
      std::lock_guard<std::mutex> lock(resources_mtx);
      }
      cl_image_format
   image::format() const {
         }
      size_t
   image::width() const {
         }
      size_t
   image::height() const {
         }
      size_t
   image::depth() const {
         }
      size_t
   image::pixel_size() const {
         }
      size_t
   image::row_pitch() const {
         }
      size_t
   image::slice_pitch() const {
         }
      size_t
   image::array_size() const {
         }
      cl_mem
   image::buffer() const {
         }
      image1d::image1d(clover::context &ctx,
                  std::vector<cl_mem_properties> properties,
   cl_mem_flags flags,
   const cl_image_format *format,
   basic_image(ctx, properties, flags, format, width, 1, 1, 0,
      }
      image1d_buffer::image1d_buffer(clover::context &ctx,
                                    basic_image(ctx, properties, flags, format, width, 1, 1, 0,
      }
      image1d_array::image1d_array(clover::context &ctx,
                              std::vector<cl_mem_properties> properties,
   cl_mem_flags flags,
   basic_image(ctx, properties, flags, format, width, 1, 1, array_size,
      }
      image2d::image2d(clover::context &ctx,
                  std::vector<cl_mem_properties> properties,
   cl_mem_flags flags,
   const cl_image_format *format, size_t width,
   basic_image(ctx, properties, flags, format, width, height, 1, 0,
      }
      image2d_array::image2d_array(clover::context &ctx,
                              std::vector<cl_mem_properties> properties,
   cl_mem_flags flags,
   basic_image(ctx, properties, flags, format, width, height, 1, array_size,
      }
      image3d::image3d(clover::context &ctx,
                  std::vector<cl_mem_properties> properties,
   cl_mem_flags flags,
   const cl_image_format *format,
   size_t width, size_t height, size_t depth,
   basic_image(ctx, properties, flags, format, width, height, depth, 0,
            }
