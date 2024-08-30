   /*
   * Copyright © 2016 Intel Corporation
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
      #include <stdio.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdarg.h>
   #include <string.h>
   #include <expat.h>
   #include <inttypes.h>
   #include <zlib.h>
      #include <util/list.h>
   #include <util/macros.h>
   #include <util/os_file.h>
   #include <util/ralloc.h>
   #include <util/u_math.h>
      #include "intel_decoder.h"
      #include "isl/isl.h"
   #include "genxml/genX_xml.h"
      #define XML_BUFFER_SIZE 4096
   #define MAX_VALUE_ITEMS 128
      struct location {
      const char *filename;
      };
      struct genxml_import_exclusion {
      struct list_head link;
      };
      struct genxml_import {
      struct list_head link;
   struct list_head exclusions;
      };
      struct parser_context {
      XML_Parser parser;
   int foo;
            struct intel_group *group;
   struct intel_enum *enoom;
   const char *dirname;
            int n_values, n_allocated_values;
                        };
      const char *
   intel_group_get_name(const struct intel_group *group)
   {
         }
      uint32_t
   intel_group_get_opcode(const struct intel_group *group)
   {
         }
      struct intel_group *
   intel_spec_find_struct(struct intel_spec *spec, const char *name)
   {
      struct hash_entry *entry = _mesa_hash_table_search(spec->structs,
            }
      struct intel_group *
   intel_spec_find_register(struct intel_spec *spec, uint32_t offset)
   {
      struct hash_entry *entry =
      _mesa_hash_table_search(spec->registers_by_offset,
         }
      struct intel_group *
   intel_spec_find_register_by_name(struct intel_spec *spec, const char *name)
   {
      struct hash_entry *entry =
            }
      struct intel_enum *
   intel_spec_find_enum(struct intel_spec *spec, const char *name)
   {
      struct hash_entry *entry = _mesa_hash_table_search(spec->enums,
            }
      uint32_t
   intel_spec_get_gen(struct intel_spec *spec)
   {
         }
      static void __attribute__((noreturn))
   fail(struct location *loc, const char *msg, ...)
   {
               va_start(ap, msg);
   fprintf(stderr, "%s:%d: error: ",
         vfprintf(stderr, msg, ap);
   fprintf(stderr, "\n");
   va_end(ap);
      }
      static void
   get_array_offset_count(const char **atts, uint32_t *offset, uint32_t *count,
         {
      for (int i = 0; atts[i]; i += 2) {
               if (strcmp(atts[i], "count") == 0) {
      *count = strtoul(atts[i + 1], &p, 0);
   if (*count == 0)
      } else if (strcmp(atts[i], "start") == 0) {
         } else if (strcmp(atts[i], "size") == 0) {
            }
      }
      static struct intel_group *
   create_group(struct parser_context *ctx,
               const char *name,
   const char **atts,
   {
               group = rzalloc(ctx->spec, struct intel_group);
   if (name)
            group->spec = ctx->spec;
   group->variable = false;
   group->fixed_length = fixed_length;
   group->dword_length_field = NULL;
   group->dw_length = 0;
   group->engine_mask = INTEL_ENGINE_CLASS_TO_MASK(INTEL_ENGINE_CLASS_RENDER) |
                              for (int i = 0; atts[i]; i += 2) {
      char *p;
   if (strcmp(atts[i], "length") == 0) {
         } else if (strcmp(atts[i], "bias") == 0) {
         } else if (strcmp(atts[i], "engine") == 0) {
      void *mem_ctx = ralloc_context(NULL);
   char *tmp = ralloc_strdup(mem_ctx, atts[i + 1]);
                  group->engine_mask = 0;
   while (tok != NULL) {
      if (strcmp(tok, "render") == 0) {
         } else if (strcmp(tok, "compute") == 0) {
         } else if (strcmp(tok, "video") == 0) {
         } else if (strcmp(tok, "blitter") == 0) {
         } else {
                                             if (parent) {
      group->parent = parent;
   get_array_offset_count(atts,
                                    }
      static struct intel_enum *
   create_enum(struct parser_context *ctx, const char *name, const char **atts)
   {
               e = rzalloc(ctx->spec, struct intel_enum);
   if (name)
               }
      static void
   get_register_offset(const char **atts, uint32_t *offset)
   {
      for (int i = 0; atts[i]; i += 2) {
               if (strcmp(atts[i], "num") == 0)
      }
      }
      static void
   get_start_end_pos(int *start, int *end)
   {
      /* start value has to be mod with 32 as we need the relative
   * start position in the first DWord. For the end position, add
   * the length of the field to the start position to get the
   * relative position in the 64 bit address.
   */
   if (*end - *start > 32) {
      int len = *end - *start;
   *start = *start % 32;
      } else {
      *start = *start % 32;
                  }
      static inline uint64_t
   mask(int start, int end)
   {
                           }
      static inline uint64_t
   field_value(uint64_t value, int start, int end)
   {
      get_start_end_pos(&start, &end);
      }
      static struct intel_type
   string_to_type(struct parser_context *ctx, const char *s)
   {
      int i, f;
   struct intel_group *g;
            if (strcmp(s, "int") == 0)
         else if (strcmp(s, "uint") == 0)
         else if (strcmp(s, "bool") == 0)
         else if (strcmp(s, "float") == 0)
         else if (strcmp(s, "address") == 0)
         else if (strcmp(s, "offset") == 0)
         else if (sscanf(s, "u%d.%d", &i, &f) == 2)
         else if (sscanf(s, "s%d.%d", &i, &f) == 2)
         else if (g = intel_spec_find_struct(ctx->spec, s), g != NULL)
         else if (e = intel_spec_find_enum(ctx->spec, s), e != NULL)
         else if (strcmp(s, "mbo") == 0)
         else if (strcmp(s, "mbz") == 0)
         else
      }
      static struct intel_field *
   create_field(struct parser_context *ctx, const char **atts)
   {
               field = rzalloc(ctx->group, struct intel_field);
            for (int i = 0; atts[i]; i += 2) {
               if (strcmp(atts[i], "name") == 0) {
      field->name = ralloc_strdup(field, atts[i + 1]);
   if (strcmp(field->name, "DWord Length") == 0) {
            } else if (strcmp(atts[i], "start") == 0) {
         } else if (strcmp(atts[i], "end") == 0) {
         } else if (strcmp(atts[i], "type") == 0) {
         } else if (strcmp(atts[i], "default") == 0 &&
            field->has_default = true;
                     }
      static struct intel_field *
   create_array_field(struct parser_context *ctx, struct intel_group *array)
   {
               field = rzalloc(ctx->group, struct intel_field);
            field->array = array;
               }
      static struct intel_value *
   create_value(struct parser_context *ctx, const char **atts)
   {
               for (int i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], "name") == 0)
         else if (strcmp(atts[i], "value") == 0)
                  }
      static struct intel_field *
   create_and_append_field(struct parser_context *ctx,
               {
      struct intel_field *field = array ?
                  while (list && field->start > list->start) {
      prev = list;
               field->next = list;
   if (prev == NULL)
         else
               }
      static bool
   start_genxml_import(struct parser_context *ctx, const char **atts)
   {
      assert(ctx->import.name == NULL);
   assert(list_is_empty(&ctx->import.exclusions));
            for (int i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], "name") == 0) {
                     if (ctx->import.name == NULL)
               }
      static struct genxml_import_exclusion *
   add_genxml_import_exclusion(struct parser_context *ctx, const char **atts)
   {
               if (ctx->import.name == NULL) {
      fail(&ctx->loc, "exclude found without a named import");
                        for (int i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], "name") == 0) {
                     if (exclusion->name != NULL) {
         } else {
      ralloc_free(exclusion);
                  }
      static void
   move_group_to_spec(struct intel_spec *new_spec, struct intel_spec *old_spec,
            static void
   move_field_to_spec(struct intel_spec *new_spec, struct intel_spec *old_spec,
         {
      while (field != NULL) {
      if (field->array != NULL && field->array->spec == old_spec)
         if (field->type.kind == INTEL_TYPE_STRUCT &&
      field->type.intel_struct->spec == old_spec)
      if (field->type.kind == INTEL_TYPE_ENUM)
               }
      static void
   move_group_to_spec(struct intel_spec *new_spec, struct intel_spec *old_spec,
         {
      struct intel_group *g = group;
   while (g != NULL) {
      if (g->spec == old_spec) {
      if (ralloc_parent(g) == old_spec)
            }
      }
   move_field_to_spec(new_spec, old_spec, group->fields);
      }
      static bool
   finish_genxml_import(struct parser_context *ctx)
   {
      struct intel_spec *spec = ctx->spec;
            if (import->name == NULL) {
      fail(&ctx->loc, "import without name");
               struct intel_spec *imported_spec =
         if (import->name == NULL) {
      fail(&ctx->loc, "failed to load %s for importing", import->name);
                        list_for_each_entry(struct genxml_import_exclusion, exclusion,
            struct hash_entry *entry;
   entry = _mesa_hash_table_search(imported_spec->commands,
         if (entry != NULL) {
         }
   entry = _mesa_hash_table_search(imported_spec->structs,
         if (entry != NULL) {
         }
   entry = _mesa_hash_table_search(imported_spec->registers_by_name,
         if (entry != NULL) {
      struct intel_group *group = entry->data;
   _mesa_hash_table_remove(imported_spec->registers_by_name, entry);
   entry = _mesa_hash_table_search(imported_spec->registers_by_offset,
         if (entry != NULL)
      }
   entry = _mesa_hash_table_search(imported_spec->enums,
         if (entry != NULL) {
                     hash_table_foreach(imported_spec->commands, entry) {
      struct intel_group *group = entry->data;
   move_group_to_spec(spec, imported_spec, group);
      }
   hash_table_foreach(imported_spec->structs, entry) {
      struct intel_group *group = entry->data;
   move_group_to_spec(spec, imported_spec, group);
      }
   hash_table_foreach(imported_spec->registers_by_name, entry) {
      struct intel_group *group = entry->data;
   move_group_to_spec(spec, imported_spec, group);
   _mesa_hash_table_insert(spec->registers_by_name, group->name, group);
   _mesa_hash_table_insert(spec->registers_by_offset,
            }
   hash_table_foreach(imported_spec->enums, entry) {
      struct intel_enum *enoom = entry->data;
   ralloc_steal(spec, enoom);
               intel_spec_destroy(imported_spec);
   ralloc_free(ctx->import.name); /* also frees exclusions */
   ctx->import.name = NULL;
               }
      static void
   start_element(void *data, const char *element_name, const char **atts)
   {
      struct parser_context *ctx = data;
   const char *name = NULL;
                     for (int i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], "name") == 0)
         else if (strcmp(atts[i], "gen") == 0)
               if (strcmp(element_name, "genxml") == 0) {
      if (name == NULL)
         if (gen == NULL)
            int major, minor;
   int n = sscanf(gen, "%d.%d", &major, &minor);
   if (n == 0)
         if (n == 1)
               } else if (strcmp(element_name, "instruction") == 0) {
         } else if (strcmp(element_name, "struct") == 0) {
         } else if (strcmp(element_name, "register") == 0) {
      ctx->group = create_group(ctx, name, atts, NULL, true);
      } else if (strcmp(element_name, "group") == 0) {
      struct intel_group *group = create_group(ctx, "", atts, ctx->group, false);
   ctx->last_field = create_and_append_field(ctx, NULL, group);
      } else if (strcmp(element_name, "field") == 0) {
         } else if (strcmp(element_name, "enum") == 0) {
         } else if (strcmp(element_name, "value") == 0) {
      if (ctx->n_values >= ctx->n_allocated_values) {
      ctx->n_allocated_values = MAX2(2, ctx->n_allocated_values * 2);
   ctx->values = reralloc_array_size(ctx->spec, ctx->values,
            }
   assert(ctx->n_values < ctx->n_allocated_values);
      } else if (strcmp(element_name, "import") == 0) {
         } else if (strcmp(element_name, "exclude") == 0) {
               }
      static void
   end_element(void *data, const char *name)
   {
      struct parser_context *ctx = data;
            if (strcmp(name, "instruction") == 0 ||
      strcmp(name, "struct") == 0 ||
   strcmp(name, "register") == 0) {
   struct intel_group *group = ctx->group;
                     if (strcmp(name, "instruction") == 0) {
      while (list && list->end <= 31) {
      if (list->start >= 16 && list->has_default) {
      group->opcode_mask |=
            }
                  if (strcmp(name, "instruction") == 0)
         else if (strcmp(name, "struct") == 0)
         else if (strcmp(name, "register") == 0) {
      _mesa_hash_table_insert(spec->registers_by_name, group->name, group);
   _mesa_hash_table_insert(spec->registers_by_offset,
               } else if (strcmp(name, "group") == 0) {
         } else if (strcmp(name, "field") == 0) {
      struct intel_field *field = ctx->last_field;
   ctx->last_field = NULL;
   field->inline_enum.values = ctx->values;
   ralloc_steal(field, ctx->values);
   field->inline_enum.nvalues = ctx->n_values;
   ctx->values = ralloc_array(ctx->spec, struct intel_value*, ctx->n_allocated_values = 2);
      } else if (strcmp(name, "enum") == 0) {
      struct intel_enum *e = ctx->enoom;
   e->values = ctx->values;
   ralloc_steal(e, ctx->values);
   e->nvalues = ctx->n_values;
   ctx->values = ralloc_array(ctx->spec, struct intel_value*, ctx->n_allocated_values = 2);
   ctx->n_values = 0;
   ctx->enoom = NULL;
      } else if (strcmp(name, "import") == 0) {
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
      inflateEnd(&zstream);
               if (zstream.avail_out)
            out = realloc(out, 2*zstream.total_out);
   if (out == NULL) {
      inflateEnd(&zstream);
               zstream.next_out = (unsigned char *)out + zstream.total_out;
         end:
      inflateEnd(&zstream);
   *out_ptr = out;
      }
      static uint32_t _hash_uint32(const void *key)
   {
         }
      static struct intel_spec *
   intel_spec_init(void)
   {
      struct intel_spec *spec;
   spec = rzalloc(NULL, struct intel_spec);
   if (spec == NULL)
            spec->commands =
         spec->structs =
         spec->registers_by_name =
         spec->registers_by_offset =
         spec->enums =
         spec->access_cache =
               }
      static bool
   get_xml_data_dir(const char *dirname, const char *filename,
         {
      size_t fullname_len = strlen(dirname) + strlen(filename) + 2;
            if (fullname == NULL)
            ASSERTED size_t len = snprintf(fullname, fullname_len, "%s/%s",
                  *data = (void*)os_read_file(fullname, data_len);
   free(fullname);
      }
      static bool
   get_embedded_xml_data(int verx10, void **data, size_t *data_len)
   {
      uint8_t *text_data = NULL;
   uint32_t text_offset = 0, text_length = 0;
            for (int i = 0; i < ARRAY_SIZE(genxml_files_table); i++) {
      if (genxml_files_table[i].ver_10 == verx10) {
      text_offset = genxml_files_table[i].offset;
   text_length = genxml_files_table[i].length;
                  if (text_length == 0) {
      fprintf(stderr, "unable to find gen (%u) data\n", verx10);
               total_length = zlib_inflate(compress_genxmls,
                        *data = malloc(text_length);
   if (*data == NULL) {
      free(text_data);
               memcpy(*data, &text_data[text_offset], text_length);
   free(text_data);
   *data_len = text_length;
      }
      static bool
   get_embedded_xml_data_by_name(const char *filename,
         {
      int filename_len = strlen(filename);
   if (filename_len < 8 || filename_len > 10)
            if (strncmp(filename, "gen", 3) != 0 ||
      strcmp(filename + filename_len - 4, ".xml") != 0)
         char *numstr = strndup(filename + 3, filename_len - 7);
   char *endptr;
   long num = strtol(numstr, &endptr, 10);
   if (*endptr != '\0') {
      free(numstr);
      }
   /* convert ver numbers to verx10 */
   if (num < 45)
            free(numstr);
      }
      static bool
   get_xml_data(int verx10, const char *dirname, const char *filename,
         {
      if (dirname != NULL)
         else if (filename != NULL)
         else
      }
      static struct intel_spec *
   intel_spec_load_common(int verx10, const char *dirname, const char *filename)
   {
      struct parser_context ctx;
   void *xmlbuf, *data;
            if (!get_xml_data(verx10, dirname, filename, &data, &data_len))
            memset(&ctx, 0, sizeof ctx);
   ctx.dirname = dirname;
   list_inithead(&ctx.import.exclusions);
   ctx.parser = XML_ParserCreate(NULL);
   XML_SetUserData(ctx.parser, &ctx);
   if (ctx.parser == NULL) {
      free(data);
   fprintf(stderr, "failed to create parser\n");
               XML_SetElementHandler(ctx.parser, start_element, end_element);
            ctx.spec = intel_spec_init();
   if (ctx.spec == NULL) {
      free(data);
   fprintf(stderr, "Failed to create intel_spec\n");
               xmlbuf = XML_GetBuffer(ctx.parser, data_len);
   memcpy(xmlbuf, data, data_len);
   free(data);
            if (XML_ParseBuffer(ctx.parser, data_len, true) == 0) {
      fprintf(stderr,
         "Error parsing XML at line %ld col %ld byte %ld/%zu: %s\n",
   XML_GetCurrentLineNumber(ctx.parser),
   XML_GetCurrentColumnNumber(ctx.parser),
   XML_GetCurrentByteIndex(ctx.parser), data_len,
   XML_ParserFree(ctx.parser);
               XML_ParserFree(ctx.parser);
               }
      struct intel_spec *
   intel_spec_load(const struct intel_device_info *devinfo)
   {
         }
      struct intel_spec *
   intel_spec_load_filename(const char *dir, const char *name)
   {
         }
      struct intel_spec *
   intel_spec_load_from_path(const struct intel_device_info *devinfo,
         {
      char filename[20];
            ASSERTED size_t len = snprintf(filename, ARRAY_SIZE(filename), "gen%i.xml",
                     }
      void intel_spec_destroy(struct intel_spec *spec)
   {
         }
      struct intel_group *
   intel_spec_find_instruction(struct intel_spec *spec,
               {
      hash_table_foreach(spec->commands, entry) {
      struct intel_group *command = entry->data;
   uint32_t opcode = *p & command->opcode_mask;
   if ((command->engine_mask & INTEL_ENGINE_CLASS_TO_MASK(engine)) &&
      opcode == command->opcode)
               }
      struct intel_field *
   intel_group_find_field(struct intel_group *group, const char *name)
   {
      char path[256];
            struct intel_spec *spec = group->spec;
   struct hash_entry *entry = _mesa_hash_table_search(spec->access_cache,
         if (entry)
            struct intel_field *field = group->fields;
   while (field) {
      if (strcmp(field->name, name) == 0) {
      _mesa_hash_table_insert(spec->access_cache,
                  }
                  }
      int
   intel_group_get_length(const struct intel_group *group, const uint32_t *p)
   {
      if (group) {
      if (group->fixed_length)
         else {
      struct intel_field *field = group->dword_length_field;
   if (field) {
                        uint32_t h = p[0];
            switch (type) {
   case 0: /* MI */ {
      uint32_t opcode = field_value(h, 23, 28);
   if (opcode < 16)
         else
                     case 2: /* BLT */ {
                  case 3: /* Render */ {
      uint32_t subtype = field_value(h, 27, 28);
   uint32_t opcode = field_value(h, 24, 26);
   uint16_t whole_opcode = field_value(h, 16, 31);
   switch (subtype) {
   case 0:
      if (whole_opcode == 0x6104 /* PIPELINE_SELECT_965 */)
         else if (opcode < 2)
         else
      case 1:
      if (opcode < 2)
         else
      case 2: {
      if (opcode == 0)
         else if (opcode < 3)
         else
      }
   case 3:
      if (whole_opcode == 0x780b)
         else if (opcode < 4)
         else
         }
               }
      static const char *
   intel_get_enum_name(struct intel_enum *e, uint64_t value)
   {
      for (int i = 0; i < e->nvalues; i++) {
      if (e->values[i]->value == value) {
            }
      }
      static bool
   iter_more_fields(const struct intel_field_iterator *iter)
   {
         }
      static uint32_t
   iter_array_offset_bits(const struct intel_field_iterator *iter)
   {
      if (iter->level == 0)
            uint32_t offset = 0;
   const struct intel_group *group = iter->groups[1];
   for (int level = 1; level <= iter->level; level++, group = iter->groups[level]) {
      uint32_t array_idx = iter->array_iter[level];
                  }
      /* Checks whether we have more items in the array to iterate, or more arrays to
   * iterate through.
   */
   /* descend into a non-array field */
   static void
   iter_push_array(struct intel_field_iterator *iter)
   {
               iter->group = iter->field->array;
   iter->level++;
   assert(iter->level < DECODE_MAX_ARRAY_DEPTH);
   iter->groups[iter->level] = iter->group;
            assert(iter->group->fields != NULL); /* an empty <group> makes no sense */
   iter->field = iter->group->fields;
      }
      static void
   iter_pop_array(struct intel_field_iterator *iter)
   {
               iter->level--;
   iter->field = iter->fields[iter->level];
      }
      static void
   iter_start_field(struct intel_field_iterator *iter, struct intel_field *field)
   {
      iter->field = field;
            while (iter->field->array)
                     iter->start_bit = array_member_offset + iter->field->start;
   iter->end_bit = array_member_offset + iter->field->end;
      }
      static void
   iter_advance_array(struct intel_field_iterator *iter)
   {
      assert(iter->level > 0);
            if (iter->group->variable)
         else {
      if ((iter->array_iter[lvl] + 1) < iter->group->array_count) {
                        }
      static bool
   iter_more_array_elems(const struct intel_field_iterator *iter)
   {
      int lvl = iter->level;
            if (iter->group->variable) {
      int length = intel_group_get_length(iter->group, iter->p);
   assert(length >= 0 && "error the length is unknown!");
   return iter_array_offset_bits(iter) + iter->group->array_item_size <
      } else {
            }
      static bool
   iter_advance_field(struct intel_field_iterator *iter)
   {
      /* Keep looping while we either have more fields to look at, or we are
   * inside a <group> and can go up a level.
   */
   while (iter_more_fields(iter) || iter->level > 0) {
      if (iter_more_fields(iter)) {
      iter_start_field(iter, iter->field->next);
                        if (iter_more_array_elems(iter)) {
      iter_advance_array(iter);
               /* At this point, we reached the end of the <group> and were on the last
   * iteration. So it's time to go back to the parent and then advance the
   * field.
   */
                  }
      static bool
   iter_decode_field_raw(struct intel_field_iterator *iter, uint64_t *qw)
   {
               int field_start = iter->p_bit + iter->start_bit;
            const uint32_t *p = iter->p + (iter->start_bit / 32);
   if (iter->p_end && p >= iter->p_end)
            if ((field_end - field_start) > 32) {
      if (!iter->p_end || (p + 1) < iter->p_end)
            } else
                     /* Address & offset types have to be aligned to dwords, their start bit is
   * a reminder of the alignment requirement.
   */
   if (iter->field->type.kind == INTEL_TYPE_ADDRESS ||
      iter->field->type.kind == INTEL_TYPE_OFFSET)
            }
      static bool
   iter_decode_field(struct intel_field_iterator *iter)
   {
      union {
      uint64_t qw;
               if (iter->field->name)
         else
                     if (!iter_decode_field_raw(iter, &iter->raw_value))
                     v.qw = iter->raw_value;
   switch (iter->field->type.kind) {
   case INTEL_TYPE_UNKNOWN:
   case INTEL_TYPE_INT: {
      snprintf(iter->value, sizeof(iter->value), "%"PRId64, v.qw);
   enum_name = intel_get_enum_name(&iter->field->inline_enum, v.qw);
      }
   case INTEL_TYPE_MBZ:
   case INTEL_TYPE_UINT: {
      snprintf(iter->value, sizeof(iter->value), "%"PRIu64, v.qw);
   enum_name = intel_get_enum_name(&iter->field->inline_enum, v.qw);
      }
   case INTEL_TYPE_BOOL: {
      const char *true_string =
         snprintf(iter->value, sizeof(iter->value), "%s",
            }
   case INTEL_TYPE_FLOAT:
      snprintf(iter->value, sizeof(iter->value), "%f", v.f);
      case INTEL_TYPE_ADDRESS:
   case INTEL_TYPE_OFFSET:
      snprintf(iter->value, sizeof(iter->value), "0x%08"PRIx64, v.qw);
      case INTEL_TYPE_STRUCT:
      snprintf(iter->value, sizeof(iter->value), "<struct %s>",
         iter->struct_desc =
      intel_spec_find_struct(iter->group->spec,
         case INTEL_TYPE_UFIXED:
      snprintf(iter->value, sizeof(iter->value), "%f",
            case INTEL_TYPE_SFIXED: {
      /* Sign extend before converting */
   int bits = iter->field->type.i + iter->field->type.f + 1;
   int64_t v_sign_extend = util_mask_sign_extend(v.qw, bits);
   snprintf(iter->value, sizeof(iter->value), "%f",
            }
   case INTEL_TYPE_MBO:
         case INTEL_TYPE_ENUM: {
      snprintf(iter->value, sizeof(iter->value), "%"PRId64, v.qw);
   enum_name = intel_get_enum_name(iter->field->type.intel_enum, v.qw);
      }
            if (strlen(iter->group->name) == 0) {
      int length = strlen(iter->name);
            int level = 1;
   char *buf = iter->name + length;
   while (level <= iter->level) {
      int printed = snprintf(buf, sizeof(iter->name) - length,
         level++;
   length += printed;
                  if (enum_name) {
      int length = strlen(iter->value);
   snprintf(iter->value + length, sizeof(iter->value) - length,
      } else if (strcmp(iter->name, "Surface Format") == 0 ||
            if (isl_format_is_valid((enum isl_format)v.qw)) {
      const char *fmt_name = isl_format_get_name((enum isl_format)v.qw);
   int length = strlen(iter->value);
   snprintf(iter->value + length, sizeof(iter->value) - length,
                     }
      void
   intel_field_iterator_init(struct intel_field_iterator *iter,
                     {
               iter->groups[iter->level] = group;
   iter->group = group;
   iter->p = p;
            int length = intel_group_get_length(iter->group, iter->p);
   assert(length >= 0 && "error the length is unknown!");
   iter->p_end = length >= 0 ? &p[length] : NULL;
      }
      bool
   intel_field_iterator_next(struct intel_field_iterator *iter)
   {
      /* Initial condition */
   if (!iter->field) {
      if (iter->group->fields)
            bool result = iter_decode_field(iter);
   if (!result && iter->p_end) {
      /* We're dealing with a non empty struct of length=0 (BLEND_STATE on
   * Gen 7.5)
   */
                           if (!iter_advance_field(iter))
            if (!iter_decode_field(iter))
               }
      static void
   print_dword_header(FILE *outfile,
               {
      fprintf(outfile, "0x%08"PRIx64":  0x%08x : Dword %d\n",
      }
      bool
   intel_field_is_header(struct intel_field *field)
   {
               /* Instructions are identified by the first DWord. */
   if (field->start >= 32 ||
      field->end >= 32)
         bits = (1ULL << (field->end - field->start + 1)) - 1;
               }
      void
   intel_print_group(FILE *outfile, struct intel_group *group, uint64_t offset,
         {
      struct intel_field_iterator iter;
            intel_field_iterator_init(&iter, group, p, p_bit, color);
   while (intel_field_iterator_next(&iter)) {
      int iter_dword = iter.end_bit / 32;
   if (last_dword != iter_dword) {
      for (int i = last_dword + 1; i <= iter_dword; i++)
            }
   if (!intel_field_is_header(iter.field)) {
      fprintf(outfile, "    %s: %s\n", iter.name, iter.value);
   if (iter.struct_desc) {
      int struct_dword = iter.start_bit / 32;
   uint64_t struct_offset = offset + 4 * struct_dword;
   intel_print_group(outfile, iter.struct_desc, struct_offset,
               }
