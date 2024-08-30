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
      #include <unordered_map>
      #include "api/dispatch.hpp"
   #include "api/util.hpp"
   #include "core/platform.hpp"
   #include "git_sha1.h"
   #include "util/u_debug.h"
      using namespace clover;
      namespace {
         }
      CLOVER_API cl_int
   clGetPlatformIDs(cl_uint num_entries, cl_platform_id *rd_platforms,
            if ((!num_entries && rd_platforms) ||
      (!rnum_platforms && !rd_platforms))
         if (rnum_platforms)
         if (rd_platforms)
               }
      platform &clover::find_platform(cl_platform_id d_platform)
   {
      /* this error is only added in CL2.0 */
   if (d_platform != desc(_clover_platform))
            }
      cl_int
   clover::GetPlatformInfo(cl_platform_id d_platform, cl_platform_info param,
                              switch (param) {
   case CL_PLATFORM_PROFILE:
      buf.as_string() = "FULL_PROFILE";
         case CL_PLATFORM_VERSION: {
      buf.as_string() = "OpenCL " + platform.platform_version_as_string() + " Mesa " PACKAGE_VERSION MESA_GIT_SHA1;
      }
   case CL_PLATFORM_NAME:
      buf.as_string() = "Clover";
         case CL_PLATFORM_VENDOR:
      buf.as_string() = "Mesa";
         case CL_PLATFORM_EXTENSIONS:
      buf.as_string() = platform.supported_extensions_as_string();
         case CL_PLATFORM_ICD_SUFFIX_KHR:
      buf.as_string() = "MESA";
         case CL_PLATFORM_NUMERIC_VERSION: {
      buf.as_scalar<cl_version>() = platform.platform_version();
               case CL_PLATFORM_EXTENSIONS_WITH_VERSION:
      buf.as_vector<cl_name_version>() = platform.supported_extensions();
         case CL_PLATFORM_HOST_TIMER_RESOLUTION:
      buf.as_scalar<cl_ulong>() = 0;
         default:
                        } catch (error &e) {
         }
      void *
   clover::GetExtensionFunctionAddressForPlatform(cl_platform_id d_platform,
            obj(d_platform);
         } catch (error &) {
         }
      namespace {
      cl_int
   enqueueSVMFreeARM(cl_command_queue command_queue,
                     cl_uint num_svm_pointers,
   void *svm_pointers[],
   void (CL_CALLBACK *pfn_free_func) (
   cl_command_queue queue, cl_uint num_svm_pointers,
   void *svm_pointers[], void *user_data),
   void *user_data,
         return EnqueueSVMFree(command_queue, num_svm_pointers, svm_pointers,
            }
      cl_int
   enqueueSVMMapARM(cl_command_queue command_queue,
                  cl_bool blocking_map,
   cl_map_flags map_flags,
   void *svm_ptr,
   size_t size,
            return EnqueueSVMMap(command_queue, blocking_map, map_flags, svm_ptr, size,
            }
      cl_int
   enqueueSVMMemcpyARM(cl_command_queue command_queue,
                     cl_bool blocking_copy,
   void *dst_ptr,
   const void *src_ptr,
   size_t size,
         return EnqueueSVMMemcpy(command_queue, blocking_copy, dst_ptr, src_ptr,
            }
      cl_int
   enqueueSVMMemFillARM(cl_command_queue command_queue,
                        void *svm_ptr,
   const void *pattern,
   size_t pattern_size,
            return EnqueueSVMMemFill(command_queue, svm_ptr, pattern, pattern_size,
            }
      cl_int
   enqueueSVMUnmapARM(cl_command_queue command_queue,
                     void *svm_ptr,
         return EnqueueSVMUnmap(command_queue, svm_ptr, num_events_in_wait_list,
      }
      const std::unordered_map<std::string, void *>
   ext_funcs = {
      // cl_arm_shared_virtual_memory
   { "clEnqueueSVMFreeARM", reinterpret_cast<void *>(enqueueSVMFreeARM) },
   { "clEnqueueSVMMapARM", reinterpret_cast<void *>(enqueueSVMMapARM) },
   { "clEnqueueSVMMemcpyARM", reinterpret_cast<void *>(enqueueSVMMemcpyARM) },
   { "clEnqueueSVMMemFillARM", reinterpret_cast<void *>(enqueueSVMMemFillARM) },
   { "clEnqueueSVMUnmapARM", reinterpret_cast<void *>(enqueueSVMUnmapARM) },
   { "clSetKernelArgSVMPointerARM", reinterpret_cast<void *>(clSetKernelArgSVMPointer) },
   { "clSetKernelExecInfoARM", reinterpret_cast<void *>(clSetKernelExecInfo) },
   { "clSVMAllocARM", reinterpret_cast<void *>(clSVMAlloc) },
            // cl_khr_icd
            // cl_khr_il_program
      };
      } // anonymous namespace
      void *
   clover::GetExtensionFunctionAddress(const char *p_name) try {
         } catch (...) {
         }
      cl_int
   clover::IcdGetPlatformIDsKHR(cl_uint num_entries, cl_platform_id *rd_platforms,
               }
      CLOVER_ICD_API cl_int
   clGetPlatformInfo(cl_platform_id d_platform, cl_platform_info param,
               }
      CLOVER_ICD_API void *
   clGetExtensionFunctionAddress(const char *p_name) {
         }
      CLOVER_ICD_API void *
   clGetExtensionFunctionAddressForPlatform(cl_platform_id d_platform,
               }
      CLOVER_ICD_API cl_int
   clIcdGetPlatformIDsKHR(cl_uint num_entries, cl_platform_id *rd_platforms,
               }
