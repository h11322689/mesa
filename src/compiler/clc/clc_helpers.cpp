   //
   // Copyright 2012-2016 Francisco Jerez
   // Copyright 2012-2016 Advanced Micro Devices, Inc.
   // Copyright 2014-2016 Jan Vesely
   // Copyright 2014-2015 Serge Martin
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
      #include <cstdlib>
   #include <filesystem>
   #include <sstream>
   #include <mutex>
      #include <llvm/ADT/ArrayRef.h>
   #include <llvm/IR/DiagnosticPrinter.h>
   #include <llvm/IR/DiagnosticInfo.h>
   #include <llvm/IR/LLVMContext.h>
   #include <llvm/IR/Type.h>
   #include <llvm/Support/raw_ostream.h>
   #include <llvm/Bitcode/BitcodeWriter.h>
   #include <llvm/Bitcode/BitcodeReader.h>
   #include <llvm-c/Core.h>
   #include <llvm-c/Target.h>
   #include <LLVMSPIRVLib/LLVMSPIRVLib.h>
      #include <clang/Config/config.h>
   #include <clang/Driver/Driver.h>
   #include <clang/CodeGen/CodeGenAction.h>
   #include <clang/Lex/PreprocessorOptions.h>
   #include <clang/Frontend/CompilerInstance.h>
   #include <clang/Frontend/TextDiagnosticBuffer.h>
   #include <clang/Frontend/TextDiagnosticPrinter.h>
   #include <clang/Basic/TargetInfo.h>
      #include <spirv-tools/libspirv.hpp>
   #include <spirv-tools/linker.hpp>
   #include <spirv-tools/optimizer.hpp>
      #include "util/macros.h"
   #include "glsl_types.h"
      #include "spirv.h"
      #if DETECT_OS_UNIX
   #include <dlfcn.h>
   #endif
      #ifdef USE_STATIC_OPENCL_C_H
   #if LLVM_VERSION_MAJOR < 15
   #include "opencl-c.h.h"
   #endif
   #include "opencl-c-base.h.h"
   #endif
      #include "clc_helpers.h"
      namespace fs = std::filesystem;
      /* Use the highest version of SPIRV supported by SPIRV-Tools. */
   constexpr spv_target_env spirv_target = SPV_ENV_UNIVERSAL_1_5;
      constexpr SPIRV::VersionNumber invalid_spirv_trans_version = static_cast<SPIRV::VersionNumber>(0);
      using ::llvm::Function;
   using ::llvm::LLVMContext;
   using ::llvm::Module;
   using ::llvm::raw_string_ostream;
   using ::clang::driver::Driver;
      static void
   llvm_log_handler(const ::llvm::DiagnosticInfo &di, void *data) {
               std::string log;
   raw_string_ostream os { log };
   ::llvm::DiagnosticPrinterRawOStream printer { os };
               }
      class SPIRVKernelArg {
   public:
      SPIRVKernelArg(uint32_t id, uint32_t typeId) : id(id), typeId(typeId),
                              uint32_t id;
   uint32_t typeId;
   std::string name;
   std::string typeName;
   enum clc_kernel_arg_address_qualifier addrQualifier;
   unsigned accessQualifier;
      };
      class SPIRVKernelInfo {
   public:
      SPIRVKernelInfo(uint32_t fid, const char *nm)
                  uint32_t funcId;
   std::string name;
   std::vector<SPIRVKernelArg> args;
   unsigned vecHint;
   unsigned localSize[3];
      };
      class SPIRVKernelParser {
   public:
      SPIRVKernelParser() : curKernel(NULL)
   {
                  ~SPIRVKernelParser()
   {
   spvContextDestroy(ctx);
            void parseEntryPoint(const spv_parsed_instruction_t *ins)
   {
                                          for (auto &iter : kernels) {
      if (funcId == iter.funcId)
               op = &ins->operands[2];
   assert(op->type == SPV_OPERAND_TYPE_LITERAL_STRING);
                        void parseFunction(const spv_parsed_instruction_t *ins)
   {
                                          for (auto &kernel : kernels) {
      if (funcId == kernel.funcId && !kernel.args.size()) {
   return;
                        void parseFunctionParam(const spv_parsed_instruction_t *ins)
   {
      const spv_parsed_operand_t *op;
            if (!curKernel)
            assert(ins->num_operands == 2);
   op = &ins->operands[0];
   assert(op->type == SPV_OPERAND_TYPE_TYPE_ID);
   typeId = ins->words[op->offset];
   op = &ins->operands[1];
   assert(op->type == SPV_OPERAND_TYPE_RESULT_ID);
   id = ins->words[op->offset];
               void parseName(const spv_parsed_instruction_t *ins)
   {
      const spv_parsed_operand_t *op;
   const char *name;
                     op = &ins->operands[0];
   assert(op->type == SPV_OPERAND_TYPE_ID);
   id = ins->words[op->offset];
   op = &ins->operands[1];
   assert(op->type == SPV_OPERAND_TYPE_LITERAL_STRING);
            for (auto &kernel : kernels) {
      for (auto &arg : kernel.args) {
      if (arg.id == id && arg.name.empty()) {
      }
                        void parseTypePointer(const spv_parsed_instruction_t *ins)
   {
      enum clc_kernel_arg_address_qualifier addrQualifier;
   uint32_t typeId, storageClass;
                     op = &ins->operands[0];
   assert(op->type == SPV_OPERAND_TYPE_RESULT_ID);
            op = &ins->operands[1];
   assert(op->type == SPV_OPERAND_TYPE_STORAGE_CLASS);
   storageClass = ins->words[op->offset];
   switch (storageClass) {
   case SpvStorageClassCrossWorkgroup:
      addrQualifier = CLC_KERNEL_ARG_ADDRESS_GLOBAL;
      case SpvStorageClassWorkgroup:
      addrQualifier = CLC_KERNEL_ARG_ADDRESS_LOCAL;
      case SpvStorageClassUniformConstant:
      addrQualifier = CLC_KERNEL_ARG_ADDRESS_CONSTANT;
      default:
      addrQualifier = CLC_KERNEL_ARG_ADDRESS_PRIVATE;
               for (auto &arg : kernel.args) {
               if (arg.typeId == typeId) {
      arg.addrQualifier = addrQualifier;
   if (addrQualifier == CLC_KERNEL_ARG_ADDRESS_CONSTANT)
                        void parseOpString(const spv_parsed_instruction_t *ins)
   {
      const spv_parsed_operand_t *op;
                     op = &ins->operands[1];
   assert(op->type == SPV_OPERAND_TYPE_LITERAL_STRING);
            size_t start = 0;
   enum class string_type {
      arg_type,
               if (str.find("kernel_arg_type.") == 0) {
      start = sizeof("kernel_arg_type.") - 1;
      } else if (str.find("kernel_arg_type_qual.") == 0) {
      start = sizeof("kernel_arg_type_qual.") - 1;
      } else {
                  for (auto &kernel : kernels) {
      pos = str.find(kernel.name, start);
            if (pos == std::string::npos ||
         pos = start + kernel.name.size();
                           for (auto &arg : kernel.args) {
                  if (entryEnd == std::string::npos)
                                    if (str_type == string_type::arg_type) {
         } else if (str_type == string_type::arg_type_qual) {
      if (entryVal.find("const") != std::string::npos)
                        void applyDecoration(uint32_t id, const spv_parsed_instruction_t *ins)
   {
      auto iter = decorationGroups.find(id);
   if (iter != decorationGroups.end()) {
      for (uint32_t entry : iter->second)
                     const spv_parsed_operand_t *op;
                     op = &ins->operands[1];
   assert(op->type == SPV_OPERAND_TYPE_DECORATION);
            if (decoration == SpvDecorationSpecId) {
      uint32_t spec_id = ins->words[ins->operands[2].offset];
   for (auto &c : specConstants) {
      if (c.second.id == spec_id) {
            }
   specConstants.emplace_back(id, clc_parsed_spec_constant{ spec_id });
               for (auto &kernel : kernels) {
      for (auto &arg : kernel.args) {
      if (arg.id == id) {
      switch (decoration) {
   case SpvDecorationVolatile:
      arg.typeQualifier |= CLC_KERNEL_ARG_TYPE_VOLATILE;
      case SpvDecorationConstant:
      arg.typeQualifier |= CLC_KERNEL_ARG_TYPE_CONST;
      case SpvDecorationRestrict:
      arg.typeQualifier |= CLC_KERNEL_ARG_TYPE_RESTRICT;
      case SpvDecorationFuncParamAttr:
      op = &ins->operands[2];
   assert(op->type == SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE);
   switch (ins->words[op->offset]) {
   case SpvFunctionParameterAttributeNoAlias:
      arg.typeQualifier |= CLC_KERNEL_ARG_TYPE_RESTRICT;
      case SpvFunctionParameterAttributeNoWrite:
      arg.typeQualifier |= CLC_KERNEL_ARG_TYPE_CONST;
      }
                              void parseOpDecorate(const spv_parsed_instruction_t *ins)
   {
      const spv_parsed_operand_t *op;
                     op = &ins->operands[0];
   assert(op->type == SPV_OPERAND_TYPE_ID);
                        void parseOpGroupDecorate(const spv_parsed_instruction_t *ins)
   {
               const spv_parsed_operand_t *op = &ins->operands[0];
   assert(op->type == SPV_OPERAND_TYPE_ID);
            auto lowerBound = decorationGroups.lower_bound(groupId);
   if (lowerBound != decorationGroups.end() &&
      lowerBound->first == groupId)
               auto iter = decorationGroups.emplace_hint(lowerBound, groupId, std::vector<uint32_t>{});
   auto& vec = iter->second;
   vec.reserve(ins->num_operands - 1);
   for (uint32_t i = 1; i < ins->num_operands; ++i) {
      op = &ins->operands[i];
   assert(op->type == SPV_OPERAND_TYPE_ID);
                  void parseOpTypeImage(const spv_parsed_instruction_t *ins)
   {
      const spv_parsed_operand_t *op;
   uint32_t typeId;
            op = &ins->operands[0];
   assert(op->type == SPV_OPERAND_TYPE_RESULT_ID);
            if (ins->num_operands >= 9) {
      op = &ins->operands[8];
   assert(op->type == SPV_OPERAND_TYPE_ACCESS_QUALIFIER);
   switch (ins->words[op->offset]) {
   case SpvAccessQualifierReadOnly:
      accessQualifier = CLC_KERNEL_ARG_ACCESS_READ;
      case SpvAccessQualifierWriteOnly:
      accessQualifier = CLC_KERNEL_ARG_ACCESS_WRITE;
      case SpvAccessQualifierReadWrite:
      accessQualifier = CLC_KERNEL_ARG_ACCESS_WRITE |
                        for (auto &arg : kernel.args) {
               if (arg.typeId == typeId) {
      arg.accessQualifier = accessQualifier;
                        void parseExecutionMode(const spv_parsed_instruction_t *ins)
   {
      uint32_t executionMode = ins->words[ins->operands[1].offset];
            for (auto& kernel : kernels) {
      if (kernel.funcId == funcId) {
      switch (executionMode) {
   case SpvExecutionModeVecTypeHint:
      kernel.vecHint = ins->words[ins->operands[2].offset];
      case SpvExecutionModeLocalSize:
      kernel.localSize[0] = ins->words[ins->operands[2].offset];
   kernel.localSize[1] = ins->words[ins->operands[3].offset];
      case SpvExecutionModeLocalSizeHint:
      kernel.localSizeHint[0] = ins->words[ins->operands[2].offset];
   kernel.localSizeHint[1] = ins->words[ins->operands[3].offset];
      default:
                           void parseLiteralType(const spv_parsed_instruction_t *ins)
   {
      uint32_t typeId = ins->words[ins->operands[0].offset];
   auto& literalType = literalTypes[typeId];
   switch (ins->opcode) {
   case SpvOpTypeBool:
      literalType = CLC_SPEC_CONSTANT_BOOL;
      case SpvOpTypeFloat: {
      uint32_t sizeInBits = ins->words[ins->operands[1].offset];
   switch (sizeInBits) {
   case 32:
      literalType = CLC_SPEC_CONSTANT_FLOAT;
      case 64:
      literalType = CLC_SPEC_CONSTANT_DOUBLE;
      case 16:
      /* Can't be used for a spec constant */
      default:
         }
      }
   case SpvOpTypeInt: {
      uint32_t sizeInBits = ins->words[ins->operands[1].offset];
   bool isSigned = ins->words[ins->operands[2].offset];
   if (isSigned) {
      switch (sizeInBits) {
   case 8:
      literalType = CLC_SPEC_CONSTANT_INT8;
      case 16:
      literalType = CLC_SPEC_CONSTANT_INT16;
      case 32:
      literalType = CLC_SPEC_CONSTANT_INT32;
      case 64:
      literalType = CLC_SPEC_CONSTANT_INT64;
      default:
            } else {
      switch (sizeInBits) {
   case 8:
      literalType = CLC_SPEC_CONSTANT_UINT8;
      case 16:
      literalType = CLC_SPEC_CONSTANT_UINT16;
      case 32:
      literalType = CLC_SPEC_CONSTANT_UINT32;
      case 64:
      literalType = CLC_SPEC_CONSTANT_UINT64;
      default:
            }
      }
   default:
                     void parseSpecConstant(const spv_parsed_instruction_t *ins)
   {
      uint32_t id = ins->result_id;
   for (auto& c : specConstants) {
      if (c.first == id) {
      auto& data = c.second;
   switch (ins->opcode) {
                                          data.type = typeIter->second;
      }
   case SpvOpSpecConstantFalse:
   case SpvOpSpecConstantTrue:
      data.type = CLC_SPEC_CONSTANT_BOOL;
      default:
                           static spv_result_t
   parseInstruction(void *data, const spv_parsed_instruction_t *ins)
   {
               switch (ins->opcode) {
   case SpvOpName:
      parser->parseName(ins);
      case SpvOpEntryPoint:
      parser->parseEntryPoint(ins);
      case SpvOpFunction:
      parser->parseFunction(ins);
      case SpvOpFunctionParameter:
      parser->parseFunctionParam(ins);
      case SpvOpFunctionEnd:
   case SpvOpLabel:
      parser->curKernel = NULL;
      case SpvOpTypePointer:
      parser->parseTypePointer(ins);
      case SpvOpTypeImage:
      parser->parseOpTypeImage(ins);
      case SpvOpString:
      parser->parseOpString(ins);
      case SpvOpDecorate:
      parser->parseOpDecorate(ins);
      case SpvOpGroupDecorate:
      parser->parseOpGroupDecorate(ins);
      case SpvOpExecutionMode:
      parser->parseExecutionMode(ins);
      case SpvOpTypeBool:
   case SpvOpTypeInt:
   case SpvOpTypeFloat:
      parser->parseLiteralType(ins);
      case SpvOpSpecConstant:
   case SpvOpSpecConstantFalse:
   case SpvOpSpecConstantTrue:
      parser->parseSpecConstant(ins);
      default:
                              bool parseBinary(const struct clc_binary &spvbin, const struct clc_logger *logger)
   {
      /* 3 passes should be enough to retrieve all kernel information:
   * 1st pass: all entry point name and number of args
   * 2nd pass: argument names and type names
   * 3rd pass: pointer type names
   */
   for (unsigned pass = 0; pass < 3; pass++) {
      spv_diagnostic diagnostic = NULL;
   auto result = spvBinaryParse(ctx, reinterpret_cast<void *>(this),
                  if (result != SPV_SUCCESS) {
      if (diagnostic && logger)
                                    std::vector<SPIRVKernelInfo> kernels;
   std::vector<std::pair<uint32_t, clc_parsed_spec_constant>> specConstants;
   std::map<uint32_t, enum clc_spec_constant_type> literalTypes;
   std::map<uint32_t, std::vector<uint32_t>> decorationGroups;
   SPIRVKernelInfo *curKernel;
      };
      bool
   clc_spirv_get_kernels_info(const struct clc_binary *spvbin,
                                 {
      struct clc_kernel_info *kernels;
                     if (!parser.parseBinary(*spvbin, logger))
            *num_kernels = parser.kernels.size();
   *num_spec_constants = parser.specConstants.size();
   if (!*num_kernels)
            kernels = reinterpret_cast<struct clc_kernel_info *>(calloc(*num_kernels,
         assert(kernels);
   for (unsigned i = 0; i < parser.kernels.size(); i++) {
      kernels[i].name = strdup(parser.kernels[i].name.c_str());
   kernels[i].num_args = parser.kernels[i].args.size();
   kernels[i].vec_hint_size = parser.kernels[i].vecHint >> 16;
   kernels[i].vec_hint_type = (enum clc_vec_hint_type)(parser.kernels[i].vecHint & 0xFFFF);
   memcpy(kernels[i].local_size, parser.kernels[i].localSize, sizeof(kernels[i].local_size));
   memcpy(kernels[i].local_size_hint, parser.kernels[i].localSizeHint, sizeof(kernels[i].local_size_hint));
   if (!kernels[i].num_args)
                     args = reinterpret_cast<struct clc_kernel_arg *>(calloc(kernels[i].num_args,
         kernels[i].args = args;
   assert(args);
   for (unsigned j = 0; j < kernels[i].num_args; j++) {
      if (!parser.kernels[i].args[j].name.empty())
         args[j].type_name = strdup(parser.kernels[i].args[j].typeName.c_str());
   args[j].address_qualifier = parser.kernels[i].args[j].addrQualifier;
   args[j].type_qualifier = parser.kernels[i].args[j].typeQualifier;
                  if (*num_spec_constants) {
      spec_constants = reinterpret_cast<struct clc_parsed_spec_constant *>(calloc(*num_spec_constants,
                  for (unsigned i = 0; i < parser.specConstants.size(); ++i) {
                     *out_kernels = kernels;
               }
      void
   clc_free_kernels_info(const struct clc_kernel_info *kernels,
         {
      if (!kernels)
            for (unsigned i = 0; i < num_kernels; i++) {
      if (kernels[i].args) {
      for (unsigned j = 0; j < kernels[i].num_args; j++) {
      free((void *)kernels[i].args[j].name);
      }
      }
                  }
      static std::unique_ptr<::llvm::Module>
   clc_compile_to_llvm_module(LLVMContext &llvm_ctx,
               {
      static_assert(std::has_unique_object_representations<clc_optional_features>(),
            std::string diag_log_str;
                     clang::DiagnosticsEngine diag {
      new clang::DiagnosticIDs,
   new clang::DiagnosticOptions,
   new clang::TextDiagnosticPrinter(diag_log_stream,
                        std::vector<const char *> clang_opts = {
      args->source.name,
   "-triple", triple,
   // By default, clang prefers to use modules to pull in the default headers,
   #if LLVM_VERSION_MAJOR >= 15
         #else
         #endif
   #if LLVM_VERSION_MAJOR >= 15 && LLVM_VERSION_MAJOR < 17
         #endif
         // Add a default CL compiler version. Clang will pick the last one specified
   // on the command line, so the app can override this one.
   "-cl-std=cl1.2",
   // The LLVM-SPIRV-Translator doesn't support memset with variable size
   "-fno-builtin-memset",
   // LLVM's optimizations can produce code that the translator can't translate
   "-O0",
   // Ensure inline functions are actually emitted
   "-fgnu89-inline",
   // Undefine clang added SPIR(V) defines so we don't magically enable extensions
   "-U__SPIR__",
                        clang_opts.push_back("-Dcl_khr_expect_assume=1");
   if (args->features.integer_dot_product) {
      clang_opts.push_back("-Dcl_khr_integer_dot_product=1");
   clang_opts.push_back("-D__opencl_c_integer_dot_product_input_4x8bit_packed=1");
               // We assume there's appropriate defines for __OPENCL_VERSION__ and __IMAGE_SUPPORT__
   // being provided by the caller here.
               #if LLVM_VERSION_MAJOR >= 10
         #else
               #endif
               clc_error(logger, "Couldn't create Clang invocation.\n");
               if (diag.hasErrorOccurred()) {
      clc_error(logger, "%sErrors occurred during Clang invocation.\n",
                     // This is a workaround for a Clang bug which causes the number
   // of warnings and errors to be printed to stderr.
   // http://www.llvm.org/bugs/show_bug.cgi?id=19735
            c->createDiagnostics(new clang::TextDiagnosticPrinter(
                  c->setTarget(clang::TargetInfo::CreateTargetInfo(
                  #ifdef USE_STATIC_OPENCL_C_H
      c->getHeaderSearchOpts().UseBuiltinIncludes = false;
            // Add opencl-c generic search path
   {
      ::llvm::SmallString<128> system_header_path;
   ::llvm::sys::path::system_temp_directory(true, system_header_path);
   ::llvm::sys::path::append(system_header_path, "openclon12");
   c->getHeaderSearchOpts().AddPath(system_header_path.str(),
            #if LLVM_VERSION_MAJOR < 15
         ::llvm::sys::path::append(system_header_path, "opencl-c.h");
   c->getPreprocessorOpts().addRemappedFile(system_header_path.str(),
         #endif
            ::llvm::sys::path::append(system_header_path, "opencl-c-base.h");
   c->getPreprocessorOpts().addRemappedFile(system_header_path.str(),
      #if LLVM_VERSION_MAJOR >= 15
         #endif
         #else
         Dl_info info;
   if (dladdr((void *)clang::CompilerInvocation::CreateFromArgs, &info) == 0) {
      clc_error(logger, "Couldn't find libclang path.\n");
               char *clang_path = realpath(info.dli_fname, NULL);
   if (clang_path == nullptr) {
      clc_error(logger, "Couldn't find libclang path.\n");
               // GetResourcePath is a way to retrive the actual libclang resource dir based on a given binary
   // or library.
   auto clang_res_path =
                  c->getHeaderSearchOpts().UseBuiltinIncludes = true;
   c->getHeaderSearchOpts().UseStandardSystemIncludes = true;
            // Add opencl-c generic search path
   c->getHeaderSearchOpts().AddPath(clang_res_path.string(),
                  #if LLVM_VERSION_MAJOR >= 15
         #else
         #endif
   #endif
      #if LLVM_VERSION_MAJOR >= 14
      c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("-all");
   c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_byte_addressable_store");
   c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_global_int32_base_atomics");
   c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_global_int32_extended_atomics");
   c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_local_int32_base_atomics");
   c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_local_int32_extended_atomics");
   if (args->features.fp16) {
         }
   if (args->features.fp64) {
      c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_fp64");
      }
   if (args->features.int64) {
      c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cles_khr_int64");
      }
   if (args->features.images) {
         }
   if (args->features.images_read_write) {
         }
   if (args->features.images_write_3d) {
      c->getTargetOpts().OpenCLExtensionsAsWritten.push_back("+cl_khr_3d_image_writes");
      }
   if (args->features.intel_subgroups) {
         }
   if (args->features.subgroups) {
         }
   if (args->features.subgroups_ifp) {
      assert(args->features.subgroups);
         #endif
         if (args->num_headers) {
      ::llvm::SmallString<128> tmp_header_path;
   ::llvm::sys::path::system_temp_directory(true, tmp_header_path);
            c->getHeaderSearchOpts().AddPath(tmp_header_path.str(),
                  for (size_t i = 0; i < args->num_headers; i++) {
      auto path_copy = tmp_header_path;
   ::llvm::sys::path::append(path_copy, ::llvm::sys::path::convert_to_slash(args->headers[i].name));
   c->getPreprocessorOpts().addRemappedFile(path_copy.str(),
                  c->getPreprocessorOpts().addRemappedFile(
                  // Compile the code
   clang::EmitLLVMOnlyAction act(&llvm_ctx);
   if (!c->ExecuteAction(act)) {
      clc_error(logger, "%sError executing LLVM compilation action.\n",
                        }
      static SPIRV::VersionNumber
   spirv_version_to_llvm_spirv_translator_version(enum clc_spirv_version version)
   {
      switch (version) {
   case CLC_SPIRV_VERSION_MAX: return SPIRV::VersionNumber::MaximumVersion;
   case CLC_SPIRV_VERSION_1_0: return SPIRV::VersionNumber::SPIRV_1_0;
   case CLC_SPIRV_VERSION_1_1: return SPIRV::VersionNumber::SPIRV_1_1;
   case CLC_SPIRV_VERSION_1_2: return SPIRV::VersionNumber::SPIRV_1_2;
      #ifdef HAS_SPIRV_1_4
         #endif
      default:      return invalid_spirv_trans_version;
      }
      static int
   llvm_mod_to_spirv(std::unique_ptr<::llvm::Module> mod,
                     LLVMContext &context,
   {
               SPIRV::VersionNumber version =
         if (version == invalid_spirv_trans_version) {
      clc_error(logger, "Invalid/unsupported SPIRV specified.\n");
               const char *const *extensions = NULL;
   if (args)
         if (!extensions) {
      /* The SPIR-V parser doesn't handle all extensions */
   static const char *default_extensions[] = {
      "SPV_EXT_shader_atomic_float_add",
   "SPV_EXT_shader_atomic_float_min_max",
   "SPV_KHR_float_controls",
      };
               SPIRV::TranslatorOpts::ExtensionsStatusMap ext_map;
      #define EXT(X) \
         if (strcmp(#X, extensions[i]) == 0) \
   #include "LLVMSPIRVLib/LLVMSPIRVExtensions.inc"
   #undef EXT
      }
         #if LLVM_VERSION_MAJOR >= 13
      /* This was the default in 12.0 and older, but currently we'll fail to parse without this */
      #endif
         std::ostringstream spv_stream;
   if (!::llvm::writeSpirv(mod.get(), spirv_opts, spv_stream, log)) {
      clc_error(logger, "%sTranslation from LLVM IR to SPIR-V failed.\n",
                     const std::string spv_out = spv_stream.str();
   out_spirv->size = spv_out.size();
   out_spirv->data = malloc(out_spirv->size);
               }
      int
   clc_c_to_spir(const struct clc_compile_args *args,
               {
               LLVMContext llvm_ctx;
   llvm_ctx.setDiagnosticHandlerCallBack(llvm_log_handler,
            auto mod = clc_compile_to_llvm_module(llvm_ctx, args, logger);
   if (!mod)
            ::llvm::SmallVector<char, 0> buffer;
   ::llvm::BitcodeWriter writer(buffer);
            out_spir->size = buffer.size_in_bytes();
   out_spir->data = malloc(out_spir->size);
               }
      int
   clc_c_to_spirv(const struct clc_compile_args *args,
               {
               LLVMContext llvm_ctx;
   llvm_ctx.setDiagnosticHandlerCallBack(llvm_log_handler,
            auto mod = clc_compile_to_llvm_module(llvm_ctx, args, logger);
   if (!mod)
            }
      int
   clc_spir_to_spirv(const struct clc_binary *in_spir,
               {
               LLVMContext llvm_ctx;
   llvm_ctx.setDiagnosticHandlerCallBack(llvm_log_handler,
            ::llvm::StringRef spir_ref(static_cast<const char*>(in_spir->data), in_spir->size);
   auto mod = ::llvm::parseBitcodeFile(::llvm::MemoryBufferRef(spir_ref, "<spir>"), llvm_ctx);
   if (!mod)
               }
      class SPIRVMessageConsumer {
   public:
               void operator()(spv_message_level_t level, const char *src,
         {
      if (level == SPV_MSG_INFO || level == SPV_MSG_DEBUG)
            std::ostringstream message;
   message << "(file=" << (src ? src : "input")
         << ",line=" << pos.line
   << ",column=" << pos.column
            if (level == SPV_MSG_WARNING)
         else
            private:
         };
      int
   clc_link_spirv_binaries(const struct clc_linker_args *args,
               {
               for (unsigned i = 0; i < args->num_in_objs; i++) {
      const uint32_t *data = static_cast<const uint32_t *>(args->in_objs[i]->data);
   std::vector<uint32_t> bin(data, data + (args->in_objs[i]->size / 4));
               SPIRVMessageConsumer msgconsumer(logger);
   spvtools::Context context(spirv_target);
   context.SetMessageConsumer(msgconsumer);
   spvtools::LinkerOptions options;
   options.SetAllowPartialLinkage(args->create_library);
   options.SetCreateLibrary(args->create_library);
   std::vector<uint32_t> linkingResult;
   spv_result_t status = spvtools::Link(context, binaries, &linkingResult, options);
   if (status != SPV_SUCCESS) {
                  out_spirv->size = linkingResult.size() * 4;
   out_spirv->data = static_cast<uint32_t *>(malloc(out_spirv->size));
               }
      bool
   clc_validate_spirv(const struct clc_binary *spirv,
               {
      SPIRVMessageConsumer msgconsumer(logger);
   spvtools::SpirvTools tools(spirv_target);
   tools.SetMessageConsumer(msgconsumer);
   spvtools::ValidatorOptions spirv_options;
            if (options) {
      spirv_options.SetUniversalLimit(
      spv_validator_limit_max_function_args,
               }
      int
   clc_spirv_specialize(const struct clc_binary *in_spirv,
                     {
      std::unordered_map<uint32_t, std::vector<uint32_t>> spec_const_map;
   for (unsigned i = 0; i < consts->num_specializations; ++i) {
      unsigned id = consts->specializations[i].id;
   auto parsed_spec_const = std::find_if(parsed_data->spec_constants,
      parsed_data->spec_constants + parsed_data->num_spec_constants,
               std::vector<uint32_t> words;
   switch (parsed_spec_const->type) {
   case CLC_SPEC_CONSTANT_BOOL:
      words.push_back(consts->specializations[i].value.b);
      case CLC_SPEC_CONSTANT_INT32:
   case CLC_SPEC_CONSTANT_UINT32:
   case CLC_SPEC_CONSTANT_FLOAT:
      words.push_back(consts->specializations[i].value.u32);
      case CLC_SPEC_CONSTANT_INT16:
      words.push_back((uint32_t)(int32_t)consts->specializations[i].value.i16);
      case CLC_SPEC_CONSTANT_INT8:
      words.push_back((uint32_t)(int32_t)consts->specializations[i].value.i8);
      case CLC_SPEC_CONSTANT_UINT16:
      words.push_back((uint32_t)consts->specializations[i].value.u16);
      case CLC_SPEC_CONSTANT_UINT8:
      words.push_back((uint32_t)consts->specializations[i].value.u8);
      case CLC_SPEC_CONSTANT_DOUBLE:
   case CLC_SPEC_CONSTANT_INT64:
   case CLC_SPEC_CONSTANT_UINT64:
      words.resize(2);
   memcpy(words.data(), &consts->specializations[i].value.u64, 8);
      case CLC_SPEC_CONSTANT_UNKNOWN:
      assert(0);
               ASSERTED auto ret = spec_const_map.emplace(id, std::move(words));
               spvtools::Optimizer opt(spirv_target);
            std::vector<uint32_t> result;
   if (!opt.Run(static_cast<const uint32_t*>(in_spirv->data), in_spirv->size / 4, &result))
            out_spirv->size = result.size() * 4;
   out_spirv->data = malloc(out_spirv->size);
   memcpy(out_spirv->data, result.data(), out_spirv->size);
      }
      void
   clc_dump_spirv(const struct clc_binary *spvbin, FILE *f)
   {
      spvtools::SpirvTools tools(spirv_target);
   const uint32_t *data = static_cast<const uint32_t *>(spvbin->data);
   std::vector<uint32_t> bin(data, data + (spvbin->size / 4));
   std::string out;
   tools.Disassemble(bin, &out,
                  }
      void
   clc_free_spir_binary(struct clc_binary *spir)
   {
         }
      void
   clc_free_spirv_binary(struct clc_binary *spvbin)
   {
         }
      void
   initialize_llvm_once(void)
   {
      LLVMInitializeAllTargets();
   LLVMInitializeAllTargetInfos();
   LLVMInitializeAllTargetMCs();
   LLVMInitializeAllAsmParsers();
      }
      std::once_flag initialize_llvm_once_flag;
      void
   clc_initialize_llvm(void)
   {
      std::call_once(initialize_llvm_once_flag,
      }
