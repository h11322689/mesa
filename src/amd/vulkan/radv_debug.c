   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based in part on anv driver which is:
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
      #include <stdio.h>
   #include <stdlib.h>
   #ifndef _WIN32
   #include <sys/utsname.h>
   #endif
   #include <sys/stat.h>
      #include "util/mesa-sha1.h"
   #include "util/os_time.h"
   #include "ac_debug.h"
   #include "radv_debug.h"
   #include "radv_shader.h"
   #include "sid.h"
      #define TRACE_BO_SIZE 4096
   #define TMA_BO_SIZE   4096
      #define COLOR_RESET  "\033[0m"
   #define COLOR_RED    "\033[31m"
   #define COLOR_GREEN  "\033[1;32m"
   #define COLOR_YELLOW "\033[1;33m"
   #define COLOR_CYAN   "\033[1;36m"
      #define RADV_DUMP_DIR "radv_dumps"
      /* Trace BO layout (offsets are 4 bytes):
   *
   * [0]: primary trace ID
   * [1]: secondary trace ID
   * [2-3]: 64-bit GFX ring pipeline pointer
   * [4-5]: 64-bit COMPUTE ring pipeline pointer
   * [6-7]: Vertex descriptors pointer
   * [8-9]: 64-bit Vertex prolog pointer
   * [10-11]: 64-bit descriptor set #0 pointer
   * ...
   * [72-73]: 64-bit descriptor set #31 pointer
   */
      bool
   radv_init_trace(struct radv_device *device)
   {
      struct radeon_winsys *ws = device->ws;
            result = ws->buffer_create(
      ws, TRACE_BO_SIZE, 8, RADEON_DOMAIN_VRAM,
   RADEON_FLAG_CPU_ACCESS | RADEON_FLAG_NO_INTERPROCESS_SHARING | RADEON_FLAG_ZERO_VRAM | RADEON_FLAG_VA_UNCACHED,
      if (result != VK_SUCCESS)
            result = ws->buffer_make_resident(ws, device->trace_bo, true);
   if (result != VK_SUCCESS)
            device->trace_id_ptr = ws->buffer_map(device->trace_bo);
   if (!device->trace_id_ptr)
               }
      void
   radv_finish_trace(struct radv_device *device)
   {
               if (unlikely(device->trace_bo)) {
      ws->buffer_make_resident(ws, device->trace_bo, false);
         }
      static void
   radv_dump_trace(const struct radv_device *device, struct radeon_cmdbuf *cs, FILE *f)
   {
      fprintf(f, "Trace ID: %x\n", *device->trace_id_ptr);
      }
      static void
   radv_dump_mmapped_reg(const struct radv_device *device, FILE *f, unsigned offset)
   {
      struct radeon_winsys *ws = device->ws;
            if (ws->read_registers(ws, offset, 1, &value))
      ac_dump_reg(f, device->physical_device->rad_info.gfx_level, device->physical_device->rad_info.family, offset,
   }
      static void
   radv_dump_debug_registers(const struct radv_device *device, FILE *f)
   {
               fprintf(f, "Memory-mapped registers:\n");
            radv_dump_mmapped_reg(device, f, R_008008_GRBM_STATUS2);
   radv_dump_mmapped_reg(device, f, R_008014_GRBM_STATUS_SE0);
   radv_dump_mmapped_reg(device, f, R_008018_GRBM_STATUS_SE1);
   radv_dump_mmapped_reg(device, f, R_008038_GRBM_STATUS_SE2);
   radv_dump_mmapped_reg(device, f, R_00803C_GRBM_STATUS_SE3);
   radv_dump_mmapped_reg(device, f, R_00D034_SDMA0_STATUS_REG);
   radv_dump_mmapped_reg(device, f, R_00D834_SDMA1_STATUS_REG);
   if (info->gfx_level <= GFX8) {
      radv_dump_mmapped_reg(device, f, R_000E50_SRBM_STATUS);
   radv_dump_mmapped_reg(device, f, R_000E4C_SRBM_STATUS2);
      }
   radv_dump_mmapped_reg(device, f, R_008680_CP_STAT);
   radv_dump_mmapped_reg(device, f, R_008674_CP_STALLED_STAT1);
   radv_dump_mmapped_reg(device, f, R_008678_CP_STALLED_STAT2);
   radv_dump_mmapped_reg(device, f, R_008670_CP_STALLED_STAT3);
   radv_dump_mmapped_reg(device, f, R_008210_CP_CPC_STATUS);
   radv_dump_mmapped_reg(device, f, R_008214_CP_CPC_BUSY_STAT);
   radv_dump_mmapped_reg(device, f, R_008218_CP_CPC_STALLED_STAT1);
   radv_dump_mmapped_reg(device, f, R_00821C_CP_CPF_STATUS);
   radv_dump_mmapped_reg(device, f, R_008220_CP_CPF_BUSY_STAT);
   radv_dump_mmapped_reg(device, f, R_008224_CP_CPF_STALLED_STAT1);
      }
      static void
   radv_dump_buffer_descriptor(enum amd_gfx_level gfx_level, enum radeon_family family, const uint32_t *desc, FILE *f)
   {
      fprintf(f, COLOR_CYAN "    Buffer:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 4; j++)
      }
      static void
   radv_dump_image_descriptor(enum amd_gfx_level gfx_level, enum radeon_family family, const uint32_t *desc, FILE *f)
   {
               fprintf(f, COLOR_CYAN "    Image:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 8; j++)
            fprintf(f, COLOR_CYAN "    FMASK:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 8; j++)
      }
      static void
   radv_dump_sampler_descriptor(enum amd_gfx_level gfx_level, enum radeon_family family, const uint32_t *desc, FILE *f)
   {
      fprintf(f, COLOR_CYAN "    Sampler state:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 4; j++) {
            }
      static void
   radv_dump_combined_image_sampler_descriptor(enum amd_gfx_level gfx_level, enum radeon_family family,
         {
      radv_dump_image_descriptor(gfx_level, family, desc, f);
      }
      static void
   radv_dump_descriptor_set(const struct radv_device *device, const struct radv_descriptor_set *set, unsigned id, FILE *f)
   {
      enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   enum radeon_family family = device->physical_device->rad_info.family;
   const struct radv_descriptor_set_layout *layout;
            if (!set)
                  for (i = 0; i < set->header.layout->binding_count; i++) {
               switch (layout->binding[i].type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      radv_dump_buffer_descriptor(gfx_level, family, desc, f);
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      radv_dump_image_descriptor(gfx_level, family, desc, f);
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      radv_dump_combined_image_sampler_descriptor(gfx_level, family, desc, f);
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      radv_dump_sampler_descriptor(gfx_level, family, desc, f);
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
   case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      /* todo */
      default:
      assert(!"unknown descriptor type");
      }
      }
      }
      static void
   radv_dump_descriptors(struct radv_device *device, FILE *f)
   {
      uint64_t *ptr = (uint64_t *)device->trace_id_ptr;
            fprintf(f, "Descriptors:\n");
   for (i = 0; i < MAX_SETS; i++) {
                     }
      struct radv_shader_inst {
      char text[160];  /* one disasm line */
   unsigned offset; /* instruction offset */
      };
      /* Split a disassembly string into lines and add them to the array pointed
   * to by "instructions". */
   static void
   si_add_split_disasm(const char *disasm, uint64_t start_addr, unsigned *num, struct radv_shader_inst *instructions)
   {
      struct radv_shader_inst *last_inst = *num ? &instructions[*num - 1] : NULL;
            while ((next = strchr(disasm, '\n'))) {
      struct radv_shader_inst *inst = &instructions[*num];
            if (!memchr(disasm, ';', len)) {
      /* Ignore everything that is not an instruction. */
   disasm = next + 1;
               assert(len < ARRAY_SIZE(inst->text));
   memcpy(inst->text, disasm, len);
   inst->text[len] = 0;
            const char *semicolon = strchr(disasm, ';');
   assert(semicolon);
   /* More than 16 chars after ";" means the instruction is 8 bytes long. */
            snprintf(inst->text + len, ARRAY_SIZE(inst->text) - len, " [PC=0x%" PRIx64 ", off=%u, size=%u]",
            last_inst = inst;
   (*num)++;
         }
      static void
   radv_dump_annotated_shader(const struct radv_shader *shader, gl_shader_stage stage, struct ac_wave_info *waves,
         {
      uint64_t start_addr, end_addr;
            if (!shader)
            start_addr = radv_shader_get_va(shader);
            /* See if any wave executes the shader. */
   for (i = 0; i < num_waves; i++) {
      if (start_addr <= waves[i].pc && waves[i].pc <= end_addr)
               if (i == num_waves)
            /* Remember the first found wave. The waves are sorted according to PC. */
   waves = &waves[i];
            /* Get the list of instructions.
   * Buffer size / 4 is the upper bound of the instruction count.
   */
   unsigned num_inst = 0;
                              /* Print instructions with annotations. */
   for (i = 0; i < num_inst; i++) {
                        /* Print which waves execute the instruction right now. */
   while (num_waves && start_addr + inst->offset == waves->pc) {
      fprintf(f,
                        if (inst->size == 4) {
         } else {
                  waves->matched = true;
   waves = &waves[1];
                  fprintf(f, "\n\n");
      }
      static void
   radv_dump_spirv(const struct radv_shader *shader, const char *sha1, const char *dump_dir)
   {
      char dump_path[512];
                     f = fopen(dump_path, "w+");
   if (f) {
      fwrite(shader->spirv, shader->spirv_size, 1, f);
         }
      static void
   radv_dump_shader(struct radv_device *device, struct radv_pipeline *pipeline, struct radv_shader *shader,
         {
      if (!shader)
                     if (shader->spirv) {
      unsigned char sha1[21];
            _mesa_sha1_compute(shader->spirv, shader->spirv_size, sha1);
            fprintf(f, "SPIRV (see %s.spv)\n\n", sha1buf);
               if (shader->nir_string) {
                  fprintf(f, "%s IR:\n%s\n", device->physical_device->use_llvm ? "LLVM" : "ACO", shader->ir_string);
               }
      static void
   radv_dump_vertex_descriptors(const struct radv_device *device, const struct radv_graphics_pipeline *pipeline, FILE *f)
   {
      struct radv_shader *vs = radv_get_shader(pipeline->base.shaders, MESA_SHADER_VERTEX);
   uint64_t *ptr = (uint64_t *)device->trace_id_ptr;
   uint32_t count = util_bitcount(vs->info.vs.vb_desc_usage_mask);
            if (!count)
            fprintf(f, "Num vertex %s: %d\n", vs->info.vs.use_per_attribute_vb_descs ? "attributes" : "bindings", count);
   for (uint32_t i = 0; i < count; i++) {
      uint32_t *desc = &((uint32_t *)vb_ptr)[i * 4];
            va |= desc[0];
            fprintf(f, "VBO#%d:\n", i);
   fprintf(f, "\tVA: 0x%" PRIx64 "\n", va);
   fprintf(f, "\tStride: %d\n", G_008F04_STRIDE(desc[1]));
         }
      static struct radv_shader_part *
   radv_get_saved_vs_prolog(const struct radv_device *device)
   {
      uint64_t *ptr = (uint64_t *)device->trace_id_ptr;
      }
      static void
   radv_dump_vs_prolog(const struct radv_device *device, const struct radv_graphics_pipeline *pipeline, FILE *f)
   {
      struct radv_shader_part *vs_prolog = radv_get_saved_vs_prolog(device);
            if (!vs_prolog || !vs_shader || !vs_shader->info.vs.has_prolog)
            fprintf(f, "Vertex prolog:\n\n");
      }
      static struct radv_pipeline *
   radv_get_saved_pipeline(struct radv_device *device, enum amd_ip_type ring)
   {
      uint64_t *ptr = (uint64_t *)device->trace_id_ptr;
               }
      static void
   radv_dump_queue_state(struct radv_queue *queue, const char *dump_dir, FILE *f)
   {
      struct radv_device *device = queue->device;
   enum amd_ip_type ring = radv_queue_ring(queue);
                     pipeline = radv_get_saved_pipeline(queue->device, ring);
   if (pipeline) {
      if (pipeline->type == RADV_PIPELINE_GRAPHICS) {
                        /* Dump active graphics shaders. */
   unsigned stages = graphics_pipeline->active_stages;
                     radv_dump_shader(device, &graphics_pipeline->base, graphics_pipeline->base.shaders[stage], stage, dump_dir,
         } else if (pipeline->type == RADV_PIPELINE_RAY_TRACING) {
      struct radv_ray_tracing_pipeline *rt_pipeline = radv_pipeline_to_ray_tracing(pipeline);
   for (unsigned i = 0; i < rt_pipeline->stage_count; i++) {
      if (radv_ray_tracing_stage_is_compiled(&rt_pipeline->stages[i])) {
      struct radv_shader *shader = container_of(rt_pipeline->stages[i].shader, struct radv_shader, base);
         }
   radv_dump_shader(device, pipeline, pipeline->shaders[MESA_SHADER_INTERSECTION], MESA_SHADER_INTERSECTION,
      } else {
               radv_dump_shader(device, &compute_pipeline->base, compute_pipeline->base.shaders[MESA_SHADER_COMPUTE],
               if (!(queue->device->instance->debug_flags & RADV_DEBUG_NO_UMR)) {
      struct ac_wave_info waves[AC_MAX_WAVES_PER_CHIP];
                                             /* Dump annotated active graphics shaders. */
   unsigned stages = graphics_pipeline->active_stages;
                           } else if (pipeline->type == RADV_PIPELINE_RAY_TRACING) {
      struct radv_ray_tracing_pipeline *rt_pipeline = radv_pipeline_to_ray_tracing(pipeline);
   for (unsigned i = 0; i < rt_pipeline->stage_count; i++) {
      if (radv_ray_tracing_stage_is_compiled(&rt_pipeline->stages[i])) {
      struct radv_shader *shader = container_of(rt_pipeline->stages[i].shader, struct radv_shader, base);
         }
   radv_dump_annotated_shader(pipeline->shaders[MESA_SHADER_INTERSECTION], MESA_SHADER_INTERSECTION, waves,
                        radv_dump_annotated_shader(compute_pipeline->base.shaders[MESA_SHADER_COMPUTE], MESA_SHADER_COMPUTE, waves,
               /* Print waves executing shaders that are not currently bound. */
   unsigned i;
   bool found = false;
   for (i = 0; i < num_waves; i++) {
                     if (!found) {
      fprintf(f, COLOR_CYAN "Waves not executing currently-bound shaders:" COLOR_RESET "\n");
      }
   fprintf(f, "    SE%u SH%u CU%u SIMD%u WAVE%u  EXEC=%016" PRIx64 "  INST=%08X %08X  PC=%" PRIx64 "\n",
            }
   if (found)
               if (pipeline->type == RADV_PIPELINE_GRAPHICS) {
      struct radv_graphics_pipeline *graphics_pipeline = radv_pipeline_to_graphics(pipeline);
      }
         }
      static void
   radv_dump_cmd(const char *cmd, FILE *f)
   {
   #ifndef _WIN32
      char line[2048];
            p = popen(cmd, "r");
   if (p) {
      while (fgets(line, sizeof(line), p))
         fprintf(f, "\n");
         #endif
   }
      static void
   radv_dump_dmesg(FILE *f)
   {
      fprintf(f, "\nLast 60 lines of dmesg:\n\n");
      }
      void
   radv_dump_enabled_options(const struct radv_device *device, FILE *f)
   {
               if (device->instance->debug_flags) {
               mask = device->instance->debug_flags;
   while (mask) {
      int i = u_bit_scan64(&mask);
      }
               if (device->instance->perftest_flags) {
               mask = device->instance->perftest_flags;
   while (mask) {
      int i = u_bit_scan64(&mask);
      }
         }
      static void
   radv_dump_app_info(const struct radv_device *device, FILE *f)
   {
               fprintf(f, "Application name: %s\n", instance->vk.app_info.app_name);
   fprintf(f, "Application version: %d\n", instance->vk.app_info.app_version);
   fprintf(f, "Engine name: %s\n", instance->vk.app_info.engine_name);
   fprintf(f, "Engine version: %d\n", instance->vk.app_info.engine_version);
   fprintf(f, "API version: %d.%d.%d\n", VK_VERSION_MAJOR(instance->vk.app_info.api_version),
               }
      static void
   radv_dump_device_name(const struct radv_device *device, FILE *f)
   {
         #ifndef _WIN32
      char kernel_version[128] = {0};
      #endif
      #ifdef _WIN32
      fprintf(f, "Device name: %s (DRM %i.%i.%i)\n\n", device->physical_device->marketing_name, info->drm_major,
      #else
      if (uname(&uname_data) == 0)
            fprintf(f, "Device name: %s (DRM %i.%i.%i%s)\n\n", device->physical_device->marketing_name, info->drm_major,
      #endif
   }
      static void
   radv_dump_umr_ring(const struct radv_queue *queue, FILE *f)
   {
      const enum amd_ip_type ring = radv_queue_ring(queue);
   const struct radv_device *device = queue->device;
            /* TODO: Dump compute ring. */
   if (ring != AMD_IP_GFX)
                     fprintf(f, "\nUMR GFX ring:\n\n");
      }
      static void
   radv_dump_umr_waves(struct radv_queue *queue, FILE *f)
   {
      enum amd_ip_type ring = radv_queue_ring(queue);
   struct radv_device *device = queue->device;
            /* TODO: Dump compute ring. */
   if (ring != AMD_IP_GFX)
            sprintf(cmd, "umr -O bits,halt_waves -go 0 -wa %s -go 1 2>&1",
            fprintf(f, "\nUMR GFX waves:\n\n");
      }
      static bool
   radv_gpu_hang_occurred(struct radv_queue *queue, enum amd_ip_type ring)
   {
               if (!ws->ctx_wait_idle(queue->hw_ctx, ring, queue->vk.index_in_family))
               }
      bool
   radv_vm_fault_occurred(struct radv_device *device, struct radv_winsys_gpuvm_fault_info *fault_info)
   {
      if (!device->physical_device->rad_info.has_gpuvm_fault_query)
               }
      void
   radv_check_gpu_hangs(struct radv_queue *queue, const struct radv_winsys_submit_info *submit_info)
   {
                        bool hang_occurred = radv_gpu_hang_occurred(queue, ring);
   if (!hang_occurred)
                  #ifndef _WIN32
      struct radv_winsys_gpuvm_fault_info fault_info = {0};
            /* Query if a VM fault happened for this GPU hang. */
            /* Create a directory into $HOME/radv_dumps_<pid>_<time> to save
   * various debugging info about that GPU hang.
   */
   struct tm *timep, result;
   time_t raw_time;
   FILE *f;
            time(&raw_time);
   timep = os_localtime(&raw_time, &result);
            snprintf(dump_dir, sizeof(dump_dir), "%s/" RADV_DUMP_DIR "_%d_%s", debug_get_option("HOME", "."), getpid(),
         if (mkdir(dump_dir, 0774) && errno != EEXIST) {
      fprintf(stderr, "radv: can't create directory '%s' (%i).\n", dump_dir, errno);
                        /* Dump trace file. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "trace.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_trace(queue->device, submit_info->cs_array[0], f);
               /* Dump pipeline state. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "pipeline.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_queue_state(queue, dump_dir, f);
               if (!(device->instance->debug_flags & RADV_DEBUG_NO_UMR)) {
      /* Dump UMR waves. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "umr_waves.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_umr_waves(queue, f);
               /* Dump UMR ring. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "umr_ring.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_umr_ring(queue, f);
                  /* Dump debug registers. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "registers.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_debug_registers(device, f);
               /* Dump BO ranges. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "bo_ranges.log");
   f = fopen(dump_path, "w+");
   if (f) {
      device->ws->dump_bo_ranges(device->ws, f);
               /* Dump BO log. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "bo_history.log");
   f = fopen(dump_path, "w+");
   if (f) {
      device->ws->dump_bo_log(device->ws, f);
               /* Dump VM fault info. */
   if (vm_fault_occurred) {
      snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "vm_fault.log");
   f = fopen(dump_path, "w+");
   if (f) {
      fprintf(f, "VM fault report.\n\n");
   fprintf(f, "Failing VM page: 0x%08" PRIx64 "\n", fault_info.addr);
                  /* Dump app info. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "app_info.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_app_info(device, f);
               /* Dump GPU info. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "gpu_info.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_device_name(device, f);
   ac_print_gpu_info(&device->physical_device->rad_info, f);
               /* Dump dmesg. */
   snprintf(dump_path, sizeof(dump_path), "%s/%s", dump_dir, "dmesg.log");
   f = fopen(dump_path, "w+");
   if (f) {
      radv_dump_dmesg(f);
         #endif
         fprintf(stderr, "radv: GPU hang report saved successfully!\n");
      }
      void
   radv_print_spirv(const char *data, uint32_t size, FILE *fp)
   {
   #ifndef _WIN32
      char path[] = "/tmp/fileXXXXXX";
   char command[128];
            /* Dump the binary into a temporary file. */
   fd = mkstemp(path);
   if (fd < 0)
            if (write(fd, data, size) == -1)
            /* Disassemble using spirv-dis if installed. */
   sprintf(command, "spirv-dis %s", path);
         fail:
      close(fd);
      #endif
   }
      bool
   radv_trap_handler_init(struct radv_device *device)
   {
      struct radeon_winsys *ws = device->ws;
            /* Create the trap handler shader and upload it like other shaders. */
   device->trap_handler_shader = radv_create_trap_handler_shader(device);
   if (!device->trap_handler_shader) {
      fprintf(stderr, "radv: failed to create the trap handler shader.\n");
               result = ws->buffer_make_resident(ws, device->trap_handler_shader->bo, true);
   if (result != VK_SUCCESS)
            result = ws->buffer_create(
      ws, TMA_BO_SIZE, 256, RADEON_DOMAIN_VRAM,
   RADEON_FLAG_CPU_ACCESS | RADEON_FLAG_NO_INTERPROCESS_SHARING | RADEON_FLAG_ZERO_VRAM | RADEON_FLAG_32BIT,
      if (result != VK_SUCCESS)
            result = ws->buffer_make_resident(ws, device->tma_bo, true);
   if (result != VK_SUCCESS)
            device->tma_ptr = ws->buffer_map(device->tma_bo);
   if (!device->tma_ptr)
            /* Upload a buffer descriptor to store various info from the trap. */
   uint64_t tma_va = radv_buffer_get_va(device->tma_bo) + 16;
            desc[0] = tma_va;
   desc[1] = S_008F04_BASE_ADDRESS_HI(tma_va >> 32);
   desc[2] = TMA_BO_SIZE;
   desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
                              }
      void
   radv_trap_handler_finish(struct radv_device *device)
   {
               if (unlikely(device->trap_handler_shader)) {
      ws->buffer_make_resident(ws, device->trap_handler_shader->bo, false);
               if (unlikely(device->tma_bo)) {
      ws->buffer_make_resident(ws, device->tma_bo, false);
         }
      static void
   radv_dump_faulty_shader(struct radv_device *device, uint64_t faulty_pc)
   {
      struct radv_shader *shader;
   uint64_t start_addr, end_addr;
            shader = radv_find_shader(device, faulty_pc);
   if (!shader)
            start_addr = radv_shader_get_va(shader);
   end_addr = start_addr + shader->code_size;
            fprintf(stderr,
         "Faulty shader found "
            /* Get the list of instructions.
   * Buffer size / 4 is the upper bound of the instruction count.
   */
   unsigned num_inst = 0;
            /* Split the disassembly string into instructions. */
            /* Print instructions with annotations. */
   for (unsigned i = 0; i < num_inst; i++) {
               if (start_addr + inst->offset == faulty_pc) {
      fprintf(stderr, "\n!!! Faulty instruction below !!!\n");
   fprintf(stderr, "%s\n", inst->text);
      } else {
                        }
      struct radv_sq_hw_reg {
      uint32_t status;
   uint32_t trap_sts;
   uint32_t hw_id;
      };
      static void
   radv_dump_sq_hw_regs(struct radv_device *device)
   {
      enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   enum radeon_family family = device->physical_device->rad_info.family;
            fprintf(stderr, "\nHardware registers:\n");
   if (device->physical_device->rad_info.gfx_level >= GFX10) {
      ac_dump_reg(stderr, gfx_level, family, R_000408_SQ_WAVE_STATUS, regs->status, ~0);
   ac_dump_reg(stderr, gfx_level, family, R_00040C_SQ_WAVE_TRAPSTS, regs->trap_sts, ~0);
   ac_dump_reg(stderr, gfx_level, family, R_00045C_SQ_WAVE_HW_ID1, regs->hw_id, ~0);
      } else {
      ac_dump_reg(stderr, gfx_level, family, R_000048_SQ_WAVE_STATUS, regs->status, ~0);
   ac_dump_reg(stderr, gfx_level, family, R_00004C_SQ_WAVE_TRAPSTS, regs->trap_sts, ~0);
   ac_dump_reg(stderr, gfx_level, family, R_000050_SQ_WAVE_HW_ID, regs->hw_id, ~0);
      }
      }
      void
   radv_check_trap_handler(struct radv_queue *queue)
   {
      enum amd_ip_type ring = radv_queue_ring(queue);
   struct radv_device *device = queue->device;
            /* Wait for the context to be idle in a finite time. */
            /* Try to detect if the trap handler has been reached by the hw by
   * looking at ttmp0 which should be non-zero if a shader exception
   * happened.
   */
   if (!device->tma_ptr[4])
         #if 0
   fprintf(stderr, "tma_ptr:\n");
   for (unsigned i = 0; i < 10; i++)
   fprintf(stderr, "tma_ptr[%d]=0x%x\n", i, device->tma_ptr[i]);
   #endif
                  uint32_t ttmp0 = device->tma_ptr[4];
            /* According to the ISA docs, 3.10 Trap and Exception Registers:
   *
   * "{ttmp1, ttmp0} = {3'h0, pc_rewind[3:0], HT[0], trapID[7:0], PC[47:0]}"
   *
   * "When the trap handler is entered, the PC of the faulting
   *  instruction is: (PC - PC_rewind * 4)."
   * */
   uint8_t trap_id = (ttmp1 >> 16) & 0xff;
   uint8_t ht = (ttmp1 >> 24) & 0x1;
   uint8_t pc_rewind = (ttmp1 >> 25) & 0xf;
                                 }
