   /*
   * Copyright © 2017 Intel Corporation
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
   */
      #include "vk_shader_module.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_log.h"
   #include "vk_nir.h"
   #include "vk_pipeline.h"
   #include "vk_util.h"
      void vk_shader_module_init(struct vk_device *device,
               {
                        module->size = create_info->codeSize;
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateShaderModule(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
            assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
            module = vk_alloc2(&device->alloc, pAllocator,
               if (module == NULL)
                                 }
      const uint8_t vk_shaderModuleIdentifierAlgorithmUUID[VK_UUID_SIZE] = "MESA-BLAKE3";
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetShaderModuleIdentifierEXT(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_shader_module, module, _module);
   memcpy(pIdentifier->identifier, module->hash, sizeof(module->hash));
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetShaderModuleCreateInfoIdentifierEXT(VkDevice _device,
               {
      _mesa_blake3_compute(pCreateInfo->pCode, pCreateInfo->codeSize,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyShaderModule(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (!module)
            /* NIR modules (which are only created internally by the driver) are not
   * dynamically allocated so we should never call this for them.
   * Instead the driver is responsible for freeing the NIR code when it is
   * no longer needed.
   */
               }
      #define SPIR_V_MAGIC_NUMBER 0x07230203
      uint32_t
   vk_shader_module_spirv_version(const struct vk_shader_module *mod)
   {
      if (mod->nir != NULL)
               }
      VkResult
   vk_shader_module_to_nir(struct vk_device *device,
                           const struct vk_shader_module *mod,
   gl_shader_stage stage,
   const char *entrypoint_name,
   {
      const VkPipelineShaderStageCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = mesa_to_vk_shader_stage(stage),
   .module = vk_shader_module_to_handle((struct vk_shader_module *)mod),
   .pName = entrypoint_name,
      };
   return vk_pipeline_shader_stage_to_nir(device, &info,
            }
