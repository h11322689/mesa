   /*
   * Copyright © 2016 Intel Corporation
   * Copyright © 2017 Broadcom
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "v3d_decoder.h"
      #include <stdio.h>
   #include <stdlib.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdarg.h>
   #include <string.h>
   #ifdef WITH_LIBEXPAT
   #include <expat.h>
   #endif
   #include <inttypes.h>
   #include <zlib.h>
      #include <util/macros.h>
   #include <util/ralloc.h>
   #include <util/u_debug.h>
      #include "v3d_packet_helpers.h"
   #include "v3d_xml.h"
   #include "broadcom/clif/clif_private.h"
      struct v3d_spec {
                  int ncommands;
   struct v3d_group *commands[256];
   int nstructs;
   struct v3d_group *structs[256];
   int nregisters;
   struct v3d_group *registers[256];
   int nenums;
   };
      #ifdef WITH_LIBEXPAT
      struct location {
         const char *filename;
   };
      struct parser_context {
         XML_Parser parser;
   const struct v3d_device_info *devinfo;
   int foo;
            struct v3d_group *group;
            int nvalues;
                     int parse_depth;
   };
      #endif /* WITH_LIBEXPAT */
      const char *
   v3d_group_get_name(struct v3d_group *group)
   {
         }
      uint8_t
   v3d_group_get_opcode(struct v3d_group *group)
   {
         }
      struct v3d_group *
   v3d_spec_find_struct(struct v3d_spec *spec, const char *name)
   {
         for (int i = 0; i < spec->nstructs; i++)
                  }
      struct v3d_group *
   v3d_spec_find_register(struct v3d_spec *spec, uint32_t offset)
   {
         for (int i = 0; i < spec->nregisters; i++)
                  }
      struct v3d_group *
   v3d_spec_find_register_by_name(struct v3d_spec *spec, const char *name)
   {
         for (int i = 0; i < spec->nregisters; i++) {
                        }
      struct v3d_enum *
   v3d_spec_find_enum(struct v3d_spec *spec, const char *name)
   {
         for (int i = 0; i < spec->nenums; i++)
                  }
      #ifdef WITH_LIBEXPAT
      static void __attribute__((noreturn))
   fail(struct location *loc, const char *msg, ...)
   {
                  va_start(ap, msg);
   fprintf(stderr, "%s:%d: error: ",
         vfprintf(stderr, msg, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   }
      static void *
   fail_on_null(void *p)
   {
         if (p == NULL) {
                        }
      static char *
   xstrdup(const char *s)
   {
         }
      static void *
   zalloc(size_t s)
   {
         }
      static void *
   xzalloc(size_t s)
   {
         }
      /* We allow fields to have either a bit index, or append "b" for a byte index.
   */
   static bool
   is_byte_offset(const char *value)
   {
         }
      static void
   get_group_offset_count(const char **atts, uint32_t *offset, uint32_t *count,
         {
         char *p;
            for (i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "count") == 0) {
            *count = strtoul(atts[i + 1], &p, 0);
      } else if (strcmp(atts[i], "start") == 0) {
         } else if (strcmp(atts[i], "size") == 0) {
      }
   }
      static struct v3d_group *
   create_group(struct parser_context *ctx,
               const char *name,
   {
                  group = xzalloc(sizeof(*group));
   if (name)
            group->spec = ctx->spec;
   group->group_offset = 0;
   group->group_count = 0;
            if (parent) {
            group->parent = parent;
   get_group_offset_count(atts,
                           }
      static struct v3d_enum *
   create_enum(struct parser_context *ctx, const char *name, const char **atts)
   {
                  e = xzalloc(sizeof(*e));
   if (name)
                     }
      static void
   get_register_offset(const char **atts, uint32_t *offset)
   {
         char *p;
            for (i = 0; atts[i]; i += 2) {
               }
   }
      static struct v3d_type
   string_to_type(struct parser_context *ctx, const char *s)
   {
         int i, f;
   struct v3d_group *g;
            if (strcmp(s, "int") == 0)
         else if (strcmp(s, "uint") == 0)
         else if (strcmp(s, "bool") == 0)
         else if (strcmp(s, "float") == 0)
         else if (strcmp(s, "f187") == 0)
         else if (strcmp(s, "address") == 0)
         else if (strcmp(s, "offset") == 0)
         else if (sscanf(s, "u%d.%d", &i, &f) == 2)
         else if (sscanf(s, "s%d.%d", &i, &f) == 2)
         else if (g = v3d_spec_find_struct(ctx->spec, s), g != NULL)
         else if (e = v3d_spec_find_enum(ctx->spec, s), e != NULL)
         else if (strcmp(s, "mbo") == 0)
         else
   }
      static struct v3d_field *
   create_field(struct parser_context *ctx, const char **atts)
   {
         struct v3d_field *field;
   char *p;
   int i;
                     for (i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "name") == 0)
         else if (strcmp(atts[i], "start") == 0) {
            field->start = strtoul(atts[i + 1], &p, 0);
      } else if (strcmp(atts[i], "end") == 0) {
            field->end = strtoul(atts[i + 1], &p, 0) - 1;
      } else if (strcmp(atts[i], "size") == 0) {
            size = strtoul(atts[i + 1], &p, 0);
      } else if (strcmp(atts[i], "type") == 0)
         else if (strcmp(atts[i], "default") == 0) {
               } else if (strcmp(atts[i], "minus_one") == 0) {
                     if (size)
            }
      static struct v3d_value *
   create_value(struct parser_context *ctx, const char **atts)
   {
                  for (int i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "name") == 0)
                     }
      static void
   create_and_append_field(struct parser_context *ctx,
         {
         if (ctx->group->nfields == ctx->group->fields_size) {
            ctx->group->fields_size = MAX2(ctx->group->fields_size * 2, 2);
   ctx->group->fields =
                     }
      static void
   set_group_opcode(struct v3d_group *group, const char **atts)
   {
         char *p;
            for (i = 0; atts[i]; i += 2) {
               }
   }
      static bool
   ver_in_range(int ver, int min_ver, int max_ver)
   {
         return ((min_ver == 0 || ver >= min_ver) &&
   }
      static bool
   skip_if_ver_mismatch(struct parser_context *ctx, int min_ver, int max_ver)
   {
         if (!ctx->parse_skip_depth && !ver_in_range(ctx->devinfo->ver,
                              }
      static void
   start_element(void *data, const char *element_name, const char **atts)
   {
         struct parser_context *ctx = data;
   int i;
   const char *name = NULL;
   const char *ver = NULL;
   int min_ver = 0;
                     for (i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "shortname") == 0)
         else if (strcmp(atts[i], "name") == 0 && !name)
         else if (strcmp(atts[i], "gen") == 0)
         else if (strcmp(atts[i], "min_ver") == 0)
                     if (skip_if_ver_mismatch(ctx, min_ver, max_ver))
            if (strcmp(element_name, "vcxml") == 0) {
                                                int major, minor;
   int n = sscanf(ver, "%d.%d", &major, &minor);
   if (n == 0)
                     } else if (strcmp(element_name, "packet") == 0 ||
                              } else if (strcmp(element_name, "register") == 0) {
               } else if (strcmp(element_name, "group") == 0) {
                                 struct v3d_group *group = create_group(ctx, "", atts,
            } else if (strcmp(element_name, "field") == 0) {
         } else if (strcmp(element_name, "enum") == 0) {
         } else if (strcmp(element_name, "value") == 0) {
                  skip:
         }
      static int
   field_offset_compare(const void *a, const void *b)
   {
         return ((*(const struct v3d_field **)a)->start -
   }
      static void
   end_element(void *data, const char *name)
   {
         struct parser_context *ctx = data;
                     if (ctx->parse_skip_depth) {
            if (ctx->parse_skip_depth == ctx->parse_depth)
               if (strcmp(name, "packet") == 0 ||
         strcmp(name, "struct") == 0 ||
                                                      /* V3D packet XML has the packet contents with offsets
   * starting from the first bit after the opcode, to
   * match the spec.  Shift the fields up now.
   */
   for (int i = 0; i < group->nfields; i++) {
            }
   else if (strcmp(name, "struct") == 0)
                        /* Sort the fields in increasing offset order.  The XML might
   * be specified in any order, but we'll want to iterate from
   * the bottom.
                        assert(spec->ncommands < ARRAY_SIZE(spec->commands));
      } else if (strcmp(name, "group") == 0) {
         } else if (strcmp(name, "field") == 0) {
            assert(ctx->group->nfields > 0);
   struct v3d_field *field = ctx->group->fields[ctx->group->nfields - 1];
   size_t size = ctx->nvalues * sizeof(ctx->values[0]);
   field->inline_enum.values = xzalloc(size);
   field->inline_enum.nvalues = ctx->nvalues;
      } else if (strcmp(name, "enum") == 0) {
            struct v3d_enum *e = ctx->enoom;
   size_t size = ctx->nvalues * sizeof(ctx->values[0]);
   e->values = xzalloc(size);
   e->nvalues = ctx->nvalues;
   memcpy(e->values, ctx->values, size);
   ctx->nvalues = 0;
      }
      static void
   character_data(void *data, const XML_Char *s, int len)
   {
   }
      static uint32_t zlib_inflate(const void *compressed_data,
               {
         struct z_stream_s zstream;
                     zstream.next_in = (unsigned char *)compressed_data;
            if (inflateInit(&zstream) != Z_OK)
            out = malloc(4096);
   zstream.next_out = out;
            do {
            switch (inflate(&zstream, Z_SYNC_FLUSH)) {
   case Z_STREAM_END:
         case Z_OK:
         default:
                                       out = realloc(out, 2*zstream.total_out);
   if (out == NULL) {
                           end:
         inflateEnd(&zstream);
   *out_ptr = out;
   }
      #endif /* WITH_LIBEXPAT */
      struct v3d_spec *
   v3d_spec_load(const struct v3d_device_info *devinfo)
   {
         struct v3d_spec *spec = calloc(1, sizeof(struct v3d_spec));
   if (!spec)
      #ifdef WITH_LIBEXPAT
         struct parser_context ctx;
   void *buf;
   uint8_t *text_data = NULL;
   uint32_t text_offset = 0, text_length = 0;
            for (int i = 0; i < ARRAY_SIZE(genxml_files_table); i++) {
            if (i != 0) {
                        if (genxml_files_table[i].ver_10 <= devinfo->ver) {
                     if (text_length == 0) {
            fprintf(stderr, "unable to find gen (%u) data\n", devinfo->ver);
               memset(&ctx, 0, sizeof ctx);
   ctx.parser = XML_ParserCreate(NULL);
   ctx.devinfo = devinfo;
   XML_SetUserData(ctx.parser, &ctx);
   if (ctx.parser == NULL) {
            fprintf(stderr, "failed to create parser\n");
               XML_SetElementHandler(ctx.parser, start_element, end_element);
                     total_length = zlib_inflate(compress_genxmls,
                        buf = XML_GetBuffer(ctx.parser, text_length);
            if (XML_ParseBuffer(ctx.parser, text_length, true) == 0) {
            fprintf(stderr,
            "Error parsing XML at line %ld col %ld byte %ld/%u: %s\n",
   XML_GetCurrentLineNumber(ctx.parser),
   XML_GetCurrentColumnNumber(ctx.parser),
      XML_ParserFree(ctx.parser);
   free(text_data);
               XML_ParserFree(ctx.parser);
            #else /* !WITH_LIBEXPAT */
         debug_warn_once("CLIF dumping not supported due to missing libexpat");
   #endif /* !WITH_LIBEXPAT */
   }
      struct v3d_group *
   v3d_spec_find_instruction(struct v3d_spec *spec, const uint8_t *p)
   {
                  for (int i = 0; i < spec->ncommands; i++) {
                                    /* If there's a "sub-id" field, make sure that it matches the
   * instruction being decoded.
   */
   struct v3d_field *subid = NULL;
   for (int j = 0; j < group->nfields; j++) {
            struct v3d_field *field = group->fields[j];
   if (strcmp(field->name, "sub-id") == 0) {
                     if (subid && (__gen_unpack_uint(p, subid->start, subid->end) !=
                              }
      /** Returns the size of a V3D packet. */
   int
   v3d_group_get_length(struct v3d_group *group)
   {
         int last_bit = 0;
   for (int i = 0; i < group->nfields; i++) {
                  }
   }
      void
   v3d_field_iterator_init(struct v3d_field_iterator *iter,
               {
                  iter->group = group;
   }
      static const char *
   v3d_get_enum_name(struct v3d_enum *e, uint64_t value)
   {
         for (int i = 0; i < e->nvalues; i++) {
            if (e->values[i]->value == value) {
      }
   }
      static bool
   iter_more_fields(const struct v3d_field_iterator *iter)
   {
         }
      static uint32_t
   iter_group_offset_bits(const struct v3d_field_iterator *iter,
         {
         return iter->group->group_offset + (group_iter *
   }
      static bool
   iter_more_groups(const struct v3d_field_iterator *iter)
   {
         if (iter->group->variable) {
               } else {
               }
      static void
   iter_advance_group(struct v3d_field_iterator *iter)
   {
         if (iter->group->variable)
         else {
            if ((iter->group_iter + 1) < iter->group->group_count) {
         } else {
                     }
      static bool
   iter_advance_field(struct v3d_field_iterator *iter)
   {
         while (!iter_more_fields(iter)) {
                                 iter->field = iter->group->fields[iter->field_iter++];
   if (iter->field->name)
         else
         iter->offset = iter_group_offset_bits(iter, iter->group_iter) / 8 +
                  }
      bool
   v3d_field_iterator_next(struct clif_dump *clif, struct v3d_field_iterator *iter)
   {
         if (!iter_advance_field(iter))
                     int group_member_offset =
         int s = group_member_offset + iter->field->start;
            assert(!iter->field->minus_one ||
                  switch (iter->field->type.kind) {
   case V3D_TYPE_UNKNOWN:
   case V3D_TYPE_INT: {
            uint32_t value = __gen_unpack_sint(iter->p, s, e);
   if (iter->field->minus_one)
         snprintf(iter->value, sizeof(iter->value), "%d", value);
      }
   case V3D_TYPE_UINT: {
            uint32_t value = __gen_unpack_uint(iter->p, s, e);
   if (iter->field->minus_one)
         if (strcmp(iter->field->name, "Vec size") == 0 && value == 0)
         snprintf(iter->value, sizeof(iter->value), "%u", value);
      }
   case V3D_TYPE_BOOL:
            snprintf(iter->value, sizeof(iter->value), "%s",
            case V3D_TYPE_FLOAT:
                        case V3D_TYPE_F187:
                        case V3D_TYPE_ADDRESS: {
            uint32_t addr =
         struct clif_bo *bo = clif_lookup_bo(clif, addr);
   if (bo) {
            snprintf(iter->value, sizeof(iter->value),
      } else if (addr) {
               } else {
                              case V3D_TYPE_OFFSET:
            snprintf(iter->value, sizeof(iter->value), "0x%08"PRIx64,
      case V3D_TYPE_STRUCT:
            snprintf(iter->value, sizeof(iter->value), "<struct %s>",
         iter->struct_desc =
            case V3D_TYPE_SFIXED:
            if (clif->pretty) {
            snprintf(iter->value, sizeof(iter->value), "%f",
      } else {
                  case V3D_TYPE_UFIXED:
            if (clif->pretty) {
            snprintf(iter->value, sizeof(iter->value), "%f",
      } else {
                  case V3D_TYPE_MBO:
         case V3D_TYPE_ENUM: {
            uint32_t value = __gen_unpack_uint(iter->p, s, e);
   snprintf(iter->value, sizeof(iter->value), "%d", value);
      }
            if (strlen(iter->group->name) == 0) {
            int length = strlen(iter->name);
               if (enum_name) {
            int length = strlen(iter->value);
               }
      void
   v3d_print_group(struct clif_dump *clif, struct v3d_group *group,
         {
                  v3d_field_iterator_init(&iter, group, p);
   while (v3d_field_iterator_next(clif, &iter)) {
            /* Clif parsing uses the packet name, and expects no
   * sub-id.
   */
   if (strcmp(iter.field->name, "sub-id") == 0 ||
                        if (clif->pretty) {
               } else {
               }
   if (iter.struct_desc) {
            uint64_t struct_offset = offset + iter.offset;
   v3d_print_group(clif, iter.struct_desc,
   }
