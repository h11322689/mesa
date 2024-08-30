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
      #include "api/util.hpp"
   #include "core/kernel.hpp"
   #include "core/event.hpp"
      using namespace clover;
      CLOVER_API cl_kernel
   clCreateKernel(cl_program d_prog, const char *name, cl_int *r_errcode) try {
               if (!name)
                     ret_error(r_errcode, CL_SUCCESS);
         } catch (std::out_of_range &) {
      ret_error(r_errcode, CL_INVALID_KERNEL_NAME);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_int
   clCreateKernelsInProgram(cl_program d_prog, cl_uint count,
            auto &prog = obj(d_prog);
            if (rd_kerns && count < syms.size())
            if (rd_kerns)
      copy(map([&](const binary::symbol &sym) {
            return desc(new kernel(prog,
                        if (r_count)
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clRetainKernel(cl_kernel d_kern) try {
      obj(d_kern).retain();
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clReleaseKernel(cl_kernel d_kern) try {
      if (obj(d_kern).release())
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clSetKernelArg(cl_kernel d_kern, cl_uint idx, size_t size,
            obj(d_kern).args().at(idx).set(size, value);
         } catch (std::out_of_range &) {
            } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetKernelInfo(cl_kernel d_kern, cl_kernel_info param,
            property_buffer buf { r_buf, size, r_size };
            switch (param) {
   case CL_KERNEL_FUNCTION_NAME:
      buf.as_string() = kern.name();
         case CL_KERNEL_NUM_ARGS:
      buf.as_scalar<cl_uint>() = kern.args().size();
         case CL_KERNEL_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = kern.ref_count();
         case CL_KERNEL_CONTEXT:
      buf.as_scalar<cl_context>() = desc(kern.program().context());
         case CL_KERNEL_PROGRAM:
      buf.as_scalar<cl_program>() = desc(kern.program());
         case CL_KERNEL_ATTRIBUTES:
      buf.as_string() = find(name_equals(kern.name()), kern.program().symbols()).attributes;
         default:
                        } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetKernelWorkGroupInfo(cl_kernel d_kern, cl_device_id d_dev,
                  property_buffer buf { r_buf, size, r_size };
   auto &kern = obj(d_kern);
            if (!count(dev, kern.program().devices()))
            switch (param) {
   case CL_KERNEL_WORK_GROUP_SIZE:
      buf.as_scalar<size_t>() = dev.max_threads_per_block();
         case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
      buf.as_vector<size_t>() = kern.required_block_size();
         case CL_KERNEL_LOCAL_MEM_SIZE:
      buf.as_scalar<cl_ulong>() = kern.mem_local();
         case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE:
      buf.as_scalar<size_t>() = dev.subgroup_size();
         case CL_KERNEL_PRIVATE_MEM_SIZE:
      buf.as_scalar<cl_ulong>() = kern.mem_private();
         default:
                        } catch (error &e) {
            } catch (std::out_of_range &) {
         }
      CLOVER_API cl_int
   clGetKernelArgInfo(cl_kernel d_kern,
                                    if (info.arg_name.empty())
            switch (param) {
   case CL_KERNEL_ARG_ADDRESS_QUALIFIER:
      buf.as_scalar<cl_kernel_arg_address_qualifier>() = info.address_qualifier;
         case CL_KERNEL_ARG_ACCESS_QUALIFIER:
      buf.as_scalar<cl_kernel_arg_access_qualifier>() = info.access_qualifier;
         case CL_KERNEL_ARG_TYPE_NAME:
      buf.as_string() = info.type_name;
         case CL_KERNEL_ARG_TYPE_QUALIFIER:
      buf.as_scalar<cl_kernel_arg_type_qualifier>() = info.type_qualifier;
         case CL_KERNEL_ARG_NAME:
      buf.as_string() = info.arg_name;
         default:
                        } catch (std::out_of_range &) {
            } catch (error &e) {
         }
      namespace {
      ///
   /// Common argument checking shared by kernel invocation commands.
   ///
   void
   validate_common(const command_queue &q, kernel &kern,
            if (kern.program().context() != q.context() ||
      any_of([&](const event &ev) {
                     if (any_of([](kernel::argument &arg) {
                        // If the command queue's device is not associated to the program, we get
   // a binary, with no sections, which will also fail the following test.
   auto &b = kern.program().build(q.device()).bin;
   if (!any_of(type_equals(binary::section::text_executable), b.secs))
               std::vector<size_t>
   validate_grid_size(const command_queue &q, cl_uint dims,
                     if (dims < 1 || dims > q.device().max_block_size().size())
                        std::vector<size_t>
   validate_grid_offset(const command_queue &q, cl_uint dims,
            if (d_grid_offset)
         else
               std::vector<size_t>
   validate_block_size(const command_queue &q, const kernel &kern,
                           if (d_block_size) {
               if (any_of(is_zero(), block_size) ||
                                 if (fold(multiplies(), 1u, block_size) >
                        } else {
               }
      CLOVER_API cl_int
   clEnqueueNDRangeKernel(cl_command_queue d_q, cl_kernel d_kern,
                        cl_uint dims, const size_t *d_grid_offset,
   auto &q = obj(d_q);
   auto &kern = obj(d_kern);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto grid_size = validate_grid_size(q, dims, d_grid_size);
   auto grid_offset = validate_grid_offset(q, dims, d_grid_offset);
   auto block_size = validate_block_size(q, kern, dims,
                     auto hev = create<hard_event>(
      q, CL_COMMAND_NDRANGE_KERNEL, deps,
   [=, &kern, &q](event &) {
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueTask(cl_command_queue d_q, cl_kernel d_kern,
                  auto &q = obj(d_q);
   auto &kern = obj(d_kern);
                     auto hev = create<hard_event>(
      q, CL_COMMAND_TASK, deps,
   [=, &kern, &q](event &) {
               ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueNativeKernel(cl_command_queue d_q,
                        void (CL_CALLBACK * func)(void *),
   void *args, size_t args_size,
      }
      CLOVER_API cl_int
   clSetKernelArgSVMPointer(cl_kernel d_kern,
               if (!any_of(std::mem_fn(&device::svm_support), obj(d_kern).program().devices()))
            obj(d_kern).args().at(arg_index).set_svm(arg_value);
         } catch (std::out_of_range &) {
            } catch (error &e) {
         }
      CLOVER_API cl_int
   clSetKernelExecInfo(cl_kernel d_kern,
                           if (!any_of(std::mem_fn(&device::svm_support), obj(d_kern).program().devices()))
                     const bool has_system_svm = all_of(std::mem_fn(&device::has_system_svm),
            if (!param_value)
            switch (param_name) {
   case CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM:
   case CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM_ARM: {
      if (param_value_size != sizeof(cl_bool))
            cl_bool val = *static_cast<const cl_bool*>(param_value);
   if (val == CL_TRUE && !has_system_svm)
         else
               case CL_KERNEL_EXEC_INFO_SVM_PTRS:
   case CL_KERNEL_EXEC_INFO_SVM_PTRS_ARM:
      if (has_system_svm)
            CLOVER_NOT_SUPPORTED_UNTIL("2.0");
         default:
               } catch (error &e) {
         }
