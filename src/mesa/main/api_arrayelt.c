   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      /**
   * This file implements the glArrayElement() function.
   * It involves looking at the format/type of all the enabled vertex arrays
   * and emitting a list of pointers to functions which set the per-vertex
   * state for the element/index.
   */
         /* Author:
   *    Keith Whitwell <keithw@vmware.com>
   */
      #include "util/glheader.h"
   #include "arrayobj.h"
   #include "api_arrayelt.h"
   #include "bufferobj.h"
   #include "context.h"
      #include "macros.h"
   #include "mtypes.h"
   #include "main/dispatch.h"
   #include "varray.h"
   #include "api_exec_decl.h"
      typedef void (GLAPIENTRY *attrib_func)( GLuint indx, const void *data );
      /*
   * Convert GL_BYTE, GL_UNSIGNED_BYTE, .. GL_DOUBLE into an integer
   * in the range [0, 7].  Luckily these type tokens are sequentially
   * numbered in gl.h, except for GL_DOUBLE.
   */
   static inline int
   TYPE_IDX(GLenum t)
   {
         }
         /*
   * Convert normalized/integer/double to the range [0, 3].
   */
   static inline int
   vertex_format_to_index(const struct gl_vertex_format *vformat)
   {
      if (vformat->User.Doubles)
         else if (vformat->User.Integer)
         else if (vformat->User.Normalized)
         else
      }
         #define NUM_TYPES 8
      static struct _glapi_table *
   get_dispatch(void)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   ** GL_NV_vertex_program
   **/
      /* GL_BYTE attributes */
      static void GLAPIENTRY
   VertexAttrib1NbvNV(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1bvNV(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NbvNV(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2bvNV(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3NbvNV(GLuint index, const GLbyte *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, BYTE_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3bvNV(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib4NbvNV(GLuint index, const GLbyte *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, BYTE_TO_FLOAT(v[0]),
            BYTE_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib4bvNV(GLuint index, const GLbyte *v)
   {
         }
      /* GL_UNSIGNED_BYTE attributes */
      static void GLAPIENTRY
   VertexAttrib1NubvNV(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1ubvNV(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NubvNV(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, UBYTE_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2ubvNV(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3NubvNV(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, UBYTE_TO_FLOAT(v[0]),
            }
   static void GLAPIENTRY
   VertexAttrib3ubvNV(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib4NubvNV(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, UBYTE_TO_FLOAT(v[0]),
                  }
      static void GLAPIENTRY
   VertexAttrib4ubvNV(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, (GLfloat)v[0],
            }
      /* GL_SHORT attributes */
      static void GLAPIENTRY
   VertexAttrib1NsvNV(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1svNV(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NsvNV(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, SHORT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2svNV(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3NsvNV(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, SHORT_TO_FLOAT(v[0]),
      SHORT_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib3svNV(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      static void GLAPIENTRY
   VertexAttrib4NsvNV(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, SHORT_TO_FLOAT(v[0]),
      SHORT_TO_FLOAT(v[1]),
   SHORT_TO_FLOAT(v[2]),
   }
      static void GLAPIENTRY
   VertexAttrib4svNV(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_UNSIGNED_SHORT attributes */
      static void GLAPIENTRY
   VertexAttrib1NusvNV(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1usvNV(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NusvNV(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, USHORT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2usvNV(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib3NusvNV(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, USHORT_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3usvNV(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      static void GLAPIENTRY
   VertexAttrib4NusvNV(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, USHORT_TO_FLOAT(v[0]),
            USHORT_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib4usvNV(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_INT attributes */
      static void GLAPIENTRY
   VertexAttrib1NivNV(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1ivNV(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NivNV(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, INT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2ivNV(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3NivNV(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, INT_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3ivNV(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      static void GLAPIENTRY
   VertexAttrib4NivNV(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, INT_TO_FLOAT(v[0]),
                  }
      static void GLAPIENTRY
   VertexAttrib4ivNV(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_UNSIGNED_INT attributes */
      static void GLAPIENTRY
   VertexAttrib1NuivNV(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1uivNV(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NuivNV(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, UINT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2uivNV(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib2fNV(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib3NuivNV(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, UINT_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3uivNV(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib3fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      static void GLAPIENTRY
   VertexAttrib4NuivNV(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, UINT_TO_FLOAT(v[0]),
            UINT_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib4uivNV(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib4fNV(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_FLOAT attributes */
      static void GLAPIENTRY
   VertexAttrib1fvNV(GLuint index, const GLfloat *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2fvNV(GLuint index, const GLfloat *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3fvNV(GLuint index, const GLfloat *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib4fvNV(GLuint index, const GLfloat *v)
   {
         }
      /* GL_DOUBLE attributes */
      static void GLAPIENTRY
   VertexAttrib1dvNV(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2dvNV(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3dvNV(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib4dvNV(GLuint index, const GLdouble *v)
   {
         }
         /*
   * Array [size][type] of VertexAttrib functions
   */
   static const attrib_func AttribFuncsNV[2][4][NUM_TYPES] = {
      {
      /* non-normalized */
   {
      /* size 1 */
   (attrib_func) VertexAttrib1bvNV,
   (attrib_func) VertexAttrib1ubvNV,
   (attrib_func) VertexAttrib1svNV,
   (attrib_func) VertexAttrib1usvNV,
   (attrib_func) VertexAttrib1ivNV,
   (attrib_func) VertexAttrib1uivNV,
   (attrib_func) VertexAttrib1fvNV,
      },
   {
      /* size 2 */
   (attrib_func) VertexAttrib2bvNV,
   (attrib_func) VertexAttrib2ubvNV,
   (attrib_func) VertexAttrib2svNV,
   (attrib_func) VertexAttrib2usvNV,
   (attrib_func) VertexAttrib2ivNV,
   (attrib_func) VertexAttrib2uivNV,
   (attrib_func) VertexAttrib2fvNV,
      },
   {
      /* size 3 */
   (attrib_func) VertexAttrib3bvNV,
   (attrib_func) VertexAttrib3ubvNV,
   (attrib_func) VertexAttrib3svNV,
   (attrib_func) VertexAttrib3usvNV,
   (attrib_func) VertexAttrib3ivNV,
   (attrib_func) VertexAttrib3uivNV,
   (attrib_func) VertexAttrib3fvNV,
      },
   {
      /* size 4 */
   (attrib_func) VertexAttrib4bvNV,
   (attrib_func) VertexAttrib4ubvNV,
   (attrib_func) VertexAttrib4svNV,
   (attrib_func) VertexAttrib4usvNV,
   (attrib_func) VertexAttrib4ivNV,
   (attrib_func) VertexAttrib4uivNV,
   (attrib_func) VertexAttrib4fvNV,
         },
   {
      /* normalized (except for float/double) */
   {
      /* size 1 */
   (attrib_func) VertexAttrib1NbvNV,
   (attrib_func) VertexAttrib1NubvNV,
   (attrib_func) VertexAttrib1NsvNV,
   (attrib_func) VertexAttrib1NusvNV,
   (attrib_func) VertexAttrib1NivNV,
   (attrib_func) VertexAttrib1NuivNV,
   (attrib_func) VertexAttrib1fvNV,
      },
   {
      /* size 2 */
   (attrib_func) VertexAttrib2NbvNV,
   (attrib_func) VertexAttrib2NubvNV,
   (attrib_func) VertexAttrib2NsvNV,
   (attrib_func) VertexAttrib2NusvNV,
   (attrib_func) VertexAttrib2NivNV,
   (attrib_func) VertexAttrib2NuivNV,
   (attrib_func) VertexAttrib2fvNV,
      },
   {
      /* size 3 */
   (attrib_func) VertexAttrib3NbvNV,
   (attrib_func) VertexAttrib3NubvNV,
   (attrib_func) VertexAttrib3NsvNV,
   (attrib_func) VertexAttrib3NusvNV,
   (attrib_func) VertexAttrib3NivNV,
   (attrib_func) VertexAttrib3NuivNV,
   (attrib_func) VertexAttrib3fvNV,
      },
   {
      /* size 4 */
   (attrib_func) VertexAttrib4NbvNV,
   (attrib_func) VertexAttrib4NubvNV,
   (attrib_func) VertexAttrib4NsvNV,
   (attrib_func) VertexAttrib4NusvNV,
   (attrib_func) VertexAttrib4NivNV,
   (attrib_func) VertexAttrib4NuivNV,
   (attrib_func) VertexAttrib4fvNV,
            };
         /**
   ** GL_ARB_vertex_program
   **/
      /* GL_BYTE attributes */
      static void GLAPIENTRY
   VertexAttrib1NbvARB(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1bvARB(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NbvARB(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2bvARB(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3NbvARB(GLuint index, const GLbyte *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, BYTE_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3bvARB(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib4NbvARB(GLuint index, const GLbyte *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, BYTE_TO_FLOAT(v[0]),
            BYTE_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib4bvARB(GLuint index, const GLbyte *v)
   {
         }
      /* GL_UNSIGNED_BYTE attributes */
      static void GLAPIENTRY
   VertexAttrib1NubvARB(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1ubvARB(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NubvARB(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index,
            }
      static void GLAPIENTRY
   VertexAttrib2ubvARB(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index,
      }
      static void GLAPIENTRY
   VertexAttrib3NubvARB(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index,
                  }
   static void GLAPIENTRY
   VertexAttrib3ubvARB(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index,
                  }
      static void GLAPIENTRY
   VertexAttrib4NubvARB(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(),
                        (index,
   }
      static void GLAPIENTRY
   VertexAttrib4ubvARB(GLuint index, const GLubyte *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(),
                  }
      /* GL_SHORT attributes */
      static void GLAPIENTRY
   VertexAttrib1NsvARB(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1svARB(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NsvARB(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(),
            }
      static void GLAPIENTRY
   VertexAttrib2svARB(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(),
      }
      static void GLAPIENTRY
   VertexAttrib3NsvARB(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(),
                        }
      static void GLAPIENTRY
   VertexAttrib3svARB(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(),
            }
      static void GLAPIENTRY
   VertexAttrib4NsvARB(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(),
                        (index,
   }
      static void GLAPIENTRY
   VertexAttrib4svARB(GLuint index, const GLshort *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_UNSIGNED_SHORT attributes */
      static void GLAPIENTRY
   VertexAttrib1NusvARB(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1usvARB(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NusvARB(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index, USHORT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2usvARB(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib3NusvARB(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, USHORT_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3usvARB(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib4NusvARB(GLuint index, const GLushort *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, USHORT_TO_FLOAT(v[0]),
            USHORT_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib4usvARB(GLuint index, const GLushort *v)
   {
         }
      /* GL_INT attributes */
      static void GLAPIENTRY
   VertexAttrib1NivARB(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1ivARB(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NivARB(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index, INT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2ivARB(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib3NivARB(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, INT_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3ivARB(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib4NivARB(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, INT_TO_FLOAT(v[0]),
            INT_TO_FLOAT(v[1]),
   }
      static void GLAPIENTRY
   VertexAttrib4ivARB(GLuint index, const GLint *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_UNSIGNED_INT attributes */
      static void GLAPIENTRY
   VertexAttrib1NuivARB(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib1uivARB(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2NuivARB(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index, UINT_TO_FLOAT(v[0]),
      }
      static void GLAPIENTRY
   VertexAttrib2uivARB(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib2fARB(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib3NuivARB(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, UINT_TO_FLOAT(v[0]),
            }
      static void GLAPIENTRY
   VertexAttrib3uivARB(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib3fARB(get_dispatch(), (index, (GLfloat)v[0],
      }
      static void GLAPIENTRY
   VertexAttrib4NuivARB(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, UINT_TO_FLOAT(v[0]),
                  }
      static void GLAPIENTRY
   VertexAttrib4uivARB(GLuint index, const GLuint *v)
   {
      CALL_VertexAttrib4fARB(get_dispatch(), (index, (GLfloat)v[0], (GLfloat)v[1],
      }
      /* GL_FLOAT attributes */
      static void GLAPIENTRY
   VertexAttrib1fvARB(GLuint index, const GLfloat *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2fvARB(GLuint index, const GLfloat *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3fvARB(GLuint index, const GLfloat *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib4fvARB(GLuint index, const GLfloat *v)
   {
         }
      /* GL_DOUBLE attributes */
      static void GLAPIENTRY
   VertexAttrib1dvARB(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib2dvARB(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib3dvARB(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttrib4dvARB(GLuint index, const GLdouble *v)
   {
         }
         /**
   * Integer-valued attributes
   */
   static void GLAPIENTRY
   VertexAttribI1bv(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI2bv(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI3bv(GLuint index, const GLbyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI4bv(GLuint index, const GLbyte *v)
   {
         }
         static void GLAPIENTRY
   VertexAttribI1ubv(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI2ubv(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI3ubv(GLuint index, const GLubyte *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI4ubv(GLuint index, const GLubyte *v)
   {
         }
            static void GLAPIENTRY
   VertexAttribI1sv(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI2sv(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI3sv(GLuint index, const GLshort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI4sv(GLuint index, const GLshort *v)
   {
         }
         static void GLAPIENTRY
   VertexAttribI1usv(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI2usv(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI3usv(GLuint index, const GLushort *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI4usv(GLuint index, const GLushort *v)
   {
         }
            static void GLAPIENTRY
   VertexAttribI1iv(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI2iv(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI3iv(GLuint index, const GLint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI4iv(GLuint index, const GLint *v)
   {
         }
         static void GLAPIENTRY
   VertexAttribI1uiv(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI2uiv(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI3uiv(GLuint index, const GLuint *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribI4uiv(GLuint index, const GLuint *v)
   {
         }
      /* GL_DOUBLE unconverted attributes */
      static void GLAPIENTRY
   VertexAttribL1dv(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribL2dv(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribL3dv(GLuint index, const GLdouble *v)
   {
         }
      static void GLAPIENTRY
   VertexAttribL4dv(GLuint index, const GLdouble *v)
   {
         }
      /*
   * Array [unnormalized/normalized/integer][size][type] of VertexAttrib
   * functions
   */
   static const attrib_func AttribFuncsARB[4][4][NUM_TYPES] = {
      {
      /* non-normalized */
   {
      /* size 1 */
   (attrib_func) VertexAttrib1bvARB,
   (attrib_func) VertexAttrib1ubvARB,
   (attrib_func) VertexAttrib1svARB,
   (attrib_func) VertexAttrib1usvARB,
   (attrib_func) VertexAttrib1ivARB,
   (attrib_func) VertexAttrib1uivARB,
   (attrib_func) VertexAttrib1fvARB,
      },
   {
      /* size 2 */
   (attrib_func) VertexAttrib2bvARB,
   (attrib_func) VertexAttrib2ubvARB,
   (attrib_func) VertexAttrib2svARB,
   (attrib_func) VertexAttrib2usvARB,
   (attrib_func) VertexAttrib2ivARB,
   (attrib_func) VertexAttrib2uivARB,
   (attrib_func) VertexAttrib2fvARB,
      },
   {
      /* size 3 */
   (attrib_func) VertexAttrib3bvARB,
   (attrib_func) VertexAttrib3ubvARB,
   (attrib_func) VertexAttrib3svARB,
   (attrib_func) VertexAttrib3usvARB,
   (attrib_func) VertexAttrib3ivARB,
   (attrib_func) VertexAttrib3uivARB,
   (attrib_func) VertexAttrib3fvARB,
      },
   {
      /* size 4 */
   (attrib_func) VertexAttrib4bvARB,
   (attrib_func) VertexAttrib4ubvARB,
   (attrib_func) VertexAttrib4svARB,
   (attrib_func) VertexAttrib4usvARB,
   (attrib_func) VertexAttrib4ivARB,
   (attrib_func) VertexAttrib4uivARB,
   (attrib_func) VertexAttrib4fvARB,
         },
   {
      /* normalized (except for float/double) */
   {
      /* size 1 */
   (attrib_func) VertexAttrib1NbvARB,
   (attrib_func) VertexAttrib1NubvARB,
   (attrib_func) VertexAttrib1NsvARB,
   (attrib_func) VertexAttrib1NusvARB,
   (attrib_func) VertexAttrib1NivARB,
   (attrib_func) VertexAttrib1NuivARB,
   (attrib_func) VertexAttrib1fvARB,
      },
   {
      /* size 2 */
   (attrib_func) VertexAttrib2NbvARB,
   (attrib_func) VertexAttrib2NubvARB,
   (attrib_func) VertexAttrib2NsvARB,
   (attrib_func) VertexAttrib2NusvARB,
   (attrib_func) VertexAttrib2NivARB,
   (attrib_func) VertexAttrib2NuivARB,
   (attrib_func) VertexAttrib2fvARB,
      },
   {
      /* size 3 */
   (attrib_func) VertexAttrib3NbvARB,
   (attrib_func) VertexAttrib3NubvARB,
   (attrib_func) VertexAttrib3NsvARB,
   (attrib_func) VertexAttrib3NusvARB,
   (attrib_func) VertexAttrib3NivARB,
   (attrib_func) VertexAttrib3NuivARB,
   (attrib_func) VertexAttrib3fvARB,
      },
   {
      /* size 4 */
   (attrib_func) VertexAttrib4NbvARB,
   (attrib_func) VertexAttrib4NubvARB,
   (attrib_func) VertexAttrib4NsvARB,
   (attrib_func) VertexAttrib4NusvARB,
   (attrib_func) VertexAttrib4NivARB,
   (attrib_func) VertexAttrib4NuivARB,
   (attrib_func) VertexAttrib4fvARB,
                  {
      /* integer-valued */
   {
      /* size 1 */
   (attrib_func) VertexAttribI1bv,
   (attrib_func) VertexAttribI1ubv,
   (attrib_func) VertexAttribI1sv,
   (attrib_func) VertexAttribI1usv,
   (attrib_func) VertexAttribI1iv,
   (attrib_func) VertexAttribI1uiv,
   NULL, /* GL_FLOAT */
      },
   {
      /* size 2 */
   (attrib_func) VertexAttribI2bv,
   (attrib_func) VertexAttribI2ubv,
   (attrib_func) VertexAttribI2sv,
   (attrib_func) VertexAttribI2usv,
   (attrib_func) VertexAttribI2iv,
   (attrib_func) VertexAttribI2uiv,
   NULL, /* GL_FLOAT */
      },
   {
      /* size 3 */
   (attrib_func) VertexAttribI3bv,
   (attrib_func) VertexAttribI3ubv,
   (attrib_func) VertexAttribI3sv,
   (attrib_func) VertexAttribI3usv,
   (attrib_func) VertexAttribI3iv,
   (attrib_func) VertexAttribI3uiv,
   NULL, /* GL_FLOAT */
      },
   {
      /* size 4 */
   (attrib_func) VertexAttribI4bv,
   (attrib_func) VertexAttribI4ubv,
   (attrib_func) VertexAttribI4sv,
   (attrib_func) VertexAttribI4usv,
   (attrib_func) VertexAttribI4iv,
   (attrib_func) VertexAttribI4uiv,
   NULL, /* GL_FLOAT */
         },
   {
      /* double-valued */
   {
      /* size 1 */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
      },
   {
      /* size 2 */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
      },
   {
      /* size 3 */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
      },
   {
      /* size 4 */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
               };
         /*
   * Return VertexAttrib*NV function pointer matching the provided vertex format.
   */
   static inline attrib_func
   func_nv(const struct gl_vertex_format *vformat)
   {
      return AttribFuncsNV[vformat->User.Normalized][vformat->User.Size-1]
      }
         /*
   * Return VertexAttrib*ARB function pointer matching the provided vertex format.
   */
   static inline attrib_func
   func_arb(const struct gl_vertex_format *vformat)
   {
      return AttribFuncsARB[vertex_format_to_index(vformat)][vformat->User.Size-1]
      }
         /*
   * Return the address of the array attribute array at elt in the
   * vertex array object vao.
   */
   static inline const void *
   attrib_src(const struct gl_vertex_array_object *vao,
         {
      const struct gl_vertex_buffer_binding *binding =
                  if (binding->BufferObj) {
      src = ADD_POINTERS(binding->BufferObj->Mappings[MAP_INTERNAL].Pointer,
                  }
         void
   _mesa_array_element(struct gl_context *ctx, GLint elt)
   {
      const struct gl_vertex_array_object *vao = ctx->Array.VAO;
            /* emit conventional arrays elements */
   mask = (VERT_BIT_FF_ALL & ~VERT_BIT_POS) & vao->Enabled;
   while (mask) {
      const gl_vert_attrib attrib = u_bit_scan(&mask);
   const struct gl_array_attributes *array = &vao->VertexAttrib[attrib];
   const void *src = attrib_src(vao, array, elt);
               /* emit generic attribute elements */
   mask = (VERT_BIT_GENERIC_ALL & ~VERT_BIT_GENERIC0) & vao->Enabled;
   while (mask) {
      const gl_vert_attrib attrib = u_bit_scan(&mask);
   const struct gl_array_attributes *array = &vao->VertexAttrib[attrib];
   const void *src = attrib_src(vao, array, elt);
               /* finally, vertex position */
   if (vao->Enabled & VERT_BIT_GENERIC0) {
      const gl_vert_attrib attrib = VERT_ATTRIB_GENERIC0;
   const struct gl_array_attributes *array = &vao->VertexAttrib[attrib];
   const void *src = attrib_src(vao, array, elt);
      } else if (vao->Enabled & VERT_BIT_POS) {
      const gl_vert_attrib attrib = VERT_ATTRIB_POS;
   const struct gl_array_attributes *array = &vao->VertexAttrib[attrib];
   const void *src = attrib_src(vao, array, elt);
         }
         /**
   * Called via glArrayElement() and glDrawArrays().
   * Issue the glNormal, glVertex, glColor, glVertexAttrib, etc functions
   * for all enabled vertex arrays (for elt-th element).
   * Note: this may be called during display list construction.
   */
   void GLAPIENTRY
   _mesa_ArrayElement(GLint elt)
   {
      GET_CURRENT_CONTEXT(ctx);
            /* If PrimitiveRestart is enabled and the index is the RestartIndex
   * then we call PrimitiveRestartNV and return.
   */
   if (ctx->Array.PrimitiveRestart && (elt == ctx->Array.RestartIndex)) {
      CALL_PrimitiveRestartNV(ctx->Dispatch.Current, ());
               vao = ctx->Array.VAO;
                        }
