   #include "zink_format.h"
   #include "vk_format.h"
      int
   main(int argc, char *argv[])
   {
      int ret = 0;
   for (int i = 0; i < PIPE_FORMAT_COUNT; ++i) {
      enum pipe_format pipe_fmt = i;
            /* skip unsupported formats */
   if (vk_fmt == VK_FORMAT_UNDEFINED)
                     /* This one gets aliased to ETC2 rather than round tripping. */
   if (pipe_fmt == PIPE_FORMAT_ETC1_RGB8 && roundtrip == PIPE_FORMAT_ETC2_RGB8)
            if (roundtrip != pipe_fmt) {
      fprintf(stderr, "Format does not roundtrip\n"
                  "\tgot: %s\n"
            }
      }
