   /*
   * (C) Copyright IBM Corporation 2002, 2004
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file glxextensions.c
   *
   * \author Ian Romanick <idr@us.ibm.com>
   */
      #include "glxclient.h"
   #include <X11/extensions/extutil.h>
   #include <X11/extensions/Xext.h>
   #include <string.h>
   #include "glxextensions.h"
      #include "util/driconf.h"
   #include "util/macros.h"
      #define SET_BIT(m,b)   (m[ (b) / 8 ] |=  (1U << ((b) % 8)))
   #define CLR_BIT(m,b)   (m[ (b) / 8 ] &= ~(1U << ((b) % 8)))
   #define IS_SET(m,b)    ((m[ (b) / 8 ] & (1U << ((b) % 8))) != 0)
   #define GLX(n) "GLX_" # n, 4 + sizeof( # n ) - 1, PASTE2(n,_bit)
   #define GL(n)  "GL_" # n,  3 + sizeof( # n ) - 1, GL_ ## n ## _bit
   #define Y  1
   #define N  0
   #define EXT_ENABLED(bit,supported) (IS_SET( supported, bit ))
         struct extension_info
   {
      const char *const name;
                     /**
   * The direct-renderer (e.g., i965_dri.so) supports this extension.
   *
   * For cases where all of the infrastructure to support the extension is a
   * required part of the loader/driver interface, this can default to Y.
   * For most cases, extended functionality, usually in the form of DRI2
   * extensions, is necessary to support the extension.  The loader will set
   * the flag true if all the requirements are met.
   *
   * If the display is capable of direct rendering, ::direct_support is
   * required for the extension to be enabled.
   */
            /**
   * The extension only functions with direct-rendering contexts
   *
   * The extension has no GLX protocol, and, therefore, no explicit
   * dependency on the server.  The functionality is contained entirely in
   * the client library and the direct renderer.  A few of the swap-related
   * extensions are intended to behave this way.
   */
      };
      /* *INDENT-OFF* */
   static const struct extension_info known_glx_extensions[] = {
      { GLX(ARB_context_flush_control),      N, N },
   { GLX(ARB_create_context),             N, N },
   { GLX(ARB_create_context_no_error),    N, N },
   { GLX(ARB_create_context_profile),     N, N },
   { GLX(ARB_create_context_robustness),  N, N },
   { GLX(ARB_fbconfig_float),             Y, N },
   { GLX(ARB_framebuffer_sRGB),           Y, N },
   { GLX(ARB_get_proc_address),           N, N },
   { GLX(ARB_multisample),                Y, N },
   { GLX(EXT_buffer_age),                 N, Y },
   { GLX(EXT_create_context_es2_profile), N, N },
   { GLX(EXT_create_context_es_profile),  N, N },
   { GLX(EXT_fbconfig_packed_float),      Y, N },
   { GLX(EXT_framebuffer_sRGB),           Y, N },
   { GLX(EXT_import_context),             Y, N },
   { GLX(EXT_no_config_context),          N, N },
   { GLX(EXT_swap_control),               N, Y },
   { GLX(EXT_swap_control_tear),          N, Y },
   { GLX(EXT_texture_from_pixmap),        N, N },
   { GLX(EXT_visual_info),                Y, N },
   { GLX(EXT_visual_rating),              Y, N },
   { GLX(ATI_pixel_format_float),         N, N },
   { GLX(INTEL_swap_event),               N, N },
   { GLX(MESA_copy_sub_buffer),           N, N },
   { GLX(MESA_gl_interop),                N, Y },
   { GLX(MESA_query_renderer),            N, Y },
   { GLX(MESA_swap_control),              N, Y },
   { GLX(NV_float_buffer),                N, N },
   { GLX(OML_sync_control),               N, Y },
   { GLX(SGIS_multisample),               Y, N },
   { GLX(SGIX_fbconfig),                  Y, N },
   { GLX(SGIX_pbuffer),                   Y, N },
   { GLX(SGIX_visual_select_group),       Y, N },
   { GLX(SGI_make_current_read),          N, N },
   { GLX(SGI_swap_control),               N, N },
   { GLX(SGI_video_sync),                 N, Y },
      };
      static const struct extension_info known_gl_extensions[] = {
      { GL(ARB_depth_texture),               N, N },
   { GL(ARB_draw_buffers),                N, N },
   { GL(ARB_fragment_program),            N, N },
   { GL(ARB_fragment_program_shadow),     N, N },
   { GL(ARB_framebuffer_object),          N, N },
   { GL(ARB_imaging),                     N, N },
   { GL(ARB_multisample),                 N, N },
   { GL(ARB_multitexture),                N, N },
   { GL(ARB_occlusion_query),             N, N },
   { GL(ARB_point_parameters),            N, N },
   { GL(ARB_point_sprite),                N, N },
   { GL(ARB_shadow),                      N, N },
   { GL(ARB_shadow_ambient),              N, N },
   { GL(ARB_texture_border_clamp),        N, N },
   { GL(ARB_texture_compression),         N, N },
   { GL(ARB_texture_cube_map),            N, N },
   { GL(ARB_texture_env_add),             N, N },
   { GL(ARB_texture_env_combine),         N, N },
   { GL(ARB_texture_env_crossbar),        N, N },
   { GL(ARB_texture_env_dot3),            N, N },
   { GL(ARB_texture_filter_anisotropic),  N, N },
   { GL(ARB_texture_mirrored_repeat),     N, N },
   { GL(ARB_texture_non_power_of_two),    N, N },
   { GL(ARB_texture_rectangle),           N, N },
   { GL(ARB_texture_rg),                  N, N },
   { GL(ARB_transpose_matrix),            N, N },
   { GL(ARB_vertex_program),              N, N },
   { GL(ARB_window_pos),                  N, N },
   { GL(EXT_abgr),                        N, N },
   { GL(EXT_bgra),                        N, N },
   { GL(EXT_blend_color),                 N, N },
   { GL(EXT_blend_equation_separate),     N, N },
   { GL(EXT_blend_func_separate),         N, N },
   { GL(EXT_blend_logic_op),              N, N },
   { GL(EXT_blend_minmax),                N, N },
   { GL(EXT_blend_subtract),              N, N },
   { GL(EXT_clip_volume_hint),            N, N },
   { GL(EXT_copy_texture),                N, N },
   { GL(EXT_draw_range_elements),         N, N },
   { GL(EXT_fog_coord),                   N, N },
   { GL(EXT_framebuffer_blit),            N, N },
   { GL(EXT_framebuffer_multisample),     N, N },
   { GL(EXT_framebuffer_object),          N, N },
   { GL(EXT_framebuffer_sRGB),            N, N },
   { GL(EXT_multi_draw_arrays),           N, N },
   { GL(EXT_packed_depth_stencil),        N, N },
   { GL(EXT_packed_pixels),               N, N },
   { GL(EXT_paletted_texture),            N, N },
   { GL(EXT_point_parameters),            N, N },
   { GL(EXT_polygon_offset),              N, N },
   { GL(EXT_rescale_normal),              N, N },
   { GL(EXT_secondary_color),             N, N },
   { GL(EXT_separate_specular_color),     N, N },
   { GL(EXT_shadow_funcs),                N, N },
   { GL(EXT_shared_texture_palette),      N, N },
   { GL(EXT_stencil_two_side),            N, N },
   { GL(EXT_stencil_wrap),                N, N },
   { GL(EXT_subtexture),                  N, N },
   { GL(EXT_texture),                     N, N },
   { GL(EXT_texture3D),                   N, N },
   { GL(EXT_texture_compression_dxt1),    N, N },
   { GL(EXT_texture_compression_s3tc),    N, N },
   { GL(EXT_texture_edge_clamp),          N, N },
   { GL(EXT_texture_env_add),             N, N },
   { GL(EXT_texture_env_combine),         N, N },
   { GL(EXT_texture_env_dot3),            N, N },
   { GL(EXT_texture_filter_anisotropic),  N, N },
   { GL(EXT_texture_integer),             N, N },
   { GL(EXT_texture_lod),                 N, N },
   { GL(EXT_texture_lod_bias),            N, N },
   { GL(EXT_texture_mirror_clamp),        N, N },
   { GL(EXT_texture_rectangle),           N, N },
   { GL(EXT_vertex_array),                N, N },
   { GL(3DFX_texture_compression_FXT1),   N, N },
   { GL(APPLE_packed_pixels),             N, N },
   { GL(APPLE_ycbcr_422),                 N, N },
   { GL(ATI_draw_buffers),                N, N },
   { GL(ATI_text_fragment_shader),        N, N },
   { GL(ATI_texture_env_combine3),        N, N },
   { GL(ATI_texture_float),               N, N },
   { GL(ATI_texture_mirror_once),         N, N },
   { GL(ATIX_texture_env_combine3),       N, N },
   { GL(HP_convolution_border_modes),     N, N },
   { GL(HP_occlusion_test),               N, N },
   { GL(IBM_cull_vertex),                 N, N },
   { GL(IBM_pixel_filter_hint),           N, N },
   { GL(IBM_rasterpos_clip),              N, N },
   { GL(IBM_texture_clamp_nodraw),        N, N },
   { GL(IBM_texture_mirrored_repeat),     N, N },
   { GL(INGR_blend_func_separate),        N, N },
   { GL(INGR_interlace_read),             N, N },
   { GL(MESA_pack_invert),                N, N },
   { GL(MESA_ycbcr_texture),              N, N },
   { GL(NV_blend_square),                 N, N },
   { GL(NV_copy_depth_to_color),          N, N },
   { GL(NV_depth_clamp),                  N, N },
   { GL(NV_fog_distance),                 N, N },
   { GL(NV_fragment_program),             N, N },
   { GL(NV_fragment_program_option),      N, N },
   { GL(NV_fragment_program2),            N, N },
   { GL(NV_light_max_exponent),           N, N },
   { GL(NV_multisample_filter_hint),      N, N },
   { GL(NV_packed_depth_stencil),         N, N },
   { GL(NV_point_sprite),                 N, N },
   { GL(NV_texgen_reflection),            N, N },
   { GL(NV_texture_compression_vtc),      N, N },
   { GL(NV_texture_env_combine4),         N, N },
   { GL(NV_texture_rectangle),            N, N },
   { GL(NV_vertex_program),               N, N },
   { GL(NV_vertex_program1_1),            N, N },
   { GL(NV_vertex_program2),              N, N },
   { GL(NV_vertex_program2_option),       N, N },
   { GL(NV_vertex_program3),              N, N },
   { GL(OES_read_format),                 N, N },
   { GL(OES_compressed_paletted_texture), N, N },
   { GL(SGI_color_matrix),                N, N },
   { GL(SGI_color_table),                 N, N },
   { GL(SGI_texture_color_table),         N, N },
   { GL(SGIS_generate_mipmap),            N, N },
   { GL(SGIS_multisample),                N, N },
   { GL(SGIS_texture_border_clamp),       N, N },
   { GL(SGIS_texture_edge_clamp),         N, N },
   { GL(SGIS_texture_lod),                N, N },
   { GL(SGIX_blend_alpha_minmax),         N, N },
   { GL(SGIX_clipmap),                    N, N },
   { GL(SGIX_depth_texture),              N, N },
   { GL(SGIX_fog_offset),                 N, N },
   { GL(SGIX_shadow),                     N, N },
   { GL(SGIX_shadow_ambient),             N, N },
   { GL(SGIX_texture_coordinate_clamp),   N, N },
   { GL(SGIX_texture_lod_bias),           N, N },
   { GL(SGIX_texture_range),              N, N },
   { GL(SGIX_texture_scale_bias),         N, N },
   { GL(SGIX_vertex_preclip),             N, N },
   { GL(SGIX_vertex_preclip_hint),        N, N },
   { GL(SGIX_ycrcb),                      N, N },
   { GL(SUN_convolution_border_modes),    N, N },
   { GL(SUN_multi_draw_arrays),           N, N },
   { GL(SUN_slice_accum),                 N, N },
      };
   /* *INDENT-ON* */
         /* global bit-fields of available extensions and their characteristics */
   static unsigned char client_glx_only[__GLX_EXT_BYTES];
   static unsigned char direct_glx_only[__GLX_EXT_BYTES];
      /**
   * Bits representing the set of extensions that are enabled by default in all
   * direct rendering drivers.
   */
   static unsigned char direct_glx_support[__GLX_EXT_BYTES];
      /* client extensions string */
   static const char *__glXGLXClientExtensions = NULL;
      static void __glXExtensionsCtr(void);
   static void __glXExtensionsCtrScreen(struct glx_screen * psc);
   static void __glXProcessServerString(const struct extension_info *ext,
                  /**
   * Find an extension in the list based on its name.
   *
   * \param ext       List of extensions where to search.
   * \param name      Name of the extension.
   * \param name_len  Length, in characters, of the extension name.
   */
   static const struct extension_info *
   find_extension(const struct extension_info *ext, const char *name,
         {
               for (i = 0; ext[i].name != NULL; i++) {
      if ((name_len == ext[i].name_len)
      && (strncmp(ext[i].name, name, name_len) == 0)) {
                     }
      /**
   * Set the state of a GLX extension.
   *
   * \param name      Name of the extension.
   * \param name_len  Length, in characters, of the extension name.
   * \param state     New state (either enabled or disabled) of the extension.
   * \param supported Table in which the state of the extension is to be set.
   */
   static void
   set_glx_extension(const struct extension_info *ext_list,
               {
      const struct extension_info *ext = find_extension(ext_list, name, name_len);
   if (!ext)
            if (state) {
         } else {
            }
         #define NUL '\0'
   #define SEPARATOR ' '
      /**
   * Convert the server's extension string to a bit-field.
   *
   * \param server_string   GLX extension string from the server.
   * \param server_support  Bit-field of supported extensions.
   *
   * \note
   * This function is used to process both GLX and GL extension strings.  The
   * bit-fields used to track each of these have different sizes.  Therefore,
   * the data pointed by \c server_support must be preinitialized to zero.
   */
   static void
   __glXProcessServerString(const struct extension_info *ext,
               {
      unsigned base;
            for (base = 0; server_string[base] != NUL; /* empty */ ) {
      /* Determine the length of the next extension name.
   */
   for (len = 0; (server_string[base + len] != SEPARATOR)
      && (server_string[base + len] != NUL); len++) {
               /* Set the bit for the extension in the server_support table.
   */
   set_glx_extension(ext, &server_string[base], len, GL_TRUE,
               /* Advance to the next extension string.  This means that we skip
   * over the previous string and any trialing white-space.
   */
   for (base += len; (server_string[base] == SEPARATOR)
      && (server_string[base] != NUL); base++) {
            }
      void
   __glXEnableDirectExtension(struct glx_screen * psc, const char *name)
   {
      __glXExtensionsCtr();
            set_glx_extension(known_glx_extensions,
      }
      static void
   __ParseExtensionOverride(struct glx_screen *psc,
                           {
      const struct extension_info *ext;
            if (override == NULL)
            /* Copy env_const because strtok() is destructive. */
   env = strdup(override);
   if (env == NULL)
            for (field = strtok(env, " "); field!= NULL; field = strtok(NULL, " ")) {
               switch (field[0]) {
   case '+':
      enable = GL_TRUE;
   ++field;
      case '-':
      enable = GL_FALSE;
   ++field;
      default:
      enable = GL_TRUE;
               ext = find_extension(ext_list, field, strlen(field));
   if (ext) {
      if (enable)
         else
      } else {
      fprintf(stderr, "WARNING: Trying to %s the unknown extension '%s'\n",
                     }
      /**
   * \brief Parse the list of GLX extensions that the user wants to
   * force-enable/disable by using \c override, and write the results to the
   * screen's context.
   *
   * \param psc        Pointer to GLX per-screen record.
   * \param override   A space-separated list of extensions to enable or disable.
   * The list is processed thus:
   *    - Enable recognized extension names that are prefixed with '+'.
   *    - Disable recognized extension names that are prefixed with '-'.
   *    - Enable recognized extension names that are not prefixed.
   */
   void
   __glXParseExtensionOverride(struct glx_screen *psc, const char *override)
   {
      __ParseExtensionOverride(psc, known_glx_extensions, psc->glx_force_enabled,
      }
      /**
   * \brief Parse the list of GL extensions that the user wants to
   * force-enable/disable by using \c override, and write the results to the
   * screen's context.
   *
   * \param psc        Pointer to GLX per-screen record.
   * \param override   A space-separated list of extensions to enable or disable.
   * The list is processed thus:
   *    - Enable recognized extension names that are prefixed with '+'.
   *    - Disable recognized extension names that are prefixed with '-'.
   *    - Enable recognized extension names that are not prefixed.
   */
   void
   __IndirectGlParseExtensionOverride(struct glx_screen *psc, const char *override)
   {
      __ParseExtensionOverride(psc, known_gl_extensions, psc->gl_force_enabled,
      }
         /**
   * Initialize global extension support tables.
   */
      static void
   __glXExtensionsCtr(void)
   {
      unsigned i;
               if (ext_list_first_time) {
               (void) memset(direct_glx_support, 0, sizeof(direct_glx_support));
   (void) memset(client_glx_only, 0, sizeof(client_glx_only));
            SET_BIT(client_glx_only, ARB_get_proc_address_bit);
   for (i = 0; known_glx_extensions[i].name != NULL; i++) {
               if (known_glx_extensions[i].direct_support) {
                  if (known_glx_extensions[i].direct_only) {
                  }
         /**
   * Make sure that per-screen direct-support table is initialized.
   *
   * \param psc  Pointer to GLX per-screen record.
   */
      static void
   __glXExtensionsCtrScreen(struct glx_screen * psc)
   {
      if (psc->ext_list_first_time) {
      psc->ext_list_first_time = GL_FALSE;
   (void) memcpy(psc->direct_support, direct_glx_support,
         (void) memset(psc->glx_force_enabled, 0,
         (void) memset(psc->glx_force_disabled, 0,
         (void) memset(psc->gl_force_enabled, 0,
         (void) memset(psc->gl_force_disabled, 0,
         }
         /**
   * Check if a certain extension is enabled on a given screen.
   *
   * \param psc  Pointer to GLX per-screen record.
   * \param bit  Bit index in the direct-support table.
   * \returns If the extension bit is enabled for the screen, \c GL_TRUE is
   *          returned.  If the extension bit is not enabled or if \c psc is
   *          \c NULL, then \c GL_FALSE is returned.
   */
   GLboolean
   __glXExtensionBitIsEnabled(struct glx_screen * psc, unsigned bit)
   {
               if (psc != NULL) {
      __glXExtensionsCtr();
   __glXExtensionsCtrScreen(psc);
                  }
         /**
   * Check if a certain extension is enabled in a given context.
   *
   */
   GLboolean
   __glExtensionBitIsEnabled(struct glx_context *gc, unsigned bit)
   {
               if (gc != NULL) {
                     }
            /**
   * Convert a bit-field to a string of supported extensions.
   */
   static char *
   __glXGetStringFromTable(const struct extension_info *ext,
         {
      unsigned i;
   unsigned ext_str_len;
   char *ext_str;
               ext_str_len = 0;
   for (i = 0; ext[i].name != NULL; i++) {
      if (!filter || EXT_ENABLED(ext[i].bit, filter)) {
                     ext_str = malloc(ext_str_len + 1);
   if (ext_str != NULL) {
               for (i = 0; ext[i].name != NULL; i++) {
      if (!filter || EXT_ENABLED(ext[i].bit, filter)) {
                     *point = ' ';
                                 }
         /**
   * Get the string of client library supported extensions.
   */
   const char *
   __glXGetClientExtensions(Display *dpy)
   {
      if (__glXGLXClientExtensions == NULL) {
      __glXExtensionsCtr();
   __glXGLXClientExtensions = __glXGetStringFromTable(known_glx_extensions,
                  }
         /**
   * Calculate the list of application usable extensions.  The resulting
   * string is stored in \c psc->effectiveGLXexts.
   *
   * \param psc                        Pointer to GLX per-screen record.
   * \param display_is_direct_capable  True if the display is capable of
   *                                   direct rendering.
   */
      void
   __glXCalculateUsableExtensions(struct glx_screen * psc,
         {
      unsigned char server_support[__GLX_EXT_BYTES];
   unsigned char usable[__GLX_EXT_BYTES];
            __glXExtensionsCtr();
            (void) memset(server_support, 0, sizeof(server_support));
   __glXProcessServerString(known_glx_extensions,
               /* An extension is supported if the client-side (i.e., libGL) supports
   * it and the "server" supports it.  In this case that means that either
   * the true server supports it or it is only for direct-rendering and
   * the direct rendering driver supports it.
   *
   * If the display is not capable of direct rendering, then the extension
   * is enabled if and only if the client-side library and the server
   * support it.
            if (display_is_direct_capable) {
      for (i = 0; i < __GLX_EXT_BYTES; i++) {
      /* Enable extensions that the client supports that only have a client-side
   * component.
                  /* Enable extensions that are supported for direct rendering, and either
   * are supported by the server or only have a direct-rendering component.
                  /* Finally, apply driconf options to force some extension bits either
   * enabled or disabled.
   */
                        }
   else {
      for (i = 0; i < __GLX_EXT_BYTES; i++) {
      /* Enable extensions that the client supports that only have a
   * client-side component.
                                 /* Finally, apply driconf options to force some extension bits either
   * enabled or disabled.
   */
                                 psc->effectiveGLXexts = __glXGetStringFromTable(known_glx_extensions,
      }
      /**
   * Calculate the list of application usable extensions.  The resulting
   * string is stored in \c gc->extensions.
   *
   * \param gc             Pointer to GLX context.
   * \param server_string  Extension string from the server.
   */
      void
   __glXCalculateUsableGLExtensions(struct glx_context * gc,
         {
      struct glx_screen *psc = gc->psc;
   unsigned char server_support[__GL_EXT_BYTES];
   unsigned char usable[__GL_EXT_BYTES];
            (void) memset(server_support, 0, sizeof(server_support));
   __glXProcessServerString(known_gl_extensions, server_string,
            /* These extensions are wholly inside the client-side indirect code */
   (void) memset(usable, 0, sizeof(usable));
   SET_BIT(usable, GL_ARB_transpose_matrix_bit);
   SET_BIT(usable, GL_EXT_draw_range_elements_bit);
   SET_BIT(usable, GL_EXT_multi_draw_arrays_bit);
            for (i = 0; i < __GL_EXT_BYTES; i++) {
      /* Usable if the server supports it, or if it's been forced on */
            /* But not if it's been forced off */
               gc->extensions = (unsigned char *)
            }
      /**
   * Get a string representing the set of extensions supported by the client
   * library.  This is currently only used to send the list of extensions
   * supported by the client to the server.
   */
   char *
   __glXGetClientGLExtensionString(int screen)
   {
      if (screen < 0)
               }
