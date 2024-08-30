   /*
   * Copyright © 2022 Igalia S.L.
   * SPDX-License-Identifier: MIT
   */
      #include <assert.h>
   #include <ctype.h>
   #include <err.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <getopt.h>
   #include <inttypes.h>
   #include <signal.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #if !FD_REPLAY_KGSL
   #include <xf86drm.h>
   #include "drm-uapi/msm_drm.h"
   #else
   #include "../vulkan/msm_kgsl.h"
   #endif
      #include <sys/ioctl.h>
   #include <sys/mman.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <sys/wait.h>
      #include "util/os_time.h"
   #include "util/rb_tree.h"
   #include "util/u_vector.h"
   #include "util/vma.h"
   #include "buffers.h"
   #include "cffdec.h"
   #include "io.h"
   #include "redump.h"
   #include "rdutil.h"
      /**
   * Replay command stream obtained from:
   * - /sys/kernel/debug/dri/0/rd
   * - /sys/kernel/debug/dri/0/hangrd
   * !!! Command stream capture should be done with ALL buffers:
   * - echo 1 > /sys/module/msm/parameters/rd_full
   *
   * Requires kernel with MSM_INFO_SET_IOVA support.
   * In case userspace IOVAs are not supported, like on KGSL, we have to
   * pre-allocate a single buffer and hope it always allocated starting
   * from the same address.
   *
   * TODO: Misrendering, would require marking framebuffer images
   *       at each renderpass in order to fetch and decode them.
   *
   * Code from Freedreno/Turnip is not re-used here since the relevant
   * pieces may introduce additional allocations which cannot be allowed
   * during the replay.
   *
   * For how-to see freedreno.rst
   */
      static const char *exename = NULL;
      static const uint64_t FAKE_ADDRESS_SPACE_SIZE = 1024 * 1024 * 1024;
      static int handle_file(const char *filename, uint32_t first_submit,
                  static void
   print_usage(const char *name)
   {
      /* clang-format off */
   fprintf(stderr, "Usage:\n\n"
         "\t%s [OPTSIONS]... FILE...\n\n"
   "Options:\n"
   "\t-e, --exe=NAME         - only use cmdstream from named process\n"
   "\t-o  --override=submit  - № of the submit to override\n"
   "\t-g  --generator=path   - executable which generate cmdstream for override\n"
   "\t-f  --first=submit     - first submit № to replay\n"
   "\t-l  --last=submit      - last submit № to replay\n"
   "\t-h, --help             - show this message\n"
   /* clang-format on */
      }
      /* clang-format off */
   static const struct option opts[] = {
         { "exe",       required_argument, 0, 'e' },
   { "override",  required_argument, 0, 'o' },
   { "generator", required_argument, 0, 'g' },
   { "first",     required_argument, 0, 'f' },
   { "last",      required_argument, 0, 'l' },
   };
   /* clang-format on */
      int
   main(int argc, char **argv)
   {
      int ret = -1;
            uint32_t submit_to_override = -1;
   uint32_t first_submit = 0;
   uint32_t last_submit = -1;
            while ((c = getopt_long(argc, argv, "e:o:g:f:l:h", opts, NULL)) != -1) {
      switch (c) {
   case 0:
      /* option that set a flag, nothing to do */
      case 'e':
      exename = optarg;
      case 'o':
      submit_to_override = strtoul(optarg, NULL, 0);
      case 'g':
      cmdstreamgen = optarg;
      case 'f':
      first_submit = strtoul(optarg, NULL, 0);
      case 'l':
      last_submit = strtoul(optarg, NULL, 0);
      case 'h':
   default:
                     while (optind < argc) {
      ret = handle_file(argv[optind], first_submit, last_submit,
         if (ret) {
      fprintf(stderr, "error reading: %s\n", argv[optind]);
      }
               if (ret)
               }
      struct buffer {
               uint32_t gem_handle;
   uint64_t size;
   uint64_t iova;
            bool used;
      };
      struct cmdstream {
      uint64_t iova;
      };
      struct device {
               struct rb_tree buffers;
                     uint64_t shader_log_iova;
                     uint32_t va_id;
   void *va_map;
         #ifdef FD_REPLAY_KGSL
         #endif
   };
      void buffer_mem_free(struct device *dev, struct buffer *buf);
      static int
   rb_buffer_insert_cmp(const struct rb_node *n1, const struct rb_node *n2)
   {
      const struct buffer *buf1 = (const struct buffer *)n1;
   const struct buffer *buf2 = (const struct buffer *)n2;
   /* Note that gpuaddr comparisions can overflow an int: */
   if (buf1->iova > buf2->iova)
         else if (buf1->iova < buf2->iova)
            }
      static int
   rb_buffer_search_cmp(const struct rb_node *node, const void *addrptr)
   {
      const struct buffer *buf = (const struct buffer *)node;
   uint64_t iova = *(uint64_t *)addrptr;
   if (buf->iova + buf->size <= iova)
         else if (buf->iova > iova)
            }
      static struct buffer *
   device_get_buffer(struct device *dev, uint64_t iova)
   {
      if (iova == 0)
         return (struct buffer *)rb_tree_search(&dev->buffers, &iova,
      }
      static void
   device_mark_buffers(struct device *dev)
   {
      rb_tree_foreach_safe (struct buffer, buf, &dev->buffers, node) {
            }
      static void
   device_free_unused_buffers(struct device *dev)
   {
      rb_tree_foreach_safe (struct buffer, buf, &dev->buffers, node) {
      if (!buf->used) {
      buffer_mem_free(dev, buf);
   rb_tree_remove(&dev->buffers, &buf->node);
            }
      static void
   device_print_shader_log(struct device *dev)
   {
      struct shader_log {
      uint64_t cur_iova;
   union {
      uint32_t entries_u32[0];
                  if (dev->shader_log_iova != 0)
   {
      struct buffer *buf = device_get_buffer(dev, dev->shader_log_iova);
   if (buf) {
      struct shader_log *log = buf->map + (dev->shader_log_iova - buf->iova);
                           for (uint32_t i = 0; i < count; i++) {
      printf("[%u] %08x %.4f\n", i, log->entries_u32[i],
                        }
      static void
   device_print_cp_log(struct device *dev)
   {
      struct cp_log {
      uint64_t cur_iova;
   uint64_t tmp;
               struct cp_log_entry {
      uint64_t size;
               if (dev->cp_log_iova == 0)
            struct buffer *buf = device_get_buffer(dev, dev->cp_log_iova);
   if (!buf)
            struct cp_log *log = buf->map + (dev->cp_log_iova - buf->iova);
   if (log->first_entry_size == 0)
            struct cp_log_entry *log_entry =
         uint32_t idx = 0;
   while (log_entry->size != 0) {
      printf("\nCP Log [%u]:\n", idx++);
            for (uint32_t i = 0; i < dwords; i++) {
      if (i % 8 == 0)
         printf("%08x ", log_entry->data[i]);
   if (i % 8 == 7)
      }
            log_entry = (void *)log_entry + log_entry->size +
         }
      #if !FD_REPLAY_KGSL
   static inline void
   get_abs_timeout(struct drm_msm_timespec *tv, uint64_t ns)
   {
      struct timespec t;
   clock_gettime(CLOCK_MONOTONIC, &t);
   tv->tv_sec = t.tv_sec + ns / 1000000000;
      }
      static struct device *
   device_create()
   {
               dev->fd = drmOpenWithType("msm", NULL, DRM_NODE_RENDER);
   if (dev->fd < 0) {
                           struct drm_msm_param req = {
      .pipe = MSM_PIPE_3D0,
               int ret = drmCommandWriteRead(dev->fd, DRM_MSM_GET_PARAM, &req, sizeof(req));
            if (!ret) {
      req.param = MSM_PARAM_VA_SIZE;
   ret = drmCommandWriteRead(dev->fd, DRM_MSM_GET_PARAM, &req, sizeof(req));
                        if (ret) {
               struct drm_msm_gem_new req_new = {.size = FAKE_ADDRESS_SPACE_SIZE, .flags = MSM_BO_CACHED_COHERENT};
   drmCommandWriteRead(dev->fd, DRM_MSM_GEM_NEW, &req_new, sizeof(req_new));
            struct drm_msm_gem_info req_info = {
      .handle = req_new.handle,
               drmCommandWriteRead(dev->fd,
                  struct drm_msm_gem_info req_offset = {
      .handle = req_new.handle,
                        dev->va_map = mmap(0, FAKE_ADDRESS_SPACE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
         if (dev->va_map == MAP_FAILED) {
                  va_start = dev->va_iova;
                        rb_tree_init(&dev->buffers);
   util_vma_heap_init(&dev->vma, va_start, ROUND_DOWN_TO(va_size, 4096));
               }
      static void
   device_submit_cmdstreams(struct device *dev)
   {
      device_free_unused_buffers(dev);
            if (!u_vector_length(&dev->cmdstreams))
                     uint32_t idx = 0;
   struct cmdstream *cmd;
   u_vector_foreach(cmd, &dev->cmdstreams) {
               uint32_t bo_idx = 0;
   rb_tree_foreach (struct buffer, buf, &dev->buffers, node) {
                                 if (cmdstream_buf)
            struct drm_msm_gem_submit_cmd *submit_cmd = &cmds[idx];
   submit_cmd->type = MSM_SUBMIT_CMD_BUF;
   submit_cmd->submit_idx = bo_idx;
   if (dev->has_set_iova) {
         } else {
         }
   submit_cmd->size = cmd->size;
   submit_cmd->pad = 0;
   submit_cmd->nr_relocs = 0;
                        uint32_t bo_count = 0;
   rb_tree_foreach (struct buffer, buf, &dev->buffers, node) {
      if (buf)
               if (!dev->has_set_iova) {
                  struct drm_msm_gem_submit_bo *bo_list =
            if (dev->has_set_iova) {
      uint32_t bo_idx = 0;
   rb_tree_foreach (struct buffer, buf, &dev->buffers, node) {
      struct drm_msm_gem_submit_bo *submit_bo = &bo_list[bo_idx++];
   submit_bo->handle = buf->gem_handle;
   submit_bo->flags =
                        } else {
      bo_list[0].handle = dev->va_id;
   bo_list[0].flags =
                     struct drm_msm_gem_submit submit_req = {
      .flags = MSM_PIPE_3D0,
   .queueid = 0,
   .bos = (uint64_t)(uintptr_t)bo_list,
   .nr_bos = bo_count,
   .cmds = (uint64_t)(uintptr_t)cmds,
   .nr_cmds = u_vector_length(&dev->cmdstreams),
   .in_syncobjs = 0,
   .out_syncobjs = 0,
   .nr_in_syncobjs = 0,
   .nr_out_syncobjs = 0,
               int ret = drmCommandWriteRead(dev->fd, DRM_MSM_GEM_SUBMIT, &submit_req,
            if (ret) {
                  /* Wait for submission to complete in order to be sure that
   * freeing buffers would free their VMAs in the kernel.
   * Makes sure that new allocations won't clash with old ones.
   */
   struct drm_msm_wait_fence wait_req = {
      .fence = submit_req.fence,
      };
            ret =
         if (ret && (ret != -ETIMEDOUT)) {
                  u_vector_finish(&dev->cmdstreams);
            device_print_shader_log(dev);
      }
      static void
   buffer_mem_alloc(struct device *dev, struct buffer *buf)
   {
               if (!dev->has_set_iova) {
      uint64_t offset = buf->iova - dev->va_iova;
   assert(offset < FAKE_ADDRESS_SPACE_SIZE && (offset + buf->size) <= FAKE_ADDRESS_SPACE_SIZE);
   buf->map = ((uint8_t*)dev->va_map) + offset;
               {
               int ret =
         if (ret) {
                              {
      struct drm_msm_gem_info req = {
      .handle = buf->gem_handle,
   .info = MSM_INFO_SET_IOVA,
               int ret =
            if (ret) {
                     {
      struct drm_msm_gem_info req = {
      .handle = buf->gem_handle,
               int ret =
         if (ret) {
                  void *map = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
         if (map == MAP_FAILED) {
                        }
      void
   buffer_mem_free(struct device *dev, struct buffer *buf)
   {
      if (dev->has_set_iova) {
               struct drm_gem_close req = {
         };
                  }
      #else
   static int
   safe_ioctl(int fd, unsigned long request, void *arg)
   {
               do {
                     }
      static struct device *
   device_create()
   {
                        dev->fd = open(path, O_RDWR | O_CLOEXEC);
   if (dev->fd < 0) {
                  struct kgsl_gpumem_alloc_id req = {
      .size = FAKE_ADDRESS_SPACE_SIZE,
               int ret = safe_ioctl(dev->fd, IOCTL_KGSL_GPUMEM_ALLOC_ID, &req);
   if (ret) {
                  dev->va_id = req.id;
   dev->va_iova = req.gpuaddr;
   dev->va_map = mmap(0, FAKE_ADDRESS_SPACE_SIZE, PROT_READ | PROT_WRITE,
            rb_tree_init(&dev->buffers);
   util_vma_heap_init(&dev->vma, req.gpuaddr, ROUND_DOWN_TO(FAKE_ADDRESS_SPACE_SIZE, 4096));
            struct kgsl_drawctxt_create drawctxt_req = {
      .flags = KGSL_CONTEXT_SAVE_GMEM |
                     ret = safe_ioctl(dev->fd, IOCTL_KGSL_DRAWCTXT_CREATE, &drawctxt_req);
   if (ret) {
                                       }
      static void
   device_submit_cmdstreams(struct device *dev)
   {
      device_free_unused_buffers(dev);
            if (!u_vector_length(&dev->cmdstreams))
                     uint32_t idx = 0;
   struct cmdstream *cmd;
   u_vector_foreach(cmd, &dev->cmdstreams) {
      struct kgsl_command_object *submit_cmd = &cmds[idx++];
   submit_cmd->gpuaddr = cmd->iova;
   submit_cmd->size = cmd->size;
   submit_cmd->flags = KGSL_CMDLIST_IB;
               struct kgsl_gpu_command submit_req = {
      .flags = KGSL_CMDBATCH_SUBMIT_IB_LIST,
   .cmdlist = (uintptr_t) &cmds,
   .cmdsize = sizeof(struct kgsl_command_object),
   .numcmds = u_vector_length(&dev->cmdstreams),
   .numsyncs = 0,
                        if (ret) {
                  struct kgsl_device_waittimestamp_ctxtid wait = {
      .context_id = dev->context_id,
   .timestamp = submit_req.timestamp,
                        if (ret) {
                  u_vector_finish(&dev->cmdstreams);
            device_print_shader_log(dev);
      }
      static void
   buffer_mem_alloc(struct device *dev, struct buffer *buf)
   {
                  }
      void
   buffer_mem_free(struct device *dev, struct buffer *buf)
   {
         }
   #endif
      static void
   upload_buffer(struct device *dev, uint64_t iova, unsigned int size,
         {
               if (!buf) {
      buf = calloc(sizeof(struct buffer), 1);
   buf->iova = iova;
                        } else if (buf->size != size) {
      buffer_mem_free(dev, buf);
   buf->size = size;
                           }
      static int
   override_cmdstream(struct device *dev, struct cmdstream *cs,
         {
   #if FD_REPLAY_KGSL
         #else
         #endif
            /* Find a free space for the new cmdstreams and resources we will use
   * when overriding existing cmdstream.
   */
   /* TODO: should the size be configurable? */
   uint64_t hole_size = 32 * 1024 * 1024;
   dev->vma.alloc_high = true;
   uint64_t hole_iova = util_vma_heap_alloc(&dev->vma, hole_size, 4096);
   dev->vma.alloc_high = false;
            char cmd[2048];
   snprintf(cmd, sizeof(cmd),
                           int ret = system(cmd);
   if (ret) {
      fprintf(stderr, "Error executing %s\n", cmd);
               struct io *io;
            io = io_open(tmpfilename);
   if (!io) {
      fprintf(stderr, "could not open: %s\n", tmpfilename);
               struct {
      unsigned int len;
               while (parse_rd_section(io, &ps)) {
      switch (ps.type) {
   case RD_GPUADDR:
      parse_addr(ps.buf, ps.sz, &gpuaddr.len, &gpuaddr.gpuaddr);
   /* no-op */
      case RD_BUFFER_CONTENTS:
      upload_buffer(dev, gpuaddr.gpuaddr, gpuaddr.len, ps.buf);
   ps.buf = NULL;
      case RD_CMDSTREAM_ADDR: {
      unsigned int sizedwords;
   uint64_t gpuaddr;
                  cs->iova = gpuaddr;
   cs->size = sizedwords * sizeof(uint32_t);
      }
   case RD_SHADER_LOG_BUFFER: {
      unsigned int sizedwords;
   parse_addr(ps.buf, ps.sz, &sizedwords, &dev->shader_log_iova);
      }
   case RD_CP_LOG_BUFFER: {
      unsigned int sizedwords;
   parse_addr(ps.buf, ps.sz, &sizedwords, &dev->cp_log_iova);
      }
   default:
                     io_close(io);
   if (ps.ret < 0) {
                     }
      static int
   handle_file(const char *filename, uint32_t first_submit, uint32_t last_submit,
         {
      struct io *io;
   int submit = 0;
   bool skip = false;
   bool need_submit = false;
                     if (!strcmp(filename, "-"))
         else
            if (!io) {
      fprintf(stderr, "could not open: %s\n", filename);
                        struct {
      unsigned int len;
               while (parse_rd_section(io, &ps)) {
      switch (ps.type) {
   case RD_TEST:
   case RD_VERT_SHADER:
   case RD_FRAG_SHADER:
      /* no-op */
      case RD_CMD:
      skip = false;
   if (exename) {
         } else {
      skip |= (strstr(ps.buf, "fdperf") == ps.buf);
   skip |= (strstr(ps.buf, "chrome") == ps.buf);
   skip |= (strstr(ps.buf, "surfaceflinger") == ps.buf);
                  case RD_GPUADDR:
      if (need_submit) {
      need_submit = false;
               parse_addr(ps.buf, ps.sz, &gpuaddr.len, &gpuaddr.gpuaddr);
   /* no-op */
      case RD_BUFFER_CONTENTS:
      /* TODO: skip buffer uploading and even reading if this buffer
   * is used for submit outside of [first_submit, last_submit]
   * range. A set of buffers is shared between several cmdstreams,
   * so we'd have to find starting from which RD_CMD to upload
   * the buffers.
   */
   upload_buffer(dev, gpuaddr.gpuaddr, gpuaddr.len, ps.buf);
      case RD_CMDSTREAM_ADDR: {
      unsigned int sizedwords;
                  bool add_submit = !skip && (submit >= first_submit) && (submit <= last_submit);
                                    if (submit == submit_to_override) {
      if (override_cmdstream(dev, cs, cmdstreamgen) < 0)
      } else {
                                    submit++;
      }
   case RD_GPU_ID: {
      uint32_t gpu_id = parse_gpu_id(ps.buf);
   if (gpu_id)
            }
   case RD_CHIP_ID: {
      uint64_t chip_id = parse_chip_id(ps.buf);
   printf("chip_id: 0x%" PRIx64 "\n", chip_id);
      }
   default:
                     if (need_submit)
                     io_close(io);
            if (ps.ret < 0) {
         }
      }
