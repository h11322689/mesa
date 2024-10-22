   /*
   * Copyright © 2014 Intel Corporation
   * Copyright © 2016 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /**
   * This program reads and compiles multiple GLSL shaders from one source file.
   * Each shader must begin with the #shader directive. Syntax:
   *     #shader [vs|tcs|tes|gs|ps|cs] [name]
   *
   * The shader name is printed, followed by the stderr output from
   * glCreateShaderProgramv. (radeonsi prints the shader disassembly there)
   *
   * The optional parameter -mcpu=[processor] forces radeonsi to compile for
   * the specified GPU processor. (e.g. tahiti, bonaire, tonga)
   *
   * The program doesn't check if the underlying driver is really radeonsi.
   * OpenGL 4.3 Core profile is required.
   */
      /* for asprintf() */
   #define _GNU_SOURCE
      #include <stdio.h>
   #include <fcntl.h>
   #include <string.h>
   #include <stdlib.h>
      #include <epoxy/gl.h>
   #include <epoxy/egl.h>
   #include <gbm.h>
      #define unlikely(x) __builtin_expect(!!(x), 0)
   #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
      static int fd;
   static EGLDisplay egl_dpy;
   static EGLContext ctx;
      static void
   create_gl_core_context()
   {
      const char *client_extensions = eglQueryString(EGL_NO_DISPLAY,
         if (!client_extensions) {
      fprintf(stderr, "ERROR: Missing EGL_EXT_client_extensions\n");
               if (!strstr(client_extensions, "EGL_MESA_platform_gbm")) {
      fprintf(stderr, "ERROR: Missing EGL_MESA_platform_gbm\n");
               fd = open("/dev/dri/renderD128", O_RDWR);
   if (unlikely(fd < 0)) {
      fprintf(stderr, "ERROR: Couldn't open /dev/dri/renderD128\n");
               struct gbm_device *gbm = gbm_create_device(fd);
   if (unlikely(gbm == NULL)) {
      fprintf(stderr, "ERROR: Couldn't create gbm device\n");
               egl_dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA,
         if (unlikely(egl_dpy == EGL_NO_DISPLAY)) {
      fprintf(stderr, "ERROR: eglGetDisplay() failed\n");
               if (unlikely(!eglInitialize(egl_dpy, NULL, NULL))) {
      fprintf(stderr, "ERROR: eglInitialize() failed\n");
               static const char *egl_extension[] = {
               };
   const char *extension_string = eglQueryString(egl_dpy, EGL_EXTENSIONS);
   for (int i = 0; i < ARRAY_SIZE(egl_extension); i++) {
      if (strstr(extension_string, egl_extension[i]) == NULL) {
         fprintf(stderr, "ERROR: Missing %s\n", egl_extension[i]);
               static const EGLint config_attribs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      };
   EGLConfig cfg;
            if (!eglChooseConfig(egl_dpy, config_attribs, &cfg, 1, &count) ||
      count == 0) {
   fprintf(stderr, "ERROR: eglChooseConfig() failed\n");
      }
            static const EGLint attribs[] = {
      EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
   EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
   EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
   EGL_CONTEXT_MINOR_VERSION_KHR, 3,
      };
   ctx = eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, attribs);
   if (ctx == EGL_NO_CONTEXT) {
      fprintf(stderr, "eglCreateContext(GL 3.2) failed.\n");
               if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
      fprintf(stderr, "eglMakeCurrent failed.\n");
         }
      static char *
   read_file(const char *filename)
   {
      FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Can't open file: %s\n", filename);
               fseek(f, 0, SEEK_END);
   int filesize = ftell(f);
            char *input = (char*)malloc(filesize + 1);
   if (!input) {
      fprintf(stderr, "malloc failed\n");
               if (fread(input, filesize, 1, f) != 1) {
      fprintf(stderr, "fread failed\n");
      }
            input[filesize] = 0;
      }
      static void addenv(const char *name, const char *value)
   {
      const char *orig = getenv(name);
   if (orig) {
      char *newval;
   (void)!asprintf(&newval, "%s,%s", orig, value);
   setenv(name, newval, 1);
      } else {
            }
      int
   main(int argc, char **argv)
   {
               for (int i = 1; i < argc; i++) {
      if (strstr(argv[i], "-mcpu=") == argv[i]) {
         } else if (filename == NULL) {
         } else {
                        fprintf(stderr, "Usage: amdgcn_glslc -mcpu=[chip name] [glsl filename]\n");
               if (filename == NULL) {
      fprintf(stderr, "No filename specified.\n");
                                 /* Read the source. */
            /* Comment out lines beginning with ; (FileCheck prefix). */
   if (input[0] == ';')
            char *s = input;
   while (s = strstr(s, "\n;"))
            s = input;
   while (s && (s = strstr(s, "#shader "))) {
      char type_str[16], name[128];
            /* If #shader is not at the beginning of the line. */
   if (s != input && s[-1] != '\n' && s[-1] != 0) {
         s = strstr(s, "\n");
            /* Parse the #shader directive. */
   if (sscanf(s + 8, "%s %s", type_str, name) != 2) {
         fprintf(stderr, "Cannot parse #shader directive.\n");
            if (!strcmp(type_str, "vs"))
         else if (!strcmp(type_str, "tcs"))
         else if (!strcmp(type_str, "tes"))
         else if (!strcmp(type_str, "gs"))
         else if (!strcmp(type_str, "fs"))
         else if (!strcmp(type_str, "cs"))
            /* Go the next line. */
   s = strstr(s, "\n");
   if (!s)
                           /* Cut the shader source at the end. */
   s = strstr(s, "#shader");
   if (s && s[-1] == '\n')
            /* Compile the shader. */
            /* Redirect stderr to stdout for the compiler. */
   FILE *stderr_original = stderr;
   stderr = stdout;
   GLuint prog = glCreateShaderProgramv(type, 1, &source);
            GLint linked;
            if (!linked) {
                        glGetProgramInfoLog(prog, sizeof(log), &length, log);
   fprintf(stderr, "ERROR: Compile failure:\n\n%s\n%s\n",
                     }
      }
