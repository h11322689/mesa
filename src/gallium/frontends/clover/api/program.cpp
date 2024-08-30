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
   #include "core/program.hpp"
   #include "core/platform.hpp"
   #include "spirv/invocation.hpp"
   #include "util/u_debug.h"
      #include <limits>
   #include <sstream>
      using namespace clover;
      namespace {
         std::string
   build_options(const char *p_opts, const char *p_debug) {
      auto opts = std::string(p_opts ? p_opts : "");
                        class build_notifier {
   public:
      build_notifier(cl_program prog,
                  ~build_notifier() {
      if (notifer)
            private:
      cl_program prog_;
   void (CL_CALLBACK * notifer)(cl_program, void *);
               void
   validate_build_common(const program &prog, cl_uint num_devs,
                        if (!pfn_notify && user_data)
            if (prog.kernel_ref_count())
            if (any_of([&](const device &dev) {
                           enum program::il_type
   identify_and_validate_il(const std::string &il,
                        #ifdef HAVE_CLOVER_SPIRV
         if (spirv::is_binary_spirv(il)) {
      std::string log;
   if (!spirv::is_valid_spirv(il, opencl_version, log)) {
      if (notify) {
         }
      }
      #endif
                  }
      CLOVER_API cl_program
   clCreateProgramWithSource(cl_context d_ctx, cl_uint count,
                  auto &ctx = obj(d_ctx);
            if (!count || !strings ||
      any_of(is_zero(), range(strings, count)))
         // Concatenate all the provided fragments together
   for (unsigned i = 0; i < count; ++i)
         source += (lengths && lengths[i] ?
            // ...and create a program object for them.
   ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_program
   clCreateProgramWithBinary(cl_context d_ctx, cl_uint n,
                              auto &ctx = obj(d_ctx);
            if (!lengths || !binaries)
            if (any_of([&](const device &dev) {
                        // Deserialize the provided binaries,
   std::vector<std::pair<cl_int, binary>> result = map(
      [](const unsigned char *p, size_t l) -> std::pair<cl_int, binary> {
                     try {
                           } catch (std::istream::failure &) {
            },
   range(binaries, n),
         // update the status array,
   if (r_status)
            if (any_of(key_equals(CL_INVALID_VALUE), result))
            if (any_of(key_equals(CL_INVALID_BINARY), result))
            // initialize a program object with them.
   ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      cl_program
   clover::CreateProgramWithILKHR(cl_context d_ctx, const void *il,
                     if (!il || !length)
            // Compute the highest OpenCL version supported by all devices associated to
   // the context. That is the version used for validating the SPIR-V binary.
   cl_version min_opencl_version = std::numeric_limits<uint32_t>::max();
   for (const device &dev : ctx.devices()) {
      const cl_version opencl_version = dev.device_version();
               const char *stream = reinterpret_cast<const char *>(il);
   std::string binary(stream, stream + length);
   const enum program::il_type il_type = identify_and_validate_il(binary,
                  if (il_type == program::il_type::none)
            // Initialize a program object with it.
   ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_program
   clCreateProgramWithIL(cl_context d_ctx,
                           }
      CLOVER_API cl_program
   clCreateProgramWithBuiltInKernels(cl_context d_ctx, cl_uint n,
                        auto &ctx = obj(d_ctx);
            if (any_of([&](const device &dev) {
                        // No currently supported built-in kernels.
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
         CLOVER_API cl_int
   clRetainProgram(cl_program d_prog) try {
      obj(d_prog).retain();
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clReleaseProgram(cl_program d_prog) try {
      if (obj(d_prog).release())
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clBuildProgram(cl_program d_prog, cl_uint num_devs,
                  const cl_device_id *d_devs, const char *p_opts,
   auto &prog = obj(d_prog);
   auto devs =
                                    if (prog.il_type() != program::il_type::none) {
      prog.compile(devs, opts);
      } else if (any_of([&](const device &dev){
         return prog.build(dev).binary_type() != CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
      // According to the OpenCL 1.2 specification, “if program is created
   // with clCreateProgramWithBinary, then the program binary must be an
   // executable binary (not a compiled binary or library).”
                     } catch (error &e) {
         }
      CLOVER_API cl_int
   clCompileProgram(cl_program d_prog, cl_uint num_devs,
                  const cl_device_id *d_devs, const char *p_opts,
   cl_uint num_headers, const cl_program *d_header_progs,
   const char **header_names,
   auto &prog = obj(d_prog);
   auto devs =
         const auto opts = build_options(p_opts, "CLOVER_EXTRA_COMPILE_OPTIONS");
                              if (bool(num_headers) != bool(header_names))
            if (prog.il_type() == program::il_type::none)
            for_each([&](const char *name, const program &header) {
                        if (!any_of(key_equals(name), headers))
      headers.push_back(std::pair<std::string, std::string>(
   },
   range(header_names, num_headers),
         prog.compile(devs, opts, headers);
         } catch (invalid_build_options_error &) {
            } catch (build_error &) {
            } catch (error &e) {
         }
      namespace {
      ref_vector<device>
   validate_link_devices(const ref_vector<program> &progs,
                  std::vector<device *> devs;
   const bool create_library =
         const bool enable_link_options =
         const bool has_link_options =
      opts.find("-cl-denorms-are-zero") != std::string::npos ||
   opts.find("-cl-no-signed-zeroes") != std::string::npos ||
   opts.find("-cl-unsafe-math-optimizations") != std::string::npos ||
   opts.find("-cl-finite-math-only") != std::string::npos ||
               // According to the OpenCL 1.2 specification, "[the
   // -enable-link-options] option must be specified with the
   // create-library option".
   if (enable_link_options && !create_library)
            // According to the OpenCL 1.2 specification, "the
   // [program linking options] can be specified when linking a program
   // executable".
   if (has_link_options && create_library)
            for (auto &dev : all_devs) {
      const auto has_binary = [&](const program &prog) {
      const auto t = prog.build(dev).binary_type();
   return t == CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT ||
               // According to the OpenCL 1.2 specification, a library is made of
   // “compiled binaries specified in input_programs argument to
   // clLinkProgram“; compiled binaries does not refer to libraries:
   // “input_programs is an array of program objects that are compiled
   // binaries or libraries that are to be linked to create the program
   // executable”.
   if (create_library && any_of([&](const program &prog) {
            const auto t = prog.build(dev).binary_type();
               // According to the CL 1.2 spec, when "all programs specified [..]
   // contain a compiled binary or library for the device [..] a link is
   // performed",
                  // otherwise if "none of the programs contain a compiled binary or
   // library for that device [..] no link is performed.  All other
   // cases will return a CL_INVALID_OPERATION error."
                  // According to the OpenCL 1.2 specification, "[t]he linker may apply
   // [program linking options] to all compiled program objects
   // specified to clLinkProgram. The linker may apply these options
   // only to libraries which were created with the
   // -enable-link-option."
   else if (has_link_options && any_of([&](const program &prog) {
            const auto t = prog.build(dev).binary_type();
   return !(t == CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT ||
         (t == CL_PROGRAM_BINARY_TYPE_LIBRARY &&
                        }
      CLOVER_API cl_program
   clLinkProgram(cl_context d_ctx, cl_uint num_devs, const cl_device_id *d_devs,
               const char *p_opts, cl_uint num_progs, const cl_program *d_progs,
      auto &ctx = obj(d_ctx);
   const auto opts = build_options(p_opts, "CLOVER_EXTRA_LINK_OPTIONS");
   auto progs = objs(d_progs, num_progs);
   auto all_devs =
         auto prog = create<program>(ctx, all_devs);
                                       try {
      prog().link(devs, opts, progs);
         } catch (build_error &) {
                        } catch (invalid_build_options_error &) {
      ret_error(r_errcode, CL_INVALID_LINKER_OPTIONS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_int
   clUnloadCompiler() {
         }
      CLOVER_API cl_int
   clUnloadPlatformCompiler(cl_platform_id d_platform) try {
      find_platform(d_platform);
      } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetProgramInfo(cl_program d_prog, cl_program_info param,
            property_buffer buf { r_buf, size, r_size };
            switch (param) {
   case CL_PROGRAM_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = prog.ref_count();
         case CL_PROGRAM_CONTEXT:
      buf.as_scalar<cl_context>() = desc(prog.context());
         case CL_PROGRAM_NUM_DEVICES:
      buf.as_scalar<cl_uint>() = (prog.devices().size() ?
                     case CL_PROGRAM_DEVICES:
      buf.as_vector<cl_device_id>() = (prog.devices().size() ?
                     case CL_PROGRAM_SOURCE:
      buf.as_string() = prog.source();
         case CL_PROGRAM_BINARY_SIZES:
      buf.as_vector<size_t>() = map([&](const device &dev) {
            },
            case CL_PROGRAM_BINARIES:
      buf.as_matrix<unsigned char>() = map([&](const device &dev) {
         std::stringbuf bin;
   std::ostream s(&bin);
   prog.build(dev).bin.serialize(s);
      },
            case CL_PROGRAM_NUM_KERNELS:
      buf.as_scalar<cl_uint>() = prog.symbols().size();
         case CL_PROGRAM_KERNEL_NAMES:
      buf.as_string() = fold([](const std::string &a, const binary::symbol &s) {
                     case CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT:
   case CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT:
      buf.as_scalar<cl_bool>() = CL_FALSE;
         case CL_PROGRAM_IL:
      if (prog.il_type() == program::il_type::spirv)
         else if (r_size)
            default:
                        } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetProgramBuildInfo(cl_program d_prog, cl_device_id d_dev,
                  property_buffer buf { r_buf, size, r_size };
   auto &prog = obj(d_prog);
            if (!count(dev, prog.context().devices()))
            switch (param) {
   case CL_PROGRAM_BUILD_STATUS:
      buf.as_scalar<cl_build_status>() = prog.build(dev).status();
         case CL_PROGRAM_BUILD_OPTIONS:
      buf.as_string() = prog.build(dev).opts;
         case CL_PROGRAM_BUILD_LOG:
      buf.as_string() = prog.build(dev).log;
         case CL_PROGRAM_BINARY_TYPE:
      buf.as_scalar<cl_program_binary_type>() = prog.build(dev).binary_type();
         case CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE:
      buf.as_scalar<size_t>() = 0;
         default:
                        } catch (error &e) {
         }
