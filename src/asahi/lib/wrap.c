   /*
   * Copyright 2021-2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
   #include <assert.h>
   #include <dlfcn.h>
   #include <inttypes.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <unistd.h>
      #include <IOKit/IOKitLib.h>
   #include <mach/mach.h>
      #include "util/compiler.h"
   #include "util/u_hexdump.h"
   #include "agx_iokit.h"
   #include "decode.h"
   #include "dyld_interpose.h"
   #include "util.h"
      /*
   * Wrap IOKit entrypoints to intercept communication between the AGX kernel
   * extension and userspace clients. IOKit prototypes are public from the IOKit
   * source release.
   */
      mach_port_t metal_connection = 0;
      kern_return_t
   wrap_Method(mach_port_t connection, uint32_t selector, const uint64_t *input,
               uint32_t inputCnt, const void *inputStruct, size_t inputStructCnt,
   {
      /* Heuristic guess which connection is Metal, skip over I/O from everything
   * else. This is technically wrong but it works in practice, and reduces the
   * surface area we need to wrap.
   */
   if (selector == AGX_SELECTOR_SET_API) {
         } else if (metal_connection != connection) {
      return IOConnectCallMethod(connection, selector, input, inputCnt,
                              /* Check the arguments make sense */
   assert((input != NULL) == (inputCnt != 0));
   assert((inputStruct != NULL) == (inputStructCnt != 0));
   assert((output != NULL) == (outputCnt != 0));
            /* Dump inputs */
   switch (selector) {
   case AGX_SELECTOR_SET_API:
      assert(input == NULL && output == NULL && outputStruct == NULL);
   assert(inputStruct != NULL && inputStructCnt == 16);
            printf("%X: SET_API(%s)\n", connection, (const char *)inputStruct);
         case AGX_SELECTOR_ALLOCATE_MEM: {
      const struct agx_allocate_resource_req *req = inputStruct;
   struct agx_allocate_resource_req *req2 = (void *)inputStruct;
                     printf("Resource allocation:\n");
   printf("  Mode: 0x%X%s\n", req->mode & ~0x800,
         printf("  CPU fixed: 0x%" PRIx64 "\n", req->cpu_fixed);
   printf("  CPU fixed (parent): 0x%" PRIx64 "\n", req->cpu_fixed_parent);
   printf("  Size: 0x%X\n", req->size);
            if (suballocated) {
         } else {
                  for (unsigned i = 0; i < ARRAY_SIZE(req->unk0); ++i) {
      if (req->unk0[i])
               for (unsigned i = 0; i < ARRAY_SIZE(req->unk6); ++i) {
      if (req->unk6[i])
               if (req->unk17)
            if (req->unk19)
            for (unsigned i = 0; i < ARRAY_SIZE(req->unk21); ++i) {
      if (req->unk21[i])
                           case AGX_SELECTOR_SUBMIT_COMMAND_BUFFERS:
      assert(output == NULL && outputStruct == NULL);
   assert(inputStructCnt == sizeof(struct agx_submit_cmdbuf_req));
            printf("%X: SUBMIT_COMMAND_BUFFERS command queue id:%llx %p\n",
                     agxdecode_cmdstream(req->command_buffer_shmem_id,
            if (getenv("ASAHI_DUMP"))
            agxdecode_next_frame();
         default:
      printf("%X: call %s (out %p, %zu)", connection,
                  for (uint64_t u = 0; u < inputCnt; ++u)
            if (inputStructCnt) {
      printf(", struct:\n");
      } else {
                              /* Invoke the real method */
   kern_return_t ret = IOConnectCallMethod(
      connection, selector, input, inputCnt, inputStruct, inputStructCnt,
         if (ret != 0)
            /* Track allocations for later analysis (dumping, disassembly, etc) */
   switch (selector) {
   case AGX_SELECTOR_CREATE_SHMEM: {
      assert(inputCnt == 2);
   assert((*outputStructCntP) == 0x10);
                     assert(type <= 2);
   if (type == 2)
            uint64_t *ptr = (uint64_t *)outputStruct;
            agxdecode_track_alloc(&(struct agx_bo){
      .handle = words[1],
   .ptr.cpu = (void *)*ptr,
                           case AGX_SELECTOR_ALLOCATE_MEM: {
      assert((*outputStructCntP) == 0x50);
   const struct agx_allocate_resource_req *req = inputStruct;
   struct agx_allocate_resource_resp *resp = outputStruct;
   if (resp->cpu && req->cpu_fixed)
         printf("Response:\n");
   printf("  GPU VA: 0x%" PRIx64 "\n", resp->gpu_va);
   printf("  CPU VA: 0x%" PRIx64 "\n", resp->cpu);
   printf("  Handle: %u\n", resp->handle);
   printf("  Root size: 0x%" PRIx64 "\n", resp->root_size);
   printf("  Suballocation size: 0x%" PRIx64 "\n", resp->sub_size);
   printf("  GUID: 0x%X\n", resp->guid);
   for (unsigned i = 0; i < ARRAY_SIZE(resp->unk4); ++i) {
      if (resp->unk4[i])
      }
   for (unsigned i = 0; i < ARRAY_SIZE(resp->unk11); ++i) {
      if (resp->unk11[i])
               if (req->parent)
         else
            agxdecode_track_alloc(&(struct agx_bo){
      .type = AGX_ALLOC_REGULAR,
   .size = resp->sub_size,
   .handle = resp->handle,
   .ptr.gpu = resp->gpu_va,
                           case AGX_SELECTOR_FREE_MEM: {
      assert(inputCnt == 1);
   assert(inputStruct == NULL);
   assert(output == NULL);
            agxdecode_track_free(
                        case AGX_SELECTOR_FREE_SHMEM: {
      assert(inputCnt == 1);
   assert(inputStruct == NULL);
   assert(output == NULL);
            agxdecode_track_free(
                        default:
      /* Dump the outputs */
   if (outputCnt) {
                                          if (outputStructCntP) {
                     if (selector == 2) {
      /* Dump linked buffer as well */
   void **o = outputStruct;
                  printf("\n");
                  }
      kern_return_t
   wrap_AsyncMethod(mach_port_t connection, uint32_t selector,
                  mach_port_t wakePort, uint64_t *reference,
   uint32_t referenceCnt, const uint64_t *input,
      {
      /* Check the arguments make sense */
   assert((input != NULL) == (inputCnt != 0));
   assert((inputStruct != NULL) == (inputStructCnt != 0));
   assert((output != NULL) == (outputCnt != 0));
            printf("%X: call %X, wake port %X (out %p, %zu)", connection, selector,
            for (uint64_t u = 0; u < inputCnt; ++u)
            if (inputStructCnt) {
      printf(", struct:\n");
      } else {
                  printf(", references: ");
   for (unsigned i = 0; i < referenceCnt; ++i)
                  kern_return_t ret = IOConnectCallAsyncMethod(
      connection, selector, wakePort, reference, referenceCnt, input, inputCnt,
   inputStruct, inputStructCnt, output, outputCnt, outputStruct,
                  if (outputCnt) {
               for (uint64_t u = 0; u < *outputCnt; ++u)
                        if (outputStructCntP) {
      printf(" struct\n");
            if (selector == 2) {
      /* Dump linked buffer as well */
   void **o = outputStruct;
                  printf("\n");
      }
      kern_return_t
   wrap_StructMethod(mach_port_t connection, uint32_t selector,
               {
      return wrap_Method(connection, selector, NULL, 0, inputStruct,
            }
      kern_return_t
   wrap_AsyncStructMethod(mach_port_t connection, uint32_t selector,
                           {
      return wrap_AsyncMethod(connection, selector, wakePort, reference,
            }
      kern_return_t
   wrap_ScalarMethod(mach_port_t connection, uint32_t selector,
               {
      return wrap_Method(connection, selector, input, inputCnt, NULL, 0, output,
      }
      kern_return_t
   wrap_AsyncScalarMethod(mach_port_t connection, uint32_t selector,
                     {
      return wrap_AsyncMethod(connection, selector, wakePort, reference,
            }
      mach_port_t
   wrap_DataQueueAllocateNotificationPort()
   {
      mach_port_t ret = IODataQueueAllocateNotificationPort();
   printf("Allocated notif port %X\n", ret);
      }
      kern_return_t
   wrap_SetNotificationPort(io_connect_t connect, uint32_t type, mach_port_t port,
         {
      printf(
      "Set noficiation port connect=%X, type=%X, port=%X, reference=%" PRIx64
   "\n",
            }
      IOReturn
   wrap_DataQueueWaitForAvailableData(IODataQueueMemory *dataQueue,
         {
      printf("Waiting for data queue at notif port %X\n", notificationPort);
   IOReturn ret = IODataQueueWaitForAvailableData(dataQueue, notificationPort);
   printf("ret=%X\n", ret);
      }
      IODataQueueEntry *
   wrap_DataQueuePeek(IODataQueueMemory *dataQueue)
   {
      printf("Peeking data queue\n");
      }
      IOReturn
   wrap_DataQueueDequeue(IODataQueueMemory *dataQueue, void *data,
         {
      printf("Dequeueing (dataQueue=%p, data=%p, buffer %u)\n", dataQueue, data,
         IOReturn ret = IODataQueueDequeue(dataQueue, data, dataSize);
            uint8_t *data8 = data;
   for (unsigned i = 0; i < *dataSize; ++i) {
         }
               }
      DYLD_INTERPOSE(wrap_Method, IOConnectCallMethod);
   DYLD_INTERPOSE(wrap_AsyncMethod, IOConnectCallAsyncMethod);
   DYLD_INTERPOSE(wrap_StructMethod, IOConnectCallStructMethod);
   DYLD_INTERPOSE(wrap_AsyncStructMethod, IOConnectCallAsyncStructMethod);
   DYLD_INTERPOSE(wrap_ScalarMethod, IOConnectCallScalarMethod);
   DYLD_INTERPOSE(wrap_AsyncScalarMethod, IOConnectCallAsyncScalarMethod);
   DYLD_INTERPOSE(wrap_SetNotificationPort, IOConnectSetNotificationPort);
   DYLD_INTERPOSE(wrap_DataQueueAllocateNotificationPort,
         DYLD_INTERPOSE(wrap_DataQueueWaitForAvailableData,
         DYLD_INTERPOSE(wrap_DataQueuePeek, IODataQueuePeek);
   DYLD_INTERPOSE(wrap_DataQueueDequeue, IODataQueueDequeue);
