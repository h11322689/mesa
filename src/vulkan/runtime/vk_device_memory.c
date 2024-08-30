   /*
   * Copyright © 2023 Collabora, Ltd.
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
      #include "vk_device_memory.h"
      #include "vk_android.h"
   #include "vk_common_entrypoints.h"
   #include "vk_util.h"
      #if defined(ANDROID) && ANDROID_API_LEVEL >= 26
   #include <vndk/hardware_buffer.h>
   #endif
      void *
   vk_device_memory_create(struct vk_device *device,
                     {
      struct vk_device_memory *mem =
         if (mem == NULL)
                     mem->size = pAllocateInfo->allocationSize;
            vk_foreach_struct_const(ext, pAllocateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO: {
      const VkExportMemoryAllocateInfo *export_info = (void *)ext;
   mem->export_handle_types = export_info->handleTypes;
               #if defined(ANDROID) && ANDROID_API_LEVEL >= 26
                     assert(mem->import_handle_type == 0);
                  /* From the Vulkan 1.3.242 spec:
   *
   *    "If the vkAllocateMemory command succeeds, the implementation
   *    must acquire a reference to the imported hardware buffer, which
   *    it must release when the device memory object is freed. If the
   *    command fails, the implementation must not retain a
   *    reference."
   *
   * We assume that if the driver fails to create its memory object,
   * it will call vk_device_memory_destroy which will delete our
   * reference.
   */
   AHardwareBuffer_acquire(ahb_info->buffer);
      #else
         #endif /* ANDROID_API_LEVEL >= 26 */
                  case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR: {
      const VkImportMemoryFdInfoKHR *fd_info = (void *)ext;
   if (fd_info->handleType) {
      assert(fd_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
         assert(mem->import_handle_type == 0);
      }
               case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT: {
      const VkImportMemoryHostPointerInfoEXT *host_ptr_info = (void *)ext;
   if (host_ptr_info->handleType) {
                     assert(mem->import_handle_type == 0);
   mem->import_handle_type = host_ptr_info->handleType;
      }
               #ifdef VK_USE_PLATFORM_WIN32_KHR
            const VkImportMemoryWin32HandleInfoKHR *w32h_info = (void *)ext;
   if (w32h_info->handleType) {
      assert(w32h_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT ||
         w32h_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT ||
   w32h_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT ||
   w32h_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT ||
   w32h_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT ||
   assert(mem->import_handle_type == 0);
         #else
         #endif
                  case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO: {
      const VkMemoryAllocateFlagsInfo *flags_info = (void *)ext;
   mem->alloc_flags = flags_info->flags;
               default:
                     /* From the Vulkan Specification 1.3.261:
   *
   *    VUID-VkMemoryAllocateInfo-allocationSize-07897
   *
   *   "If the parameters do not define an import or export operation,
   *   allocationSize must be greater than 0."
   */
   if (!mem->import_handle_type && !mem->export_handle_types)
            /* From the Vulkan Specification 1.3.261:
   *
   *    VUID-VkMemoryAllocateInfo-allocationSize-07899
   *
   *    "If the parameters define an export operation and the handle type is
   *    not VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
   *    allocationSize must be greater than 0."
   */
   if (mem->export_handle_types &&
      mem->export_handle_types !=
               if ((mem->export_handle_types &
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) &&
   mem->ahardware_buffer == NULL) {
   /* If we need to be able to export an Android hardware buffer but none
   * is provided as an import, create a new one.
   */
   mem->ahardware_buffer = vk_alloc_ahardware_buffer(pAllocateInfo);
   if (mem->ahardware_buffer == NULL) {
      vk_device_memory_destroy(device, alloc, mem);
                     }
      void
   vk_device_memory_destroy(struct vk_device *device,
               {
   #if defined(ANDROID) && ANDROID_API_LEVEL >= 26
      if (mem->ahardware_buffer)
      #endif /* ANDROID_API_LEVEL >= 26 */
            }
      #if defined(ANDROID) && ANDROID_API_LEVEL >= 26
   VkResult
   vk_common_GetMemoryAndroidHardwareBufferANDROID(
      VkDevice _device,
   const VkMemoryGetAndroidHardwareBufferInfoANDROID *pInfo,
      {
               /* Some quotes from Vulkan spec:
   *
   *    "If the device memory was created by importing an Android hardware
   *    buffer, vkGetMemoryAndroidHardwareBufferANDROID must return that same
   *    Android hardware buffer object."
   *
   *    "VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID
   *    must have been included in VkExportMemoryAllocateInfo::handleTypes
   *    when memory was created."
   */
   if (mem->ahardware_buffer) {
      *pBuffer = mem->ahardware_buffer;
   /* Increase refcount. */
   AHardwareBuffer_acquire(*pBuffer);
                  }
   #endif
