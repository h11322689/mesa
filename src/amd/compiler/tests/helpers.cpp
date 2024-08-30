   /*
   * Copyright © 2020 Valve Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   */
   #include "helpers.h"
      #include "common/amd_family.h"
   #include "vulkan/vk_format.h"
      #include <llvm-c/Target.h>
      #include <mutex>
   #include <sstream>
   #include <stdio.h>
      using namespace aco;
      extern "C" {
   PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName);
   }
      ac_shader_config config;
   aco_shader_info info;
   std::unique_ptr<Program> program;
   Builder bld(NULL);
   Temp inputs[16];
      static VkInstance instance_cache[CHIP_LAST] = {VK_NULL_HANDLE};
   static VkDevice device_cache[CHIP_LAST] = {VK_NULL_HANDLE};
   static std::mutex create_device_mutex;
      #define FUNCTION_LIST                                                                              \
      ITEM(CreateInstance)                                                                            \
   ITEM(DestroyInstance)                                                                           \
   ITEM(EnumeratePhysicalDevices)                                                                  \
   ITEM(GetPhysicalDeviceProperties2)                                                              \
   ITEM(CreateDevice)                                                                              \
   ITEM(DestroyDevice)                                                                             \
   ITEM(CreateShaderModule)                                                                        \
   ITEM(DestroyShaderModule)                                                                       \
   ITEM(CreateGraphicsPipelines)                                                                   \
   ITEM(CreateComputePipelines)                                                                    \
   ITEM(DestroyPipeline)                                                                           \
   ITEM(CreateDescriptorSetLayout)                                                                 \
   ITEM(DestroyDescriptorSetLayout)                                                                \
   ITEM(CreatePipelineLayout)                                                                      \
   ITEM(DestroyPipelineLayout)                                                                     \
   ITEM(CreateRenderPass)                                                                          \
   ITEM(DestroyRenderPass)                                                                         \
   ITEM(GetPipelineExecutablePropertiesKHR)                                                        \
         #define ITEM(n) PFN_vk##n n;
   FUNCTION_LIST
   #undef ITEM
      void
   create_program(enum amd_gfx_level gfx_level, Stage stage, unsigned wave_size,
         {
      memset(&config, 0, sizeof(config));
            program.reset(new Program);
   aco::init_program(program.get(), stage, &info, gfx_level, family, false, &config);
   program->workgroup_size = UINT_MAX;
            program->debug.func = nullptr;
            program->debug.output = output;
   program->debug.shorten_messages = true;
   program->debug.func = nullptr;
            Block* block = program->create_and_insert_block();
                        }
      bool
   setup_cs(const char* input_spec, enum amd_gfx_level gfx_level, enum radeon_family family,
         {
      if (!set_variant(gfx_level, subvariant))
            memset(&info, 0, sizeof(info));
            if (input_spec) {
      std::vector<RegClass> input_classes;
   while (input_spec[0]) {
      RegType type = input_spec[0] == 'v' ? RegType::vgpr : RegType::sgpr;
   unsigned size = input_spec[1] - '0';
                  input_spec += 2 + in_bytes;
   while (input_spec[0] == ' ')
               aco_ptr<Instruction> startpgm{create_instruction<Pseudo_instruction>(
         for (unsigned i = 0; i < input_classes.size(); i++) {
      inputs[i] = bld.tmp(input_classes[i]);
      }
                  }
      void
   finish_program(Program* prog, bool endpgm)
   {
      for (Block& BB : prog->blocks) {
      for (unsigned idx : BB.linear_preds)
         for (unsigned idx : BB.logical_preds)
               if (endpgm) {
      for (Block& block : prog->blocks) {
      if (block.linear_succs.size() == 0) {
      block.kind |= block_kind_uniform;
               }
      void
   finish_validator_test()
   {
      finish_program(program.get());
   aco_print_program(program.get(), output);
   fprintf(output, "Validation results:\n");
   if (aco::validate_ir(program.get()))
         else
      }
      void
   finish_opt_test()
   {
      finish_program(program.get());
   if (!aco::validate_ir(program.get())) {
      fail_test("Validation before optimization failed");
      }
   aco::optimize(program.get());
   if (!aco::validate_ir(program.get())) {
      fail_test("Validation after optimization failed");
      }
      }
      void
   finish_setup_reduce_temp_test()
   {
      finish_program(program.get());
   if (!aco::validate_ir(program.get())) {
      fail_test("Validation before setup_reduce_temp failed");
      }
   aco::setup_reduce_temp(program.get());
   if (!aco::validate_ir(program.get())) {
      fail_test("Validation after setup_reduce_temp failed");
      }
      }
      void
   finish_ra_test(ra_test_policy policy, bool lower)
   {
      finish_program(program.get());
   if (!aco::validate_ir(program.get())) {
      fail_test("Validation before register allocation failed");
               program->workgroup_size = program->wave_size;
   aco::live live_vars = aco::live_var_analysis(program.get());
            if (aco::validate_ra(program.get())) {
      fail_test("Validation after register allocation failed");
               if (lower) {
      aco::ssa_elimination(program.get());
                  }
      void
   finish_optimizer_postRA_test()
   {
      finish_program(program.get());
   aco::optimize_postRA(program.get());
      }
      void
   finish_to_hw_instr_test()
   {
      finish_program(program.get());
   aco::lower_to_hw_instr(program.get());
      }
      void
   finish_waitcnt_test()
   {
      finish_program(program.get());
   aco::insert_wait_states(program.get());
      }
      void
   finish_insert_nops_test(bool endpgm)
   {
      finish_program(program.get(), endpgm);
   aco::insert_NOPs(program.get());
      }
      void
   finish_form_hard_clause_test()
   {
      finish_program(program.get());
   aco::form_hard_clauses(program.get());
      }
      void
   finish_assembler_test()
   {
      finish_program(program.get());
   std::vector<uint32_t> binary;
            /* we could use CLRX for disassembly but that would require it to be
   * installed */
   if (program->gfx_level >= GFX8) {
         } else {
      // TODO: maybe we should use CLRX and skip this test if it's not available?
   for (uint32_t dword : binary)
         }
      void
   writeout(unsigned i, Temp tmp)
   {
      if (tmp.id())
         else
      }
      void
   writeout(unsigned i, aco::Builder::Result res)
   {
         }
      void
   writeout(unsigned i, Operand op)
   {
         }
      void
   writeout(unsigned i, Operand op0, Operand op1)
   {
         }
      Temp
   fneg(Temp src, Builder b)
   {
      if (src.bytes() == 2)
         else
      }
      Temp
   fabs(Temp src, Builder b)
   {
      if (src.bytes() == 2) {
      Builder::Result res =
         res->valu().abs[1] = true;
      } else {
      Builder::Result res =
         res->valu().abs[1] = true;
         }
      Temp
   f2f32(Temp src, Builder b)
   {
         }
      Temp
   f2f16(Temp src, Builder b)
   {
         }
      Temp
   u2u16(Temp src, Builder b)
   {
         }
      Temp
   fadd(Temp src0, Temp src1, Builder b)
   {
      if (src0.bytes() == 2)
         else
      }
      Temp
   fmul(Temp src0, Temp src1, Builder b)
   {
      if (src0.bytes() == 2)
         else
      }
      Temp
   fma(Temp src0, Temp src1, Temp src2, Builder b)
   {
      if (src0.bytes() == 2)
         else
      }
      Temp
   fsat(Temp src, Builder b)
   {
      if (src.bytes() == 2)
      return b.vop3(aco_opcode::v_med3_f16, b.def(v2b), Operand::c16(0u), Operand::c16(0x3c00u),
      else
      return b.vop3(aco_opcode::v_med3_f32, b.def(v1), Operand::zero(), Operand::c32(0x3f800000u),
   }
      Temp
   fmin(Temp src0, Temp src1, Builder b)
   {
         }
      Temp
   fmax(Temp src0, Temp src1, Builder b)
   {
         }
      Temp
   ext_ushort(Temp src, unsigned idx, Builder b)
   {
      return b.pseudo(aco_opcode::p_extract, b.def(src.regClass()), src, Operand::c32(idx),
      }
      Temp
   ext_ubyte(Temp src, unsigned idx, Builder b)
   {
      return b.pseudo(aco_opcode::p_extract, b.def(src.regClass()), src, Operand::c32(idx),
      }
      void
   emit_divergent_if_else(Program* prog, aco::Builder& b, Operand cond, std::function<void()> then,
         {
               Block* if_block = &prog->blocks.back();
   Block* then_logical = prog->create_and_insert_block();
   Block* then_linear = prog->create_and_insert_block();
   Block* invert = prog->create_and_insert_block();
   Block* else_logical = prog->create_and_insert_block();
   Block* else_linear = prog->create_and_insert_block();
            if_block->kind |= block_kind_branch;
   invert->kind |= block_kind_invert;
            /* Set up logical CF */
   then_logical->logical_preds.push_back(if_block->index);
   else_logical->logical_preds.push_back(if_block->index);
   endif_block->logical_preds.push_back(then_logical->index);
            /* Set up linear CF */
   then_logical->linear_preds.push_back(if_block->index);
   then_linear->linear_preds.push_back(if_block->index);
   invert->linear_preds.push_back(then_logical->index);
   invert->linear_preds.push_back(then_linear->index);
   else_logical->linear_preds.push_back(invert->index);
   else_linear->linear_preds.push_back(invert->index);
   endif_block->linear_preds.push_back(else_logical->index);
                     b.reset(if_block);
   Temp saved_exec = b.sop1(Builder::s_and_saveexec, b.def(b.lm, saved_exec_reg),
         b.branch(aco_opcode::p_cbranch_nz, Definition(vcc, bld.lm), then_logical->index,
            b.reset(then_logical);
   b.pseudo(aco_opcode::p_logical_start);
   then();
   b.pseudo(aco_opcode::p_logical_end);
            b.reset(then_linear);
            b.reset(invert);
   b.sop2(Builder::s_andn2, Definition(exec, bld.lm), Definition(scc, s1),
         b.branch(aco_opcode::p_cbranch_nz, Definition(vcc, bld.lm), else_logical->index,
            b.reset(else_logical);
   b.pseudo(aco_opcode::p_logical_start);
   els();
   b.pseudo(aco_opcode::p_logical_end);
            b.reset(else_linear);
            b.reset(endif_block);
   b.pseudo(aco_opcode::p_parallelcopy, Definition(exec, bld.lm),
      }
      VkDevice
   get_vk_device(enum amd_gfx_level gfx_level)
   {
      enum radeon_family family;
   switch (gfx_level) {
   case GFX6: family = CHIP_TAHITI; break;
   case GFX7: family = CHIP_BONAIRE; break;
   case GFX8: family = CHIP_POLARIS10; break;
   case GFX9: family = CHIP_VEGA10; break;
   case GFX10: family = CHIP_NAVI10; break;
   case GFX10_3: family = CHIP_NAVI21; break;
   case GFX11: family = CHIP_NAVI31; break;
   default: family = CHIP_UNKNOWN; break;
   }
      }
      VkDevice
   get_vk_device(enum radeon_family family)
   {
                        if (device_cache[family])
                     VkApplicationInfo app_info = {};
   app_info.pApplicationName = "aco_tests";
   app_info.apiVersion = VK_API_VERSION_1_2;
   VkInstanceCreateInfo instance_create_info = {};
   instance_create_info.pApplicationInfo = &app_info;
   instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   ASSERTED VkResult result = ((PFN_vkCreateInstance)vk_icdGetInstanceProcAddr(
               #define ITEM(n) n = (PFN_vk##n)vk_icdGetInstanceProcAddr(instance_cache[family], "vk" #n);
         #undef ITEM
         uint32_t device_count = 1;
   VkPhysicalDevice device = VK_NULL_HANDLE;
   result = EnumeratePhysicalDevices(instance_cache[family], &device_count, &device);
   assert(result == VK_SUCCESS);
            VkDeviceCreateInfo device_create_info = {};
   device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   static const char* extensions[] = {"VK_KHR_pipeline_executable_properties"};
   device_create_info.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
   device_create_info.ppEnabledExtensionNames = extensions;
               }
      static struct DestroyDevices {
      ~DestroyDevices()
   {
      for (unsigned i = 0; i < CHIP_LAST; i++) {
      if (!device_cache[i])
         DestroyDevice(device_cache[i], NULL);
            } destroy_devices;
      void
   print_pipeline_ir(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits stages,
         {
      uint32_t executable_count = 16;
   VkPipelineExecutablePropertiesKHR executables[16];
   VkPipelineInfoKHR pipeline_info;
   pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
   pipeline_info.pNext = NULL;
   pipeline_info.pipeline = pipeline;
   ASSERTED VkResult result =
                  uint32_t executable = 0;
   for (; executable < executable_count; executable++) {
      if (executables[executable].stages == stages)
      }
            VkPipelineExecutableInfoKHR exec_info;
   exec_info.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
   exec_info.pNext = NULL;
   exec_info.pipeline = pipeline;
            uint32_t ir_count = 16;
   VkPipelineExecutableInternalRepresentationKHR ir[16];
   memset(ir, 0, sizeof(ir));
   result = GetPipelineExecutableInternalRepresentationsKHR(device, &exec_info, &ir_count, ir);
            VkPipelineExecutableInternalRepresentationKHR* requested_ir = nullptr;
   for (unsigned i = 0; i < ir_count; ++i) {
      if (strcmp(ir[i].name, name) == 0) {
      requested_ir = &ir[i];
         }
            char* data = (char*)malloc(requested_ir->dataSize);
   requested_ir->pData = data;
   result = GetPipelineExecutableInternalRepresentationsKHR(device, &exec_info, &ir_count, ir);
            if (remove_encoding) {
      for (char* c = data; *c; c++) {
      if (*c == ';') {
      for (; *c && *c != '\n'; c++)
                     fprintf(output, "%s", data);
      }
      VkShaderModule
   __qoCreateShaderModule(VkDevice dev, const QoShaderModuleCreateInfo* module_info)
   {
      VkShaderModuleCreateInfo vk_module_info;
   vk_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   vk_module_info.pNext = NULL;
   vk_module_info.flags = 0;
   vk_module_info.codeSize = module_info->spirvSize;
            VkShaderModule module;
   ASSERTED VkResult result = CreateShaderModule(dev, &vk_module_info, NULL, &module);
               }
      PipelineBuilder::PipelineBuilder(VkDevice dev)
   {
      memset(this, 0, sizeof(*this));
   topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      }
      PipelineBuilder::~PipelineBuilder()
   {
               for (unsigned i = 0; i < (is_compute() ? 1 : gfx_pipeline_info.stageCount); i++) {
      VkPipelineShaderStageCreateInfo* stage_info = &stages[i];
   if (owned_stages & stage_info->stage)
                        for (unsigned i = 0; i < util_bitcount64(desc_layouts_used); i++)
               }
      void
   PipelineBuilder::add_desc_binding(VkShaderStageFlags stage_flags, uint32_t layout, uint32_t binding,
         {
      desc_layouts_used |= 1ull << layout;
      }
      void
   PipelineBuilder::add_vertex_binding(uint32_t binding, uint32_t stride, VkVertexInputRate rate)
   {
         }
      void
   PipelineBuilder::add_vertex_attribute(uint32_t location, uint32_t binding, VkFormat format,
         {
         }
      void
   PipelineBuilder::add_resource_decls(QoShaderModuleCreateInfo* module)
   {
      for (unsigned i = 0; i < module->declarationCount; i++) {
      const QoShaderDecl* decl = &module->pDeclarations[i];
   switch (decl->decl_type) {
   case QoShaderDeclType_ubo:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_ssbo:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_img_buf:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_img:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_tex_buf:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_combined:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_tex:
      add_desc_binding(module->stage, decl->set, decl->binding,
            case QoShaderDeclType_samp:
      add_desc_binding(module->stage, decl->set, decl->binding, VK_DESCRIPTOR_TYPE_SAMPLER);
      default: break;
         }
      void
   PipelineBuilder::add_io_decls(QoShaderModuleCreateInfo* module)
   {
      unsigned next_vtx_offset = 0;
   for (unsigned i = 0; i < module->declarationCount; i++) {
      const QoShaderDecl* decl = &module->pDeclarations[i];
   switch (decl->decl_type) {
   case QoShaderDeclType_in:
      if (module->stage == VK_SHADER_STAGE_VERTEX_BIT) {
      if (!strcmp(decl->type, "float") || decl->type[0] == 'v')
      add_vertex_attribute(decl->location, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
      else if (decl->type[0] == 'u')
      add_vertex_attribute(decl->location, 0, VK_FORMAT_R32G32B32A32_UINT,
      else if (decl->type[0] == 'i')
      add_vertex_attribute(decl->location, 0, VK_FORMAT_R32G32B32A32_SINT,
         }
      case QoShaderDeclType_out:
      if (module->stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
      if (!strcmp(decl->type, "float") || decl->type[0] == 'v')
         else if (decl->type[0] == 'u')
         else if (decl->type[0] == 'i')
      }
      default: break;
      }
   if (next_vtx_offset)
      }
      void
   PipelineBuilder::add_stage(VkShaderStageFlagBits stage, VkShaderModule module, const char* name)
   {
      VkPipelineShaderStageCreateInfo* stage_info;
   if (stage == VK_SHADER_STAGE_COMPUTE_BIT)
         else
         stage_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stage_info->pNext = NULL;
   stage_info->flags = 0;
   stage_info->stage = stage;
   stage_info->module = module;
   stage_info->pName = name;
   stage_info->pSpecializationInfo = NULL;
      }
      void
   PipelineBuilder::add_stage(VkShaderStageFlagBits stage, QoShaderModuleCreateInfo module,
         {
      add_stage(stage, __qoCreateShaderModule(device, &module), name);
   add_resource_decls(&module);
      }
      void
   PipelineBuilder::add_vsfs(VkShaderModule vs, VkShaderModule fs)
   {
      add_stage(VK_SHADER_STAGE_VERTEX_BIT, vs);
      }
      void
   PipelineBuilder::add_vsfs(QoShaderModuleCreateInfo vs, QoShaderModuleCreateInfo fs)
   {
      add_stage(VK_SHADER_STAGE_VERTEX_BIT, vs);
      }
      void
   PipelineBuilder::add_cs(VkShaderModule cs)
   {
         }
      void
   PipelineBuilder::add_cs(QoShaderModuleCreateInfo cs)
   {
         }
      bool
   PipelineBuilder::is_compute()
   {
         }
      void
   PipelineBuilder::create_compute_pipeline()
   {
      VkComputePipelineCreateInfo create_info;
   create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
   create_info.pNext = NULL;
   create_info.flags = VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR;
   create_info.stage = stages[0];
   create_info.layout = pipeline_layout;
   create_info.basePipelineHandle = VK_NULL_HANDLE;
            ASSERTED VkResult result =
            }
      void
   PipelineBuilder::create_graphics_pipeline()
   {
      /* create the create infos */
   if (!samples)
            unsigned num_color_attachments = 0;
   VkPipelineColorBlendAttachmentState blend_attachment_states[16];
   VkAttachmentReference color_attachments[16];
   VkAttachmentDescription attachment_descs[17];
   for (unsigned i = 0; i < 16; i++) {
      if (color_outputs[i] == VK_FORMAT_UNDEFINED)
            VkAttachmentDescription* desc = &attachment_descs[num_color_attachments];
   desc->flags = 0;
   desc->format = color_outputs[i];
   desc->samples = samples;
   desc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   desc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   desc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
   desc->initialLayout = VK_IMAGE_LAYOUT_GENERAL;
            VkAttachmentReference* ref = &color_attachments[num_color_attachments];
   ref->attachment = num_color_attachments;
            VkPipelineColorBlendAttachmentState* blend = &blend_attachment_states[num_color_attachments];
   blend->blendEnable = false;
   blend->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        unsigned num_attachments = num_color_attachments;
   VkAttachmentReference ds_attachment;
   if (ds_output != VK_FORMAT_UNDEFINED) {
      VkAttachmentDescription* desc = &attachment_descs[num_attachments];
   desc->flags = 0;
   desc->format = ds_output;
   desc->samples = samples;
   desc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   desc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   desc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
   desc->initialLayout = VK_IMAGE_LAYOUT_GENERAL;
            ds_attachment.attachment = num_color_attachments;
                        vs_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vs_input.pNext = NULL;
   vs_input.flags = 0;
   vs_input.pVertexBindingDescriptions = vs_bindings;
            VkPipelineInputAssemblyStateCreateInfo assembly_state;
   assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   assembly_state.pNext = NULL;
   assembly_state.flags = 0;
   assembly_state.topology = topology;
            VkPipelineTessellationStateCreateInfo tess_state;
   tess_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
   tess_state.pNext = NULL;
   tess_state.flags = 0;
            VkPipelineViewportStateCreateInfo viewport_state;
   viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewport_state.pNext = NULL;
   viewport_state.flags = 0;
   viewport_state.viewportCount = 1;
   viewport_state.pViewports = NULL;
   viewport_state.scissorCount = 1;
            VkPipelineRasterizationStateCreateInfo rasterization_state;
   rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterization_state.pNext = NULL;
   rasterization_state.flags = 0;
   rasterization_state.depthClampEnable = false;
   rasterization_state.rasterizerDiscardEnable = false;
   rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
   rasterization_state.cullMode = VK_CULL_MODE_NONE;
   rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   rasterization_state.depthBiasEnable = false;
            VkPipelineMultisampleStateCreateInfo ms_state;
   ms_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   ms_state.pNext = NULL;
   ms_state.flags = 0;
   ms_state.rasterizationSamples = samples;
   ms_state.sampleShadingEnable = sample_shading_enable;
   ms_state.minSampleShading = min_sample_shading;
   VkSampleMask sample_mask = 0xffffffff;
   ms_state.pSampleMask = &sample_mask;
   ms_state.alphaToCoverageEnable = false;
            VkPipelineDepthStencilStateCreateInfo ds_state;
   ds_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   ds_state.pNext = NULL;
   ds_state.flags = 0;
   ds_state.depthTestEnable = ds_output != VK_FORMAT_UNDEFINED;
   ds_state.depthWriteEnable = true;
   ds_state.depthCompareOp = VK_COMPARE_OP_ALWAYS;
   ds_state.depthBoundsTestEnable = false;
   ds_state.stencilTestEnable = true;
   ds_state.front.failOp = VK_STENCIL_OP_KEEP;
   ds_state.front.passOp = VK_STENCIL_OP_REPLACE;
   ds_state.front.depthFailOp = VK_STENCIL_OP_REPLACE;
   ds_state.front.compareOp = VK_COMPARE_OP_ALWAYS;
   ds_state.front.compareMask = 0xffffffff, ds_state.front.writeMask = 0;
   ds_state.front.reference = 0;
            VkPipelineColorBlendStateCreateInfo color_blend_state;
   color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   color_blend_state.pNext = NULL;
   color_blend_state.flags = 0;
   color_blend_state.logicOpEnable = false;
   color_blend_state.attachmentCount = num_color_attachments;
            VkDynamicState dynamic_states[9] = {VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR,
            VkPipelineDynamicStateCreateInfo dynamic_state;
   dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamic_state.pNext = NULL;
   dynamic_state.flags = 0;
   dynamic_state.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
            gfx_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   gfx_pipeline_info.pNext = NULL;
   gfx_pipeline_info.flags = VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR;
   gfx_pipeline_info.pVertexInputState = &vs_input;
   gfx_pipeline_info.pInputAssemblyState = &assembly_state;
   gfx_pipeline_info.pTessellationState = &tess_state;
   gfx_pipeline_info.pViewportState = &viewport_state;
   gfx_pipeline_info.pRasterizationState = &rasterization_state;
   gfx_pipeline_info.pMultisampleState = &ms_state;
   gfx_pipeline_info.pDepthStencilState = &ds_state;
   gfx_pipeline_info.pColorBlendState = &color_blend_state;
   gfx_pipeline_info.pDynamicState = &dynamic_state;
            /* create the objects used to create the pipeline */
   VkSubpassDescription subpass;
   subpass.flags = 0;
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.inputAttachmentCount = 0;
   subpass.pInputAttachments = NULL;
   subpass.colorAttachmentCount = num_color_attachments;
   subpass.pColorAttachments = color_attachments;
   subpass.pResolveAttachments = NULL;
   subpass.pDepthStencilAttachment = ds_output == VK_FORMAT_UNDEFINED ? NULL : &ds_attachment;
   subpass.preserveAttachmentCount = 0;
            VkRenderPassCreateInfo renderpass_info;
   renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderpass_info.pNext = NULL;
   renderpass_info.flags = 0;
   renderpass_info.attachmentCount = num_attachments;
   renderpass_info.pAttachments = attachment_descs;
   renderpass_info.subpassCount = 1;
   renderpass_info.pSubpasses = &subpass;
   renderpass_info.dependencyCount = 0;
            ASSERTED VkResult result = CreateRenderPass(device, &renderpass_info, NULL, &render_pass);
            gfx_pipeline_info.layout = pipeline_layout;
            /* create the pipeline */
            result = CreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gfx_pipeline_info, NULL, &pipeline);
      }
      void
   PipelineBuilder::create_pipeline()
   {
      unsigned num_desc_layouts = 0;
   for (unsigned i = 0; i < 64; i++) {
      if (!(desc_layouts_used & (1ull << i)))
            VkDescriptorSetLayoutCreateInfo desc_layout_info;
   desc_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   desc_layout_info.pNext = NULL;
   desc_layout_info.flags = 0;
   desc_layout_info.bindingCount = num_desc_bindings[i];
            ASSERTED VkResult result = CreateDescriptorSetLayout(device, &desc_layout_info, NULL,
         assert(result == VK_SUCCESS);
               VkPipelineLayoutCreateInfo pipeline_layout_info;
   pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipeline_layout_info.pNext = NULL;
   pipeline_layout_info.flags = 0;
   pipeline_layout_info.pushConstantRangeCount = 1;
   pipeline_layout_info.pPushConstantRanges = &push_constant_range;
   pipeline_layout_info.setLayoutCount = num_desc_layouts;
            ASSERTED VkResult result =
                  if (is_compute())
         else
      }
      void
   PipelineBuilder::print_ir(VkShaderStageFlagBits stage_flags, const char* name, bool remove_encoding)
   {
      if (!pipeline)
            }
