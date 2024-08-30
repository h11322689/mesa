   /*
   * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
   * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "draw/draw_context.h"
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_pack_color.h"
      #include "r300_context.h"
   #include "r300_fs.h"
   #include "r300_screen.h"
   #include "r300_shader_semantics.h"
   #include "r300_state_inlines.h"
   #include "r300_texture.h"
   #include "r300_vs.h"
      /* r300_state_derived: Various bits of state which are dependent upon
   * currently bound CSO data. */
      enum r300_rs_swizzle {
      SWIZ_XYZW = 0,
   SWIZ_X001,
   SWIZ_XY01,
      };
      enum r300_rs_col_write_type {
      WRITE_COLOR = 0,
      };
      static void r300_draw_emit_attrib(struct r300_context* r300,
               {
      struct r300_vertex_shader_code* vs = r300_vs(r300)->shader;
   struct tgsi_shader_info* info = &vs->info;
            output = draw_find_shader_output(r300->draw,
                  }
      static void r300_draw_emit_all_attribs(struct r300_context* r300)
   {
      struct r300_vertex_shader_code* vs = r300_vs(r300)->shader;
   struct r300_shader_semantics* vs_outputs = &vs->outputs;
            /* Position. */
   if (vs_outputs->pos != ATTR_UNUSED) {
         } else {
                  /* Point size. */
   if (vs_outputs->psize != ATTR_UNUSED) {
                  /* Colors. */
   for (i = 0; i < ATTR_COLOR_COUNT; i++) {
      if (vs_outputs->color[i] != ATTR_UNUSED) {
                     /* Back-face colors. */
   for (i = 0; i < ATTR_COLOR_COUNT; i++) {
      if (vs_outputs->bcolor[i] != ATTR_UNUSED) {
                     /* Texture coordinates. */
   /* Only 8 generic vertex attributes can be used. If there are more,
   * they won't be rasterized. */
   gen_count = 0;
   for (i = 0; i < ATTR_GENERIC_COUNT && gen_count < 8; i++) {
      if (vs_outputs->generic[i] != ATTR_UNUSED &&
         (!(r300->sprite_coord_enable & (1U << i)) || !r300->is_point)) {
   r300_draw_emit_attrib(r300, EMIT_4F, vs_outputs->generic[i]);
               /* Texcoords */
   for (i = 0; i < ATTR_TEXCOORD_COUNT && gen_count < 8; i++) {
      if (vs_outputs->texcoord[i] != ATTR_UNUSED &&
         (!(r300->sprite_coord_enable & (1U << i)) || !r300->is_point)) {
   r300_draw_emit_attrib(r300, EMIT_4F, vs_outputs->texcoord[i]);
               /* Fog coordinates. */
   if (gen_count < 8 && vs_outputs->fog != ATTR_UNUSED) {
      r300_draw_emit_attrib(r300, EMIT_4F, vs_outputs->fog);
               /* WPOS. */
   if (r300_fs(r300)->shader->inputs.wpos != ATTR_UNUSED && gen_count < 8) {
      DBG(r300, DBG_SWTCL, "draw_emit_attrib: WPOS, index: %i\n",
               }
      /* Update the PSC tables for SW TCL, using Draw. */
   static void r300_swtcl_vertex_psc(struct r300_context *r300)
   {
      struct r300_vertex_stream_state *vstream = r300->vertex_stream_state.state;
   struct vertex_info *vinfo = &r300->vertex_info;
   uint16_t type, swizzle;
   enum pipe_format format;
   unsigned i, attrib_count;
                     /* For each Draw attribute, route it to the fragment shader according
   * to the vs_output_tab. */
   attrib_count = vinfo->num_attribs;
   DBG(r300, DBG_SWTCL, "r300: attrib count: %d\n", attrib_count);
   for (i = 0; i < attrib_count; i++) {
      if (vs_output_tab[i] == -1) {
         assert(0);
                     DBG(r300, DBG_SWTCL,
                  /* Obtain the type of data in this attribute. */
   type = r300_translate_vertex_data_type(format);
   if (type == R300_INVALID_FORMAT) {
         fprintf(stderr, "r300: Bad vertex format %s.\n",
         assert(0);
                     /* Obtain the swizzle for this attribute. Note that the default
                  /* Add the attribute to the PSC table. */
   if (i & 1) {
         vstream->vap_prog_stream_cntl[i >> 1] |= type << 16;
   } else {
         vstream->vap_prog_stream_cntl[i >> 1] |= type;
               /* Set the last vector in the PSC. */
   if (i) {
         }
   vstream->vap_prog_stream_cntl[i >> 1] |=
            vstream->count = (i >> 1) + 1;
   r300_mark_atom_dirty(r300, &r300->vertex_stream_state);
      }
      static void r300_rs_col(struct r300_rs_block* rs, int id, int ptr,
         {
      rs->ip[id] |= R300_RS_COL_PTR(ptr);
   if (swiz == SWIZ_0001) {
         } else {
         }
      }
      static void r300_rs_col_write(struct r300_rs_block* rs, int id, int fp_offset,
         {
      assert(type == WRITE_COLOR);
   rs->inst[id] |= R300_RS_INST_COL_CN_WRITE |
      }
      static void r300_rs_tex(struct r300_rs_block* rs, int id, int ptr,
         {
      if (swiz == SWIZ_X001) {
      rs->ip[id] |= R300_RS_TEX_PTR(ptr) |
                  R300_RS_SEL_S(R300_RS_SEL_C0) |
   } else if (swiz == SWIZ_XY01) {
      rs->ip[id] |= R300_RS_TEX_PTR(ptr) |
                  R300_RS_SEL_S(R300_RS_SEL_C0) |
   } else {
      rs->ip[id] |= R300_RS_TEX_PTR(ptr) |
                  R300_RS_SEL_S(R300_RS_SEL_C0) |
   }
      }
      static void r300_rs_tex_write(struct r300_rs_block* rs, int id, int fp_offset)
   {
      rs->inst[id] |= R300_RS_INST_TEX_CN_WRITE |
      }
      static void r500_rs_col(struct r300_rs_block* rs, int id, int ptr,
         {
      rs->ip[id] |= R500_RS_COL_PTR(ptr);
   if (swiz == SWIZ_0001) {
         } else {
         }
      }
      static void r500_rs_col_write(struct r300_rs_block* rs, int id, int fp_offset,
         {
      if (type == WRITE_FACE)
      rs->inst[id] |= R500_RS_INST_COL_CN_WRITE_BACKFACE |
      else
      rs->inst[id] |= R500_RS_INST_COL_CN_WRITE |
      }
      static void r500_rs_tex(struct r300_rs_block* rs, int id, int ptr,
         {
      if (swiz == SWIZ_X001) {
      rs->ip[id] |= R500_RS_SEL_S(ptr) |
                  } else if (swiz == SWIZ_XY01) {
      rs->ip[id] |= R500_RS_SEL_S(ptr) |
                  } else {
      rs->ip[id] |= R500_RS_SEL_S(ptr) |
                  }
      }
      static void r500_rs_tex_write(struct r300_rs_block* rs, int id, int fp_offset)
   {
      rs->inst[id] |= R500_RS_INST_TEX_CN_WRITE |
      }
      /* Set up the RS block.
   *
   * This is the part of the chipset that is responsible for linking vertex
   * and fragment shaders and stuffed texture coordinates.
   *
   * The rasterizer reads data from VAP, which produces vertex shader outputs,
   * and GA, which produces stuffed texture coordinates. VAP outputs have
   * precedence over GA. All outputs must be rasterized otherwise it locks up.
   * If there are more outputs rasterized than is set in VAP/GA, it locks up
   * too. The funky part is that this info has been pretty much obtained by trial
   * and error. */
   static void r300_update_rs_block(struct r300_context *r300)
   {
      struct r300_vertex_shader_code *vs = r300_vs(r300)->shader;
   struct r300_shader_semantics *vs_outputs = &vs->outputs;
   struct r300_shader_semantics *fs_inputs = &r300_fs(r300)->shader->inputs;
   struct r300_rs_block rs = {0};
   int i, col_count = 0, tex_count = 0, fp_offset = 0, count, loc = 0, tex_ptr = 0;
   int gen_offset = 0;
   void (*rX00_rs_col)(struct r300_rs_block*, int, int, enum r300_rs_swizzle);
   void (*rX00_rs_col_write)(struct r300_rs_block*, int, int, enum r300_rs_col_write_type);
   void (*rX00_rs_tex)(struct r300_rs_block*, int, int, enum r300_rs_swizzle);
   void (*rX00_rs_tex_write)(struct r300_rs_block*, int, int);
   bool any_bcolor_used = vs_outputs->bcolor[0] != ATTR_UNUSED ||
         int *stream_loc_notcl = r300->stream_loc_notcl;
            if (r300->screen->caps.is_r500) {
      rX00_rs_col       = r500_rs_col;
   rX00_rs_col_write = r500_rs_col_write;
   rX00_rs_tex       = r500_rs_tex;
      } else {
      rX00_rs_col       = r300_rs_col;
   rX00_rs_col_write = r300_rs_col_write;
   rX00_rs_tex       = r300_rs_tex;
               /* 0x5555 copied from classic, which means:
   * Select user color 0 for COLOR0 up to COLOR7.
   * What the hell does that mean? */
            /* The position is always present in VAP. */
   rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_POS;
   rs.vap_out_vtx_fmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;
            /* Set up the point size in VAP. */
   if (vs_outputs->psize != ATTR_UNUSED) {
      rs.vap_out_vtx_fmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
               /* Set up and rasterize colors. */
   for (i = 0; i < ATTR_COLOR_COUNT; i++) {
      if (vs_outputs->color[i] != ATTR_UNUSED || any_bcolor_used ||
         vs_outputs->color[1] != ATTR_UNUSED) {
   /* Set up the color in VAP. */
   rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_COLOR;
   rs.vap_out_vtx_fmt[0] |=
                                 /* Write it to the FS input register if it's needed by the FS. */
   if (fs_inputs->color[i] != ATTR_UNUSED) {
                     DBG(r300, DBG_RS,
      } else {
         }
   } else {
         /* Skip the FS input register, leave it uninitialized. */
   /* If we try to set it to (0,0,0,1), it will lock up. */
                     DBG(r300, DBG_RS, "r300: FS input color %i unassigned.\n",
                  /* Set up back-face colors. The rasterizer will do the color selection
   * automatically. */
   if (any_bcolor_used) {
      if (r300->two_sided_color) {
         /* Rasterize as back-face colors. */
   for (i = 0; i < ATTR_COLOR_COUNT; i++) {
      rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_COLOR;
   rs.vap_out_vtx_fmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << (2+i);
      } else {
         /* Rasterize two fake texcoords to prevent from the two-sided color
   * selection. */
   /* XXX Consider recompiling the vertex shader to save 2 RS units. */
   for (i = 0; i < 2; i++) {
                           /* Rasterize it. */
   rX00_rs_tex(&rs, tex_count, tex_ptr, SWIZ_XYZW);
   tex_count++;
                  /* gl_FrontFacing.
   * Note that we can use either the two-sided color selection based on
   * the front and back vertex shader colors, or gl_FrontFacing,
   * but not both! It locks up otherwise.
   *
   * In Direct3D 9, the two-sided color selection can be used
   * with shaders 2.0 only, while gl_FrontFacing can be used
   * with shaders 3.0 only. The hardware apparently hasn't been designed
   * to support both at the same time. */
   if (r300->screen->caps.is_r500 && fs_inputs->face != ATTR_UNUSED &&
      !(any_bcolor_used && r300->two_sided_color)) {
   rX00_rs_col(&rs, col_count, col_count, SWIZ_XYZW);
   rX00_rs_col_write(&rs, col_count, fp_offset, WRITE_FACE);
   fp_offset++;
   col_count++;
      } else if (fs_inputs->face != ATTR_UNUSED) {
                  /* Re-use color varyings for generics if possible.
   *
   * The colors are interpolated as 20-bit floats (reduced precision),
   * Use this hack only if there are too many generic varyings.
   * (number of generics + texcoords + fog + wpos + pcoord > 8) */
   if (r300->screen->caps.is_r500 && !any_bcolor_used && !r300->flatshade &&
      fs_inputs->face == ATTR_UNUSED &&
   vs_outputs->num_texcoord + vs_outputs->num_generic +
   (vs_outputs->fog != ATTR_UNUSED) + (fs_inputs->wpos != ATTR_UNUSED) +
   (vs_outputs->pcoord != ATTR_UNUSED) > 8 &&
   for (i = 0; i < ATTR_GENERIC_COUNT && col_count < 2; i++) {
      /* Cannot use color varyings for sprite coords. */
      (r300->sprite_coord_enable & (1U << i)) && r300->is_point) {
   break;
                  /* Set up the color in VAP. */
   rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_COLOR;
   rs.vap_out_vtx_fmt[0] |=
         stream_loc_notcl[loc++] = 2 + col_count;
      /* Rasterize it. */
   rX00_rs_col(&rs, col_count, col_count, SWIZ_XYZW);
      /* Write it to the FS input register if it's needed by the FS. */
   if (fs_inputs->generic[i] != ATTR_UNUSED) {
         rX00_rs_col_write(&rs, col_count, fp_offset, WRITE_COLOR);
               "r300: Rasterized generic %i redirected to color %i and written to FS.\n",
      } else {
         DBG(r300, DBG_RS, "r300: Rasterized generic %i redirected to color %i unused.\n",
   }
   col_count++;
         /* Skip the FS input register, leave it uninitialized. */
   /* If we try to set it to (0,0,0,1), it will lock up. */
   if (fs_inputs->generic[i] != ATTR_UNUSED) {
                  }
         }
   gen_offset = i;
               /* Rasterize generics. */
      bool sprite_coord = false;
      if (fs_inputs->generic[i] != ATTR_UNUSED) {
         }
            if (vs_outputs->generic[i] != ATTR_UNUSED || sprite_coord) {
         if (!sprite_coord) {
      /* Set up the texture coordinates in VAP. */
   rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
   rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
      } else
                                       /* Write it to the FS input register if it's needed by the FS. */
   if (fs_inputs->generic[i] != ATTR_UNUSED) {
                     DBG(r300, DBG_RS,
      "r300: Rasterized generic %i written to FS%s in texcoord %d.\n",
   } else {
      DBG(r300, DBG_RS,
      "r300: Rasterized generic %i unused%s.\n",
   }
   tex_count++;
   } else {
         /* Skip the FS input register, leave it uninitialized. */
   /* If we try to set it to (0,0,0,1), it will lock up. */
                     DBG(r300, DBG_RS, "r300: FS input generic %i unassigned%s.\n",
                  for (; i < ATTR_GENERIC_COUNT; i++) {
      if (fs_inputs->generic[i] != ATTR_UNUSED) {
         fprintf(stderr, "r300: ERROR: FS input generic %i unassigned, "
                     gen_offset = 0;
   /* Re-use color varyings for texcoords if possible.
   *
   * The colors are interpolated as 20-bit floats (reduced precision),
   * Use this hack only if there are too many generic varyings.
   * (number of generics + texcoords + fog + wpos + pcoord > 8) */
   if (r300->screen->caps.is_r500 && !any_bcolor_used && !r300->flatshade &&
      fs_inputs->face == ATTR_UNUSED &&
   vs_outputs->num_texcoord + vs_outputs->num_generic +
   (vs_outputs->fog != ATTR_UNUSED) + (fs_inputs->wpos != ATTR_UNUSED) +
   (vs_outputs->pcoord != ATTR_UNUSED) > 8 &&
   for (i = 0; i < ATTR_TEXCOORD_COUNT && col_count < 2; i++) {
      /* Cannot use color varyings for sprite coords. */
      (r300->sprite_coord_enable & (1U << i)) && r300->is_point) {
   break;
                  /* Set up the color in VAP. */
   rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_COLOR;
   rs.vap_out_vtx_fmt[0] |=
         stream_loc_notcl[loc++] = 2 + col_count;
      /* Rasterize it. */
   rX00_rs_col(&rs, col_count, col_count, SWIZ_XYZW);
      /* Write it to the FS input register if it's needed by the FS. */
   if (fs_inputs->texcoord[i] != ATTR_UNUSED) {
         rX00_rs_col_write(&rs, col_count, fp_offset, WRITE_COLOR);
               "r300: Rasterized texcoord %i redirected to color %i and written to FS.\n",
      } else {
         DBG(r300, DBG_RS, "r300: Rasterized texcoord %i redirected to color %i unused.\n",
   }
   col_count++;
         /* Skip the FS input register, leave it uninitialized. */
   /* If we try to set it to (0,0,0,1), it will lock up. */
   if (fs_inputs->texcoord[i] != ATTR_UNUSED) {
                  }
         }
   gen_offset = i;
               /* Rasterize texcords. */
   for (i = gen_offset; i < ATTR_TEXCOORD_COUNT && tex_count < 8; i++) {
               if (fs_inputs->texcoord[i] != ATTR_UNUSED) {
                  if (vs_outputs->texcoord[i] != ATTR_UNUSED || sprite_coord) {
         if (!sprite_coord) {
      /* Set up the texture coordinates in VAP. */
   rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
   rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
      } else
                                       /* Write it to the FS input register if it's needed by the FS. */
   if (fs_inputs->texcoord[i] != ATTR_UNUSED) {
                     DBG(r300, DBG_RS,
      "r300: Rasterized texcoord %i written to FS%s in texcoord %d.\n",
   } else {
      DBG(r300, DBG_RS,
      "r300: Rasterized texcoord %i unused%s.\n",
   }
   tex_count++;
   } else {
         /* Skip the FS input register, leave it uninitialized. */
   /* If we try to set it to (0,0,0,1), it will lock up. */
                     DBG(r300, DBG_RS, "r300: FS input texcoord %i unassigned%s.\n",
                  for (; i < ATTR_TEXCOORD_COUNT; i++) {
      if (fs_inputs->texcoord[i] != ATTR_UNUSED) {
         fprintf(stderr, "r300: ERROR: FS input texcoord %i unassigned, "
                     /* Rasterize pointcoord. */
               stuffing_enable |=
            /* Rasterize it. */
            /* Write it to the FS input register if it's needed by the FS. */
   rX00_rs_tex_write(&rs, tex_count, fp_offset);
            DBG(r300, DBG_RS,
                  tex_count++;
               /* Rasterize fog coordinates. */
   if (vs_outputs->fog != ATTR_UNUSED && tex_count < 8) {
      /* Set up the fog coordinates in VAP. */
   rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
   rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
            /* Rasterize it. */
            /* Write it to the FS input register if it's needed by the FS. */
   if (fs_inputs->fog != ATTR_UNUSED) {
                        } else {
         }
   tex_count++;
      } else {
      /* Skip the FS input register, leave it uninitialized. */
   /* If we try to set it to (0,0,0,1), it will lock up. */
   if (fs_inputs->fog != ATTR_UNUSED) {
                  if (tex_count < 8) {
         } else {
      fprintf(stderr, "r300: ERROR: FS input fog unassigned, "
                        /* Rasterize WPOS. */
   /* Don't set it in VAP if the FS doesn't need it. */
   if (fs_inputs->wpos != ATTR_UNUSED && tex_count < 8) {
      /* Set up the WPOS coordinates in VAP. */
   rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
   rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
            /* Rasterize it. */
            /* Write it to the FS input register. */
                     fp_offset++;
   tex_count++;
      } else {
      if (fs_inputs->wpos != ATTR_UNUSED && tex_count >= 8) {
         fprintf(stderr, "r300: ERROR: FS input WPOS unassigned, "
                     /* Invalidate the rest of the no-TCL (GA) stream locations. */
   for (; loc < 16;) {
                  /* Rasterize at least one color, or bad things happen. */
   if (col_count == 0 && tex_count == 0) {
      rX00_rs_col(&rs, 0, 0, SWIZ_0001);
                        DBG(r300, DBG_RS, "r300: --- Rasterizer status ---: colors: %i, "
            rs.count = MIN2(tex_ptr, 32) | (col_count << R300_IC_COUNT_SHIFT) |
            count = MAX3(col_count, tex_count, 1);
            /* set the GB enable flags */
   if ((r300->sprite_coord_enable || fs_inputs->pcoord != ATTR_UNUSED) &&
      stuffing_enable |= R300_GB_POINT_STUFF_ENABLE;
                        /* Now, after all that, see if we actually need to update the state. */
   if (memcmp(r300->rs_block_state.state, &rs, sizeof(struct r300_rs_block))) {
      memcpy(r300->rs_block_state.state, &rs, sizeof(struct r300_rs_block));
         }
      static void rgba_to_bgra(float color[4])
   {
      float x = color[0];
   color[0] = color[2];
      }
      static uint32_t r300_get_border_color(enum pipe_format format,
               {
      const struct util_format_description *desc = util_format_description(format);
   float border_swizzled[4] = {0};
                     /* Do depth formats first. */
   if (util_format_is_depth_or_stencil(format)) {
      switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
         if (is_r500) {
         } else {
         default:
         assert(0);
               /* Apply inverse swizzle of the format. */
            /* Compressed formats. */
   if (util_format_is_compressed(format)) {
      switch (format) {
   case PIPE_FORMAT_RGTC1_SNORM:
   case PIPE_FORMAT_LATC1_SNORM:
         border_swizzled[0] = border_swizzled[0] < 0 ?
                  case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_LATC1_UNORM:
         /* Add 1/32 to round the border color instead of truncating. */
   /* The Y component is used for the border color. */
   border_swizzled[1] = border_swizzled[0] + 1.0f/32;
   util_pack_color(border_swizzled, PIPE_FORMAT_B4G4R4A4_UNORM, &uc);
   case PIPE_FORMAT_RGTC2_SNORM:
   case PIPE_FORMAT_LATC2_SNORM:
         util_pack_color(border_swizzled, PIPE_FORMAT_R8G8B8A8_SNORM, &uc);
   case PIPE_FORMAT_RGTC2_UNORM:
   case PIPE_FORMAT_LATC2_UNORM:
         util_pack_color(border_swizzled, PIPE_FORMAT_R8G8B8A8_UNORM, &uc);
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
         util_pack_color(border_swizzled, PIPE_FORMAT_B8G8R8A8_SRGB, &uc);
   default:
         util_pack_color(border_swizzled, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
               switch (desc->channel[0].size) {
      case 2:
         rgba_to_bgra(border_swizzled);
            case 4:
         rgba_to_bgra(border_swizzled);
            case 5:
         rgba_to_bgra(border_swizzled);
   if (desc->channel[1].size == 5) {
         } else if (desc->channel[1].size == 6) {
         } else {
                  default:
   case 8:
         if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         } else if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      if (desc->nr_channels == 2) {
      border_swizzled[3] = border_swizzled[1];
      } else {
            } else {
                  case 10:
                  case 16:
         if (desc->nr_channels <= 2) {
      if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) {
         } else if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         } else {
            } else {
      if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         } else {
                     case 32:
         if (desc->nr_channels == 1) {
         } else {
                        }
      static void r300_merge_textures_and_samplers(struct r300_context* r300)
   {
      struct r300_textures_state *state =
         struct r300_texture_sampler_state *texstate;
   struct r300_sampler_state *sampler;
   struct r300_sampler_view *view;
   struct r300_resource *tex;
   unsigned base_level, min_level, level_count, i, j, size;
   unsigned count = MIN2(state->sampler_view_count,
                  /* The KIL opcode fix, see below. */
   if (!count && !r300->screen->caps.is_r500)
            state->tx_enable = 0;
   state->count = 0;
            for (i = 0; i < count; i++) {
      if (state->sampler_views[i] && state->sampler_states[i]) {
                  view = state->sampler_views[i];
                  texstate = &state->regs[i];
   texstate->format = view->format;
                  /* Set the border color. */
   texstate->border_color =
                        /* determine min/max levels */
   base_level = view->base.u.tex.first_level;
   min_level = sampler->min_lod;
   level_count = MIN3(sampler->max_lod,
                                    if (tex->tex.is_npot) {
      /* Even though we do not implement mipmapping for NPOT
      * textures, we should at least honor the minimum level
   * which is allowed to be displayed. We do this by setting up
                        r300_texture_setup_format_state(r300->screen, tex,
                                                                  /* Depth textures are kinda special. */
                     if (!r300->screen->caps.is_r500 &&
      util_format_get_blocksizebits(view->base.format) == 32) {
   /* X24x8 is sampled as Y16X16 on r3xx-r4xx.
         for (j = 0; j < 4; j++)
      } else {
                        /* If compare mode is disabled, sampler view swizzles
   * are stored in the format.
   * Otherwise, the swizzles must be applied after the compare
   * mode in the fragment shader. */
   if (sampler->state.compare_mode == PIPE_TEX_COMPARE_NONE) {
      texstate->format.format1 |=
            } else {
      texstate->format.format1 |=
                  if (r300->screen->caps.dxtc_swizzle &&
                        /* to emulate 1D textures through 2D ones correctly */
   if (tex->b.target == PIPE_TEXTURE_1D) {
                        /* The hardware doesn't like CLAMP and CLAMP_TO_BORDER
   * for the 3rd coordinate if the texture isn't 3D. */
   if (tex->b.target != PIPE_TEXTURE_3D) {
                  if (tex->tex.is_npot) {
                           /* Mask out the mirrored flag. */
   if (texstate->filter0 & R300_TX_WRAP_S(R300_TX_MIRRORED)) {
         }
                        /* Change repeat to clamp-to-edge.
   * (the repeat bit has a value of 0, no masking needed). */
   if ((texstate->filter0 & R300_TX_WRAP_S_MASK) ==
      R300_TX_WRAP_S(R300_TX_REPEAT)) {
      }
   if ((texstate->filter0 & R300_TX_WRAP_T_MASK) ==
      R300_TX_WRAP_T(R300_TX_REPEAT)) {
         } else {
      /* the MAX_MIP level is the largest (finest) one */
                     /* Float textures only support nearest and mip-nearest filtering. */
   if (util_format_is_float(view->base.format)) {
      /* No MAG linear filtering. */
   if ((texstate->filter0 & R300_TX_MAG_FILTER_MASK) ==
      R300_TX_MAG_FILTER_LINEAR) {
   texstate->filter0 &= ~R300_TX_MAG_FILTER_MASK;
      }
   /* No MIN linear filtering. */
   if ((texstate->filter0 & R300_TX_MIN_FILTER_MASK) ==
      R300_TX_MIN_FILTER_LINEAR) {
   texstate->filter0 &= ~R300_TX_MIN_FILTER_MASK;
      }
   /* No mipmap linear filtering. */
   if ((texstate->filter0 & R300_TX_MIN_FILTER_MIP_MASK) ==
      R300_TX_MIN_FILTER_MIP_LINEAR) {
   texstate->filter0 &= ~R300_TX_MIN_FILTER_MIP_MASK;
      }
   /* No anisotropic filtering. */
   texstate->filter0 &= ~R300_TX_MAX_ANISO_MASK;
                              size += 16 + (has_us_format ? 2 : 0);
   } else {
         /* For the KIL opcode to work on r3xx-r4xx, the texture unit
   * assigned to this opcode (it's always the first one) must be
   * enabled. Otherwise the opcode doesn't work.
   *
   * In order to not depend on the fragment shader, we just make
   * the first unit enabled all the time. */
   if (i == 0 && !r300->screen->caps.is_r500) {
                                             /* Just set some valid state. */
   texstate->format = r300->texkill_sampler->format;
   texstate->filter0 =
            r300_translate_tex_filters(PIPE_TEX_FILTER_NEAREST,
                           texstate->filter0 |= i << 28;
   size += 16 + (has_us_format ? 2 : 0);
                           /* Pick a fragment shader based on either the texture compare state
   * or the uses_pitch flag or some other external state. */
   if (count &&
      r300->fs_status == FRAGMENT_SHADER_VALID) {
         }
      static void r300_decompress_depth_textures(struct r300_context *r300)
   {
      struct r300_textures_state *state =
         struct pipe_resource *tex;
   unsigned count = MIN2(state->sampler_view_count,
                  if (!r300->locked_zbuffer) {
                  for (i = 0; i < count; i++) {
      if (state->sampler_views[i] && state->sampler_states[i]) {
                  if (tex == r300->locked_zbuffer->texture) {
      r300_decompress_zmask_locked(r300);
            }
      static void r300_validate_fragment_shader(struct r300_context *r300)
   {
               if (r300->fs.state && r300->fs_status != FRAGMENT_SHADER_VALID) {
      struct r300_fragment_program_external_state state;
   memset(&state, 0, sizeof(state));
            /* Pick the fragment shader based on external states.
      * Then mark the state dirty if the fragment shader is either dirty
      if (r300_pick_fragment_shader(r300, r300_fs(r300), &state) ||
         r300->fs_status == FRAGMENT_SHADER_DIRTY) {
                  /* Does Multiwrite need to be changed? */
   if (fb->nr_cbufs > 1) {
                     if (r300->fb_multiwrite != new_multiwrite) {
      r300->fb_multiwrite = new_multiwrite;
         }
         }
      static void r300_pick_vertex_shader(struct r300_context *r300)
   {
      struct r300_vertex_shader_code *ptr;
            if (r300->vs_state.state) {
               if (!vs->first) {
         /* Build the vertex shader for the first time. */
   vs->first = vs->shader = CALLOC_STRUCT(r300_vertex_shader_code);
   vs->first->wpos = wpos;
   r300_translate_vertex_shader(r300, vs);
   if (!vs->first->dummy)
         }
   /* Pick the vertex shader based on whether we need wpos */
   if (vs->first->wpos != wpos) {
         if (vs->first->next && vs->first->next->wpos == wpos) {
      ptr = vs->first->next;
   vs->first->next = NULL;
   ptr->next = vs->first;
      } else {
      ptr = CALLOC_STRUCT(r300_vertex_shader_code);
   ptr->next = vs->first;
   vs->first = vs->shader = ptr;
   vs->shader->wpos = wpos;
      }
   if (!vs->first->dummy)
         }
      void r300_update_derived_state(struct r300_context* r300)
   {
      if (r300->textures_state.dirty) {
      r300_decompress_depth_textures(r300);
               r300_validate_fragment_shader(r300);
   if (r300->screen->caps.has_tcl)
            if (r300->rs_block_state.dirty) {
               if (r300->draw) {
         memset(&r300->vertex_info, 0, sizeof(struct vertex_info));
   r300_draw_emit_all_attribs(r300);
   draw_compute_vertex_size(&r300->vertex_info);
                  }
