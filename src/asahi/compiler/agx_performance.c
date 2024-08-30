   /*
   * Copyright 2023 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compiler.h"
      /* Table describing the relationship between registers pressure and thread
   * count. Each entry describes a maximum number of registers and the associated
   * best-case thread count.
   *
   * Sorted in ascending order of maximum registers for easy lookup.
   */
   static const struct agx_occupancy occupancies[] = {
      {104, 1024}, {112, 896}, {128, 832}, {136, 768}, {144, 704},
      };
      struct agx_occupancy
   agx_occupancy_for_register_count(unsigned halfregs)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(occupancies); ++i) {
      unsigned max = occupancies[i].max_registers;
            if (halfregs <= max)
                  }
