   /*
   * Copyright © 2015 Intel Corporation
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
      #include "aub_write.h"
      #include <inttypes.h>
   #include <signal.h>
   #include <stdarg.h>
   #include <stdlib.h>
   #include <string.h>
      #include "intel_aub.h"
   #include "intel_context.h"
      #include "util/u_math.h"
      #ifndef ALIGN
   #define ALIGN(x, y) (((x) + (y)-1) & ~((y)-1))
   #endif
      #define MI_BATCH_NON_SECURE_I965 (1 << 8)
      #define min(a, b) ({                            \
            __typeof(a) _a = (a);                  \
   __typeof(b) _b = (b);                  \
         #define max(a, b) ({                            \
            __typeof(a) _a = (a);                  \
   __typeof(b) _b = (b);                  \
         static struct aub_context *aub_context_new(struct aub_file *aub, uint32_t new_id);
   static void mem_trace_memory_write_header_out(struct aub_file *aub, uint64_t addr,
                  #define fail_if(cond, ...) _fail_if(cond, NULL, __VA_ARGS__)
      static void
   aub_ppgtt_table_finish(struct aub_ppgtt_table *table, int level)
   {
      if (level == 1)
            for (unsigned i = 0; i < ARRAY_SIZE(table->subtables); i++) {
      if (table->subtables[i]) {
      aub_ppgtt_table_finish(table->subtables[i], level - 1);
            }
      static void
   data_out(struct aub_file *aub, const void *data, size_t size)
   {
      if (size == 0)
            fail_if(fwrite(data, 1, size, aub->file) == 0,
      }
      static void
   dword_out(struct aub_file *aub, uint32_t data)
   {
         }
      static void
   write_execlists_header(struct aub_file *aub, const char *name)
   {
      char app_name[8 * 4];
            app_name_len =
      snprintf(app_name, sizeof(app_name), "PCI-ID=0x%X %s",
               dwords = 5 + app_name_len / sizeof(uint32_t);
   dword_out(aub, CMD_MEM_TRACE_VERSION | (dwords - 1));
   dword_out(aub, AUB_MEM_TRACE_VERSION_FILE_VERSION);
   dword_out(aub, aub->devinfo.simulator_id << AUB_MEM_TRACE_VERSION_DEVICE_SHIFT);
   dword_out(aub, 0);      /* version */
   dword_out(aub, 0);      /* version */
      }
      static void
   write_legacy_header(struct aub_file *aub, const char *name)
   {
      char app_name[8 * 4];
   char comment[16];
            comment_len = snprintf(comment, sizeof(comment), "PCI-ID=0x%x", aub->pci_id);
            /* Start with a (required) version packet. */
   dwords = 13 + comment_dwords;
   dword_out(aub, CMD_AUB_HEADER | (dwords - 2));
   dword_out(aub, (4 << AUB_HEADER_MAJOR_SHIFT) |
            /* Next comes a 32-byte application name. */
   strncpy(app_name, name, sizeof(app_name));
   app_name[sizeof(app_name) - 1] = 0;
            dword_out(aub, 0); /* timestamp */
   dword_out(aub, 0); /* timestamp */
   dword_out(aub, comment_len);
      }
         static void
   aub_write_header(struct aub_file *aub, const char *app_name)
   {
      if (aub_use_execlists(aub))
         else
      }
      void
   aub_file_init(struct aub_file *aub, FILE *file, FILE *debug, uint16_t pci_id, const char *app_name)
   {
               aub->verbose_log_file = debug;
   aub->file = file;
   aub->pci_id = pci_id;
   fail_if(!intel_get_device_info_from_pci_id(pci_id, &aub->devinfo),
                           aub->phys_addrs_allocator = 0;
   aub->ggtt_addrs_allocator = 0;
            mem_trace_memory_write_header_out(aub, aub->ggtt_addrs_allocator++,
                     dword_out(aub, 1);
            aub->next_context_handle = 1;
      }
      void
   aub_file_finish(struct aub_file *aub)
   {
      aub_ppgtt_table_finish(&aub->pml4, 4);
      }
      uint32_t
   aub_gtt_size(struct aub_file *aub)
   {
         }
      static void
   mem_trace_memory_write_header_out(struct aub_file *aub, uint64_t addr,
               {
               if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
                     dword_out(aub, CMD_MEM_TRACE_MEMORY_WRITE | (5 + dwords - 1));
   dword_out(aub, addr & 0xFFFFFFFF);   /* addr lo */
   dword_out(aub, addr >> 32);   /* addr hi */
   dword_out(aub, addr_space);   /* gtt */
      }
      static void
   register_write_out(struct aub_file *aub, uint32_t addr, uint32_t value)
   {
               if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
               dword_out(aub, CMD_MEM_TRACE_REGISTER_WRITE | (5 + dwords - 1));
   dword_out(aub, addr);
   dword_out(aub, AUB_MEM_TRACE_REGISTER_SIZE_DWORD |
         dword_out(aub, 0xFFFFFFFF);   /* mask lo */
   dword_out(aub, 0x00000000);   /* mask hi */
      }
      static void
   populate_ppgtt_table(struct aub_file *aub, struct aub_ppgtt_table *table,
         {
      uint64_t entries[512] = {0};
            if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
                     for (int i = start; i <= end; i++) {
      if (!table->subtables[i]) {
      dirty_start = min(dirty_start, i);
   dirty_end = max(dirty_end, i);
   if (level == 1) {
      table->subtables[i] =
         if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
               } else {
      table->subtables[i] =
         table->subtables[i]->phys_addr =
         if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
                  }
   entries[i] = 3 /* read/write | present */ |
      (level == 1 ? (uint64_t)(uintptr_t)table->subtables[i] :
            if (dirty_start <= dirty_end) {
      uint64_t write_addr = table->phys_addr + dirty_start *
         uint64_t write_size = (dirty_end - dirty_start + 1) *
         mem_trace_memory_write_header_out(aub, write_addr, write_size,
                     }
      void
   aub_map_ppgtt(struct aub_file *aub, uint64_t start, uint64_t size)
   {
      uint64_t l4_start = start & 0xff8000000000;
         #define L4_index(addr) (((addr) >> 39) & 0x1ff)
   #define L3_index(addr) (((addr) >> 30) & 0x1ff)
   #define L2_index(addr) (((addr) >> 21) & 0x1ff)
   #define L1_index(addr) (((addr) >> 12) & 0x1ff)
      #define L3_table(addr) (aub->pml4.subtables[L4_index(addr)])
   #define L2_table(addr) (L3_table(addr)->subtables[L3_index(addr)])
   #define L1_table(addr) (L2_table(addr)->subtables[L2_index(addr)])
         if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
                              for (uint64_t l4 = l4_start; l4 < l4_end; l4 += (1ULL << 39)) {
      uint64_t l3_start = max(l4, start & 0xffffc0000000);
   uint64_t l3_end = min(l4 + (1ULL << 39) - 1,
         uint64_t l3_start_idx = L3_index(l3_start);
                     for (uint64_t l3 = l3_start; l3 < l3_end; l3 += (1ULL << 30)) {
      uint64_t l2_start = max(l3, start & 0xffffffe00000);
   uint64_t l2_end = min(l3 + (1ULL << 30) - 1,
                                 for (uint64_t l2 = l2_start; l2 < l2_end; l2 += (1ULL << 21)) {
      uint64_t l1_start = max(l2, start & 0xfffffffff000);
   uint64_t l1_end = min(l2 + (1ULL << 21) - 1,
                                    }
      static uint64_t
   ppgtt_lookup(struct aub_file *aub, uint64_t ppgtt_addr)
   {
         }
      static const struct engine {
      const char *name;
   enum intel_engine_class engine_class;
   uint32_t hw_class;
   uint32_t elsp_reg;
   uint32_t elsq_reg;
   uint32_t status_reg;
      } engines[] = {
      [INTEL_ENGINE_CLASS_RENDER] = {
      .name = "RENDER",
   .engine_class = INTEL_ENGINE_CLASS_RENDER,
   .hw_class = 1,
   .elsp_reg = RCSUNIT(EXECLIST_SUBMITPORT),
   .elsq_reg = RCSUNIT(EXECLIST_SQ_CONTENTS),
   .status_reg = RCSUNIT(EXECLIST_STATUS),
      },
   [INTEL_ENGINE_CLASS_VIDEO] = {
      .name = "VIDEO",
   .engine_class = INTEL_ENGINE_CLASS_VIDEO,
   .hw_class = 3,
   .elsp_reg = VCSUNIT0(EXECLIST_SUBMITPORT),
   .elsq_reg = VCSUNIT0(EXECLIST_SQ_CONTENTS),
   .status_reg = VCSUNIT0(EXECLIST_STATUS),
      },
   [INTEL_ENGINE_CLASS_COPY] = {
      .name = "BLITTER",
   .engine_class = INTEL_ENGINE_CLASS_COPY,
   .hw_class = 2,
   .elsp_reg = BCSUNIT0(EXECLIST_SUBMITPORT),
   .elsq_reg = BCSUNIT0(EXECLIST_SQ_CONTENTS),
   .status_reg = BCSUNIT0(EXECLIST_STATUS),
         };
      static void
   aub_map_ggtt(struct aub_file *aub, uint64_t virt_addr, uint64_t size)
   {
      /* Makes the code below a bit simpler. In practice all of the write we
   * receive from error2aub are page aligned.
   */
   assert(virt_addr % 4096 == 0);
            /* GGTT PT */
   uint32_t ggtt_ptes = DIV_ROUND_UP(size, 4096);
   uint64_t phys_addr = aub->phys_addrs_allocator << 12;
            if (aub->verbose_log_file) {
      fprintf(aub->verbose_log_file,
                     mem_trace_memory_write_header_out(aub,
                           for (uint32_t i = 0; i < ggtt_ptes; i++) {
      dword_out(aub, 1 + phys_addr + i * 4096);
         }
      void
   aub_write_ggtt(struct aub_file *aub, uint64_t virt_addr, uint64_t size, const void *data)
   {
      /* Default setup assumes a 1 to 1 mapping between physical and virtual GGTT
   * addresses. This is somewhat incompatible with the aub_write_ggtt()
   * function. In practice it doesn't matter as the GGTT writes are used to
   * replace the default setup and we've taken care to setup the PML4 as the
   * top of the GGTT.
   */
                     /* We write the GGTT buffer through the GGTT aub command rather than the
   * PHYSICAL aub command. This is because the Gfx9 simulator seems to have 2
   * different set of memory pools for GGTT and physical (probably someone
   * didn't really understand the concept?).
   */
   static const char null_block[8 * 4096];
   for (uint64_t offset = 0; offset < size; offset += 4096) {
               mem_trace_memory_write_header_out(aub, virt_addr + offset, block_size,
                        /* Pad to a multiple of 4 bytes. */
         }
      static const struct engine *
   engine_from_engine_class(enum intel_engine_class engine_class)
   {
      switch (engine_class) {
   case INTEL_ENGINE_CLASS_RENDER:
   case INTEL_ENGINE_CLASS_COPY:
   case INTEL_ENGINE_CLASS_VIDEO:
         default:
            }
      static void
   get_context_init(const struct intel_device_info *devinfo,
                  const struct intel_context_parameters *params,
      {
      static const intel_context_init_t gfx8_contexts[] = {
      [INTEL_ENGINE_CLASS_RENDER] = gfx8_render_context_init,
   [INTEL_ENGINE_CLASS_COPY] = gfx8_blitter_context_init,
      };
   static const intel_context_init_t gfx10_contexts[] = {
      [INTEL_ENGINE_CLASS_RENDER] = gfx10_render_context_init,
   [INTEL_ENGINE_CLASS_COPY] = gfx10_blitter_context_init,
                        if (devinfo->ver <= 10)
         else
      }
      static uint64_t
   alloc_ggtt_address(struct aub_file *aub, uint64_t size)
   {
      uint32_t ggtt_ptes = DIV_ROUND_UP(size, 4096);
            aub->ggtt_addrs_allocator += ggtt_ptes;
               }
      static void
   write_hwsp(struct aub_file *aub,
         {
      uint32_t reg = 0;
   switch (engine_class) {
   case INTEL_ENGINE_CLASS_RENDER:  reg = RCSUNIT (HWS_PGA); break;
   case INTEL_ENGINE_CLASS_COPY:    reg = BCSUNIT0(HWS_PGA); break;
   case INTEL_ENGINE_CLASS_VIDEO:   reg = VCSUNIT0(HWS_PGA); break;
   default:
                     }
      static uint32_t
   write_engine_execlist_setup(struct aub_file *aub,
                     {
      const struct engine *cs = engine_from_engine_class(engine_class);
                     /* GGTT PT */
   uint32_t total_size = RING_SIZE + PPHWSP_SIZE + context_size;
   char name[80];
                     /* RING */
   hw_ctx->ring_addr = ggtt_addr;
   snprintf(name, sizeof(name), "%s RING", cs->name);
   mem_trace_memory_write_header_out(aub, ggtt_addr, RING_SIZE,
               for (uint32_t i = 0; i < RING_SIZE; i += sizeof(uint32_t))
                  /* PPHWSP */
   hw_ctx->pphwsp_addr = ggtt_addr;
   snprintf(name, sizeof(name), "%s PPHWSP", cs->name);
   mem_trace_memory_write_header_out(aub, ggtt_addr,
                     for (uint32_t i = 0; i < PPHWSP_SIZE; i += sizeof(uint32_t))
            /* CONTEXT */
   struct intel_context_parameters params = {
      .ring_addr = hw_ctx->ring_addr,
   .ring_size = RING_SIZE,
      };
   uint32_t *context_data = calloc(1, context_size);
   get_context_init(&aub->devinfo, &params, engine_class, context_data, &context_size);
   data_out(aub, context_data, context_size);
                        }
      static void
   write_execlists_default_setup(struct aub_file *aub)
   {
      register_write_out(aub, RCSUNIT(GFX_MODE), 0x80008000 /* execlist enable */);
   register_write_out(aub, VCSUNIT0(GFX_MODE), 0x80008000 /* execlist enable */);
      }
      static void write_legacy_default_setup(struct aub_file *aub)
   {
               /* Set up the GTT. The max we can handle is 64M */
   dword_out(aub, CMD_AUB_TRACE_HEADER_BLOCK |
         dword_out(aub, AUB_TRACE_MEMTYPE_GTT_ENTRY |
         dword_out(aub, 0); /* subtype */
   dword_out(aub, 0); /* offset */
   dword_out(aub, aub_gtt_size(aub)); /* size */
   if (aub->addr_bits > 32)
         for (uint32_t i = 0; i < NUM_PT_ENTRIES; i++) {
      dword_out(aub, entry + 0x1000 * i);
   if (aub->addr_bits > 32)
         }
      /**
   * Sets up a default GGTT/PPGTT address space and execlists context (when
   * supported).
   */
   void
   aub_write_default_setup(struct aub_file *aub)
   {
      if (aub_use_execlists(aub))
         else
               }
      static struct aub_context *
   aub_context_new(struct aub_file *aub, uint32_t new_id)
   {
               struct aub_context *ctx = &aub->contexts[aub->num_contexts++];
   memset(ctx, 0, sizeof(*ctx));
               }
      uint32_t
   aub_write_context_create(struct aub_file *aub, uint32_t *ctx_id)
   {
                        if (!ctx_id)
               }
      static struct aub_context *
   aub_context_find(struct aub_file *aub, uint32_t id)
   {
      for (int i = 0; i < aub->num_contexts; i++) {
      if (aub->contexts[i].id == id)
                  }
      static struct aub_hw_context *
   aub_write_ensure_context(struct aub_file *aub, uint32_t ctx_id,
         {
      struct aub_context *ctx = aub_context_find(aub, ctx_id);
            struct aub_hw_context *hw_ctx = &ctx->hw_contexts[engine_class];
   if (!hw_ctx->initialized)
               }
      static uint64_t
   get_context_descriptor(struct aub_file *aub,
               {
         }
      /**
   * Break up large objects into multiple writes.  Otherwise a 128kb VBO
   * would overflow the 16 bits of size field in the packet header and
   * everything goes badly after that.
   */
   void
   aub_write_trace_block(struct aub_file *aub,
               {
      uint32_t block_size;
   uint32_t subtype = 0;
            for (uint32_t offset = 0; offset < size; offset += block_size) {
               if (aub_use_execlists(aub)) {
      block_size = min(4096, block_size);
   mem_trace_memory_write_header_out(aub,
                        } else {
      dword_out(aub, CMD_AUB_TRACE_HEADER_BLOCK |
         dword_out(aub, AUB_TRACE_MEMTYPE_GTT |
         dword_out(aub, subtype);
   dword_out(aub, gtt_offset + offset);
   dword_out(aub, align(block_size, 4));
   if (aub->addr_bits > 32)
               if (virtual)
         else
            /* Pad to a multiple of 4 bytes. */
         }
      static void
   aub_dump_ring_buffer_execlist(struct aub_file *aub,
                     {
      mem_trace_memory_write_header_out(aub, hw_ctx->ring_addr, 16,
               dword_out(aub, AUB_MI_BATCH_BUFFER_START | MI_BATCH_NON_SECURE_I965 | (3 - 2));
   dword_out(aub, batch_offset & 0xFFFFFFFF);
   dword_out(aub, batch_offset >> 32);
            mem_trace_memory_write_header_out(aub, hw_ctx->ring_addr + 8192 + 20, 4,
               dword_out(aub, 0); /* RING_BUFFER_HEAD */
   mem_trace_memory_write_header_out(aub, hw_ctx->ring_addr + 8192 + 28, 4,
                  }
      static void
   aub_dump_execlist(struct aub_file *aub, const struct engine *cs, uint64_t descriptor)
   {
      if (aub->devinfo.ver >= 11) {
      register_write_out(aub, cs->elsq_reg, descriptor & 0xFFFFFFFF);
   register_write_out(aub, cs->elsq_reg + sizeof(uint32_t), descriptor >> 32);
      } else {
      register_write_out(aub, cs->elsp_reg, 0);
   register_write_out(aub, cs->elsp_reg, 0);
   register_write_out(aub, cs->elsp_reg, descriptor >> 32);
               dword_out(aub, CMD_MEM_TRACE_REGISTER_POLL | (5 + 1 - 1));
   dword_out(aub, cs->status_reg);
   dword_out(aub, AUB_MEM_TRACE_REGISTER_SIZE_DWORD |
         if (aub->devinfo.ver >= 11) {
      dword_out(aub, 0x00000001);   /* mask lo */
   dword_out(aub, 0x00000000);   /* mask hi */
      } else {
      dword_out(aub, 0x00000010);   /* mask lo */
   dword_out(aub, 0x00000000);   /* mask hi */
         }
      static void
   aub_dump_ring_buffer_legacy(struct aub_file *aub,
                     {
      uint32_t ringbuffer[4096];
   unsigned aub_mi_bbs_len;
   int ring_count = 0;
   static const int engine_class_to_ring[] = {
      [INTEL_ENGINE_CLASS_RENDER] = AUB_TRACE_TYPE_RING_PRB0,
   [INTEL_ENGINE_CLASS_VIDEO]  = AUB_TRACE_TYPE_RING_PRB1,
      };
            /* Make a ring buffer to execute our batchbuffer. */
            aub_mi_bbs_len = aub->addr_bits > 32 ? 3 : 2;
   ringbuffer[ring_count] = AUB_MI_BATCH_BUFFER_START | (aub_mi_bbs_len - 2);
   aub_write_reloc(&aub->devinfo, &ringbuffer[ring_count + 1], batch_offset);
            /* Write out the ring.  This appears to trigger execution of
   * the ring in the simulator.
   */
   dword_out(aub, CMD_AUB_TRACE_HEADER_BLOCK |
         dword_out(aub, AUB_TRACE_MEMTYPE_GTT | ring | AUB_TRACE_OP_COMMAND_WRITE);
   dword_out(aub, 0); /* general/surface subtype */
   dword_out(aub, offset);
   dword_out(aub, ring_count * 4);
   if (aub->addr_bits > 32)
               }
      static void
   aub_write_ensure_hwsp(struct aub_file *aub,
         {
               if (*hwsp_addr != 0)
            *hwsp_addr = alloc_ggtt_address(aub, 4096);
      }
      void
   aub_write_exec(struct aub_file *aub, uint32_t ctx_id, uint64_t batch_addr,
         {
               if (aub_use_execlists(aub)) {
      struct aub_hw_context *hw_ctx =
         uint64_t descriptor = get_context_descriptor(aub, cs, hw_ctx);
   aub_write_ensure_hwsp(aub, engine_class);
   aub_dump_ring_buffer_execlist(aub, hw_ctx, cs, batch_addr);
      } else {
      /* Dump ring buffer */
      }
      }
      void
   aub_write_context_execlists(struct aub_file *aub, uint64_t context_addr,
         {
      const struct engine *cs = engine_from_engine_class(engine_class);
   uint64_t descriptor = ((uint64_t)1 << 62 | context_addr  | CONTEXT_FLAGS);
      }
