   /*
   * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <fcntl.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "disasm.h"
   #include "io.h"
   #include "redump.h"
      #define ASCII_XOR 0xff
   #include "util.h"
      struct pgm_header {
      uint32_t size;
   uint32_t unknown1;
   uint32_t unknown2;
   uint32_t revision;
   uint32_t unknown4;
   uint32_t unknown5;
   uint32_t unknown6;
   uint32_t unknown7;
   uint32_t unknown8;
   uint32_t num_attribs;
   uint32_t num_uniforms;
   uint32_t num_samplers;
   uint32_t num_varyings;
      };
      struct vs_header {
      uint32_t unknown1; /* seems to be # of sections up to and including shader */
   uint32_t unknown2; /* seems to be low byte or so of SQ_PROGRAM_CNTL */
   uint32_t unknown3;
   uint32_t unknown4;
   uint32_t unknown5;
   uint32_t unknown6;
   uint32_t unknown7;
   uint32_t unknown8;
      };
      struct fs_header {
         };
   /*
         // Covers a lot of type_info
   // varying, attribute, uniform, sampler
   type_info & 0xFF
   if ((type_info >> 8) == 0x8b) // vector
            0x50 = vec2
   0x51 = vec3
   0x52 = vec4
   0x53 = ivec2
   0x54 = ivec3
   0x55 = ivec4
   0x56 = bool // Why is this in vector?
   0x57 = bvec2
   0x58 = bvec3
   0x59 = bvec4
   0x5a = mat2
   0x5b = mat3
   0x5c = mat4
   0x5a = mat2x2 // Same as mat2
   0x65 = mat2x3
   0x66 = mat2x4
   0x67 = mat3x2
   0x5b = mat3x3 // Same as mat3
   0x68 = mat3x4
   0x69 = mat4x2
   0x6a = mat4x3
   0x5c = mat4x4 // same as mat4
   0x5e = sampler2D
   0x5f = sampler3D
   0x60 = samplerCube // XXX: Doesn't work
   0x62 = sampler2DShadow
   0xc6 = uvec2
      else if ((type_info >> 8) == 0x8d) // GLES3 samplers
            0xC1 = sampler2DArray
   0xC4 = sampler2DArrayShadow
   0xC5 = samplerCubeShadow
   0xCA = isampler2D
   0xCB = isampler3D
   0xCC = isamplerCube
   0xD2 = usampler2D
   0xD3 = usampler3D
   0xD4 = usamplerCube
      else // 0x14 = single
               */
   struct attribute {
      uint32_t type_info;
   uint32_t reg; /* seems to be the register the fetch instruction loads to */
   uint32_t const_idx; /* the CONST() indx value for sampler */
   uint32_t unknown2;
   uint32_t unknown3;
   uint32_t unknown4;
   uint32_t unknown5;
      };
      struct uniform {
      uint32_t type_info;
   uint32_t unknown2;
   uint32_t unknown3;
   uint32_t unknown4;
   uint32_t const_base; /* const base register (for uniforms that take more than
         uint32_t unknown6;
   uint32_t const_reg; /* the const register holding the value */
   uint32_t unknown7;
   uint32_t unknown8;
   uint32_t unknown9;
   union {
      struct {
         } v1;
   struct {
      uint32_t unknown10;
   uint32_t unknown11;
   uint32_t unknown12;
            };
      struct uniformblockmember {
      uint32_t type_info;
   uint32_t is_array;
   uint32_t array_size; /* elements in the array */
   uint32_t unknown2;   /* Same as array_size */
   uint32_t
         uint32_t unknown4;
   uint32_t
         uint32_t unknown6;
   uint32_t unknown7;
   uint32_t unknown8;
   uint32_t unknown9; /* UBO block index? */
   uint32_t unknown10;
   uint32_t unknown11;
   uint32_t unknown12;
      };
      struct uniformblock {
      uint32_t type_info;
   uint32_t unknown1;
   uint32_t unknown2;
   uint32_t unknown3;
   uint32_t unknown4;
   uint32_t num_members;
   uint32_t num_members2;
   uint32_t unknown5;
   uint32_t unknown6;
   uint32_t unknown7;
      };
      struct sampler {
      uint32_t type_info;
   uint32_t is_array;
   uint32_t array_size; /* elements in the array */
   uint32_t unknown4;   /* same as array_size */
   uint32_t unknown5;
   uint32_t unknown6;
   uint32_t const_idx; /* the CONST() indx value for the sampler */
   uint32_t unknown7;
      };
      struct varying {
      uint32_t type_info;
   uint32_t unknown2;
   uint32_t unknown3;
   uint32_t reg; /* the register holding the value (on entry to the shader) */
      };
      struct output {
      uint32_t type_info;
   uint32_t unknown2;
   uint32_t unknown3;
   uint32_t unknown4;
   uint32_t unknown5;
   uint32_t unknown6;
   uint32_t unknown7;
   uint32_t unknown8;
      };
      struct constant {
      uint32_t unknown1;
   uint32_t unknown2;
   uint32_t unknown3;
   uint32_t const_idx;
      };
      struct state {
      char *buf;
   int sz;
   struct pgm_header *hdr;
   struct attribute *attribs[32]; /* don't really know the upper limit.. */
   struct uniform *uniforms[32];
   struct sampler *samplers[32];
   struct varying *varyings[32];
   struct {
      struct uniformblock *header;
   struct uniformblockmember **members; /* GL ES 3.0 spec mandates minimum
      } uniformblocks[24];                    /* Maximum a330 supports */
      };
      static const char *infile;
   static int full_dump = 1;
   static int dump_shaders = 0;
   static int gpu_id;
      static char *
   find_sect_end(char *buf, int sz)
   {
      uint8_t *ptr = (uint8_t *)buf;
            while (ptr < end) {
               d |= ptr[0] << 0;
   d |= ptr[1] << 8;
   d |= ptr[2] << 16;
            /* someone at QC likes baseball */
   if (d == 0xba5eba11)
               }
      }
      static void *
   next_sect(struct state *state, int *sect_size)
   {
      char *end = find_sect_end(state->buf, state->sz);
            if (!end)
                     /* copy the section to keep things nicely 32b aligned: */
   sect = malloc(ALIGN(*sect_size, 4));
            state->sz -= *sect_size + 4;
               }
      static int
   valid_type(uint32_t type_info)
   {
      switch ((type_info >> 8) & 0xff) {
   case 0x8b: /* vector */
   case 0x8d: /* GLES3 samplers */
   case 0x14: /* float */
         default:
            }
      #if 0
   static int valid_uniformblock(uint32_t type_info)
   {
      if (type_info == 0x128)
            }
   #endif
      static void
   dump_attribute(struct attribute *attrib)
   {
      printf("\tR%d, CONST(%d): %s\n", attrib->reg, attrib->const_idx,
      }
      static inline int
   is_uniform_v2(struct uniform *uniform)
   {
      /* TODO maybe this should be based on revision #? */
   if (uniform->v2.unknown10 == 0)
            }
      static void
   dump_uniform(struct uniform *uniform)
   {
      char *name = is_uniform_v2(uniform) ? uniform->v2.name : uniform->v1.name;
   if (uniform->const_reg == -1) {
         } else {
            }
      static void
   dump_sampler(struct sampler *sampler)
   {
         }
      static void
   dump_varying(struct varying *varying)
   {
         }
      static void
   dump_uniformblock(struct uniformblock *uniformblock)
   {
      printf("\tUniform Block: %s(%d)\n", uniformblock->name,
      }
      static void
   dump_uniformblockmember(struct uniformblockmember *member)
   {
         }
      static void
   dump_output(struct output *output)
   {
         }
      static void
   dump_constant(struct constant *constant)
   {
      printf("\tC%d: %f, %f, %f, %f\n", constant->const_idx, constant->val[0],
      }
      /* dump attr/uniform/sampler/varying/const summary: */
   static void
   dump_short_summary(struct state *state, int nconsts,
         {
               /* dump attr/uniform/sampler/varying/const summary: */
   for (i = 0; i < state->hdr->num_varyings; i++) {
         }
   for (i = 0; i < state->hdr->num_attribs; i++) {
         }
   for (i = 0; i < state->hdr->num_uniforms; i++) {
         }
   for (i = 0; i < state->hdr->num_samplers; i++) {
         }
   for (i = 0; i < nconsts - 1; i++) {
      if (constants[i]->unknown2 == 0) {
            }
      }
      static void
   dump_raw_shader(uint32_t *dwords, uint32_t sizedwords, int n, char *ext)
   {
      static char filename[256];
            if (!dump_shaders)
            sprintf(filename, "%.*s-%d.%s", (int)strlen(infile) - 3, infile, n, ext);
   fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   if (fd != -1) {
      write(fd, dwords, sizedwords * 4);
         }
      static void
   dump_shaders_a2xx(struct state *state)
   {
      int i, sect_size;
            /* dump vertex shaders: */
   for (i = 0; i < 3; i++) {
      struct vs_header *vs_hdr = next_sect(state, &sect_size);
   struct constant *constants[32];
                     if (full_dump) {
      printf("#######################################################\n");
   printf("######## VS%d HEADER: (size %d)\n", i, sect_size);
               for (j = 0; j < (int)vs_hdr->unknown1 - 1; j++) {
      constants[j] = next_sect(state, &sect_size);
   if (full_dump) {
      printf("######## VS%d CONST: (size=%d)\n", i, sect_size);
   dump_constant(constants[j]);
                  ptr = next_sect(state, &sect_size);
   printf("######## VS%d SHADER: (size=%d)\n", i, sect_size);
   if (full_dump) {
      dump_hex(ptr, sect_size);
      } else {
         }
   disasm_a2xx((uint32_t *)(ptr + 32), (sect_size - 32) / 4, level + 1,
         dump_raw_shader((uint32_t *)(ptr + 32), (sect_size - 32) / 4, i, "vo");
            for (j = 0; j < vs_hdr->unknown9; j++) {
      ptr = next_sect(state, &sect_size);
   if (full_dump) {
      printf("######## VS%d CONST?: (size=%d)\n", i, sect_size);
      }
               for (j = 0; j < vs_hdr->unknown1 - 1; j++) {
                              /* dump fragment shaders: */
   for (i = 0; i < 1; i++) {
      struct fs_header *fs_hdr = next_sect(state, &sect_size);
   struct constant *constants[32];
                     if (full_dump) {
      printf("#######################################################\n");
   printf("######## FS%d HEADER: (size %d)\n", i, sect_size);
               for (j = 0; j < fs_hdr->unknown1 - 1; j++) {
      constants[j] = next_sect(state, &sect_size);
   if (full_dump) {
      printf("######## FS%d CONST: (size=%d)\n", i, sect_size);
   dump_constant(constants[j]);
                  ptr = next_sect(state, &sect_size);
   printf("######## FS%d SHADER: (size=%d)\n", i, sect_size);
   if (full_dump) {
      dump_hex(ptr, sect_size);
      } else {
         }
   disasm_a2xx((uint32_t *)(ptr + 32), (sect_size - 32) / 4, level + 1,
         dump_raw_shader((uint32_t *)(ptr + 32), (sect_size - 32) / 4, i, "fo");
            for (j = 0; j < fs_hdr->unknown1 - 1; j++) {
                        }
      static void
   dump_shaders_a3xx(struct state *state)
   {
               /* dump vertex shaders: */
   for (i = 0; i < 2; i++) {
      int instrs_size, hdr_size, sect_size, nconsts = 0, level = 0, compact = 0;
   uint8_t *vs_hdr;
   struct constant *constants[32];
            vs_hdr = next_sect(state, &hdr_size);
            /* seems like there are two cases, either:
   *  1) 152 byte header,
   *  2) zero or more 32 byte compiler const sections
   *  3) followed by shader instructions
   * or, if there are no compiler consts, this can be
   * all smashed in one large section
   */
   int n;
   if (state->hdr->revision >= 0xb)
         else if (state->hdr->revision >= 7)
         else
         if (hdr_size > n) {
      instrs = &vs_hdr[n];
   instrs_size = hdr_size - n;
   hdr_size = n;
      } else {
                        if ((sect_size != 32) && (sect_size != 44)) {
      /* end of constants: */
   instrs = ptr;
   instrs_size = sect_size;
      }
   dump_hex_ascii(ptr, sect_size, 0);
                           if (full_dump) {
      printf("#######################################################\n");
   printf("######## VS%d HEADER: (size %d)\n", i, hdr_size);
   dump_hex((void *)vs_hdr, hdr_size);
   for (j = 0; j < nconsts; j++) {
      printf("######## VS%d CONST: (size=%d)\n", i,
         dump_constant(constants[j]);
                  printf("######## VS%d SHADER: (size=%d)\n", i, instrs_size);
   if (full_dump) {
      dump_hex(instrs, instrs_size);
      } else {
                  if (!compact) {
      if (state->hdr->revision >= 7) {
      instrs += ALIGN(instrs_size, 8) - instrs_size;
      }
   instrs += 32;
               disasm_a3xx((uint32_t *)instrs, instrs_size / 4, level + 1, stdout,
         dump_raw_shader((uint32_t *)instrs, instrs_size / 4, i, "vo3");
               /* dump fragment shaders: */
   for (i = 0; i < 1; i++) {
      int instrs_size, hdr_size, sect_size, nconsts = 0, level = 0, compact = 0;
   uint8_t *fs_hdr;
   struct constant *constants[32];
                     printf("hdr_size=%d\n", hdr_size);
   /* two cases, similar to vertex shader, but magic # is 200
   * (or 208 for newer?)..
   */
   int n;
   if (state->hdr->revision >= 0xb)
         else if (state->hdr->revision >= 8)
         else if (state->hdr->revision == 7)
         else
            if (hdr_size > n) {
      instrs = &fs_hdr[n];
   instrs_size = hdr_size - n;
   hdr_size = n;
      } else {
                        if ((sect_size != 32) && (sect_size != 44)) {
      /* end of constants: */
   instrs = ptr;
                     dump_hex_ascii(ptr, sect_size, 0);
                           if (full_dump) {
      printf("#######################################################\n");
   printf("######## FS%d HEADER: (size %d)\n", i, hdr_size);
   dump_hex((void *)fs_hdr, hdr_size);
   for (j = 0; j < nconsts; j++) {
      printf("######## FS%d CONST: (size=%d)\n", i,
         dump_constant(constants[j]);
                  printf("######## FS%d SHADER: (size=%d)\n", i, instrs_size);
   if (full_dump) {
      dump_hex(instrs, instrs_size);
      } else {
                  if (!compact) {
      if (state->hdr->revision >= 7) {
      instrs += 44;
      } else {
      instrs += 32;
         }
   disasm_a3xx((uint32_t *)instrs, instrs_size / 4, level + 1, stdout,
         dump_raw_shader((uint32_t *)instrs, instrs_size / 4, i, "fo3");
         }
      static void
   dump_program(struct state *state)
   {
      int i, sect_size;
                     printf("######## HEADER: (size %d)\n", sect_size);
   printf("\tsize:           %d\n", state->hdr->size);
   printf("\trevision:       %d\n", state->hdr->revision);
   printf("\tattributes:     %d\n", state->hdr->num_attribs);
   printf("\tuniforms:       %d\n", state->hdr->num_uniforms);
   printf("\tsamplers:       %d\n", state->hdr->num_samplers);
   printf("\tvaryings:       %d\n", state->hdr->num_varyings);
   printf("\tuniform blocks: %d\n", state->hdr->num_uniformblocks);
   if (full_dump)
                  /* there seems to be two 0xba5eba11's at the end of the header, possibly
   * with some other stuff between them:
   */
   ptr = next_sect(state, &sect_size);
   if (full_dump) {
                  for (i = 0; (i < state->hdr->num_attribs) && (state->sz > 0); i++) {
               /* hmm, for a3xx (or maybe just newer driver version), we have some
   * extra sections that don't seem useful, so skip these:
   */
   while (!valid_type(state->attribs[i]->type_info)) {
      dump_hex_ascii(state->attribs[i], sect_size, 0);
               clean_ascii(state->attribs[i]->name, sect_size - 28);
   if (full_dump) {
      printf("######## ATTRIBUTE: (size %d)\n", sect_size);
   dump_attribute(state->attribs[i]);
                  for (i = 0; (i < state->hdr->num_uniforms) && (state->sz > 0); i++) {
               /* hmm, for a3xx (or maybe just newer driver version), we have some
   * extra sections that don't seem useful, so skip these:
   */
   while (!valid_type(state->uniforms[i]->type_info)) {
      dump_hex_ascii(state->uniforms[i], sect_size, 0);
               if (is_uniform_v2(state->uniforms[i])) {
         } else {
                  if (full_dump) {
      printf("######## UNIFORM: (size %d)\n", sect_size);
   dump_uniform(state->uniforms[i]);
                  for (i = 0; (i < state->hdr->num_samplers) && (state->sz > 0); i++) {
               /* hmm, for a3xx (or maybe just newer driver version), we have some
   * extra sections that don't seem useful, so skip these:
   */
   while (!valid_type(state->samplers[i]->type_info)) {
      dump_hex_ascii(state->samplers[i], sect_size, 0);
               clean_ascii(state->samplers[i]->name, sect_size - 33);
   if (full_dump) {
      printf("######## SAMPLER: (size %d)\n", sect_size);
   dump_sampler(state->samplers[i]);
                  // These sections show up after all of the other sampler sections
   // Loops through them all since we don't deal with them
   if (state->hdr->revision >= 7) {
      for (i = 0; (i < state->hdr->num_samplers) && (state->sz > 0); i++) {
      ptr = next_sect(state, &sect_size);
                  for (i = 0; (i < state->hdr->num_varyings) && (state->sz > 0); i++) {
               /* hmm, for a3xx (or maybe just newer driver version), we have some
   * extra sections that don't seem useful, so skip these:
   */
   while (!valid_type(state->varyings[i]->type_info)) {
      dump_hex_ascii(state->varyings[i], sect_size, 0);
               clean_ascii(state->varyings[i]->name, sect_size - 16);
   if (full_dump) {
      printf("######## VARYING: (size %d)\n", sect_size);
   dump_varying(state->varyings[i]);
                  /* show up again for revision >= 14?? */
   if (state->hdr->revision >= 14) {
      for (i = 0; (i < state->hdr->num_varyings) && (state->sz > 0); i++) {
      ptr = next_sect(state, &sect_size);
                  /* not sure exactly which revision started this, but seems at least
   * rev7 and rev8 implicitly include a new section for gl_FragColor:
   */
   if (state->hdr->revision >= 7) {
      /* I guess only one? */
            clean_ascii(state->outputs[0]->name, sect_size - 32);
   if (full_dump) {
      printf("######## OUTPUT: (size %d)\n", sect_size);
   dump_output(state->outputs[0]);
                  for (i = 0; (i < state->hdr->num_uniformblocks) && (state->sz > 0); i++) {
               clean_ascii(state->uniformblocks[i].header->name, sect_size - 40);
   if (full_dump) {
      printf("######## UNIFORM BLOCK: (size %d)\n", sect_size);
   dump_uniformblock(state->uniformblocks[i].header);
               /*
   * OpenGL ES 3.0 spec mandates a minimum amount of 16K members supported
   * a330 supports a minimum of 65K
   */
   state->uniformblocks[i].members =
            int member = 0;
   for (member = 0; (member < state->uniformblocks[i].header->num_members) &&
                           clean_ascii(state->uniformblocks[i].members[member]->name,
         if (full_dump) {
      printf("######## UNIFORM BLOCK MEMBER: (size %d)\n", sect_size);
   dump_uniformblockmember(state->uniformblocks[i].members[member]);
   dump_hex((char *)state->uniformblocks[i].members[member],
         }
   /*
   * Qualcomm saves the UBO members twice for each UBO
   * Don't ask me why
   */
   for (member = 0; (member < state->uniformblocks[i].header->num_members) &&
                           clean_ascii(state->uniformblocks[i].members[member]->name,
         if (full_dump) {
      printf("######## UNIFORM BLOCK MEMBER2: (size %d)\n", sect_size);
   dump_uniformblockmember(state->uniformblocks[i].members[member]);
   dump_hex((char *)state->uniformblocks[i].members[member],
                     if (gpu_id >= 300) {
         } else {
                  if (!full_dump)
            /* dump ascii version of shader program: */
   ptr = next_sect(state, &sect_size);
   printf("\n#######################################################\n");
   printf("######## SHADER SRC: (size=%d)\n", sect_size);
   dump_ascii(ptr, sect_size);
            /* dump remaining sections (there shouldn't be any): */
   while (state->sz > 0) {
      ptr = next_sect(state, &sect_size);
   printf("######## section (size=%d)\n", sect_size);
   printf("as hex:\n");
   dump_hex(ptr, sect_size);
   printf("as float:\n");
   dump_float(ptr, sect_size);
   printf("as ascii:\n");
   dump_ascii(ptr, sect_size);
      }
   /* cleanup the uniform buffer members we allocated */
   if (state->hdr->num_uniformblocks > 0)
      }
      int
   main(int argc, char **argv)
   {
      enum rd_sect_type type = RD_NONE;
   enum debug_t debug = PRINT_RAW | PRINT_STATS;
   void *buf = NULL;
   int sz;
   struct io *io;
                     while (1) {
      if ((argc > 1) && !strcmp(argv[1], "--verbose")) {
      debug |= PRINT_RAW | PRINT_VERBOSE;
   argv++;
   argc--;
      }
   if ((argc > 1) && !strcmp(argv[1], "--expand")) {
      debug |= EXPAND_REPEAT;
   argv++;
   argc--;
      }
   if ((argc > 1) && !strcmp(argv[1], "--short")) {
      /* only short dump, original shader, symbol table, and disassembly */
   full_dump = 0;
   argv++;
   argc--;
      }
   if ((argc > 1) && !strcmp(argv[1], "--dump-shaders")) {
      dump_shaders = 1;
   argv++;
   argc--;
      }
   if ((argc > 1) && !strcmp(argv[1], "--raw")) {
      raw_program = 1;
   argv++;
   argc--;
      }
   if ((argc > 1) && !strcmp(argv[1], "--gpu300")) {
      gpu_id = 320;
   argv++;
   argc--;
      }
               if (argc != 2) {
      fprintf(
      stderr,
                  disasm_a2xx_set_debug(debug);
                     io = io_open(infile);
   if (!io) {
      fprintf(stderr, "could not open: %s\n", infile);
               if (raw_program) {
      io_readn(io, &sz, 4);
            /* note: allow hex dumps to go a bit past the end of the buffer..
   * might see some garbage, but better than missing the last few bytes..
   */
   buf = calloc(1, sz + 3);
   io_readn(io, buf + 4, sz);
            struct state state = {
      .buf = buf,
      };
   printf("############################################################\n");
   printf("program:\n");
   dump_program(&state);
   printf("############################################################\n");
               /* figure out what sort of input we are dealing with: */
   if (!(check_extension(infile, ".rd") || check_extension(infile, ".rd.gz"))) {
      gl_shader_stage shader = ~0;
   int ret;
   if (check_extension(infile, ".vo")) {
         } else if (check_extension(infile, ".fo")) {
         } else if (check_extension(infile, ".vo3")) {
   } else if (check_extension(infile, ".fo3")) {
   } else if (check_extension(infile, ".co3")) {
   } else {
      fprintf(stderr, "invalid input file: %s\n", infile);
      }
   buf = calloc(1, 100 * 1024);
   ret = io_readn(io, buf, 100 * 1024);
   if (ret < 0) {
      fprintf(stderr, "error: %m");
      }
   if (shader != ~0) {
         } else {
      /* disassembly does not depend on shader stage on a3xx+: */
                  while ((io_readn(io, &type, sizeof(type)) > 0) &&
                     /* note: allow hex dumps to go a bit past the end of the buffer..
   * might see some garbage, but better than missing the last few bytes..
   */
   buf = calloc(1, sz + 3);
            switch (type) {
   case RD_TEST:
      if (full_dump)
            case RD_VERT_SHADER:
      printf("vertex shader:\n%s\n", (char *)buf);
      case RD_FRAG_SHADER:
      printf("fragment shader:\n%s\n", (char *)buf);
      case RD_PROGRAM: {
      struct state state = {
      .buf = buf,
      };
   printf(
         printf("program:\n");
   dump_program(&state);
   printf(
            }
   case RD_GPU_ID:
      gpu_id = *((unsigned int *)buf);
   printf("gpu_id: %d\n", gpu_id);
      default:
                                 }
