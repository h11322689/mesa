   /*
   * Copyright © 2013 Intel Corporation
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
      #include "brw_compiler.h"
   #include "compiler/nir/nir.h"
      static char const *get_qual_name(int mode)
   {
      switch (mode) {
      case INTERP_MODE_NONE:          return "none";
   case INTERP_MODE_FLAT:          return "flat";
   case INTERP_MODE_SMOOTH:        return "smooth";
   case INTERP_MODE_NOPERSPECTIVE: return "nopersp";
         }
      static void
   gfx4_frag_prog_set_interp_modes(struct brw_wm_prog_data *prog_data,
                     {
      for (unsigned k = 0; k < slot_count; k++) {
      unsigned slot = vue_map->varying_to_slot[location + k];
   if (slot != -1 && prog_data->interp_mode[slot] == INTERP_MODE_NONE) {
               if (prog_data->interp_mode[slot] == INTERP_MODE_FLAT) {
         } else if (prog_data->interp_mode[slot] == INTERP_MODE_NOPERSPECTIVE) {
                  }
      /* Set up interpolation modes for every element in the VUE */
   void
   brw_setup_vue_interpolation(const struct brw_vue_map *vue_map, nir_shader *nir,
         {
      /* Initialise interp_mode. INTERP_MODE_NONE == 0 */
            if (!vue_map)
            /* HPOS always wants noperspective. setting it up here allows
   * us to not need special handling in the SF program.
   */
   unsigned pos_slot = vue_map->varying_to_slot[VARYING_SLOT_POS];
   if (pos_slot != -1) {;
      prog_data->interp_mode[pos_slot] = INTERP_MODE_NOPERSPECTIVE;
               nir_foreach_shader_in_variable(var, nir) {
      unsigned location = var->data.location;
            gfx4_frag_prog_set_interp_modes(prog_data, vue_map, location, slot_count,
            if (location == VARYING_SLOT_COL0 || location == VARYING_SLOT_COL1) {
      location = location + VARYING_SLOT_BFC0 - VARYING_SLOT_COL0;
   gfx4_frag_prog_set_interp_modes(prog_data, vue_map, location,
                  const bool debug = false;
   if (debug) {
      fprintf(stderr, "VUE map:\n");
   for (int i = 0; i < vue_map->num_slots; i++) {
      int varying = vue_map->slot_to_varying[i];
   if (varying == -1) {
      fprintf(stderr, "%d: --\n", i);
               fprintf(stderr, "%d: %d %s ofs %d\n",
         i, varying,
            }
