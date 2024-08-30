   /*
   * Copyright © 2022 Mary Guillemard
   * SPDX-License-Identifier: MIT
   */
   #include "mme_fermi.h"
      #include "mme_fermi_isa.h"
   #include "isa.h"
      #include <stdlib.h>
      static void
   disasm_instr_cb(void *d, unsigned n, void *instr)
   {
         }
      void
   mme_fermi_dump(FILE *fp, uint32_t *encoded, size_t encoded_size)
   {
      const struct isa_decode_options opts = {
      .show_errors = true,
   .branch_labels = true,
   .cbdata = fp,
      };
      }
