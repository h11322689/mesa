   //
   // Copyright 2012-2016 Francisco Jerez
   // Copyright 2012-2016 Advanced Micro Devices, Inc.
   // Copyright 2015 Zoltan Gilian
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
      ///
   /// \file
   /// Codegen back-end-independent part of the construction of an executable
   /// clover::binary, including kernel argument metadata extraction and
   /// formatting of the pre-generated binary code in a form that can be
   /// understood by pipe drivers.
   ///
      #include <llvm/IR/Type.h>
   #include <llvm/Support/Allocator.h>
      #include "llvm/codegen.hpp"
   #include "llvm/compat.hpp"
   #include "llvm/metadata.hpp"
      #include "CL/cl.h"
      #include "pipe/p_state.h"
   #include "util/u_math.h"
      #include <clang/Basic/TargetInfo.h>
      using clover::binary;
   using clover::detokenize;
   using namespace clover::llvm;
      using ::llvm::Module;
   using ::llvm::Function;
   using ::llvm::Type;
   using ::llvm::isa;
   using ::llvm::cast;
   using ::llvm::dyn_cast;
      namespace {
      enum binary::argument::type
   get_image_type(const std::string &type,
            if (type == "image1d_t" || type == "image2d_t" || type == "image3d_t") {
      if (qual == "read_only")
         else if (qual == "write_only")
                           binary::arg_info create_arg_info(const std::string &arg_name,
                                 cl_kernel_arg_type_qualifier cl_type_qualifier =
         if (type_qualifier.find("const") != std::string::npos)
         if (type_qualifier.find("restrict") != std::string::npos)
         if (type_qualifier.find("volatile") != std::string::npos)
            cl_kernel_arg_address_qualifier cl_address_qualifier =
         if (address_qualifier == 1)
         else if (address_qualifier == 2)
         else if (address_qualifier == 3)
            cl_kernel_arg_access_qualifier cl_access_qualifier =
         if (access_qualifier == "read_only")
         else if (access_qualifier == "write_only")
         else if (access_qualifier == "read_write")
            return binary::arg_info(arg_name, type_name, cl_type_qualifier,
               std::vector<size_t>
   get_reqd_work_group_size(const Module &mod,
            const Function &f = *mod.getFunction(kernel_name);
                           std::string
   kernel_attributes(const Module &mod, const std::string &kernel_name) {
                        auto vec_type_hint = get_type_kernel_metadata(f, "vec_type_hint");
   if (!vec_type_hint.empty())
            auto work_group_size_hint = get_uint_vector_kernel_metadata(f, "work_group_size_hint");
   if (!work_group_size_hint.empty()) {
      std::string s = "work_group_size_hint(";
   s += detokenize(work_group_size_hint, ",");
   s += ")";
               auto reqd_work_group_size = get_uint_vector_kernel_metadata(f, "reqd_work_group_size");
   if (!reqd_work_group_size.empty()) {
      std::string s = "reqd_work_group_size(";
   s += detokenize(reqd_work_group_size, ",");
   s += ")";
               auto nosvm = get_str_kernel_metadata(f, "nosvm");
   if (!nosvm.empty())
                        // Parse the type which are pointers to CL vector types with no prefix.
   // so e.g. char/uchar, short/ushort, int/uint, long/ulong
   // half/float/double, followed by the vector length, followed by *.
   // uint8 is 8x32-bit integer, short4 is 4x16-bit integer etc.
   // Since this is a pointer only path, assert the * is on the end.
   ::llvm::Type *
   ptr_arg_to_llvm_type(const Module &mod, std::string type_name) {
      int len = type_name.length();
   assert (type_name[len-1] == '*');
   ::llvm::Type *base_type = NULL;
   if (type_name.find("void") != std::string::npos)
         else if (type_name.find("char") != std::string::npos)
         else if (type_name.find("short") != std::string::npos)
         else if (type_name.find("int") != std::string::npos)
         else if (type_name.find("long") != std::string::npos)
         else if (type_name.find("half") != std::string::npos)
         else if (type_name.find("float") != std::string::npos)
         else if (type_name.find("double") != std::string::npos)
            assert(base_type);
   if (type_name.find("2") != std::string::npos)
         else if (type_name.find("3") != std::string::npos)
         else if (type_name.find("4") != std::string::npos)
         else if (type_name.find("8") != std::string::npos)
         else if (type_name.find("16") != std::string::npos)
                     std::vector<binary::argument>
   make_kernel_args(const Module &mod, const std::string &kernel_name,
            std::vector<binary::argument> args;
   const Function &f = *mod.getFunction(kernel_name);
   ::llvm::DataLayout dl(&mod);
   const auto size_type =
                  for (const auto &arg : f.args()) {
               // OpenCL 1.2 specification, Ch. 6.1.5: "A built-in data
   // type that is not a power of two bytes in size must be
   // aligned to the next larger power of two.
                                 const auto type_name = get_str_argument_metadata(f, arg,
         if (type_name == "image2d_t" || type_name == "image3d_t") {
      // Image.
   const auto access_qual = get_str_argument_metadata(
         args.emplace_back(get_image_type(type_name, access_qual),
               } else if (type_name == "sampler_t") {
      args.emplace_back(binary::argument::sampler, arg_api_size,
               } else if (type_name == "__llvm_image_size") {
      // Image size implicit argument.
   args.emplace_back(binary::argument::scalar, sizeof(cl_uint),
                           } else if (type_name == "__llvm_image_format") {
      // Image format implicit argument.
   args.emplace_back(binary::argument::scalar, sizeof(cl_uint),
                           } else {
      // Other types.
   const auto actual_type =
                  if (actual_type->isPointerTy()) {
                     const auto &map = c.getTarget().getAddressSpaceMap();
   const auto offset =
                           args.emplace_back(binary::argument::local, arg_api_size,
                        } else {
      // XXX: Correctly handle constant address space.  There is no
   // way for r600g to pass a handle for constant buffers back
   // to clover like it can for global buffers, so
   // creating constant arguments will break r600g.  For now,
   // continue treating constant buffers as global buffers
   // until we can come up with a way to create handles for
   // constant buffers.
   args.emplace_back(binary::argument::global, arg_api_size,
                  } else {
                     args.emplace_back(binary::argument::scalar, arg_api_size,
                           // Add kernel argument infos if built with -cl-kernel-arg-info.
   if (c.getCodeGenOpts().EmitOpenCLArgMetadata) {
      args.back().info = create_arg_info(
      get_str_argument_metadata(f, arg, "kernel_arg_name"),
   type_name,
   get_str_argument_metadata(f, arg, "kernel_arg_type_qual"),
   get_uint_argument_metadata(f, arg, "kernel_arg_addr_space"),
                  // Append implicit arguments.  XXX - The types, ordering and
   // vector size of the implicit arguments should depend on the
   // target according to the selected calling convention.
   args.emplace_back(binary::argument::scalar, sizeof(cl_uint),
                              args.emplace_back(binary::argument::scalar, sizeof(cl_uint),
                                          binary::section
   make_text_section(const std::vector<char> &code) {
      const pipe_binary_program_header header { uint32_t(code.size()) };
   binary::section text { 0, binary::section::text_executable,
            text.data.insert(text.data.end(), reinterpret_cast<const char *>(&header),
                        }
      binary
   clover::llvm::build_module_common(const Module &mod,
                                       for (const auto &llvm_name : map(std::mem_fn(&Function::getName),
            const ::std::string name(llvm_name);
   if (offsets.count(name))
      b.syms.emplace_back(name, kernel_attributes(mod, name),
                        b.secs.push_back(make_text_section(code));
      }
