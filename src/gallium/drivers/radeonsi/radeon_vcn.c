   /*
   * Copyright © 2022 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "radeon_vcn.h"
      /* vcn unified queue (sq) ib header */
   void rvcn_sq_header(struct radeon_cmdbuf *cs,
               {
      /* vcn ib signature */
   radeon_emit(cs, RADEON_VCN_SIGNATURE_SIZE);
   radeon_emit(cs, RADEON_VCN_SIGNATURE);
   sq->ib_checksum = &cs->current.buf[cs->current.cdw];
   radeon_emit(cs, 0);
   sq->ib_total_size_in_dw = &cs->current.buf[cs->current.cdw];
            /* vcn ib engine info */
   radeon_emit(cs, RADEON_VCN_ENGINE_INFO_SIZE);
   radeon_emit(cs, RADEON_VCN_ENGINE_INFO);
   radeon_emit(cs, enc ? RADEON_VCN_ENGINE_TYPE_ENCODE
            }
      void rvcn_sq_tail(struct radeon_cmdbuf *cs,
         {
      uint32_t *end;
   uint32_t size_in_dw;
            if (sq->ib_checksum == NULL || sq->ib_total_size_in_dw == NULL)
            end = &cs->current.buf[cs->current.cdw];
   size_in_dw = end - sq->ib_total_size_in_dw - 1;
   *sq->ib_total_size_in_dw = size_in_dw;
            for (int i = 0; i < size_in_dw; i++)
               }
