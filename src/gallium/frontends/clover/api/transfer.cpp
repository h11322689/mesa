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
      #include <cstring>
      #include "util/bitscan.h"
      #include "api/dispatch.hpp"
   #include "api/util.hpp"
   #include "core/event.hpp"
   #include "core/memory.hpp"
      using namespace clover;
      namespace {
               vector_t
   vector(const size_t *p) {
      if (!p)
                     vector_t
   pitch(const vector_t &region, vector_t pitch) {
      for (auto x : zip(tail(pitch),
            // The spec defines a value of zero as the natural pitch,
   // i.e. the unaligned size of the previous dimension.
   if (std::get<0>(x) == 0)
                           ///
   /// Size of a region in bytes.
   ///
   size_t
   size(const vector_t &pitch, const vector_t &region) {
      if (any_of(is_zero(), region))
         else
               ///
   /// Common argument checking shared by memory transfer commands.
   ///
   void
   validate_common(command_queue &q,
            if (any_of([&](const event &ev) {
                           ///
   /// Common error checking for a buffer object argument.
   ///
   void
   validate_object(command_queue &q, buffer &mem, const vector_t &origin,
            if (mem.context() != q.context())
            // The region must fit within the specified pitch,
   if (any_of(greater(), map(multiplies(), pitch, region), tail(pitch)))
            // ...and within the specified object.
   if (dot(pitch, origin) + size(pitch, region) > mem.size())
            if (any_of(is_zero(), region))
               ///
   /// Common error checking for an image argument.
   ///
   void
   validate_object(command_queue &q, image &img,
            size_t height = img.type() == CL_MEM_OBJECT_IMAGE1D_ARRAY ? img.array_size() : img.height();
   size_t depth = img.type() == CL_MEM_OBJECT_IMAGE2D_ARRAY ? img.array_size() : img.depth();
   vector_t size = { img.width(), height, depth };
            if (!dev.image_support())
            if (img.context() != q.context())
            if (any_of(greater(), orig + region, size))
            if (any_of(is_zero(), region))
            switch (img.type()) {
   case CL_MEM_OBJECT_IMAGE1D: {
      const size_t max = dev.max_image_size();
   if (img.width() > max)
            }
   case CL_MEM_OBJECT_IMAGE1D_ARRAY: {
      const size_t max_size = dev.max_image_size();
   const size_t max_array = dev.max_image_array_number();
   if (img.width() > max_size || img.array_size() > max_array)
            }
   case CL_MEM_OBJECT_IMAGE2D: {
      const size_t max = dev.max_image_size();
   if (img.width() > max || img.height() > max)
            }
   case CL_MEM_OBJECT_IMAGE2D_ARRAY: {
      const size_t max_size = dev.max_image_size();
   const size_t max_array = dev.max_image_array_number();
   if (img.width() > max_size || img.height() > max_size || img.array_size() > max_array)
            }
   case CL_MEM_OBJECT_IMAGE3D: {
      const size_t max = dev.max_image_size_3d();
   if (img.width() > max || img.height() > max || img.depth() > max)
            }
   // XXX: Implement missing checks once Clover supports more image types.
   default:
                     ///
   /// Common error checking for a host pointer argument.
   ///
   void
   validate_object(command_queue &q, const void *ptr, const vector_t &orig,
            if (!ptr)
            // The region must fit within the specified pitch.
   if (any_of(greater(), map(multiplies(), pitch, region), tail(pitch)))
               ///
   /// Common argument checking for a copy between two buffer objects.
   ///
   void
   validate_copy(command_queue &q, buffer &dst_mem,
               const vector_t &dst_orig, const vector_t &dst_pitch,
   buffer &src_mem,
      if (dst_mem == src_mem) {
                     if (interval_overlaps()(
         dst_offset, dst_offset + size(dst_pitch, region),
                  ///
   /// Common argument checking for a copy between two image objects.
   ///
   void
   validate_copy(command_queue &q,
               image &dst_img, const vector_t &dst_orig,
      if (dst_img.format() != src_img.format())
            if (dst_img == src_img) {
      if (all_of(interval_overlaps(),
            dst_orig, dst_orig + region,
               ///
   /// Checks that the host access flags of the memory object are
   /// within the allowed set \a flags.
   ///
   void
   validate_object_access(const memory_obj &mem, const cl_mem_flags flags) {
      if (mem.flags() & ~flags &
      (CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_WRITE_ONLY |
   CL_MEM_HOST_NO_ACCESS))
            ///
   /// Checks that the mapping flags are correct.
   ///
   void
   validate_map_flags(const memory_obj &mem, const cl_map_flags flags) {
      if ((flags & (CL_MAP_WRITE | CL_MAP_READ)) &&
                  if (flags & CL_MAP_READ)
            if (flags & (CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION))
               ///
   /// Checks that the memory migration flags are correct.
   ///
   void
   validate_mem_migration_flags(const cl_mem_migration_flags flags) {
      const cl_mem_migration_flags valid =
                  if (flags & ~valid)
               ///
   /// Class that encapsulates the task of mapping an object of type
   /// \a T.  The return value of get() should be implicitly
   /// convertible to \a void *.
   ///
   template<typename T>
            template<>
   struct _map<image*> {
      _map(command_queue &q, image *img, cl_map_flags flags,
      vector_t offset, vector_t pitch, vector_t region) :
   map(q, img->resource_in(q), flags, true, offset, region),
               template<typename T>
   operator T *() const {
                  mapping map;
               template<>
   struct _map<buffer*> {
      _map(command_queue &q, buffer *mem, cl_map_flags flags,
      vector_t offset, vector_t pitch, vector_t region) :
   map(q, mem->resource_in(q), flags, true,
                     template<typename T>
   operator T *() const {
                  mapping map;
               template<typename P>
   struct _map<P *> {
      _map(command_queue &q, P *ptr, cl_map_flags flags,
      vector_t offset, vector_t pitch, vector_t region) :
               template<typename T>
   operator T *() const {
                  P *ptr;
               ///
   /// Software copy from \a src_obj to \a dst_obj.  They can be
   /// either pointers or memory objects.
   ///
   template<typename T, typename S>
   std::function<void (event &)>
   soft_copy_op(command_queue &q,
               T dst_obj, const vector_t &dst_orig, const vector_t &dst_pitch,
      return [=, &q](event &) {
      _map<T> dst = { q, dst_obj, CL_MAP_WRITE,
         _map<S> src = { q, src_obj, CL_MAP_READ,
                        for (v[2] = 0; v[2] < region[2]; ++v[2]) {
      for (v[1] = 0; v[1] < region[1]; ++v[1]) {
      std::memcpy(
      static_cast<char *>(dst) + dot(dst.pitch, v),
   static_cast<const char *>(src) + dot(src.pitch, v),
                     ///
   /// Hardware copy from \a src_obj to \a dst_obj.
   ///
   template<typename T, typename S>
   std::function<void (event &)>
   hard_copy_op(command_queue &q, T dst_obj, const vector_t &dst_orig,
            return [=, &q](event &) {
      dst_obj->resource_in(q).copy(q, dst_orig, region,
            }
      CLOVER_API cl_int
   clEnqueueReadBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                        auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t obj_origin = { offset };
            validate_common(q, deps);
   validate_object(q, ptr, {}, obj_pitch, region);
   validate_object(q, mem, obj_origin, obj_pitch, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_READ_BUFFER, deps,
   soft_copy_op(q, ptr, {}, obj_pitch,
               if (blocking)
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueWriteBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                        auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t obj_origin = { offset };
            validate_common(q, deps);
   validate_object(q, mem, obj_origin, obj_pitch, region);
   validate_object(q, ptr, {}, obj_pitch, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_WRITE_BUFFER, deps,
   soft_copy_op(q, &mem, obj_origin, obj_pitch,
               if (blocking)
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueReadBufferRect(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                           const size_t *p_obj_origin,
   const size_t *p_host_origin,
   const size_t *p_region,
   size_t obj_row_pitch, size_t obj_slice_pitch,
      auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto obj_origin = vector(p_obj_origin);
   auto obj_pitch = pitch(region, {{ 1, obj_row_pitch, obj_slice_pitch }});
   auto host_origin = vector(p_host_origin);
            validate_common(q, deps);
   validate_object(q, ptr, host_origin, host_pitch, region);
   validate_object(q, mem, obj_origin, obj_pitch, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_READ_BUFFER_RECT, deps,
   soft_copy_op(q, ptr, host_origin, host_pitch,
               if (blocking)
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueWriteBufferRect(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                           const size_t *p_obj_origin,
   const size_t *p_host_origin,
   const size_t *p_region,
   size_t obj_row_pitch, size_t obj_slice_pitch,
      auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto obj_origin = vector(p_obj_origin);
   auto obj_pitch = pitch(region, {{ 1, obj_row_pitch, obj_slice_pitch }});
   auto host_origin = vector(p_host_origin);
            validate_common(q, deps);
   validate_object(q, mem, obj_origin, obj_pitch, region);
   validate_object(q, ptr, host_origin, host_pitch, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_WRITE_BUFFER_RECT, deps,
   soft_copy_op(q, &mem, obj_origin, obj_pitch,
               if (blocking)
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueFillBuffer(cl_command_queue d_queue, cl_mem d_mem,
                     const void *pattern, size_t pattern_size,
      auto &q = obj(d_queue);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t origin = { offset };
            validate_common(q, deps);
            if (!pattern)
            if (!util_is_power_of_two_nonzero(pattern_size) ||
      pattern_size > 128 || size % pattern_size
   || offset % pattern_size) {
               auto sub = dynamic_cast<sub_buffer *>(&mem);
   if (sub && sub->offset() % q.device().mem_base_addr_align()) {
                  std::string data = std::string((char *)pattern, pattern_size);
   auto hev = create<hard_event>(
      q, CL_COMMAND_FILL_BUFFER, deps,
   [=, &q, &mem](event &) {
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueCopyBuffer(cl_command_queue d_q, cl_mem d_src_mem, cl_mem d_dst_mem,
                        auto &q = obj(d_q);
   auto &src_mem = obj<buffer>(d_src_mem);
   auto &dst_mem = obj<buffer>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t dst_origin = { dst_offset };
   auto dst_pitch = pitch(region, {{ 1 }});
   vector_t src_origin = { src_offset };
            validate_common(q, deps);
   validate_object(q, dst_mem, dst_origin, dst_pitch, region);
   validate_object(q, src_mem, src_origin, src_pitch, region);
   validate_copy(q, dst_mem, dst_origin, dst_pitch,
            auto hev = create<hard_event>(
      q, CL_COMMAND_COPY_BUFFER, deps,
   hard_copy_op(q, &dst_mem, dst_origin,
         ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueCopyBufferRect(cl_command_queue d_q, cl_mem d_src_mem,
                           cl_mem d_dst_mem,
   const size_t *p_src_origin, const size_t *p_dst_origin,
   const size_t *p_region,
      auto &q = obj(d_q);
   auto &src_mem = obj<buffer>(d_src_mem);
   auto &dst_mem = obj<buffer>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto dst_origin = vector(p_dst_origin);
   auto dst_pitch = pitch(region, {{ 1, dst_row_pitch, dst_slice_pitch }});
   auto src_origin = vector(p_src_origin);
            validate_common(q, deps);
   validate_object(q, dst_mem, dst_origin, dst_pitch, region);
   validate_object(q, src_mem, src_origin, src_pitch, region);
   validate_copy(q, dst_mem, dst_origin, dst_pitch,
            auto hev = create<hard_event>(
      q, CL_COMMAND_COPY_BUFFER_RECT, deps,
   soft_copy_op(q, &dst_mem, dst_origin, dst_pitch,
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueReadImage(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                     const size_t *p_origin, const size_t *p_region,
      auto &q = obj(d_q);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto dst_pitch = pitch(region, {{ img.pixel_size(),
         auto src_origin = vector(p_origin);
   auto src_pitch = pitch(region, {{ img.pixel_size(),
            validate_common(q, deps);
   validate_object(q, ptr, {}, dst_pitch, region);
   validate_object(q, img, src_origin, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_READ_IMAGE, deps,
   soft_copy_op(q, ptr, {}, dst_pitch,
               if (blocking)
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueWriteImage(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                     const size_t *p_origin, const size_t *p_region,
      auto &q = obj(d_q);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto dst_origin = vector(p_origin);
   auto dst_pitch = pitch(region, {{ img.pixel_size(),
         auto src_pitch = pitch(region, {{ img.pixel_size(),
            validate_common(q, deps);
   validate_object(q, img, dst_origin, region);
   validate_object(q, ptr, {}, src_pitch, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_WRITE_IMAGE, deps,
   soft_copy_op(q, &img, dst_origin, dst_pitch,
               if (blocking)
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueFillImage(cl_command_queue d_queue, cl_mem d_mem,
                     const void *fill_color,
      auto &q = obj(d_queue);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto origin = vector(p_origin);
            validate_common(q, deps);
            if (!fill_color)
            std::string data = std::string((char *)fill_color, sizeof(cl_uint4));
   auto hev = create<hard_event>(
      q, CL_COMMAND_FILL_IMAGE, deps,
   [=, &q, &img](event &) {
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueCopyImage(cl_command_queue d_q, cl_mem d_src_mem, cl_mem d_dst_mem,
                     const size_t *p_src_origin, const size_t *p_dst_origin,
      auto &q = obj(d_q);
   auto &src_img = obj<image>(d_src_mem);
   auto &dst_img = obj<image>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto dst_origin = vector(p_dst_origin);
            validate_common(q, deps);
   validate_object(q, dst_img, dst_origin, region);
   validate_object(q, src_img, src_origin, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_COPY_IMAGE, deps,
   hard_copy_op(q, &dst_img, dst_origin,
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueCopyImageToBuffer(cl_command_queue d_q,
                              cl_mem d_src_mem, cl_mem d_dst_mem,
   auto &q = obj(d_q);
   auto &src_img = obj<image>(d_src_mem);
   auto &dst_mem = obj<buffer>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   vector_t dst_origin = { dst_offset };
   auto dst_pitch = pitch(region, {{ src_img.pixel_size() }});
   auto src_origin = vector(p_src_origin);
   auto src_pitch = pitch(region, {{ src_img.pixel_size(),
                  validate_common(q, deps);
   validate_object(q, dst_mem, dst_origin, dst_pitch, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_COPY_IMAGE_TO_BUFFER, deps,
   soft_copy_op(q, &dst_mem, dst_origin, dst_pitch,
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueCopyBufferToImage(cl_command_queue d_q,
                              cl_mem d_src_mem, cl_mem d_dst_mem,
   auto &q = obj(d_q);
   auto &src_mem = obj<buffer>(d_src_mem);
   auto &dst_img = obj<image>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
   auto dst_origin = vector(p_dst_origin);
   auto dst_pitch = pitch(region, {{ dst_img.pixel_size(),
               vector_t src_origin = { src_offset };
            validate_common(q, deps);
   validate_object(q, dst_img, dst_origin, region);
            auto hev = create<hard_event>(
      q, CL_COMMAND_COPY_BUFFER_TO_IMAGE, deps,
   soft_copy_op(q, &dst_img, dst_origin, dst_pitch,
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API void *
   clEnqueueMapBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                        auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t obj_origin = { offset };
            validate_common(q, deps);
   validate_object(q, mem, obj_origin, obj_pitch, region);
                     auto hev = create<hard_event>(q, CL_COMMAND_MAP_BUFFER, deps);
   if (blocking)
            ret_object(rd_ev, hev);
   ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API void *
   clEnqueueMapImage(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                     cl_map_flags flags,
   const size_t *p_origin, const size_t *p_region,
      auto &q = obj(d_q);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto region = vector(p_region);
            validate_common(q, deps);
   validate_object(q, img, origin, region);
            if (!row_pitch)
            if ((img.slice_pitch() || img.array_size()) && !slice_pitch)
            auto *map = img.resource_in(q).add_map(q, flags, blocking, origin, region);
   *row_pitch = map->pitch()[1];
   if (slice_pitch)
            auto hev = create<hard_event>(q, CL_COMMAND_MAP_IMAGE, deps);
   if (blocking)
            ret_object(rd_ev, hev);
   ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_int
   clEnqueueUnmapMemObject(cl_command_queue d_q, cl_mem d_mem, void *ptr,
                  auto &q = obj(d_q);
   auto &mem = obj(d_mem);
                     auto hev = create<hard_event>(
      q, CL_COMMAND_UNMAP_MEM_OBJECT, deps,
   [=, &q, &mem](event &) {
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueMigrateMemObjects(cl_command_queue d_q,
                              cl_uint num_mems,
   const cl_mem *d_mems,
   auto &q = obj(d_q);
   auto mems = objs<memory_obj>(d_mems, num_mems);
            validate_common(q, deps);
            if (any_of([&](const memory_obj &m) {
         return m.context() != q.context();
            auto hev = create<hard_event>(
      q, CL_COMMAND_MIGRATE_MEM_OBJECTS, deps,
   [=, &q](event &) {
      for (auto &mem: mems) {
      if (flags & CL_MIGRATE_MEM_OBJECT_HOST) {
                                       } else {
      if (flags & CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED)
         else
                  ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      static void CL_CALLBACK
   free_queue(cl_command_queue d_q, cl_uint num_svm_pointers,
            clover::context &ctx = obj(d_q).context();
   for (void *p : range(svm_pointers, num_svm_pointers)) {
      ctx.remove_svm_allocation(p);
         }
      cl_int
   clover::EnqueueSVMFree(cl_command_queue d_q,
                        cl_uint num_svm_pointers,
   void *svm_pointers[],
   void (CL_CALLBACK *pfn_free_func) (
         cl_command_queue queue, cl_uint num_svm_pointers,
   void *user_data,
            if (bool(num_svm_pointers) != bool(svm_pointers))
                     if (!q.device().svm_support())
            bool can_emulate = q.device().has_system_svm();
                     std::vector<void *> svm_pointers_cpy(svm_pointers,
         if (!pfn_free_func) {
      if (!can_emulate) {
      CLOVER_NOT_SUPPORTED_UNTIL("2.0");
      }
               auto hev = create<hard_event>(q, cmd, deps,
      [=](clover::event &) mutable {
      pfn_free_func(d_q, num_svm_pointers, svm_pointers_cpy.data(),
            ret_object(event, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueSVMFree(cl_command_queue d_q,
                  cl_uint num_svm_pointers,
   void *svm_pointers[],
   void (CL_CALLBACK *pfn_free_func) (
      cl_command_queue queue, cl_uint num_svm_pointers,
      void *user_data,
            return EnqueueSVMFree(d_q, num_svm_pointers, svm_pointers,
            }
      cl_int
   clover::EnqueueSVMMemcpy(cl_command_queue d_q,
                           cl_bool blocking_copy,
   void *dst_ptr,
   const void *src_ptr,
   size_t size,
               if (!q.device().svm_support())
            if (dst_ptr == nullptr || src_ptr == nullptr)
            if (static_cast<size_t>(abs(reinterpret_cast<ptrdiff_t>(dst_ptr) -
                     bool can_emulate = q.device().has_system_svm();
                     if (can_emulate) {
      auto hev = create<hard_event>(q, cmd, deps,
      [=](clover::event &) {
               if (blocking_copy)
         ret_object(event, hev);
               CLOVER_NOT_SUPPORTED_UNTIL("2.0");
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueSVMMemcpy(cl_command_queue d_q,
                     cl_bool blocking_copy,
   void *dst_ptr,
   const void *src_ptr,
   size_t size,
         return EnqueueSVMMemcpy(d_q, blocking_copy, dst_ptr, src_ptr,
            }
      cl_int
   clover::EnqueueSVMMemFill(cl_command_queue d_q,
                           void *svm_ptr,
   const void *pattern,
   size_t pattern_size,
   size_t size,
               if (!q.device().svm_support())
            if (svm_ptr == nullptr || pattern == nullptr ||
      !util_is_power_of_two_nonzero(pattern_size) ||
   pattern_size > 128 ||
   !ptr_is_aligned(svm_ptr, pattern_size) ||
   size % pattern_size)
         bool can_emulate = q.device().has_system_svm();
                     if (can_emulate) {
      auto hev = create<hard_event>(q, cmd, deps,
      [=](clover::event &) {
      void *ptr = svm_ptr;
   for (size_t s = size; s; s -= pattern_size) {
      memcpy(ptr, pattern, pattern_size);
               ret_object(event, hev);
               CLOVER_NOT_SUPPORTED_UNTIL("2.0");
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueSVMMemFill(cl_command_queue d_q,
                     void *svm_ptr,
   const void *pattern,
   size_t pattern_size,
   size_t size,
         return EnqueueSVMMemFill(d_q, svm_ptr, pattern, pattern_size,
            }
      cl_int
   clover::EnqueueSVMMap(cl_command_queue d_q,
                        cl_bool blocking_map,
   cl_map_flags map_flags,
   void *svm_ptr,
   size_t size,
   cl_uint num_events_in_wait_list,
            if (!q.device().svm_support())
            if (svm_ptr == nullptr || size == 0)
            bool can_emulate = q.device().has_system_svm();
                     if (can_emulate) {
      auto hev = create<hard_event>(q, cmd, deps,
            ret_object(event, hev);
               CLOVER_NOT_SUPPORTED_UNTIL("2.0");
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueSVMMap(cl_command_queue d_q,
                  cl_bool blocking_map,
   cl_map_flags map_flags,
   void *svm_ptr,
   size_t size,
            return EnqueueSVMMap(d_q, blocking_map, map_flags, svm_ptr, size,
            }
      cl_int
   clover::EnqueueSVMUnmap(cl_command_queue d_q,
                           void *svm_ptr,
               if (!q.device().svm_support())
            if (svm_ptr == nullptr)
            bool can_emulate = q.device().has_system_svm();
                     if (can_emulate) {
      auto hev = create<hard_event>(q, cmd, deps,
            ret_object(event, hev);
               CLOVER_NOT_SUPPORTED_UNTIL("2.0");
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueSVMUnmap(cl_command_queue d_q,
                     void *svm_ptr,
         return EnqueueSVMUnmap(d_q, svm_ptr, num_events_in_wait_list,
      }
      CLOVER_API cl_int
   clEnqueueSVMMigrateMem(cl_command_queue d_q,
                        cl_uint num_svm_pointers,
   const void **svm_pointers,
   const size_t *sizes,
   const cl_mem_migration_flags flags,
   auto &q = obj(d_q);
            validate_common(q, deps);
            if (!q.device().svm_support())
            if (!num_svm_pointers || !svm_pointers)
            std::vector<size_t> sizes_copy(num_svm_pointers);
            for (unsigned i = 0; i < num_svm_pointers; ++i) {
      const void *ptr = svm_pointers[i];
   size_t size = sizes ? sizes[i] : 0;
   if (!ptr)
            auto p = q.context().find_svm_allocation(ptr);
   if (!p.first)
            std::ptrdiff_t pdiff = (uint8_t*)ptr - (uint8_t*)p.first;
   if (size && size + pdiff > p.second)
            sizes_copy[i] = size ? size : p.second;
               auto hev = create<hard_event>(
      q, CL_COMMAND_MIGRATE_MEM_OBJECTS, deps,
   [=, &q](event &) {
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
