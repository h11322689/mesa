   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
   * \file
   * \brief Extension handling
   */
      #include "util/os_misc.h"
      #include "util/glheader.h"
      #include "context.h"
   #include "extensions.h"
   #include "macros.h"
   #include "mtypes.h"
      struct gl_extensions _mesa_extension_override_enables;
   struct gl_extensions _mesa_extension_override_disables;
      #define MAX_UNRECOGNIZED_EXTENSIONS 16
   static struct {
      char *env;
      } unrecognized_extensions;
      /**
   * Given a member \c x of struct gl_extensions, return offset of
   * \c x in bytes.
   */
   #define o(x) offsetof(struct gl_extensions, x)
      static int
   extension_name_compare(const void *name, const void *elem)
   {
      const struct mesa_extension *entry = elem;
      }
      /**
   * Given an extension name, lookup up the corresponding member of struct
   * gl_extensions and return that member's index.  If the name is
   * not found in the \c _mesa_extension_table, return -1.
   *
   * \param name Name of extension.
   * \return Index of member in struct gl_extensions.
   */
   static int
   name_to_index(const char* name)
   {
               if (!name)
            entry = bsearch(name,
                        if (entry)
               }
      /**
   * Overrides extensions in \c ctx based on the values in
   * _mesa_extension_override_enables and _mesa_extension_override_disables.
   */
   void
   _mesa_override_extensions(struct gl_context *ctx)
   {
      unsigned i;
   const GLboolean *enables =
         const GLboolean *disables =
                  for (i = 0; i < MESA_EXTENSION_COUNT; ++i) {
               assert(!enables[offset] || !disables[offset]);
   if (enables[offset]) {
         } else if (disables[offset]) {
               }
      /**
   * Either enable or disable the named extension.
   * \return offset of extensions withint `ext' or 0 if extension is not known
   */
   static size_t
   set_extension(struct gl_extensions *ext, int i, GLboolean state)
   {
               offset = i < 0 ? 0 : _mesa_extension_table[i].offset;
   if (offset != 0 && (offset != o(dummy_true) || state != GL_FALSE)) {
                     }
         /**
   * \brief Free string pointed by unrecognized_extensions
   *
   * This string is allocated early during the first context creation by
   * _mesa_one_time_init_extension_overrides.
   */
   static void
   free_unknown_extensions_strings(void)
   {
      free(unrecognized_extensions.env);
   for (int i = 0; i < MAX_UNRECOGNIZED_EXTENSIONS; ++i)
      }
         /**
   * \brief Initialize extension override tables based on \c override
   *
   * This should be called one time early during first context initialization.
      * \c override is a space-separated list of extensions to
   * enable or disable. The list is processed thus:
   *    - Enable recognized extension names that are prefixed with '+'.
   *    - Disable recognized extension names that are prefixed with '-'.
   *    - Enable recognized extension names that are not prefixed.
   *    - Collect unrecognized extension names in a new string.
   */
   void
   _mesa_one_time_init_extension_overrides(const char *override)
   {
      char *env;
   char *ext;
   size_t offset;
            memset(&_mesa_extension_override_enables, 0, sizeof(struct gl_extensions));
            if (override == NULL || override[0] == '\0') {
                  /* Copy 'override' because strtok() is destructive. */
            if (env == NULL)
            for (ext = strtok(env, " "); ext != NULL; ext = strtok(NULL, " ")) {
      int enable;
   int i;
   bool recognized;
   switch (ext[0]) {
   case '+':
      enable = 1;
   ++ext;
      case '-':
      enable = 0;
   ++ext;
      default:
      enable = 1;
               i = name_to_index(ext);
   offset = set_extension(&_mesa_extension_override_enables, i, enable);
   offset = set_extension(&_mesa_extension_override_disables, i, !enable);
   if (offset != 0)
         else
            if (!enable && recognized && offset <= 1) {
      printf("Warning: extension '%s' cannot be disabled\n", ext);
               if (!recognized && enable) {
                        if (!warned) {
      warned = true;
   _mesa_problem(NULL, "Trying to enable too many unknown extension. "
               } else {
      unrecognized_extensions.names[unknown_ext] = ext;
   unknown_ext++;
                     if (!unknown_ext) {
         } else {
      unrecognized_extensions.env = env;
         }
         /**
   * \brief Initialize extension tables and enable default extensions.
   *
   * This should be called during context initialization.
   * Note: Sets gl_extensions.dummy_true to true.
   */
   void
   _mesa_init_extensions(struct gl_extensions *extensions)
   {
      GLboolean *base = (GLboolean *) extensions;
   GLboolean *sentinel = base + o(extension_sentinel);
            /* First, turn all extensions off. */
   for (i = base; i != sentinel; ++i)
            /* Then, selectively turn default extensions on. */
            /* Always enable these extensions for all drivers.
   * We can't use dummy_true in extensions_table.h for these
   * because this would make them non-disablable using
   * _mesa_override_extensions.
   */
   extensions->MESA_pack_invert = GL_TRUE;
            extensions->ARB_ES2_compatibility = GL_TRUE;
   extensions->ARB_draw_elements_base_vertex = GL_TRUE;
   extensions->ARB_explicit_attrib_location = GL_TRUE;
   extensions->ARB_explicit_uniform_location = GL_TRUE;
   extensions->ARB_fragment_coord_conventions = GL_TRUE;
   extensions->ARB_fragment_program = GL_TRUE;
   extensions->ARB_fragment_shader = GL_TRUE;
   extensions->ARB_half_float_vertex = GL_TRUE;
   extensions->ARB_internalformat_query = GL_TRUE;
   extensions->ARB_internalformat_query2 = GL_TRUE;
   extensions->ARB_map_buffer_range = GL_TRUE;
   extensions->ARB_occlusion_query = GL_TRUE;
   extensions->ARB_sync = GL_TRUE;
   extensions->ARB_vertex_program = GL_TRUE;
            extensions->EXT_EGL_image_storage = GL_TRUE;
   extensions->EXT_gpu_program_parameters = GL_TRUE;
   extensions->EXT_provoking_vertex = GL_TRUE;
   extensions->EXT_stencil_two_side = GL_TRUE;
            extensions->ATI_fragment_shader = GL_TRUE;
                     extensions->NV_copy_image = GL_TRUE;
   extensions->NV_fog_distance = GL_TRUE;
   extensions->NV_texture_env_combine4 = GL_TRUE;
            extensions->OES_EGL_image = GL_TRUE;
   extensions->OES_EGL_image_external = GL_TRUE;
      }
         typedef unsigned short extension_index;
         /**
   * Given an extension enum, return whether or not the extension is supported
   * dependent on the following factors:
   * There's driver support and the OpenGL/ES version is at least that
   * specified in the _mesa_extension_table.
   */
   static inline bool
   _mesa_extension_supported(const struct gl_context *ctx, extension_index i)
   {
      const bool *base = (bool *) &ctx->Extensions;
               }
      /**
   * Compare two entries of the extensions table.  Sorts first by year,
   * then by name.
   *
   * Arguments are indices into _mesa_extension_table.
   */
   static int
   extension_compare(const void *p1, const void *p2)
   {
      extension_index i1 = * (const extension_index *) p1;
   extension_index i2 = * (const extension_index *) p2;
   const struct mesa_extension *e1 = &_mesa_extension_table[i1];
   const struct mesa_extension *e2 = &_mesa_extension_table[i2];
                     if (res == 0) {
                     }
         /**
   * Construct the GL_EXTENSIONS string.  Called the first time that
   * glGetString(GL_EXTENSIONS) is called.
   */
   GLubyte*
   _mesa_make_extension_string(struct gl_context *ctx)
   {
      /* The extension string. */
   char *exts = NULL;
   /* Length of extension string. */
   size_t length = 0;
   /* Number of extensions */
   unsigned count;
   /* Indices of the extensions sorted by year */
   extension_index extension_indices[MESA_EXTENSION_COUNT];
   unsigned k;
   unsigned j;
            /* Check if the MESA_EXTENSION_MAX_YEAR env var is set */
   {
      const char *env = getenv("MESA_EXTENSION_MAX_YEAR");
   if (env) {
      maxYear = atoi(env);
   _mesa_debug(ctx, "Note: limiting GL extensions to %u or earlier\n",
                  /* Compute length of the extension string. */
   count = 0;
   for (k = 0; k < MESA_EXTENSION_COUNT; ++k) {
               if (i->year <= maxYear &&
   length += strlen(i->name) + 1; /* +1 for space */
   ++count;
            }
   for (k = 0; k < MAX_UNRECOGNIZED_EXTENSIONS; k++)
      if (unrecognized_extensions.names[k])
         exts = calloc(ALIGN(length + 1, 4), sizeof(char));
   if (exts == NULL) {
                  /* Sort extensions in chronological order because idTech 2/3 games
   * (e.g., Quake3 demo) store the extension list in a fixed size buffer.
   * Some cases truncate, while others overflow the buffer. Resulting in
   * misrendering and crashes, respectively.
   * Address the former here, while the latter will be addressed by setting
   * the MESA_EXTENSION_MAX_YEAR environment variable.
   */
   j = 0;
   for (k = 0; k < MESA_EXTENSION_COUNT; ++k) {
      if (_mesa_extension_table[k].year <= maxYear &&
      _mesa_extension_supported(ctx, k)) {
         }
   assert(j == count);
   qsort(extension_indices, count,
            /* Build the extension string.*/
   for (j = 0; j < count; ++j) {
      const struct mesa_extension *i = &_mesa_extension_table[extension_indices[j]];
   assert(_mesa_extension_supported(ctx, extension_indices[j]));
   strcat(exts, i->name);
      }
   for (j = 0; j < MAX_UNRECOGNIZED_EXTENSIONS; j++) {
      if (unrecognized_extensions.names[j]) {
      strcat(exts, unrecognized_extensions.names[j]);
                     }
      /**
   * Return number of enabled extensions.
   */
   GLuint
   _mesa_get_extension_count(struct gl_context *ctx)
   {
               /* only count once */
   if (ctx->Extensions.Count != 0)
            for (k = 0; k < MESA_EXTENSION_COUNT; ++k) {
      ctx->Extensions.Count++;
               for (k = 0; k < MAX_UNRECOGNIZED_EXTENSIONS; ++k) {
      if (unrecognized_extensions.names[k])
      }
      }
      /**
   * Return name of i-th enabled extension
   */
   const GLubyte *
   _mesa_get_enabled_extension(struct gl_context *ctx, GLuint index)
   {
      size_t n = 0;
            for (i = 0; i < MESA_EXTENSION_COUNT; ++i) {
      if (_mesa_extension_supported(ctx, i)) {
      if (n == index)
         else
                  for (i = 0; i < MAX_UNRECOGNIZED_EXTENSIONS; ++i) {
      if (unrecognized_extensions.names[i]) {
      if (n == index)
         else
         }
      }
