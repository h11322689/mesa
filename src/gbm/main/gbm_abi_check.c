   /*
   * Copyright © 2021 NVIDIA Corporation
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "gbm_backend_abi.h" /* Current GBM backend ABI implementation */
      #include <stddef.h> /* offsetof */
   #include <stdio.h> /* printf */
      /*
   * The following are previous implementations of the structures defined in
   * gbm_backend_abi.h, with their ABI version appended.
   *
   * DO NOT EVER CHANGE EXISTING DEFINITIONS HERE!
   *
   * Changing them implies breaking the GBM backend ABI. Instead, to extend the
   * ABI, in gbm_backend_abi.h:
   *
   * -Add a new versioned struct
   * -Append it to the associated top-level object's struct
   * -Increment GBM_BACKEND_ABI_VERSION
   *
   * Then, here:
   *
   * -Add a new block of definitions below for the new ABI content
   * -Add a new block of checks in main()
   */
      /*
   * From: Simon Ser - "gbm: assume USE_SCANOUT in create_with_modifiers"
   *
   * Note: ABI 1 is identical to ABI 0, except gbm_device_v0.bo_create can
   * provide both modifiers and usage.
   */
   #define GBM_BACKEND_ABI_VERSION_abi0 1
   struct gbm_device_v0_abi0 {
      const struct gbm_backend_desc *backend_desc;
   uint32_t backend_version;
   int fd;
   const char *name;
   void (*destroy)(struct gbm_device *gbm);
   int (*is_format_supported)(struct gbm_device *gbm,
               int (*get_format_modifier_plane_count)(struct gbm_device *device,
               struct gbm_bo *(*bo_create)(struct gbm_device *gbm,
                                 struct gbm_bo *(*bo_import)(struct gbm_device *gbm, uint32_t type,
         void *(*bo_map)(struct gbm_bo *bo,
                           void (*bo_unmap)(struct gbm_bo *bo, void *map_data);
   int (*bo_write)(struct gbm_bo *bo, const void *buf, size_t data);
   int (*bo_get_fd)(struct gbm_bo *bo);
   int (*bo_get_planes)(struct gbm_bo *bo);
   union gbm_bo_handle (*bo_get_handle)(struct gbm_bo *bo, int plane);
   int (*bo_get_plane_fd)(struct gbm_bo *bo, int plane);
   uint32_t (*bo_get_stride)(struct gbm_bo *bo, int plane);
   uint32_t (*bo_get_offset)(struct gbm_bo *bo, int plane);
   uint64_t (*bo_get_modifier)(struct gbm_bo *bo);
   void (*bo_destroy)(struct gbm_bo *bo);
   struct gbm_surface *(*surface_create)(struct gbm_device *gbm,
                           struct gbm_bo *(*surface_lock_front_buffer)(struct gbm_surface *surface);
   void (*surface_release_buffer)(struct gbm_surface *surface,
         int (*surface_has_free_buffers)(struct gbm_surface *surface);
      };
      struct gbm_device_abi0 {
      /* Hack to make a gbm_device detectable by its first element. */
   struct gbm_device *(*dummy)(int);
      };
      /**
   * GBM buffer object interface corresponding to GBM_BACKEND_ABI_VERSION = 0
   *
   * DO NOT MODIFY THIS STRUCT. Instead, introduce a gbm_bo_v1, increment
   * GBM_BACKEND_ABI_VERSION, and append gbm_bo_v1 to gbm_bo.
   */
   struct gbm_bo_v0_abi0 {
      uint32_t width;
   uint32_t height;
   uint32_t stride;
   uint32_t format;
   union gbm_bo_handle  handle;
   void *user_data;
      };
      /**
   * The allocated buffer object.
   *
   * The members in this structure should not be accessed directly.
   *
   * To modify this structure, introduce a new gbm_bo_v<N> structure, add it to
   * the end of this structure, and increment GBM_BACKEND_ABI_VERSION.
   */
   struct gbm_bo_abi0 {
      struct gbm_device *gbm;
      };
      /**
   * GBM surface interface corresponding to GBM_BACKEND_ABI_VERSION = 0
   *
   * DO NOT MODIFY THIS STRUCT. Instead, introduce a gbm_surface_v1, increment
   * GBM_BACKEND_ABI_VERSION, and append gbm_surface_v1 to gbm_surface.
   */
   struct gbm_surface_v0_abi0 {
      uint32_t width;
   uint32_t height;
   uint32_t format;
   uint32_t flags;
   struct {
      uint64_t *modifiers;
         };
      /**
   * An allocated GBM surface.
   *
   * To modify this structure, introduce a new gbm_surface_v<N> structure, add it
   * to the end of this structure, and increment GBM_BACKEND_ABI_VERSION.
   */
   struct gbm_surface_abi0 {
      struct gbm_device *gbm;
      };
      /**
   * GBM backend interfaces corresponding to GBM_BACKEND_ABI_VERSION = 0
   *
   * DO NOT MODIFY THIS STRUCT. Instead, introduce a gbm_backend_v1, increment
   * GBM_BACKEND_ABI_VERSION, append gbm_backend_v1 to gbm_backend.
   */
   struct gbm_backend_v0_abi0 {
      /**
   * The version of the GBM backend interface supported by this backend. This
   * is set by the backend itself, and may be greater or less than the version
   * supported by the loader. It is the responsibility of the GBM loader to
   * respect this version when accessing fields in this structure.
   */
            const char *backend_name;
      };
      /**
   * The interface exposed by an external GBM backend.
   *
   * To modify this structure, introduce a new gbm_backend_v<N> structure, add it
   * to the end of this structure, and increment GBM_BACKEND_ABI_VERSION.
   */
   struct gbm_backend_abi0 {
         };
      /**
   * GBM interfaces exposed to GBM backends at GBM_BACKEND_ABI_VERSION >= 0
   *
   * DO NOT MODIFY THIS STRUCT. Instead, introduce a gbm_core_v1, increment
   * GBM_BACKEND_ABI_VERSION, and append gbm_core_v1 to gbm_backend.
   */
   struct gbm_core_v0_abi0 {
      /**
   * The version of the GBM backend interface supported by the GBM loader. This
   * is set by the loader, and may be greater or less than the version
   * supported by a given backend. It is the responsibility of the backend to
   * respect this version when accessing fields in this structure and other
   * structures allocated or modified by the loader.
   */
               };
      /**
   * The interface exposed by the GBM core/loader code to GBM backends.
   *
   * To modify this structure, introduce a new gbm_core_v<N> structure, add it
   * to the end of this structure, and increment GBM_BACKEND_ABI_VERSION.
   */
   struct gbm_core_abi0 {
         };
      typedef const struct gbm_backend *(*GBM_GET_BACKEND_PROC_PTR_abi0)(const struct gbm_core *gbm_core);
      /*
   * Structure/member ABI-checking helper macros
   */
   #define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)
      #define CHECK_RENAMED_MEMBER_BASE(type, a_ver, b_ver, a_member, b_member)     \
      do {                                                                       \
      if (offsetof(struct type ## a_ver, a_member) !=                         \
      offsetof(struct type ## b_ver, b_member)) {                         \
   printf("Backards incompatible change detected!\n   "                 \
         "offsetof(struct " #type #a_ver "::" #a_member ") != "        \
      }                                                                       \
         if (MEMBER_SIZE(struct type ## a_ver, a_member) !=                      \
      MEMBER_SIZE(struct type ## b_ver, b_member)) {                      \
   printf("Backards incompatible change detected!\n   "                 \
         "MEMBER_SIZE(struct " #type #a_ver "::" #a_member ") != "     \
               #define CHECK_RENAMED_MEMBER_TYPE(type, a_ver, b_ver, a_member, b_member)     \
      do {                                                                       \
      /* Compile-time type compatibility check */                             \
   struct type ## a_ver a;                                                 \
   struct type ## b_ver b = {0};                                           \
   a.a_member = b.b_member;                                                \
            #define CHECK_RENAMED_MEMBER(type, a_ver, b_ver, a_member, b_member)          \
      do {                                                                       \
      CHECK_RENAMED_MEMBER_BASE(type, a_ver, b_ver, a_member, b_member);      \
         #define CHECK_RENAMED_MEMBER_NO_TYPE(type, a_ver, b_ver, a_member, b_member)  \
            #define CHECK_MEMBER(type, a_ver, b_ver, member) \
         #define CHECK_MEMBER_NO_TYPE(type, a_ver, b_ver, member) \
         #define CHECK_MEMBER_CURRENT(type, a_ver, member) \
         #define CHECK_MEMBER_CURRENT_NO_TYPE(type, a_ver, member) \
            #define CHECK_SIZE(type, a_ver, b_ver)                                     \
      do {                                                                    \
      if (sizeof(struct type ## a_ver) >                                   \
      sizeof(struct type ## b_ver)) {                                  \
   printf("Backards incompatible change detected!\n   "              \
         "sizeof(struct " #type #a_ver ") > "                       \
               #define CHECK_SIZE_CURRENT(type, a_ver)                                    \
      do {                                                                    \
      if (sizeof(struct type ## a_ver) !=                                  \
      sizeof(struct type)) {                                           \
   printf("Backards incompatible change detected!\n   "              \
         "sizeof(struct " #type #a_ver ") != "                      \
               #define CHECK_VERSION(a_ver, b_ver)                                        \
      do {                                                                    \
      if ((GBM_BACKEND_ABI_VERSION ## a_ver) >=                            \
      (GBM_BACKEND_ABI_VERSION ## b_ver)) {                            \
   printf("Backards incompatible change detected!\n   "              \
         "GBM_BACKEND_ABI_VERSION" #a_ver " >= "                    \
               #define CHECK_VERSION_CURRENT(a_ver)                                       \
      do {                                                                    \
      if ((GBM_BACKEND_ABI_VERSION ## a_ver) !=                            \
      (GBM_BACKEND_ABI_VERSION)) {                                     \
   printf("Backards incompatible change detected!\n   "              \
         "GBM_BACKEND_ABI_VERSION" #a_ver " != "                    \
               #define CHECK_PROC(proc, a_ver, b_ver)                                     \
      do {                                                                    \
      proc ## a_ver a;                                                     \
   proc ## b_ver b = NULL;                                              \
   a = b;                                                               \
            #define CHECK_PROC_CURRENT(proc, a_ver)                                    \
            int main(int argc, char **argv)
   {
      /********************************************/
   /*** Compare Current ABI to ABI version 0 ***/
            /* Check current gbm_device ABI against gbm_device_abi0*/
   CHECK_MEMBER_CURRENT(gbm_device, _abi0, dummy);
            CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, backend_desc);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, backend_version);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, fd);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, name);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, destroy);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, is_format_supported);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, get_format_modifier_plane_count);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_create);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_import);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_map);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_unmap);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_write);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_fd);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_planes);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_handle);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_plane_fd);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_stride);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_offset);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_get_modifier);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, bo_destroy);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, surface_create);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, surface_lock_front_buffer);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, surface_release_buffer);
   CHECK_MEMBER_CURRENT(gbm_device_v0, _abi0, surface_has_free_buffers);
            /* Size of ABI-versioned substructures verified by above member checks */
               /* Check current gbm_bo ABI against gbm_bo_abi0*/
   CHECK_MEMBER_CURRENT(gbm_bo, _abi0, gbm);
            CHECK_MEMBER_CURRENT(gbm_bo_v0, _abi0, width);
   CHECK_MEMBER_CURRENT(gbm_bo_v0, _abi0, height);
   CHECK_MEMBER_CURRENT(gbm_bo_v0, _abi0, stride);
   CHECK_MEMBER_CURRENT(gbm_bo_v0, _abi0, format);
   CHECK_MEMBER_CURRENT(gbm_bo_v0, _abi0, handle);
   CHECK_MEMBER_CURRENT(gbm_bo_v0, _abi0, user_data);
            /* Size of ABI-versioned substructures verified by above member checks */
               /* Check current gbm_surface ABI against gbm_surface_abi0 */
   CHECK_MEMBER_CURRENT(gbm_surface, _abi0, gbm);
            CHECK_MEMBER_CURRENT(gbm_surface_v0, _abi0, width);
   CHECK_MEMBER_CURRENT(gbm_surface_v0, _abi0, height);
   CHECK_MEMBER_CURRENT(gbm_surface_v0, _abi0, format);
   CHECK_MEMBER_CURRENT(gbm_surface_v0, _abi0, flags);
   CHECK_MEMBER_CURRENT(gbm_surface_v0, _abi0, modifiers);
            /* Size of ABI-versioned substructures verified by above member checks */
               /* Check current gbm_backend ABI against gbm_backend_abi0 */
            CHECK_MEMBER_CURRENT(gbm_backend_v0, _abi0, backend_version);
   CHECK_MEMBER_CURRENT(gbm_backend_v0, _abi0, backend_name);
            /* Size of ABI-versioned substructures verified by above member checks */
               /* Check current gbm_core ABI against gbm_core_abi0 */
            CHECK_MEMBER_CURRENT(gbm_core_v0, _abi0, core_version);
            /* Size of ABI-versioned substructures verified by above member checks */
                           }
