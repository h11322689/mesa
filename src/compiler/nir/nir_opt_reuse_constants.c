   /*
   * Copyright 2023 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "nir.h"
   #include "nir_instr_set.h"
      bool
   nir_opt_reuse_constants(nir_shader *shader)
   {
               struct set *consts = nir_instr_set_create(NULL);
   nir_foreach_function_impl(impl, shader) {
               nir_block *start_block = nir_start_block(impl);
            nir_foreach_block_safe(block, impl) {
      const bool in_start_block = start_block == block;
   nir_foreach_instr_safe(instr, block) {
                     struct set_entry *entry = _mesa_set_search(consts, instr);
   if (!entry) {
      if (!in_start_block)
                                    if (func_progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                     nir_instr_set_destroy(consts);
      }
