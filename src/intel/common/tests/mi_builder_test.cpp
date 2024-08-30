   /*
   * Copyright © 2019 Intel Corporation
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
      #include <fcntl.h>
   #include <string.h>
   #include <xf86drm.h>
   #include <sys/mman.h>
      #include <gtest/gtest.h>
      #include "c99_compat.h"
   #include "common/intel_gem.h"
   #include "dev/intel_device_info.h"
   #include "intel_gem.h"
   #include "isl/isl.h"
   #include "drm-uapi/i915_drm.h"
   #include "genxml/gen_macros.h"
   #include "util/macros.h"
      class mi_builder_test;
      struct address {
      uint32_t gem_handle;
      };
      #define __gen_address_type struct address
   #define __gen_user_data ::mi_builder_test
      uint64_t __gen_combine_address(mi_builder_test *test, void *location,
         void * __gen_get_batch_dwords(mi_builder_test *test, unsigned num_dwords);
   struct address __gen_get_batch_address(mi_builder_test *test,
            struct address
   __gen_address_offset(address addr, uint64_t offset)
   {
      addr.offset += offset;
      }
      #if GFX_VERx10 >= 75
   #define RSVD_TEMP_REG 0x2678 /* MI_ALU_REG15 */
   #else
   #define RSVD_TEMP_REG 0x2430 /* GFX7_3DPRIM_START_VERTEX */
   #endif
   #define MI_BUILDER_NUM_ALLOC_GPRS 15
   #define INPUT_DATA_OFFSET 0
   #define OUTPUT_DATA_OFFSET 2048
      #define __genxml_cmd_length(cmd) cmd ## _length
   #define __genxml_cmd_length_bias(cmd) cmd ## _length_bias
   #define __genxml_cmd_header(cmd) cmd ## _header
   #define __genxml_cmd_pack(cmd) cmd ## _pack
      #include "genxml/genX_pack.h"
   #include "mi_builder.h"
      #define emit_cmd(cmd, name)                                           \
      for (struct cmd name = { __genxml_cmd_header(cmd) },               \
      *_dst = (struct cmd *) emit_dwords(__genxml_cmd_length(cmd)); \
   __builtin_expect(_dst != NULL, 1);                            \
      #include <vector>
      class mi_builder_test : public ::testing::Test {
   public:
      mi_builder_test();
                     void *emit_dwords(int num_dwords);
            inline address in_addr(uint32_t offset)
   {
      address addr;
   addr.gem_handle = data_bo_handle;
   addr.offset = INPUT_DATA_OFFSET + offset;
               inline address out_addr(uint32_t offset)
   {
      address addr;
   addr.gem_handle = data_bo_handle;
   addr.offset = OUTPUT_DATA_OFFSET + offset;
               inline mi_value in_mem64(uint32_t offset)
   {
                  inline mi_value in_mem32(uint32_t offset)
   {
                  inline mi_value out_mem64(uint32_t offset)
   {
                  inline mi_value out_mem32(uint32_t offset)
   {
                  int fd;
   uint32_t ctx_id;
               #if GFX_VER >= 8
         #endif
      uint32_t batch_offset;
         #if GFX_VER < 8
         #endif
            #if GFX_VER >= 8
         #endif
      void *data_map;
   char *input;
   char *output;
               };
      mi_builder_test::mi_builder_test() :
   fd(-1)
   { }
      mi_builder_test::~mi_builder_test()
   {
         }
      // 1 MB of batch should be enough for anyone, right?
   #define BATCH_BO_SIZE (256 * 4096)
   #define DATA_BO_SIZE 4096
      void
   mi_builder_test::SetUp()
   {
      drmDevicePtr devices[8];
            int i;
   for (i = 0; i < max_devices; i++) {
      if (devices[i]->available_nodes & 1 << DRM_NODE_RENDER &&
      devices[i]->bustype == DRM_BUS_PCI &&
   devices[i]->deviceinfo.pci->vendor_id == 0x8086) {
   fd = open(devices[i]->nodes[DRM_NODE_RENDER], O_RDWR | O_CLOEXEC);
                  /* We don't really need to do this when running on hardware because
   * we can just pull it from the drmDevice.  However, without doing
   * this, intel_dump_gpu gets a bit of heartburn and we can't use the
   * --device option with it.
   */
   int device_id;
                  ASSERT_TRUE(intel_get_device_info_from_fd(fd, &devinfo));
   if (devinfo.ver != GFX_VER ||
      (devinfo.platform == INTEL_PLATFORM_HSW) != (GFX_VERx10 == 75)) {
   close(fd);
   fd = -1;
                  /* Found a device! */
         }
                     if (GFX_VER >= 8) {
      /* On gfx8+, we require softpin */
   int has_softpin;
   ASSERT_TRUE(intel_gem_get_param(fd, I915_PARAM_HAS_EXEC_SOFTPIN, &has_softpin))
                     // Create the batch buffer
   drm_i915_gem_create gem_create = drm_i915_gem_create();
   gem_create.size = BATCH_BO_SIZE;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE,
            #if GFX_VER >= 8
         #endif
         if (devinfo.has_caching_uapi) {
      drm_i915_gem_caching gem_caching = drm_i915_gem_caching();
   gem_caching.handle = batch_bo_handle;
   gem_caching.caching = I915_CACHING_CACHED;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_SET_CACHING,
               if (devinfo.has_mmap_offset) {
      drm_i915_gem_mmap_offset gem_mmap_offset = drm_i915_gem_mmap_offset();
   gem_mmap_offset.handle = batch_bo_handle;
   gem_mmap_offset.flags = devinfo.has_local_mem ?
               ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET,
            batch_map = mmap(NULL, BATCH_BO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
            } else {
      drm_i915_gem_mmap gem_mmap = drm_i915_gem_mmap();
   gem_mmap.handle = batch_bo_handle;
   gem_mmap.offset = 0;
   gem_mmap.size = BATCH_BO_SIZE;
   gem_mmap.flags = 0;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP,
                     // Start the batch at zero
            // Create the data buffer
   gem_create = drm_i915_gem_create();
   gem_create.size = DATA_BO_SIZE;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE,
            #if GFX_VER >= 8
         #endif
         if (devinfo.has_caching_uapi) {
      drm_i915_gem_caching gem_caching = drm_i915_gem_caching();
   gem_caching.handle = data_bo_handle;
   gem_caching.caching = I915_CACHING_CACHED;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_SET_CACHING,
               if (devinfo.has_mmap_offset) {
      drm_i915_gem_mmap_offset gem_mmap_offset = drm_i915_gem_mmap_offset();
   gem_mmap_offset.handle = data_bo_handle;
   gem_mmap_offset.flags = devinfo.has_local_mem ?
               ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET,
            data_map = mmap(NULL, DATA_BO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
            } else {
      drm_i915_gem_mmap gem_mmap = drm_i915_gem_mmap();
   gem_mmap.handle = data_bo_handle;
   gem_mmap.offset = 0;
   gem_mmap.size = DATA_BO_SIZE;
   gem_mmap.flags = 0;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP,
                     input = (char *)data_map + INPUT_DATA_OFFSET;
            // Fill the test data with garbage
   memset(data_map, 139, DATA_BO_SIZE);
            struct isl_device isl_dev;
   isl_device_init(&isl_dev, &devinfo);
   mi_builder_init(&b, &devinfo, this);
   const uint32_t mocs = isl_mocs(&isl_dev, 0, false);
      }
      void *
   mi_builder_test::emit_dwords(int num_dwords)
   {
      void *ptr = (void *)((char *)batch_map + batch_offset);
   batch_offset += num_dwords * 4;
   assert(batch_offset < BATCH_BO_SIZE);
      }
      void
   mi_builder_test::submit_batch()
   {
               // Round batch up to an even number of dwords.
   if (batch_offset & 4)
            drm_i915_gem_exec_object2 objects[2];
            objects[0].handle = data_bo_handle;
   objects[0].relocation_count = 0;
      #if GFX_VER >= 8 /* On gfx8+, we pin everything */
      objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS |
                  #else
      objects[0].flags = EXEC_OBJECT_WRITE;
      #endif
            #if GFX_VER >= 8 /* On gfx8+, we don't use relocations */
      objects[1].relocation_count = 0;
   objects[1].relocs_ptr = 0;
   objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS |
            #else
      objects[1].relocation_count = relocs.size();
   objects[1].relocs_ptr = (uintptr_t)(void *)&relocs[0];
   objects[1].flags = 0;
      #endif
         drm_i915_gem_execbuffer2 execbuf = drm_i915_gem_execbuffer2();
   execbuf.buffers_ptr = (uintptr_t)(void *)objects;
   execbuf.buffer_count = 2;
   execbuf.batch_start_offset = 0;
   execbuf.batch_len = batch_offset;
   execbuf.flags = I915_EXEC_HANDLE_LUT | I915_EXEC_RENDER;
            ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2,
            drm_i915_gem_wait gem_wait = drm_i915_gem_wait();
   gem_wait.bo_handle = batch_bo_handle;
   gem_wait.timeout_ns = INT64_MAX;
   ASSERT_EQ(drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT,
      }
      uint64_t
   __gen_combine_address(mi_builder_test *test, void *location,
         {
   #if GFX_VER >= 8
      uint64_t addr_u64 = addr.gem_handle == test->data_bo_handle ?
            #else
      drm_i915_gem_relocation_entry reloc = drm_i915_gem_relocation_entry();
   reloc.target_handle = addr.gem_handle == test->data_bo_handle ? 0 : 1;
   reloc.delta = addr.offset + delta;
   reloc.offset = (char *)location - (char *)test->batch_map;
   reloc.presumed_offset = -1;
               #endif
   }
      void *
   __gen_get_batch_dwords(mi_builder_test *test, unsigned num_dwords)
   {
         }
      struct address
   __gen_get_batch_address(mi_builder_test *test, void *location)
   {
      assert(location >= test->batch_map);
   size_t offset = (char *)location - (char *)test->batch_map;
   assert(offset < BATCH_BO_SIZE);
            return (struct address) {
      .gem_handle = test->batch_bo_handle,
         }
      #include "genxml/genX_pack.h"
   #include "mi_builder.h"
      TEST_F(mi_builder_test, imm_mem)
   {
               mi_store(&b, out_mem64(0), mi_imm(value));
                     // 64 -> 64
            // 64 -> 32
   EXPECT_EQ(*(uint32_t *)(output + 8),  (uint32_t)value);
      }
      /* mem -> mem copies are only supported on HSW+ */
   #if GFX_VERx10 >= 75
   TEST_F(mi_builder_test, mem_mem)
   {
      const uint64_t value = 0x0123456789abcdef;
            mi_store(&b, out_mem64(0),   in_mem64(0));
   mi_store(&b, out_mem32(8),   in_mem64(0));
   mi_store(&b, out_mem32(16),  in_mem32(0));
                     // 64 -> 64
            // 64 -> 32
   EXPECT_EQ(*(uint32_t *)(output + 8),  (uint32_t)value);
            // 32 -> 32
   EXPECT_EQ(*(uint32_t *)(output + 16), (uint32_t)value);
            // 32 -> 64
      }
   #endif
      TEST_F(mi_builder_test, imm_reg)
   {
               mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(canary));
   mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(value));
            mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(canary));
   mi_store(&b, mi_reg32(RSVD_TEMP_REG), mi_imm(value));
                     // 64 -> 64
            // 64 -> 32
   EXPECT_EQ(*(uint32_t *)(output + 8),  (uint32_t)value);
      }
      TEST_F(mi_builder_test, mem_reg)
   {
      const uint64_t value = 0x0123456789abcdef;
            mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(canary));
   mi_store(&b, mi_reg64(RSVD_TEMP_REG), in_mem64(0));
            mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(canary));
   mi_store(&b, mi_reg32(RSVD_TEMP_REG), in_mem64(0));
            mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(canary));
   mi_store(&b, mi_reg32(RSVD_TEMP_REG), in_mem32(0));
            mi_store(&b, mi_reg64(RSVD_TEMP_REG), mi_imm(canary));
   mi_store(&b, mi_reg64(RSVD_TEMP_REG), in_mem32(0));
                     // 64 -> 64
            // 64 -> 32
   EXPECT_EQ(*(uint32_t *)(output + 8),  (uint32_t)value);
            // 32 -> 32
   EXPECT_EQ(*(uint32_t *)(output + 16), (uint32_t)value);
            // 32 -> 64
      }
      TEST_F(mi_builder_test, memset)
   {
                                 uint32_t *out_u32 = (uint32_t *)output;
   for (unsigned i = 0; i <  memset_size / sizeof(*out_u32); i++)
      }
      TEST_F(mi_builder_test, memcpy)
   {
               uint8_t *in_u8 = (uint8_t *)input;
   for (unsigned i = 0; i < memcpy_size; i++)
                              uint8_t *out_u8 = (uint8_t *)output;
   for (unsigned i = 0; i < memcpy_size; i++)
      }
      /* Start of MI_MATH section */
   #if GFX_VERx10 >= 75
      #define EXPECT_EQ_IMM(x, imm) EXPECT_EQ(x, mi_value_to_u64(imm))
      TEST_F(mi_builder_test, inot)
   {
      const uint64_t value = 0x0123456789abcdef;
   const uint32_t value_lo = (uint32_t)value;
   const uint32_t value_hi = (uint32_t)(value >> 32);
            mi_store(&b, out_mem64(0),  mi_inot(&b, in_mem64(0)));
   mi_store(&b, out_mem64(8),  mi_inot(&b, mi_inot(&b, in_mem64(0))));
   mi_store(&b, out_mem64(16), mi_inot(&b, in_mem32(0)));
   mi_store(&b, out_mem64(24), mi_inot(&b, in_mem32(4)));
   mi_store(&b, out_mem32(32), mi_inot(&b, in_mem64(0)));
   mi_store(&b, out_mem32(36), mi_inot(&b, in_mem32(0)));
   mi_store(&b, out_mem32(40), mi_inot(&b, mi_inot(&b, in_mem32(0))));
                     EXPECT_EQ(*(uint64_t *)(output + 0),  ~value);
   EXPECT_EQ(*(uint64_t *)(output + 8),  value);
   EXPECT_EQ(*(uint64_t *)(output + 16), ~(uint64_t)value_lo);
   EXPECT_EQ(*(uint64_t *)(output + 24), ~(uint64_t)value_hi);
   EXPECT_EQ(*(uint32_t *)(output + 32), (uint32_t)~value);
   EXPECT_EQ(*(uint32_t *)(output + 36), (uint32_t)~value_lo);
   EXPECT_EQ(*(uint32_t *)(output + 40), (uint32_t)value_lo);
      }
      /* Test adding of immediates of all kinds including
   *
   *  - All zeroes
   *  - All ones
   *  - inverted constants
   */
   TEST_F(mi_builder_test, add_imm)
   {
      const uint64_t value = 0x0123456789abcdef;
   const uint64_t add = 0xdeadbeefac0ffee2;
            mi_store(&b, out_mem64(0),
         mi_store(&b, out_mem64(8),
         mi_store(&b, out_mem64(16),
         mi_store(&b, out_mem64(24),
         mi_store(&b, out_mem64(32),
         mi_store(&b, out_mem64(40),
         mi_store(&b, out_mem64(48),
         mi_store(&b, out_mem64(56),
         mi_store(&b, out_mem64(64),
         mi_store(&b, out_mem64(72),
         mi_store(&b, out_mem64(80),
         mi_store(&b, out_mem64(88),
            // And some add_imm just for good measure
   mi_store(&b, out_mem64(96), mi_iadd_imm(&b, in_mem64(0), 0));
                     EXPECT_EQ(*(uint64_t *)(output + 0),   value);
   EXPECT_EQ(*(uint64_t *)(output + 8),   value - 1);
   EXPECT_EQ(*(uint64_t *)(output + 16),  value - 1);
   EXPECT_EQ(*(uint64_t *)(output + 24),  value);
   EXPECT_EQ(*(uint64_t *)(output + 32),  value + add);
   EXPECT_EQ(*(uint64_t *)(output + 40),  value + ~add);
   EXPECT_EQ(*(uint64_t *)(output + 48),  value);
   EXPECT_EQ(*(uint64_t *)(output + 56),  value - 1);
   EXPECT_EQ(*(uint64_t *)(output + 64),  value - 1);
   EXPECT_EQ(*(uint64_t *)(output + 72),  value);
   EXPECT_EQ(*(uint64_t *)(output + 80),  value + add);
   EXPECT_EQ(*(uint64_t *)(output + 88),  value + ~add);
   EXPECT_EQ(*(uint64_t *)(output + 96),  value);
      }
      TEST_F(mi_builder_test, ult_uge_ieq_ine)
   {
      uint64_t values[8] = {
      0x0123456789abcdef,
   0xdeadbeefac0ffee2,
   (uint64_t)-1,
   1,
   0,
   1049571,
   (uint64_t)-240058,
      };
            for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(values); j++) {
      mi_store(&b, out_mem64(i * 256 + j * 32 + 0),
         mi_store(&b, out_mem64(i * 256 + j * 32 + 8),
         mi_store(&b, out_mem64(i * 256 + j * 32 + 16),
         mi_store(&b, out_mem64(i * 256 + j * 32 + 24),
                           for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(values); j++) {
      uint64_t *out_u64 = (uint64_t *)(output + i * 256 + j * 32);
   EXPECT_EQ_IMM(out_u64[0], mi_ult(&b, mi_imm(values[i]),
         EXPECT_EQ_IMM(out_u64[1], mi_uge(&b, mi_imm(values[i]),
         EXPECT_EQ_IMM(out_u64[2], mi_ieq(&b, mi_imm(values[i]),
         EXPECT_EQ_IMM(out_u64[3], mi_ine(&b, mi_imm(values[i]),
            }
      TEST_F(mi_builder_test, z_nz)
   {
      uint64_t values[8] = {
      0,
   1,
   UINT32_MAX,
   UINT32_MAX + 1,
      };
            for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      mi_store(&b, out_mem64(i * 16 + 0), mi_nz(&b, in_mem64(i * 8)));
                        for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      uint64_t *out_u64 = (uint64_t *)(output + i * 16);
   EXPECT_EQ_IMM(out_u64[0], mi_nz(&b, mi_imm(values[i])));
         }
      TEST_F(mi_builder_test, iand)
   {
      const uint64_t values[2] = {
      0x0123456789abcdef,
      };
                              EXPECT_EQ_IMM(*(uint64_t *)output, mi_iand(&b, mi_imm(values[0]),
      }
      #if GFX_VERx10 >= 125
   TEST_F(mi_builder_test, ishl)
   {
      const uint64_t value = 0x0123456789abcdef;
            uint32_t shifts[] = { 0, 1, 2, 4, 8, 16, 32 };
            for (unsigned i = 0; i < ARRAY_SIZE(shifts); i++) {
      mi_store(&b, out_mem64(i * 8),
                        for (unsigned i = 0; i < ARRAY_SIZE(shifts); i++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8),
         }
      TEST_F(mi_builder_test, ushr)
   {
      const uint64_t value = 0x0123456789abcdef;
            uint32_t shifts[] = { 0, 1, 2, 4, 8, 16, 32 };
            for (unsigned i = 0; i < ARRAY_SIZE(shifts); i++) {
      mi_store(&b, out_mem64(i * 8),
                        for (unsigned i = 0; i < ARRAY_SIZE(shifts); i++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8),
         }
      TEST_F(mi_builder_test, ushr_imm)
   {
      const uint64_t value = 0x0123456789abcdef;
                     for (unsigned i = 0; i <= max_shift; i++)
                     for (unsigned i = 0; i <= max_shift; i++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8),
         }
      TEST_F(mi_builder_test, ishr)
   {
      const uint64_t values[] = {
      0x0123456789abcdef,
      };
            uint32_t shifts[] = { 0, 1, 2, 4, 8, 16, 32 };
            for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(shifts); j++) {
      mi_store(&b, out_mem64(i * 8 + j * 16),
                           for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(shifts); j++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8 + j * 16),
            }
      TEST_F(mi_builder_test, ishr_imm)
   {
      const uint64_t value = 0x0123456789abcdef;
                     for (unsigned i = 0; i <= max_shift; i++)
                     for (unsigned i = 0; i <= max_shift; i++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8),
         }
   #endif /* if GFX_VERx10 >= 125 */
      TEST_F(mi_builder_test, imul_imm)
   {
      uint64_t lhs[2] = {
      0x0123456789abcdef,
      };
            /* Some random 32-bit unsigned integers.  The first four have been
   * hand-chosen just to ensure some good low integers; the rest were
   * generated with a python script.
   */
   uint32_t rhs[20] = {
      1,       2,       3,       5,
   10800,   193,     64,      40,
   3796,    256,     88,      473,
   1421,    706,     175,     850,
               for (unsigned i = 0; i < ARRAY_SIZE(lhs); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(rhs); j++) {
      mi_store(&b, out_mem64(i * 160 + j * 8),
                           for (unsigned i = 0; i < ARRAY_SIZE(lhs); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(rhs); j++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 160 + j * 8),
            }
      TEST_F(mi_builder_test, ishl_imm)
   {
      const uint64_t value = 0x0123456789abcdef;
                     for (unsigned i = 0; i <= max_shift; i++)
                     for (unsigned i = 0; i <= max_shift; i++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8),
         }
      TEST_F(mi_builder_test, ushr32_imm)
   {
      const uint64_t value = 0x0123456789abcdef;
                     for (unsigned i = 0; i <= max_shift; i++)
                     for (unsigned i = 0; i <= max_shift; i++) {
      EXPECT_EQ_IMM(*(uint64_t *)(output + i * 8),
         }
      TEST_F(mi_builder_test, udiv32_imm)
   {
      /* Some random 32-bit unsigned integers.  The first four have been
   * hand-chosen just to ensure some good low integers; the rest were
   * generated with a python script.
   */
   uint32_t values[20] = {
      1,       2,       3,       5,
   10800,   193,     64,      40,
   3796,    256,     88,      473,
   1421,    706,     175,     850,
      };
            for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(values); j++) {
      mi_store(&b, out_mem32(i * 80 + j * 4),
                           for (unsigned i = 0; i < ARRAY_SIZE(values); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(values); j++) {
      EXPECT_EQ_IMM(*(uint32_t *)(output + i * 80 + j * 4),
            }
      TEST_F(mi_builder_test, store_if)
   {
      uint64_t u64 = 0xb453b411deadc0deull;
            /* Write values with the predicate enabled */
   emit_cmd(GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
               mi_store_if(&b, out_mem64(0), mi_imm(u64));
            /* Set predicate to false, write garbage that shouldn't land */
   emit_cmd(GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
               mi_store_if(&b, out_mem64(0), mi_imm(0xd0d0d0d0d0d0d0d0ull));
                     EXPECT_EQ(*(uint64_t *)(output + 0), u64);
   EXPECT_EQ(*(uint32_t *)(output + 8), u32);
      }
      #endif /* GFX_VERx10 >= 75 */
      #if GFX_VERx10 >= 125
      /*
   * Indirect load/store tests.  Only available on XE_HP+
   */
      TEST_F(mi_builder_test, load_mem64_offset)
   {
      uint64_t values[8] = {
      0x0123456789abcdef,
   0xdeadbeefac0ffee2,
   (uint64_t)-1,
   1,
   0,
   1049571,
   (uint64_t)-240058,
      };
            uint32_t offsets[8] = { 0, 40, 24, 48, 56, 8, 32, 16 };
            for (unsigned i = 0; i < ARRAY_SIZE(offsets); i++) {
      mi_store(&b, out_mem64(i * 8),
                        for (unsigned i = 0; i < ARRAY_SIZE(offsets); i++)
      }
      TEST_F(mi_builder_test, store_mem64_offset)
   {
      uint64_t values[8] = {
      0x0123456789abcdef,
   0xdeadbeefac0ffee2,
   (uint64_t)-1,
   1,
   0,
   1049571,
   (uint64_t)-240058,
      };
            uint32_t offsets[8] = { 0, 40, 24, 48, 56, 8, 32, 16 };
            for (unsigned i = 0; i < ARRAY_SIZE(offsets); i++) {
      mi_store_mem64_offset(&b, out_addr(0), in_mem32(i * 4 + 64),
                        for (unsigned i = 0; i < ARRAY_SIZE(offsets); i++)
      }
      /*
   * Control-flow tests.  Only available on XE_HP+
   */
      TEST_F(mi_builder_test, goto)
   {
                        struct mi_goto_target t = MI_GOTO_TARGET_INIT;
            /* This one should be skipped */
                                 }
      #define MI_PREDICATE_RESULT  0x2418
      TEST_F(mi_builder_test, goto_if)
   {
      const uint64_t values[] = {
      0xb453b411deadc0deull,
   0x0123456789abcdefull,
                        emit_cmd(GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
               struct mi_goto_target t = MI_GOTO_TARGET_INIT;
                     emit_cmd(GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
                        /* This one should be skipped */
                                 }
      TEST_F(mi_builder_test, loop_simple)
   {
                        mi_loop(&b) {
                                       }
      TEST_F(mi_builder_test, loop_break)
   {
      mi_loop(&b) {
                                                                  }
      TEST_F(mi_builder_test, loop_continue)
   {
               mi_store(&b, out_mem64(0), mi_imm(0));
            mi_loop(&b) {
               mi_store(&b, out_mem64(0), mi_iadd_imm(&b, out_mem64(0), 1));
                                          EXPECT_EQ(*(uint64_t *)(output + 0), loop_count);
      }
      TEST_F(mi_builder_test, loop_continue_if)
   {
               mi_store(&b, out_mem64(0), mi_imm(0));
            mi_loop(&b) {
               mi_store(&b, out_mem64(0), mi_iadd_imm(&b, out_mem64(0), 1));
            emit_cmd(GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
                                 emit_cmd(GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
                                             EXPECT_EQ(*(uint64_t *)(output + 0), loop_count);
      }
   #endif /* GFX_VERx10 >= 125 */
