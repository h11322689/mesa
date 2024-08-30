   /*
   * Copyright 2019 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include <errno.h>
      #include "os_socket.h"
      #if defined(__linux__)
      #include <fcntl.h>
   #include <poll.h>
   #include <stddef.h>
   #include <string.h>
   #include <sys/socket.h>
   #include <sys/un.h>
   #include <unistd.h>
      int
   os_socket_listen_abstract(const char *path, int count)
   {
      int s = socket(AF_UNIX, SOCK_STREAM, 0);
   if (s < 0)
            struct sockaddr_un addr;
   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
            /* Create an abstract socket */
   int ret = bind(s, (struct sockaddr*)&addr,
               if (ret < 0) {
      close(s);
               if (listen(s, count) < 0) {
      close(s);
                  }
      int
   os_socket_accept(int s)
   {
         }
      ssize_t
   os_socket_recv(int socket, void *buffer, size_t length, int flags)
   {
         }
      ssize_t
   os_socket_send(int socket, const void *buffer, size_t length, int flags)
   {
         }
      void
   os_socket_block(int s, bool block)
   {
      int old = fcntl(s, F_GETFL, 0);
   if (old == -1)
            /* TODO obey block */
   if (block)
         else
      }
      void
   os_socket_close(int s)
   {
         }
      #else
      int
   os_socket_listen_abstract(const char *path, int count)
   {
      errno = -ENOSYS;
      }
      int
   os_socket_accept(int s)
   {
      errno = -ENOSYS;
      }
      ssize_t
   os_socket_recv(int socket, void *buffer, size_t length, int flags)
   {
      errno = -ENOSYS;
      }
      ssize_t
   os_socket_send(int socket, const void *buffer, size_t length, int flags)
   {
      errno = -ENOSYS;
      }
      void
   os_socket_block(int s, bool block)
   {
   }
      void
   os_socket_close(int s)
   {
   }
      #endif
