   /*
   * Copyright © 2020 Intel Corporation
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
      #include "anv_private.h"
      #include <math.h>
      #include "util/u_debug.h"
   #include "util/half_float.h"
   #include "util/u_atomic.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
   #include "genxml/genX_rt_pack.h"
      #include "ds/intel_tracepoints.h"
      #if GFX_VERx10 == 125
   #include "grl/grl_structs.h"
      /* Wait for the previous dispatches to finish and flush their data port
   * writes.
   */
   #define ANV_GRL_FLUSH_FLAGS (ANV_PIPE_END_OF_PIPE_SYNC_BIT | \
                  static const VkAccelerationStructureGeometryKHR *
   get_geometry(const VkAccelerationStructureBuildGeometryInfoKHR *pInfo,
         {
      return pInfo->pGeometries ? &pInfo->pGeometries[index] :
      }
      static size_t align_transient_size(size_t bytes)
   {
         }
      static size_t align_private_size(size_t bytes)
   {
         }
      static size_t get_scheduler_size(size_t num_builds)
   {
      size_t scheduler_size = sizeof(union SchedulerUnion);
   /* add more memory for qnode creation stage if needed */
   if (num_builds > QNODE_GLOBAL_ROOT_BUFFER_MIN_ENTRIES_NUM) {
      scheduler_size += (num_builds - QNODE_GLOBAL_ROOT_BUFFER_MIN_ENTRIES_NUM) * 2 *
                  }
      static size_t
   get_batched_binnedsah_transient_mem_size(size_t num_builds)
   {
      if (num_builds == 0)
            }
      static size_t
   get_batched_binnedsah_private_mem_size(size_t num_builds)
   {
      if (num_builds == 0)
            size_t globals_size = align_private_size(num_builds * sizeof(struct SAHBuildGlobals));
      }
      static uint32_t
   estimate_qbvh6_nodes(const uint32_t N)
   {
      const uint32_t W = 6;
   const uint32_t N0 = N / 2 + N % 2; // lowest level with 2 leaves per QBVH6 node
   const uint32_t N1 = N0 / W + (N0 % W ? 1 : 0); // filled level
   const uint32_t N2 = N0 / W + (N1 % W ? 1 : 0); // filled level
   const uint32_t N3 = N0 / W + (N2 % W ? 1 : 0); // filled level
   const uint32_t N4 = N3; // overestimate remaining nodes
      }
      /* Estimates the worst case number of QBVH6 nodes for a top-down BVH
   * build that guarantees to produce subtree with N >= K primitives
   * from which a single QBVH6 node is created.
   */
   static uint32_t
   estimate_qbvh6_nodes_minK(const uint32_t N, uint32_t K)
   {
      const uint32_t N0 = N / K + (N % K ? 1 : 0); // lowest level of nodes with K leaves minimally
      }
      static size_t
   estimate_qbvh6_fatleafs(const size_t P)
   {
         }
      static size_t
   estimate_qbvh6_nodes_worstcase(const size_t P)
   {
               // worst-case each inner node having 5 fat-leaf children.
   //  number of inner nodes is F/5 and number of fat-leaves is F
      }
      #define sizeof_PrimRef      32
   #define sizeof_HwInstanceLeaf (GENX(RT_BVH_INSTANCE_LEAF_length) * 4)
   #define sizeof_InternalNode   (GENX(RT_BVH_INTERNAL_NODE_length) * 4)
   #define sizeof_Procedural     (GENX(RT_BVH_PROCEDURAL_LEAF_length) * 4)
   #define sizeof_Quad           (GENX(RT_BVH_QUAD_LEAF_length) * 4)
      static struct MKSizeEstimate
   get_gpu_size_estimate(const VkAccelerationStructureBuildGeometryInfoKHR *pInfo,
               {
      uint32_t num_triangles = 0, num_aabbs = 0, num_instances = 0;
   for (unsigned g = 0; g < pInfo->geometryCount; g++) {
      const VkAccelerationStructureGeometryKHR *pGeometry =
         uint32_t prim_count = pBuildRangeInfos != NULL ?
            switch (pGeometry->geometryType) {
   case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      num_triangles += prim_count;
      case VK_GEOMETRY_TYPE_AABBS_KHR:
      num_aabbs += prim_count;
      case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      num_instances += prim_count;
      default:
            }
                     uint64_t size = sizeof(BVHBase);
            /* Must immediately follow BVHBase because we use fixed offset to nodes. */
            switch (pInfo->type) {
   case VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR: {
               est.numPrimitives = num_instances;
   est.numPrimitivesToSplit = 0;
            est.min_primitives = est.numPrimitives;
            unsigned int sizeInnerNodes =
      (unsigned int) estimate_qbvh6_nodes_worstcase(est.numBuildPrimitives) *
      if (sizeInnerNodes == 0)
                     size += sizeInnerNodes;
            est.leaf_data_start = size;
   size += est.numBuildPrimitives * sizeof_HwInstanceLeaf;
                                 case VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR: {
               /* RT: TODO */
   const float split_factor = 0.0f;
   uint32_t num_prims_to_split = 0;
   if (false)
            const uint32_t num_build_triangles = num_triangles + num_prims_to_split;
            est.numPrimitives = num_primitives;
   est.numTriangles = num_triangles;
   est.numProcedurals = num_aabbs;
   est.numMeshes = pInfo->geometryCount;
   est.numBuildPrimitives = num_build_primitives;
   est.numPrimitivesToSplit = num_prims_to_split;
            est.min_primitives = (size_t)(num_build_triangles * 0.5f + num_aabbs);
            size_t nodeBytes = 0;
   nodeBytes += estimate_qbvh6_nodes_worstcase(num_build_triangles) * sizeof_InternalNode;
   nodeBytes += estimate_qbvh6_nodes_worstcase(num_aabbs) * sizeof_InternalNode;
   if (nodeBytes == 0) // for case with 0 primitives
                           size += nodeBytes;
            est.leaf_data_start = size;
   size += num_build_triangles * sizeof_Quad;
            est.procedural_data_start = size;
   size += num_aabbs * sizeof_Procedural;
            est.leaf_data_size = num_build_triangles * sizeof_Quad +
            if (num_build_primitives == 0)
                     default:
                  size = align64(size, 64);
   est.instance_descs_start = size;
            est.geo_meta_data_start = size;
   size += sizeof(struct GeoMetaData) * pInfo->geometryCount;
            assert(size == align64(size, 64));
            const bool alloc_backpointers = false; /* RT TODO */
   if (alloc_backpointers) {
      size += est.max_inner_nodes * sizeof(uint32_t);
               assert(size < UINT32_MAX);
               }
      struct scratch_layout {
      gpuva_t base;
            gpuva_t primrefs;
   gpuva_t globals;
   gpuva_t leaf_index_buffers;
            /* new_sah */
   gpuva_t qnode_buffer;
      };
      static size_t
   get_bvh2_size(uint32_t num_primitivies)
   {
      if (num_primitivies == 0)
         return sizeof(struct BVH2) +
      }
      static struct scratch_layout
   get_gpu_scratch_layout(struct anv_address base,
               {
      struct scratch_layout scratch = {
         };
            scratch.globals = current;
            scratch.primrefs = intel_canonical_address(current);
            scratch.leaf_index_buffers = intel_canonical_address(current);
   current += est.numBuildPrimitives * sizeof(uint32_t) * 2;
            switch (build_method) {
   case ANV_BVH_BUILD_METHOD_TRIVIAL:
            case ANV_BVH_BUILD_METHOD_NEW_SAH: {
      size_t bvh2_size = get_bvh2_size(est.numBuildPrimitives);
   if (est.leaf_data_size < bvh2_size) {
      scratch.bvh2_buffer = intel_canonical_address(current);
               scratch.qnode_buffer = intel_canonical_address(current);
   current += 2 * sizeof(dword) * est.max_inner_nodes;
               default:
                  assert((current - scratch.base) < UINT32_MAX);
               }
      static void
   anv_get_gpu_acceleration_structure_size(
      UNUSED struct anv_device                   *device,
   VkAccelerationStructureBuildTypeKHR         buildType,
   const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
   const uint32_t*                             pMaxPrimitiveCounts,
      {
         struct MKSizeEstimate est = get_gpu_size_estimate(pBuildInfo, NULL,
         struct scratch_layout scratch = get_gpu_scratch_layout(ANV_NULL_ADDRESS, est,
            pSizeInfo->accelerationStructureSize = est.sizeTotal;
   pSizeInfo->buildScratchSize = scratch.total_size;
      }
      void
   genX(GetAccelerationStructureBuildSizesKHR)(
      VkDevice                                    _device,
   VkAccelerationStructureBuildTypeKHR         buildType,
   const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
   const uint32_t*                             pMaxPrimitiveCounts,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   assert(pSizeInfo->sType ==
            VkAccelerationStructureBuildSizesInfoKHR gpu_size_info;
   anv_get_gpu_acceleration_structure_size(device, buildType, pBuildInfo,
                  pSizeInfo->accelerationStructureSize =
         pSizeInfo->buildScratchSize = gpu_size_info.buildScratchSize;
      }
      void
   genX(GetDeviceAccelerationStructureCompatibilityKHR)(
      VkDevice                                    _device,
   const VkAccelerationStructureVersionInfoKHR* pVersionInfo,
      {
               if (memcmp(pVersionInfo->pVersionData,
            device->physical->rt_uuid,
      } else {
            }
      static inline uint8_t
   vk_to_grl_GeometryFlags(VkGeometryFlagsKHR flags)
   {
      uint8_t grl_flags = GEOMETRY_FLAG_NONE;
   unsigned mask = flags;
   while (mask) {
      int i = u_bit_scan(&mask);
   switch ((VkGeometryFlagBitsKHR)(1u << i)) {
   case VK_GEOMETRY_OPAQUE_BIT_KHR:
      grl_flags |= GEOMETRY_FLAG_OPAQUE;
      case VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR:
      grl_flags |= GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
      default:
            }
      }
      static inline IndexFormat
   vk_to_grl_IndexFormat(VkIndexType type)
   {
      switch (type) {
   case VK_INDEX_TYPE_NONE_KHR:  return INDEX_FORMAT_NONE;
   case VK_INDEX_TYPE_UINT8_EXT: unreachable("No UINT8 support yet");
   case VK_INDEX_TYPE_UINT16:    return INDEX_FORMAT_R16_UINT;
   case VK_INDEX_TYPE_UINT32:    return INDEX_FORMAT_R32_UINT;
   default:
            }
      static inline VertexFormat
   vk_to_grl_VertexFormat(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_R32G32_SFLOAT:       return VERTEX_FORMAT_R32G32_FLOAT;
   case VK_FORMAT_R32G32B32_SFLOAT:    return VERTEX_FORMAT_R32G32B32_FLOAT;
   case VK_FORMAT_R16G16_SFLOAT:       return VERTEX_FORMAT_R16G16_FLOAT;
   case VK_FORMAT_R16G16B16A16_SFLOAT: return VERTEX_FORMAT_R16G16B16A16_FLOAT;
   case VK_FORMAT_R16G16_SNORM:        return VERTEX_FORMAT_R16G16_SNORM;
   case VK_FORMAT_R16G16B16A16_SNORM:  return VERTEX_FORMAT_R16G16B16A16_SNORM;
   case VK_FORMAT_R16G16B16A16_UNORM:  return VERTEX_FORMAT_R16G16B16A16_UNORM;
   case VK_FORMAT_R16G16_UNORM:        return VERTEX_FORMAT_R16G16_UNORM;
   /* case VK_FORMAT_R10G10B10A2_UNORM:   return VERTEX_FORMAT_R10G10B10A2_UNORM; */
   case VK_FORMAT_R8G8B8A8_UNORM:      return VERTEX_FORMAT_R8G8B8A8_UNORM;
   case VK_FORMAT_R8G8_UNORM:          return VERTEX_FORMAT_R8G8_UNORM;
   case VK_FORMAT_R8G8B8A8_SNORM:      return VERTEX_FORMAT_R8G8B8A8_SNORM;
   case VK_FORMAT_R8G8_SNORM:          return VERTEX_FORMAT_R8G8_SNORM;
   default:
            }
      static struct Geo
   vk_to_grl_Geo(const VkAccelerationStructureGeometryKHR *pGeometry,
               uint32_t prim_count,
   uint32_t transform_offset,
   {
      struct Geo geo = {
                  switch (pGeometry->geometryType) {
   case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
      const VkAccelerationStructureGeometryTrianglesDataKHR *vk_tri =
                     geo.Desc.Triangles.pTransformBuffer =
         geo.Desc.Triangles.pIndexBuffer =
         geo.Desc.Triangles.pVertexBuffer =
                  if (geo.Desc.Triangles.pTransformBuffer)
            if (vk_tri->indexType == VK_INDEX_TYPE_NONE_KHR) {
      geo.Desc.Triangles.IndexCount = 0;
   geo.Desc.Triangles.VertexCount = prim_count * 3;
   geo.Desc.Triangles.IndexFormat = INDEX_FORMAT_NONE;
      } else {
      geo.Desc.Triangles.IndexCount = prim_count * 3;
   geo.Desc.Triangles.VertexCount = vk_tri->maxVertex;
   geo.Desc.Triangles.IndexFormat =
                     geo.Desc.Triangles.VertexFormat =
         geo.Desc.Triangles.pVertexBuffer += vk_tri->vertexStride * first_vertex;
               case VK_GEOMETRY_TYPE_AABBS_KHR: {
      const VkAccelerationStructureGeometryAabbsDataKHR *vk_aabbs =
         geo.Type = GEOMETRY_TYPE_PROCEDURAL;
   geo.Desc.Procedural.pAABBs_GPUVA =
         geo.Desc.Procedural.AABBByteStride = vk_aabbs->stride;
   geo.Desc.Procedural.AABBCount = prim_count;
               default:
                     }
      #include "grl/grl_metakernel_copy.h"
   #include "grl/grl_metakernel_misc.h"
   #include "grl/grl_metakernel_build_primref.h"
   #include "grl/grl_metakernel_new_sah_builder.h"
   #include "grl/grl_metakernel_build_leaf.h"
      struct build_state {
               struct MKSizeEstimate estimate;
   struct scratch_layout scratch;
                     size_t geom_size_prefix_sum_buffer;
            uint32_t leaf_type;
            uint32_t num_geometries;
            uint64_t instances_addr;
               };
      static void
   get_binnedsah_scratch_buffers(struct build_state *bs,
                     {
      if (bs->estimate.numBuildPrimitives == 0)
   {
      *p_qnode_buffer = 0;
         *p_primref_indices = 0;
               size_t bvh2_size = get_bvh2_size(bs->estimate.numBuildPrimitives);
   if (bs->estimate.leaf_data_size < bvh2_size) {
      assert(bs->scratch.bvh2_buffer != 0);
      } else {
      *p_bvh2 = intel_canonical_address(bs->state.bvh_buffer +
               assert(bs->scratch.qnode_buffer != 0);
            assert(bs->scratch.leaf_index_buffers != 0);
      }
      static void
   write_memory(struct anv_cmd_alloc alloc, size_t offset, const void *data, size_t data_len)
   {
      assert((offset + data_len) < alloc.size);
      }
      static void
   cmd_build_acceleration_structures(
      struct anv_cmd_buffer *cmd_buffer,
   uint32_t infoCount,
   const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
   const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos,
   const VkDeviceAddress *pIndirectDeviceAddresses,
   const uint32_t *pIndirectStrides,
      {
      struct anv_device *device = cmd_buffer->device;
            struct build_state *builds;
            if (!vk_multialloc_zalloc(&ma,
                  anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_OUT_OF_HOST_MEMORY);
                        /* TODO: Indirect */
            size_t transient_mem_init_globals_size = 0;
                              size_t num_trivial_builds = 0;
            /* Prepare a bunch of data for the kernels we have to run. */
   for (uint32_t i = 0; i < infoCount; i++) {
               const VkAccelerationStructureBuildGeometryInfoKHR *pInfo = &pInfos[i];
   struct anv_address scratch_addr =
            const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfos =
         const uint32_t *pMaxPrimitiveCounts =
            ANV_FROM_HANDLE(vk_acceleration_structure, dst_accel,
                              bs->estimate = get_gpu_size_estimate(pInfo, pBuildRangeInfos,
         bs->scratch = get_gpu_scratch_layout(scratch_addr, bs->estimate,
                     switch (pInfo->type) {
   case VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR: {
               const VkAccelerationStructureGeometryKHR *pGeometry =
                                 bs->num_instances = pBuildRangeInfos[0].primitiveCount;
   bs->instances_addr = instances->data.deviceAddress;
   bs->array_of_instances_ptr = instances->arrayOfPointers;
   leaf_type = NODE_TYPE_INSTANCE;
   leaf_size = GENX(RT_BVH_INSTANCE_LEAF_length) * 4;
               case VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR: {
      bs->num_geometries = pInfo->geometryCount;
   leaf_type = NODE_TYPE_QUAD;
   leaf_size = GENX(RT_BVH_QUAD_LEAF_length) * 4;
               default:
                  size_t geom_struct_size = bs->num_geometries * sizeof(struct Geo);
                              bs->state = (struct MKBuilderState) {
      .geomDesc_buffer = bs->geom_size_prefix_sum_buffer +
         .build_primref_buffer = bs->scratch.primrefs,
   .build_globals = bs->scratch.globals,
   .bvh_buffer = anv_address_physical(bs->bvh_addr),
   .leaf_type = leaf_type,
                        switch (device->bvh_build_method) {
   case ANV_BVH_BUILD_METHOD_TRIVIAL:
      num_trivial_builds++;
      case ANV_BVH_BUILD_METHOD_NEW_SAH:
      num_new_sah_builds++;
      default:
                              transient_total = align_transient_size(transient_total);
   transient_mem_init_globals_offset = transient_total;
            size_t transient_mem_binnedsah_size = 0;
   size_t transient_mem_binnedsah_offset = 0;
   size_t private_mem_binnedsah_size = 0;
            transient_mem_binnedsah_size = get_batched_binnedsah_transient_mem_size(num_new_sah_builds);
   transient_mem_binnedsah_offset = transient_total;
            private_mem_binnedsah_size = get_batched_binnedsah_private_mem_size(num_new_sah_builds);
   private_mem_binnedsah_offset = private_mem_total;
            /* Allocate required memory, unless we already have a suiteable buffer */
   struct anv_cmd_alloc private_mem_alloc;
   if (private_mem_total > cmd_buffer->state.rt.build_priv_mem_size) {
      private_mem_alloc =
      anv_cmd_buffer_alloc_space(cmd_buffer, private_mem_total, 64,
      if (anv_cmd_alloc_is_empty(private_mem_alloc)) {
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_OUT_OF_DEVICE_MEMORY);
               cmd_buffer->state.rt.build_priv_mem_addr = private_mem_alloc.address;
      } else {
      private_mem_alloc = (struct anv_cmd_alloc) {
      .address = cmd_buffer->state.rt.build_priv_mem_addr,
   .map     = anv_address_map(cmd_buffer->state.rt.build_priv_mem_addr),
                  struct anv_cmd_alloc transient_mem_alloc =
      anv_cmd_buffer_alloc_space(cmd_buffer, transient_total, 64,
      if (transient_total > 0 && anv_cmd_alloc_is_empty(transient_mem_alloc)) {
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_OUT_OF_DEVICE_MEMORY);
               uint64_t private_base = anv_address_physical(private_mem_alloc.address);
            /* Prepare transient memory */
   for (uint32_t i = 0; i < infoCount; i++) {
                        const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfos =
            struct Geo *geos = transient_mem_alloc.map + bs->state.geomDesc_buffer;
   uint32_t *prefixes = transient_mem_alloc.map + bs->geom_size_prefix_sum_buffer;
   uint32_t prefix_sum = 0;
   for (unsigned g = 0; g < bs->num_geometries; g++) {
      const VkAccelerationStructureGeometryKHR *pGeometry = get_geometry(pInfo, g);
   uint32_t prim_count = pBuildRangeInfos[g].primitiveCount;
   geos[g] = vk_to_grl_Geo(pGeometry, prim_count,
                        prefixes[g] = prefix_sum;
                        bs->geom_size_prefix_sum_buffer =
      intel_canonical_address(bs->geom_size_prefix_sum_buffer +
      bs->state.geomDesc_buffer =
                  struct BatchedInitGlobalsData data = {
                     .numPrimitives = 0,
                  .instance_descs_start = bs->estimate.instance_descs_start,
   .geo_meta_data_start = bs->estimate.geo_meta_data_start,
   .node_data_start = bs->estimate.node_data_start,
   .leaf_data_start = bs->estimate.leaf_data_start,
   .procedural_data_start = bs->estimate.procedural_data_start,
                  .leafType = bs->state.leaf_type,
               write_memory(transient_mem_alloc,
                     if (anv_cmd_buffer_is_render_queue(cmd_buffer))
            /* Due to the nature of GRL and its heavy use of jumps/predication, we
   * cannot tell exactly in what order the CFE_STATE we insert are going to
   * be executed. So always use the largest possible size.
   */
   genX(cmd_buffer_ensure_cfe_state)(
      cmd_buffer,
         /* Round 1 : init_globals kernel */
   genX(grl_misc_batched_init_globals)(
      cmd_buffer,
   intel_canonical_address(transient_base +
               anv_add_pending_pipe_bits(cmd_buffer,
                  /* Round 2 : Copy instance/geometry data from the application provided
   *           buffers into the acceleration structures.
   */
   for (uint32_t i = 0; i < infoCount; i++) {
               /* Metadata */
   if (bs->num_instances) {
               const uint64_t copy_size = bs->num_instances * sizeof(InstanceDesc);
   /* This must be calculated in same way as
   * groupCountForGeoMetaDataCopySize
                  if (bs->array_of_instances_ptr) {
      genX(grl_misc_copy_instance_ptrs)(
      cmd_buffer,
   anv_address_physical(anv_address_add(bs->bvh_addr,
         bs->instances_addr,
   } else {
      genX(grl_misc_copy_instances)(
      cmd_buffer,
   anv_address_physical(anv_address_add(bs->bvh_addr,
         bs->instances_addr,
               if (bs->num_geometries) {
                     /* This must be calculated in same way as
   * groupCountForGeoMetaDataCopySize
                  genX(grl_misc_copy_geo_meta_data)(
      cmd_buffer,
   anv_address_physical(anv_address_add(bs->bvh_addr,
         bs->state.geomDesc_buffer,
   copy_size,
            /* Primrefs */
   if (bs->num_instances) {
      if (bs->array_of_instances_ptr) {
      genX(grl_build_primref_buildPrimirefsFromInstancesArrOfPtrs)(
      cmd_buffer,
   bs->instances_addr,
   PREFIX_MK_SIZE(grl_build_primref, bs->estimate),
   PREFIX_MK_STATE(grl_build_primref, bs->state),
   } else {
      genX(grl_build_primref_buildPrimirefsFromInstances)(
      cmd_buffer,
   bs->instances_addr,
   PREFIX_MK_SIZE(grl_build_primref, bs->estimate),
   PREFIX_MK_STATE(grl_build_primref, bs->state),
               if (bs->num_geometries) {
      const VkAccelerationStructureBuildGeometryInfoKHR *pInfo = &pInfos[i];
                  assert(pInfo->geometryCount == bs->num_geometries);
   for (unsigned g = 0; g < pInfo->geometryCount; g++) {
                     switch (pGeometry->geometryType) {
   case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      genX(grl_build_primref_primrefs_from_tris)(
      cmd_buffer,
   PREFIX_MK_STATE(grl_build_primref, bs->state),
   PREFIX_MK_SIZE(grl_build_primref, bs->estimate),
   bs->state.geomDesc_buffer + g * sizeof(struct Geo),
   g,
   vk_to_grl_GeometryFlags(pGeometry->flags),
                  case VK_GEOMETRY_TYPE_AABBS_KHR:
      genX(grl_build_primref_primrefs_from_proc)(
      cmd_buffer,
   PREFIX_MK_STATE(grl_build_primref, bs->state),
   PREFIX_MK_SIZE(grl_build_primref, bs->estimate),
   bs->state.geomDesc_buffer + g * sizeof(struct Geo),
   g,
   vk_to_grl_GeometryFlags(pGeometry->flags),
                  default:
                           anv_add_pending_pipe_bits(cmd_buffer,
                  /* Dispatch trivial builds */
   if (num_trivial_builds) {
      for (uint32_t i = 0; i < infoCount; i++) {
                              genX(grl_new_sah_builder_single_pass_binsah)(
      cmd_buffer,
   bs->scratch.globals,
   bs->state.bvh_buffer,
   bs->state.build_primref_buffer,
   bs->scratch.leaf_index_buffers,
               /* Dispatch new SAH builds */
   if (num_new_sah_builds) {
      size_t global_ptrs_offset  = transient_mem_binnedsah_offset;
            size_t scheduler_offset   = private_mem_binnedsah_offset;
            struct SAHBuildArgsBatchable args = {
      .num_builds                               = infoCount,
   .p_globals_ptrs                           = intel_canonical_address(transient_base + global_ptrs_offset),
   .p_buffers_info                           = intel_canonical_address(transient_base + buffers_info_offset),
   .p_scheduler                              = intel_canonical_address(private_base + scheduler_offset),
   .p_sah_globals                            = intel_canonical_address(private_base + sah_globals_offset),
               for (uint32_t i = 0; i < infoCount; i++) {
                              uint64_t p_build_primref_index_buffers;
                  get_binnedsah_scratch_buffers(bs,
                        struct SAHBuildBuffersInfo buffers = {
      .p_primref_index_buffers  = bs->scratch.leaf_index_buffers,
   .p_bvh_base               = bs->state.bvh_buffer,
   .p_primrefs_buffer        = bs->state.build_primref_buffer,
   .p_bvh2                   = p_bvh2,
   .p_qnode_root_buffer      = p_qnode_child_buffer,
                              write_memory(transient_mem_alloc, global_ptrs_offset, &bs->state.build_globals,
                     genX(grl_new_sah_builder_new_sah_build_batchable)(
               if (num_new_sah_builds == 0)
      anv_add_pending_pipe_bits(cmd_buffer,
               /* Finally write the leaves. */
   for (uint32_t i = 0; i < infoCount; i++) {
               if (bs->num_instances) {
      assert(bs->num_geometries == 0);
   if (bs->array_of_instances_ptr) {
      genX(grl_leaf_builder_buildLeafDXR_instances_pointers)(cmd_buffer,
      PREFIX_MK_STATE(grl_leaf_builder, bs->state),
   bs->scratch.leaf_index_buffers,
   bs->instances_addr,
   bs->scratch.leaf_index_buffer_stride,
   0 /* offset */,
   } else {
      genX(grl_leaf_builder_buildLeafDXR_instances)(cmd_buffer,
      PREFIX_MK_STATE(grl_leaf_builder, bs->state),
   bs->scratch.leaf_index_buffers,
   bs->instances_addr,
   bs->scratch.leaf_index_buffer_stride,
   0 /* offset */,
               if (bs->num_geometries) {
      assert(bs->num_instances == 0);
                  assert(bs->estimate.numProcedurals == 0 ||
         if (bs->estimate.numProcedurals) {
      genX(grl_leaf_builder_buildLeafDXR_procedurals)(
      cmd_buffer,
   PREFIX_MK_STATE(grl_leaf_builder, bs->state),
   bs->scratch.leaf_index_buffers,
   bs->scratch.leaf_index_buffer_stride,
   0 /* offset */,
   } else {
      genX(grl_leaf_builder_buildLeafDXR_quads)(
      cmd_buffer,
   PREFIX_MK_STATE(grl_leaf_builder, bs->state),
   bs->scratch.leaf_index_buffers,
   bs->scratch.leaf_index_buffer_stride,
   0 /* offset */,
   p_numPrimitives,
                  anv_add_pending_pipe_bits(cmd_buffer,
                        error:
         }
      void
   genX(CmdBuildAccelerationStructuresKHR)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    infoCount,
   const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
      {
               if (anv_batch_has_error(&cmd_buffer->batch))
            cmd_build_acceleration_structures(cmd_buffer, infoCount, pInfos,
      }
      void
   genX(CmdBuildAccelerationStructuresIndirectKHR)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    infoCount,
   const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
   const VkDeviceAddress*                      pIndirectDeviceAddresses,
   const uint32_t*                             pIndirectStrides,
      {
         }
      void
   genX(CmdCopyAccelerationStructureKHR)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(vk_acceleration_structure, src_accel, pInfo->src);
            assert(pInfo->mode == VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR ||
            if (pInfo->mode == VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR) {
      uint64_t src_size_addr =
      vk_acceleration_structure_get_va(src_accel) +
      genX(grl_copy_clone_indirect)(
      cmd_buffer,
   vk_acceleration_structure_get_va(dst_accel),
   vk_acceleration_structure_get_va(src_accel),
   } else {
      genX(grl_copy_compact)(
      cmd_buffer,
   vk_acceleration_structure_get_va(dst_accel),
            anv_add_pending_pipe_bits(cmd_buffer,
            }
      void
   genX(CmdCopyAccelerationStructureToMemoryKHR)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(vk_acceleration_structure, src_accel, pInfo->src);
   struct anv_device *device = cmd_buffer->device;
   uint64_t src_size_addr =
      vk_acceleration_structure_get_va(src_accel) +
                  genX(grl_copy_serialize_indirect)(
      cmd_buffer,
   pInfo->dst.deviceAddress,
   vk_acceleration_structure_get_va(src_accel),
   anv_address_physical(device->rt_uuid_addr),
         anv_add_pending_pipe_bits(cmd_buffer,
            }
      void
   genX(CmdCopyMemoryToAccelerationStructureKHR)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
                     uint64_t src_size_addr = pInfo->src.deviceAddress +
         genX(grl_copy_deserialize_indirect)(
      cmd_buffer,
   vk_acceleration_structure_get_va(dst_accel),
   pInfo->src.deviceAddress,
         anv_add_pending_pipe_bits(cmd_buffer,
            }
      /* TODO: Host commands */
      VkResult
   genX(BuildAccelerationStructuresKHR)(
      VkDevice                                    _device,
   VkDeferredOperationKHR                      deferredOperation,
   uint32_t                                    infoCount,
   const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
      }
      VkResult
   genX(CopyAccelerationStructureKHR)(
      VkDevice                                    _device,
   VkDeferredOperationKHR                      deferredOperation,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
      }
      VkResult
   genX(CopyAccelerationStructureToMemoryKHR)(
      VkDevice                                    _device,
   VkDeferredOperationKHR                      deferredOperation,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
      }
      VkResult
   genX(CopyMemoryToAccelerationStructureKHR)(
      VkDevice                                    _device,
   VkDeferredOperationKHR                      deferredOperation,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
      }
      VkResult
   genX(WriteAccelerationStructuresPropertiesKHR)(
      VkDevice                                    _device,
   uint32_t                                    accelerationStructureCount,
   const VkAccelerationStructureKHR*           pAccelerationStructures,
   VkQueryType                                 queryType,
   size_t                                      dataSize,
   void*                                       pData,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
      }
      #endif /* GFX_VERx10 >= 125 */
