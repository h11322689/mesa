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
      #include <algorithm>
   #include "core/device.hpp"
   #include "core/platform.hpp"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "util/bitscan.h"
   #include "util/disk_cache.h"
   #include "util/u_debug.h"
   #include "nir.h"
   #include <fstream>
      #ifdef HAVE_CLOVER_SPIRV
   #include "spirv/invocation.hpp"
   #include "nir/invocation.hpp"
   #endif
      using namespace clover;
      namespace {
      template<typename T>
   std::vector<T>
   get_compute_param(pipe_screen *pipe, pipe_shader_ir ir_format,
            int sz = pipe->get_compute_param(pipe, ir_format, cap, NULL);
            pipe->get_compute_param(pipe, ir_format, cap, &v.front());
               cl_version
   get_highest_supported_version(const device &dev) {
      // All the checks below assume that the device supports FULL_PROFILE
   // (which is the only profile support by clover) and that a device is
   // not CUSTOM.
                     const auto has_extension =
      [extensions = dev.supported_extensions()](const char *extension_name){
      return std::find_if(extensions.begin(), extensions.end(),
         [extension_name](const cl_name_version &extension){
   };
            // Check requirements for OpenCL 1.0
   if (dev.max_compute_units() < 1 ||
      dev.max_block_size().size() < 3 ||
   // TODO: Check CL_DEVICE_MAX_WORK_ITEM_SIZES
   dev.max_threads_per_block() < 1 ||
   (dev.address_bits() != 32 && dev.address_bits() != 64) ||
   dev.max_mem_alloc_size() < std::max(dev.max_mem_global() / 4,
         dev.max_mem_input() < 256 ||
   dev.max_const_buffer_size() < 64 * 1024 ||
   dev.max_const_buffers() < 8 ||
   dev.max_mem_local() < 16 * 1024 ||
   dev.clc_version < CL_MAKE_VERSION(1, 0, 0)) {
      }
            // Check requirements for OpenCL 1.1
   if (!has_extension("cl_khr_byte_addressable_store") ||
      !has_extension("cl_khr_global_int32_base_atomics") ||
   !has_extension("cl_khr_global_int32_extended_atomics") ||
   !has_extension("cl_khr_local_int32_base_atomics") ||
   !has_extension("cl_khr_local_int32_extended_atomics") ||
   // OpenCL 1.1 increased the minimum value for
   // CL_DEVICE_MAX_PARAMETER_SIZE to 1024 bytes.
   dev.max_mem_input() < 1024 ||
   dev.mem_base_addr_align() < sizeof(cl_long16) ||
   // OpenCL 1.1 increased the minimum value for
   // CL_DEVICE_LOCAL_MEM_SIZE to 32 KB.
   dev.max_mem_local() < 32 * 1024 ||
   dev.clc_version < CL_MAKE_VERSION(1, 1, 0)) {
      }
            // Check requirements for OpenCL 1.2
   if ((dev.has_doubles() && !has_extension("cl_khr_fp64")) ||
      dev.clc_version < CL_MAKE_VERSION(1, 2, 0) ||
   dev.max_printf_buffer_size() < 1 * 1024 * 1024 ||
   (supports_images &&
   (dev.max_image_buffer_size()  < 65536 ||
            }
            // Check requirements for OpenCL 3.0
   if (dev.max_mem_alloc_size() < std::max(std::min((cl_ulong)1024 * 1024 * 1024,
                  // TODO: If pipes are supported, check:
   //       * CL_DEVICE_MAX_PIPE_ARGS
   //       * CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS
   //       * CL_DEVICE_PIPE_MAX_PACKET_SIZE
   // TODO: If on-device queues are supported, check:
   //       * CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES
   //       * CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE
   //       * CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE
   //       * CL_DEVICE_MAX_ON_DEVICE_QUEUES
   //       * CL_DEVICE_MAX_ON_DEVICE_EVENTS
   dev.clc_version < CL_MAKE_VERSION(3, 0, 0) ||
   (supports_images &&
   (dev.max_images_write() < 64 ||
            }
                        static cl_device_type
   parse_env_device_type() {
      const char* val = getenv("CLOVER_DEVICE_TYPE");
   if (!val) {
         }
   if (strcmp(val, "cpu") == 0) {
         }
   if (strcmp(val, "gpu") == 0) {
         }
   if (strcmp(val, "accelerator") == 0) {
         }
   /* CL_DEVICE_TYPE_CUSTOM isn't implemented
   because CL_DEVICE_TYPE_CUSTOM is OpenCL 1.2
   and Clover is OpenCL 1.1. */
         }
      device::device(clover::platform &platform, pipe_loader_device *ldev) :
      platform(platform), clc_cache(NULL), ldev(ldev) {
   pipe = pipe_loader_create_screen(ldev);
   if (pipe && pipe->get_param(pipe, PIPE_CAP_COMPUTE)) {
      const bool has_supported_ir = supports_ir(PIPE_SHADER_IR_NATIVE) ||
         if (has_supported_ir) {
      unsigned major = 1, minor = 1;
   debug_get_version_option("CLOVER_DEVICE_CLC_VERSION_OVERRIDE",
                  version = get_highest_supported_version(*this);
   major = CL_VERSION_MAJOR(version);
   minor = CL_VERSION_MINOR(version);
   debug_get_version_option("CLOVER_DEVICE_VERSION_OVERRIDE", &major,
                        if (supports_ir(PIPE_SHADER_IR_NATIVE))
   #ifdef HAVE_CLOVER_SPIRV
         if (supports_ir(PIPE_SHADER_IR_NIR_SERIALIZED)) {
      nir::check_for_libclc(*this);
   clc_cache = nir::create_clc_disk_cache();
   clc_nir = lazy<std::shared_ptr<nir_shader>>([&] () { std::string log; return std::shared_ptr<nir_shader>(nir::load_libclc_nir(*this, log), ralloc_free); });
      #endif
      }
   if (pipe)
            }
      device::~device() {
      if (clc_cache)
         if (pipe)
         if (ldev)
      }
      bool
   device::operator==(const device &dev) const {
         }
      cl_device_type
   device::type() const {
      cl_device_type type = parse_env_device_type();
   if (type != 0) {
                  switch (ldev->type) {
   case PIPE_LOADER_DEVICE_SOFTWARE:
         case PIPE_LOADER_DEVICE_PCI:
   case PIPE_LOADER_DEVICE_PLATFORM:
         default:
            }
      cl_uint
   device::vendor_id() const {
      switch (ldev->type) {
   case PIPE_LOADER_DEVICE_SOFTWARE:
   case PIPE_LOADER_DEVICE_PLATFORM:
         case PIPE_LOADER_DEVICE_PCI:
         default:
            }
      size_t
   device::max_images_read() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      size_t
   device::max_images_write() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      size_t
   device::max_image_buffer_size() const {
         }
      cl_uint
   device::max_image_size() const {
         }
      cl_uint
   device::max_image_size_3d() const {
         }
      size_t
   device::max_image_array_number() const {
         }
      cl_uint
   device::max_samplers() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      cl_ulong
   device::max_mem_global() const {
      return get_compute_param<uint64_t>(pipe, ir_format(),
      }
      cl_ulong
   device::max_mem_local() const {
      return get_compute_param<uint64_t>(pipe, ir_format(),
      }
      cl_ulong
   device::max_mem_input() const {
      return get_compute_param<uint64_t>(pipe, ir_format(),
      }
      cl_ulong
   device::max_const_buffer_size() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      cl_uint
   device::max_const_buffers() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      size_t
   device::max_threads_per_block() const {
      return get_compute_param<uint64_t>(
      }
      cl_ulong
   device::max_mem_alloc_size() const {
      return get_compute_param<uint64_t>(pipe, ir_format(),
      }
      cl_uint
   device::max_clock_frequency() const {
      return get_compute_param<uint32_t>(pipe, ir_format(),
      }
      cl_uint
   device::max_compute_units() const {
      return get_compute_param<uint32_t>(pipe, ir_format(),
      }
      cl_uint
   device::max_printf_buffer_size() const {
         }
      bool
   device::image_support() const {
      bool supports_images = get_compute_param<uint32_t>(pipe, ir_format(),
         if (!supports_images)
            /* If the gallium driver supports images, but does not support the
   * minimum requirements for opencl 1.0 images, then don't claim to
   * support images.
   */
   if (max_images_read() < 128 ||
      max_images_write() < 8 ||
   max_image_size() < 8192 ||
   max_image_size_3d() < 2048 ||
   max_samplers() < 16)
            }
      bool
   device::has_doubles() const {
      nir_shader_compiler_options *options =
         (nir_shader_compiler_options *)pipe->get_compiler_options(pipe,
         return pipe->get_param(pipe, PIPE_CAP_DOUBLES) &&
      }
      bool
   device::has_halves() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      bool
   device::has_int64_atomics() const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      bool
   device::has_unified_memory() const {
         }
      size_t
   device::mem_base_addr_align() const {
      uint64_t page_size = 0;
   os_get_page_size(&page_size);
      }
      cl_device_svm_capabilities
   device::svm_support() const {
      // Without CAP_RESOURCE_FROM_USER_MEMORY SVM and CL_MEM_USE_HOST_PTR
   // interactions won't work according to spec as clover manages a GPU side
   // copy of the host data.
   //
   // The biggest problem are memory buffers created with CL_MEM_USE_HOST_PTR,
   // but the application and/or the kernel updates the memory via SVM and not
   // the cl_mem buffer.
   // We can't even do proper tracking on what memory might have been accessed
   // as the host ptr to the buffer could be within a SVM region, where through
   // the CL API there is no reliable way of knowing if a certain cl_mem buffer
   // was accessed by a kernel or not and the runtime can't reliably know from
   // which side the GPU buffer content needs to be updated.
   //
   // Another unsolvable scenario is a cl_mem object passed by cl_mem reference
   // and SVM pointer into the same kernel at the same time.
   if (allows_user_pointers() && pipe->get_param(pipe, PIPE_CAP_SYSTEM_SVM))
      // we can emulate all lower levels if we support fine grain system
   return CL_DEVICE_SVM_FINE_GRAIN_SYSTEM |
               }
      bool
   device::allows_user_pointers() const {
      return pipe->get_param(pipe, PIPE_CAP_RESOURCE_FROM_USER_MEMORY) ||
      }
      std::vector<size_t>
   device::max_block_size() const {
      auto v = get_compute_param<uint64_t>(pipe, ir_format(),
            }
      cl_uint
   device::subgroup_size() const {
      cl_uint subgroup_sizes =
         if (!subgroup_sizes)
            }
      cl_uint
   device::address_bits() const {
      return get_compute_param<uint32_t>(pipe, ir_format(),
      }
      std::string
   device::device_name() const {
         }
      std::string
   device::vendor_name() const {
         }
      enum pipe_shader_ir
   device::ir_format() const {
      if (supports_ir(PIPE_SHADER_IR_NATIVE))
            assert(supports_ir(PIPE_SHADER_IR_NIR_SERIALIZED));
      }
      std::string
   device::ir_target() const {
      std::vector<char> target = get_compute_param<char>(
            }
      enum pipe_endian
   device::endianness() const {
         }
      std::string
   device::device_version_as_string() const {
      static const std::string version_string =
      std::to_string(CL_VERSION_MAJOR(version)) + "." +
         }
      std::string
   device::device_clc_version_as_string() const {
      int major = CL_VERSION_MAJOR(clc_version);
            /* for CL 3.0 we need this to be 1.2 until we support 2.0. */
   if (major == 3) {
      major = 1;
      }
   static const std::string version_string =
      std::to_string(major) + "." +
         }
      bool
   device::supports_ir(enum pipe_shader_ir ir) const {
      return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
      }
      std::vector<cl_name_version>
   device::supported_extensions() const {
               vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_byte_addressable_store" } );
   vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_global_int32_base_atomics" } );
   vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_global_int32_extended_atomics" } );
   vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_local_int32_base_atomics" } );
   vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_local_int32_extended_atomics" } );
   if (has_int64_atomics()) {
      vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_int64_base_atomics" } );
      }
   if (has_doubles())
         if (has_halves())
         if (svm_support())
      #ifdef HAVE_CLOVER_SPIRV
      if (!clover::spirv::supported_versions().empty() &&
      supports_ir(PIPE_SHADER_IR_NIR_SERIALIZED))
   #endif
      vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_extended_versioning" } );
      }
      std::string
   device::supported_extensions_as_string() const {
               if (!extensions_string.empty())
            const auto extension_list = supported_extensions();
   for (const auto &extension : extension_list) {
      if (!extensions_string.empty())
            }
      }
      std::vector<cl_name_version>
   device::supported_il_versions() const {
   #ifdef HAVE_CLOVER_SPIRV
         #else
         #endif
   }
      const void *
   device::get_compiler_options(enum pipe_shader_ir ir) const {
         }
      cl_version
   device::device_version() const {
         }
      cl_version
   device::device_clc_version(bool api) const {
      /*
   * For the API we have to limit this to 1.2,
   * but internally we want 3.0 if it works.
   */
   if (!api)
            int major = CL_VERSION_MAJOR(clc_version);
   /* for CL 3.0 we need this to be 1.2 until we support 2.0. */
   if (major == 3) {
         }
      }
      std::vector<cl_name_version>
   device::opencl_c_all_versions() const {
      std::vector<cl_name_version> vec;
   vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "OpenCL C" } );
            if (CL_VERSION_MAJOR(clc_version) == 1 &&
      CL_VERSION_MINOR(clc_version) == 2)
      if (CL_VERSION_MAJOR(clc_version) == 3) {
      vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 2, 0), "OpenCL C" } );
      }
      }
      std::vector<cl_name_version>
   device::opencl_c_features() const {
               vec.push_back( cl_name_version {CL_MAKE_VERSION(3, 0, 0), "__opencl_c_int64" });
   if (has_doubles())
               }
