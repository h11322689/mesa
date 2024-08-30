   /*
   * Copyright © 2020 Valve Corporation
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
      /* This is a basic c implementation of a fossilize db like format intended for
   * use with the Mesa shader cache.
   *
   * The format is compatible enough to allow the fossilize db tools to be used
   * to do things like merge db collections.
   */
      #include "fossilize_db.h"
      #ifdef FOZ_DB_UTIL
      #include <assert.h>
   #include <stddef.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/file.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <unistd.h>
      #ifdef FOZ_DB_UTIL_DYNAMIC_LIST
   #include <sys/inotify.h>
   #endif
      #include "util/u_debug.h"
      #include "crc32.h"
   #include "hash_table.h"
   #include "mesa-sha1.h"
   #include "ralloc.h"
      #define FOZ_REF_MAGIC_SIZE 16
      static const uint8_t stream_reference_magic_and_version[FOZ_REF_MAGIC_SIZE] = {
      0x81, 'F', 'O', 'S',
   'S', 'I', 'L', 'I',
   'Z', 'E', 'D', 'B',
      };
      /* Mesa uses 160bit hashes to identify cache entries, a hash of this size
   * makes collisions virtually impossible for our use case. However the foz db
   * format uses a 64bit hash table to lookup file offsets for reading cache
   * entries so we must shorten our hash.
   */
   static uint64_t
   truncate_hash_to_64bits(const uint8_t *cache_key)
   {
      uint64_t hash = 0;
   unsigned shift = 7;
   for (unsigned i = 0; i < 8; i++) {
      hash |= ((uint64_t)cache_key[i]) << shift * 8;
      }
      }
      static bool
   check_files_opened_successfully(FILE *file, FILE *db_idx)
   {
      if (!file) {
      if (db_idx)
                     if (!db_idx) {
      if (file)
                        }
      static bool
   create_foz_db_filenames(const char *cache_path,
                     {
      if (asprintf(filename, "%s/%s.foz", cache_path, name) == -1)
            if (asprintf(idx_filename, "%s/%s_idx.foz", cache_path, name) == -1) {
      free(*filename);
                  }
         /* This looks at stuff that was added to the index since the last time we looked at it. This is safe
   * to do without locking the file as we assume the file is append only */
   static void
   update_foz_index(struct foz_db *foz_db, FILE *db_idx, unsigned file_idx)
   {
      uint64_t offset = ftell(db_idx);
   fseek(db_idx, 0, SEEK_END);
   uint64_t len = ftell(db_idx);
            if (offset == len)
            fseek(db_idx, offset, SEEK_SET);
   while (offset < len) {
      char bytes_to_read[FOSSILIZE_BLOB_HASH_LENGTH + sizeof(struct foz_payload_header)];
            /* Corrupt entry. Our process might have been killed before we
   * could write all data.
   */
   if (offset + sizeof(bytes_to_read) > len)
            /* NAME + HEADER in one read */
   if (fread(bytes_to_read, 1, sizeof(bytes_to_read), db_idx) !=
                  offset += sizeof(bytes_to_read);
            /* Corrupt entry. Our process might have been killed before we
   * could write all data.
   */
   if (offset + header->payload_size > len ||
                  char hash_str[FOSSILIZE_BLOB_HASH_LENGTH + 1] = {0};
            /* read cache item offset from index file */
   uint64_t cache_offset;
   if (fread(&cache_offset, 1, sizeof(cache_offset), db_idx) !=
                  offset += header->payload_size;
            struct foz_db_entry *entry = ralloc(foz_db->mem_ctx,
         entry->header = *header;
   entry->file_idx = file_idx;
            /* Truncate the entry's hash string to a 64bit hash for use with a
   * 64bit hash table for looking up file offsets.
   */
   hash_str[16] = '\0';
                                       }
      /* exclusive flock with timeout. timeout is in nanoseconds */
   static int lock_file_with_timeout(FILE *f, int64_t timeout)
   {
      int err;
   int fd = fileno(f);
            /* Since there is no blocking flock with timeout and we don't want to totally spin on getting the
   * lock, use a nonblocking method and retry every millisecond. */
   for (int64_t iter = 0; iter < iterations; ++iter) {
      err = flock(fd, LOCK_EX | LOCK_NB);
   if (err == 0 || errno != EAGAIN)
            }
      }
      static bool
   load_foz_dbs(struct foz_db *foz_db, FILE *db_idx, uint8_t file_idx,
         {
      /* Scan through the archive and get the list of cache entries. */
   fseek(db_idx, 0, SEEK_END);
   size_t len = ftell(db_idx);
            /* Try not to take the lock if len >= the size of the header, but if it is smaller we take the
   * lock to potentially initialize the files. */
   if (len < sizeof(stream_reference_magic_and_version)) {
      /* Wait for 100 ms in case of contention, after that we prioritize getting the app started. */
   int err = lock_file_with_timeout(foz_db->file[file_idx], 100000000);
   if (err == -1)
            /* Compute length again so we know nobody else did it in the meantime */
   fseek(db_idx, 0, SEEK_END);
   len = ftell(db_idx);
               if (len != 0) {
      uint8_t magic[FOZ_REF_MAGIC_SIZE];
   if (fread(magic, 1, FOZ_REF_MAGIC_SIZE, db_idx) != FOZ_REF_MAGIC_SIZE)
            if (memcmp(magic, stream_reference_magic_and_version,
                  int version = magic[FOZ_REF_MAGIC_SIZE - 1];
   if (version > FOSSILIZE_FORMAT_VERSION ||
               } else {
      /* Appending to a fresh file. Make sure we have the magic. */
   if (fwrite(stream_reference_magic_and_version, 1,
                        if (fwrite(stream_reference_magic_and_version, 1,
                        fflush(foz_db->file[file_idx]);
                        if (foz_db->updater.thrd) {
   /* If MESA_DISK_CACHE_READ_ONLY_FOZ_DBS_DYNAMIC_LIST is enabled, access to
   * the foz_db hash table requires locking to prevent racing between this
   * updated thread loading DBs at runtime and cache entry read/writes. */
      simple_mtx_lock(&foz_db->mtx);
   update_foz_index(foz_db, db_idx, file_idx);
      } else {
                  foz_db->alive = true;
         fail:
      flock(fileno(foz_db->file[file_idx]), LOCK_UN);
      }
      static void
   load_foz_dbs_ro(struct foz_db *foz_db, char *foz_dbs_ro)
   {
      uint8_t file_idx = 1;
   char *filename = NULL;
            for (unsigned n; n = strcspn(foz_dbs_ro, ","), *foz_dbs_ro;
      foz_dbs_ro += MAX2(1, n)) {
            filename = NULL;
   idx_filename = NULL;
   if (!create_foz_db_filenames(foz_db->cache_path, foz_db_filename,
            free(foz_db_filename);
      }
            /* Open files as read only */
   foz_db->file[file_idx] = fopen(filename, "rb");
            free(filename);
            if (!check_files_opened_successfully(foz_db->file[file_idx], db_idx)) {
                                 if (!load_foz_dbs(foz_db, db_idx, file_idx, true)) {
      fclose(db_idx);
                              fclose(db_idx);
            if (file_idx >= FOZ_MAX_DBS)
         }
      #ifdef FOZ_DB_UTIL_DYNAMIC_LIST
   static bool
   check_file_already_loaded(struct foz_db *foz_db,
               {
               if (fstat(fileno(db_file), &new_file_stat) == -1)
            for (int i = 0; i < max_file_idx; i++) {
               if (fstat(fileno(foz_db->file[i]), &loaded_file_stat) == -1)
            if ((loaded_file_stat.st_dev == new_file_stat.st_dev) &&
      (loaded_file_stat.st_ino == new_file_stat.st_ino))
               }
      static bool
   load_from_list_file(struct foz_db *foz_db, const char *foz_dbs_list_filename)
   {
      uint8_t file_idx;
            /* Find the first empty file idx slot */
   for (file_idx = 0; file_idx < FOZ_MAX_DBS; file_idx++) {
      if (!foz_db->file[file_idx])
               if (file_idx >= FOZ_MAX_DBS)
            FILE *foz_dbs_list_file = fopen(foz_dbs_list_filename, "rb");
   if (!foz_dbs_list_file)
            while (fgets(list_entry, sizeof(list_entry), foz_dbs_list_file)) {
      char *db_filename = NULL;
   char *idx_filename = NULL;
   FILE *db_file = NULL;
                     if (!create_foz_db_filenames(foz_db->cache_path, list_entry,
                  db_file = fopen(db_filename, "rb");
            free(db_filename);
            if (!check_files_opened_successfully(db_file, idx_file))
            if (check_file_already_loaded(foz_db, db_file, file_idx)) {
                                 /* Must be set before calling load_foz_dbs() */
            if (!load_foz_dbs(foz_db, idx_file, file_idx, true)) {
      fclose(db_file);
                              fclose(idx_file);
            if (file_idx >= FOZ_MAX_DBS)
               fclose(foz_dbs_list_file);
      }
      static int
   foz_dbs_list_updater_thrd(void *data)
   {
      char buf[10 * (sizeof(struct inotify_event) + NAME_MAX + 1)];
   struct foz_db *foz_db = data;
            while (1) {
               if (len == -1 && errno != EAGAIN)
            int i = 0;
   while (i < len) {
                                       /* List file deleted or watch removed by foz destroy */
   if ((event->mask & IN_DELETE_SELF) || (event->mask & IN_IGNORED))
                     }
      static bool
   foz_dbs_list_updater_init(struct foz_db *foz_db, char *list_filename)
   {
               /* Initial load */
   if (!load_from_list_file(foz_db, list_filename))
                     int fd = inotify_init1(IN_CLOEXEC);
   if (fd < 0)
            int wd = inotify_add_watch(fd, foz_db->updater.list_filename,
         if (wd < 0) {
      close(fd);
               updater->inotify_fd = fd;
            if (thrd_create(&updater->thrd, foz_dbs_list_updater_thrd, foz_db)) {
      inotify_rm_watch(fd, wd);
                           }
   #endif
      /* Here we open mesa cache foz dbs files. If the files exist we load the index
   * db into a hash table. The index db contains the offsets needed to later
   * read cache entries from the foz db containing the actual cache entries.
   */
   bool
   foz_prepare(struct foz_db *foz_db, char *cache_path)
   {
      char *filename = NULL;
            simple_mtx_init(&foz_db->mtx, mtx_plain);
   simple_mtx_init(&foz_db->flock_mtx, mtx_plain);
   foz_db->mem_ctx = ralloc_context(NULL);
   foz_db->index_db = _mesa_hash_table_u64_create(NULL);
            /* Open the default foz dbs for read/write. If the files didn't already exist
   * create them.
   */
   if (debug_get_bool_option("MESA_DISK_CACHE_SINGLE_FILE", false)) {
      if (!create_foz_db_filenames(cache_path, "foz_cache",
                  foz_db->file[0] = fopen(filename, "a+b");
            free(filename);
            if (!check_files_opened_successfully(foz_db->file[0], foz_db->db_idx))
            if (!load_foz_dbs(foz_db, foz_db->db_idx, 0, false))
               char *foz_dbs_ro = getenv("MESA_DISK_CACHE_READ_ONLY_FOZ_DBS");
   if (foz_dbs_ro)
         #ifdef FOZ_DB_UTIL_DYNAMIC_LIST
      char *foz_dbs_list =
         if (foz_dbs_list)
      #endif
               fail:
                  }
      void
   foz_destroy(struct foz_db *foz_db)
   {
   #ifdef FOZ_DB_UTIL_DYNAMIC_LIST
      struct foz_dbs_list_updater *updater = &foz_db->updater;
   if (updater->thrd) {
      inotify_rm_watch(updater->inotify_fd, updater->inotify_wd);
   /* inotify_rm_watch() triggers the IN_IGNORE event for the thread
   * to exit.
   */
   thrd_join(updater->thrd, NULL);
         #endif
         if (foz_db->db_idx)
         for (unsigned i = 0; i < FOZ_MAX_DBS; i++) {
      if (foz_db->file[i])
               if (foz_db->mem_ctx) {
      _mesa_hash_table_u64_destroy(foz_db->index_db);
   ralloc_free(foz_db->mem_ctx);
   simple_mtx_destroy(&foz_db->flock_mtx);
                  }
      /* Here we lookup a cache entry in the index hash table. If an entry is found
   * we use the retrieved offset to read the cache entry from disk.
   */
   void *
   foz_read_entry(struct foz_db *foz_db, const uint8_t *cache_key_160bit,
         {
                        if (!foz_db->alive)
                     struct foz_db_entry *entry =
         if (!entry && foz_db->db_idx) {
      update_foz_index(foz_db, foz_db->db_idx, 0);
      }
   if (!entry) {
      simple_mtx_unlock(&foz_db->mtx);
               uint8_t file_idx = entry->file_idx;
   if (fseek(foz_db->file[file_idx], entry->offset, SEEK_SET) < 0)
            uint32_t header_size = sizeof(struct foz_payload_header);
   if (fread(&entry->header, 1, header_size, foz_db->file[file_idx]) !=
      header_size)
         /* Check for collision using full 160bit hash for increased assurance
   * against potential collisions.
   */
   for (int i = 0; i < 20; i++) {
      if (cache_key_160bit[i] != entry->key[i])
               uint32_t data_sz = entry->header.payload_size;
   data = malloc(data_sz);
   if (fread(data, 1, data_sz, foz_db->file[file_idx]) != data_sz)
            /* verify checksum */
   if (entry->header.crc != 0) {
      if (util_hash_crc32(data, data_sz) != entry->header.crc)
                        if (size)
                  fail:
               /* reading db entry failed. reset the file offset */
               }
      /* Here we write the cache entry to disk and store its offset in the index db.
   */
   bool
   foz_write_entry(struct foz_db *foz_db, const uint8_t *cache_key_160bit,
         {
               if (!foz_db->alive || !foz_db->file[0])
            /* The flock is per-fd, not per thread, we do it outside of the main mutex to avoid having to
   * wait in the mutex potentially blocking reads. We use the secondary flock_mtx to stop race
   * conditions between the write threads sharing the same file descriptor. */
            /* Wait for 1 second. This is done outside of the main mutex as I believe there is more potential
   * for file contention than mtx contention of significant length. */
   int err = lock_file_with_timeout(foz_db->file[0], 1000000000);
   if (err == -1)
                              struct foz_db_entry *entry =
         if (entry) {
      simple_mtx_unlock(&foz_db->mtx);
   flock(fileno(foz_db->file[0]), LOCK_UN);
   simple_mtx_unlock(&foz_db->flock_mtx);
               /* Prepare db entry header and blob ready for writing */
   struct foz_payload_header header;
   header.uncompressed_size = blob_size;
   header.format = FOSSILIZE_COMPRESSION_NONE;
   header.payload_size = blob_size;
                     /* Write hash header to db */
   char hash_str[FOSSILIZE_BLOB_HASH_LENGTH + 1]; /* 40 digits + null */
   _mesa_sha1_format(hash_str, cache_key_160bit);
   if (fwrite(hash_str, 1, FOSSILIZE_BLOB_HASH_LENGTH, foz_db->file[0]) !=
      FOSSILIZE_BLOB_HASH_LENGTH)
                  /* Write db entry header */
   if (fwrite(&header, 1, sizeof(header), foz_db->file[0]) != sizeof(header))
            /* Now write the db entry blob */
   if (fwrite(blob, 1, blob_size, foz_db->file[0]) != blob_size)
            /* Flush everything to file to reduce chance of cache corruption */
            /* Write hash header to index db */
   if (fwrite(hash_str, 1, FOSSILIZE_BLOB_HASH_LENGTH, foz_db->db_idx) !=
      FOSSILIZE_BLOB_HASH_LENGTH)
         header.uncompressed_size = sizeof(uint64_t);
   header.format = FOSSILIZE_COMPRESSION_NONE;
   header.payload_size = sizeof(uint64_t);
            if (fwrite(&header, 1, sizeof(header), foz_db->db_idx) !=
      sizeof(header))
         if (fwrite(&offset, 1, sizeof(uint64_t), foz_db->db_idx) !=
      sizeof(uint64_t))
         /* Flush everything to file to reduce chance of cache corruption */
            entry = ralloc(foz_db->mem_ctx, struct foz_db_entry);
   entry->header = header;
   entry->offset = offset;
   entry->file_idx = 0;
   _mesa_sha1_hex_to_sha1(entry->key, hash_str);
            simple_mtx_unlock(&foz_db->mtx);
   flock(fileno(foz_db->file[0]), LOCK_UN);
                  fail:
         fail_file:
      flock(fileno(foz_db->file[0]), LOCK_UN);
   simple_mtx_unlock(&foz_db->flock_mtx);
      }
   #else
      bool
   foz_prepare(struct foz_db *foz_db, char *filename)
   {
      fprintf(stderr, "Warning: Mesa single file cache selected but Mesa wasn't "
         "built with single cache file support. Shader cache will be disabled"
      }
      void
   foz_destroy(struct foz_db *foz_db)
   {
   }
      void *
   foz_read_entry(struct foz_db *foz_db, const uint8_t *cache_key_160bit,
         {
         }
      bool
   foz_write_entry(struct foz_db *foz_db, const uint8_t *cache_key_160bit,
         {
         }
      #endif
