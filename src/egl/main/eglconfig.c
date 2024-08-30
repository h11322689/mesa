   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
   * Copyright 2010-2011 LunarG, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * EGL Configuration (pixel format) functions.
   */
      #include <assert.h>
   #include <stdlib.h>
   #include <string.h>
   #include "util/macros.h"
      #include "eglconfig.h"
   #include "eglconfigdebug.h"
   #include "eglcurrent.h"
   #include "egldisplay.h"
   #include "egllog.h"
      /**
   * Init the given _EGLconfig to default values.
   * \param id  the configuration's ID.
   *
   * Note that id must be positive for the config to be valid.
   * It is also recommended that when there are N configs, their
   * IDs are from 1 to N respectively.
   */
   void
   _eglInitConfig(_EGLConfig *conf, _EGLDisplay *disp, EGLint id)
   {
                        /* some attributes take non-zero default values */
   conf->ConfigID = id;
   conf->ConfigCaveat = EGL_NONE;
   conf->TransparentType = EGL_NONE;
   conf->NativeVisualType = EGL_NONE;
   conf->ColorBufferType = EGL_RGB_BUFFER;
      }
      /**
   * Link a config to its display and return the handle of the link.
   * The handle can be passed to client directly.
   *
   * Note that we just save the ptr to the config (we don't copy the config).
   */
   EGLConfig
   _eglLinkConfig(_EGLConfig *conf)
   {
               /* sanity check */
   assert(disp);
            if (!disp->Configs) {
      disp->Configs = _eglCreateArray("Config", 16);
   if (!disp->Configs)
                           }
      /**
   * Lookup a handle to find the linked config.
   * Return NULL if the handle has no corresponding linked config.
   */
   _EGLConfig *
   _eglLookupConfig(EGLConfig config, _EGLDisplay *disp)
   {
               if (!disp)
            conf = (_EGLConfig *)_eglFindArray(disp->Configs, (void *)config);
   if (conf)
               }
      enum type {
      ATTRIB_TYPE_INTEGER,
   ATTRIB_TYPE_BOOLEAN,
   ATTRIB_TYPE_BITMASK,
   ATTRIB_TYPE_ENUM,
   ATTRIB_TYPE_PSEUDO,   /* non-queryable */
      };
      enum criterion {
      ATTRIB_CRITERION_EXACT,
   ATTRIB_CRITERION_ATLEAST,
   ATTRIB_CRITERION_MASK,
   ATTRIB_CRITERION_SPECIAL,
      };
      /* EGL spec Table 3.1 and 3.4 */
   static const struct {
      EGLint attr;
   enum type type;
   enum criterion criterion;
      } _eglValidationTable[] = {
      /* clang-format off */
   /* core */
   { EGL_BUFFER_SIZE,               ATTRIB_TYPE_INTEGER,
               { EGL_RED_SIZE,                  ATTRIB_TYPE_INTEGER,
               { EGL_GREEN_SIZE,                ATTRIB_TYPE_INTEGER,
               { EGL_BLUE_SIZE,                 ATTRIB_TYPE_INTEGER,
               { EGL_LUMINANCE_SIZE,            ATTRIB_TYPE_INTEGER,
               { EGL_ALPHA_SIZE,                ATTRIB_TYPE_INTEGER,
               { EGL_ALPHA_MASK_SIZE,           ATTRIB_TYPE_INTEGER,
               { EGL_BIND_TO_TEXTURE_RGB,       ATTRIB_TYPE_BOOLEAN,
               { EGL_BIND_TO_TEXTURE_RGBA,      ATTRIB_TYPE_BOOLEAN,
               { EGL_COLOR_BUFFER_TYPE,         ATTRIB_TYPE_ENUM,
               { EGL_CONFIG_CAVEAT,             ATTRIB_TYPE_ENUM,
               { EGL_CONFIG_ID,                 ATTRIB_TYPE_INTEGER,
               { EGL_CONFORMANT,                ATTRIB_TYPE_BITMASK,
               { EGL_DEPTH_SIZE,                ATTRIB_TYPE_INTEGER,
               { EGL_LEVEL,                     ATTRIB_TYPE_PLATFORM,
               { EGL_MAX_PBUFFER_WIDTH,         ATTRIB_TYPE_INTEGER,
               { EGL_MAX_PBUFFER_HEIGHT,        ATTRIB_TYPE_INTEGER,
               { EGL_MAX_PBUFFER_PIXELS,        ATTRIB_TYPE_INTEGER,
               { EGL_MAX_SWAP_INTERVAL,         ATTRIB_TYPE_INTEGER,
               { EGL_MIN_SWAP_INTERVAL,         ATTRIB_TYPE_INTEGER,
               { EGL_NATIVE_RENDERABLE,         ATTRIB_TYPE_BOOLEAN,
               { EGL_NATIVE_VISUAL_ID,          ATTRIB_TYPE_PLATFORM,
               { EGL_NATIVE_VISUAL_TYPE,        ATTRIB_TYPE_PLATFORM,
               { EGL_RENDERABLE_TYPE,           ATTRIB_TYPE_BITMASK,
               { EGL_SAMPLE_BUFFERS,            ATTRIB_TYPE_INTEGER,
               { EGL_SAMPLES,                   ATTRIB_TYPE_INTEGER,
               { EGL_STENCIL_SIZE,              ATTRIB_TYPE_INTEGER,
               { EGL_SURFACE_TYPE,              ATTRIB_TYPE_BITMASK,
               { EGL_TRANSPARENT_TYPE,          ATTRIB_TYPE_ENUM,
               { EGL_TRANSPARENT_RED_VALUE,     ATTRIB_TYPE_INTEGER,
               { EGL_TRANSPARENT_GREEN_VALUE,   ATTRIB_TYPE_INTEGER,
               { EGL_TRANSPARENT_BLUE_VALUE,    ATTRIB_TYPE_INTEGER,
               { EGL_MATCH_NATIVE_PIXMAP,       ATTRIB_TYPE_PSEUDO,
               /* extensions */
   { EGL_Y_INVERTED_NOK,            ATTRIB_TYPE_BOOLEAN,
               { EGL_FRAMEBUFFER_TARGET_ANDROID, ATTRIB_TYPE_BOOLEAN,
               { EGL_RECORDABLE_ANDROID,        ATTRIB_TYPE_BOOLEAN,
               { EGL_COLOR_COMPONENT_TYPE_EXT,  ATTRIB_TYPE_ENUM,
                  };
      /**
   * Return true if a config is valid.  When for_matching is true,
   * EGL_DONT_CARE is accepted as a valid attribute value, and checks
   * for conflicting attribute values are skipped.
   *
   * Note that some attributes are platform-dependent and are not
   * checked.
   */
   EGLBoolean
   _eglValidateConfig(const _EGLConfig *conf, EGLBoolean for_matching)
   {
      _EGLDisplay *disp = conf->Display;
   EGLint i, attr, val;
            /* check attributes by their types */
   for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
               attr = _eglValidationTable[i].attr;
            switch (_eglValidationTable[i].type) {
   case ATTRIB_TYPE_INTEGER:
      switch (attr) {
   case EGL_CONFIG_ID:
      /* config id must be positive */
   if (val <= 0)
            case EGL_SAMPLE_BUFFERS:
      /* there can be at most 1 sample buffer */
   if (val > 1 || val < 0)
            default:
      if (val < 0)
            }
      case ATTRIB_TYPE_BOOLEAN:
      if (val != EGL_TRUE && val != EGL_FALSE)
            case ATTRIB_TYPE_ENUM:
      switch (attr) {
   case EGL_CONFIG_CAVEAT:
      if (val != EGL_NONE && val != EGL_SLOW_CONFIG &&
      val != EGL_NON_CONFORMANT_CONFIG)
         case EGL_TRANSPARENT_TYPE:
      if (val != EGL_NONE && val != EGL_TRANSPARENT_RGB)
            case EGL_COLOR_BUFFER_TYPE:
      if (val != EGL_RGB_BUFFER && val != EGL_LUMINANCE_BUFFER)
            case EGL_COLOR_COMPONENT_TYPE_EXT:
      if (val != EGL_COLOR_COMPONENT_TYPE_FIXED_EXT &&
      val != EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT)
         default:
      unreachable("check _eglValidationTable[]");
      }
      case ATTRIB_TYPE_BITMASK:
      switch (attr) {
   case EGL_SURFACE_TYPE:
      mask = EGL_PBUFFER_BIT | EGL_PIXMAP_BIT | EGL_WINDOW_BIT |
         EGL_VG_COLORSPACE_LINEAR_BIT | EGL_VG_ALPHA_FORMAT_PRE_BIT |
   EGL_MULTISAMPLE_RESOLVE_BOX_BIT |
   if (disp->Extensions.KHR_mutable_render_buffer)
            case EGL_RENDERABLE_TYPE:
   case EGL_CONFORMANT:
      mask = EGL_OPENGL_ES_BIT | EGL_OPENVG_BIT | EGL_OPENGL_ES2_BIT |
            default:
      unreachable("check _eglValidationTable[]");
   mask = 0;
      }
   if (val & ~mask)
            case ATTRIB_TYPE_PLATFORM:
      /* unable to check platform-dependent attributes here */
      case ATTRIB_TYPE_PSEUDO:
      /* pseudo attributes should not be set */
   if (val != 0)
                     if (!valid && for_matching) {
      /* accept EGL_DONT_CARE as a valid value */
   if (val == EGL_DONT_CARE)
         if (_eglValidationTable[i].criterion == ATTRIB_CRITERION_SPECIAL)
      }
   if (!valid) {
      _eglLog(_EGL_DEBUG, "attribute 0x%04x has an invalid value 0x%x", attr,
                        /* any invalid attribute value should have been caught */
   if (!valid || for_matching)
                     switch (conf->ColorBufferType) {
   case EGL_RGB_BUFFER:
      if (conf->LuminanceSize)
         if (conf->RedSize + conf->GreenSize + conf->BlueSize + conf->AlphaSize !=
      conf->BufferSize)
         case EGL_LUMINANCE_BUFFER:
      if (conf->RedSize || conf->GreenSize || conf->BlueSize)
         if (conf->LuminanceSize + conf->AlphaSize != conf->BufferSize)
            }
   if (!valid) {
      _eglLog(_EGL_DEBUG, "conflicting color buffer type and channel sizes");
               if (!conf->SampleBuffers && conf->Samples)
         if (!valid) {
      _eglLog(_EGL_DEBUG, "conflicting samples and sample buffers");
               if (!(conf->SurfaceType & EGL_WINDOW_BIT)) {
      if (conf->NativeVisualID != 0 || conf->NativeVisualType != EGL_NONE)
      }
   if (!(conf->SurfaceType & EGL_PBUFFER_BIT)) {
      if (conf->BindToTextureRGB || conf->BindToTextureRGBA)
      }
   if (!valid) {
      _eglLog(_EGL_DEBUG,
                        }
      /**
   * Return true if a config matches the criteria.  This and
   * _eglParseConfigAttribList together implement the algorithm
   * described in "Selection of EGLConfigs".
   *
   * Note that attributes that are special (currently, only
   * EGL_MATCH_NATIVE_PIXMAP) are ignored.
   */
   EGLBoolean
   _eglMatchConfig(const _EGLConfig *conf, const _EGLConfig *criteria)
   {
      EGLint attr, val, i;
            for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
      EGLint cmp;
   if (_eglValidationTable[i].criterion == ATTRIB_CRITERION_IGNORE)
            attr = _eglValidationTable[i].attr;
   cmp = _eglGetConfigKey(criteria, attr);
   if (cmp == EGL_DONT_CARE)
            val = _eglGetConfigKey(conf, attr);
   switch (_eglValidationTable[i].criterion) {
   case ATTRIB_CRITERION_EXACT:
      if (val != cmp)
            case ATTRIB_CRITERION_ATLEAST:
      if (val < cmp)
            case ATTRIB_CRITERION_MASK:
      if ((val & cmp) != cmp)
            case ATTRIB_CRITERION_SPECIAL:
      /* ignored here */
      case ATTRIB_CRITERION_IGNORE:
      unreachable("already handled above");
               #ifndef DEBUG
            /* only print the common errors when DEBUG is not defined */
      #endif
            _eglLog(_EGL_DEBUG,
         "the value (0x%x) of attribute 0x%04x did not meet the "
   "criteria (0x%x)",
                     }
      static inline EGLBoolean
   _eglIsConfigAttribValid(const _EGLConfig *conf, EGLint attr)
   {
      if (_eglOffsetOfConfig(attr) < 0)
            switch (attr) {
   case EGL_Y_INVERTED_NOK:
         case EGL_FRAMEBUFFER_TARGET_ANDROID:
         case EGL_RECORDABLE_ANDROID:
         default:
                     }
      /**
   * Initialize a criteria config from the given attribute list.
   * Return EGL_FALSE if any of the attribute is invalid.
   */
   EGLBoolean
   _eglParseConfigAttribList(_EGLConfig *conf, _EGLDisplay *disp,
         {
                        /* reset to default values */
   for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
      attr = _eglValidationTable[i].attr;
   val = _eglValidationTable[i].default_value;
               /* parse the list */
   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i += 2) {
      attr = attrib_list[i];
            if (!_eglIsConfigAttribValid(conf, attr))
                        if (!_eglValidateConfig(conf, EGL_TRUE))
            /* EGL_LEVEL and EGL_MATCH_NATIVE_PIXMAP cannot be EGL_DONT_CARE */
   if (conf->Level == EGL_DONT_CARE || conf->MatchNativePixmap == EGL_DONT_CARE)
            /* ignore other attributes when EGL_CONFIG_ID is given */
   if (conf->ConfigID != EGL_DONT_CARE) {
      for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
      attr = _eglValidationTable[i].attr;
   if (attr != EGL_CONFIG_ID)
         } else {
      if (!(conf->SurfaceType & EGL_WINDOW_BIT))
            if (conf->TransparentType == EGL_NONE) {
      conf->TransparentRedValue = EGL_DONT_CARE;
   conf->TransparentGreenValue = EGL_DONT_CARE;
                     }
      /**
   * Decide the ordering of conf1 and conf2, under the given criteria.
   * When compare_id is true, this implements the algorithm described
   * in "Sorting of EGLConfigs".  When compare_id is false,
   * EGL_CONFIG_ID is not compared.
   *
   * It returns a negative integer if conf1 is considered to come
   * before conf2;  a positive integer if conf2 is considered to come
   * before conf1;  zero if the ordering cannot be decided.
   *
   * Note that EGL_NATIVE_VISUAL_TYPE is platform-dependent and is
   * ignored here.
   */
   EGLint
   _eglCompareConfigs(const _EGLConfig *conf1, const _EGLConfig *conf2,
         {
      const EGLint compare_attribs[] = {
      EGL_BUFFER_SIZE, EGL_SAMPLE_BUFFERS, EGL_SAMPLES,
      };
   EGLint val1, val2;
            if (conf1 == conf2)
            /* the enum values have the desired ordering */
   STATIC_ASSERT(EGL_NONE < EGL_SLOW_CONFIG);
   STATIC_ASSERT(EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG);
   val1 = conf1->ConfigCaveat - conf2->ConfigCaveat;
   if (val1)
            /* the enum values have the desired ordering */
   STATIC_ASSERT(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER);
   val1 = conf1->ColorBufferType - conf2->ColorBufferType;
   if (val1)
            if (criteria) {
      val1 = val2 = 0;
   if (conf1->ColorBufferType == EGL_RGB_BUFFER) {
      if (criteria->RedSize > 0) {
      val1 += conf1->RedSize;
      }
   if (criteria->GreenSize > 0) {
      val1 += conf1->GreenSize;
      }
   if (criteria->BlueSize > 0) {
      val1 += conf1->BlueSize;
         } else {
      if (criteria->LuminanceSize > 0) {
      val1 += conf1->LuminanceSize;
         }
   if (criteria->AlphaSize > 0) {
      val1 += conf1->AlphaSize;
         } else {
      /* assume the default criteria, which gives no specific ordering */
               /* for color bits, larger one is preferred */
   if (val1 != val2)
            for (i = 0; i < ARRAY_SIZE(compare_attribs); i++) {
      val1 = _eglGetConfigKey(conf1, compare_attribs[i]);
   val2 = _eglGetConfigKey(conf2, compare_attribs[i]);
   if (val1 != val2)
                           }
      static inline void
   _eglSwapConfigs(const _EGLConfig **conf1, const _EGLConfig **conf2)
   {
      const _EGLConfig *tmp = *conf1;
   *conf1 = *conf2;
      }
      /**
   * Quick sort an array of configs.  This differs from the standard
   * qsort() in that the compare function accepts an additional
   * argument.
   */
   static void
   _eglSortConfigs(const _EGLConfig **configs, EGLint count,
                     {
      const EGLint pivot = 0;
            if (count <= 1)
            _eglSwapConfigs(&configs[pivot], &configs[count / 2]);
   i = 1;
   j = count - 1;
   do {
      while (i < count && compare(configs[i], configs[pivot], priv_data) < 0)
         while (compare(configs[j], configs[pivot], priv_data) > 0)
         if (i < j) {
      _eglSwapConfigs(&configs[i], &configs[j]);
   i++;
      } else if (i == j) {
      i++;
   j--;
         } while (i <= j);
            _eglSortConfigs(configs, j, compare, priv_data);
      }
      /**
   * A helper function for implementing eglChooseConfig.  See _eglFilterArray and
   * _eglSortConfigs for the meanings of match and compare.
   */
   static EGLBoolean
   _eglFilterConfigArray(_EGLArray *array, EGLConfig *configs, EGLint config_size,
                        EGLint *num_configs,
   EGLBoolean (*match)(const _EGLConfig *,
      {
      _EGLConfig **configList;
            /* get the number of matched configs */
   count = _eglFilterArray(array, NULL, 0, (_EGLArrayForEach)match, priv_data);
   if (!count) {
      *num_configs = count;
               configList = malloc(sizeof(*configList) * count);
   if (!configList)
            /* get the matched configs */
   _eglFilterArray(array, (void **)configList, count, (_EGLArrayForEach)match,
            /* perform sorting of configs */
   if (configs && count) {
      _eglSortConfigs((const _EGLConfig **)configList, count, compare,
         count = MIN2(count, config_size);
   for (i = 0; i < count; i++)
                                    }
      static EGLint
   _eglFallbackCompare(const _EGLConfig *conf1, const _EGLConfig *conf2,
         {
      return _eglCompareConfigs(conf1, conf2, (const _EGLConfig *)priv_data,
      }
      /**
   * Typical fallback routine for eglChooseConfig
   */
   EGLBoolean
   _eglChooseConfig(_EGLDisplay *disp, const EGLint *attrib_list,
         {
      _EGLConfig criteria;
            if (!_eglParseConfigAttribList(&criteria, disp, attrib_list))
            result = _eglFilterConfigArray(disp->Configs, configs, config_size,
                  if (result && (_eglGetLogLevel() == _EGL_DEBUG))
               }
      /**
   * Fallback for eglGetConfigAttrib.
   */
   EGLBoolean
   _eglGetConfigAttrib(const _EGLDisplay *disp, const _EGLConfig *conf,
         {
      if (!_eglIsConfigAttribValid(conf, attribute))
            /* nonqueryable attributes */
   switch (attribute) {
   case EGL_MATCH_NATIVE_PIXMAP:
      return _eglError(EGL_BAD_ATTRIBUTE, "eglGetConfigAttrib");
      default:
                  if (!value)
            *value = _eglGetConfigKey(conf, attribute);
      }
      static EGLBoolean
   _eglFlattenConfig(void *elem, void *buffer)
   {
      _EGLConfig *conf = (_EGLConfig *)elem;
   EGLConfig *handle = (EGLConfig *)buffer;
   *handle = _eglGetConfigHandle(conf);
      }
      /**
   * Fallback for eglGetConfigs.
   */
   EGLBoolean
   _eglGetConfigs(_EGLDisplay *disp, EGLConfig *configs, EGLint config_size,
         {
      *num_config =
      _eglFlattenArray(disp->Configs, (void *)configs, sizeof(configs[0]),
         if (_eglGetLogLevel() == _EGL_DEBUG)
               }
