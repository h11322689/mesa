   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   *
   **************************************************************************/
         /**
   * Code to implement GL_OES_query_matrix.  See the spec at:
   * http://www.khronos.org/registry/gles/extensions/OES/OES_query_matrix.txt
   */
         #include <stdlib.h>
   #include <math.h>
      #include "util/glheader.h"
   #include "main/get.h"
   #include "util/macros.h"
   #include "api_exec_decl.h"
         /**
   * This is from the GL_OES_query_matrix extension specification:
   *
   *  GLbitfield glQueryMatrixxOES( GLfixed mantissa[16],
   *                                GLint   exponent[16] )
   *  mantissa[16] contains the contents of the current matrix in GLfixed
   *  format.  exponent[16] contains the unbiased exponents applied to the
   *  matrix components, so that the internal representation of component i
   *  is close to mantissa[i] * 2^exponent[i].  The function returns a status
   *  word which is zero if all the components are valid. If
   *  status & (1<<i) != 0, the component i is invalid (e.g., NaN, Inf).
   *  The implementations are not required to keep track of overflows.  In
   *  that case, the invalid bits are never set.
   */
      #define INT_TO_FIXED(x) ((GLfixed) ((x) << 16))
   #define FLOAT_TO_FIXED(x) ((GLfixed) ((x) * 65536.0))
         GLbitfield GLAPIENTRY
   _mesa_QueryMatrixxOES(GLfixed *mantissa, GLint *exponent)
   {
      GLfloat matrix[16];
   GLint tmp;
   GLenum currentMode = GL_FALSE;
   GLenum desiredMatrix = GL_FALSE;
   /* The bitfield returns 1 for each component that is invalid (i.e.
   * NaN or Inf).  In case of error, everything is invalid.
   */
   GLbitfield rv;
            /* This data structure defines the mapping between the current matrix
   * mode and the desired matrix identifier.
   */
   static const struct {
      GLenum currentMode;
      } modes[] = {
      {GL_MODELVIEW, GL_MODELVIEW_MATRIX},
   {GL_PROJECTION, GL_PROJECTION_MATRIX},
               /* Call Mesa to get the current matrix in floating-point form.  First,
   * we have to figure out what the current matrix mode is.
   */
   _mesa_GetIntegerv(GL_MATRIX_MODE, &tmp);
            /* The mode is either GL_FALSE, if for some reason we failed to query
   * the mode, or a given mode from the above table.  Search for the
   * returned mode to get the desired matrix; if we don't find it,
   * we can return immediately, as _mesa_GetInteger() will have
   * logged the necessary error already.
   */
   for (i = 0; i < ARRAY_SIZE(modes); i++) {
      if (modes[i].currentMode == currentMode) {
      desiredMatrix = modes[i].desiredMatrix;
         }
   if (desiredMatrix == GL_FALSE) {
      /* Early error means all values are invalid. */
               /* Now pull the matrix itself. */
            rv = 0;
   for (i = 0, bit = 1; i < 16; i++, bit<<=1) {
      float normalizedFraction;
            switch (fpclassify(matrix[i])) {
   case FP_SUBNORMAL:
   case FP_NORMAL:
   case FP_ZERO:
      /* A "subnormal" or denormalized number is too small to be
   * represented in normal format; but despite that it's a
   * valid floating point number.  FP_ZERO and FP_NORMAL
   * are both valid as well.  We should be fine treating
   * these three cases as legitimate floating-point numbers.
   */
   normalizedFraction = (GLfloat)frexp(matrix[i], &exp);
   mantissa[i] = FLOAT_TO_FIXED(normalizedFraction);
               case FP_NAN:
      /* If the entry is not-a-number or an infinity, then the
   * matrix component is invalid.  The invalid flag for
   * the component is already set; might as well set the
   * other return values to known values.  We'll set
   * distinct values so that a savvy end user could determine
   * whether the matrix component was a NaN or an infinity,
   * but this is more useful for debugging than anything else
   * since the standard doesn't specify any such magic
   * values to return.
   */
   mantissa[i] = INT_TO_FIXED(0);
   exponent[i] = (GLint) 0;
               case FP_INFINITE:
      /* Return +/- 1 based on whether it's a positive or
   * negative infinity.
   */
   if (matrix[i] > 0) {
         }
   else {
         }
   exponent[i] = (GLint) 0;
               default:
      /* We should never get here; but here's a catching case
   * in case fpclassify() is returnings something unexpected.
   */
   mantissa[i] = INT_TO_FIXED(2);
   exponent[i] = (GLint) 0;
   rv |= bit;
                     /* All done */
      }
