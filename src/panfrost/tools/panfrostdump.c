   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   */
      /*
   * Debug dump analyser for panfrost.  In case of a gpu crash/hang,
   * the coredump should be found in:
   *
   *    /sys/class/devcoredump/devcd<n>/data
   *
   * The crashdump will hang around for 5min, it can be cleared by writing to
   * the file, ie:
   *
   *    echo 1 > /sys/class/devcoredump/devcd<n>/data
   *
   * (the driver won't log any new crashdumps until the previous one is cleared
   * or times out after 5min)
   */
      #include <endian.h>
   #include <errno.h>
   #include <getopt.h>
   #include <inttypes.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
      #include <drm-uapi/panfrost_drm.h>
      #include "decode.h"
      /* Same as panfrost_dump_object_header, but with field
   * entries in host byte order
   */
   struct panfrost_dump_object_header_ho {
      uint32_t magic;
   uint32_t type;
   uint32_t file_size;
            union {
      struct pan_reg_hdr_ho {
      uint64_t jc;
   uint32_t gpu_id;
   uint32_t major;
   uint32_t minor;
               struct pan_bomap_hdr_ho {
      uint32_t valid;
   uint64_t iova;
                     };
      #define MAX_BODUMP_FILENAME 32
   #define GPU_PAGE_SIZE       4096
      static bool
   read_header(FILE *fp, struct panfrost_dump_object_header_ho *pdoh)
   {
      /* Fields in the coredump file header structures
   * are found in little-endian order
   */
   struct panfrost_dump_object_header doh_le;
            nr = fread(&doh_le, 1, sizeof(struct panfrost_dump_object_header), fp);
   if (nr < sizeof(struct panfrost_dump_object_header)) {
      fprintf(stderr, "Wrong header read\n");
               /* Convert from little-endian to host byte order */
   pdoh->magic = le32toh(doh_le.magic);
   if (pdoh->magic != PANFROSTDUMP_MAGIC) {
      fprintf(stderr, "Wrong header magic\n");
               pdoh->type = le32toh(doh_le.type);
   pdoh->file_offset = le32toh(doh_le.file_offset);
            switch (pdoh->type) {
   case PANFROSTDUMP_BUF_REG:
      pdoh->reghdr.jc = le64toh(doh_le.reghdr.jc);
   pdoh->reghdr.gpu_id = le32toh(doh_le.reghdr.gpu_id);
   pdoh->reghdr.major = le32toh(doh_le.reghdr.major);
   pdoh->reghdr.minor = le32toh(doh_le.reghdr.minor);
   pdoh->reghdr.nbos = le64toh(doh_le.reghdr.nbos);
      case PANFROSTDUMP_BUF_BO:
      pdoh->bomap.iova = le64toh(doh_le.bomap.iova);
   pdoh->bomap.valid = le32toh(doh_le.bomap.valid);
   pdoh->bomap.data[0] = le32toh(doh_le.bomap.data[0]);
                  }
      static bool
   read_register(uint32_t *ro, uint32_t *rv, FILE *fp)
   {
      /* Register pair we read form memory is
   * laid out in little-endian order
   */
   struct panfrost_dump_registers reg_le;
            nr = fread(&reg_le, 1, sizeof(reg_le), fp);
   if (nr < sizeof(reg_le)) {
      fprintf(stderr, "Wrong register read\n");
               *ro = le32toh(reg_le.reg);
               }
      static bool
   read_page_addr(uint64_t *phys_page, FILE *fp)
   {
      uint64_t phys_addr_le;
            nr = fread(&phys_addr_le, 1, sizeof(uint64_t), fp);
   if (nr < sizeof(uint64_t)) {
               /* Skip over to the next address */
   if (fseek(fp, sizeof(uint64_t) - nr, SEEK_CUR)) {
      perror("fseek error");
                                       }
      /* Keeping these definitions as global/static shouldn't be
   * an issue because this tool will always be single-threaded
   */
   static FILE *hdr_fp;
   static FILE *data_fp;
   static char **bos;
   static uint32_t bo_num;
      static void
   cleanup(void)
   {
      if (hdr_fp != NULL)
         if (data_fp != NULL)
         if (bos != NULL) {
      for (int k = 0; k < bo_num; k++)
               }
      static void
   print_help(const char *progname, FILE *file)
   {
      fprintf(file,
         "Usage: %s [OPTION] inputfile\n"
   "Decode Panfrost coredump file.\n\n"
   "    -h, --help             display this help and exit\n"
   "    -a, --addr             print BO physical addresses\n"
   "    -r, --regs             print Panfrost HW registers\n"
   "Example:\n"
      }
      int
   main(int argc, char *argv[])
   {
      struct panfrost_dump_object_header_ho doh;
   bool print_addr = false;
   bool print_reg = false;
   uint32_t gpu_id = 0;
   uint64_t jc = 0;
   size_t nbytes;
            if (argc < 2) {
      printf("Pass a coredump file\n");
               /* clang-format off */
   const struct option longopts[] = {
      { "addr", no_argument, (int *) &print_addr, true },
   { "regs", no_argument, (int *) &print_reg, true },
   { "help", no_argument, NULL, 'h' },
      };
            while ((c = getopt_long(argc, argv, "arh", longopts, NULL)) != -1) {
      switch (c) {
   case 'h':
      print_help(argv[0], stderr);
      case 'a':
      print_addr = true;
      case 'r':
      print_reg = true;
      default:
      fprintf(stderr, "Unknown option\n");
   print_help(argv[0], stderr);
                           atexit(cleanup);
            hdr_fp = fopen(argv[optind], "r");
   if (!hdr_fp) {
      perror("failed to open file");
               data_fp = fopen(argv[optind], "r");
   if (!data_fp) {
      perror("failed to open file");
               /* Read register header */
   if (!read_header(hdr_fp, &doh))
            if (fseek(data_fp, doh.file_offset, SEEK_SET)) {
      perror("fseek error");
               if (doh.type == PANFROSTDUMP_BUF_REG) {
      jc = doh.reghdr.jc;
   gpu_id = doh.reghdr.gpu_id;
            bos = calloc(bo_num, sizeof(char *));
   if (!bos) {
      fprintf(stderr, "Failed to allocate memory for BO pointer array\n");
                        if (print_reg) {
      puts("GPU registers:");
   for (i = 0;
      i < (doh.file_size / sizeof(struct panfrost_dump_registers));
   i++) {
                  if (read_register(&reg_offset, &reg_val, data_fp))
                     if (!read_header(hdr_fp, &doh))
            if (doh.type == PANFROSTDUMP_BUF_BOMAP) {
               if (!jc || !gpu_id) {
      fprintf(stderr, "Missing initial dump header\n");
               if (!read_header(hdr_fp, &doh))
            while (doh.type != PANFROSTDUMP_BUF_TRAILER) {
      if (doh.bomap.valid) {
      if (fseek(data_fp, bomap_offset + doh.bomap.data[0], SEEK_SET)) {
                        if (print_addr) {
                                                                           /* Copy the BO into external file */
                           if ((bodump = fopen(bodump_filename, "wb"))) {
      if (fseek(data_fp, doh.file_offset, SEEK_SET)) {
                        bos[j] = malloc(doh.file_size);
   if (!bos[j]) {
                                 nbytes = fread(bos[j], 1, doh.file_size, data_fp);
   if (nbytes < doh.file_size) {
      fprintf(stderr, "Read less than BO size: %u\n", errno);
      }
   nbytes = fwrite(bos[j], 1, doh.file_size, bodump);
   if (nbytes < doh.file_size) {
      fprintf(stderr, "Failed to write BO contents into file: %u\n",
                                          } else {
            } else {
                                       } else {
      if (!read_header(hdr_fp, &doh))
               if (doh.type != PANFROSTDUMP_BUF_TRAILER)
            pandecode_jc(ctx, jc, gpu_id);
            fclose(data_fp);
   fclose(hdr_fp);
               }
