   /*
   * Copyright © 2014-2015 Broadcom
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
      #include "util/blob.h"
   #include "util/u_debug.h"
   #include "util/disk_cache.h"
   #include "util/u_memory.h"
   #include "util/perf/cpu_trace.h"
   #include "util/ralloc.h"
   #include "pipe/p_screen.h"
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_control_flow.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_serialize.h"
   #include "compiler/shader_enums.h"
      #include "tgsi_to_nir.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_info.h"
   #include "tgsi/tgsi_scan.h"
   #include "tgsi/tgsi_from_mesa.h"
      #define SWIZ(X, Y, Z, W) (unsigned[4]){      \
         TGSI_SWIZZLE_##X,                      \
   TGSI_SWIZZLE_##Y,                      \
   TGSI_SWIZZLE_##Z,                      \
            struct ttn_reg_info {
      /** nir register handle containing this TGSI index. */
   nir_def *reg;
   nir_variable *var;
   /** Offset (in vec4s) from the start of var for this TGSI index. */
      };
      struct ttn_compile {
      union tgsi_full_token *token;
   nir_builder build;
            struct ttn_reg_info *output_regs;
   struct ttn_reg_info *temp_regs;
            unsigned num_samp_types;
                     nir_variable **inputs;
   nir_variable **outputs;
   nir_variable *samplers[PIPE_MAX_SAMPLERS];
   nir_variable *images[PIPE_MAX_SHADER_IMAGES];
   nir_variable *ssbo[PIPE_MAX_SHADER_BUFFERS];
            unsigned num_samplers;
   unsigned num_images;
            nir_variable *input_var_face;
   nir_variable *input_var_position;
   nir_variable *input_var_point;
            /* How many TGSI_FILE_IMMEDIATE vec4s have been parsed so far. */
            bool cap_face_is_sysval;
   bool cap_position_is_sysval;
   bool cap_point_is_sysval;
   bool cap_samplers_as_deref;
   bool cap_integers;
      };
      #define ttn_swizzle(b, src, x, y, z, w) \
         #define ttn_channel(b, src, swiz) \
            static gl_varying_slot
   tgsi_varying_semantic_to_slot(unsigned semantic, unsigned index)
   {
      switch (semantic) {
   case TGSI_SEMANTIC_POSITION:
         case TGSI_SEMANTIC_COLOR:
      if (index == 0)
         else
      case TGSI_SEMANTIC_BCOLOR:
      if (index == 0)
         else
      case TGSI_SEMANTIC_FOG:
         case TGSI_SEMANTIC_PSIZE:
         case TGSI_SEMANTIC_GENERIC:
      assert(index < 32);
      case TGSI_SEMANTIC_FACE:
         case TGSI_SEMANTIC_EDGEFLAG:
         case TGSI_SEMANTIC_PRIMID:
         case TGSI_SEMANTIC_CLIPDIST:
      if (index == 0)
         else
      case TGSI_SEMANTIC_CLIPVERTEX:
         case TGSI_SEMANTIC_TEXCOORD:
      assert(index < 8);
      case TGSI_SEMANTIC_PCOORD:
         case TGSI_SEMANTIC_VIEWPORT_INDEX:
         case TGSI_SEMANTIC_LAYER:
         case TGSI_SEMANTIC_TESSINNER:
         case TGSI_SEMANTIC_TESSOUTER:
         default:
      fprintf(stderr, "Bad TGSI semantic: %d/%d\n", semantic, index);
         }
      static enum gl_frag_depth_layout
   ttn_get_depth_layout(unsigned tgsi_fs_depth_layout)
   {
      switch (tgsi_fs_depth_layout) {
   case TGSI_FS_DEPTH_LAYOUT_NONE:
         case TGSI_FS_DEPTH_LAYOUT_ANY:
         case TGSI_FS_DEPTH_LAYOUT_GREATER:
         case TGSI_FS_DEPTH_LAYOUT_LESS:
         case TGSI_FS_DEPTH_LAYOUT_UNCHANGED:
         default:
            }
      static enum glsl_interp_mode
   ttn_translate_interp_mode(unsigned tgsi_interp)
   {
      switch (tgsi_interp) {
   case TGSI_INTERPOLATE_CONSTANT:
         case TGSI_INTERPOLATE_LINEAR:
         case TGSI_INTERPOLATE_PERSPECTIVE:
         case TGSI_INTERPOLATE_COLOR:
         default:
            }
      static void
   ttn_emit_declaration(struct ttn_compile *c)
   {
      nir_builder *b = &c->build;
   struct tgsi_full_declaration *decl = &c->token->FullDeclaration;
   unsigned array_size = decl->Range.Last - decl->Range.First + 1;
   unsigned file = decl->Declaration.File;
            if (file == TGSI_FILE_TEMPORARY) {
      if (decl->Declaration.Array) {
      /* for arrays, we create variables instead of registers: */
   nir_variable *var =
      nir_variable_create(b->shader, nir_var_shader_temp,
                     for (i = 0; i < array_size; i++) {
      /* point all the matching slots to the same var,
   * with appropriate offset set, mostly just so
   * we know what to do when tgsi does a non-indirect
   * access
   */
   c->temp_regs[decl->Range.First + i].reg = NULL;
   c->temp_regs[decl->Range.First + i].var = var;
         } else {
      for (i = 0; i < array_size; i++) {
      nir_def *reg = nir_decl_reg(b, 4, 32, 0);
   c->temp_regs[decl->Range.First + i].reg = reg;
   c->temp_regs[decl->Range.First + i].var = NULL;
            } else if (file == TGSI_FILE_ADDRESS) {
         } else if (file == TGSI_FILE_SYSTEM_VALUE) {
         } else if (file == TGSI_FILE_BUFFER) {
         } else if (file == TGSI_FILE_IMAGE) {
         } else if (file == TGSI_FILE_SAMPLER) {
         } else if (file == TGSI_FILE_SAMPLER_VIEW) {
      struct tgsi_declaration_sampler_view *sview = &decl->SamplerView;
            assert((sview->ReturnTypeX == sview->ReturnTypeY) &&
                  switch (sview->ReturnTypeX) {
   case TGSI_RETURN_TYPE_SINT:
      type = nir_type_int32;
      case TGSI_RETURN_TYPE_UINT:
      type = nir_type_uint32;
      case TGSI_RETURN_TYPE_FLOAT:
   default:
      type = nir_type_float32;
               for (i = 0; i < array_size; i++) {
            } else {
               assert(file == TGSI_FILE_INPUT ||
                  /* nothing to do for UBOs: */
   if ((file == TGSI_FILE_CONSTANT) && decl->Declaration.Dimension &&
      decl->Dim.Index2D != 0) {
   b->shader->info.num_ubos =
         c->ubo_sizes[decl->Dim.Index2D] =
                     if ((file == TGSI_FILE_INPUT) || (file == TGSI_FILE_OUTPUT)) {
      is_array = (is_array && decl->Declaration.Array &&
               for (i = 0; i < array_size; i++) {
                              var->type = glsl_vec4_type();
                  switch (file) {
   case TGSI_FILE_INPUT:
      var->data.read_only = true;
                  if (c->scan->processor == PIPE_SHADER_FRAGMENT) {
      if (decl->Semantic.Name == TGSI_SEMANTIC_FACE) {
      var->type = glsl_bool_type();
   if (c->cap_face_is_sysval) {
      var->data.mode = nir_var_system_value;
      } else {
         }
      } else if (decl->Semantic.Name == TGSI_SEMANTIC_POSITION) {
      if (c->cap_position_is_sysval) {
      var->data.mode = nir_var_system_value;
      } else {
         }
      } else if (decl->Semantic.Name == TGSI_SEMANTIC_PCOORD) {
      if (c->cap_point_is_sysval) {
      var->data.mode = nir_var_system_value;
      } else {
         }
      } else {
      var->data.location =
      tgsi_varying_semantic_to_slot(decl->Semantic.Name,
      } else {
      assert(!decl->Declaration.Semantic);
      }
   var->data.index = 0;
                                             case TGSI_FILE_OUTPUT: {
      int semantic_name = decl->Semantic.Name;
   int semantic_index = decl->Semantic.Index;
   /* Since we can't load from outputs in the IR, we make temporaries
   * for the outputs and emit stores to the real outputs at the end of
   * the shader.
   */
                  var->data.mode = nir_var_shader_out;
   var->name = ralloc_asprintf(var, "out_%d", idx);
   var->data.index = 0;
   var->data.interpolation =
         var->data.patch = semantic_name == TGSI_SEMANTIC_TESSINNER ||
                  if (c->scan->processor == PIPE_SHADER_FRAGMENT) {
      switch (semantic_name) {
   case TGSI_SEMANTIC_COLOR: {
      /* TODO tgsi loses some information, so we cannot
   * actually differentiate here between DSB and MRT
   * at this point.  But so far no drivers using tgsi-
   * to-nir support dual source blend:
   */
   bool dual_src_blend = false;
   if (dual_src_blend && (semantic_index == 1)) {
      var->data.location = FRAG_RESULT_DATA0;
      } else {
      if (c->scan->properties[TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS])
         else
      }
      }
   case TGSI_SEMANTIC_POSITION:
      var->data.location = FRAG_RESULT_DEPTH;
   var->type = glsl_float_type();
      case TGSI_SEMANTIC_STENCIL:
      var->data.location = FRAG_RESULT_STENCIL;
   var->type = glsl_int_type();
      case TGSI_SEMANTIC_SAMPLEMASK:
                        default:
      fprintf(stderr, "Bad TGSI semantic: %d/%d\n",
               } else {
      var->data.location =
         if (var->data.location == VARYING_SLOT_FOGC ||
      var->data.location == VARYING_SLOT_PSIZ) {
      } else if (var->data.location == VARYING_SLOT_LAYER) {
         } else if (c->cap_compact_arrays &&
            var->type = glsl_array_type(glsl_float_type(),
                              if (is_array) {
      unsigned j;
   for (j = 0; j < array_size; j++) {
      c->output_regs[idx + j].offset = i + j;
         } else {
                                 if (c->cap_compact_arrays && var->data.location == VARYING_SLOT_CLIP_DIST1) {
                        for (int i = 0; i < array_size; i++)
      }
         case TGSI_FILE_CONSTANT:
      var->data.mode = nir_var_uniform;
   var->name = ralloc_asprintf(var, "uniform_%d", idx);
   var->data.location = idx;
      default:
      unreachable("bad declaration file");
                        if (is_array)
               }
      static void
   ttn_emit_immediate(struct ttn_compile *c)
   {
      nir_builder *b = &c->build;
   struct tgsi_full_immediate *tgsi_imm = &c->token->FullImmediate;
   nir_load_const_instr *load_const;
            load_const = nir_load_const_instr_create(b->shader, 4, 32);
   c->imm_defs[c->next_imm] = &load_const->def;
            for (i = 0; i < load_const->def.num_components; i++)
               }
      static nir_def *
   ttn_src_for_indirect(struct ttn_compile *c, struct tgsi_ind_register *indirect);
      /* generate either a constant or indirect deref chain for accessing an
   * array variable.
   */
   static nir_deref_instr *
   ttn_array_deref(struct ttn_compile *c, nir_variable *var, unsigned offset,
         {
      nir_deref_instr *deref = nir_build_deref_var(&c->build, var);
   nir_def *index = nir_imm_int(&c->build, offset);
   if (indirect)
            }
      /* Special case: Turn the frontface varying into a load of the
   * frontface variable, and create the vector as required by TGSI.
   */
   static nir_def *
   ttn_emulate_tgsi_front_face(struct ttn_compile *c)
   {
               if (c->cap_face_is_sysval) {
      /* When it's a system value, it should be an integer vector: (F, 0, 0, 1)
   * F is 0xffffffff if front-facing, 0 if not.
                     tgsi_frontface[0] = nir_bcsel(&c->build,
                     tgsi_frontface[1] = nir_imm_int(&c->build, 0);
   tgsi_frontface[2] = nir_imm_int(&c->build, 0);
      } else {
      /* When it's an input, it should be a float vector: (F, 0.0, 0.0, 1.0)
   * F is positive if front-facing, negative if not.
            assert(c->input_var_face);
            tgsi_frontface[0] = nir_bcsel(&c->build,
                     tgsi_frontface[1] = nir_imm_float(&c->build, 0.0);
   tgsi_frontface[2] = nir_imm_float(&c->build, 0.0);
                  }
      static nir_src
   ttn_src_for_file_and_index(struct ttn_compile *c, unsigned file, unsigned index,
                           {
      nir_builder *b = &c->build;
                     switch (file) {
   case TGSI_FILE_TEMPORARY:
      if (c->temp_regs[index].var) {
      unsigned offset = c->temp_regs[index].offset;
   nir_variable *var = c->temp_regs[index].var;
                     } else {
      assert(!indirect);
      }
   assert(!dim);
         case TGSI_FILE_ADDRESS:
      src = nir_src_for_ssa(nir_load_reg(b, c->addr_reg));
   assert(!dim);
         case TGSI_FILE_IMMEDIATE:
      src = nir_src_for_ssa(c->imm_defs[index]);
   assert(!indirect);
   assert(!dim);
         case TGSI_FILE_SYSTEM_VALUE: {
               assert(!indirect);
            switch (c->scan->system_value_semantic_name[index]) {
   case TGSI_SEMANTIC_VERTEXID_NOBASE:
      load = nir_load_vertex_id_zero_base(b);
      case TGSI_SEMANTIC_VERTEXID:
      load = nir_load_vertex_id(b);
      case TGSI_SEMANTIC_BASEVERTEX:
      load = nir_load_base_vertex(b);
      case TGSI_SEMANTIC_INSTANCEID:
      load = nir_load_instance_id(b);
      case TGSI_SEMANTIC_FACE:
      assert(c->cap_face_is_sysval);
   load = ttn_emulate_tgsi_front_face(c);
      case TGSI_SEMANTIC_POSITION:
      assert(c->cap_position_is_sysval);
   load = nir_load_frag_coord(b);
      case TGSI_SEMANTIC_PCOORD:
      assert(c->cap_point_is_sysval);
   load = nir_load_point_coord(b);
      case TGSI_SEMANTIC_THREAD_ID:
      load = nir_load_local_invocation_id(b);
      case TGSI_SEMANTIC_BLOCK_ID:
      load = nir_load_workgroup_id(b);
      case TGSI_SEMANTIC_BLOCK_SIZE:
      load = nir_load_workgroup_size(b);
      case TGSI_SEMANTIC_CS_USER_DATA_AMD:
      load = nir_load_user_data_amd(b);
      case TGSI_SEMANTIC_TESS_DEFAULT_INNER_LEVEL:
      load = nir_load_tess_level_inner_default(b);
      case TGSI_SEMANTIC_TESS_DEFAULT_OUTER_LEVEL:
      load = nir_load_tess_level_outer_default(b);
      case TGSI_SEMANTIC_SAMPLEID:
      load = nir_load_sample_id(b);
      default:
                  if (load->num_components == 2)
         else if (load->num_components == 3)
            src = nir_src_for_ssa(load);
               case TGSI_FILE_INPUT:
      if (c->scan->processor == PIPE_SHADER_FRAGMENT &&
      c->scan->input_semantic_name[index] == TGSI_SEMANTIC_FACE) {
   assert(!c->cap_face_is_sysval && c->input_var_face);
      } else if (c->scan->processor == PIPE_SHADER_FRAGMENT &&
      c->scan->input_semantic_name[index] == TGSI_SEMANTIC_POSITION) {
   assert(!c->cap_position_is_sysval && c->input_var_position);
      } else if (c->scan->processor == PIPE_SHADER_FRAGMENT &&
      c->scan->input_semantic_name[index] == TGSI_SEMANTIC_PCOORD) {
   assert(!c->cap_point_is_sysval && c->input_var_point);
      } else {
      /* Indirection on input arrays isn't supported by TTN. */
   assert(!dim);
   nir_deref_instr *deref = nir_build_deref_var(&c->build,
            }
         case TGSI_FILE_OUTPUT:
      if (c->scan->processor == PIPE_SHADER_FRAGMENT) {
      c->outputs[index]->data.fb_fetch_output = 1;
   nir_deref_instr *deref = nir_build_deref_var(&c->build,
            }
   unreachable("unsupported output read");
         case TGSI_FILE_CONSTANT: {
      nir_intrinsic_instr *load;
   nir_intrinsic_op op;
            if (dim && (dim->Index > 0 || dim->Indirect)) {
         } else {
                  load = nir_intrinsic_instr_create(b->shader, op);
   if (op == nir_intrinsic_load_uniform) {
      nir_intrinsic_set_dest_type(load, src_is_float ? nir_type_float :
               load->num_components = 4;
   if (dim && (dim->Index > 0 || dim->Indirect)) {
      if (dimind) {
      load->src[srcn] =
      ttn_src_for_file_and_index(c, dimind->File, dimind->Index,
   } else {
      /* UBOs start at index 1 in TGSI: */
   load->src[srcn] =
      }
               nir_def *offset;
   if (op == nir_intrinsic_load_ubo) {
      /* UBO loads don't have a base offset. */
   offset = nir_imm_int(b, index);
   if (indirect) {
         }
   /* UBO offsets are in bytes, but TGSI gives them to us in vec4's */
                  /* Set a very conservative base/range of the access: 16 bytes if not
   * indirect at all, offset to the end of the UBO if the offset is
   * indirect, and totally unknown if the block number is indirect.
   */
   uint32_t base = index * 16;
   nir_intrinsic_set_range_base(load, base);
   if (dimind)
         else if (indirect)
         else
      } else {
      nir_intrinsic_set_base(load, index);
   if (indirect) {
      offset = ttn_src_for_indirect(c, indirect);
      } else {
      offset = nir_imm_int(b, 0);
         }
            nir_def_init(&load->instr, &load->def, 4, 32);
            src = nir_src_for_ssa(&load->def);
               default:
                        }
      static nir_def *
   ttn_src_for_indirect(struct ttn_compile *c, struct tgsi_ind_register *indirect)
   {
      nir_builder *b = &c->build;
   nir_alu_src src;
   memset(&src, 0, sizeof(src));
   for (int i = 0; i < 4; i++)
         src.src = ttn_src_for_file_and_index(c,
                              }
      static nir_variable *
   ttn_get_var(struct ttn_compile *c, struct tgsi_full_dst_register *tgsi_fdst)
   {
      struct tgsi_dst_register *tgsi_dst = &tgsi_fdst->Register;
            if (tgsi_dst->File == TGSI_FILE_TEMPORARY) {
      /* we should not have an indirect when there is no var! */
   if (!c->temp_regs[index].var)
                        }
      static nir_def *
   ttn_get_src(struct ttn_compile *c, struct tgsi_full_src_register *tgsi_fsrc,
         {
      nir_builder *b = &c->build;
   struct tgsi_src_register *tgsi_src = &tgsi_fsrc->Register;
   enum tgsi_opcode opcode = c->token->FullInstruction.Instruction.Opcode;
   unsigned tgsi_src_type = tgsi_opcode_infer_src_type(opcode, src_idx);
   bool src_is_float = (tgsi_src_type == TGSI_TYPE_FLOAT ||
                                 if (tgsi_src->File == TGSI_FILE_NULL) {
         } else if (tgsi_src->File == TGSI_FILE_SAMPLER ||
            tgsi_src->File == TGSI_FILE_IMAGE ||
   /* Only the index of the resource gets used in texturing, and it will
   * handle looking that up on its own instead of using the nir_alu_src.
   */
   assert(!tgsi_src->Indirect);
      } else {
      struct tgsi_ind_register *ind = NULL;
   struct tgsi_dimension *dim = NULL;
   struct tgsi_ind_register *dimind = NULL;
   if (tgsi_src->Indirect)
         if (tgsi_src->Dimension) {
      dim = &tgsi_fsrc->Dimension;
   if (dim->Indirect)
      }
   src.src = ttn_src_for_file_and_index(c,
                                 src.swizzle[0] = tgsi_src->SwizzleX;
   src.swizzle[1] = tgsi_src->SwizzleY;
   src.swizzle[2] = tgsi_src->SwizzleZ;
                     if (tgsi_type_is_64bit(tgsi_src_type))
            if (tgsi_src->Absolute) {
      assert(src_is_float);
               if (tgsi_src->Negate) {
      if (src_is_float)
         else
                  }
      static nir_def *
   ttn_alu(nir_builder *b, nir_op op, unsigned dest_bitsize, nir_def **src)
   {
      nir_def *def = nir_build_alu_src_arr(b, op, src);
   if (def->bit_size == 1)
         assert(def->bit_size == dest_bitsize);
   if (dest_bitsize == 64) {
      /* Replicate before bitcasting, so we end up with 4x32 at the end */
   if (def->num_components == 1)
            if (def->num_components > 2) {
      /* 32 -> 64 bit conversion ops are supposed to only convert the first
   * two components, and we need to truncate here to avoid creating a
   * vec8 after bitcasting the destination.
   */
      }
      }
      }
      /* EXP - Approximate Exponential Base 2
   *  dst.x = 2^{\lfloor src.x\rfloor}
   *  dst.y = src.x - \lfloor src.x\rfloor
   *  dst.z = 2^{src.x}
   *  dst.w = 1.0
   */
   static nir_def *
   ttn_exp(nir_builder *b, nir_def **src)
   {
               return nir_vec4(b, nir_fexp2(b, nir_ffloor(b, srcx)),
                  }
      /* LOG - Approximate Logarithm Base 2
   *  dst.x = \lfloor\log_2{|src.x|}\rfloor
   *  dst.y = \frac{|src.x|}{2^{\lfloor\log_2{|src.x|}\rfloor}}
   *  dst.z = \log_2{|src.x|}
   *  dst.w = 1.0
   */
   static nir_def *
   ttn_log(nir_builder *b, nir_def **src)
   {
      nir_def *abs_srcx = nir_fabs(b, ttn_channel(b, src[0], X));
            return nir_vec4(b, nir_ffloor(b, log2),
                  }
      /* DST - Distance Vector
   *   dst.x = 1.0
   *   dst.y = src0.y \times src1.y
   *   dst.z = src0.z
   *   dst.w = src1.w
   */
   static nir_def *
   ttn_dst(nir_builder *b, nir_def **src)
   {
      return nir_vec4(b, nir_imm_float(b, 1.0),
                        }
      /* LIT - Light Coefficients
   *  dst.x = 1.0
   *  dst.y = max(src.x, 0.0)
   *  dst.z = (src.x > 0.0) ? max(src.y, 0.0)^{clamp(src.w, -128.0, 128.0))} : 0
   *  dst.w = 1.0
   */
   static nir_def *
   ttn_lit(nir_builder *b, nir_def **src)
   {
      nir_def *src0_y = ttn_channel(b, src[0], Y);
   nir_def *wclamp = nir_fmax(b, nir_fmin(b, ttn_channel(b, src[0], W),
               nir_def *pow = nir_fpow(b, nir_fmax(b, src0_y, nir_imm_float(b, 0.0)),
         nir_def *z = nir_bcsel(b, nir_flt_imm(b, ttn_channel(b, src[0], X), 0.0),
            return nir_vec4(b, nir_imm_float(b, 1.0),
                  }
      static void
   ttn_barrier(nir_builder *b)
   {
         }
      static void
   ttn_kill(nir_builder *b)
   {
      nir_discard(b);
      }
      static void
   ttn_kill_if(nir_builder *b, nir_def **src)
   {
      /* flt must be exact, because NaN shouldn't discard. (apps rely on this) */
   b->exact = true;
   nir_def *cmp = nir_bany(b, nir_flt_imm(b, src[0], 0.0));
            nir_discard_if(b, cmp);
      }
      static void
   get_texture_info(unsigned texture,
                     {
      assert(is_array);
            if (is_shadow)
            switch (texture) {
   case TGSI_TEXTURE_BUFFER:
      *dim = GLSL_SAMPLER_DIM_BUF;
      case TGSI_TEXTURE_1D:
      *dim = GLSL_SAMPLER_DIM_1D;
      case TGSI_TEXTURE_1D_ARRAY:
      *dim = GLSL_SAMPLER_DIM_1D;
   *is_array = true;
      case TGSI_TEXTURE_SHADOW1D:
      *dim = GLSL_SAMPLER_DIM_1D;
   *is_shadow = true;
      case TGSI_TEXTURE_SHADOW1D_ARRAY:
      *dim = GLSL_SAMPLER_DIM_1D;
   *is_shadow = true;
   *is_array = true;
      case TGSI_TEXTURE_2D:
      *dim = GLSL_SAMPLER_DIM_2D;
      case TGSI_TEXTURE_2D_ARRAY:
      *dim = GLSL_SAMPLER_DIM_2D;
   *is_array = true;
      case TGSI_TEXTURE_2D_MSAA:
      *dim = GLSL_SAMPLER_DIM_MS;
      case TGSI_TEXTURE_2D_ARRAY_MSAA:
      *dim = GLSL_SAMPLER_DIM_MS;
   *is_array = true;
      case TGSI_TEXTURE_SHADOW2D:
      *dim = GLSL_SAMPLER_DIM_2D;
   *is_shadow = true;
      case TGSI_TEXTURE_SHADOW2D_ARRAY:
      *dim = GLSL_SAMPLER_DIM_2D;
   *is_shadow = true;
   *is_array = true;
      case TGSI_TEXTURE_3D:
      *dim = GLSL_SAMPLER_DIM_3D;
      case TGSI_TEXTURE_CUBE:
      *dim = GLSL_SAMPLER_DIM_CUBE;
      case TGSI_TEXTURE_CUBE_ARRAY:
      *dim = GLSL_SAMPLER_DIM_CUBE;
   *is_array = true;
      case TGSI_TEXTURE_SHADOWCUBE:
      *dim = GLSL_SAMPLER_DIM_CUBE;
   *is_shadow = true;
      case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      *dim = GLSL_SAMPLER_DIM_CUBE;
   *is_shadow = true;
   *is_array = true;
      case TGSI_TEXTURE_RECT:
      *dim = GLSL_SAMPLER_DIM_RECT;
      case TGSI_TEXTURE_SHADOWRECT:
      *dim = GLSL_SAMPLER_DIM_RECT;
   *is_shadow = true;
      default:
      fprintf(stderr, "Unknown TGSI texture target %d\n", texture);
         }
      static enum glsl_base_type
   base_type_for_alu_type(nir_alu_type type)
   {
               switch (type) {
   case nir_type_float:
         case nir_type_int:
         case nir_type_uint:
         default:
            }
      static nir_variable *
   get_sampler_var(struct ttn_compile *c, int binding,
                  enum glsl_sampler_dim dim,
   bool is_shadow,
      {
      nir_variable *var = c->samplers[binding];
   if (!var) {
      const struct glsl_type *type =
         var = nir_variable_create(c->build.shader, nir_var_uniform, type,
         var->data.binding = binding;
            c->samplers[binding] = var;
            /* Record textures used */
   BITSET_SET(c->build.shader->info.textures_used, binding);
   if (op == nir_texop_txf || op == nir_texop_txf_ms)
                        }
      static nir_variable *
   get_image_var(struct ttn_compile *c, int binding,
               enum glsl_sampler_dim dim,
   bool is_array,
   enum glsl_base_type base_type,
   {
               if (!var) {
               var = nir_variable_create(c->build.shader, nir_var_image, type, "image");
   var->data.binding = binding;
   var->data.explicit_binding = true;
   var->data.access = access;
            c->images[binding] = var;
   c->num_images = MAX2(c->num_images, binding + 1);
   if (dim == GLSL_SAMPLER_DIM_MS)
                  }
      static void
   add_ssbo_var(struct ttn_compile *c, int binding)
   {
               if (!var) {
      /* A length of 0 is used to denote unsized arrays */
            struct glsl_struct_field field = {
         .type = type,
   .name = "data",
            var = nir_variable_create(c->build.shader, nir_var_mem_ssbo, type, "ssbo");
   var->data.binding = binding;
   var->interface_type =
      glsl_interface_type(&field, 1, GLSL_INTERFACE_PACKING_STD430,
            }
      static nir_def *
   ttn_tex(struct ttn_compile *c, nir_def **src)
   {
      nir_builder *b = &c->build;
   struct tgsi_full_instruction *tgsi_inst = &c->token->FullInstruction;
   nir_tex_instr *instr;
   nir_texop op;
            switch (tgsi_inst->Instruction.Opcode) {
   case TGSI_OPCODE_TEX:
      op = nir_texop_tex;
   num_srcs = 1;
      case TGSI_OPCODE_TEX2:
      op = nir_texop_tex;
   num_srcs = 1;
   samp = 2;
      case TGSI_OPCODE_TXP:
      op = nir_texop_tex;
   num_srcs = 2;
      case TGSI_OPCODE_TXB:
      op = nir_texop_txb;
   num_srcs = 2;
      case TGSI_OPCODE_TXB2:
      op = nir_texop_txb;
   num_srcs = 2;
   samp = 2;
      case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TEX_LZ:
      op = nir_texop_txl;
   num_srcs = 2;
      case TGSI_OPCODE_TXL2:
      op = nir_texop_txl;
   num_srcs = 2;
   samp = 2;
      case TGSI_OPCODE_TXF:
   case TGSI_OPCODE_TXF_LZ:
      if (tgsi_inst->Texture.Texture == TGSI_TEXTURE_2D_MSAA ||
      tgsi_inst->Texture.Texture == TGSI_TEXTURE_2D_ARRAY_MSAA) {
      } else {
         }
   num_srcs = 2;
      case TGSI_OPCODE_TXD:
      op = nir_texop_txd;
   num_srcs = 3;
   samp = 3;
      case TGSI_OPCODE_LODQ:
      op = nir_texop_lod;
   num_srcs = 1;
         default:
      fprintf(stderr, "unknown TGSI tex op %d\n", tgsi_inst->Instruction.Opcode);
               if (tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D ||
      tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D_ARRAY ||
   tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D ||
   tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D_ARRAY ||
   tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOWRECT ||
   tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE ||
   tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE_ARRAY) {
               /* Deref sources */
                     instr = nir_tex_instr_create(b->shader, num_srcs);
            get_texture_info(tgsi_inst->Texture.Texture,
            instr->coord_components =
            if (instr->is_array)
                     /* TODO if we supported any opc's which take an explicit SVIEW
   * src, we would use that here instead.  But for the "legacy"
   * texture opc's the SVIEW index is same as SAMP index:
   */
            nir_alu_type sampler_type =
            if (op == nir_texop_lod) {
         } else {
                  nir_variable *var =
      get_sampler_var(c, sview, instr->sampler_dim,
                                             instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         src_number++;
   instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
                  instr->src[src_number] =
      nir_tex_src_for_ssa(nir_tex_src_coord,
               if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXP) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_projector,
                     if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXB) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_bias,
                     if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXB2) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_bias,
                     if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXL ||
      tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TEX_LZ) {
   if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TEX_LZ)
         else
         instr->src[src_number].src_type = nir_tex_src_lod;
               if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXL2) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_lod,
                     if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXF ||
      tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXF_LZ) {
   if (op == nir_texop_txf_ms) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_ms_index,
      } else {
      if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXF_LZ)
         else
            }
               if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXD) {
      instr->src[src_number] =
      nir_tex_src_for_ssa(nir_tex_src_ddx,
      src_number++;
   instr->src[src_number] =
      nir_tex_src_for_ssa(nir_tex_src_ddy,
                  if (instr->is_shadow) {
      if (instr->coord_components == 4)
         else if (instr->coord_components == 3)
         else
            instr->src[src_number].src_type = nir_tex_src_comparator;
               for (i = 0; i < tgsi_inst->Texture.NumOffsets; i++) {
      struct tgsi_texture_offset *tex_offset = &tgsi_inst->TexOffsets[i];
   /* since TexOffset ins't using tgsi_full_src_register we get to
   * do some extra gymnastics:
   */
                     src.src = ttn_src_for_file_and_index(c,
                              src.swizzle[0] = tex_offset->SwizzleX;
   src.swizzle[1] = tex_offset->SwizzleY;
   src.swizzle[2] = tex_offset->SwizzleZ;
            instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_offset,
                     assert(src_number == num_srcs);
            nir_def_init(&instr->instr, &instr->def,
         nir_builder_instr_insert(b, &instr->instr);
      }
      /* TGSI_OPCODE_TXQ is actually two distinct operations:
   *
   *     dst.x = texture\_width(unit, lod)
   *     dst.y = texture\_height(unit, lod)
   *     dst.z = texture\_depth(unit, lod)
   *     dst.w = texture\_levels(unit)
   *
   * dst.xyz map to NIR txs opcode, and dst.w maps to query_levels
   */
   static nir_def *
   ttn_txq(struct ttn_compile *c, nir_def **src)
   {
      nir_builder *b = &c->build;
   struct tgsi_full_instruction *tgsi_inst = &c->token->FullInstruction;
            txs = nir_tex_instr_create(b->shader, 2);
   txs->op = nir_texop_txs;
   txs->dest_type = nir_type_uint32;
   get_texture_info(tgsi_inst->Texture.Texture,
            qlv = nir_tex_instr_create(b->shader, 1);
   qlv->op = nir_texop_query_levels;
   qlv->dest_type = nir_type_uint32;
   get_texture_info(tgsi_inst->Texture.Texture,
            assert(tgsi_inst->Src[1].Register.File == TGSI_FILE_SAMPLER);
            nir_alu_type sampler_type =
            nir_variable *var =
      get_sampler_var(c, sview, txs->sampler_dim,
                                    txs->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
            qlv->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
            /* lod: */
   txs->src[1] = nir_tex_src_for_ssa(nir_tex_src_lod,
            nir_def_init(&txs->instr, &txs->def, nir_tex_instr_dest_size(txs), 32);
            nir_def_init(&qlv->instr, &qlv->def, 1, 32);
            return nir_vector_insert_imm(b,
            }
      static enum glsl_base_type
   get_image_base_type(struct tgsi_full_instruction *tgsi_inst)
   {
      const struct util_format_description *desc =
            if (desc->channel[0].pure_integer) {
      if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED)
         else
      }
      }
      static enum gl_access_qualifier
   get_mem_qualifier(struct tgsi_full_instruction *tgsi_inst)
   {
               if (tgsi_inst->Memory.Qualifier & TGSI_MEMORY_COHERENT)
         if (tgsi_inst->Memory.Qualifier & TGSI_MEMORY_RESTRICT)
         if (tgsi_inst->Memory.Qualifier & TGSI_MEMORY_VOLATILE)
         if (tgsi_inst->Memory.Qualifier & TGSI_MEMORY_STREAM_CACHE_POLICY)
               }
      static nir_def *
   ttn_mem(struct ttn_compile *c, nir_def **src)
   {
      nir_builder *b = &c->build;
   struct tgsi_full_instruction *tgsi_inst = &c->token->FullInstruction;
   nir_intrinsic_instr *instr = NULL;
            switch (tgsi_inst->Instruction.Opcode) {
   case TGSI_OPCODE_LOAD:
      assert(!tgsi_inst->Src[0].Register.Indirect);
   resource_index = tgsi_inst->Src[0].Register.Index;
   file = tgsi_inst->Src[0].Register.File;
   addr_src_index = 1;
      case TGSI_OPCODE_STORE:
      assert(!tgsi_inst->Dst[0].Register.Indirect);
   resource_index = tgsi_inst->Dst[0].Register.Index;
   file = tgsi_inst->Dst[0].Register.File;
   addr_src_index = 0;
      default:
                  if (file == TGSI_FILE_BUFFER) {
               switch (tgsi_inst->Instruction.Opcode) {
   case TGSI_OPCODE_LOAD:
      op = nir_intrinsic_load_ssbo;
      case TGSI_OPCODE_STORE:
      op = nir_intrinsic_store_ssbo;
      default:
                           instr = nir_intrinsic_instr_create(b->shader, op);
   instr->num_components = util_last_bit(tgsi_inst->Dst[0].Register.WriteMask);
   nir_intrinsic_set_access(instr, get_mem_qualifier(tgsi_inst));
            unsigned i = 0;
   if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_STORE)
      instr->src[i++] = nir_src_for_ssa(nir_swizzle(b, src[1], SWIZ(X, Y, Z, W),
      instr->src[i++] = nir_src_for_ssa(nir_imm_int(b, resource_index));
            if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_STORE)
         } else if (file == TGSI_FILE_IMAGE) {
               switch (tgsi_inst->Instruction.Opcode) {
   case TGSI_OPCODE_LOAD:
      op = nir_intrinsic_image_deref_load;
      case TGSI_OPCODE_STORE:
      op = nir_intrinsic_image_deref_store;
      default:
                           /* Set the image variable dereference. */
   enum glsl_sampler_dim dim;
   bool is_array;
            enum glsl_base_type base_type = get_image_base_type(tgsi_inst);
            nir_variable *image =
      get_image_var(c, resource_index,
            nir_deref_instr *image_deref = nir_build_deref_var(b, image);
                     instr->src[0] = nir_src_for_ssa(&image_deref->def);
            /* Set the sample argument, which is undefined for single-sample images. */
   if (glsl_get_sampler_dim(type) == GLSL_SAMPLER_DIM_MS) {
         } else {
                  if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_LOAD) {
                           if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_STORE) {
      instr->src[3] = nir_src_for_ssa(nir_swizzle(b, src[1], SWIZ(X, Y, Z, W),
                        } else {
                     if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_LOAD) {
      nir_def_init(&instr->instr, &instr->def, instr->num_components, 32);
   nir_builder_instr_insert(b, &instr->instr);
      } else {
      nir_builder_instr_insert(b, &instr->instr);
         }
      static const nir_op op_trans[TGSI_OPCODE_LAST] = {
      [TGSI_OPCODE_ARL] = 0,
   [TGSI_OPCODE_MOV] = nir_op_mov,
   [TGSI_OPCODE_FBFETCH] = nir_op_mov,
   [TGSI_OPCODE_LIT] = 0,
   [TGSI_OPCODE_RCP] = nir_op_frcp,
   [TGSI_OPCODE_RSQ] = nir_op_frsq,
   [TGSI_OPCODE_EXP] = 0,
   [TGSI_OPCODE_LOG] = 0,
   [TGSI_OPCODE_MUL] = nir_op_fmul,
   [TGSI_OPCODE_ADD] = nir_op_fadd,
   [TGSI_OPCODE_DP3] = 0,
   [TGSI_OPCODE_DP4] = 0,
   [TGSI_OPCODE_DST] = 0,
   [TGSI_OPCODE_MIN] = nir_op_fmin,
   [TGSI_OPCODE_MAX] = nir_op_fmax,
   [TGSI_OPCODE_SLT] = nir_op_slt,
   [TGSI_OPCODE_SGE] = nir_op_sge,
   [TGSI_OPCODE_MAD] = nir_op_ffma,
   [TGSI_OPCODE_TEX_LZ] = 0,
   [TGSI_OPCODE_LRP] = 0,
   [TGSI_OPCODE_SQRT] = nir_op_fsqrt,
   [TGSI_OPCODE_FRC] = nir_op_ffract,
   [TGSI_OPCODE_TXF_LZ] = 0,
   [TGSI_OPCODE_FLR] = nir_op_ffloor,
   [TGSI_OPCODE_ROUND] = nir_op_fround_even,
   [TGSI_OPCODE_EX2] = nir_op_fexp2,
   [TGSI_OPCODE_LG2] = nir_op_flog2,
   [TGSI_OPCODE_POW] = nir_op_fpow,
   [TGSI_OPCODE_COS] = nir_op_fcos,
   [TGSI_OPCODE_DDX] = nir_op_fddx,
   [TGSI_OPCODE_DDY] = nir_op_fddy,
   [TGSI_OPCODE_KILL] = 0,
   [TGSI_OPCODE_PK2H] = 0, /* XXX */
   [TGSI_OPCODE_PK2US] = 0, /* XXX */
   [TGSI_OPCODE_PK4B] = 0, /* XXX */
   [TGSI_OPCODE_PK4UB] = 0, /* XXX */
   [TGSI_OPCODE_SEQ] = nir_op_seq,
   [TGSI_OPCODE_SGT] = 0,
   [TGSI_OPCODE_SIN] = nir_op_fsin,
   [TGSI_OPCODE_SNE] = nir_op_sne,
   [TGSI_OPCODE_SLE] = 0,
   [TGSI_OPCODE_TEX] = 0,
   [TGSI_OPCODE_TXD] = 0,
   [TGSI_OPCODE_TXP] = 0,
   [TGSI_OPCODE_UP2H] = 0, /* XXX */
   [TGSI_OPCODE_UP2US] = 0, /* XXX */
   [TGSI_OPCODE_UP4B] = 0, /* XXX */
   [TGSI_OPCODE_UP4UB] = 0, /* XXX */
            /* No function calls, yet. */
   [TGSI_OPCODE_CAL] = 0, /* XXX */
            [TGSI_OPCODE_SSG] = nir_op_fsign,
   [TGSI_OPCODE_CMP] = 0,
   [TGSI_OPCODE_TXB] = 0,
   [TGSI_OPCODE_DIV] = nir_op_fdiv,
   [TGSI_OPCODE_DP2] = 0,
            [TGSI_OPCODE_BRK] = 0,
   [TGSI_OPCODE_IF] = 0,
   [TGSI_OPCODE_UIF] = 0,
   [TGSI_OPCODE_ELSE] = 0,
            [TGSI_OPCODE_DDX_FINE] = nir_op_fddx_fine,
            [TGSI_OPCODE_CEIL] = nir_op_fceil,
   [TGSI_OPCODE_I2F] = nir_op_i2f32,
   [TGSI_OPCODE_NOT] = nir_op_inot,
   [TGSI_OPCODE_TRUNC] = nir_op_ftrunc,
   [TGSI_OPCODE_SHL] = nir_op_ishl,
   [TGSI_OPCODE_AND] = nir_op_iand,
   [TGSI_OPCODE_OR] = nir_op_ior,
   [TGSI_OPCODE_MOD] = nir_op_umod,
   [TGSI_OPCODE_XOR] = nir_op_ixor,
   [TGSI_OPCODE_TXF] = 0,
                     [TGSI_OPCODE_EMIT] = 0, /* XXX */
            [TGSI_OPCODE_BGNLOOP] = 0,
   [TGSI_OPCODE_BGNSUB] = 0, /* XXX: no function calls */
   [TGSI_OPCODE_ENDLOOP] = 0,
            [TGSI_OPCODE_NOP] = 0,
   [TGSI_OPCODE_FSEQ] = nir_op_feq,
   [TGSI_OPCODE_FSGE] = nir_op_fge,
   [TGSI_OPCODE_FSLT] = nir_op_flt,
                              [TGSI_OPCODE_F2I] = nir_op_f2i32,
   [TGSI_OPCODE_IDIV] = nir_op_idiv,
   [TGSI_OPCODE_IMAX] = nir_op_imax,
   [TGSI_OPCODE_IMIN] = nir_op_imin,
   [TGSI_OPCODE_INEG] = nir_op_ineg,
   [TGSI_OPCODE_ISGE] = nir_op_ige,
   [TGSI_OPCODE_ISHR] = nir_op_ishr,
   [TGSI_OPCODE_ISLT] = nir_op_ilt,
   [TGSI_OPCODE_F2U] = nir_op_f2u32,
   [TGSI_OPCODE_U2F] = nir_op_u2f32,
   [TGSI_OPCODE_UADD] = nir_op_iadd,
   [TGSI_OPCODE_UDIV] = nir_op_udiv,
   [TGSI_OPCODE_UMAD] = 0,
   [TGSI_OPCODE_UMAX] = nir_op_umax,
   [TGSI_OPCODE_UMIN] = nir_op_umin,
   [TGSI_OPCODE_UMOD] = nir_op_umod,
   [TGSI_OPCODE_UMUL] = nir_op_imul,
   [TGSI_OPCODE_USEQ] = nir_op_ieq,
   [TGSI_OPCODE_USGE] = nir_op_uge,
   [TGSI_OPCODE_USHR] = nir_op_ushr,
   [TGSI_OPCODE_USLT] = nir_op_ult,
            [TGSI_OPCODE_SWITCH] = 0, /* not emitted by glsl_to_tgsi.cpp */
   [TGSI_OPCODE_CASE] = 0, /* not emitted by glsl_to_tgsi.cpp */
   [TGSI_OPCODE_DEFAULT] = 0, /* not emitted by glsl_to_tgsi.cpp */
                     [TGSI_OPCODE_UARL] = nir_op_mov,
   [TGSI_OPCODE_UCMP] = 0,
   [TGSI_OPCODE_IABS] = nir_op_iabs,
            [TGSI_OPCODE_LOAD] = 0,
                     [TGSI_OPCODE_TEX2] = 0,
   [TGSI_OPCODE_TXB2] = 0,
            [TGSI_OPCODE_IMUL_HI] = nir_op_imul_high,
            [TGSI_OPCODE_TG4] = 0,
            [TGSI_OPCODE_IBFE] = nir_op_ibitfield_extract,
   [TGSI_OPCODE_UBFE] = nir_op_ubitfield_extract,
   [TGSI_OPCODE_BFI] = nir_op_bitfield_insert,
   [TGSI_OPCODE_BREV] = nir_op_bitfield_reverse,
   [TGSI_OPCODE_POPC] = nir_op_bit_count,
   [TGSI_OPCODE_LSB] = nir_op_find_lsb,
   [TGSI_OPCODE_IMSB] = nir_op_ifind_msb,
            [TGSI_OPCODE_INTERP_CENTROID] = 0, /* XXX */
   [TGSI_OPCODE_INTERP_SAMPLE] = 0, /* XXX */
            [TGSI_OPCODE_F2D] = nir_op_f2f64,
   [TGSI_OPCODE_D2F] = nir_op_f2f32,
   [TGSI_OPCODE_DMUL] = nir_op_fmul,
   [TGSI_OPCODE_D2U] = nir_op_f2u32,
            [TGSI_OPCODE_U64ADD] = nir_op_iadd,
   [TGSI_OPCODE_U64MUL] = nir_op_imul,
   [TGSI_OPCODE_U64DIV] = nir_op_udiv,
   [TGSI_OPCODE_U64SNE] = nir_op_ine,
   [TGSI_OPCODE_I64NEG] = nir_op_ineg,
      };
      static void
   ttn_emit_instruction(struct ttn_compile *c)
   {
      nir_builder *b = &c->build;
   struct tgsi_full_instruction *tgsi_inst = &c->token->FullInstruction;
   unsigned i;
   unsigned tgsi_op = tgsi_inst->Instruction.Opcode;
            if (tgsi_op == TGSI_OPCODE_END)
            nir_def *src[TGSI_FULL_MAX_SRC_REGISTERS];
   for (i = 0; i < tgsi_inst->Instruction.NumSrcRegs; i++) {
                           /* The destination bitsize of the NIR opcode (not TGSI, where it's always
   * 32 bits). This needs to be passed into ttn_alu() because it can't be
   * inferred for comparison opcodes.
   */
            /* If this is non-NULL after the switch, it will be written to the
   * corresponding register/variable/etc after.
   */
            switch (tgsi_op) {
   case TGSI_OPCODE_RSQ:
      dst = nir_frsq(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_SQRT:
      dst = nir_fsqrt(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_RCP:
      dst = nir_frcp(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_EX2:
      dst = nir_fexp2(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_LG2:
      dst = nir_flog2(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_POW:
      dst = nir_fpow(b, ttn_channel(b, src[0], X), ttn_channel(b, src[1], X));
         case TGSI_OPCODE_COS:
      dst = nir_fcos(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_SIN:
      dst = nir_fsin(b, ttn_channel(b, src[0], X));
         case TGSI_OPCODE_ARL:
      dst = nir_f2i32(b, nir_ffloor(b, src[0]));
         case TGSI_OPCODE_EXP:
      dst = ttn_exp(b, src);
         case TGSI_OPCODE_LOG:
      dst = ttn_log(b, src);
         case TGSI_OPCODE_DST:
      dst = ttn_dst(b, src);
         case TGSI_OPCODE_LIT:
      dst = ttn_lit(b, src);
         case TGSI_OPCODE_DP2:
      dst = nir_fdot2(b, src[0], src[1]);
         case TGSI_OPCODE_DP3:
      dst = nir_fdot3(b, src[0], src[1]);
         case TGSI_OPCODE_DP4:
      dst = nir_fdot4(b, src[0], src[1]);
         case TGSI_OPCODE_UMAD:
      dst = nir_iadd(b, nir_imul(b, src[0], src[1]), src[2]);
         case TGSI_OPCODE_LRP:
      dst = nir_flrp(b, src[2], src[1], src[0]);
         case TGSI_OPCODE_KILL:
      ttn_kill(b);
         case TGSI_OPCODE_ARR:
      dst = nir_f2i32(b, nir_fround_even(b, src[0]));
         case TGSI_OPCODE_CMP:
      dst = nir_bcsel(b, nir_flt(b, src[0], nir_imm_float(b, 0.0)),
               case TGSI_OPCODE_UCMP:
      dst = nir_bcsel(b, nir_ine(b, src[0], nir_imm_int(b, 0)),
               case TGSI_OPCODE_SGT:
      dst = nir_slt(b, src[1], src[0]);
         case TGSI_OPCODE_SLE:
      dst = nir_sge(b, src[1], src[0]);
         case TGSI_OPCODE_KILL_IF:
      ttn_kill_if(b, src);
         case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TEX_LZ:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXD:
   case TGSI_OPCODE_TEX2:
   case TGSI_OPCODE_TXL2:
   case TGSI_OPCODE_TXB2:
   case TGSI_OPCODE_TXF:
   case TGSI_OPCODE_TXF_LZ:
   case TGSI_OPCODE_TG4:
   case TGSI_OPCODE_LODQ:
      dst = ttn_tex(c, src);
         case TGSI_OPCODE_TXQ:
      dst = ttn_txq(c, src);
         case TGSI_OPCODE_LOAD:
   case TGSI_OPCODE_STORE:
      dst = ttn_mem(c, src);
         case TGSI_OPCODE_NOP:
            case TGSI_OPCODE_IF:
      nir_push_if(b, nir_fneu_imm(b, nir_channel(b, src[0], 0), 0.0));
         case TGSI_OPCODE_UIF:
      nir_push_if(b, nir_ine_imm(b, nir_channel(b, src[0], 0), 0));
         case TGSI_OPCODE_ELSE:
      nir_push_else(&c->build, NULL);
         case TGSI_OPCODE_ENDIF:
      nir_pop_if(&c->build, NULL);
         case TGSI_OPCODE_BGNLOOP:
      nir_push_loop(&c->build);
         case TGSI_OPCODE_BRK:
      nir_jump(b, nir_jump_break);
         case TGSI_OPCODE_CONT:
      nir_jump(b, nir_jump_continue);
         case TGSI_OPCODE_ENDLOOP:
      nir_pop_loop(&c->build, NULL);
         case TGSI_OPCODE_BARRIER:
      ttn_barrier(b);
         default:
      if (op_trans[tgsi_op] != 0 || tgsi_op == TGSI_OPCODE_MOV) {
         } else {
      fprintf(stderr, "unknown TGSI opcode: %s\n",
            }
               if (dst == NULL)
            if (tgsi_inst->Instruction.Saturate)
            if (dst->num_components == 1)
         else if (dst->num_components == 2)
                     /* Finally, copy the SSA def to the NIR variable/register */
   nir_variable *var = ttn_get_var(c, tgsi_dst);
   if (var) {
      unsigned index = tgsi_dst->Register.Index;
   unsigned offset = c->temp_regs[index].offset;
   struct tgsi_ind_register *indirect = tgsi_dst->Register.Indirect ?
         nir_store_deref(b, ttn_array_deref(c, var, offset, indirect), dst,
      } else {
      unsigned index = tgsi_dst->Register.Index;
   nir_def *reg = NULL;
            if (tgsi_dst->Register.File == TGSI_FILE_TEMPORARY) {
                     reg = c->temp_regs[index].reg;
      } else if (tgsi_dst->Register.File == TGSI_FILE_OUTPUT) {
      reg = c->output_regs[index].reg;
      } else if (tgsi_dst->Register.File == TGSI_FILE_ADDRESS) {
      assert(index == 0);
               if (tgsi_dst->Register.Indirect) {
      nir_def *indirect = ttn_src_for_indirect(c, &tgsi_dst->Indirect);
   nir_store_reg_indirect(b, dst, reg, indirect, .base = base_offset,
      } else {
      nir_build_store_reg(b, dst, reg, .base = base_offset,
            }
      /**
   * Puts a NIR intrinsic to store of each TGSI_FILE_OUTPUT value to the output
   * variables at the end of the shader.
   *
   * We don't generate these incrementally as the TGSI_FILE_OUTPUT values are
   * written, because there's no output load intrinsic, which means we couldn't
   * handle writemasks.
   */
   static void
   ttn_add_output_stores(struct ttn_compile *c)
   {
               for (int i = 0; i < c->build.shader->num_outputs; i++) {
      nir_variable *var = c->outputs[i];
   if (!var)
            nir_def *store_value =
                  uint32_t store_mask = BITFIELD_MASK(store_value->num_components);
   if (c->build.shader->info.stage == MESA_SHADER_FRAGMENT) {
      /* TGSI uses TGSI_SEMANTIC_POSITION.z for the depth output
   * and TGSI_SEMANTIC_STENCIL.y for the stencil output,
   * while NIR uses a single-component output.
   */
   if (var->data.location == FRAG_RESULT_DEPTH)
         else if (var->data.location == FRAG_RESULT_STENCIL)
         else if (var->data.location == FRAG_RESULT_SAMPLE_MASK)
      } else {
      /* FOGC, LAYER, and PSIZ are scalar values */
   if (var->data.location == VARYING_SLOT_FOGC ||
      var->data.location == VARYING_SLOT_LAYER ||
   var->data.location == VARYING_SLOT_PSIZ) {
      }
   if (var->data.location == VARYING_SLOT_CLIP_DIST0)
         else if (var->data.location == VARYING_SLOT_CLIP_DIST1) {
      if (c->build.shader->info.clip_distance_array_size > 4)
         else
                  if (c->cap_compact_arrays &&
      (var->data.location == VARYING_SLOT_CLIP_DIST0 ||
   var->data.location == VARYING_SLOT_CLIP_DIST1)) {
                  nir_deref_instr *deref = nir_build_deref_var(b, c->clipdist);
   nir_def *zero = nir_imm_zero(b, 1, 32);
   unsigned offset = var->data.location == VARYING_SLOT_CLIP_DIST1 ? 4 : 0;
   unsigned size = var->data.location == VARYING_SLOT_CLIP_DIST1 ?
               for (unsigned i = offset; i < size; i++) {
      /* deref the array member and store each component */
   nir_deref_instr *component_deref = nir_build_deref_array_imm(b, deref, i);
   nir_def *val = zero;
   if (store_mask & BITFIELD_BIT(i - offset))
               } else {
               }
      /**
   * Parses the given TGSI tokens.
   */
   static void
   ttn_parse_tgsi(struct ttn_compile *c, const void *tgsi_tokens)
   {
      struct tgsi_parse_context parser;
            ret = tgsi_parse_init(&parser, tgsi_tokens);
            while (!tgsi_parse_end_of_tokens(&parser)) {
      tgsi_parse_token(&parser);
            switch (parser.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_DECLARATION:
                  case TGSI_TOKEN_TYPE_INSTRUCTION:
                  case TGSI_TOKEN_TYPE_IMMEDIATE:
      ttn_emit_immediate(c);
                     }
      static void
   ttn_read_pipe_caps(struct ttn_compile *c,
         {
      c->cap_samplers_as_deref = screen->get_param(screen, PIPE_CAP_NIR_SAMPLERS_AS_DEREF);
   c->cap_face_is_sysval = screen->get_param(screen, PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL);
   c->cap_position_is_sysval = screen->get_param(screen, PIPE_CAP_FS_POSITION_IS_SYSVAL);
   c->cap_point_is_sysval = screen->get_param(screen, PIPE_CAP_FS_POINT_IS_SYSVAL);
   c->cap_integers = screen->get_shader_param(screen, c->scan->processor, PIPE_SHADER_CAP_INTEGERS);
      }
      #define BITSET_SET32(bitset, u32_mask) do { \
      STATIC_ASSERT(sizeof((bitset)[0]) >= sizeof(u32_mask)); \
   BITSET_ZERO(bitset); \
      } while (0)
      /**
   * Initializes a TGSI-to-NIR compiler.
   */
   static struct ttn_compile *
   ttn_compile_init(const void *tgsi_tokens,
               {
      struct ttn_compile *c;
   struct nir_shader *s;
            assert(options || screen);
            tgsi_scan_shader(tgsi_tokens, &scan);
            if (!options) {
      options =
               c->build = nir_builder_init_simple_shader(tgsi_processor_to_shader_stage(scan.processor),
                     if (screen) {
         } else {
      /* TTN used to be hard coded to always make FACE a sysval,
   * so it makes sense to preserve that behavior so users don't break. */
                        if (s->info.stage == MESA_SHADER_FRAGMENT)
            s->num_inputs = scan.file_max[TGSI_FILE_INPUT] + 1;
   s->num_uniforms = scan.const_file_max[0] + 1;
   s->num_outputs = scan.file_max[TGSI_FILE_OUTPUT] + 1;
   s->info.num_ssbos = util_last_bit(scan.shader_buffers_declared);
   s->info.num_ubos = util_last_bit(scan.const_buffers_declared >> 1);
   s->info.num_images = util_last_bit(scan.images_declared);
   BITSET_SET32(s->info.images_used, scan.images_declared);
   BITSET_SET32(s->info.image_buffers, scan.images_buffers);
   BITSET_SET32(s->info.msaa_images, scan.msaa_images_declared);
   s->info.num_textures = util_last_bit(scan.samplers_declared);
   BITSET_SET32(s->info.textures_used, scan.samplers_declared);
   BITSET_ZERO(s->info.textures_used_by_txf); /* No scan information yet */
   BITSET_SET32(s->info.samplers_used, scan.samplers_declared);
            /* Default for TGSI is separate, this is assumed throughout the tree */
            for (unsigned i = 0; i < TGSI_PROPERTY_COUNT; i++) {
               switch (i) {
   case TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS:
         case TGSI_PROPERTY_FS_COORD_ORIGIN:
      if (s->info.stage == MESA_SHADER_FRAGMENT)
            case TGSI_PROPERTY_FS_COORD_PIXEL_CENTER:
      if (s->info.stage == MESA_SHADER_FRAGMENT)
            case TGSI_PROPERTY_FS_DEPTH_LAYOUT:
      if (s->info.stage == MESA_SHADER_FRAGMENT)
            case TGSI_PROPERTY_VS_WINDOW_SPACE_POSITION:
      if (s->info.stage == MESA_SHADER_VERTEX)
            case TGSI_PROPERTY_NEXT_SHADER:
      s->info.next_stage = tgsi_processor_to_shader_stage(value);
      case TGSI_PROPERTY_VS_BLIT_SGPRS_AMD:
      if (s->info.stage == MESA_SHADER_VERTEX)
            case TGSI_PROPERTY_CS_FIXED_BLOCK_WIDTH:
      if (s->info.stage == MESA_SHADER_COMPUTE)
            case TGSI_PROPERTY_CS_FIXED_BLOCK_HEIGHT:
      if (s->info.stage == MESA_SHADER_COMPUTE)
            case TGSI_PROPERTY_CS_FIXED_BLOCK_DEPTH:
      if (s->info.stage == MESA_SHADER_COMPUTE)
            case TGSI_PROPERTY_CS_USER_DATA_COMPONENTS_AMD:
      if (s->info.stage == MESA_SHADER_COMPUTE)
            case TGSI_PROPERTY_NUM_CLIPDIST_ENABLED:
      s->info.clip_distance_array_size = value;
      case TGSI_PROPERTY_LEGACY_MATH_RULES:
      s->info.use_legacy_math_rules = value;
      default:
      if (value) {
      fprintf(stderr, "tgsi_to_nir: unhandled TGSI property %u = %u\n",
                           if (s->info.stage == MESA_SHADER_COMPUTE &&
      (!s->info.workgroup_size[0] ||
   !s->info.workgroup_size[1] ||
   !s->info.workgroup_size[2]))
         c->inputs = rzalloc_array(c, struct nir_variable *, s->num_inputs);
            c->output_regs = rzalloc_array(c, struct ttn_reg_info,
         c->temp_regs = rzalloc_array(c, struct ttn_reg_info,
         c->imm_defs = rzalloc_array(c, nir_def *,
            c->num_samp_types = scan.file_max[TGSI_FILE_SAMPLER_VIEW] + 1;
            ttn_parse_tgsi(c, tgsi_tokens);
                        }
      static void
   ttn_optimize_nir(nir_shader *nir)
   {
               do {
                        /* Linking deals with unused inputs/outputs, but here we can remove
   * things local to the shader in the hopes that we can cleanup other
   * things. This pass will also remove variables with only stores, so we
   * might be able to make progress after it.
   */
   NIR_PASS(progress, nir, nir_remove_dead_variables,
                        NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
            if (nir->options->lower_to_scalar) {
      NIR_PASS_V(nir, nir_lower_alu_to_scalar,
                     NIR_PASS_V(nir, nir_lower_alu);
   NIR_PASS_V(nir, nir_lower_pack);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_dce);
   if (nir_opt_trivial_continues(nir)) {
      progress = true;
   NIR_PASS(progress, nir, nir_copy_prop);
      }
   NIR_PASS(progress, nir, nir_opt_if, nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_cse);
            NIR_PASS(progress, nir, nir_opt_phi_precision);
   NIR_PASS(progress, nir, nir_opt_algebraic);
            if (!nir->info.flrp_lowered) {
      unsigned lower_flrp =
      (nir->options->lower_flrp16 ? 16 : 0) |
                                 NIR_PASS(lower_flrp_progress, nir, nir_lower_flrp,
               if (lower_flrp_progress) {
      NIR_PASS(progress, nir,
                        /* Nothing should rematerialize any flrps, so we only need to do this
   * lowering once.
   */
               NIR_PASS(progress, nir, nir_opt_undef);
   NIR_PASS(progress, nir, nir_opt_conditional_discard);
   if (nir->options->max_unroll_iterations) {
               }
      static bool
   lower_clipdistance_to_array(nir_shader *nir)
   {
      bool progress = false;
   nir_variable *dist0 = nir_find_variable_with_location(nir, nir_var_shader_out, VARYING_SLOT_CLIP_DIST0);
   nir_variable *dist1 = nir_find_variable_with_location(nir, nir_var_shader_out, VARYING_SLOT_CLIP_DIST1);
   /* resize VARYING_SLOT_CLIP_DIST0 to the full array size */
   dist0->type = glsl_array_type(glsl_float_type(), nir->info.clip_distance_array_size, sizeof(float));
   struct set *deletes = _mesa_set_create(NULL, _mesa_hash_pointer, _mesa_key_pointer_equal);
   nir_foreach_function_impl(impl, nir) {
      bool func_progress = false;
   nir_builder b = nir_builder_at(nir_before_impl(impl));
   /* create a new deref for the arrayed clipdistance variable at the start of the function */
   nir_deref_instr *clipdist_deref = nir_build_deref_var(&b, dist0);
   nir_def *zero = nir_imm_zero(&b, 1, 32);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      /* filter through until a clipdistance store is reached */
   if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_deref)
         nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var != dist0 && (!dist1 || var != dist1))
         b.cursor = nir_before_instr(instr);
   uint32_t wrmask = nir_intrinsic_write_mask(intr);
   unsigned offset = var == dist1 ? 4 : 0;
   /* iterate over the store's writemask for components */
   for (unsigned i = 0; i < nir->info.clip_distance_array_size; i++) {
      /* deref the array member and store each component */
   nir_deref_instr *component_deref = nir_build_deref_array_imm(&b, clipdist_deref, i);
   nir_def *val = zero;
   if (wrmask & BITFIELD_BIT(i - offset))
            }
   func_progress = true;
   /* immediately remove the old store, save the original deref */
   nir_instr_remove(instr);
         }
   if (func_progress)
         /* derefs must be queued for deletion to avoid deleting the same deref repeatedly */
   set_foreach_remove(deletes, he)
      }
   /* VARYING_SLOT_CLIP_DIST1 is no longer used and can be removed */
   if (dist1)
            }
      /**
   * Finalizes the NIR in a similar way as st_glsl_to_nir does.
   *
   * Drivers expect that these passes are already performed,
   * so we have to do it here too.
   */
   static void
   ttn_finalize_nir(struct ttn_compile *c, struct pipe_screen *screen)
   {
                        NIR_PASS_V(nir, nir_lower_vars_to_ssa);
            NIR_PASS_V(nir, nir_lower_global_vars_to_local);
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_system_values);
            if (!screen->get_param(screen, PIPE_CAP_TEXRECT)) {
      const struct nir_lower_tex_options opts = { .lower_rect = true, };
               /* driver needs clipdistance as array<float> */
   if ((nir->info.outputs_written &
      (BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0) | BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1))) &&
   screen->get_param(screen, PIPE_CAP_NIR_COMPACT_ARRAYS)) {
               if (nir->options->lower_uniforms_to_ubo)
            if (nir->options->lower_int64_options)
            if (!c->cap_samplers_as_deref)
            if (screen->finalize_nir) {
      char *msg = screen->finalize_nir(screen, nir);
      } else {
      ttn_optimize_nir(nir);
               nir->info.num_images = c->num_images;
               }
      static void save_nir_to_disk_cache(struct disk_cache *cache,
               {
               blob_init(&blob);
   /* Because we cannot fully trust disk_cache_put
   * (EGL_ANDROID_blob_cache) we add the shader size,
   * which we'll check after disk_cache_get().
   */
   if (blob_reserve_uint32(&blob) != 0) {
      blob_finish(&blob);
               nir_serialize(&blob, s, true);
            disk_cache_put(cache, key, blob.data, blob.size, NULL);
      }
      static nir_shader *
   load_nir_from_disk_cache(struct disk_cache *cache,
                     {
      const nir_shader_compiler_options *options =
         struct blob_reader blob_reader;
   size_t size;
            uint32_t *buffer = (uint32_t *)disk_cache_get(cache, key, &size);
   if (!buffer)
            /* Match found. No need to check crc32 or other things.
   * disk_cache_get is supposed to do that for us.
   * However we do still check if the first element is indeed the size,
   * as we cannot fully trust disk_cache_get (EGL_ANDROID_blob_cache) */
   if (buffer[0] != size) {
                  size -= 4;
   blob_reader_init(&blob_reader, buffer + 1, size);
   s = nir_deserialize(NULL, options, &blob_reader);
   free(buffer); /* buffer was malloc-ed */
      }
      struct nir_shader *
   tgsi_to_nir(const void *tgsi_tokens,
               {
      struct disk_cache *cache = NULL;
   struct ttn_compile *c;
   struct nir_shader *s = NULL;
   uint8_t key[CACHE_KEY_SIZE];
            if (allow_disk_cache)
            /* Look first in the cache */
   if (cache) {
      disk_cache_compute_key(cache,
                     processor = tgsi_get_processor_type(tgsi_tokens);
               if (s)
         #ifndef NDEBUG
         #endif
         if (NIR_DEBUG(TGSI)) {
      fprintf(stderr, "TGSI before translation to NIR:\n");
                        c = ttn_compile_init(tgsi_tokens, NULL, screen);
   s = c->build.shader;
   ttn_finalize_nir(c, screen);
            if (NIR_DEBUG(TGSI)) {
      mesa_logi("NIR after translation from TGSI:\n");
               if (cache)
               }
      struct nir_shader *
   tgsi_to_nir_noscreen(const void *tgsi_tokens,
         {
      struct ttn_compile *c;
            c = ttn_compile_init(tgsi_tokens, options, NULL);
   s = c->build.shader;
               }
   