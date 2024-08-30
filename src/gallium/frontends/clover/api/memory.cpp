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
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "api/util.hpp"
   #include "core/memory.hpp"
   #include "core/format.hpp"
      using namespace clover;
      namespace {
      cl_mem_flags
   validate_flags(cl_mem d_parent, cl_mem_flags d_flags, bool svm) {
      const cl_mem_flags dev_access_flags =
         const cl_mem_flags host_ptr_flags =
         const cl_mem_flags host_access_flags =
         const cl_mem_flags svm_flags =
            const cl_mem_flags valid_flags =
      dev_access_flags
               if ((d_flags & ~valid_flags) ||
      util_bitcount(d_flags & dev_access_flags) > 1 ||
               if ((d_flags & CL_MEM_USE_HOST_PTR) &&
                  if ((d_flags & CL_MEM_SVM_ATOMICS) &&
                  if (d_parent) {
      const auto &parent = obj(d_parent);
   const cl_mem_flags flags = (d_flags |
                                                   // Check if new host access flags cause a mismatch between
   // host-read/write-only.
   if (!(flags & CL_MEM_HOST_NO_ACCESS) &&
                        } else {
                     std::vector<cl_mem_properties>
   fill_properties(const cl_mem_properties *d_properties) {
      std::vector<cl_mem_properties> properties;
   if (d_properties) {
      while (*d_properties) {
                     properties.push_back(*d_properties);
      };
      }
         }
      CLOVER_API cl_mem
   clCreateBufferWithProperties(cl_context d_ctx,
                           auto &ctx = obj(d_ctx);
   const cl_mem_flags flags = validate_flags(NULL, d_flags, false);
            if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                  if (!size ||
      size > fold(maximum(), cl_ulong(0),
                     ret_error(r_errcode, CL_SUCCESS);
      } catch (error &e) {
      ret_error(r_errcode, e);
      }
         CLOVER_API cl_mem
   clCreateBuffer(cl_context d_ctx, cl_mem_flags d_flags, size_t size,
            return clCreateBufferWithProperties(d_ctx, NULL, d_flags, size,
      }
      CLOVER_API cl_mem
   clCreateSubBuffer(cl_mem d_mem, cl_mem_flags d_flags,
                  auto &parent = obj<root_buffer>(d_mem);
            if (op == CL_BUFFER_CREATE_TYPE_REGION) {
               if (!reg ||
      reg->origin > parent.size() ||
               if (!reg->size)
            ret_error(r_errcode, CL_SUCCESS);
         } else {
               } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_mem
   clCreateImageWithProperties(cl_context d_ctx,
                              const cl_mem_properties *d_properties,
            if (!any_of(std::mem_fn(&device::image_support), ctx.devices()))
            if (!format)
            if (!desc)
            if (desc->image_array_size == 0 &&
      (desc->image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY ||
   desc->image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY))
         if (!host_ptr &&
      (desc->image_row_pitch || desc->image_slice_pitch))
         if (desc->num_mip_levels || desc->num_samples)
            if (bool(desc->buffer) != (desc->image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER))
            if (bool(host_ptr) != bool(d_flags & (CL_MEM_USE_HOST_PTR |
                           if (!supported_formats(ctx, desc->image_type, d_flags).count(*format))
            std::vector<cl_mem_properties> properties = fill_properties(d_properties);
            const size_t row_pitch = desc->image_row_pitch ? desc->image_row_pitch :
            switch (desc->image_type) {
   case CL_MEM_OBJECT_IMAGE1D:
      if (!desc->image_width)
            if (all_of([=](const device &dev) {
            const size_t max = dev.max_image_size();
               return new image1d(ctx, properties, flags, format,
               case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      if (!desc->image_width)
            if (all_of([=](const device &dev) {
            const size_t max = dev.max_image_buffer_size();
               return new image1d_buffer(ctx, properties, flags, format,
               case CL_MEM_OBJECT_IMAGE1D_ARRAY: {
      if (!desc->image_width)
            if (all_of([=](const device &dev) {
            const size_t max = dev.max_image_size();
   const size_t amax = dev.max_image_array_number();
   return (desc->image_width > max ||
               const size_t slice_pitch = desc->image_slice_pitch ?
            return new image1d_array(ctx, properties, flags, format,
                           case CL_MEM_OBJECT_IMAGE2D:
      if (!desc->image_width || !desc->image_height)
            if (all_of([=](const device &dev) {
            const size_t max = dev.max_image_size();
   return (desc->image_width > max ||
               return new image2d(ctx, properties, flags, format,
               case CL_MEM_OBJECT_IMAGE2D_ARRAY: {
      if (!desc->image_width || !desc->image_height || !desc->image_array_size)
            if (all_of([=](const device &dev) {
            const size_t max = dev.max_image_size();
   const size_t amax = dev.max_image_array_number();
   return (desc->image_width > max ||
                     const size_t slice_pitch = desc->image_slice_pitch ?
            return new image2d_array(ctx, properties, flags, format,
                           case CL_MEM_OBJECT_IMAGE3D: {
      if (!desc->image_width || !desc->image_height || !desc->image_depth)
            if (all_of([=](const device &dev) {
            const size_t max = dev.max_image_size_3d();
   return (desc->image_width > max ||
                     const size_t slice_pitch = desc->image_slice_pitch ?
            return new image3d(ctx, properties, flags, format,
                           default:
               } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_mem
   clCreateImage(cl_context d_ctx,
               cl_mem_flags d_flags,
   const cl_image_format *format,
         }
         CLOVER_API cl_mem
   clCreateImage2D(cl_context d_ctx, cl_mem_flags d_flags,
                  const cl_image_format *format,
   const cl_image_desc desc = { CL_MEM_OBJECT_IMAGE2D, width, height, 0, 0,
               }
      CLOVER_API cl_mem
   clCreateImage3D(cl_context d_ctx, cl_mem_flags d_flags,
                  const cl_image_format *format,
   size_t width, size_t height, size_t depth,
   const cl_image_desc desc = { CL_MEM_OBJECT_IMAGE3D, width, height, depth, 0,
               }
      CLOVER_API cl_int
   clGetSupportedImageFormats(cl_context d_ctx, cl_mem_flags flags,
                  auto &ctx = obj(d_ctx);
            if (flags & CL_MEM_KERNEL_READ_AND_WRITE) {
      if (r_count)
                     if (flags & (CL_MEM_WRITE_ONLY | CL_MEM_READ_WRITE) &&
      type == CL_MEM_OBJECT_IMAGE3D) {
   if (r_count)
                              if (r_buf && !count)
            if (r_buf)
      std::copy_n(formats.begin(),
               if (r_count)
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetMemObjectInfo(cl_mem d_mem, cl_mem_info param,
            property_buffer buf { r_buf, size, r_size };
            switch (param) {
   case CL_MEM_TYPE:
      buf.as_scalar<cl_mem_object_type>() = mem.type();
         case CL_MEM_FLAGS:
      buf.as_scalar<cl_mem_flags>() = mem.flags();
         case CL_MEM_SIZE:
      buf.as_scalar<size_t>() = mem.size();
         case CL_MEM_HOST_PTR:
      buf.as_scalar<void *>() = mem.host_ptr();
         case CL_MEM_MAP_COUNT:
      buf.as_scalar<cl_uint>() = 0;
         case CL_MEM_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = mem.ref_count();
         case CL_MEM_CONTEXT:
      buf.as_scalar<cl_context>() = desc(mem.context());
         case CL_MEM_ASSOCIATED_MEMOBJECT: {
      sub_buffer *sub = dynamic_cast<sub_buffer *>(&mem);
   if (sub) {
      buf.as_scalar<cl_mem>() = desc(sub->parent());
               image *img = dynamic_cast<image *>(&mem);
   if (img) {
      buf.as_scalar<cl_mem>() = desc(img->buffer());
               buf.as_scalar<cl_mem>() = NULL;
      }
   case CL_MEM_OFFSET: {
      sub_buffer *sub = dynamic_cast<sub_buffer *>(&mem);
   buf.as_scalar<size_t>() = (sub ? sub->offset() : 0);
      }
   case CL_MEM_USES_SVM_POINTER:
   case CL_MEM_USES_SVM_POINTER_ARM: {
      // with system SVM all host ptrs are SVM pointers
   // TODO: once we support devices with lower levels of SVM, we have to
   // check the ptr in more detail
   const bool system_svm = all_of(std::mem_fn(&device::has_system_svm),
         buf.as_scalar<cl_bool>() = mem.host_ptr() && system_svm;
      }
   case CL_MEM_PROPERTIES:
      buf.as_vector<cl_mem_properties>() = mem.properties();
      default:
                        } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetImageInfo(cl_mem d_mem, cl_image_info param,
            property_buffer buf { r_buf, size, r_size };
            switch (param) {
   case CL_IMAGE_FORMAT:
      buf.as_scalar<cl_image_format>() = img.format();
         case CL_IMAGE_ELEMENT_SIZE:
      buf.as_scalar<size_t>() = img.pixel_size();
         case CL_IMAGE_ROW_PITCH:
      buf.as_scalar<size_t>() = img.row_pitch();
         case CL_IMAGE_SLICE_PITCH:
      buf.as_scalar<size_t>() = img.slice_pitch();
         case CL_IMAGE_WIDTH:
      buf.as_scalar<size_t>() = img.width();
         case CL_IMAGE_HEIGHT:
      buf.as_scalar<size_t>() = img.dimensions() > 1 ? img.height() : 0;
         case CL_IMAGE_DEPTH:
      buf.as_scalar<size_t>() = img.dimensions() > 2 ? img.depth() : 0;
         case CL_IMAGE_ARRAY_SIZE:
      buf.as_scalar<size_t>() = img.array_size();
         case CL_IMAGE_BUFFER:
      buf.as_scalar<cl_mem>() = img.buffer();
         case CL_IMAGE_NUM_MIP_LEVELS:
      buf.as_scalar<cl_uint>() = 0;
         case CL_IMAGE_NUM_SAMPLES:
      buf.as_scalar<cl_uint>() = 0;
         default:
                        } catch (error &e) {
         }
      CLOVER_API cl_int
   clRetainMemObject(cl_mem d_mem) try {
      obj(d_mem).retain();
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clReleaseMemObject(cl_mem d_mem) try {
      if (obj(d_mem).release())
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clSetMemObjectDestructorCallback(cl_mem d_mem,
                           if (!pfn_notify)
                           } catch (error &e) {
         }
      CLOVER_API void *
   clSVMAlloc(cl_context d_ctx,
            cl_svm_mem_flags flags,
   size_t size,
            if (!any_of(std::mem_fn(&device::svm_support), ctx.devices()))
                     if (!size ||
      size > fold(minimum(), cl_ulong(ULONG_MAX),
               if (!util_is_power_of_two_or_zero(alignment))
            if (!alignment)
         #if defined(HAVE_POSIX_MEMALIGN)
      bool can_emulate = all_of(std::mem_fn(&device::has_system_svm), ctx.devices());
   if (can_emulate) {
      // we can ignore all the flags as it's not required to honor them.
   void *ptr = nullptr;
   if (alignment < sizeof(void*))
         int ret = posix_memalign(&ptr, alignment, size);
   if (ret)
            if (ptr)
                  #endif
         CLOVER_NOT_SUPPORTED_UNTIL("2.0");
         } catch (error &) {
         }
      CLOVER_API void
   clSVMFree(cl_context d_ctx,
                     if (!any_of(std::mem_fn(&device::svm_support), ctx.devices()))
                     if (can_emulate) {
      ctx.remove_svm_allocation(svm_pointer);
                     } catch (error &) {
   }
