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
   #include "core/context.hpp"
   #include "core/platform.hpp"
      using namespace clover;
      CLOVER_API cl_context
   clCreateContext(const cl_context_properties *d_props, cl_uint num_devs,
                  const cl_device_id *d_devs,
   void (CL_CALLBACK *pfn_notify)(const char *, const void *,
   auto props = obj<property_list_tag>(d_props);
            if (!pfn_notify && user_data)
            for (auto &prop : props) {
      if (prop.first == CL_CONTEXT_PLATFORM)
         else
               const auto notify = (!pfn_notify ? context::notify_action() :
                        ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_context
   clCreateContextFromType(const cl_context_properties *d_props,
                              cl_platform_id d_platform;
   cl_uint num_platforms;
   cl_int ret;
   std::vector<cl_device_id> devs;
            ret = clGetPlatformIDs(1, &d_platform, &num_platforms);
   if (ret || !num_platforms)
            ret = clGetDeviceIDs(d_platform, type, 0, NULL, &num_devices);
   if (ret)
         devs.resize(num_devices);
   ret = clGetDeviceIDs(d_platform, type, num_devices, devs.data(), 0);
   if (ret)
            return clCreateContext(d_props, num_devices, devs.data(), pfn_notify,
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_int
   clRetainContext(cl_context d_ctx) try {
      obj(d_ctx).retain();
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clReleaseContext(cl_context d_ctx) try {
      if (obj(d_ctx).release())
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetContextInfo(cl_context d_ctx, cl_context_info param,
            property_buffer buf { r_buf, size, r_size };
            switch (param) {
   case CL_CONTEXT_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = ctx.ref_count();
         case CL_CONTEXT_NUM_DEVICES:
      buf.as_scalar<cl_uint>() = ctx.devices().size();
         case CL_CONTEXT_DEVICES:
      buf.as_vector<cl_device_id>() = descs(ctx.devices());
         case CL_CONTEXT_PROPERTIES:
      buf.as_vector<cl_context_properties>() = desc(ctx.properties());
         default:
                        } catch (error &e) {
         }
      CLOVER_API cl_int
   clSetContextDestructorCallback(cl_context d_ctx,
                  CLOVER_NOT_SUPPORTED_UNTIL("3.0");
            if (!pfn_notify)
                           } catch (error &e) {
         }
