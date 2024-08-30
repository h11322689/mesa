   #include <inttypes.h>  /* for PRIu64 macro */
   #include "util/u_math.h"
   #include "lp_rast_priv.h"
   #include "lp_state_fs.h"
         struct tile {
      int coverage;
   int overdraw;
   const struct lp_rast_state *state;
      };
         static char
   get_label(int i)
   {
      static const char *cmd_labels =
                  if (i < max_label)
         else
      }
         static const char *cmd_names[] =
   {
      "clear_color",
   "clear_zstencil",
   "triangle_1",
   "triangle_2",
   "triangle_3",
   "triangle_4",
   "triangle_5",
   "triangle_6",
   "triangle_7",
   "triangle_8",
   "triangle_3_4",
   "triangle_3_16",
   "triangle_4_16",
   "shade_tile",
   "shade_tile_opaque",
   "begin_query",
   "end_query",
   "set_state",
   "triangle_32_1",
   "triangle_32_2",
   "triangle_32_3",
   "triangle_32_4",
   "triangle_32_5",
   "triangle_32_6",
   "triangle_32_7",
   "triangle_32_8",
   "triangle_32_3_4",
   "triangle_32_3_16",
   "triangle_32_4_16",
   "lp_rast_triangle_ms_1",
   "lp_rast_triangle_ms_2",
   "lp_rast_triangle_ms_3",
   "lp_rast_triangle_ms_4",
   "lp_rast_triangle_ms_5",
   "lp_rast_triangle_ms_6",
   "lp_rast_triangle_ms_7",
   "lp_rast_triangle_ms_8",
   "lp_rast_triangle_ms_3_4",
   "lp_rast_triangle_ms_3_16",
   "lp_rast_triangle_ms_4_16",
   "rectangle",
      };
         static const char *
   cmd_name(unsigned cmd)
   {
      STATIC_ASSERT(ARRAY_SIZE(cmd_names) == LP_RAST_OP_MAX);
   assert(ARRAY_SIZE(cmd_names) > cmd);
      }
         static const struct lp_fragment_shader_variant *
   get_variant(const struct lp_rast_state *state,
               {
      if (!state)
            if (block->cmd[k] == LP_RAST_OP_SHADE_TILE ||
      block->cmd[k] == LP_RAST_OP_SHADE_TILE_OPAQUE ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_1 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_2 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_3 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_4 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_5 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_6 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_7 ||
   block->cmd[k] == LP_RAST_OP_RECTANGLE ||
   block->cmd[k] == LP_RAST_OP_BLIT)
            }
         static bool
   is_blend(const struct lp_rast_state *state,
               {
      const struct lp_fragment_shader_variant *variant =
            if (variant)
               }
         static bool
   is_linear(const struct lp_rast_state *state,
               {
      if (block->cmd[k] == LP_RAST_OP_BLIT)
            if (block->cmd[k] == LP_RAST_OP_SHADE_TILE ||
      block->cmd[k] == LP_RAST_OP_SHADE_TILE_OPAQUE)
         if (block->cmd[k] == LP_RAST_OP_RECTANGLE)
               }
         static const char *
   get_fs_kind(const struct lp_rast_state *state,
               {
      const struct lp_fragment_shader_variant *variant =
            if (variant)
               }
         static void
   debug_bin(const struct cmd_bin *bin, int x, int y)
   {
      const struct lp_rast_state *state = NULL;
   const struct cmd_block *head = bin->head;
                     if (info.type & LP_RAST_FLAGS_BLIT)
         else if (info.type & LP_RAST_FLAGS_TILE)
         else if (info.type & LP_RAST_FLAGS_RECT)
         else if (info.type & LP_RAST_FLAGS_TRI)
         else
                     int j = 0;
   while (head) {
      for (int i = 0; i < head->count; i++, j++) {
                     debug_printf("%d: %s %s\n", j,
            }
         }
         static void
   plot(struct tile *tile,
      int x, int y,
   char val,
      {
      if (tile->data[x][y] == ' ')
         else
               }
         /**
   * Scan the tile in chunks and figure out which pixels to rasterize
   * for this rectangle.
   */
   static int
   debug_rectangle(int x, int y,
                     {
               /* Check for "disabled" rectangles generated in out-of-memory
   * conditions.
   */
   if (rect->inputs.disable) {
      /* This command was partially binned and has been disabled */
               bool blend = tile->state->variant->key.blend.rt[0].blend_enable;
   unsigned count = 0;
   for (unsigned i = 0; i < TILE_SIZE; i++) {
      for (unsigned j = 0; j < TILE_SIZE; j++) {
      if (rect->box.x0 <= x + i &&
      rect->box.x1 >= x + i &&
   rect->box.y0 <= y + j &&
   rect->box.y1 >= y + j) {
   plot(tile, i, j, val, blend);
            }
      }
         static int
   debug_blit_tile(int x, int y,
                     {
               if (inputs->disable)
            for (unsigned i = 0; i < TILE_SIZE; i++)
      for (unsigned j = 0; j < TILE_SIZE; j++)
            }
         static int
   debug_shade_tile(int x, int y,
                     {
      if (!tile->state)
            const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   if (inputs->disable)
                     for (unsigned i = 0; i < TILE_SIZE; i++)
      for (unsigned j = 0; j < TILE_SIZE; j++)
            }
         static int
   debug_clear_tile(int x, int y,
                     {
      for (unsigned i = 0; i < TILE_SIZE; i++)
      for (unsigned j = 0; j < TILE_SIZE; j++)
            }
         static int
   debug_triangle(int tilex, int tiley,
                     {
      const struct lp_rast_triangle *tri = arg.triangle.tri;
   unsigned plane_mask = arg.triangle.plane_mask;
   const struct lp_rast_plane *tri_plane = GET_PLANES(tri);
   struct lp_rast_plane plane[8];
   int x, y;
   int count = 0;
   unsigned i, nr_planes = 0;
            if (tri->inputs.disable) {
      /* This triangle was partially binned and has been disabled */
               while (plane_mask) {
      plane[nr_planes] = tri_plane[u_bit_scan(&plane_mask)];
   plane[nr_planes].c = (plane[nr_planes].c +
                           for (y = 0; y < TILE_SIZE; y++) {
      for (x = 0; x < TILE_SIZE; x++) {
      for (i = 0; i < nr_planes; i++)
                              out:
      for (i = 0; i < nr_planes; i++)
               for (i = 0; i < nr_planes; i++) {
      plane[i].c += IMUL64(plane[i].dcdx, TILE_SIZE);
         }
      }
         static void
   do_debug_bin(struct tile *tile,
               const struct cmd_bin *bin,
   {
      unsigned k, j = 0;
            int tx = x * TILE_SIZE;
            memset(tile->data, ' ', sizeof tile->data);
   tile->coverage = 0;
   tile->overdraw = 0;
            for (block = bin->head; block; block = block->next) {
      for (k = 0; k < block->count; k++, j++) {
      bool blend = is_blend(tile->state, block, k);
   bool linear = is_linear(tile->state, block, k);
   const char *fskind = get_fs_kind(tile->state, block, k);
                                                if (block->cmd[k] == LP_RAST_OP_CLEAR_COLOR ||
                                 if (block->cmd[k] == LP_RAST_OP_SHADE_TILE ||
                  if (block->cmd[k] == LP_RAST_OP_TRIANGLE_1 ||
      block->cmd[k] == LP_RAST_OP_TRIANGLE_2 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_3 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_4 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_5 ||
   block->cmd[k] == LP_RAST_OP_TRIANGLE_6 ||
                                                                                                   }
         void
   lp_debug_bin(const struct cmd_bin *bin, int i, int j)
   {
               if (bin->head) {
               debug_printf("------------------------------------------------------------------\n");
   for (int y = 0; y < TILE_SIZE; y++) {
      for (int x = 0; x < TILE_SIZE; x++) {
         }
      }
            debug_printf("each pixel drawn avg %f times\n",
         }
         /** Return number of bytes used for a single bin */
   static unsigned
   lp_scene_bin_size(const struct lp_scene *scene, unsigned x, unsigned y)
   {
      struct cmd_bin *bin = lp_scene_get_bin((struct lp_scene *) scene, x, y);
   const struct cmd_block *cmd;
   unsigned size = 0;
   for (cmd = bin->head; cmd; cmd = cmd->next) {
      size += (cmd->count *
      }
      }
         void
   lp_debug_draw_bins_by_coverage(struct lp_scene *scene)
   {
      unsigned total = 0;
   unsigned possible = 0;
   static uint64_t _total = 0;
            for (unsigned x = 0; x < scene->tiles_x; x++)
                  for (unsigned y = 0; y < scene->tiles_y; y++) {
      for (unsigned x = 0; x < scene->tiles_x; x++) {
               if (bin->head) {
                                             if (tile.coverage == 64*64)
         else if (tile.coverage) {
      const char *bits = "0123456789";
   int bit = tile.coverage/(64.0*64.0)*10;
      }
   else
      } else {
            }
               for (unsigned x = 0; x < scene->tiles_x; x++)
                  debug_printf("this tile total: %u possible %u: percentage: %f\n",
                        _total += total;
            debug_printf("overall   total: %" PRIu64
               " possible %" PRIu64 ": percentage: %f\n",
      }
         void
   lp_debug_draw_bins_by_cmd_length(struct lp_scene *scene)
   {
      for (unsigned y = 0; y < scene->tiles_y; y++) {
      for (unsigned x = 0; x < scene->tiles_x; x++) {
      const char *bits = " ...,-~:;=o+xaw*#XAWWWWWWWWWWWWWWWW";
   unsigned sz = lp_scene_bin_size(scene, x, y);
   unsigned sz2 = util_logbase2(sz);
      }
         }
         void
   lp_debug_bins(struct lp_scene *scene)
   {
      for (unsigned y = 0; y < scene->tiles_y; y++) {
      for (unsigned x = 0; x < scene->tiles_x; x++) {
      struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
   if (bin->head) {
                  }
