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
      /* A collection of unit tests for cache.c */
      #include <gtest/gtest.h>
      #include <stdio.h>
   #include <stdlib.h>
   #include <stdbool.h>
   #include <string.h>
   #include <ftw.h>
   #include <errno.h>
   #include <stdarg.h>
   #include <inttypes.h>
   #include <limits.h>
   #include <time.h>
   #include <unistd.h>
      #include "util/mesa-sha1.h"
   #include "util/disk_cache.h"
   #include "util/disk_cache_os.h"
   #include "util/ralloc.h"
      #ifdef ENABLE_SHADER_CACHE
      /* Callback for nftw used in rmrf_local below.
   */
   static int
   remove_entry(const char *path,
               const struct stat *sb,
   {
               if (err)
               }
      /* Recursively remove a directory.
   *
   * This is equivalent to "rm -rf <dir>" with one bit of protection
   * that the directory name must begin with "." to ensure we don't
   * wander around deleting more than intended.
   *
   * Returns 0 on success, -1 on any error.
   */
   static int
   rmrf_local(const char *path)
   {
      if (path == NULL || *path == '\0' || *path != '.')
               }
      static void
   check_directories_created(void *mem_ctx, const char *cache_dir)
   {
               char buf[PATH_MAX];
   if (getcwd(buf, PATH_MAX)) {
      char *full_path = ralloc_asprintf(mem_ctx, "%s%s", buf, ++cache_dir);
   struct stat sb;
   if (stat(full_path, &sb) != -1 && S_ISDIR(sb.st_mode))
                  }
      static bool
   does_cache_contain(struct disk_cache *cache, const cache_key key)
   {
                        if (result) {
      free(result);
                  }
      static bool
   cache_exists(struct disk_cache *cache)
   {
      uint8_t key[20];
            if (!cache)
            disk_cache_compute_key(cache, data, sizeof(data), key);
   disk_cache_put(cache, key, data, sizeof(data), NULL);
   disk_cache_wait_for_idle(cache);
   void *result = disk_cache_get(cache, key, NULL);
            free(result);
      }
      static void *
   poll_disk_cache_get(struct disk_cache *cache,
               {
               for (int iter = 0; iter < 1000; ++iter) {
      result = disk_cache_get(cache, key, size);
   if (result)
                           }
      #define CACHE_TEST_TMP "./cache-test-tmp"
      static void
   test_disk_cache_create(void *mem_ctx, const char *cache_dir_name,
         {
      struct disk_cache *cache;
            /* Before doing anything else, ensure that with
   * MESA_SHADER_CACHE_DISABLE set to true, that disk_cache_create returns NO-OP cache.
   */
   setenv("MESA_SHADER_CACHE_DISABLE", "true", 1);
   cache = disk_cache_create("test", driver_id, 0);
   EXPECT_EQ(cache->type, DISK_CACHE_NONE) << "disk_cache_create with MESA_SHADER_CACHE_DISABLE set";
                  #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
      /* With SHADER_CACHE_DISABLE_BY_DEFAULT, ensure that with
   * MESA_SHADER_CACHE_DISABLE set to nothing, disk_cache_create returns NO-OP cache.
   */
   unsetenv("MESA_SHADER_CACHE_DISABLE");
   cache = disk_cache_create("test", driver_id, 0);
   EXPECT_EQ(cache->type, DISK_CACHE_NONE)
      << "disk_cache_create with MESA_SHADER_CACHE_DISABLE unset "
               /* For remaining tests, ensure that the cache is enabled. */
      #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
         /* For the first real disk_cache_create() clear these environment
   * variables to test creation of cache in home directory.
   */
   unsetenv("MESA_SHADER_CACHE_DIR");
            cache = disk_cache_create("test", driver_id, 0);
                  #ifdef ANDROID
      /* Android doesn't try writing to disk (just calls the cache callbacks), so
   * the directory tests below don't apply.
   */
      #endif
         /* Test with XDG_CACHE_HOME set */
   setenv("XDG_CACHE_HOME", CACHE_TEST_TMP "/xdg-cache-home", 1);
   cache = disk_cache_create("test", driver_id, 0);
   EXPECT_FALSE(cache_exists(cache))
            err = mkdir(CACHE_TEST_TMP, 0755);
   if (err != 0) {
      fprintf(stderr, "Error creating %s: %s\n", CACHE_TEST_TMP, strerror(errno));
      }
            cache = disk_cache_create("test", driver_id, 0);
   EXPECT_TRUE(cache_exists(cache))
            char *path = ralloc_asprintf(
                           /* Test with MESA_SHADER_CACHE_DIR set */
   err = rmrf_local(CACHE_TEST_TMP);
            setenv("MESA_SHADER_CACHE_DIR", CACHE_TEST_TMP "/mesa-shader-cache-dir", 1);
   cache = disk_cache_create("test", driver_id, 0);
   EXPECT_FALSE(cache_exists(cache))
            err = mkdir(CACHE_TEST_TMP, 0755);
   if (err != 0) {
      fprintf(stderr, "Error creating %s: %s\n", CACHE_TEST_TMP, strerror(errno));
      }
            cache = disk_cache_create("test", driver_id, 0);
            path = ralloc_asprintf(
                     }
      static void
   test_put_and_get(bool test_cache_size_limit, const char *driver_id)
   {
      struct disk_cache *cache;
   char blob[] = "This is a blob of thirty-seven bytes";
   uint8_t blob_key[20];
   char string[] = "While this string has thirty-four";
   uint8_t string_key[20];
   char *result;
   size_t size;
   uint8_t *one_KB, *one_MB;
   uint8_t one_KB_key[20], one_MB_key[20];
         #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
                           /* Ensure that disk_cache_get returns nothing before anything is added. */
   result = (char *) disk_cache_get(cache, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Simple test of put and get. */
            /* disk_cache_put() hands things off to a thread so wait for it. */
            result = (char *) disk_cache_get(cache, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
                     /* Test put and get of a second item. */
   disk_cache_compute_key(cache, string, sizeof(string), string_key);
            /* disk_cache_put() hands things off to a thread so wait for it. */
            result = (char *) disk_cache_get(cache, string_key, &size);
   EXPECT_STREQ(result, string) << "2nd disk_cache_get of existing item (pointer)";
                     /* Set the cache size to 1KB and add a 1KB item to force an eviction. */
            if (!test_cache_size_limit)
            setenv("MESA_SHADER_CACHE_MAX_SIZE", "1K", 1);
                     /* Obviously the SHA-1 hash of 1024 zero bytes isn't particularly
   * interesting. But we do have want to take some special care with
   * the hash we use here. The issue is that in this artificial case,
   * (with only three files in the cache), the probability is good
   * that each of the three files will end up in their own
   * directory. Then, if the directory containing the .tmp file for
   * the new item being added for disk_cache_put() is the chosen victim
   * directory for eviction, then no suitable file will be found and
   * nothing will be evicted.
   *
   * That's actually expected given how the eviction code is
   * implemented, (which expects to only evict once things are more
   * interestingly full than that).
   *
   * For this test, we force this signature to land in the same
   * directory as the original blob first written to the cache.
   */
   disk_cache_compute_key(cache, one_KB, 1024, one_KB_key);
                              /* disk_cache_put() hands things off to a thread so wait for it. */
            result = (char *) disk_cache_get(cache, one_KB_key, &size);
   EXPECT_NE(result, nullptr) << "3rd disk_cache_get of existing item (pointer)";
                     /* Ensure eviction happened by checking that both of the previous
   * cache itesm were evicted.
   */
   bool contains_1KB_file = false;
   count = 0;
   if (does_cache_contain(cache, blob_key))
            if (does_cache_contain(cache, string_key))
            if (does_cache_contain(cache, one_KB_key)) {
      count++;
               EXPECT_TRUE(contains_1KB_file)
                  /* Now increase the size to 1M, add back both items, and ensure all
   * three that have been added are available via disk_cache_get.
   */
            setenv("MESA_SHADER_CACHE_MAX_SIZE", "1M", 1);
            disk_cache_put(cache, blob_key, blob, sizeof(blob), NULL);
            /* disk_cache_put() hands things off to a thread so wait for it. */
            count = 0;
   if (does_cache_contain(cache, blob_key))
            if (does_cache_contain(cache, string_key))
            if (does_cache_contain(cache, one_KB_key))
                     /* Finally, check eviction again after adding an object of size 1M. */
            disk_cache_compute_key(cache, one_MB, 1024 * 1024, one_MB_key);
                              /* disk_cache_put() hands things off to a thread so wait for it. */
            bool contains_1MB_file = false;
   count = 0;
   if (does_cache_contain(cache, blob_key))
            if (does_cache_contain(cache, string_key))
            if (does_cache_contain(cache, one_KB_key))
            if (does_cache_contain(cache, one_MB_key)) {
      count++;
               EXPECT_TRUE(contains_1MB_file)
                     }
      static void
   test_put_key_and_get_key(const char *driver_id)
   {
      struct disk_cache *cache;
            uint8_t key_a[20] = {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
         uint8_t key_b[20] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
         uint8_t key_a_collide[20] =
               #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
                  /* First test that disk_cache_has_key returns false before disk_cache_put_key */
   result = disk_cache_has_key(cache, key_a);
            /* Then a couple of tests of disk_cache_put_key followed by disk_cache_has_key */
   disk_cache_put_key(cache, key_a);
   result = disk_cache_has_key(cache, key_a);
            disk_cache_put_key(cache, key_b);
   result = disk_cache_has_key(cache, key_b);
            /* Test that a key with the same two bytes as an existing key
   * forces an eviction.
   */
   disk_cache_put_key(cache, key_a_collide);
   result = disk_cache_has_key(cache, key_a_collide);
            result = disk_cache_has_key(cache, key_a);
            /* And finally test that we can re-add the original key to re-evict
   * the colliding key.
   */
   disk_cache_put_key(cache, key_a);
   result = disk_cache_has_key(cache, key_a);
            result = disk_cache_has_key(cache, key_a_collide);
               }
      /* To make sure we are not just using the inmemory cache index for the single
   * file cache we test adding and retriving cache items between two different
   * cache instances.
   */
   static void
   test_put_and_get_between_instances(const char *driver_id)
   {
      char blob[] = "This is a blob of thirty-seven bytes";
   uint8_t blob_key[20];
   char string[] = "While this string has thirty-four";
   uint8_t string_key[20];
   char *result;
         #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
         struct disk_cache *cache1 = disk_cache_create("test_between_instances",
         struct disk_cache *cache2 = disk_cache_create("test_between_instances",
                     /* Ensure that disk_cache_get returns nothing before anything is added. */
   result = (char *) disk_cache_get(cache1, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get(cache1) with non-existent item (pointer)";
            result = (char *) disk_cache_get(cache2, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get(cache2) with non-existent item (pointer)";
            /* Simple test of put and get. */
            /* disk_cache_put() hands things off to a thread so wait for it. */
            result = (char *) disk_cache_get(cache2, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get(cache2) of existing item (pointer)";
                     /* Test put and get of a second item, via the opposite instances */
   disk_cache_compute_key(cache2, string, sizeof(string), string_key);
            /* disk_cache_put() hands things off to a thread so wait for it. */
            result = (char *) disk_cache_get(cache1, string_key, &size);
   EXPECT_STREQ(result, string) << "2nd disk_cache_get(cache1) of existing item (pointer)";
                     disk_cache_destroy(cache1);
      }
      static void
   test_put_and_get_between_instances_with_eviction(const char *driver_id)
   {
      cache_key small_key[8], small_key2, big_key[2];
   struct disk_cache *cache[2];
   unsigned int i, n, k;
   uint8_t *small;
   uint8_t *big;
   char *result;
         #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
                  cache[0] = disk_cache_create("test_between_instances_with_eviction", driver_id, 0);
            uint8_t two_KB[2048] = { 0 };
            /* Flush the database by adding the dummy 2KB entry */
   disk_cache_put(cache[0], two_KB_key, two_KB, sizeof(two_KB), NULL);
            int size_big = 1000;
   size_big -= sizeof(struct cache_entry_file_data);
   size_big -= mesa_cache_db_file_entry_size();
   size_big -= cache[0]->driver_keys_blob_size;
            for (i = 0; i < ARRAY_SIZE(big_key); i++) {
      big = (uint8_t *) malloc(size_big);
            disk_cache_compute_key(cache[0], big, size_big, big_key[i]);
   disk_cache_put(cache[0], big_key[i], big, size_big, NULL);
            result = (char *) disk_cache_get(cache[0], big_key[i], &size);
   EXPECT_NE(result, nullptr) << "disk_cache_get with existent item (pointer)";
   EXPECT_EQ(size, size_big) << "disk_cache_get with existent item (size)";
                        int size_small = 256;
   size_small -= sizeof(struct cache_entry_file_data);
   size_small -= mesa_cache_db_file_entry_size();
   size_small -= cache[1]->driver_keys_blob_size;
            for (i = 0; i < ARRAY_SIZE(small_key); i++) {
      small = (uint8_t *) malloc(size_small);
            disk_cache_compute_key(cache[1], small, size_small, small_key[i]);
   disk_cache_put(cache[1], small_key[i], small, size_small, NULL);
            /*
   * At first we added two 1000KB entries to cache[0]. Now, when first
   * 256KB entry is added, the two 1000KB entries are evicted because
   * at minimum cache_max_size/2 is evicted on overflow.
   *
   * All four 256KB entries stay in the cache.
   */
   for (k = 0; k < ARRAY_SIZE(cache); k++) {
      for (n = 0; n <= i; n++) {
      result = (char *) disk_cache_get(cache[k], big_key[0], &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
                  result = (char *) disk_cache_get(cache[k], big_key[1], &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
                  result = (char *) disk_cache_get(cache[k], small_key[n], &size);
   EXPECT_NE(result, nullptr) << "disk_cache_get of existing item (pointer)";
                  result = (char *) disk_cache_get(cache[k], small_key[n], &size);
   EXPECT_NE(result, nullptr) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, size_small) << "disk_cache_get of existing item (size)";
                              small = (uint8_t *) malloc(size_small);
            /* Add another 256KB entry. This will evict first five 256KB entries
   * of eight that we added previously. */
   disk_cache_compute_key(cache[0], small, size_small, small_key2);
   disk_cache_put(cache[0], small_key2, small, size_small, NULL);
                     for (k = 0; k < ARRAY_SIZE(cache); k++) {
      result = (char *) disk_cache_get(cache[k], small_key2, &size);
   EXPECT_NE(result, nullptr) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, size_small) << "disk_cache_get of existing item (size)";
               for (i = 0, k = 0; k < ARRAY_SIZE(cache); k++) {
      for (n = 0; n < ARRAY_SIZE(small_key); n++) {
      result = (char *) disk_cache_get(cache[k], small_key[n], &size);
   if (!result)
                                 disk_cache_destroy(cache[0]);
      }
   #endif /* ENABLE_SHADER_CACHE */
      class Cache : public ::testing::Test {
   protected:
               Cache() {
         }
   ~Cache() {
            };
      TEST_F(Cache, MultiFile)
   {
            #ifndef ENABLE_SHADER_CACHE
         #else
            run_tests:
      if (!compress)
         else
                                       int err = rmrf_local(CACHE_TEST_TMP);
            if (compress) {
      compress = false;
         #endif
   }
      TEST_F(Cache, SingleFile)
   {
            #ifndef ENABLE_SHADER_CACHE
         #else
            run_tests:
               if (!compress)
         else
                     /* We skip testing cache size limit as the single file cache currently
   * doesn't have any functionality to enforce cache size limits.
   */
                                       int err = rmrf_local(CACHE_TEST_TMP);
            if (compress) {
      compress = false;
         #endif
   }
      TEST_F(Cache, Database)
   {
            #ifndef ENABLE_SHADER_CACHE
         #else
      setenv("MESA_DISK_CACHE_DATABASE_NUM_PARTS", "1", 1);
                     /* We skip testing cache size limit as the single file cache compresses
   * data much better than the multi-file cache, which results in the
   * failing tests of the cache eviction function. We we will test the
   * eviction separately with the disabled compression.
   */
            int err = rmrf_local(CACHE_TEST_TMP);
                                                         setenv("MESA_DISK_CACHE_DATABASE", "false", 1);
            err = rmrf_local(CACHE_TEST_TMP);
      #endif
   }
      TEST_F(Cache, Combined)
   {
      const char *driver_id = "make_check";
   char blob[] = "This is a RO blob";
   char blob2[] = "This is a RW blob";
   uint8_t dummy_key[20] = { 0 };
   uint8_t blob_key[20];
   uint8_t blob_key2[20];
   char foz_rw_idx_file[1024];
   char foz_ro_idx_file[1024];
   char foz_rw_file[1024];
   char foz_ro_file[1024];
   char *result;
         #ifndef ENABLE_SHADER_CACHE
         #else
      setenv("MESA_DISK_CACHE_SINGLE_FILE", "true", 1);
         #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
         /* Enable Fossilize read-write cache. */
                     /* Create Fossilize writable cache. */
   struct disk_cache *cache_sf_wr = disk_cache_create("combined_test",
            disk_cache_compute_key(cache_sf_wr, blob, sizeof(blob), blob_key);
            /* Ensure that disk_cache_get returns nothing before anything is added. */
   result = (char *) disk_cache_get(cache_sf_wr, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Put blob entry to the cache. */
   disk_cache_put(cache_sf_wr, blob_key, blob, sizeof(blob), NULL);
            result = (char *) disk_cache_get(cache_sf_wr, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            /* Rename file foz_cache.foz -> ro_cache.foz */
   sprintf(foz_rw_file, "%s/foz_cache.foz", cache_sf_wr->path);
   sprintf(foz_ro_file, "%s/ro_cache.foz", cache_sf_wr->path);
            /* Rename file foz_cache_idx.foz -> ro_cache_idx.foz */
   sprintf(foz_rw_idx_file, "%s/foz_cache_idx.foz", cache_sf_wr->path);
   sprintf(foz_ro_idx_file, "%s/ro_cache_idx.foz", cache_sf_wr->path);
                     /* Disable Fossilize read-write cache. */
            /* Set up Fossilize read-only cache. */
   setenv("MESA_DISK_CACHE_COMBINE_RW_WITH_RO_FOZ", "true", 1);
            /* Create FOZ cache that fetches the RO cache. Note that this produces
   * empty RW cache files. */
   struct disk_cache *cache_sf_ro = disk_cache_create("combined_test",
            /* Blob entry must present because it shall be retrieved from the
   * ro_cache.foz */
   result = (char *) disk_cache_get(cache_sf_ro, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
                     /* Remove empty FOZ RW cache files created above. We only need RO cache. */
   EXPECT_EQ(unlink(foz_rw_file), 0);
            setenv("MESA_DISK_CACHE_SINGLE_FILE", "false", 1);
            /* Create MESA-DB cache with enabled retrieval from the read-only
   * cache. */
   struct disk_cache *cache_mesa_db = disk_cache_create("combined_test",
            /* Dummy entry must not present in any of the caches. Foz cache
   * reloads index if cache entry is missing.  This is a sanity-check
   * for foz_read_entry(), it should work properly with a disabled
   * FOZ RW cache. */
   result = (char *) disk_cache_get(cache_mesa_db, dummy_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Blob entry must present because it shall be retrieved from the
   * read-only cache. */
   result = (char *) disk_cache_get(cache_mesa_db, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            /* Blob2 entry must not present in any of the caches. */
   result = (char *) disk_cache_get(cache_mesa_db, blob_key2, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Put blob2 entry to the cache. */
   disk_cache_put(cache_mesa_db, blob_key2, blob2, sizeof(blob2), NULL);
            /* Blob2 entry must present because it shall be retrieved from the
   * read-write cache. */
   result = (char *) disk_cache_get(cache_mesa_db, blob_key2, &size);
   EXPECT_STREQ(blob2, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob2)) << "disk_cache_get of existing item (size)";
                     /* Disable read-only cache. */
            /* Create MESA-DB cache with disabled retrieval from the
   * read-only cache. */
            /* Blob2 entry must present because it shall be retrieved from the
   * MESA-DB cache. */
   result = (char *) disk_cache_get(cache_mesa_db, blob_key2, &size);
   EXPECT_STREQ(blob2, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob2)) << "disk_cache_get of existing item (size)";
                     /* Create MESA-DB cache with disabled retrieval from the read-only
   * cache. */
            /* Blob entry must not present in the cache because we disable the
   * read-only cache. */
   result = (char *) disk_cache_get(cache_mesa_db, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
                     /* Create default multi-file cache. */
            /* Enable read-only cache. */
            /* Create multi-file cache with enabled retrieval from the
   * read-only cache. */
   struct disk_cache *cache_multifile = disk_cache_create("combined_test",
            /* Blob entry must present because it shall be retrieved from the
   * read-only cache. */
   result = (char *) disk_cache_get(cache_multifile, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            /* Blob2 entry must not present in any of the caches. */
   result = (char *) disk_cache_get(cache_multifile, blob_key2, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Put blob2 entry to the cache. */
   disk_cache_put(cache_multifile, blob_key2, blob2, sizeof(blob2), NULL);
            /* Blob2 entry must present because it shall be retrieved from the
   * read-write cache. */
   result = (char *) disk_cache_get(cache_multifile, blob_key2, &size);
   EXPECT_STREQ(blob2, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob2)) << "disk_cache_get of existing item (size)";
                     /* Disable read-only cache. */
   setenv("MESA_DISK_CACHE_COMBINE_RW_WITH_RO_FOZ", "false", 1);
            /* Create multi-file cache with disabled retrieval from the
   * read-only cache. */
            /* Blob entry must not present in the cache because we disabled the
   * read-only cache. */
   result = (char *) disk_cache_get(cache_multifile, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Blob2 entry must present because it shall be retrieved from the
   * read-write cache. */
   result = (char *) disk_cache_get(cache_multifile, blob_key2, &size);
   EXPECT_STREQ(blob2, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob2)) << "disk_cache_get of existing item (size)";
                     int err = rmrf_local(CACHE_TEST_TMP);
      #endif
   }
      TEST_F(Cache, DISABLED_List)
   {
      const char *driver_id = "make_check";
   char blob[] = "This is a RO blob";
   uint8_t blob_key[20];
   char foz_rw_idx_file[1024];
   char foz_ro_idx_file[1024];
   char foz_rw_file[1024];
   char foz_ro_file[1024];
   char *result;
         #ifndef ENABLE_SHADER_CACHE
         #else
   #ifndef FOZ_DB_UTIL_DYNAMIC_LIST
         #else
            #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
                  /* Create ro files for testing */
   /* Create Fossilize writable cache. */
   struct disk_cache *cache_sf_wr =
                     /* Ensure that disk_cache_get returns nothing before anything is added. */
   result = (char *)disk_cache_get(cache_sf_wr, blob_key, &size);
   EXPECT_EQ(result, nullptr)
                  /* Put blob entry to the cache. */
   disk_cache_put(cache_sf_wr, blob_key, blob, sizeof(blob), NULL);
            result = (char *)disk_cache_get(cache_sf_wr, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            /* Rename file foz_cache.foz -> ro_cache.foz */
   sprintf(foz_rw_file, "%s/foz_cache.foz", cache_sf_wr->path);
   sprintf(foz_ro_file, "%s/ro_cache.foz", cache_sf_wr->path);
   EXPECT_EQ(rename(foz_rw_file, foz_ro_file), 0)
            /* Rename file foz_cache_idx.foz -> ro_cache_idx.foz */
   sprintf(foz_rw_idx_file, "%s/foz_cache_idx.foz", cache_sf_wr->path);
   sprintf(foz_ro_idx_file, "%s/ro_cache_idx.foz", cache_sf_wr->path);
   EXPECT_EQ(rename(foz_rw_idx_file, foz_ro_idx_file), 0)
                     const char *list_filename = CACHE_TEST_TMP "/foz_dbs_list.txt";
            /* Create new empty file */
   FILE *list_file = fopen(list_filename, "w");
   fputs("ro_cache\n", list_file);
            /* Create Fossilize writable cache. */
            /* Blob entry must present because it shall be retrieved from the
   * ro_cache.foz loaded from list at creation time */
   result = (char *)disk_cache_get(cache_sf, blob_key, &size);
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            disk_cache_destroy(cache_sf);
            /* Test loading from a list populated at runtime */
   /* Create new empty file */
   list_file = fopen(list_filename, "w");
            /* Create Fossilize writable cache. */
            /* Ensure that disk_cache returns nothing before list file is populated */
   result = (char *)disk_cache_get(cache_sf, blob_key, &size);
   EXPECT_EQ(result, nullptr)
                  /* Add ro_cache to list file for loading */
   list_file = fopen(list_filename, "a");
   fputs("ro_cache\n", list_file);
            /* Poll result to give time for updater to load ro cache */
            /* Blob entry must present because it shall be retrieved from the
   * ro_cache.foz loaded from list at runtime */
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            disk_cache_destroy(cache_sf);
            /* Test loading from a list with some invalid files */
   /* Create new empty file */
   list_file = fopen(list_filename, "w");
            /* Create Fossilize writable cache. */
            /* Ensure that disk_cache returns nothing before list file is populated */
   result = (char *)disk_cache_get(cache_sf, blob_key, &size);
   EXPECT_EQ(result, nullptr)
                  /* Add non-existant list files for loading */
   list_file = fopen(list_filename, "a");
   fputs("no_cache\n", list_file);
   fputs("no_cache2\n", list_file);
   fputs("no_cache/no_cache3\n", list_file);
   /* Add ro_cache to list file for loading */
   fputs("ro_cache\n", list_file);
            /* Poll result to give time for updater to load ro cache */
            /* Blob entry must present because it shall be retrieved from the
   * ro_cache.foz loaded from list at runtime despite invalid files
   * in the list */
   EXPECT_STREQ(blob, result) << "disk_cache_get of existing item (pointer)";
   EXPECT_EQ(size, sizeof(blob)) << "disk_cache_get of existing item (size)";
            disk_cache_destroy(cache_sf);
            int err = rmrf_local(CACHE_TEST_TMP);
            unsetenv("MESA_DISK_CACHE_SINGLE_FILE");
      #endif /* FOZ_DB_UTIL_DYNAMIC_LIST */
   #endif /* ENABLE_SHADER_CACHE */
   }
      static void
   test_multipart_eviction(const char *driver_id)
   {
      const unsigned int entry_size = 512;
   uint8_t blobs[7][entry_size];
   cache_key keys[7];
   unsigned int i;
   char *result;
            setenv("MESA_SHADER_CACHE_MAX_SIZE", "3K", 1);
                     unsigned int entry_file_size = entry_size;
   entry_file_size -= sizeof(struct cache_entry_file_data);
   entry_file_size -= mesa_cache_db_file_entry_size();
   entry_file_size -= cache->driver_keys_blob_size;
            /*
   * 1. Allocate 3KB cache in 3 parts, each part is 1KB
   * 2. Fill up cache with six 512K entries
   * 3. Touch entries of the first part, which will bump last_access_time
   *    of the first two cache entries
   * 4. Insert seventh 512K entry that will cause eviction of the second part
   * 5. Check that second entry of the second part gone due to eviction and
   *    others present
            /* Fill up cache with six 512K entries. */
   for (i = 0; i < 6; i++) {
               disk_cache_compute_key(cache,  blobs[i], entry_file_size, keys[i]);
   disk_cache_put(cache, keys[i], blobs[i], entry_file_size, NULL);
            result = (char *) disk_cache_get(cache, keys[i], &size);
   EXPECT_NE(result, nullptr) << "disk_cache_get with existent item (pointer)";
   EXPECT_EQ(size, entry_file_size) << "disk_cache_get with existent item (size)";
            /* Ensure that cache entries will have distinct last_access_time
   * during testing.
   */
   if (i % 2 == 0)
               /* Touch entries of the first part. Second part becomes outdated */
   for (i = 0; i < 2; i++) {
      result = (char *) disk_cache_get(cache, keys[i], &size);
   EXPECT_NE(result, nullptr) << "disk_cache_get with existent item (pointer)";
   EXPECT_EQ(size, entry_file_size) << "disk_cache_get with existent item (size)";
               /* Insert seventh entry. */
   memset(blobs[6], 6, entry_file_size);
   disk_cache_compute_key(cache,  blobs[6], entry_file_size, keys[6]);
   disk_cache_put(cache, keys[6], blobs[6], entry_file_size, NULL);
            /* Check whether third entry of the second part gone and others present. */
   for (i = 0; i < ARRAY_SIZE(blobs); i++) {
      result = (char *) disk_cache_get(cache, keys[i], &size);
   if (i == 2) {
         } else {
      EXPECT_NE(result, nullptr) << "disk_cache_get with existent item (pointer)";
      }
                  }
      TEST_F(Cache, DatabaseMultipartEviction)
   {
            #ifndef ENABLE_SHADER_CACHE
         #else
      setenv("MESA_DISK_CACHE_DATABASE_NUM_PARTS", "3", 1);
                              unsetenv("MESA_DISK_CACHE_DATABASE_NUM_PARTS");
            int err = rmrf_local(CACHE_TEST_TMP);
      #endif
   }
      static void
   test_put_and_get_disabled(const char *driver_id)
   {
      struct disk_cache *cache;
   char blob[] = "This is a blob of thirty-seven bytes";
   uint8_t blob_key[20];
   char *result;
                              /* Ensure that disk_cache_get returns nothing before anything is added. */
   result = (char *) disk_cache_get(cache, blob_key, &size);
   EXPECT_EQ(result, nullptr) << "disk_cache_get with non-existent item (pointer)";
            /* Simple test of put and get. */
            /* disk_cache_put() hands things off to a thread so wait for it. */
            result = (char *) disk_cache_get(cache, blob_key, &size);
   EXPECT_STREQ(result, nullptr) << "disk_cache_get of existing item (pointer)";
               }
      TEST_F(Cache, Disabled)
   {
            #ifndef ENABLE_SHADER_CACHE
         #else
            #ifdef SHADER_CACHE_DISABLE_BY_DEFAULT
         #endif /* SHADER_CACHE_DISABLE_BY_DEFAULT */
                                             setenv("MESA_SHADER_CACHE_DISABLE", "false", 1);
            int err = rmrf_local(CACHE_TEST_TMP);
      #endif
   }
