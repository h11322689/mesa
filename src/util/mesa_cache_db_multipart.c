   /*
   * Copyright © 2022 Collabora, Ltd.
   *
   * SPDX-License-Identifier: MIT
   */
      #include <sys/stat.h>
      #include "detect_os.h"
   #include "string.h"
   #include "mesa_cache_db_multipart.h"
   #include "u_debug.h"
      bool
   mesa_cache_db_multipart_open(struct mesa_cache_db_multipart *db,
         {
   #if DETECT_OS_WINDOWS
         #else
      char *part_path = NULL;
                     db->parts = calloc(db->num_parts, sizeof(*db->parts));
   if (!db->parts)
            for (i = 0; i < db->num_parts; i++) {
               if (asprintf(&part_path, "%s/part%u", cache_path, i) == -1)
            if (mkdir(part_path, 0755) == -1 && errno != EEXIST)
            /* DB opening may fail only in a case of a severe problem,
   * like IO error.
   */
   db_opened = mesa_cache_db_open(&db->parts[i], part_path);
   if (!db_opened)
                        /* remove old pre multi-part cache */
                  free_path:
         close_db:
      while (i--)
                        #endif
   }
      void
   mesa_cache_db_multipart_close(struct mesa_cache_db_multipart *db)
   {
      while (db->num_parts--)
               }
      void
   mesa_cache_db_multipart_set_size_limit(struct mesa_cache_db_multipart *db,
         {
      for (unsigned int i = 0; i < db->num_parts; i++)
      mesa_cache_db_set_size_limit(&db->parts[i],
   }
      void *
   mesa_cache_db_multipart_read_entry(struct mesa_cache_db_multipart *db,
               {
               for (unsigned int i = 0; i < db->num_parts; i++) {
               void *cache_item = mesa_cache_db_read_entry(&db->parts[part],
         if (cache_item) {
      /* Likely that the next entry lookup will hit the same DB part. */
   db->last_read_part = part;
                     }
      static unsigned
   mesa_cache_db_multipart_select_victim_part(struct mesa_cache_db_multipart *db)
   {
      double best_score = 0, score;
            for (unsigned int i = 0; i < db->num_parts; i++) {
      score = mesa_cache_db_eviction_score(&db->parts[i]);
   if (score > best_score) {
      best_score = score;
                     }
      bool
   mesa_cache_db_multipart_entry_write(struct mesa_cache_db_multipart *db,
               {
      unsigned last_written_part = db->last_written_part;
            for (unsigned int i = 0; i < db->num_parts; i++) {
               /* Note that each DB part has own locking. */
   if (mesa_cache_db_has_space(&db->parts[part], blob_size)) {
      wpart = part;
                  /* All DB parts are full. Writing to a full DB part will auto-trigger
   * eviction of LRU cache entries from the part. Select DB part that
   * contains majority of LRU cache entries.
   */
   if (wpart < 0)
                     return mesa_cache_db_entry_write(&db->parts[wpart], cache_key_160bit,
      }
      void
   mesa_cache_db_multipart_entry_remove(struct mesa_cache_db_multipart *db,
         {
      for (unsigned int i = 0; i < db->num_parts; i++)
      }
