   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compiler.h"
      bool
   agx_allows_16bit_immediate(agx_instr *I)
   {
      return (I->op == AGX_OPCODE_DEVICE_LOAD) ||
            }
