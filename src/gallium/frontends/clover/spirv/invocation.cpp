   //
   // Copyright 2018 Pierre Moreau
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
      #include "invocation.hpp"
      #include <limits>
   #include <unordered_map>
   #include <unordered_set>
   #include <vector>
      #ifdef HAVE_CLOVER_SPIRV
   #include <spirv-tools/libspirv.hpp>
   #include <spirv-tools/linker.hpp>
   #endif
      #include "core/error.hpp"
   #include "core/platform.hpp"
   #include "invocation.hpp"
   #include "llvm/util.hpp"
   #include "pipe/p_state.h"
   #include "util/algorithm.hpp"
   #include "util/functional.hpp"
   #include "util/u_math.h"
      #include "compiler/spirv/spirv.h"
      #define SPIRV_HEADER_WORD_SIZE 5
      using namespace clover;
      using clover::detokenize;
      #ifdef HAVE_CLOVER_SPIRV
   namespace {
         static const std::array<std::string,7> type_strs = {
                  template<typename T>
   T get(const char *source, size_t index) {
      const uint32_t *word_ptr = reinterpret_cast<const uint32_t *>(source);
               enum binary::argument::type
   convert_storage_class(SpvStorageClass storage_class, std::string &err) {
      switch (storage_class) {
   case SpvStorageClassFunction:
         case SpvStorageClassUniformConstant:
         case SpvStorageClassWorkgroup:
         case SpvStorageClassCrossWorkgroup:
         default:
      err += "Invalid storage type " + std::to_string(storage_class) + "\n";
                  cl_kernel_arg_address_qualifier
   convert_storage_class_to_cl(SpvStorageClass storage_class) {
      switch (storage_class) {
   case SpvStorageClassUniformConstant:
         case SpvStorageClassWorkgroup:
         case SpvStorageClassCrossWorkgroup:
         case SpvStorageClassFunction:
   default:
                     enum binary::argument::type
   convert_image_type(SpvId id, SpvDim dim, SpvAccessQualifier access,
            switch (dim) {
   case SpvDim1D:
   case SpvDim2D:
   case SpvDim3D:
   case SpvDimBuffer:
      switch (access) {
   case SpvAccessQualifierReadOnly:
         case SpvAccessQualifierWriteOnly:
         default:
      err += "Unknown access qualifier " + std::to_string(access) + " for image "
               default:
      err += "Unknown dimension " + std::to_string(dim) + " for image "
                        binary::section
   make_text_section(const std::string &code,
            const pipe_binary_program_header header { uint32_t(code.size()) };
            text.data.reserve(sizeof(header) + header.num_bytes);
   text.data.insert(text.data.end(), reinterpret_cast<const char *>(&header),
                              binary
   create_binary_from_spirv(const std::string &source,
                  const size_t length = source.size() / sizeof(uint32_t);
            std::string kernel_name;
   size_t kernel_nb = 0u;
   std::vector<binary::argument> args;
                     std::vector<std::string> attributes;
   std::unordered_map<SpvId, std::vector<size_t> > req_local_sizes;
   std::unordered_map<SpvId, std::string> kernels;
   std::unordered_map<SpvId, binary::argument> types;
   std::unordered_map<SpvId, SpvId> pointer_types;
   std::unordered_map<SpvId, unsigned int> constants;
   std::unordered_set<SpvId> packed_structures;
   std::unordered_map<SpvId, std::vector<SpvFunctionParameterAttribute>>
         std::unordered_map<SpvId, std::string> names;
   std::unordered_map<SpvId, cl_kernel_arg_type_qualifier> qualifiers;
   std::unordered_map<std::string, std::vector<std::string> > param_type_names;
            while (i < length) {
      const auto inst = &source[i * sizeof(uint32_t)];
   const auto desc_word = get<uint32_t>(inst, 0);
                  switch (opcode) {
   case SpvOpName: {
      names.emplace(get<SpvId>(inst, 1),
                     case SpvOpString: {
      // SPIRV-LLVM-Translator stores param type names as OpStrings
   std::string str(source.data() + (i + 2u) * sizeof(uint32_t));
   if (str.find("kernel_arg_type.") == 0) {
                              std::string k = line;
   while (std::getline(istream, line, ','))
      } else if (str.find("kernel_arg_type_qual.") == 0) {
                     std::getline(istream, line, '.');
   std::string k = line;
   while (std::getline(istream, line, ','))
      } else
                     case SpvOpEntryPoint:
      if (get<SpvExecutionModel>(inst, 1) == SpvExecutionModelKernel)
                     case SpvOpExecutionMode:
      switch (get<SpvExecutionMode>(inst, 2)) {
   case SpvExecutionModeLocalSize: {
      req_local_sizes[get<SpvId>(inst, 1)] = {
      get<uint32_t>(inst, 3),
   get<uint32_t>(inst, 4),
      };
   std::string s = "reqd_work_group_size(";
   s += std::to_string(get<uint32_t>(inst, 3));
   s += ",";
   s += std::to_string(get<uint32_t>(inst, 4));
   s += ",";
   s += std::to_string(get<uint32_t>(inst, 5));
   s += ")";
   attributes.emplace_back(s);
      }
   case SpvExecutionModeLocalSizeHint: {
      std::string s = "work_group_size_hint(";
   s += std::to_string(get<uint32_t>(inst, 3));
   s += ",";
   s += std::to_string(get<uint32_t>(inst, 4));
   s += ",";
   s += std::to_string(get<uint32_t>(inst, 5));
   s += ")";
   attributes.emplace_back(s);
   case SpvExecutionModeVecTypeHint: {
                              val &= 0xf;
   if (val > 6)
         std::string s = "vec_type_hint(";
   s += type_strs[val];
   s += std::to_string(size);
      break;
         }
   default:
                     case SpvOpDecorate: {
      const auto id = get<SpvId>(inst, 1);
   const auto decoration = get<SpvDecoration>(inst, 2);
   switch (decoration) {
   case SpvDecorationCPacked:
      packed_structures.emplace(id);
      case SpvDecorationFuncParamAttr: {
      const auto attribute =
         func_param_attr_map[id].push_back(attribute);
      }
   case SpvDecorationVolatile:
      qualifiers[id] |= CL_KERNEL_ARG_TYPE_VOLATILE;
      default:
         }
               case SpvOpGroupDecorate: {
      const auto group_id = get<SpvId>(inst, 1);
   if (packed_structures.count(group_id)) {
      for (unsigned int i = 2u; i < num_operands; ++i)
      }
   const auto func_param_attr_iter =
         if (func_param_attr_iter != func_param_attr_map.end()) {
      for (unsigned int i = 2u; i < num_operands; ++i) {
      auto &attrs = func_param_attr_map[get<SpvId>(inst, i)];
   attrs.insert(attrs.begin(),
               }
   if (qualifiers.count(group_id)) {
      for (unsigned int i = 2u; i < num_operands; ++i)
      }
               case SpvOpConstant:
      // We only care about constants that represent the size of arrays.
   // If they are passed as argument, they will never be more than
   // 4GB-wide, and even if they did, a clover::binary::argument size
   // is represented by an int.
               case SpvOpTypeInt:
   case SpvOpTypeFloat: {
      const auto size = get<uint32_t>(inst, 2) / 8u;
   const auto id = get<SpvId>(inst, 1);
   types[id] = { binary::argument::scalar, size, size, size,
         types[id].info.address_qualifier = CL_KERNEL_ARG_ADDRESS_PRIVATE;
               case SpvOpTypeArray: {
      const auto id = get<SpvId>(inst, 1);
   const auto type_id = get<SpvId>(inst, 2);
   const auto types_iter = types.find(type_id);
                  const auto constant_id = get<SpvId>(inst, 3);
   const auto constants_iter = constants.find(constant_id);
   if (constants_iter == constants.end()) {
      err += "Constant " + std::to_string(constant_id) +
            }
   const auto elem_size = types_iter->second.size;
   const auto elem_nbs = constants_iter->second;
   const auto size = elem_size * elem_nbs;
   types[id] = { binary::argument::scalar, size, size,
                           case SpvOpTypeStruct: {
                     unsigned struct_size = 0u;
   unsigned struct_align = 1u;
   for (unsigned j = 2u; j < num_operands; ++j) {
                     // If a type was not found, that means it is not one of the
   // types allowed as kernel arguments. And since the binary has
   // been validated, this means this type is not used for kernel
                        const auto alignment = is_packed ? 1u
         const auto padding = (-struct_size) & (alignment - 1u);
   struct_size += padding + types_iter->second.target_size;
      }
   struct_size += (-struct_size) & (struct_align - 1u);
   types[id] = { binary::argument::scalar, struct_size, struct_size,
                     case SpvOpTypeVector: {
      const auto id = get<SpvId>(inst, 1);
                  // If a type was not found, that means it is not one of the
   // types allowed as kernel arguments. And since the binary has
   // been validated, this means this type is not used for kernel
   // arguments, and therefore can be ignored.
                  const auto elem_size = types_iter->second.size;
   const auto elem_nbs = get<uint32_t>(inst, 3);
   const auto size = elem_size * (elem_nbs != 3 ? elem_nbs : 4);
   types[id] = { binary::argument::scalar, size, size, size,
         types[id].info.address_qualifier = CL_KERNEL_ARG_ADDRESS_PRIVATE;
               case SpvOpTypeForwardPointer: // FALLTHROUGH
   case SpvOpTypePointer: {
      const auto id = get<SpvId>(inst, 1);
   const auto storage_class = get<SpvStorageClass>(inst, 2);
   // Input means this is for a builtin variable, which can not be
   // passed as an argument to a kernel.
                                 binary::size_t alignment;
   if (storage_class == SpvStorageClassWorkgroup)
                        types[id] = { convert_storage_class(storage_class, err),
               sizeof(cl_mem),
   static_cast<binary::size_t>(pointer_byte_size),
   types[id].info.address_qualifier = convert_storage_class_to_cl(storage_class);
               case SpvOpTypeSampler:
      types[get<SpvId>(inst, 1)] = { binary::argument::sampler,
               case SpvOpTypeImage: {
      const auto id = get<SpvId>(inst, 1);
   const auto dim = get<SpvDim>(inst, 3);
   const auto access = get<SpvAccessQualifier>(inst, 9);
   types[id] = { convert_image_type(id, dim, access, err),
                           case SpvOpTypePipe: // FALLTHROUGH
   case SpvOpTypeQueue: {
      err += "TypePipe and TypeQueue are valid SPIR-V 1.0 types, but are "
         "not available in the currently supported OpenCL C version."
               case SpvOpFunction: {
      auto id = get<SpvId>(inst, 2);
   const auto kernels_iter = kernels.find(id);
                  const auto req_local_size_iter = req_local_sizes.find(id);
   if (req_local_size_iter != req_local_sizes.end())
                                    case SpvOpFunctionParameter: {
                     const auto id = get<SpvId>(inst, 2);
   const auto type_id = get<SpvId>(inst, 1);
   auto arg = types.find(type_id)->second;
   const auto &func_param_attr_iter =
         if (func_param_attr_iter != func_param_attr_map.end()) {
      for (auto &i : func_param_attr_iter->second) {
      switch (i) {
   case SpvFunctionParameterAttributeSext:
      arg.ext_type = binary::argument::sign_ext;
      case SpvFunctionParameterAttributeZext:
      arg.ext_type = binary::argument::zero_ext;
      case SpvFunctionParameterAttributeByVal: {
      const SpvId ptr_type_id =
         arg = types.find(ptr_type_id)->second;
      }
   case SpvFunctionParameterAttributeNoAlias:
      arg.info.type_qualifier |= CL_KERNEL_ARG_TYPE_RESTRICT;
      case SpvFunctionParameterAttributeNoWrite:
      arg.info.type_qualifier |= CL_KERNEL_ARG_TYPE_CONST;
      default:
                        auto name_it = names.find(id);
                  arg.info.type_qualifier |= qualifiers[id];
   arg.info.address_qualifier = types[type_id].info.address_qualifier;
   arg.info.access_qualifier = CL_KERNEL_ARG_ACCESS_NONE;
   args.emplace_back(arg);
               case SpvOpFunctionEnd: {
                                    for (size_t i = 0; i < param_qual_names[kernel_name].size(); i++)
      if (param_qual_names[kernel_name][i].find("const") != std::string::npos)
      b.syms.emplace_back(kernel_name, detokenize(attributes, " "),
         ++kernel_nb;
   kernel_name.clear();
   args.clear();
   attributes.clear();
      }
   default:
                              b.secs.push_back(make_text_section(source,
                     bool
   check_spirv_version(const device &dev, const char *binary,
            const auto spirv_version = get<uint32_t>(binary, 1u);
   const auto supported_spirv_versions = clover::spirv::supported_versions();
   const auto compare_versions =
      [module_version =
                     if (std::find_if(supported_spirv_versions.cbegin(),
                        r_log += "SPIR-V version " +
               for (const auto &version : supported_spirv_versions) {
         }
   r_log += "\n";
               bool
   check_capabilities(const device &dev, const std::string &source,
            const size_t length = source.size() / sizeof(uint32_t);
            while (i < length) {
      const auto desc_word = get<uint32_t>(source.data(), i);
                                 const auto capability = get<SpvCapability>(source.data(), i + 1u);
   switch (capability) {
   // Mandatory capabilities
   case SpvCapabilityAddresses:
   case SpvCapabilityFloat16Buffer:
   case SpvCapabilityGroups:
   case SpvCapabilityInt64:
   case SpvCapabilityInt16:
   case SpvCapabilityInt8:
   case SpvCapabilityKernel:
   case SpvCapabilityLinkage:
   case SpvCapabilityVector16:
         // Optional capabilities
   case SpvCapabilityImageBasic:
   case SpvCapabilityLiteralSampler:
   case SpvCapabilitySampled1D:
   case SpvCapabilityImage1D:
   case SpvCapabilitySampledBuffer:
   case SpvCapabilityImageBuffer:
      if (!dev.image_support()) {
      r_log += "Capability 'ImageBasic' is not supported.\n";
      }
      case SpvCapabilityFloat64:
      if (!dev.has_doubles()) {
      r_log += "Capability 'Float64' is not supported.\n";
      }
      // Enabled through extensions
   case SpvCapabilityFloat16:
      if (!dev.has_halves()) {
      r_log += "Capability 'Float16' is not supported.\n";
      }
      case SpvCapabilityInt64Atomics:
      if (!dev.has_int64_atomics()) {
      r_log += "Capability 'Int64Atomics' is not supported.\n";
      }
      default:
      r_log += "Capability '" + std::to_string(capability) +
                                             bool
   check_extensions(const device &dev, const std::string &source,
            const size_t length = source.size() / sizeof(uint32_t);
   size_t i = SPIRV_HEADER_WORD_SIZE; // Skip header
            while (i < length) {
      const auto desc_word = get<uint32_t>(source.data(), i);
                  if (opcode == SpvOpCapability) {
      i += num_operands;
      }
                  const std::string extension = source.data() + (i + 1u) * sizeof(uint32_t);
   if (spirv_extensions.count(extension) == 0) {
      r_log += "Extension '" + extension + "' is not supported.\n";
                                       bool
   check_memory_model(const device &dev, const std::string &source,
            const size_t length = source.size() / sizeof(uint32_t);
            while (i < length) {
      const auto desc_word = get<uint32_t>(source.data(), i);
                  switch (opcode) {
   case SpvOpMemoryModel:
      switch (get<SpvAddressingModel>(source.data(), i + 1u)) {
   case SpvAddressingModelPhysical32:
         case SpvAddressingModelPhysical64:
         default:
      unreachable("Only Physical32 and Physical64 are valid for OpenCL, and the binary was already validated");
      }
      default:
                                          // Copies the input binary and convert it to the endianness of the host CPU.
   std::string
   spirv_to_cpu(const std::string &binary)
   {
      const uint32_t first_word = get<uint32_t>(binary.data(), 0u);
   if (first_word == SpvMagicNumber)
            std::vector<char> cpu_endianness_binary(binary.size());
   for (size_t i = 0; i < (binary.size() / 4u); ++i) {
      const uint32_t word = get<uint32_t>(binary.data(), i);
   reinterpret_cast<uint32_t *>(cpu_endianness_binary.data())[i] =
               return std::string(cpu_endianness_binary.begin(),
            #ifdef HAVE_CLOVER_SPIRV
      std::string
   format_validator_msg(spv_message_level_t level, const char * /* source */,
            std::string level_str;
   switch (level) {
   case SPV_MSG_FATAL:
      level_str = "Fatal";
      case SPV_MSG_INTERNAL_ERROR:
      level_str = "Internal error";
      case SPV_MSG_ERROR:
      level_str = "Error";
      case SPV_MSG_WARNING:
      level_str = "Warning";
      case SPV_MSG_INFO:
      level_str = "Info";
      case SPV_MSG_DEBUG:
      level_str = "Debug";
      }
   return "[" + level_str + "] At word No." +
               spv_target_env
   convert_opencl_version_to_target_env(const cl_version opencl_version) {
      // Pick 1.2 for 3.0 for now
   if (opencl_version == CL_MAKE_VERSION(3, 0, 0)) {
         } else if (opencl_version == CL_MAKE_VERSION(2, 2, 0)) {
         } else if (opencl_version == CL_MAKE_VERSION(2, 1, 0)) {
         } else if (opencl_version == CL_MAKE_VERSION(2, 0, 0)) {
         } else if (opencl_version == CL_MAKE_VERSION(1, 2, 0) ||
            opencl_version == CL_MAKE_VERSION(1, 1, 0) ||
   // SPIR-V is only defined for OpenCL >= 1.2, however some drivers
   // might use it with OpenCL 1.0 and 1.1.
      } else {
               #endif
      }
      bool
   clover::spirv::is_binary_spirv(const std::string &binary)
   {
      // A SPIR-V binary is at the very least 5 32-bit words, which represent the
   // SPIR-V header.
   if (binary.size() < 20u)
            const uint32_t first_word =
         return (first_word == SpvMagicNumber) ||
      }
      std::string
   clover::spirv::version_to_string(uint32_t version) {
      const uint32_t major_version = (version >> 16) & 0xff;
   const uint32_t minor_version = (version >> 8) & 0xff;
   return std::to_string(major_version) + '.' +
      }
      binary
   clover::spirv::compile_program(const std::string &binary,
                           if (validate && !is_valid_spirv(source, dev.device_version(), r_log))
            if (!check_spirv_version(dev, source.data(), r_log))
         if (!check_capabilities(dev, source, r_log))
         if (!check_extensions(dev, source, r_log))
         if (!check_memory_model(dev, source, r_log))
            return create_binary_from_spirv(source,
      }
      binary
   clover::spirv::link_program(const std::vector<binary> &binaries,
                                    std::string ignored_options;
   for (const std::string &option : options) {
      if (option == "-create-library") {
         } else {
            }
   if (!ignored_options.empty()) {
      r_log += "Ignoring the following link options: " + ignored_options
               spvtools::LinkerOptions linker_options;
                     const auto section_type = create_library ? binary::section::text_library :
            std::vector<const uint32_t *> sections;
   sections.reserve(binaries.size());
   std::vector<size_t> lengths;
            auto const validator_consumer = [&r_log](spv_message_level_t level,
                                    for (const auto &bin : binaries) {
      const auto &bsec = find([](const binary::section &sec) {
                        const auto c_il = ((struct pipe_binary_program_header*)bsec.data.data())->blob;
            if (!check_spirv_version(dev, c_il, r_log))
            sections.push_back(reinterpret_cast<const uint32_t *>(c_il));
                        const cl_version opencl_version = dev.device_version();
   const spv_target_env target_env =
            const spvtools::MessageConsumer consumer = validator_consumer;
   spvtools::Context context(target_env);
            if (Link(context, sections.data(), lengths.data(), sections.size(),
                  std::string final_binary{
         reinterpret_cast<char *>(linked_binary.data()),
   reinterpret_cast<char *>(linked_binary.data() +
   if (!is_valid_spirv(final_binary, opencl_version, r_log))
            if (has_flag(llvm::debug::spirv))
            for (const auto &bin : binaries)
                        }
      bool
   clover::spirv::is_valid_spirv(const std::string &binary,
                  auto const validator_consumer =
      [&r_log](spv_message_level_t level, const char *source,
                     const spv_target_env target_env =
         spvtools::SpirvTools spvTool(target_env);
            spvtools::ValidatorOptions validator_options;
   validator_options.SetUniversalLimit(spv_validator_limit_max_function_args,
            return spvTool.Validate(reinterpret_cast<const uint32_t *>(binary.data()),
      }
      std::string
   clover::spirv::print_module(const std::string &binary,
            const spv_target_env target_env =
         spvtools::SpirvTools spvTool(target_env);
   spv_context spvContext = spvContextCreate(target_env);
   if (!spvContext)
            spv_text disassembly;
   spvBinaryToText(spvContext,
                              const std::string disassemblyStr = disassembly->str;
               }
      std::unordered_set<std::string>
   clover::spirv::supported_extensions() {
      return {
      /* this is only a hint so all devices support that */
         }
      std::vector<cl_name_version>
   clover::spirv::supported_versions() {
         }
      cl_version
   clover::spirv::to_opencl_version_encoding(uint32_t version) {
         return CL_MAKE_VERSION((version >> 16u) & 0xff,
   }
      uint32_t
   clover::spirv::to_spirv_version_encoding(cl_version version) {
      return ((CL_VERSION_MAJOR(version) & 0xff) << 16u) |
      }
      #else
   bool
   clover::spirv::is_binary_spirv(const std::string &binary)
   {
         }
      bool
   clover::spirv::is_valid_spirv(const std::string &/*binary*/,
                     }
      std::string
   clover::spirv::version_to_string(uint32_t version) {
         }
      binary
   clover::spirv::compile_program(const std::string &binary,
                  r_log += "SPIR-V support in clover is not enabled.\n";
      }
      binary
   clover::spirv::link_program(const std::vector<binary> &/*binaries*/,
                  r_log += "SPIR-V support in clover is not enabled.\n";
      }
      std::string
   clover::spirv::print_module(const std::string &binary,
               }
      std::unordered_set<std::string>
   clover::spirv::supported_extensions() {
         }
      std::vector<cl_name_version>
   clover::spirv::supported_versions() {
         }
      cl_version
   clover::spirv::to_opencl_version_encoding(uint32_t version) {
         }
      uint32_t
   clover::spirv::to_spirv_version_encoding(cl_version version) {
         }
   #endif
