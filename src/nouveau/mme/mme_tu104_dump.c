   /*
   * Copyright © 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
   #include "mme_tu104.h"
      #include "mme_tu104_isa.h"
   #include "isa.h"
      #include <stdlib.h>
      static void
   disasm_instr_cb(void *d, unsigned n, void *instr)
   {
      uint32_t *dwords = (uint32_t *)instr;
      }
      void
   mme_tu104_dump(FILE *fp, uint32_t *encoded, size_t encoded_size)
   {
               uint32_t *swapped = malloc(encoded_size);
   for (uint32_t i = 0; i < (encoded_size / 12); i++) {
      swapped[i * 3 + 0] = encoded[i * 3 + 2];
   swapped[i * 3 + 1] = encoded[i * 3 + 1];
               const struct isa_decode_options opts = {
      .show_errors = true,
   .branch_labels = true,
   .cbdata = fp,
      };
               }
