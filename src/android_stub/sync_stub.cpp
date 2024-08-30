   #include <sync/sync.h>
      extern "C" {
      /* timeout in msecs */
   int sync_wait(int fd, int timeout)
   {
         }
      int sync_merge(const char *name, int fd, int fd2)
   {
         }
      struct sync_file_info* sync_file_info(int32_t fd)
   {
         }
      void sync_file_info_free(struct sync_file_info* info)
   {
   }
      }
