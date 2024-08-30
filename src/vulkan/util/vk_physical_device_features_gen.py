   COPYRIGHT=u"""
   /* Copyright © 2021 Intel Corporation
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
   """
      import argparse
   from collections import OrderedDict
   from dataclasses import dataclass
   import os
   import sys
   import typing
   import xml.etree.ElementTree as et
      import mako
   from mako.template import Template
   from vk_extensions import get_all_required, filter_api
      def str_removeprefix(s, prefix):
      if s.startswith(prefix):
               RENAMED_FEATURES = {
      # See https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/17272#note_1446477 for details
            ('MeshShaderFeaturesNV', 'taskShader'): 'taskShaderNV',
            ('CooperativeMatrixFeaturesNV', 'cooperativeMatrix'): 'cooperativeMatrixNV',
      }
      KNOWN_ALIASES = [
      (['Vulkan11Features', '16BitStorageFeatures'], ['storageBuffer16BitAccess', 'uniformAndStorageBuffer16BitAccess', 'storagePushConstant16', 'storageInputOutput16']),
   (['Vulkan11Features', 'MultiviewFeatures'], ['multiview', 'multiviewGeometryShader', 'multiviewTessellationShader']),
   (['Vulkan11Features', 'VariablePointersFeatures'], ['variablePointersStorageBuffer', 'variablePointers']),
   (['Vulkan11Features', 'ProtectedMemoryFeatures'], ['protectedMemory']),
   (['Vulkan11Features', 'SamplerYcbcrConversionFeatures'], ['samplerYcbcrConversion']),
            (['Vulkan12Features', '8BitStorageFeatures'], ['storageBuffer8BitAccess', 'uniformAndStorageBuffer8BitAccess', 'storagePushConstant8']),
   (['Vulkan12Features', 'ShaderAtomicInt64Features'], ['shaderBufferInt64Atomics', 'shaderSharedInt64Atomics']),
   (['Vulkan12Features', 'ShaderFloat16Int8Features'], ['shaderFloat16', 'shaderInt8']),
   (
      ['Vulkan12Features', 'DescriptorIndexingFeatures'],
   [
         'shaderInputAttachmentArrayDynamicIndexing',
   'shaderUniformTexelBufferArrayDynamicIndexing',
   'shaderStorageTexelBufferArrayDynamicIndexing',
   'shaderUniformBufferArrayNonUniformIndexing',
   'shaderSampledImageArrayNonUniformIndexing',
   'shaderStorageBufferArrayNonUniformIndexing',
   'shaderStorageImageArrayNonUniformIndexing',
   'shaderInputAttachmentArrayNonUniformIndexing',
   'shaderUniformTexelBufferArrayNonUniformIndexing',
   'shaderStorageTexelBufferArrayNonUniformIndexing',
   'descriptorBindingUniformBufferUpdateAfterBind',
   'descriptorBindingSampledImageUpdateAfterBind',
   'descriptorBindingStorageImageUpdateAfterBind',
   'descriptorBindingStorageBufferUpdateAfterBind',
   'descriptorBindingUniformTexelBufferUpdateAfterBind',
   'descriptorBindingStorageTexelBufferUpdateAfterBind',
   'descriptorBindingUpdateUnusedWhilePending',
   'descriptorBindingPartiallyBound',
   'descriptorBindingVariableDescriptorCount',
      ),
   (['Vulkan12Features', 'ScalarBlockLayoutFeatures'], ['scalarBlockLayout']),
   (['Vulkan12Features', 'ImagelessFramebufferFeatures'], ['imagelessFramebuffer']),
   (['Vulkan12Features', 'UniformBufferStandardLayoutFeatures'], ['uniformBufferStandardLayout']),
   (['Vulkan12Features', 'ShaderSubgroupExtendedTypesFeatures'], ['shaderSubgroupExtendedTypes']),
   (['Vulkan12Features', 'SeparateDepthStencilLayoutsFeatures'], ['separateDepthStencilLayouts']),
   (['Vulkan12Features', 'HostQueryResetFeatures'], ['hostQueryReset']),
   (['Vulkan12Features', 'TimelineSemaphoreFeatures'], ['timelineSemaphore']),
   (['Vulkan12Features', 'BufferDeviceAddressFeatures', 'BufferDeviceAddressFeaturesEXT'], ['bufferDeviceAddress', 'bufferDeviceAddressMultiDevice']),
   (['Vulkan12Features', 'BufferDeviceAddressFeatures'], ['bufferDeviceAddressCaptureReplay']),
            (['Vulkan13Features', 'ImageRobustnessFeatures'], ['robustImageAccess']),
   (['Vulkan13Features', 'InlineUniformBlockFeatures'], ['inlineUniformBlock', 'descriptorBindingInlineUniformBlockUpdateAfterBind']),
   (['Vulkan13Features', 'PipelineCreationCacheControlFeatures'], ['pipelineCreationCacheControl']),
   (['Vulkan13Features', 'PrivateDataFeatures'], ['privateData']),
   (['Vulkan13Features', 'ShaderDemoteToHelperInvocationFeatures'], ['shaderDemoteToHelperInvocation']),
   (['Vulkan13Features', 'ShaderTerminateInvocationFeatures'], ['shaderTerminateInvocation']),
   (['Vulkan13Features', 'SubgroupSizeControlFeatures'], ['subgroupSizeControl', 'computeFullSubgroups']),
   (['Vulkan13Features', 'Synchronization2Features'], ['synchronization2']),
   (['Vulkan13Features', 'TextureCompressionASTCHDRFeatures'], ['textureCompressionASTC_HDR']),
   (['Vulkan13Features', 'ZeroInitializeWorkgroupMemoryFeatures'], ['shaderZeroInitializeWorkgroupMemory']),
   (['Vulkan13Features', 'DynamicRenderingFeatures'], ['dynamicRendering']),
   (['Vulkan13Features', 'ShaderIntegerDotProductFeatures'], ['shaderIntegerDotProduct']),
      ]
      for (feature_structs, features) in KNOWN_ALIASES:
      for flag in features:
      for f in feature_structs:
         rename = (f, flag)
      def get_renamed_feature(c_type, feature):
            @dataclass
   class FeatureStruct:
      c_type: str
   s_type: str
         TEMPLATE_H = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don't edit directly. */
   #ifndef VK_FEATURES_H
   #define VK_FEATURES_H
      #ifdef __cplusplus
   extern "C" {
   #endif
      struct vk_features {
   % for flag in all_flags:
         % endfor
   };
      void
   vk_set_physical_device_features(struct vk_features *all_features,
            void
   vk_set_physical_device_features_1_0(struct vk_features *all_features,
            #ifdef __cplusplus
   }
   #endif
      #endif
   """, output_encoding='utf-8')
      TEMPLATE_C = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don't edit directly. */
      #include "vk_common_entrypoints.h"
   #include "vk_log.h"
   #include "vk_physical_device.h"
   #include "vk_physical_device_features.h"
   #include "vk_util.h"
      static VkResult
   check_physical_device_features(struct vk_physical_device *physical_device,
                     {
   % for flag in pdev_features:
      if (enabled->${flag} && !supported->${flag})
      return vk_errorf(physical_device, VK_ERROR_FEATURE_NOT_PRESENT,
   % endfor
            }
      VkResult
   vk_physical_device_check_device_features(struct vk_physical_device *physical_device,
         {
      VkPhysicalDevice vk_physical_device =
            /* Query the device what kind of features are supported. */
   VkPhysicalDeviceFeatures2 supported_features2 = {
               % for f in feature_structs:
         % endfor
         vk_foreach_struct_const(features, pCreateInfo->pNext) {
      VkBaseOutStructure *supported = NULL;
   % for f in feature_structs:
         case ${f.s_type}:
         % endfor
         default:
                  /* Not a feature struct. */
   if (!supported)
            /* Check for cycles in the list */
   if (supported->pNext != NULL || supported->sType != 0)
            supported->sType = features->sType;
               physical_device->dispatch_table.GetPhysicalDeviceFeatures2(
            if (pCreateInfo->pEnabledFeatures) {
      VkResult result =
   check_physical_device_features(physical_device,
                     if (result != VK_SUCCESS)
               /* Iterate through additional feature structs */
   vk_foreach_struct_const(features, pCreateInfo->pNext) {
      /* Check each feature boolean for given structure. */
   switch (features->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2: {
      const VkPhysicalDeviceFeatures2 *features2 = (const void *)features;
   VkResult result =
      check_physical_device_features(physical_device,
                  if (result != VK_SUCCESS)
      break;
   % for f in feature_structs:
         case ${f.s_type}: {
         % for flag in f.features:
            if (b->${flag} && !a->${flag})
      % endfor
               % endfor
         default:
            } // for each extension structure
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice,
         {
            % for flag in pdev_features:
         % endfor
         vk_foreach_struct(ext, pFeatures) {
      % for f in feature_structs:
         case ${f.s_type}: {
   % for flag in f.features:
         % endfor
                  % endfor
         default:
               }
      void
   vk_set_physical_device_features(struct vk_features *all_features,
         {
      vk_foreach_struct_const(ext, pFeatures) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2: {
      const VkPhysicalDeviceFeatures2 *features = (const void *) ext;
   vk_set_physical_device_features_1_0(all_features, &features->features);
         % for f in feature_structs:
         case ${f.s_type}: {
   % for flag in f.features:
               % endfor
                  % endfor
         default:
               }
      void
   vk_set_physical_device_features_1_0(struct vk_features *all_features,
         {
   % for flag in pdev_features:
      if (pFeatures->${flag})
      % endfor
   }
   """, output_encoding='utf-8')
      def get_pdev_features(doc):
      _type = doc.find(".types/type[@name='VkPhysicalDeviceFeatures']")
   if _type is not None:
      flags = []
   for p in _type.findall('./member'):
         assert p.find('./type').text == 'VkBool32'
            def filter_api(elem, api):
      if 'api' not in elem.attrib:
                  def get_feature_structs(doc, api, beta):
                        # parse all struct types where structextends VkPhysicalDeviceFeatures2
   for _type in doc.findall('./types/type[@category="struct"]'):
      if _type.attrib.get('structextends') != 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo':
         if _type.attrib['name'] not in required:
            # Skip extensions with a define for now
   guard = required[_type.attrib['name']].guard
   if guard is not None and (guard != "VK_ENABLE_BETA_EXTENSIONS" or beta != "true"):
            # find Vulkan structure type
   for elem in _type:
                  # collect a list of feature flags
            for p in _type.findall('./member'):
                        m_name = p.find('./name').text
   if m_name == 'pNext':
         elif m_name == 'sType':
         else:
            feature_struct = FeatureStruct(c_type=_type.attrib.get('name'), s_type=s_type, features=flags)
               def get_feature_structs_from_xml(xml_files, beta, api='vulkan'):
               pdev_features = None
            for filename in xml_files:
      doc = et.parse(filename)
   feature_structs += get_feature_structs(doc, api, beta)
   if not pdev_features:
                           for flag in pdev_features:
            for f in feature_structs:
      for flag in f.features:
         renamed_flag = get_renamed_feature(f.c_type, flag)
   if renamed_flag not in features:
         else:
      a = str_removeprefix(features[renamed_flag], 'VkPhysicalDevice')
                     for rename in unused_renames:
                              def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out-c', required=True, help='Output C file.')
   parser.add_argument('--out-h', required=True, help='Output H file.')
   parser.add_argument('--beta', required=True, help='Enable beta extensions.')
   parser.add_argument('--xml',
                                 environment = {
      'filename': os.path.basename(__file__),
   'pdev_features': pdev_features,
   'feature_structs': feature_structs,
   'all_flags': all_flags,
               try:
      with open(args.out_c, 'wb') as f:
         with open(args.out_h, 'wb') as f:
      except Exception:
      # In the event there's an error, this uses some helpers from mako
   # to print a useful stack trace and prints it, then exits with
   # status 1, if python is run with debug; otherwise it just raises
   # the exception
   print(mako.exceptions.text_error_template().render(), file=sys.stderr)
      if __name__ == '__main__':
      main()
