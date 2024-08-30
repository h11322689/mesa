      #include <stdbool.h>
      #include "context.h"
   #include "blend.h"
   #include "clip.h"
   #include "context.h"
   #include "depth.h"
   #include "fog.h"
      #include "light.h"
   #include "lines.h"
   #include "matrix.h"
   #include "multisample.h"
   #include "pixelstore.h"
   #include "points.h"
   #include "polygon.h"
   #include "readpix.h"
   #include "texparam.h"
   #include "viewport.h"
   #include "vbo/vbo.h"
   #include "api_exec_decl.h"
      void GL_APIENTRY
   _mesa_AlphaFuncx(GLenum func, GLclampx ref)
   {
         }
      void GL_APIENTRY
   _mesa_ClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)
   {
      _mesa_ClearColor((GLclampf) (red / 65536.0f),
                  }
      void GL_APIENTRY
   _mesa_ClearDepthx(GLclampx depth)
   {
         }
      void GL_APIENTRY
   _mesa_ClipPlanef(GLenum plane, const GLfloat *equation)
   {
      unsigned int i;
            for (i = 0; i < ARRAY_SIZE(converted_equation); i++) {
                     }
      void GL_APIENTRY
   _mesa_ClipPlanex(GLenum plane, const GLfixed *equation)
   {
      unsigned int i;
            for (i = 0; i < ARRAY_SIZE(converted_equation); i++) {
                     }
      void GL_APIENTRY
   _mesa_Color4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
   {
      _es_Color4f((GLfloat) (red / 65536.0f),
                  }
      void GL_APIENTRY
   _mesa_DepthRangex(GLclampx zNear, GLclampx zFar)
   {
      _mesa_DepthRangef((GLclampf) (zNear / 65536.0f),
      }
      void GL_APIENTRY
   _mesa_DrawTexxOES(GLfixed x, GLfixed y, GLfixed z, GLfixed w, GLfixed h)
   {
         _mesa_DrawTexfOES((GLfloat) (x / 65536.0f),
                  (GLfloat) (y / 65536.0f),
   }
      void GL_APIENTRY
   _mesa_DrawTexxvOES(const GLfixed *coords)
   {
      unsigned int i;
            for (i = 0; i < ARRAY_SIZE(converted_coords); i++) {
                     }
      void GL_APIENTRY
   _mesa_Fogx(GLenum pname, GLfixed param)
   {
      if (pname != GL_FOG_MODE) {
         } else {
               }
      void GL_APIENTRY
   _mesa_Fogxv(GLenum pname, const GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
   GLfloat converted_params[4];
            switch(pname) {
   case GL_FOG_MODE:
      convert_params_value = false;
   n_params = 1;
      case GL_FOG_COLOR:
      n_params = 4;
      case GL_FOG_DENSITY:
   case GL_FOG_START:
   case GL_FOG_END:
      n_params = 1;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     if (convert_params_value) {
      for (i = 0; i < n_params; i++) {
            } else {
      for (i = 0; i < n_params; i++) {
                        }
      void GL_APIENTRY
   _mesa_Frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
         {
      _mesa_Frustum((GLdouble) (left),
               (GLdouble) (right),
   (GLdouble) (bottom),
      }
      void GL_APIENTRY
   _mesa_Frustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top,
         {
      _mesa_Frustum((GLdouble) (left / 65536.0),
               (GLdouble) (right / 65536.0),
   (GLdouble) (bottom / 65536.0),
      }
      void GL_APIENTRY
   _mesa_GetClipPlanef(GLenum plane, GLfloat *equation)
   {
      unsigned int i;
            _mesa_GetClipPlane(plane, converted_equation);
   for (i = 0; i < ARRAY_SIZE(converted_equation); i++) {
            }
      void GL_APIENTRY
   _mesa_GetClipPlanex(GLenum plane, GLfixed *equation)
   {
      unsigned int i;
            _mesa_GetClipPlane(plane, converted_equation);
   for (i = 0; i < ARRAY_SIZE(converted_equation); i++) {
            }
      void GL_APIENTRY
   _mesa_GetLightxv(GLenum light, GLenum pname, GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
            if (light < GL_LIGHT0 || light > GL_LIGHT7) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
   switch(pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_POSITION:
      n_params = 4;
      case GL_SPOT_DIRECTION:
      n_params = 3;
      case GL_SPOT_EXPONENT:
   case GL_SPOT_CUTOFF:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION:
      n_params = 1;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     _mesa_GetLightfv(light, pname, converted_params);
   for (i = 0; i < n_params; i++) {
            }
      void GL_APIENTRY
   _mesa_GetMaterialxv(GLenum face, GLenum pname, GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
            switch(face) {
   case GL_FRONT:
   case GL_BACK:
         default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
   switch(pname) {
   case GL_SHININESS:
      n_params = 1;
      case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_EMISSION:
      n_params = 4;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     _mesa_GetMaterialfv(face, pname, converted_params);
   for (i = 0; i < n_params; i++) {
            }
      void GL_APIENTRY
   _mesa_GetTexEnvxv(GLenum target, GLenum pname, GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
   GLfloat converted_params[4];
            switch(target) {
   case GL_POINT_SPRITE:
      if (pname != GL_COORD_REPLACE) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
      case GL_TEXTURE_FILTER_CONTROL_EXT:
      if (pname != GL_TEXTURE_LOD_BIAS_EXT) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
      case GL_TEXTURE_ENV:
      if (pname != GL_TEXTURE_ENV_COLOR &&
      pname != GL_RGB_SCALE &&
   pname != GL_ALPHA_SCALE &&
   pname != GL_TEXTURE_ENV_MODE &&
   pname != GL_COMBINE_RGB &&
   pname != GL_COMBINE_ALPHA &&
   pname != GL_SRC0_RGB &&
   pname != GL_SRC1_RGB &&
   pname != GL_SRC2_RGB &&
   pname != GL_SRC0_ALPHA &&
   pname != GL_SRC1_ALPHA &&
   pname != GL_SRC2_ALPHA &&
   pname != GL_OPERAND0_RGB &&
   pname != GL_OPERAND1_RGB &&
   pname != GL_OPERAND2_RGB &&
   pname != GL_OPERAND0_ALPHA &&
   pname != GL_OPERAND1_ALPHA &&
   pname != GL_OPERAND2_ALPHA) {
   _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
   switch(pname) {
   case GL_COORD_REPLACE:
      convert_params_value = false;
   n_params = 1;
      case GL_TEXTURE_LOD_BIAS_EXT:
      n_params = 1;
      case GL_TEXTURE_ENV_COLOR:
      n_params = 4;
      case GL_RGB_SCALE:
   case GL_ALPHA_SCALE:
      n_params = 1;
      case GL_TEXTURE_ENV_MODE:
   case GL_COMBINE_RGB:
   case GL_COMBINE_ALPHA:
   case GL_SRC0_RGB:
   case GL_SRC1_RGB:
   case GL_SRC2_RGB:
   case GL_SRC0_ALPHA:
   case GL_SRC1_ALPHA:
   case GL_SRC2_ALPHA:
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
      convert_params_value = false;
   n_params = 1;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     _mesa_GetTexEnvfv(target, pname, converted_params);
   if (convert_params_value) {
      for (i = 0; i < n_params; i++) {
            } else {
      for (i = 0; i < n_params; i++) {
               }
      void GL_APIENTRY
   _mesa_GetTexGenxvOES(GLenum coord, GLenum pname, GLfixed *params)
   {
         }
      void GL_APIENTRY
   _mesa_GetTexParameterxv(GLenum target, GLenum pname, GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
   GLfloat converted_params[4];
            switch(target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_EXTERNAL_OES:
         default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
   switch(pname) {
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_GENERATE_MIPMAP:
      convert_params_value = false;
   n_params = 1;
      case GL_TEXTURE_CROP_RECT_OES:
      n_params = 4;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     _mesa_GetTexParameterfv(target, pname, converted_params);
   if (convert_params_value) {
      for (i = 0; i < n_params; i++) {
            } else {
      for (i = 0; i < n_params; i++) {
               }
      void GL_APIENTRY
   _mesa_LightModelx(GLenum pname, GLfixed param)
   {
         }
      void GL_APIENTRY
   _mesa_LightModelxv(GLenum pname, const GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
   GLfloat converted_params[4];
            switch(pname) {
   case GL_LIGHT_MODEL_AMBIENT:
      n_params = 4;
      case GL_LIGHT_MODEL_TWO_SIDE:
      convert_params_value = false;
   n_params = 1;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     if (convert_params_value) {
      for (i = 0; i < n_params; i++) {
            } else {
      for (i = 0; i < n_params; i++) {
                        }
      void GL_APIENTRY
   _mesa_Lightx(GLenum light, GLenum pname, GLfixed param)
   {
         }
      void GL_APIENTRY
   _mesa_Lightxv(GLenum light, GLenum pname, const GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
            if (light < GL_LIGHT0 || light > GL_LIGHT7) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
   switch(pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_POSITION:
      n_params = 4;
      case GL_SPOT_DIRECTION:
      n_params = 3;
      case GL_SPOT_EXPONENT:
   case GL_SPOT_CUTOFF:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION:
      n_params = 1;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     for (i = 0; i < n_params; i++) {
                     }
      void GL_APIENTRY
   _mesa_LineWidthx(GLfixed width)
   {
         }
      void GL_APIENTRY
   _mesa_LoadMatrixx(const GLfixed *m)
   {
      unsigned int i;
            for (i = 0; i < ARRAY_SIZE(converted_m); i++) {
                     }
      void GL_APIENTRY
   _mesa_Materialx(GLenum face, GLenum pname, GLfixed param)
   {
      if (face != GL_FRONT_AND_BACK) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     if (pname != GL_SHININESS) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                        }
      void GL_APIENTRY
   _mesa_Materialxv(GLenum face, GLenum pname, const GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
            if (face != GL_FRONT_AND_BACK) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     switch(pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_AMBIENT_AND_DIFFUSE:
   case GL_SPECULAR:
   case GL_EMISSION:
      n_params = 4;
      case GL_SHININESS:
      n_params = 1;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     for (i = 0; i < n_params; i++) {
                     }
      void GL_APIENTRY
   _mesa_MultMatrixx(const GLfixed *m)
   {
      unsigned int i;
            for (i = 0; i < ARRAY_SIZE(converted_m); i++) {
                     }
      void GL_APIENTRY
   _mesa_MultiTexCoord4x(GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
   {
      _es_MultiTexCoord4f(texture,
                        }
      void GL_APIENTRY
   _mesa_Normal3x(GLfixed nx, GLfixed ny, GLfixed nz)
   {
      _es_Normal3f((GLfloat) (nx / 65536.0f),
            }
      void GL_APIENTRY
   _mesa_Orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
         {
      _mesa_Ortho((GLdouble) (left),
               (GLdouble) (right),
   (GLdouble) (bottom),
      }
      void GL_APIENTRY
   _mesa_Orthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top,
         {
      _mesa_Ortho((GLdouble) (left / 65536.0),
               (GLdouble) (right / 65536.0),
   (GLdouble) (bottom / 65536.0),
      }
      void GL_APIENTRY
   _mesa_PointParameterx(GLenum pname, GLfixed param)
   {
         }
      void GL_APIENTRY
   _mesa_PointParameterxv(GLenum pname, const GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 3;
            switch(pname) {
   case GL_POINT_SIZE_MIN:
   case GL_POINT_SIZE_MAX:
   case GL_POINT_FADE_THRESHOLD_SIZE:
      n_params = 1;
      case GL_POINT_DISTANCE_ATTENUATION:
      n_params = 3;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     for (i = 0; i < n_params; i++) {
                     }
      void GL_APIENTRY
   _mesa_PointSizex(GLfixed size)
   {
         }
      void GL_APIENTRY
   _mesa_PolygonOffsetx(GLfixed factor, GLfixed units)
   {
      _mesa_PolygonOffset((GLfloat) (factor / 65536.0f),
      }
      void GL_APIENTRY
   _mesa_Rotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
   {
      _mesa_Rotatef((GLfloat) (angle / 65536.0f),
                  }
      void GL_APIENTRY
   _mesa_SampleCoveragex(GLclampx value, GLboolean invert)
   {
      _mesa_SampleCoverage((GLclampf) (value / 65536.0f),
      }
      void GL_APIENTRY
   _mesa_Scalex(GLfixed x, GLfixed y, GLfixed z)
   {
      _mesa_Scalef((GLfloat) (x / 65536.0f),
            }
      void GL_APIENTRY
   _mesa_TexEnvx(GLenum target, GLenum pname, GLfixed param)
   {
      switch(target) {
   case GL_POINT_SPRITE:
   case GL_TEXTURE_FILTER_CONTROL_EXT:
   case GL_TEXTURE_ENV:
         default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     switch(pname) {
   case GL_COORD_REPLACE:
   case GL_TEXTURE_ENV_MODE:
   case GL_COMBINE_RGB:
   case GL_COMBINE_ALPHA:
   case GL_SRC0_RGB:
   case GL_SRC1_RGB:
   case GL_SRC2_RGB:
   case GL_SRC0_ALPHA:
   case GL_SRC1_ALPHA:
   case GL_SRC2_ALPHA:
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
      _mesa_TexEnvf(target, pname, (GLfloat) param);
      case GL_TEXTURE_LOD_BIAS_EXT:
   case GL_RGB_SCALE:
   case GL_ALPHA_SCALE:
      _mesa_TexEnvf(target, pname, (GLfloat) (param / 65536.0f));
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
               }
      void GL_APIENTRY
   _mesa_TexEnvxv(GLenum target, GLenum pname, const GLfixed *params)
   {
      switch(target) {
   case GL_POINT_SPRITE:
   case GL_TEXTURE_FILTER_CONTROL_EXT:
   case GL_TEXTURE_ENV:
         default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     switch(pname) {
   case GL_COORD_REPLACE:
   case GL_TEXTURE_ENV_MODE:
   case GL_COMBINE_RGB:
   case GL_COMBINE_ALPHA:
   case GL_SRC0_RGB:
   case GL_SRC1_RGB:
   case GL_SRC2_RGB:
   case GL_SRC0_ALPHA:
   case GL_SRC1_ALPHA:
   case GL_SRC2_ALPHA:
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
      _mesa_TexEnvf(target, pname, (GLfloat) params[0]);
      case GL_TEXTURE_LOD_BIAS_EXT:
   case GL_RGB_SCALE:
   case GL_ALPHA_SCALE:
      _mesa_TexEnvf(target, pname, (GLfloat) (params[0] / 65536.0f));
      case GL_TEXTURE_ENV_COLOR: {
      unsigned int i;
            for (i = 0; i < ARRAY_SIZE(converted_params); i++) {
                  _mesa_TexEnvfv(target, pname, converted_params);
      }
   default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
               }
      static void
   _es_TexGenf(GLenum coord, GLenum pname, GLfloat param)
   {
      if (coord != GL_TEXTURE_GEN_STR_OES) {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error( ctx, GL_INVALID_ENUM, "glTexGen[fx](pname)" );
      }
   /* set S, T, and R at the same time */
   _mesa_TexGenf(GL_S, pname, param);
   _mesa_TexGenf(GL_T, pname, param);
      }
      void GL_APIENTRY
   _mesa_TexGenxOES(GLenum coord, GLenum pname, GLfixed param)
   {
         }
      void GL_APIENTRY
   _mesa_TexGenxvOES(GLenum coord, GLenum pname, const GLfixed *params)
   {
         }
      void GL_APIENTRY
   _mesa_TexParameterx(GLenum target, GLenum pname, GLfixed param)
   {
      if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) {
         } else {
            }
      void GL_APIENTRY
   _mesa_TexParameterxv(GLenum target, GLenum pname, const GLfixed *params)
   {
      unsigned int i;
   unsigned int n_params = 4;
   GLfloat converted_params[4];
            switch(target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_EXTERNAL_OES:
         default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
            }
   switch(pname) {
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      convert_params_value = false;
   n_params = 1;
      case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_GENERATE_MIPMAP:
      convert_params_value = false;
   n_params = 1;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      n_params = 1;
      case GL_TEXTURE_CROP_RECT_OES:
      n_params = 4;
      default:
      _mesa_error(_mesa_get_current_context(), GL_INVALID_ENUM,
                     if (convert_params_value) {
      for (i = 0; i < n_params; i++) {
            } else {
      for (i = 0; i < n_params; i++) {
                        }
      void GL_APIENTRY
   _mesa_Translatex(GLfixed x, GLfixed y, GLfixed z)
   {
      _mesa_Translatef((GLfloat) (x / 65536.0f),
            }
