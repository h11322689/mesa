   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      #include "packrender.h"
      /*
   ** Routines to pack evaluator maps into the transport buffer.  Maps are
   ** allowed to have extra arbitrary data, so these routines extract just
   ** the information that the GL needs.
   */
      void
   __glFillMap1f(GLint k, GLint order, GLint stride,
         {
      if (stride == k) {
      /* Just copy the data */
      }
   else {
               for (i = 0; i < order; i++) {
      __GLX_PUT_FLOAT_ARRAY(0, points, k);
   points += stride;
            }
      void
   __glFillMap1d(GLint k, GLint order, GLint stride,
         {
      if (stride == k) {
      /* Just copy the data */
      }
   else {
      GLint i;
   for (i = 0; i < order; i++) {
      __GLX_PUT_DOUBLE_ARRAY(0, points, k);
   points += stride;
            }
      void
   __glFillMap2f(GLint k, GLint majorOrder, GLint minorOrder,
               {
               if ((minorStride == k) && (majorStride == minorOrder * k)) {
      /* Just copy the data */
   __GLX_MEM_COPY(data, points, majorOrder * majorStride *
            }
   for (i = 0; i < majorOrder; i++) {
      for (j = 0; j < minorOrder; j++) {
      for (x = 0; x < k; x++) {
         }
   points += minorStride;
      }
         }
      void
   __glFillMap2d(GLint k, GLint majorOrder, GLint minorOrder,
               {
               if ((minorStride == k) && (majorStride == minorOrder * k)) {
      /* Just copy the data */
   __GLX_MEM_COPY(data, points, majorOrder * majorStride *
                  #ifdef __GLX_ALIGN64
         #endif
      for (i = 0; i < majorOrder; i++) {
      #ifdef __GLX_ALIGN64
         #else
            for (x = 0; x < k; x++) {
      #endif
            points += minorStride;
      }
         }
