   /*
   * Copyright © 2016 Broadcom
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
      #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include "drm-uapi/v3d_drm.h"
   #include "clif_dump.h"
   #include "clif_private.h"
   #include "util/list.h"
   #include "util/ralloc.h"
      #include "broadcom/cle/v3d_decoder.h"
      struct reloc_worklist_entry *
   clif_dump_add_address_to_worklist(struct clif_dump *clif,
               {
         struct reloc_worklist_entry *entry =
         if (!entry)
            entry->type = type;
                     }
      struct clif_dump *
   clif_dump_init(const struct v3d_device_info *devinfo,
         {
                  clif->devinfo = devinfo;
   clif->out = out;
   clif->spec = v3d_spec_load(devinfo);
   clif->pretty = pretty;
                     }
      void
   clif_dump_destroy(struct clif_dump *clif)
   {
         }
      struct clif_bo *
   clif_lookup_bo(struct clif_dump *clif, uint32_t addr)
   {
         for (int i = 0; i < clif->bo_count; i++) {
                     if (addr >= bo->offset &&
      addr < bo->offset + bo->size) {
            }
      static bool
   clif_lookup_vaddr(struct clif_dump *clif, uint32_t addr, void **vaddr)
   {
         struct clif_bo *bo = clif_lookup_bo(clif, addr);
   if (!bo)
            *vaddr = bo->vaddr + addr - bo->offset;
   }
      #define out_uint(_clif, field) out(_clif, "    /* %s = */ %u\n",        \
            static bool
   clif_dump_packet(struct clif_dump *clif, uint32_t offset, const uint8_t *cl,
         {
         if (clif->devinfo->ver >= 42)
         else if (clif->devinfo->ver >= 41)
         else
   }
      static uint32_t
   clif_dump_cl(struct clif_dump *clif, uint32_t start, uint32_t end,
         {
         struct clif_bo *bo = clif_lookup_bo(clif, start);
   if (!bo) {
            out(clif, "Failed to look up address 0x%08x\n",
                        /* The end address is optional (for example, a BRANCH instruction
      * won't set an end), but is used for BCL/RCL termination.
      void *end_vaddr = NULL;
   if (end && !clif_lookup_vaddr(clif, end, &end_vaddr)) {
            out(clif, "Failed to look up address 0x%08x\n",
               if (!reloc_mode)
                  uint32_t size;
   uint8_t *cl = start_vaddr;
   while (clif_dump_packet(clif, start, cl, &size, reloc_mode)) {
                                       }
      /* Walks the worklist, parsing the relocs for any memory regions that might
   * themselves have additional relocations.
   */
   static uint32_t
   clif_dump_gl_shader_state_record(struct clif_dump *clif,
                     {
         struct v3d_group *state = v3d_spec_find_struct(clif->spec,
         struct v3d_group *attr = v3d_spec_find_struct(clif->spec,
         assert(state);
   assert(attr);
            if (including_gs) {
            struct v3d_group *gs_state = v3d_spec_find_struct(clif->spec,
         assert(gs_state);
   out(clif, "@format shadrec_gl_geom\n");
   v3d_print_group(clif, gs_state, 0, vaddr + offset);
   offset += v3d_group_get_length(gs_state);
      }
   out(clif, "@format shadrec_gl_main\n");
   v3d_print_group(clif, state, 0, vaddr + offset);
            for (int i = 0; i < reloc->shader_state.num_attrs; i++) {
            out(clif, "@format shadrec_gl_attr /* %d */\n", i);
               }
      static void
   clif_process_worklist(struct clif_dump *clif)
   {
         list_for_each_entry_safe(struct reloc_worklist_entry, reloc,
                  void *vaddr;
   if (!clif_lookup_vaddr(clif, reloc->addr, &vaddr)) {
                              switch (reloc->type) {
                        case reloc_gl_shader_state:
   case reloc_gl_including_gs_shader_state:
         case reloc_generic_tile_list:
            clif_dump_cl(clif, reloc->addr,
   }
      static int
   worklist_entry_compare(const void *a, const void *b)
   {
         return ((*(struct reloc_worklist_entry **)a)->addr -
   }
      static bool
   clif_dump_if_blank(struct clif_dump *clif, struct clif_bo *bo,
         {
         for (int i = start; i < end; i++) {
                        out(clif, "\n");
   out(clif, "@format blank %d /* [%s+0x%08x..0x%08x] */\n", end - start,
         }
      /* Dumps the binary data in the BO from start to end (relative to the start of
   * the BO).
   */
   static void
   clif_dump_binary(struct clif_dump *clif, struct clif_bo *bo,
         {
         if (clif->pretty && clif->nobin)
            if (start == end)
            if (clif_dump_if_blank(clif, bo, start, end))
            out(clif, "@format binary /* [%s+0x%08x] */\n",
            uint32_t offset = start;
   int dumped_in_line = 0;
   while (offset < end) {
                           if (end - offset >= 4) {
               } else {
                        if (++dumped_in_line == 8) {
            }
   if (dumped_in_line)
   }
      /* Walks the list of relocations, dumping each buffer's contents (using our
   * codegenned dump routines for pretty printing, and most importantly proper
   * address references so that the CLIF parser can relocate buffers).
   */
   static void
   clif_dump_buffers(struct clif_dump *clif)
   {
         int num_relocs = 0;
   list_for_each_entry(struct reloc_worklist_entry, reloc,
               }
   struct reloc_worklist_entry **relocs =
         int i = 0;
   list_for_each_entry(struct reloc_worklist_entry, reloc,
               }
            struct clif_bo *bo = NULL;
            for (i = 0; i < num_relocs; i++) {
                           if (!new_bo) {
                              if (new_bo != bo) {
            if (bo) {
                                    out(clif, "\n");
   out(clif, "@buffer %s\n", new_bo->name);
                     int reloc_offset = reloc->addr - bo->offset;
                        switch (reloc->type) {
   case reloc_cl:
                              case reloc_gl_shader_state:
   case reloc_gl_including_gs_shader_state:
            offset += clif_dump_gl_shader_state_record(clif,
                        case reloc_generic_tile_list:
            offset = clif_dump_cl(clif, reloc->addr,
                        if (bo) {
                  /* For any BOs that didn't have relocations, just dump them raw. */
   for (int i = 0; i < clif->bo_count; i++) {
            bo = &clif->bo[i];
   if (bo->dumped)
         out(clif, "@buffer %s\n", bo->name);
      }
      void
   clif_dump_add_cl(struct clif_dump *clif, uint32_t start, uint32_t end)
   {
         struct reloc_worklist_entry *entry =
            }
      static int
   clif_bo_offset_compare(const void *a, const void *b)
   {
         }
      void
   clif_dump(struct clif_dump *clif, const struct drm_v3d_submit_cl *submit)
   {
         clif_dump_add_cl(clif, submit->bcl_start, submit->bcl_end);
            qsort(clif->bo, clif->bo_count, sizeof(clif->bo[0]),
            /* A buffer needs to be defined before we can emit a CLIF address
      * referencing it, so emit them all now.
      for (int i = 0; i < clif->bo_count; i++) {
                  /* Walk the worklist figuring out the locations of structs based on
      * the CL contents.
               /* Dump the contents of the buffers using the relocations we found to
      * pretty-print structures.
               out(clif, "@add_bin 0\n  ");
   out_address(clif, submit->bcl_start);
   out(clif, "\n  ");
   out_address(clif, submit->bcl_end);
   out(clif, "\n  ");
   out_address(clif, submit->qma);
   out(clif, "\n  %d\n  ", submit->qms);
   out_address(clif, submit->qts);
   out(clif, "\n");
            out(clif, "@add_render 0\n  ");
   out_address(clif, submit->rcl_start);
   out(clif, "\n  ");
   out_address(clif, submit->rcl_end);
   out(clif, "\n  ");
   out_address(clif, submit->qma);
   out(clif, "\n");
   }
      void
   clif_dump_add_bo(struct clif_dump *clif, const char *name,
         {
         if (clif->bo_count >= clif->bo_array_size) {
            clif->bo_array_size = MAX2(4, clif->bo_array_size * 2);
               /* CLIF relocs use the buffer name, so make sure they're unique. */
   for (int i = 0; i < clif->bo_count; i++)
            clif->bo[clif->bo_count].name = ralloc_strdup(clif, name);
   clif->bo[clif->bo_count].offset = offset;
   clif->bo[clif->bo_count].size = size;
   clif->bo[clif->bo_count].vaddr = vaddr;
   clif->bo[clif->bo_count].dumped = false;
   }
