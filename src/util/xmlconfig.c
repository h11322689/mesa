   /*
   * XML DRI client-side driver configuration
   * Copyright (C) 2003 Felix Kuehling
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
   * FELIX KUEHLING, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   */
   /**
   * \file xmlconfig.c
   * \brief Driver-independent client-side part of the XML configuration
   * \author Felix Kuehling
   */
      #include "xmlconfig.h"
   #include <limits.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <assert.h>
   #if WITH_XMLCONFIG
   #include <expat.h>
   #include <unistd.h>
   #include <errno.h>
   #include <dirent.h>
   #include <sys/stat.h>
   #endif
   #ifdef NO_REGEX
   typedef int regex_t;
   #define REG_EXTENDED 0
   #define REG_NOSUB 0
   #define REG_NOMATCH 1
   static inline int regcomp(regex_t *r, const char *s, int f) { return 0; }
   static inline int regexec(regex_t *r, const char *s, int n, void *p, int f) { return REG_NOMATCH; }
   static inline void regfree(regex_t* r) {}
   #else
   #include <regex.h>
   #endif
   #include <fcntl.h>
   #include <math.h>
   #include "strndup.h"
   #include "u_process.h"
   #include "os_file.h"
   #include "os_misc.h"
      /* For systems like Hurd */
   #ifndef PATH_MAX
   #define PATH_MAX 4096
   #endif
      static bool
   be_verbose(void)
   {
      const char *s = getenv("MESA_DEBUG");
   if (!s)
               }
      /** \brief Locale-independent integer parser.
   *
   * Works similar to strtol. Leading space is NOT skipped. The input
   * number may have an optional sign. Radix is specified by base. If
   * base is 0 then decimal is assumed unless the input number is
   * prefixed by 0x or 0X for hexadecimal or 0 for octal. After
   * returning tail points to the first character that is not part of
   * the integer number. If no number was found then tail points to the
   * start of the input string. */
   static int
   strToI(const char *string, const char **tail, int base)
   {
      int radix = base == 0 ? 10 : base;
   int result = 0;
   int sign = 1;
   bool numberFound = false;
                     if (*string == '-') {
      sign = -1;
      } else if (*string == '+')
         if (base == 0 && *string == '0') {
      numberFound = true;
   if (*(string+1) == 'x' || *(string+1) == 'X') {
      radix = 16;
      } else {
      radix = 8;
         }
   do {
      int digit = -1;
   if (radix <= 10) {
      if (*string >= '0' && *string < '0' + radix)
      } else {
      if (*string >= '0' && *string <= '9')
         else if (*string >= 'a' && *string < 'a' + radix - 10)
         else if (*string >= 'A' && *string < 'A' + radix - 10)
      }
   if (digit != -1) {
      numberFound = true;
   result = radix*result + digit;
      } else
      } while (true);
   *tail = numberFound ? string : start;
      }
      /** \brief Locale-independent floating-point parser.
   *
   * Works similar to strtod. Leading space is NOT skipped. The input
   * number may have an optional sign. '.' is interpreted as decimal
   * point and may occur at most once. Optionally the number may end in
   * [eE]<exponent>, where <exponent> is an integer as recognized by
   * strToI. In that case the result is number * 10^exponent. After
   * returning tail points to the first character that is not part of
   * the floating point number. If no number was found then tail points
   * to the start of the input string.
   *
   * Uses two passes for maximum accuracy. */
   static float
   strToF(const char *string, const char **tail)
   {
      int nDigits = 0, pointPos, exponent;
   float sign = 1.0f, result = 0.0f, scale;
            /* sign */
   if (*string == '-') {
      sign = -1.0f;
      } else if (*string == '+')
            /* first pass: determine position of decimal point, number of
   * digits, exponent and the end of the number. */
   numStart = string;
   while (*string >= '0' && *string <= '9') {
      string++;
      }
   pointPos = nDigits;
   if (*string == '.') {
      string++;
   while (*string >= '0' && *string <= '9') {
      string++;
         }
   if (nDigits == 0) {
      /* no digits, no number */
   *tail = start;
      }
   *tail = string;
   if (*string == 'e' || *string == 'E') {
      const char *expTail;
   exponent = strToI(string+1, &expTail, 10);
   if (expTail == string+1)
         else
      } else
                  /* scale of the first digit */
            /* second pass: parse digits */
   do {
      if (*string != '.') {
      assert(*string >= '0' && *string <= '9');
   result += scale * (float)(*string - '0');
   scale *= 0.1f;
      }
                  }
      /** \brief Parse a value of a given type. */
   static unsigned char
   parseValue(driOptionValue *v, driOptionType type, const char *string)
   {
      const char *tail = NULL;
   /* skip leading white-space */
   string += strspn(string, " \f\n\r\t\v");
   switch (type) {
   case DRI_BOOL:
      if (!strcmp(string, "false")) {
      v->_bool = false;
      } else if (!strcmp(string, "true")) {
      v->_bool = true;
      }
   else
            case DRI_ENUM: /* enum is just a special integer */
   case DRI_INT:
      v->_int = strToI(string, &tail, 0);
      case DRI_FLOAT:
      v->_float = strToF(string, &tail);
      case DRI_STRING:
      free(v->_string);
   v->_string = strndup(string, STRING_CONF_MAXLEN);
      case DRI_SECTION:
                  if (tail == string)
         /* skip trailing white space */
   if (*tail)
         if (*tail)
               }
      /** \brief Find an option in an option cache with the name as key */
   static uint32_t
   findOption(const driOptionCache *cache, const char *name)
   {
      uint32_t len = strlen(name);
   uint32_t size = 1 << cache->tableSize, mask = size - 1;
   uint32_t hash = 0;
            /* compute a hash from the variable length name */
   for (i = 0, shift = 0; i < len; ++i, shift = (shift+8) & 31)
         hash *= hash;
            /* this is just the starting point of the linear search for the option */
   for (i = 0; i < size; ++i, hash = (hash+1) & mask) {
      /* if we hit an empty entry then the option is not defined (yet) */
   if (cache->info[hash].name == NULL)
         else if (!strcmp(name, cache->info[hash].name))
      }
   /* this assertion fails if the hash table is full */
               }
      /** \brief Like strdup with error checking. */
   #define XSTRDUP(dest,source) do {                                       \
         if (!(dest = strdup(source))) {                                   \
      fprintf(stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__); \
               /** \brief Check if a value is in info->range. */
   UNUSED static bool
   checkValue(const driOptionValue *v, const driOptionInfo *info)
   {
      switch (info->type) {
   case DRI_ENUM: /* enum is just a special integer */
   case DRI_INT:
      return (info->range.start._int == info->range.end._int ||
               case DRI_FLOAT:
      return (info->range.start._float == info->range.end._float ||
               default:
            }
      void
   driParseOptionInfo(driOptionCache *info,
               {
      /* Make the hash table big enough to fit more than the maximum number of
   * config options we've ever seen in a driver.
   */
   info->tableSize = 7;
   info->info = calloc((size_t)1 << info->tableSize, sizeof(driOptionInfo));
   info->values = calloc((size_t)1 << info->tableSize, sizeof(driOptionValue));
   if (info->info == NULL || info->values == NULL) {
      fprintf(stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
               UNUSED bool in_section = false;
   for (int o = 0; o < numOptions; o++) {
               if (opt->info.type == DRI_SECTION) {
      in_section = true;
               /* for driconf xml generation, options must always be preceded by a
   * DRI_CONF_SECTION
   */
            const char *name = opt->info.name;
   int i = findOption(info, name);
   driOptionInfo *optinfo = &info->info[i];
            if (optinfo->name) {
      /* Duplicate options override the value, but the type must match. */
      } else {
                  optinfo->type = opt->info.type;
            switch (opt->info.type) {
   case DRI_BOOL:
                  case DRI_INT:
   case DRI_ENUM:
                  case DRI_FLOAT:
                  case DRI_STRING:
                  case DRI_SECTION:
                  /* Built-in default values should always be valid. */
            const char *envVal = os_get_option(name);
   if (envVal != NULL) {
                              if (parseValue(&v, opt->info.type, envVal) &&
      checkValue(&v, optinfo)) {
   /* don't use XML_WARNING, we want the user to see this! */
   if (be_verbose()) {
      fprintf(stderr,
            }
      } else {
      fprintf(stderr, "illegal environment value for %s: \"%s\".  Ignoring.\n",
               }
      char *
   driGetOptionsXml(const driOptionDescription *configOptions, unsigned numOptions)
   {
      char *str = ralloc_strdup(NULL,
      "<?xml version=\"1.0\" standalone=\"yes\"?>\n" \
   "<!DOCTYPE driinfo [\n" \
   "   <!ELEMENT driinfo      (section*)>\n" \
   "   <!ELEMENT section      (description+, option+)>\n" \
   "   <!ELEMENT description  (enum*)>\n" \
   "   <!ATTLIST description  lang CDATA #FIXED \"en\"\n" \
   "                          text CDATA #REQUIRED>\n" \
   "   <!ELEMENT option       (description+)>\n" \
   "   <!ATTLIST option       name CDATA #REQUIRED\n" \
   "                          type (bool|enum|int|float) #REQUIRED\n" \
   "                          default CDATA #REQUIRED\n" \
   "                          valid CDATA #IMPLIED>\n" \
   "   <!ELEMENT enum         EMPTY>\n" \
   "   <!ATTLIST enum         value CDATA #REQUIRED\n" \
   "                          text CDATA #REQUIRED>\n" \
   "]>" \
         bool in_section = false;
   for (int o = 0; o < numOptions; o++) {
               const char *name = opt->info.name;
   const char *types[] = {
      [DRI_BOOL] = "bool",
   [DRI_INT] = "int",
   [DRI_FLOAT] = "float",
   [DRI_ENUM] = "enum",
               if (opt->info.type == DRI_SECTION) {
                     ralloc_asprintf_append(&str,
                        in_section = true;
               ralloc_asprintf_append(&str,
                        switch (opt->info.type) {
   case DRI_BOOL:
                  case DRI_INT:
   case DRI_ENUM:
                  case DRI_FLOAT:
                  case DRI_STRING:
                  case DRI_SECTION:
      unreachable("handled above");
      }
               switch (opt->info.type) {
   case DRI_INT:
   case DRI_ENUM:
      if (opt->info.range.start._int < opt->info.range.end._int) {
      ralloc_asprintf_append(&str, " valid=\"%d:%d\"",
                        case DRI_FLOAT:
      if (opt->info.range.start._float < opt->info.range.end._float) {
      ralloc_asprintf_append(&str, " valid=\"%f:%f\"",
                        default:
                              ralloc_asprintf_append(&str, "        <description lang=\"en\" text=\"%s\"%s>\n",
            if (opt->info.type == DRI_ENUM) {
      for (int i = 0; i < ARRAY_SIZE(opt->enums) && opt->enums[i].desc; i++) {
      ralloc_asprintf_append(&str, "          <enum value=\"%d\" text=\"%s\"/>\n",
      }
                           assert(in_section);
                     char *output = strdup(str);
               }
      /**
   * Print message to \c stderr if the \c LIBGL_DEBUG environment variable
   * is set.
   *
   * Is called from the drivers.
   *
   * \param f \c printf like format string.
   */
   static void
   __driUtilMessage(const char *f, ...)
   {
      va_list args;
            libgl_debug=getenv("LIBGL_DEBUG");
   if (libgl_debug && !strstr(libgl_debug, "quiet")) {
      fprintf(stderr, "libGL: ");
   va_start(args, f);
   vfprintf(stderr, f, args);
   va_end(args);
         }
      /* We don't have real line/column # info in static-config case: */
   #if !WITH_XML_CONFIG
   #  define XML_GetCurrentLineNumber(p) -1
   #  define XML_GetCurrentColumnNumber(p) -1
   #endif
      /** \brief Output a warning message. */
   #define XML_WARNING1(msg) do {                                          \
         __driUtilMessage("Warning in %s line %d, column %d: "msg, data->name, \
               #define XML_WARNING(msg, ...) do {                                      \
         __driUtilMessage("Warning in %s line %d, column %d: "msg, data->name, \
                     /** \brief Output an error message. */
   #define XML_ERROR1(msg) do {                                            \
         __driUtilMessage("Error in %s line %d, column %d: "msg, data->name, \
               #define XML_ERROR(msg, ...) do {                                        \
         __driUtilMessage("Error in %s line %d, column %d: "msg, data->name, \
                        /** \brief Parser context for configuration files. */
   struct OptConfData {
         #if WITH_XMLCONFIG
         #endif
      driOptionCache *cache;
   int screenNum;
   const char *driverName, *execName;
   const char *kernelDriverName;
   const char *deviceName;
   const char *engineName;
   const char *applicationName;
   uint32_t engineVersion;
   uint32_t applicationVersion;
   uint32_t ignoringDevice;
   uint32_t ignoringApp;
   uint32_t inDriConf;
   uint32_t inDevice;
   uint32_t inApp;
      };
      /** \brief Parse a list of ranges of type info->type. */
   static unsigned char
   parseRange(driOptionInfo *info, const char *string)
   {
                        char *sep;
   sep = strchr(cp, ':');
   if (!sep) {
      free(cp);
               *sep = '\0';
   if (!parseValue(&info->range.start, info->type, cp) ||
      !parseValue(&info->range.end, info->type, sep+1)) {
   free(cp);
      }
   if (info->type == DRI_INT &&
      info->range.start._int >= info->range.end._int) {
   free(cp);
      }
   if (info->type == DRI_FLOAT &&
      info->range.start._float >= info->range.end._float) {
   free(cp);
               free(cp);
      }
      /** \brief Parse attributes of a device element. */
   static void
   parseDeviceAttr(struct OptConfData *data, const char **attr)
   {
      uint32_t i;
   const char *driver = NULL, *screen = NULL, *kernel = NULL, *device = NULL;
   for (i = 0; attr[i]; i += 2) {
      if (!strcmp(attr[i], "driver")) driver = attr[i+1];
   else if (!strcmp(attr[i], "screen")) screen = attr[i+1];
   else if (!strcmp(attr[i], "kernel_driver")) kernel = attr[i+1];
   else if (!strcmp(attr[i], "device")) device = attr[i+1];
      }
   if (driver && strcmp(driver, data->driverName))
         else if (kernel && (!data->kernelDriverName ||
               else if (device && (!data->deviceName ||
               else if (screen) {
      driOptionValue screenNum;
   if (!parseValue(&screenNum, DRI_INT, screen))
         else if (screenNum._int != data->screenNum)
         }
      /** \brief Parse attributes of an application element. */
   static void
   parseAppAttr(struct OptConfData *data, const char **attr)
   {
      uint32_t i;
   const char *exec = NULL;
   const char *sha1 = NULL;
   const char *exec_regexp = NULL;
   const char *application_name_match = NULL;
   const char *application_versions = NULL;
   driOptionInfo version_range = {
                  for (i = 0; attr[i]; i += 2) {
      if (!strcmp(attr[i], "name")) /* not needed here */;
   else if (!strcmp(attr[i], "executable")) exec = attr[i+1];
   else if (!strcmp(attr[i], "executable_regexp")) exec_regexp = attr[i+1];
   else if (!strcmp(attr[i], "sha1")) sha1 = attr[i+1];
   else if (!strcmp(attr[i], "application_name_match"))
         else if (!strcmp(attr[i], "application_versions"))
            }
   if (exec && strcmp(exec, data->execName)) {
         } else if (exec_regexp) {
               if (regcomp(&re, exec_regexp, REG_EXTENDED|REG_NOSUB) == 0) {
      if (regexec(&re, data->execName, 0, NULL, 0) == REG_NOMATCH)
            } else
      } else if (sha1) {
      /* SHA1_DIGEST_STRING_LENGTH includes terminating null byte */
   if (strlen(sha1) != (SHA1_DIGEST_STRING_LENGTH - 1)) {
      XML_WARNING("Incorrect sha1 application attribute");
      } else {
      size_t len;
   char* content;
   char path[PATH_MAX];
   if (util_get_process_exec_path(path, ARRAY_SIZE(path)) > 0 &&
      (content = os_read_file(path, &len))) {
   uint8_t sha1x[SHA1_DIGEST_LENGTH];
   char sha1s[SHA1_DIGEST_STRING_LENGTH];
   _mesa_sha1_compute(content, len, sha1x);
                  if (strcmp(sha1, sha1s)) {
            } else {
               } else if (application_name_match) {
               if (regcomp(&re, application_name_match, REG_EXTENDED|REG_NOSUB) == 0) {
      if (regexec(&re, data->applicationName, 0, NULL, 0) == REG_NOMATCH)
            } else
      }
   if (application_versions) {
      driOptionValue v = { ._int = data->applicationVersion };
   if (parseRange(&version_range, application_versions)) {
      if (!checkValue(&v, &version_range))
      } else {
      XML_WARNING("Failed to parse application_versions range=\"%s\".",
            }
      /** \brief Parse attributes of an application element. */
   static void
   parseEngineAttr(struct OptConfData *data, const char **attr)
   {
      uint32_t i;
   const char *engine_name_match = NULL, *engine_versions = NULL;
   driOptionInfo version_range = {
         };
   for (i = 0; attr[i]; i += 2) {
      if (!strcmp(attr[i], "name")) /* not needed here */;
   else if (!strcmp(attr[i], "engine_name_match")) engine_name_match = attr[i+1];
   else if (!strcmp(attr[i], "engine_versions")) engine_versions = attr[i+1];
      }
   if (engine_name_match) {
               if (regcomp(&re, engine_name_match, REG_EXTENDED|REG_NOSUB) == 0) {
      if (regexec(&re, data->engineName, 0, NULL, 0) == REG_NOMATCH)
            } else
      }
   if (engine_versions) {
      driOptionValue v = { ._int = data->engineVersion };
   if (parseRange(&version_range, engine_versions)) {
      if (!checkValue(&v, &version_range))
      } else {
      XML_WARNING("Failed to parse engine_versions range=\"%s\".",
            }
      /** \brief Parse attributes of an option element. */
   static void
   parseOptConfAttr(struct OptConfData *data, const char **attr)
   {
      uint32_t i;
   const char *name = NULL, *value = NULL;
   for (i = 0; attr[i]; i += 2) {
      if (!strcmp(attr[i], "name")) name = attr[i+1];
   else if (!strcmp(attr[i], "value")) value = attr[i+1];
      }
   if (!name) XML_WARNING1("name attribute missing in option.");
   if (!value) XML_WARNING1("value attribute missing in option.");
   if (name && value) {
      driOptionCache *cache = data->cache;
   uint32_t opt = findOption(cache, name);
   if (cache->info[opt].name == NULL)
      /* don't use XML_WARNING, drirc defines options for all drivers,
   * but not all drivers support them */
      else if (getenv(cache->info[opt].name)) {
      /* don't use XML_WARNING, we want the user to see this! */
   if (be_verbose()) {
      fprintf(stderr,
               } else if (!parseValue(&cache->values[opt], cache->info[opt].type, value))
         }
      #if WITH_XMLCONFIG
      /** \brief Elements in configuration files. */
   enum OptConfElem {
         };
   static const char *OptConfElems[] = {
      [OC_APPLICATION]  = "application",
   [OC_DEVICE] = "device",
   [OC_DRICONF] = "driconf",
   [OC_ENGINE]  = "engine",
      };
      static int compare(const void *a, const void *b) {
         }
   /** \brief Binary search in a string array. */
   static uint32_t
   bsearchStr(const char *name, const char *elems[], uint32_t count)
   {
      const char **found;
   found = bsearch(&name, elems, count, sizeof(char *), compare);
   if (found)
         else
      }
      /** \brief Handler for start element events. */
   static void
   optConfStartElem(void *userData, const char *name,
         {
      struct OptConfData *data = (struct OptConfData *)userData;
   enum OptConfElem elem = bsearchStr(name, OptConfElems, OC_COUNT);
   switch (elem) {
   case OC_DRICONF:
      if (data->inDriConf)
         if (attr[0])
         data->inDriConf++;
      case OC_DEVICE:
      if (!data->inDriConf)
         if (data->inDevice)
         data->inDevice++;
   if (!data->ignoringDevice && !data->ignoringApp)
            case OC_APPLICATION:
      if (!data->inDevice)
         if (data->inApp)
         data->inApp++;
   if (!data->ignoringDevice && !data->ignoringApp)
            case OC_ENGINE:
      if (!data->inDevice)
         if (data->inApp)
         data->inApp++;
   if (!data->ignoringDevice && !data->ignoringApp)
            case OC_OPTION:
      if (!data->inApp)
         if (data->inOption)
         data->inOption++;
   if (!data->ignoringDevice && !data->ignoringApp)
            default:
            }
      /** \brief Handler for end element events. */
   static void
   optConfEndElem(void *userData, const char *name)
   {
      struct OptConfData *data = (struct OptConfData *)userData;
   enum OptConfElem elem = bsearchStr(name, OptConfElems, OC_COUNT);
   switch (elem) {
   case OC_DRICONF:
      data->inDriConf--;
      case OC_DEVICE:
      if (data->inDevice-- == data->ignoringDevice)
            case OC_APPLICATION:
   case OC_ENGINE:
      if (data->inApp-- == data->ignoringApp)
            case OC_OPTION:
      data->inOption--;
      default:
            }
      static void
   _parseOneConfigFile(XML_Parser p)
   {
   #define BUF_SIZE 0x1000
      struct OptConfData *data = (struct OptConfData *)XML_GetUserData(p);
   int status;
            if ((fd = open(data->name, O_RDONLY)) == -1) {
      __driUtilMessage("Can't open configuration file %s: %s.",
                     while (1) {
      int bytesRead;
   void *buffer = XML_GetBuffer(p, BUF_SIZE);
   if (!buffer) {
      __driUtilMessage("Can't allocate parser buffer.");
      }
   bytesRead = read(fd, buffer, BUF_SIZE);
   if (bytesRead == -1) {
      __driUtilMessage("Error reading from configuration file %s: %s.",
            }
   status = XML_ParseBuffer(p, bytesRead, bytesRead == 0);
   if (!status) {
      XML_ERROR("%s.", XML_ErrorString(XML_GetErrorCode(p)));
      }
   if (bytesRead == 0)
                  #undef BUF_SIZE
   }
      /** \brief Parse the named configuration file */
   static void
   parseOneConfigFile(struct OptConfData *data, const char *filename)
   {
               p = XML_ParserCreate(NULL); /* use encoding specified by file */
   XML_SetElementHandler(p, optConfStartElem, optConfEndElem);
   XML_SetUserData(p, data);
   data->parser = p;
   data->name = filename;
   data->ignoringDevice = 0;
   data->ignoringApp = 0;
   data->inDriConf = 0;
   data->inDevice = 0;
   data->inApp = 0;
            _parseOneConfigFile(p);
      }
      static int
   scandir_filter(const struct dirent *ent)
   {
   #ifndef DT_REG /* systems without d_type in dirent results */
               if ((lstat(ent->d_name, &st) != 0) ||
      (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)))
   #else
      /* Allow through unknown file types for filesystems that don't support d_type
   * The full filepath isn't available here to stat the file
   */
   if (ent->d_type != DT_REG && ent->d_type != DT_LNK && ent->d_type != DT_UNKNOWN)
      #endif
         int len = strlen(ent->d_name);
   if (len <= 5 || strcmp(ent->d_name + len - 5, ".conf"))
               }
      /** \brief Parse configuration files in a directory */
   static void
   parseConfigDir(struct OptConfData *data, const char *dirname)
   {
      int i, count;
            count = scandir(dirname, &entries, scandir_filter, alphasort);
   if (count < 0)
            for (i = 0; i < count; i++) {
      #ifdef DT_REG
         #endif
            snprintf(filename, PATH_MAX, "%s/%s", dirname, entries[i]->d_name);
      #ifdef DT_REG
         /* In the case of unknown d_type, ensure it is a regular file
   * This can be accomplished with stat on the full filepath
   */
   if (d_type == DT_UNKNOWN) {
      struct stat st;
   if (stat(filename, &st) != 0 ||
      !S_ISREG(st.st_mode)) {
         #endif
                           }
   #else
   #  include "driconf_static.h"
      static void
   parseStaticOptions(struct OptConfData *data, const struct driconf_option *options,
         {
      if (data->ignoringDevice || data->ignoringApp)
         for (unsigned i = 0; i < num_options; i++) {
      const char *optattr[] = {
      "name", options[i].name,
   "value", options[i].value,
      };
         }
      static void
   parseStaticConfig(struct OptConfData *data)
   {
      data->ignoringDevice = 0;
   data->ignoringApp = 0;
   data->inDriConf = 0;
   data->inDevice = 0;
   data->inApp = 0;
            for (unsigned i = 0; i < ARRAY_SIZE(driconf); i++) {
      const struct driconf_device *d = driconf[i];
   const char *devattr[] = {
      "driver", d->driver,
   "device", d->device,
               data->ignoringDevice = 0;
   data->inDevice++;
   parseDeviceAttr(data, devattr);
                     for (unsigned j = 0; j < d->num_engines; j++) {
      const struct driconf_engine *e = &d->engines[j];
   const char *engattr[] = {
      "engine_name_match", e->engine_name_match,
   "engine_versions", e->engine_versions,
               data->ignoringApp = 0;
   parseEngineAttr(data, engattr);
               for (unsigned j = 0; j < d->num_applications; j++) {
      const struct driconf_application *a = &d->applications[j];
   const char *appattr[] = {
      "name", a->name,
   "executable", a->executable,
   "executable_regexp", a->executable_regexp,
   "sha1", a->sha1,
   "application_name_match", a->application_name_match,
   "application_versions", a->application_versions,
               data->ignoringApp = 0;
   parseAppAttr(data, appattr);
                     }
   #endif /* WITH_XMLCONFIG */
      /** \brief Initialize an option cache based on info */
   static void
   initOptionCache(driOptionCache *cache, const driOptionCache *info)
   {
      unsigned i, size = 1 << info->tableSize;
   cache->info = info->info;
   cache->tableSize = info->tableSize;
   cache->values = malloc(((size_t)1 << info->tableSize) * sizeof(driOptionValue));
   if (cache->values == NULL) {
      fprintf(stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
      }
   memcpy(cache->values, info->values,
         for (i = 0; i < size; ++i) {
      if (cache->info[i].type == DRI_STRING)
         }
      #ifndef SYSCONFDIR
   #define SYSCONFDIR "/etc"
   #endif
      #ifndef DATADIR
   #define DATADIR "/usr/share"
   #endif
      static const char *execname;
      void
   driInjectExecName(const char *exec)
   {
         }
      void
   driParseConfigFiles(driOptionCache *cache, const driOptionCache *info,
                     int screenNum, const char *driverName,
   const char *kernelDriverName,
   {
      initOptionCache(cache, info);
            if (!execname)
         if (!execname)
            userData.cache = cache;
   userData.screenNum = screenNum;
   userData.driverName = driverName;
   userData.kernelDriverName = kernelDriverName;
   userData.deviceName = deviceName;
   userData.applicationName = applicationName ? applicationName : "";
   userData.applicationVersion = applicationVersion;
   userData.engineName = engineName ? engineName : "";
   userData.engineVersion = engineVersion;
         #if WITH_XMLCONFIG
               /* parse from either $DRIRC_CONFIGDIR or $datadir/drirc.d */
   if ((configdir = getenv("DRIRC_CONFIGDIR")))
         else {
      parseConfigDir(&userData, DATADIR "/drirc.d");
               if ((home = getenv("HOME"))) {
               snprintf(filename, PATH_MAX, "%s/.drirc", home);
         #else
         #endif /* WITH_XMLCONFIG */
   }
      void
   driDestroyOptionInfo(driOptionCache *info)
   {
      driDestroyOptionCache(info);
   if (info->info) {
      uint32_t i, size = 1 << info->tableSize;
   for (i = 0; i < size; ++i) {
      if (info->info[i].name) {
            }
         }
      void
   driDestroyOptionCache(driOptionCache *cache)
   {
      if (cache->info) {
      unsigned i, size = 1 << cache->tableSize;
   for (i = 0; i < size; ++i) {
      if (cache->info[i].type == DRI_STRING)
         }
      }
      unsigned char
   driCheckOption(const driOptionCache *cache, const char *name,
         {
      uint32_t i = findOption(cache, name);
      }
      unsigned char
   driQueryOptionb(const driOptionCache *cache, const char *name)
   {
      uint32_t i = findOption(cache, name);
   /* make sure the option is defined and has the correct type */
   assert(cache->info[i].name != NULL);
   assert(cache->info[i].type == DRI_BOOL);
      }
      int
   driQueryOptioni(const driOptionCache *cache, const char *name)
   {
      uint32_t i = findOption(cache, name);
   /* make sure the option is defined and has the correct type */
   assert(cache->info[i].name != NULL);
   assert(cache->info[i].type == DRI_INT || cache->info[i].type == DRI_ENUM);
      }
      float
   driQueryOptionf(const driOptionCache *cache, const char *name)
   {
      uint32_t i = findOption(cache, name);
   /* make sure the option is defined and has the correct type */
   assert(cache->info[i].name != NULL);
   assert(cache->info[i].type == DRI_FLOAT);
      }
      char *
   driQueryOptionstr(const driOptionCache *cache, const char *name)
   {
      uint32_t i = findOption(cache, name);
   /* make sure the option is defined and has the correct type */
   assert(cache->info[i].name != NULL);
   assert(cache->info[i].type == DRI_STRING);
      }
