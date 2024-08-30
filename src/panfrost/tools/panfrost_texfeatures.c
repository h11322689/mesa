   /*
   * Copyright 2022 Collabora, Ltd.
   * Copyright 2022 Amazon.com, Inc. or its affiliates.
   * SPDX-License-Identifier: MIT
   */
      #include <stdio.h>
   #include <lib/pan_device.h>
      /*
   * Mapping of texture feature bits to compressed formats on Mali-G57, other
   * Malis should be similar.
   */
   struct format {
      unsigned bit;
      };
      #define FMT(bit, name)                                                         \
      {                                                                           \
               static struct format formats[] = {
      FMT(1, "ETC2"),
   FMT(3, "ETC2 EAC"),
   FMT(19, "ETC2 PTA"),
   FMT(2, "EAC 1"),
   FMT(4, "EAC 2"),
   FMT(17, "EAC snorm 1"),
   FMT(18, "EAC snorm 2"),
   {0, NULL},
   FMT(20, "ASTC 3D LDR"),
   FMT(21, "ASTC 3D HDR"),
   FMT(22, "ASTC 2D LDR"),
   FMT(23, "ASTC 3D HDR"),
   {0, NULL},
   FMT(7, "BC1"),
   FMT(8, "BC2"),
   FMT(9, "BC3"),
   FMT(10, "BC4 unorm"),
   FMT(11, "BC4 snorm"),
   FMT(12, "BC5 unorm"),
   FMT(13, "BC5 snorm"),
   FMT(14, "BC6H UF16"),
   FMT(15, "BC6H SF16"),
      };
      /* ANSI escape code */
   #define RESET    "\033[0m"
   #define RED(x)   "\033[31m" x RESET
   #define GREEN(x) "\033[32m" x RESET
      int
   main(void)
   {
      int fd = drmOpenWithType("panfrost", NULL, DRM_NODE_RENDER);
   if (fd < 0) {
      fprintf(stderr, "No panfrost device\n");
               void *ctx = ralloc_context(NULL);
   struct panfrost_device dev = {0};
            uint32_t supported = dev.compressed_formats;
            printf("System-on-chip compressed texture support:"
            for (unsigned i = 0; i < ARRAY_SIZE(formats); ++i) {
      if (formats[i].name == NULL) {
      printf("\n");
               /* Maximum length for justification */
            bool ok = (supported & BITFIELD_BIT(formats[i].bit));
                        if (!all_ok) {
      printf(
      "\n"
   "This system-on-chip lacks support for some formats. This is not a driver bug.\n"
            panfrost_close_device(&dev);
      }
