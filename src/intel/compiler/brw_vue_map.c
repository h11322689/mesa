   /*
   * Copyright © 2011 Intel Corporation
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
   * @file brw_vue_map.c
   *
   * This file computes the "VUE map" for a (non-fragment) shader stage, which
   * describes the layout of its output varyings.  The VUE map is used to match
   * outputs from one stage with the inputs of the next.
   *
   * Largely, varyings can be placed however we like - producers/consumers simply
   * have to agree on the layout.  However, there is also a "VUE Header" that
   * prescribes a fixed-layout for items that interact with fixed function
   * hardware, such as the clipper and rasterizer.
   *
   * Authors:
   *   Paul Berry <stereotype441@gmail.com>
   *   Chris Forbes <chrisf@ijw.co.nz>
   *   Eric Anholt <eric@anholt.net>
   */
         #include "brw_compiler.h"
   #include "dev/intel_debug.h"
      static inline void
   assign_vue_slot(struct brw_vue_map *vue_map, int varying, int slot)
   {
      /* Make sure this varying hasn't been assigned a slot already */
            vue_map->varying_to_slot[varying] = slot;
      }
      /**
   * Compute the VUE map for a shader stage.
   */
   void
   brw_compute_vue_map(const struct intel_device_info *devinfo,
                     struct brw_vue_map *vue_map,
   {
      /* Keep using the packed/contiguous layout on old hardware - we only need
   * the SSO layout when using geometry/tessellation shaders or 32 FS input
   * varyings, which only exist on Gen >= 6.  It's also a bit more efficient.
   */
   if (devinfo->ver < 6)
            if (separate) {
      /* In SSO mode, we don't know whether the adjacent stage will
   * read/write gl_ClipDistance, which has a fixed slot location.
   * We have to assume the worst and reserve a slot for it, or else
   * the rest of our varyings will be off by a slot.
   *
   * Note that we don't have to worry about COL/BFC, as those built-in
   * variables only exist in legacy GL, which only supports VS and FS.
   */
   slots_valid |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0);
               vue_map->slots_valid = slots_valid;
            /* gl_Layer, gl_ViewportIndex & gl_PrimitiveShadingRateEXT don't get their
   * own varying slots -- they are stored in the first VUE slot
   * (VARYING_SLOT_PSIZ).
   */
            /* Make sure that the values we store in vue_map->varying_to_slot and
   * vue_map->slot_to_varying won't overflow the signed chars that are used
   * to store them.  Note that since vue_map->slot_to_varying sometimes holds
   * values equal to BRW_VARYING_SLOT_COUNT, we need to ensure that
   * BRW_VARYING_SLOT_COUNT is <= 127, not 128.
   */
            for (int i = 0; i < BRW_VARYING_SLOT_COUNT; ++i) {
      vue_map->varying_to_slot[i] = -1;
                        /* VUE header: format depends on chip generation and whether clipping is
   * enabled.
   *
   * See the Sandybridge PRM, Volume 2 Part 1, section 1.5.1 (page 30),
   * "Vertex URB Entry (VUE) Formats" which describes the VUE header layout.
   */
   if (devinfo->ver < 6) {
      /* There are 8 dwords in VUE header pre-Ironlake:
   * dword 0-3 is indices, point width, clip flags.
   * dword 4-7 is ndc position
   * dword 8-11 is the first vertex data.
   *
   * On Ironlake the VUE header is nominally 20 dwords, but the hardware
   * will accept the same header layout as Gfx4 [and should be a bit faster]
   */
   assign_vue_slot(vue_map, VARYING_SLOT_PSIZ, slot++);
   assign_vue_slot(vue_map, BRW_VARYING_SLOT_NDC, slot++);
      } else {
      /* There are 8 or 16 DWs (D0-D15) in VUE header on Sandybridge:
   * dword 0-3 of the header is shading rate, indices, point width, clip flags.
   * dword 4-7 is the 4D space position
   * dword 8-15 of the vertex header is the user clip distance if
   * enabled.
   * dword 8-11 or 16-19 is the first vertex element data we fill.
   */
   assign_vue_slot(vue_map, VARYING_SLOT_PSIZ, slot++);
            /* When using Primitive Replication, multiple slots are used for storing
   * positions for each view.
   */
   assert(pos_slots >= 1);
   if (pos_slots > 1) {
      for (int i = 1; i < pos_slots; i++) {
                     if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0))
         if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1))
            /* Vertex URB Formats table says: "Vertex Header shall be padded at the
   * end so that the header ends on a 32-byte boundary".
   */
            /* front and back colors need to be consecutive so that we can use
   * ATTRIBUTE_SWIZZLE_INPUTATTR_FACING to swizzle them when doing
   * two-sided color.
   */
   if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_COL0))
         if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_COL1))
         if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_BFC1))
               /* The hardware doesn't care about the rest of the vertex outputs, so we
   * can assign them however we like.  For normal programs, we simply assign
   * them contiguously.
   *
   * For separate shader pipelines, we first assign built-in varyings
   * contiguous slots.  This works because ARB_separate_shader_objects
   * requires that all shaders have matching built-in varying interface
   * blocks.  Next, we assign generic varyings based on their location
   * (either explicit or linker assigned).  This guarantees a fixed layout.
   *
   * We generally don't need to assign a slot for VARYING_SLOT_CLIP_VERTEX,
   * since it's encoded as the clip distances by emit_clip_distances().
   * However, it may be output by transform feedback, and we'd rather not
   * recompute state when TF changes, so we just always include it.
   */
   uint64_t builtins = slots_valid & BITFIELD64_MASK(VARYING_SLOT_VAR0);
   while (builtins != 0) {
      const int varying = ffsll(builtins) - 1;
   if (vue_map->varying_to_slot[varying] == -1) {
         }
               const int first_generic_slot = slot;
   uint64_t generics = slots_valid & ~BITFIELD64_MASK(VARYING_SLOT_VAR0);
   while (generics != 0) {
      const int varying = ffsll(generics) - 1;
   if (separate) {
         }
   assign_vue_slot(vue_map, varying, slot++);
               vue_map->num_slots = slot;
   vue_map->num_pos_slots = pos_slots;
   vue_map->num_per_vertex_slots = 0;
      }
      /**
   * Compute the VUE map for tessellation control shader outputs and
   * tessellation evaluation shader inputs.
   */
   void
   brw_compute_tess_vue_map(struct brw_vue_map *vue_map,
               {
      /* I don't think anything actually uses this... */
            /* separate isn't really meaningful, but make sure it's initialized */
            vertex_slots &= ~(VARYING_BIT_TESS_LEVEL_OUTER |
            /* Make sure that the values we store in vue_map->varying_to_slot and
   * vue_map->slot_to_varying won't overflow the signed chars that are used
   * to store them.  Note that since vue_map->slot_to_varying sometimes holds
   * values equal to VARYING_SLOT_TESS_MAX , we need to ensure that
   * VARYING_SLOT_TESS_MAX is <= 127, not 128.
   */
            for (int i = 0; i < VARYING_SLOT_TESS_MAX ; ++i) {
      vue_map->varying_to_slot[i] = -1;
                        /* The first 8 DWords are reserved for the "Patch Header".
   *
   * VARYING_SLOT_TESS_LEVEL_OUTER / INNER live here, but the exact layout
   * depends on the domain type.  They might not be in slots 0 and 1 as
   * described here, but pretending they're separate allows us to uniquely
   * identify them by distinct slot locations.
   */
   assign_vue_slot(vue_map, VARYING_SLOT_TESS_LEVEL_INNER, slot++);
            /* first assign per-patch varyings */
   while (patch_slots != 0) {
      const int varying = ffsll(patch_slots) - 1;
   if (vue_map->varying_to_slot[varying + VARYING_SLOT_PATCH0] == -1) {
         }
               /* apparently, including the patch header... */
            /* then assign per-vertex varyings for each vertex in our patch */
   while (vertex_slots != 0) {
      const int varying = ffsll(vertex_slots) - 1;
   if (vue_map->varying_to_slot[varying] == -1) {
         }
               vue_map->num_per_vertex_slots = slot - vue_map->num_per_patch_slots;
   vue_map->num_pos_slots = 0;
      }
      static const char *
   varying_name(brw_varying_slot slot, gl_shader_stage stage)
   {
               if (slot < VARYING_SLOT_MAX)
            static const char *brw_names[] = {
      [BRW_VARYING_SLOT_NDC - VARYING_SLOT_MAX] = "BRW_VARYING_SLOT_NDC",
   [BRW_VARYING_SLOT_PAD - VARYING_SLOT_MAX] = "BRW_VARYING_SLOT_PAD",
                  }
      void
   brw_print_vue_map(FILE *fp, const struct brw_vue_map *vue_map,
         {
      if (vue_map->num_per_vertex_slots > 0 || vue_map->num_per_patch_slots > 0) {
      fprintf(fp, "PUE map (%d slots, %d/patch, %d/vertex, %s)\n",
         vue_map->num_slots,
   vue_map->num_per_patch_slots,
   vue_map->num_per_vertex_slots,
   for (int i = 0; i < vue_map->num_slots; i++) {
      if (vue_map->slot_to_varying[i] >= VARYING_SLOT_PATCH0) {
      fprintf(fp, "  [%d] VARYING_SLOT_PATCH%d\n", i,
      } else {
      fprintf(fp, "  [%d] %s\n", i,
            } else {
      fprintf(fp, "VUE map (%d slots, %s)\n",
         for (int i = 0; i < vue_map->num_slots; i++) {
      fprintf(fp, "  [%d] %s\n", i,
         }
      }
