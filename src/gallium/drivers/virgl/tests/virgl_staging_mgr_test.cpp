   /*
   * Copyright 2019 Collabora Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include <gtest/gtest.h>
      #include "virgl_context.h"
   #include "virgl_resource.h"
   #include "virgl_screen.h"
   #include "virgl_staging_mgr.h"
   #include "virgl_winsys.h"
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
      struct virgl_hw_res {
      struct pipe_reference reference;
   uint32_t target;
   uint32_t bind;
   uint32_t size;
      };
      static struct virgl_hw_res *
   fake_resource_create(struct virgl_winsys *vws,
                        enum pipe_texture_target target,
   const void *map_front_private,
   uint32_t format, uint32_t bind,
      {
                        hw_res->target = target;
   hw_res->bind = bind;
   hw_res->size = size;
               }
      static void
   fake_resource_reference(struct virgl_winsys *vws,
               {
               if (pipe_reference(&(*dres)->reference, &sres->reference)) {
      FREE(old->data);
                  }
      static void *
   fake_resource_map(struct virgl_winsys *vws, struct virgl_hw_res *hw_res)
   {
         }
      static struct pipe_context *
   fake_virgl_context_create()
   {
      struct virgl_context *vctx = CALLOC_STRUCT(virgl_context);
   struct virgl_screen *vs = CALLOC_STRUCT(virgl_screen);
            vctx->base.screen = &vs->base;
            vs->vws->resource_create = fake_resource_create;
   vs->vws->resource_reference = fake_resource_reference;
               }
      static void
   fake_virgl_context_destroy(struct pipe_context *ctx)
   {
      struct virgl_context *vctx = virgl_context(ctx);
            FREE(vs->vws);
   FREE(vs);
      }
      static void *
   resource_map(struct virgl_hw_res *hw_res)
   {
         }
      static void
   release_resources(struct virgl_hw_res *resources[], unsigned len)
   {
      for (unsigned i = 0; i < len; ++i)
      }
      class VirglStagingMgr : public ::testing::Test
   {
   protected:
      VirglStagingMgr() : ctx(fake_virgl_context_create())
   {
                  ~VirglStagingMgr()
   {
      virgl_staging_destroy(&staging);
               static const unsigned staging_size;
   struct pipe_context * const ctx;
      };
      const unsigned VirglStagingMgr::staging_size = 4096;
      class VirglStagingMgrWithAlignment : public VirglStagingMgr,
         {
   protected:
      VirglStagingMgrWithAlignment() : alignment(GetParam()) {}
      };
      TEST_P(VirglStagingMgrWithAlignment,
         {
      const unsigned alloc_sizes[] = {16, 450, 79, 240, 128, 1001};
   const unsigned num_resources = sizeof(alloc_sizes) / sizeof(alloc_sizes[0]);
   struct virgl_hw_res *out_resource[num_resources] = {0};
   unsigned expected_offset = 0;
   unsigned out_offset;
   void *map_ptr;
            for (unsigned i = 0; i < num_resources; ++i) {
      alloc_succeeded =
                  EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, expected_offset);
   ASSERT_NE(out_resource[i], nullptr);
   if (i > 0) {
         }
   EXPECT_EQ(map_ptr,
            expected_offset += alloc_sizes[i];
                  }
      INSTANTIATE_TEST_SUITE_P(
      WithAlignment,
   VirglStagingMgrWithAlignment,
   ::testing::Values(1, 16),
      );
      TEST_F(VirglStagingMgr,
         {
      struct virgl_hw_res *out_resource[2] = {0};
   unsigned out_offset;
   void *map_ptr;
            alloc_succeeded =
      virgl_staging_alloc(&staging, staging_size - 1, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, 0);
   ASSERT_NE(out_resource[0], nullptr);
            alloc_succeeded =
      virgl_staging_alloc(&staging, 2, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, 0);
   ASSERT_NE(out_resource[1], nullptr);
   EXPECT_EQ(map_ptr, resource_map(out_resource[1]));
   /* New resource with same size as old resource. */
   EXPECT_NE(out_resource[1], out_resource[0]);
               }
      TEST_F(VirglStagingMgr,
         {
      struct virgl_hw_res *out_resource[2] = {0};
   unsigned out_offset;
   void *map_ptr;
            alloc_succeeded =
      virgl_staging_alloc(&staging, staging_size - 1, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, 0);
   ASSERT_NE(out_resource[0], nullptr);
            alloc_succeeded =
      virgl_staging_alloc(&staging, 1, 16, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, 0);
   ASSERT_NE(out_resource[1], nullptr);
   EXPECT_EQ(map_ptr, resource_map(out_resource[1]));
   /* New resource with same size as old resource. */
   EXPECT_NE(out_resource[1], out_resource[0]);
               }
      TEST_F(VirglStagingMgr,
         {
      struct virgl_hw_res *out_resource[2] = {0};
   unsigned out_offset;
   void *map_ptr;
                     alloc_succeeded =
      virgl_staging_alloc(&staging, 5123, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, 0);
   ASSERT_NE(out_resource[0], nullptr);
   EXPECT_EQ(map_ptr, resource_map(out_resource[0]));
            alloc_succeeded =
      virgl_staging_alloc(&staging, 19345, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   EXPECT_EQ(out_offset, 0);
   ASSERT_NE(out_resource[1], nullptr);
   EXPECT_EQ(map_ptr, resource_map(out_resource[1]));
   /* New resource */
   EXPECT_NE(out_resource[1], out_resource[0]);
               }
      TEST_F(VirglStagingMgr, releases_resource_on_destruction)
   {
      struct virgl_hw_res *out_resource = NULL;
   unsigned out_offset;
   void *map_ptr;
            alloc_succeeded =
      virgl_staging_alloc(&staging, 128, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   ASSERT_NE(out_resource, nullptr);
   /* The resource is referenced both by staging internally,
   * and out_resource.
   */
            /* Destroying staging releases the internal reference. */
   virgl_staging_destroy(&staging);
               }
      static struct virgl_hw_res *
   failing_resource_create(struct virgl_winsys *vws,
                           enum pipe_texture_target target,
   const void *map_front_private,
   uint32_t format, uint32_t bind,
   {
         }
      TEST_F(VirglStagingMgr, fails_gracefully_if_resource_create_fails)
   {
      struct virgl_screen *vs = virgl_screen(ctx->screen);
   struct virgl_hw_res *out_resource = NULL;
   unsigned out_offset;
   void *map_ptr;
                     alloc_succeeded =
      virgl_staging_alloc(&staging, 128, 1, &out_offset,
         EXPECT_FALSE(alloc_succeeded);
   EXPECT_EQ(out_resource, nullptr);
      }
      static void *
   failing_resource_map(struct virgl_winsys *vws, struct virgl_hw_res *hw_res)
   {
         }
      TEST_F(VirglStagingMgr, fails_gracefully_if_map_fails)
   {
      struct virgl_screen *vs = virgl_screen(ctx->screen);
   struct virgl_hw_res *out_resource = NULL;
   unsigned out_offset;
   void *map_ptr;
                     alloc_succeeded =
      virgl_staging_alloc(&staging, 128, 1, &out_offset,
         EXPECT_FALSE(alloc_succeeded);
   EXPECT_EQ(out_resource, nullptr);
      }
      TEST_F(VirglStagingMgr, uses_staging_buffer_resource)
   {
      struct virgl_hw_res *out_resource = NULL;
   unsigned out_offset;
   void *map_ptr;
            alloc_succeeded =
      virgl_staging_alloc(&staging, 128, 1, &out_offset,
         EXPECT_TRUE(alloc_succeeded);
   ASSERT_NE(out_resource, nullptr);
   EXPECT_EQ(out_resource->target, PIPE_BUFFER);
               }
