   /*
   * Copyright (c) 2018 Intel Corporation
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
      /**
   * The aux map provides a multi-level lookup of the main surface address which
   * ends up providing information about the auxiliary surface data, including
   * the address where the auxiliary data resides.
   *
   * The below sections depict address splitting and formats of table entries of
   * TGL platform. These may vary on other platforms.
   *
   * The 48-bit VMA (GPU) address of the main surface is split to do the address
   * lookup:
   *
   *  48 bit address of main surface
   * +--------+--------+--------+------+
   * | 47:36  | 35:24  | 23:16  | 15:0 |
   * | L3-idx | L2-idx | L1-idx | ...  |
   * +--------+--------+--------+------+
   *
   * The GFX_AUX_TABLE_BASE_ADDR points to a buffer. The L3 Table Entry is
   * located by indexing into this buffer as a uint64_t array using the L3-idx
   * value. The 64-bit L3 entry is defined as:
   *
   * +-------+-------------+------+---+
   * | 63:48 | 47:15       | 14:1 | 0 |
   * |  ...  | L2-tbl-addr | ...  | V |
   * +-------+-------------+------+---+
   *
   * If the `V` (valid) bit is set, then the L2-tbl-addr gives the address for
   * the level-2 table entries, with the lower address bits filled with zero.
   * The L2 Table Entry is located by indexing into this buffer as a uint64_t
   * array using the L2-idx value. The 64-bit L2 entry is similar to the L3
   * entry, except with 2 additional address bits:
   *
   * +-------+-------------+------+---+
   * | 63:48 | 47:13       | 12:1 | 0 |
   * |  ...  | L1-tbl-addr | ...  | V |
   * +-------+-------------+------+---+
   *
   * If the `V` bit is set, then the L1-tbl-addr gives the address for the
   * level-1 table entries, with the lower address bits filled with zero. The L1
   * Table Entry is located by indexing into this buffer as a uint64_t array
   * using the L1-idx value. The 64-bit L1 entry is defined as:
   *
   * +--------+------+-------+-------+-------+---------------+-----+---+
   * | 63:58  | 57   | 56:54 | 53:52 | 51:48 | 47:8          | 7:1 | 0 |
   * | Format | Y/Cr | Depth |  TM   |  ...  | aux-data-addr | ... | V |
   * +--------+------+-------+-------+-------+---------------+-----+---+
   *
   * Where:
   *  - Format: See `get_format_encoding`
   *  - Y/Cr: 0=Y(Luma), 1=Cr(Chroma)
   *  - (bit) Depth: See `get_bpp_encoding`
   *  - TM (Tile-mode): 0=Ys, 1=Y, 2=rsvd, 3=rsvd
   *  - aux-data-addr: VMA/GPU address for the aux-data
   *  - V: entry is valid
   */
      #include "intel_aux_map.h"
   #include "intel_gem.h"
      #include "dev/intel_device_info.h"
   #include "isl/isl.h"
      #include "drm-uapi/i915_drm.h"
   #include "util/list.h"
   #include "util/ralloc.h"
   #include "util/u_atomic.h"
   #include "util/u_math.h"
      #include <inttypes.h>
   #include <stdlib.h>
   #include <stdio.h>
   #include <pthread.h>
      #define INTEL_AUX_MAP_FORMAT_BITS_MASK   0xfff0000000000000ull
      /* Mask with the firt 48bits set */
   #define VALID_ADDRESS_MASK ((1ull << 48) - 1)
      #define L3_ENTRY_L2_ADDR_MASK 0xffffffff8000ull
      #define L3_L2_BITS_PER_LEVEL 12
   #define L3_L2_SUB_TABLE_LEN (sizeof(uint64_t) * (1ull << L3_L2_BITS_PER_LEVEL))
      static const bool aux_map_debug = false;
      /**
   * Auxiliary surface mapping formats
   *
   * Several formats of AUX mapping exist. The supported formats
   * are designated by generation and granularity here. A device
   * can support more than one format, depending on Hardware, but
   * we expect only one of them of a device is needed. Otherwise,
   * we could need to change this enum into a bit map in such case
   * later.
   */
   enum intel_aux_map_format {
      /**
   * 64KB granularity format on GFX12 devices
   */
            /**
   * 1MB granularity format on GFX125 devices
   */
               };
      /**
   * An incomplete description of AUX mapping formats
   *
   * Theoretically, many things can be different, depending on hardware
   * design like level of page tables, address splitting, format bits
   * etc. We only manage the known delta to simplify the implementation
   * this time.
   */
   struct aux_format_info {
      /**
   * Granularity of main surface in compression. It must be power of 2.
   */
   uint64_t main_page_size;
   /**
   * The ratio of main surface to an AUX entry.
   */
   uint64_t main_to_aux_ratio;
   /**
   * Page size of level 1 page table. It must be power of 2.
   */
   uint64_t l1_page_size;
   /**
   * Mask of index bits of level 1 page table in address splitting.
   */
   uint64_t l1_index_mask;
   /**
   * Offset of index bits of level 1 page table in address splitting.
   */
      };
      static const struct aux_format_info aux_formats[] = {
      [INTEL_AUX_MAP_GFX12_64KB] = {
      .main_page_size = 64 * 1024,
   .main_to_aux_ratio = 256,
   .l1_page_size = 8 * 1024,
   .l1_index_mask = 0xff,
      },
   [INTEL_AUX_MAP_GFX125_1MB] = {
      .main_page_size = 1024 * 1024,
   .main_to_aux_ratio = 256,
   .l1_page_size = 2 * 1024,
   .l1_index_mask = 0xf,
         };
      struct aux_map_buffer {
      struct list_head link;
      };
      struct intel_aux_level {
      /* GPU address of the  current level */
            /* Pointer to the GPU entries of this level */
            union {
      /* Host tracking of a parent level to its children (only use on L3/L2
   * levels which have 4096 entries)
   */
            /* Refcount of AUX pages at the L1 level (MTL has only 16 entries in L1,
   * which Gfx12 has 256 entries)
   */
         };
      struct intel_aux_map_context {
      void *driver_ctx;
   pthread_mutex_t mutex;
   struct intel_aux_level *l3_level;
   struct intel_mapped_pinned_buffer_alloc *buffer_alloc;
   uint32_t num_buffers;
   struct list_head buffers;
   uint32_t tail_offset, tail_remaining;
   uint32_t state_num;
      };
      static inline uint64_t
   get_page_mask(const uint64_t page_size)
   {
         }
      static inline uint64_t
   get_meta_page_size(const struct aux_format_info *info)
   {
         }
      static inline uint64_t
   get_index(const uint64_t main_address,
         {
         }
      uint64_t
   intel_aux_get_meta_address_mask(struct intel_aux_map_context *ctx)
   {
         }
      uint64_t
   intel_aux_get_main_to_aux_ratio(struct intel_aux_map_context *ctx)
   {
         }
      static const struct aux_format_info *
   get_format(enum intel_aux_map_format format)
   {
         assert(format < INTEL_AUX_MAP_LAST);
   assert(ARRAY_SIZE(aux_formats) == INTEL_AUX_MAP_LAST);
      }
      static enum intel_aux_map_format
   select_format(const struct intel_device_info *devinfo)
   {
      if (devinfo->verx10 >= 125)
         else if (devinfo->verx10 == 120)
         else
      }
      static bool
   add_buffer(struct intel_aux_map_context *ctx)
   {
      struct aux_map_buffer *buf = rzalloc(ctx, struct aux_map_buffer);
   if (!buf)
            const uint32_t size = 0x100000;
   buf->buffer = ctx->buffer_alloc->alloc(ctx->driver_ctx, size);
   if (!buf->buffer) {
      ralloc_free(buf);
                        list_addtail(&buf->link, &ctx->buffers);
   ctx->tail_offset = 0;
   ctx->tail_remaining = size;
               }
      static void
   advance_current_pos(struct intel_aux_map_context *ctx, uint32_t size)
   {
      assert(ctx->tail_remaining >= size);
   ctx->tail_remaining -= size;
      }
      static bool
   align_and_verify_space(struct intel_aux_map_context *ctx, uint32_t size,
         {
      if (ctx->tail_remaining < size)
            struct aux_map_buffer *tail =
         uint64_t gpu = tail->buffer->gpu + ctx->tail_offset;
            if ((aligned - gpu) + size > ctx->tail_remaining) {
         } else {
      if (aligned - gpu > 0)
               }
      static void
   get_current_pos(struct intel_aux_map_context *ctx, uint64_t *gpu, uint64_t **map)
   {
      assert(!list_is_empty(&ctx->buffers));
   struct aux_map_buffer *tail =
         if (gpu)
         if (map)
      }
      static struct intel_aux_level *
   add_sub_table(struct intel_aux_map_context *ctx,
               struct intel_aux_level *parent,
   {
      if (!align_and_verify_space(ctx, size, align)) {
      if (!add_buffer(ctx))
         UNUSED bool aligned = align_and_verify_space(ctx, size, align);
                        get_current_pos(ctx, &level->address, &level->entries);
   memset(level->entries, 0, size);
            if (parent != NULL) {
      assert(parent->children[parent_index] == NULL);
                  }
      uint32_t
   intel_aux_map_get_state_num(struct intel_aux_map_context *ctx)
   {
         }
      struct intel_aux_map_context *
   intel_aux_map_init(void *driver_ctx,
               {
               enum intel_aux_map_format format = select_format(devinfo);
   if (format == INTEL_AUX_MAP_LAST)
            ctx = ralloc(NULL, struct intel_aux_map_context);
   if (!ctx)
            if (pthread_mutex_init(&ctx->mutex, NULL))
            ctx->format = get_format(format);
   ctx->driver_ctx = driver_ctx;
   ctx->buffer_alloc = buffer_alloc;
   ctx->num_buffers = 0;
   list_inithead(&ctx->buffers);
   ctx->tail_offset = 0;
   ctx->tail_remaining = 0;
            ctx->l3_level = add_sub_table(ctx, NULL, 0,
         if (ctx->l3_level != NULL) {
      if (aux_map_debug)
      fprintf(stderr, "AUX-MAP L3: 0x%"PRIx64", map=%p\n",
      p_atomic_inc(&ctx->state_num);
      } else {
      ralloc_free(ctx);
         }
      void
   intel_aux_map_finish(struct intel_aux_map_context *ctx)
   {
      if (!ctx)
            pthread_mutex_destroy(&ctx->mutex);
   list_for_each_entry_safe(struct aux_map_buffer, buf, &ctx->buffers, link) {
      ctx->buffer_alloc->free(ctx->driver_ctx, buf->buffer);
   list_del(&buf->link);
   p_atomic_dec(&ctx->num_buffers);
                  }
      uint32_t
   intel_aux_map_get_alignment(struct intel_aux_map_context *ctx)
   {
         }
      uint64_t
   intel_aux_map_get_base(struct intel_aux_map_context *ctx)
   {
      /**
   * This get initialized in intel_aux_map_init, and never changes, so there is
   * no need to lock the mutex.
   */
      }
      static uint8_t
   get_bpp_encoding(enum isl_format format)
   {
      if (isl_format_is_yuv(format)) {
      switch (format) {
   case ISL_FORMAT_YCRCB_NORMAL:
   case ISL_FORMAT_YCRCB_SWAPY:
   case ISL_FORMAT_PLANAR_420_8: return 3;
   case ISL_FORMAT_PLANAR_420_12: return 2;
   case ISL_FORMAT_PLANAR_420_10: return 1;
   case ISL_FORMAT_PLANAR_420_16: return 0;
   default:
      unreachable("Unsupported format!");
         } else {
      switch (isl_format_get_layout(format)->bpb) {
   case 16:  return 0;
   case 8:   return 4;
   case 32:  return 5;
   case 64:  return 6;
   case 128: return 7;
   default:
      unreachable("Unsupported bpp!");
            }
      #define INTEL_AUX_MAP_ENTRY_Ys_TILED_BIT  (0x0ull << 52)
   #define INTEL_AUX_MAP_ENTRY_Y_TILED_BIT   (0x1ull << 52)
      uint64_t
   intel_aux_map_format_bits(enum isl_tiling tiling, enum isl_format format,
         {
      /* gfx12.5+ uses tile-4 rather than y-tiling, and gfx12.5+ also uses
   * compression info from the surface state and ignores the aux-map format
   * bits metadata.
   */
   if (!isl_tiling_is_any_y(tiling))
            if (aux_map_debug)
      fprintf(stderr, "AUX-MAP entry %s, bpp_enc=%d\n",
               assert(tiling == ISL_TILING_ICL_Ys ||
                  uint64_t format_bits =
      ((uint64_t)isl_format_get_aux_map_encoding(format) << 58) |
   ((uint64_t)(plane > 0) << 57) |
   ((uint64_t)get_bpp_encoding(format) << 54) |
   /* TODO: We assume that Yf is not Tiled-Ys, but waiting on
   *       clarification
   */
   (tiling == ISL_TILING_ICL_Ys ? INTEL_AUX_MAP_ENTRY_Ys_TILED_BIT :
                     }
      uint64_t
   intel_aux_map_format_bits_for_isl_surf(const struct isl_surf *isl_surf)
   {
      assert(!isl_format_is_planar(isl_surf->format));
      }
      static uint64_t
   get_l1_addr_mask(struct intel_aux_map_context *ctx)
   {
      uint64_t l1_addr = ~get_page_mask(ctx->format->l1_page_size);
      }
      static void
   get_aux_entry(struct intel_aux_map_context *ctx, uint64_t main_address,
               uint32_t *l1_index_out, uint64_t *l1_entry_addr_out,
   {
      struct intel_aux_level *l3_level = ctx->l3_level;
   struct intel_aux_level *l2_level;
                     if (l3_level->children[l3_index] == NULL) {
      l2_level =
      add_sub_table(ctx, ctx->l3_level, l3_index,
      if (l2_level != NULL) {
      if (aux_map_debug)
      fprintf(stderr, "AUX-MAP L3[0x%x]: 0x%"PRIx64", map=%p\n",
   } else {
         }
   l3_level->entries[l3_index] = (l2_level->address & L3_ENTRY_L2_ADDR_MASK) |
      } else {
         }
   uint32_t l2_index = (main_address >> 24) & 0xfff;
   uint64_t l1_page_size = ctx->format->l1_page_size;
   if (l2_level->children[l2_index] == NULL) {
      l1_level = add_sub_table(ctx, l2_level, l2_index, l1_page_size, l1_page_size);
   if (l1_level != NULL) {
      if (aux_map_debug)
      fprintf(stderr, "AUX-MAP L2[0x%x]: 0x%"PRIx64", map=%p\n",
   } else {
         }
   l2_level->entries[l2_index] = (l1_level->address & get_l1_addr_mask(ctx)) |
      } else {
         }
   uint32_t l1_index = get_index(main_address, ctx->format->l1_index_mask,
         if (l1_index_out)
         if (l1_entry_addr_out)
         if (l1_entry_map_out)
         if (l1_aux_level_out)
      }
      static bool
   add_mapping(struct intel_aux_map_context *ctx, uint64_t main_address,
               {
      if (aux_map_debug)
      fprintf(stderr, "AUX-MAP 0x%"PRIx64" => 0x%"PRIx64"\n", main_address,
         uint32_t l1_index;
   uint64_t *l1_entry;
   struct intel_aux_level *l1_aux_level;
            const uint64_t l1_data =
      (aux_address & intel_aux_get_meta_address_mask(ctx)) |
   format_bits |
         const uint64_t current_l1_data = *l1_entry;
   if ((current_l1_data & INTEL_AUX_MAP_ENTRY_VALID_BIT) == 0) {
      assert(l1_aux_level->ref_counts[l1_index] == 0);
   assert((aux_address & 0xffULL) == 0);
   if (aux_map_debug)
      fprintf(stderr, "AUX-MAP L1[0x%x] 0x%"PRIx64" -> 0x%"PRIx64"\n",
      /**
   * We use non-zero bits in 63:1 to indicate the entry had been filled
   * previously. If these bits are non-zero and they don't exactly match
   * what we want to program into the entry, then we must force the
   * aux-map tables to be flushed.
   */
   if (current_l1_data != 0 && \
      (current_l1_data | INTEL_AUX_MAP_ENTRY_VALID_BIT) != l1_data)
         } else {
      if (aux_map_debug)
                  if (*l1_entry != l1_data) {
      if (aux_map_debug)
      fprintf(stderr,
                                          }
      uint64_t *
   intel_aux_map_get_entry(struct intel_aux_map_context *ctx,
               {
      pthread_mutex_lock(&ctx->mutex);
   uint64_t *l1_entry_map;
   get_aux_entry(ctx, main_address, NULL, aux_entry_address, &l1_entry_map, NULL);
               }
      /**
   * We mark the leaf entry as invalid, but we don't attempt to cleanup the
   * other levels of translation mappings. Since we attempt to re-use VMA
   * ranges, hopefully this will not lead to unbounded growth of the translation
   * tables.
   */
   static void
   remove_l1_mapping_locked(struct intel_aux_map_context *ctx, uint64_t main_address,
         {
      uint32_t l1_index;
   uint64_t *l1_entry;
   struct intel_aux_level *l1_aux_level;
            const uint64_t current_l1_data = *l1_entry;
            if ((current_l1_data & INTEL_AUX_MAP_ENTRY_VALID_BIT) == 0) {
      assert(l1_aux_level->ref_counts[l1_index] == 0);
      } else if (reset_refcount) {
      l1_aux_level->ref_counts[l1_index] = 0;
   if (unlikely(l1_data == 0))
            } else {
      assert(l1_aux_level->ref_counts[l1_index] > 0);
   if (--l1_aux_level->ref_counts[l1_index] == 0) {
      /**
   * We use non-zero bits in 63:1 to indicate the entry had been filled
   * previously. In the unlikely event that these are all zero, we
   * force a flush of the aux-map tables.
   */
   if (unlikely(l1_data == 0))
                  }
      static void
   remove_mapping_locked(struct intel_aux_map_context *ctx, uint64_t main_address,
         {
      if (aux_map_debug)
      fprintf(stderr, "AUX-MAP remove 0x%"PRIx64"-0x%"PRIx64"\n", main_address,
         uint64_t main_inc_addr = main_address;
   uint64_t main_page_size = ctx->format->main_page_size;
   assert((main_address & get_page_mask(main_page_size)) == 0);
   while (main_inc_addr - main_address < size) {
      remove_l1_mapping_locked(ctx, main_inc_addr, reset_refcount,
               }
      bool
   intel_aux_map_add_mapping(struct intel_aux_map_context *ctx, uint64_t main_address,
               {
      bool state_changed = false;
   pthread_mutex_lock(&ctx->mutex);
   uint64_t main_inc_addr = main_address;
   uint64_t aux_inc_addr = aux_address;
   const uint64_t main_page_size = ctx->format->main_page_size;
   assert((main_address & get_page_mask(main_page_size)) == 0);
   const uint64_t aux_page_size = get_meta_page_size(ctx->format);
   assert((aux_address & get_page_mask(aux_page_size)) == 0);
   while (main_inc_addr - main_address < main_size_B) {
      if (!add_mapping(ctx, main_inc_addr, aux_inc_addr, format_bits,
               }
   main_inc_addr = main_inc_addr + main_page_size;
      }
   bool success = main_inc_addr - main_address >= main_size_B;
   if (!success && (main_inc_addr - main_address) > 0) {
      /* If the mapping failed, remove the mapped portion. */
   remove_mapping_locked(ctx, main_address,
            }
   pthread_mutex_unlock(&ctx->mutex);
   if (state_changed)
                  }
      void
   intel_aux_map_del_mapping(struct intel_aux_map_context *ctx, uint64_t main_address,
         {
      bool state_changed = false;
   pthread_mutex_lock(&ctx->mutex);
   remove_mapping_locked(ctx, main_address, size, false /* reset_refcount */,
         pthread_mutex_unlock(&ctx->mutex);
   if (state_changed)
      }
      void
   intel_aux_map_unmap_range(struct intel_aux_map_context *ctx, uint64_t main_address,
         {
      bool state_changed = false;
   pthread_mutex_lock(&ctx->mutex);
   remove_mapping_locked(ctx, main_address, size, true /* reset_refcount */,
         pthread_mutex_unlock(&ctx->mutex);
   if (state_changed)
      }
      uint32_t
   intel_aux_map_get_num_buffers(struct intel_aux_map_context *ctx)
   {
         }
      void
   intel_aux_map_fill_bos(struct intel_aux_map_context *ctx, void **driver_bos,
         {
      assert(p_atomic_read(&ctx->num_buffers) >= max_bos);
   uint32_t i = 0;
   list_for_each_entry(struct aux_map_buffer, buf, &ctx->buffers, link) {
      if (i >= max_bos)
               }
