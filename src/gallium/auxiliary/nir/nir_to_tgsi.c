   /*
   * Copyright © 2014-2015 Broadcom
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
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_deref.h"
   #include "compiler/nir/nir_legacy.h"
   #include "compiler/nir/nir_worklist.h"
   #include "nir/nir_to_tgsi.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_from_mesa.h"
   #include "tgsi/tgsi_info.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_ureg.h"
   #include "tgsi/tgsi_util.h"
   #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_dynarray.h"
      struct ntt_insn {
      enum tgsi_opcode opcode;
   struct ureg_dst dst[2];
   struct ureg_src src[4];
   enum tgsi_texture_type tex_target;
   enum tgsi_return_type tex_return_type;
            unsigned mem_qualifier;
            bool is_tex : 1;
   bool is_mem : 1;
      };
      struct ntt_block {
      /* Array of struct ntt_insn */
   struct util_dynarray insns;
   int start_ip;
      };
      struct ntt_reg_interval {
         };
      struct ntt_compile {
      nir_shader *s;
   nir_function_impl *impl;
   const struct nir_to_tgsi_options *options;
   struct pipe_screen *screen;
            bool needs_texcoord_semantic;
   bool native_integers;
            bool addr_declared[3];
            /* if condition set up at the end of a block, for ntt_emit_if(). */
            /* TGSI temps for our NIR SSA and register values. */
   struct ureg_dst *reg_temp;
                     /* Map from nir_block to ntt_block */
   struct hash_table *blocks;
   struct ntt_block *cur_block;
   unsigned current_if_else;
            /* Whether we're currently emitting instructiosn for a precise NIR instruction. */
            unsigned num_temps;
            /* Mappings from driver_location to TGSI input/output number.
   *
   * We'll be declaring TGSI input/outputs in an arbitrary order, and they get
   * their numbers assigned incrementally, unlike inputs or constants.
   */
   struct ureg_src *input_index_map;
            uint32_t first_ubo;
               };
      static struct ureg_dst
   ntt_temp(struct ntt_compile *c)
   {
         }
      static struct ntt_block *
   ntt_block_from_nir(struct ntt_compile *c, struct nir_block *block)
   {
      struct hash_entry *entry = _mesa_hash_table_search(c->blocks, block);
      }
      static void ntt_emit_cf_list(struct ntt_compile *c, struct exec_list *list);
   static void ntt_emit_cf_list_ureg(struct ntt_compile *c, struct exec_list *list);
      static struct ntt_insn *
   ntt_insn(struct ntt_compile *c, enum tgsi_opcode opcode,
            struct ureg_dst dst,
      {
      struct ntt_insn insn = {
      .opcode = opcode,
   .dst = { dst, ureg_dst_undef() },
   .src = { src0, src1, src2, src3 },
      };
   util_dynarray_append(&c->cur_block->insns, struct ntt_insn, insn);
      }
      #define OP00( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c)                                     \
   {                                                                                      \
         }
      #define OP01( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c,                                     \
         {                                                                                      \
         }
         #define OP10( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c,                                     \
         {                                                                                      \
         }
      #define OP11( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c,                                     \
               {                                                                                      \
         }
      #define OP12( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c,                                     \
                     {                                                                                      \
         }
      #define OP13( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c,                                     \
                           {                                                                                      \
         }
      #define OP14( op )                                                                     \
   static inline void ntt_##op(struct ntt_compile *c,                                     \
                        struct ureg_dst dst,                                              \
      {                                                                                      \
         }
      /* We hand-craft our tex instructions */
   #define OP12_TEX(op)
   #define OP14_TEX(op)
      /* Use a template include to generate a correctly-typed ntt_OP()
   * function for each TGSI opcode:
   */
   #include "gallium/auxiliary/tgsi/tgsi_opcode_tmp.h"
      /**
   * Interprets a nir_load_const used as a NIR src as a uint.
   *
   * For non-native-integers drivers, nir_load_const_instrs used by an integer ALU
   * instruction (or in a phi-web used by an integer ALU instruction) were
   * converted to floats and the ALU instruction swapped to the float equivalent.
   * However, this means that integer load_consts used by intrinsics (which don't
   * normally get that conversion) may have been reformatted to be floats.  Given
   * that all of our intrinsic nir_src_as_uint() calls are expected to be small,
   * we can just look and see if they look like floats and convert them back to
   * ints.
   */
   static uint32_t
   ntt_src_as_uint(struct ntt_compile *c, nir_src src)
   {
      uint32_t val = nir_src_as_uint(src);
   if (!c->native_integers && val >= fui(1.0))
            }
      static unsigned
   ntt_64bit_write_mask(unsigned write_mask)
   {
         }
      static struct ureg_src
   ntt_64bit_1f(struct ntt_compile *c)
   {
      return ureg_imm4u(c->ureg,
            }
      /* Per-channel masks of def/use within the block, and the per-channel
   * livein/liveout for the block as a whole.
   */
   struct ntt_live_reg_block_state {
         };
      struct ntt_live_reg_state {
                        /* Used in propagate_across_edge() */
                        };
      static void
   ntt_live_reg_mark_use(struct ntt_compile *c, struct ntt_live_reg_block_state *bs,
         {
               c->liveness[index].start = MIN2(c->liveness[index].start, ip);
         }
   static void
   ntt_live_reg_setup_def_use(struct ntt_compile *c, nir_function_impl *impl, struct ntt_live_reg_state *state)
   {
      for (int i = 0; i < impl->num_blocks; i++) {
      state->blocks[i].def = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].defin = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].defout = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].use = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].livein = rzalloc_array(state->blocks, uint8_t, c->num_temps);
               int ip = 0;
   nir_foreach_block(block, impl) {
      struct ntt_live_reg_block_state *bs = &state->blocks[block->index];
                     util_dynarray_foreach(&ntt_block->insns, struct ntt_insn, insn) {
                     /* Set up use[] for the srcs.
   *
   * Uses are the channels of the reg read in the block that don't have a
   * preceding def to screen them off.  Note that we don't do per-element
   * tracking of array regs, so they're never screened off.
   */
   for (int i = 0; i < opcode_info->num_src; i++) {
      if (insn->src[i].File != TGSI_FILE_TEMPORARY)
                  uint32_t used_mask = tgsi_util_get_src_usage_mask(insn->opcode, i,
                                                assert(!insn->src[i].Indirect || index < c->first_non_array_temp);
               if (insn->is_tex) {
      for (int i = 0; i < ARRAY_SIZE(insn->tex_offset); i++) {
      if (insn->tex_offset[i].File == TGSI_FILE_TEMPORARY)
                  /* Set up def[] for the srcs.
   *
   * Defs are the unconditionally-written (not R/M/W) channels of the reg in
   * the block that don't have a preceding use.
   */
   for (int i = 0; i < opcode_info->num_dst; i++) {
      if (insn->dst[i].File != TGSI_FILE_TEMPORARY)
                                       assert(!insn->dst[i].Indirect || index < c->first_non_array_temp);
   c->liveness[index].start = MIN2(c->liveness[index].start, ip);
      }
                     }
      static void
   ntt_live_regs(struct ntt_compile *c, nir_function_impl *impl)
   {
                        struct ntt_live_reg_state state = {
                  /* The intervals start out with start > end (indicating unused) */
   for (int i = 0; i < c->num_temps; i++)
                     /* Make a forward-order worklist of all the blocks. */
   nir_block_worklist_init(&state.worklist, impl->num_blocks, NULL);
   nir_foreach_block(block, impl) {
                  /* Propagate defin/defout down the CFG to calculate the live variables
   * potentially defined along any possible control flow path.  We'll use this
   * to keep things like conditional defs of the reg (or array regs where we
   * don't track defs!) from making the reg's live range extend back to the
   * start of the program.
   */
   while (!nir_block_worklist_is_empty(&state.worklist)) {
      nir_block *block = nir_block_worklist_pop_head(&state.worklist);
   for (int j = 0; j < ARRAY_SIZE(block->successors); j++) {
      nir_block *succ = block->successors[j];
                                    if (new_def) {
      state.blocks[succ->index].defin[i] |= new_def;
   state.blocks[succ->index].defout[i] |= new_def;
                        /* Make a reverse-order worklist of all the blocks. */
   nir_foreach_block(block, impl) {
                  /* We're now ready to work through the worklist and update the liveness sets
   * of each of the blocks.  As long as we keep the worklist up-to-date as we
   * go, everything will get covered.
   */
   while (!nir_block_worklist_is_empty(&state.worklist)) {
      /* We pop them off in the reverse order we pushed them on.  This way
   * the first walk of the instructions is backwards so we only walk
   * once in the case of no control flow.
   */
   nir_block *block = nir_block_worklist_pop_head(&state.worklist);
   struct ntt_block *ntt_block = ntt_block_from_nir(c, block);
            for (int i = 0; i < c->num_temps; i++) {
      /* Collect livein from our successors to include in our liveout. */
   for (int j = 0; j < ARRAY_SIZE(block->successors); j++) {
      nir_block *succ = block->successors[j];
   if (!succ || succ->index == impl->num_blocks)
                  uint8_t new_liveout = sbs->livein[i] & ~bs->liveout[i];
   if (new_liveout) {
      if (state.blocks[block->index].defout[i])
                        /* Propagate use requests from either our block's uses or our
   * non-screened-off liveout up to our predecessors.
   */
   uint8_t new_livein = ((bs->use[i] | (bs->liveout[i] & ~bs->def[i])) &
         if (new_livein) {
      bs->livein[i] |= new_livein;
   set_foreach(block->predecessors, entry) {
                        if (new_livein & state.blocks[block->index].defin[i])
                     ralloc_free(state.blocks);
      }
      static void
   ntt_ra_check(struct ntt_compile *c, unsigned *ra_map, BITSET_WORD *released, int ip, unsigned index)
   {
      if (index < c->first_non_array_temp)
            if (c->liveness[index].start == ip && ra_map[index] == ~0)
            if (c->liveness[index].end == ip && !BITSET_TEST(released, index)) {
      ureg_release_temporary(c->ureg, ureg_dst_register(TGSI_FILE_TEMPORARY, ra_map[index]));
         }
      static void
   ntt_allocate_regs(struct ntt_compile *c, nir_function_impl *impl)
   {
               unsigned *ra_map = ralloc_array(c, unsigned, c->num_temps);
            /* No RA on NIR array regs */
   for (int i = 0; i < c->first_non_array_temp; i++)
            for (int i = c->first_non_array_temp; i < c->num_temps; i++)
            int ip = 0;
   nir_foreach_block(block, impl) {
               for (int i = 0; i < c->num_temps; i++)
            util_dynarray_foreach(&ntt_block->insns, struct ntt_insn, insn) {
                     for (int i = 0; i < opcode_info->num_src; i++) {
      if (insn->src[i].File == TGSI_FILE_TEMPORARY) {
      ntt_ra_check(c, ra_map, released, ip, insn->src[i].Index);
                  if (insn->is_tex) {
      for (int i = 0; i < ARRAY_SIZE(insn->tex_offset); i++) {
      if (insn->tex_offset[i].File == TGSI_FILE_TEMPORARY) {
      ntt_ra_check(c, ra_map, released, ip, insn->tex_offset[i].Index);
                     for (int i = 0; i < opcode_info->num_dst; i++) {
      if (insn->dst[i].File == TGSI_FILE_TEMPORARY) {
      ntt_ra_check(c, ra_map, released, ip, insn->dst[i].Index);
         }
               for (int i = 0; i < c->num_temps; i++)
         }
      static void
   ntt_allocate_regs_unoptimized(struct ntt_compile *c, nir_function_impl *impl)
   {
      for (int i = c->first_non_array_temp; i < c->num_temps; i++)
      }
         /**
   * Try to find an iadd of a constant value with a non-constant value in the
   * nir_src's first component, returning the constant offset and replacing *src
   * with the non-constant component.
   */
   static const uint32_t
   ntt_extract_const_src_offset(nir_src *src)
   {
               while (nir_scalar_is_alu(s)) {
               if (alu->op == nir_op_iadd) {
      for (int i = 0; i < 2; i++) {
      nir_const_value *v = nir_src_as_const_value(alu->src[i].src);
   if (v != NULL) {
      *src = alu->src[1 - i].src;
                              /* We'd like to reuse nir_scalar_chase_movs(), but it assumes SSA and that
   * seems reasonable for something used in inner loops of the compiler.
   */
   if (alu->op == nir_op_mov) {
      s.def = alu->src[0].src.ssa;
      } else if (nir_op_is_vec(alu->op)) {
      s.def = alu->src[s.comp].src.ssa;
      } else {
                        }
      static const struct glsl_type *
   ntt_shader_input_type(struct ntt_compile *c,
         {
      switch (c->s->info.stage) {
   case MESA_SHADER_GEOMETRY:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_TESS_CTRL:
      if (glsl_type_is_array(var->type))
         else
      default:
            }
      static void
   ntt_get_gl_varying_semantic(struct ntt_compile *c, unsigned location,
         {
      /* We want to use most of tgsi_get_gl_varying_semantic(), but the
   * !texcoord shifting has already been applied, so avoid that.
   */
   if (!c->needs_texcoord_semantic &&
      (location >= VARYING_SLOT_VAR0 && location < VARYING_SLOT_PATCH0)) {
   *semantic_name = TGSI_SEMANTIC_GENERIC;
   *semantic_index = location - VARYING_SLOT_VAR0;
               tgsi_get_gl_varying_semantic(location, true,
      }
      /* TGSI varying declarations have a component usage mask associated (used by
   * r600 and svga).
   */
   static uint32_t
   ntt_tgsi_usage_mask(unsigned start_component, unsigned num_components,
         {
      uint32_t usage_mask =
            if (is_64) {
      if (start_component >= 2)
                     if (usage_mask & TGSI_WRITEMASK_X)
         if (usage_mask & TGSI_WRITEMASK_Y)
               } else {
            }
      /* TGSI varying declarations have a component usage mask associated (used by
   * r600 and svga).
   */
   static uint32_t
   ntt_tgsi_var_usage_mask(const struct nir_variable *var)
   {
      const struct glsl_type *type_without_array =
         unsigned num_components = glsl_get_vector_elements(type_without_array);
   if (num_components == 0) /* structs */
            return ntt_tgsi_usage_mask(var->data.location_frac, num_components,
      }
      static struct ureg_dst
   ntt_output_decl(struct ntt_compile *c, nir_intrinsic_instr *instr, uint32_t *frac)
   {
      nir_io_semantics semantics = nir_intrinsic_io_semantics(instr);
   int base = nir_intrinsic_base(instr);
   *frac = nir_intrinsic_component(instr);
            struct ureg_dst out;
   if (c->s->info.stage == MESA_SHADER_FRAGMENT) {
      unsigned semantic_name, semantic_index;
   tgsi_get_gl_frag_result_semantic(semantics.location,
                  switch (semantics.location) {
   case FRAG_RESULT_DEPTH:
      *frac = 2; /* z write is the to the .z channel in TGSI */
      case FRAG_RESULT_STENCIL:
      *frac = 1;
      default:
                     } else {
               ntt_get_gl_varying_semantic(c, semantics.location,
            uint32_t usage_mask = ntt_tgsi_usage_mask(*frac,
               uint32_t gs_streams = semantics.gs_streams;
   for (int i = 0; i < 4; i++) {
      if (!(usage_mask & (1 << i)))
               /* No driver appears to use array_id of outputs. */
            /* This bit is lost in the i/o semantics, but it's unused in in-tree
   * drivers.
   */
            unsigned num_slots = semantics.num_slots;
   if (semantics.location == VARYING_SLOT_TESS_LEVEL_INNER ||
      semantics.location == VARYING_SLOT_TESS_LEVEL_OUTER) {
   /* Compact vars get a num_slots in NIR as number of components, but we
   * want the number of vec4 slots here.
   */
               out = ureg_DECL_output_layout(c->ureg,
                                 semantic_name, semantic_index,
               unsigned write_mask;
   if (nir_intrinsic_has_write_mask(instr))
         else
            if (is_64) {
      write_mask = ntt_64bit_write_mask(write_mask);
   if (*frac >= 2)
      } else {
         }
      }
      static bool
   ntt_try_store_in_tgsi_output_with_use(struct ntt_compile *c,
               {
               switch (c->s->info.stage) {
   case MESA_SHADER_FRAGMENT:
   case MESA_SHADER_VERTEX:
         default:
      /* tgsi_exec (at least) requires that output stores happen per vertex
   * emitted, you don't get to reuse a previous output value for the next
   * vertex.
   */
               if (nir_src_is_if(src))
            if (nir_src_parent_instr(src)->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(nir_src_parent_instr(src));
   if (intr->intrinsic != nir_intrinsic_store_output ||
      !nir_src_is_const(intr->src[1])) {
               uint32_t frac;
   *dst = ntt_output_decl(c, intr, &frac);
               }
      /* If this reg is used only for storing an output, then in the simple
   * cases we can write directly to the TGSI output instead of having
   * store_output emit its own MOV.
   */
   static bool
   ntt_try_store_reg_in_tgsi_output(struct ntt_compile *c, struct ureg_dst *dst,
         {
                        /* Look for a single use for try_store_in_tgsi_output */
   nir_src *use = NULL;
   nir_foreach_reg_load(src, reg_decl) {
      nir_intrinsic_instr *load = nir_instr_as_intrinsic(nir_src_parent_instr(src));
   nir_foreach_use_including_if(load_use, &load->def) {
      /* We can only have one use */
                                 if (use == NULL)
               }
      /* If this SSA def is used only for storing an output, then in the simple
   * cases we can write directly to the TGSI output instead of having
   * store_output emit its own MOV.
   */
   static bool
   ntt_try_store_ssa_in_tgsi_output(struct ntt_compile *c, struct ureg_dst *dst,
         {
               if (!list_is_singular(&def->uses))
            nir_foreach_use_including_if(use, def) {
         }
      }
      static void
   ntt_setup_inputs(struct ntt_compile *c)
   {
      if (c->s->info.stage != MESA_SHADER_FRAGMENT)
            unsigned num_inputs = 0;
            nir_foreach_shader_in_variable(var, c->s) {
      const struct glsl_type *type = ntt_shader_input_type(c, var);
   unsigned array_len =
                                 nir_foreach_shader_in_variable(var, c->s) {
      const struct glsl_type *type = ntt_shader_input_type(c, var);
   unsigned array_len =
            unsigned interpolation = TGSI_INTERPOLATE_CONSTANT;
   unsigned sample_loc;
            if (c->s->info.stage == MESA_SHADER_FRAGMENT) {
      interpolation =
      tgsi_get_interp_mode(var->data.interpolation,
               if (var->data.location == VARYING_SLOT_POS)
               unsigned semantic_name, semantic_index;
   ntt_get_gl_varying_semantic(c, var->data.location,
            if (var->data.sample) {
         } else if (var->data.centroid) {
      sample_loc = TGSI_INTERPOLATE_LOC_CENTROID;
   c->centroid_inputs |= (BITSET_MASK(array_len) <<
      } else {
                  unsigned array_id = 0;
   if (glsl_type_is_array(type))
                     decl = ureg_DECL_fs_input_centroid_layout(c->ureg,
                                                if (semantic_name == TGSI_SEMANTIC_FACE) {
      struct ureg_dst temp = ntt_temp(c);
   if (c->native_integers) {
      /* NIR is ~0 front and 0 back, while TGSI is +1 front */
      } else {
      /* tgsi docs say that floating point FACE will be positive for
   * frontface and negative for backface, but realistically
   * GLSL-to-TGSI had been doing MOV_SAT to turn it into 0.0 vs 1.0.
   * Copy that behavior, since some drivers (r300) have been doing a
   * 0.0 vs 1.0 backface (and I don't think anybody has a non-1.0
   * front face).
   */
               }
               for (unsigned i = 0; i < array_len; i++) {
      c->input_index_map[var->data.driver_location + i] = decl;
            }
      static int
   ntt_sort_by_location(const nir_variable *a, const nir_variable *b)
   {
         }
      /**
   * Workaround for virglrenderer requiring that TGSI FS output color variables
   * are declared in order.  Besides, it's a lot nicer to read the TGSI this way.
   */
   static void
   ntt_setup_outputs(struct ntt_compile *c)
   {
      if (c->s->info.stage != MESA_SHADER_FRAGMENT)
                     nir_foreach_shader_out_variable(var, c->s) {
      if (var->data.location == FRAG_RESULT_COLOR)
            unsigned semantic_name, semantic_index;
   tgsi_get_gl_frag_result_semantic(var->data.location,
                  }
      static enum tgsi_texture_type
   tgsi_texture_type_from_sampler_dim(enum glsl_sampler_dim dim, bool is_array, bool is_shadow)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
      if (is_shadow)
         else
      case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_EXTERNAL:
      if (is_shadow)
         else
      case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_CUBE:
      if (is_shadow)
         else
      case GLSL_SAMPLER_DIM_RECT:
      if (is_shadow)
         else
      case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_BUF:
         default:
            }
      static enum tgsi_return_type
   tgsi_return_type_from_base_type(enum glsl_base_type type)
   {
      switch (type) {
   case GLSL_TYPE_INT:
         case GLSL_TYPE_UINT:
         case GLSL_TYPE_FLOAT:
   return TGSI_RETURN_TYPE_FLOAT;
   default:
            }
      static void
   ntt_setup_uniforms(struct ntt_compile *c)
   {
      nir_foreach_uniform_variable(var, c->s) {
      if (glsl_type_is_sampler(glsl_without_array(var->type)) ||
      glsl_type_is_texture(glsl_without_array(var->type))) {
   /* Don't use this size for the check for samplers -- arrays of structs
   * containing samplers should be ignored, and just the separate lowered
   * sampler uniform decl used.
   */
                  const struct glsl_type *stype = glsl_without_array(var->type);
   enum tgsi_texture_type target = tgsi_texture_type_from_sampler_dim(glsl_get_sampler_dim(stype),
               enum tgsi_return_type ret_type = tgsi_return_type_from_base_type(glsl_get_sampler_result_type(stype));
   for (int i = 0; i < size; i++) {
      ureg_DECL_sampler_view(c->ureg, var->data.binding + i,
               } else if (glsl_contains_atomic(var->type)) {
      uint32_t offset = var->data.offset / 4;
   uint32_t size = glsl_atomic_size(var->type) / 4;
               /* lower_uniforms_to_ubo lowered non-sampler uniforms to UBOs, so CB0
   * size declaration happens with other UBOs below.
               nir_foreach_image_variable(var, c->s) {
      int image_count = glsl_type_get_image_count(var->type);
   const struct glsl_type *itype = glsl_without_array(var->type);
   enum tgsi_texture_type tex_type =
                  for (int i = 0; i < image_count; i++) {
      c->images[var->data.binding] = ureg_DECL_image(c->ureg,
                                                   unsigned ubo_sizes[PIPE_MAX_CONSTANT_BUFFERS] = {0};
   nir_foreach_variable_with_modes(var, c->s, nir_var_mem_ubo) {
      int ubo = var->data.driver_location;
   if (ubo == -1)
            if (!(ubo == 0 && c->s->info.first_ubo_is_default_ubo))
                     int array_size = 1;
   if (glsl_type_is_interface(glsl_without_array(var->type)))
            for (int i = 0; i < array_size; i++) {
      /* Even if multiple NIR variables are in the same uniform block, their
   * explicit size is the size of the block.
   */
                                 for (int i = 0; i < ARRAY_SIZE(ubo_sizes); i++) {
      if (ubo_sizes[i])
               if (c->options->lower_ssbo_bindings) {
      c->first_ssbo = 255;
   nir_foreach_variable_with_modes(var, c->s, nir_var_mem_ssbo) {
      if (c->first_ssbo > var->data.binding)
         } else
            /* XXX: nv50 uses the atomic flag to set caching for (lowered) atomic
   * counters
   */
   bool atomic = false;
   for (int i = 0; i < c->s->info.num_ssbos; ++i)
         }
      static void
   ntt_setup_registers(struct ntt_compile *c)
   {
               nir_foreach_reg_decl_safe(nir_reg, nir_shader_get_entrypoint(c->s)) {
      /* Permanently allocate all the array regs at the start. */
   unsigned num_array_elems = nir_intrinsic_num_array_elems(nir_reg);
            if (num_array_elems != 0) {
      struct ureg_dst decl = ureg_DECL_array_temporary(c->ureg, num_array_elems, true);
   c->reg_temp[index] = decl;
   assert(c->num_temps == decl.Index);
         }
            /* After that, allocate non-array regs in our virtual space that we'll
   * register-allocate before ureg emit.
   */
   nir_foreach_reg_decl_safe(nir_reg, nir_shader_get_entrypoint(c->s)) {
      unsigned num_array_elems = nir_intrinsic_num_array_elems(nir_reg);
   unsigned num_components = nir_intrinsic_num_components(nir_reg);
   unsigned bit_size = nir_intrinsic_bit_size(nir_reg);
            /* We already handled arrays */
   if (num_array_elems == 0) {
                     if (!ntt_try_store_reg_in_tgsi_output(c, &decl, nir_reg)) {
      if (bit_size == 64) {
      if (num_components > 2) {
                                       }
            }
      static struct ureg_src
   ntt_get_load_const_src(struct ntt_compile *c, nir_load_const_instr *instr)
   {
               if (!c->native_integers) {
      float values[4];
   assert(instr->def.bit_size == 32);
   for (int i = 0; i < num_components; i++)
               } else {
               if (instr->def.bit_size == 32) {
      for (int i = 0; i < num_components; i++)
      } else {
      assert(num_components <= 2);
   for (int i = 0; i < num_components; i++) {
      values[i * 2 + 0] = instr->value[i].u64 & 0xffffffff;
      }
                     }
      static struct ureg_src
   ntt_reladdr(struct ntt_compile *c, struct ureg_src addr, int addr_index)
   {
               for (int i = 0; i <= addr_index; i++) {
      if (!c->addr_declared[i]) {
      c->addr_reg[i] = ureg_writemask(ureg_DECL_address(c->ureg),
                        if (c->native_integers)
         else
            }
      /* Forward declare for recursion with indirects */
   static struct ureg_src
   ntt_get_src(struct ntt_compile *c, nir_src src);
      static struct ureg_src
   ntt_get_chased_src(struct ntt_compile *c, nir_legacy_src *src)
   {
      if (src->is_ssa) {
      if (src->ssa->parent_instr->type == nir_instr_type_load_const)
               } else {
      struct ureg_dst reg_temp = c->reg_temp[src->reg.handle->index];
            if (src->reg.indirect) {
      struct ureg_src offset = ntt_get_src(c, nir_src_for_ssa(src->reg.indirect));
   return ureg_src_indirect(ureg_src(reg_temp),
      } else {
               }
      static struct ureg_src
   ntt_get_src(struct ntt_compile *c, nir_src src)
   {
      nir_legacy_src chased = nir_legacy_chase_src(&src);
      }
      static struct ureg_src
   ntt_get_alu_src(struct ntt_compile *c, nir_alu_instr *instr, int i)
   {
      /* We only support 32-bit float modifiers.  The only other modifier type
   * officially supported by TGSI is 32-bit integer negates, but even those are
   * broken on virglrenderer, so skip lowering all integer and f64 float mods.
   *
   * The options->lower_fabs requests that we not have native source modifiers
   * for fabs, and instead emit MAX(a,-a) for nir_op_fabs.
   */
   nir_legacy_alu_src src =
                  /* Expand double/dvec2 src references to TGSI swizzles using a pair of 32-bit
   * channels.  We skip this for undefs, as those don't get split to vec2s (but
   * the specific swizzles from an undef don't matter)
   */
   if (nir_src_bit_size(instr->src[i].src) == 64 &&
      !(src.src.is_ssa && src.src.ssa->parent_instr->type == nir_instr_type_undef)) {
   int chan1 = 1;
   if (nir_op_infos[instr->op].input_sizes[i] == 0) {
         }
   usrc = ureg_swizzle(usrc,
                        } else {
      usrc = ureg_swizzle(usrc,
                                 if (src.fabs)
         if (src.fneg)
               }
      /* Reswizzles a source so that the unset channels in the write mask still refer
   * to one of the channels present in the write mask.
   */
   static struct ureg_src
   ntt_swizzle_for_write_mask(struct ureg_src src, uint32_t write_mask)
   {
      assert(write_mask);
   int first_chan = ffs(write_mask) - 1;
   return ureg_swizzle(src,
                        }
      static struct ureg_dst
   ntt_get_ssa_def_decl(struct ntt_compile *c, nir_def *ssa)
   {
      uint32_t writemask = BITSET_MASK(ssa->num_components);
   if (ssa->bit_size == 64)
            struct ureg_dst dst;
   if (!ntt_try_store_ssa_in_tgsi_output(c, &dst, ssa))
                        }
      static struct ureg_dst
   ntt_get_chased_dest_decl(struct ntt_compile *c, nir_legacy_dest *dest)
   {
      if (dest->is_ssa)
         else
      }
      static struct ureg_dst
   ntt_get_chased_dest(struct ntt_compile *c, nir_legacy_dest *dest)
   {
               if (!dest->is_ssa) {
               if (dest->reg.indirect) {
      struct ureg_src offset = ntt_get_src(c, nir_src_for_ssa(dest->reg.indirect));
                     }
      static struct ureg_dst
   ntt_get_dest(struct ntt_compile *c, nir_def *def)
   {
      nir_legacy_dest chased = nir_legacy_chase_dest(def);
      }
      static struct ureg_dst
   ntt_get_alu_dest(struct ntt_compile *c, nir_def *def)
   {
      nir_legacy_alu_dest chased = nir_legacy_chase_alu_dest(def);
            if (chased.fsat)
            /* Only registers get write masks */
   if (chased.dest.is_ssa)
            int dst_64 = def->bit_size == 64;
            if (dst_64)
         else
      }
      /* For an SSA dest being populated by a constant src, replace the storage with
   * a copy of the ureg_src.
   */
   static void
   ntt_store_def(struct ntt_compile *c, nir_def *def, struct ureg_src src)
   {
      if (!src.Indirect && !src.DimIndirect) {
      switch (src.File) {
   case TGSI_FILE_IMMEDIATE:
   case TGSI_FILE_INPUT:
   case TGSI_FILE_CONSTANT:
   case TGSI_FILE_SYSTEM_VALUE:
      c->ssa_temp[def->index] = src;
                     }
      static void
   ntt_store(struct ntt_compile *c, nir_def *def, struct ureg_src src)
   {
               if (chased.is_ssa)
         else {
      struct ureg_dst dst = ntt_get_chased_dest(c, &chased);
         }
      static void
   ntt_emit_scalar(struct ntt_compile *c, unsigned tgsi_op,
                     {
               /* POW is the only 2-operand scalar op. */
   if (tgsi_op != TGSI_OPCODE_POW)
            for (i = 0; i < 4; i++) {
      if (dst.WriteMask & (1 << i)) {
      ntt_insn(c, tgsi_op,
            ureg_writemask(dst, 1 << i),
   ureg_scalar(src0, i),
         }
      static void
   ntt_emit_alu(struct ntt_compile *c, nir_alu_instr *instr)
   {
      struct ureg_src src[4];
   struct ureg_dst dst;
   unsigned i;
   int dst_64 = instr->def.bit_size == 64;
   int src_64 = nir_src_bit_size(instr->src[0].src) == 64;
            /* Don't try to translate folded fsat since their source won't be valid */
   if (instr->op == nir_op_fsat && nir_legacy_fsat_folds(instr))
                     assert(num_srcs <= ARRAY_SIZE(src));
   for (i = 0; i < num_srcs; i++)
         for (; i < ARRAY_SIZE(src); i++)
                     static enum tgsi_opcode op_map[][2] = {
               /* fabs/fneg 32-bit are special-cased below. */
   [nir_op_fabs] = { 0, TGSI_OPCODE_DABS },
            [nir_op_fdot2] = { TGSI_OPCODE_DP2 },
   [nir_op_fdot3] = { TGSI_OPCODE_DP3 },
   [nir_op_fdot4] = { TGSI_OPCODE_DP4 },
   [nir_op_fdot2_replicated] = { TGSI_OPCODE_DP2 },
   [nir_op_fdot3_replicated] = { TGSI_OPCODE_DP3 },
   [nir_op_fdot4_replicated] = { TGSI_OPCODE_DP4 },
   [nir_op_ffloor] = { TGSI_OPCODE_FLR, TGSI_OPCODE_DFLR },
   [nir_op_ffract] = { TGSI_OPCODE_FRC, TGSI_OPCODE_DFRAC },
   [nir_op_fceil] = { TGSI_OPCODE_CEIL, TGSI_OPCODE_DCEIL },
   [nir_op_fround_even] = { TGSI_OPCODE_ROUND, TGSI_OPCODE_DROUND },
   [nir_op_fdiv] = { TGSI_OPCODE_DIV, TGSI_OPCODE_DDIV },
   [nir_op_idiv] = { TGSI_OPCODE_IDIV, TGSI_OPCODE_I64DIV },
            [nir_op_frcp] = { 0, TGSI_OPCODE_DRCP },
   [nir_op_frsq] = { 0, TGSI_OPCODE_DRSQ },
            /* The conversions will have one combination of src and dst bitsize. */
   [nir_op_f2f32] = { 0, TGSI_OPCODE_D2F },
   [nir_op_f2f64] = { TGSI_OPCODE_F2D },
            [nir_op_f2i32] = { TGSI_OPCODE_F2I, TGSI_OPCODE_D2I },
   [nir_op_f2i64] = { TGSI_OPCODE_F2I64, TGSI_OPCODE_D2I64 },
   [nir_op_f2u32] = { TGSI_OPCODE_F2U, TGSI_OPCODE_D2U },
   [nir_op_f2u64] = { TGSI_OPCODE_F2U64, TGSI_OPCODE_D2U64 },
   [nir_op_i2f32] = { TGSI_OPCODE_I2F, TGSI_OPCODE_I642F },
   [nir_op_i2f64] = { TGSI_OPCODE_I2D, TGSI_OPCODE_I642D },
   [nir_op_u2f32] = { TGSI_OPCODE_U2F, TGSI_OPCODE_U642F },
            [nir_op_slt] = { TGSI_OPCODE_SLT },
   [nir_op_sge] = { TGSI_OPCODE_SGE },
   [nir_op_seq] = { TGSI_OPCODE_SEQ },
            [nir_op_flt32] = { TGSI_OPCODE_FSLT, TGSI_OPCODE_DSLT },
   [nir_op_fge32] = { TGSI_OPCODE_FSGE, TGSI_OPCODE_DSGE },
   [nir_op_feq32] = { TGSI_OPCODE_FSEQ, TGSI_OPCODE_DSEQ },
            [nir_op_ilt32] = { TGSI_OPCODE_ISLT, TGSI_OPCODE_I64SLT },
   [nir_op_ige32] = { TGSI_OPCODE_ISGE, TGSI_OPCODE_I64SGE },
   [nir_op_ieq32] = { TGSI_OPCODE_USEQ, TGSI_OPCODE_U64SEQ },
            [nir_op_ult32] = { TGSI_OPCODE_USLT, TGSI_OPCODE_U64SLT },
            [nir_op_iabs] = { TGSI_OPCODE_IABS, TGSI_OPCODE_I64ABS },
   [nir_op_ineg] = { TGSI_OPCODE_INEG, TGSI_OPCODE_I64NEG },
   [nir_op_fsign] = { TGSI_OPCODE_SSG, TGSI_OPCODE_DSSG },
   [nir_op_isign] = { TGSI_OPCODE_ISSG, TGSI_OPCODE_I64SSG },
   [nir_op_ftrunc] = { TGSI_OPCODE_TRUNC, TGSI_OPCODE_DTRUNC },
   [nir_op_fddx] = { TGSI_OPCODE_DDX },
   [nir_op_fddy] = { TGSI_OPCODE_DDY },
   [nir_op_fddx_coarse] = { TGSI_OPCODE_DDX },
   [nir_op_fddy_coarse] = { TGSI_OPCODE_DDY },
   [nir_op_fddx_fine] = { TGSI_OPCODE_DDX_FINE },
   [nir_op_fddy_fine] = { TGSI_OPCODE_DDY_FINE },
   [nir_op_pack_half_2x16] = { TGSI_OPCODE_PK2H },
   [nir_op_unpack_half_2x16] = { TGSI_OPCODE_UP2H },
   [nir_op_ibitfield_extract] = { TGSI_OPCODE_IBFE },
   [nir_op_ubitfield_extract] = { TGSI_OPCODE_UBFE },
   [nir_op_bitfield_insert] = { TGSI_OPCODE_BFI },
   [nir_op_bitfield_reverse] = { TGSI_OPCODE_BREV },
   [nir_op_bit_count] = { TGSI_OPCODE_POPC },
   [nir_op_ifind_msb] = { TGSI_OPCODE_IMSB },
   [nir_op_ufind_msb] = { TGSI_OPCODE_UMSB },
   [nir_op_find_lsb] = { TGSI_OPCODE_LSB },
   [nir_op_fadd] = { TGSI_OPCODE_ADD, TGSI_OPCODE_DADD },
   [nir_op_iadd] = { TGSI_OPCODE_UADD, TGSI_OPCODE_U64ADD },
   [nir_op_fmul] = { TGSI_OPCODE_MUL, TGSI_OPCODE_DMUL },
   [nir_op_imul] = { TGSI_OPCODE_UMUL, TGSI_OPCODE_U64MUL },
   [nir_op_imod] = { TGSI_OPCODE_MOD, TGSI_OPCODE_I64MOD },
   [nir_op_umod] = { TGSI_OPCODE_UMOD, TGSI_OPCODE_U64MOD },
   [nir_op_imul_high] = { TGSI_OPCODE_IMUL_HI },
   [nir_op_umul_high] = { TGSI_OPCODE_UMUL_HI },
   [nir_op_ishl] = { TGSI_OPCODE_SHL, TGSI_OPCODE_U64SHL },
   [nir_op_ishr] = { TGSI_OPCODE_ISHR, TGSI_OPCODE_I64SHR },
            /* These bitwise ops don't care about 32 vs 64 types, so they have the
   * same TGSI op.
   */
   [nir_op_inot] = { TGSI_OPCODE_NOT, TGSI_OPCODE_NOT },
   [nir_op_iand] = { TGSI_OPCODE_AND, TGSI_OPCODE_AND },
   [nir_op_ior] = { TGSI_OPCODE_OR, TGSI_OPCODE_OR },
            [nir_op_fmin] = { TGSI_OPCODE_MIN, TGSI_OPCODE_DMIN },
   [nir_op_imin] = { TGSI_OPCODE_IMIN, TGSI_OPCODE_I64MIN },
   [nir_op_umin] = { TGSI_OPCODE_UMIN, TGSI_OPCODE_U64MIN },
   [nir_op_fmax] = { TGSI_OPCODE_MAX, TGSI_OPCODE_DMAX },
   [nir_op_imax] = { TGSI_OPCODE_IMAX, TGSI_OPCODE_I64MAX },
   [nir_op_umax] = { TGSI_OPCODE_UMAX, TGSI_OPCODE_U64MAX },
   [nir_op_ffma] = { TGSI_OPCODE_MAD, TGSI_OPCODE_DMAD },
               if (src_64 && !dst_64) {
      if (num_srcs == 2 || nir_op_infos[instr->op].output_type == nir_type_bool32) {
      /* TGSI's 64 bit compares storing to 32-bit are weird and write .xz instead
   * of .xy.
   */
      } else {
      /* TGSI 64bit-to-32-bit conversions only generate results in the .xy
   * channels and will need to get fixed up.
      assert(!(dst.WriteMask & TGSI_WRITEMASK_ZW));
               bool table_op64 = src_64;
   if (instr->op < ARRAY_SIZE(op_map) && op_map[instr->op][table_op64] != 0) {
      /* The normal path for NIR to TGSI ALU op translation */
   ntt_insn(c, op_map[instr->op][table_op64],
      } else {
               /* TODO: Use something like the ntt_store() path for the MOV calls so we
   * don't emit extra MOVs for swizzles/srcmods of inputs/const/imm.
            switch (instr->op) {
   case nir_op_u2u64:
      ntt_AND(c, dst, ureg_swizzle(src[0],
                           case nir_op_i2i32:
   case nir_op_u2u32:
      assert(src_64);
   ntt_MOV(c, dst, ureg_swizzle(src[0],
                     case nir_op_fabs:
      /* Try to eliminate */
                  if (c->options->lower_fabs)
         else
               case nir_op_fsat:
      if (dst_64) {
      ntt_MIN(c, dst, src[0], ntt_64bit_1f(c));
      } else {
                     case nir_op_fneg:
      /* Try to eliminate */
                                 /* NOTE: TGSI 32-bit math ops have the old "one source channel
   * replicated to all dst channels" behavior, while 64 is normal mapping
   * of src channels to dst.
      case nir_op_frcp:
      assert(!dst_64);
               case nir_op_frsq:
      assert(!dst_64);
               case nir_op_fsqrt:
      assert(!dst_64);
               case nir_op_fexp2:
      assert(!dst_64);
               case nir_op_flog2:
      assert(!dst_64);
               case nir_op_b2f32:
                  case nir_op_b2f64:
      ntt_AND(c, dst,
            ureg_swizzle(src[0],
                  case nir_op_b2i32:
                  case nir_op_b2i64:
      ntt_AND(c, dst,
            ureg_swizzle(src[0],
                  case nir_op_fsin:
                  case nir_op_fcos:
                  case nir_op_fsub:
      assert(!dst_64);
               case nir_op_isub:
      assert(!dst_64);
               case nir_op_fmod:
                  case nir_op_fpow:
                  case nir_op_flrp:
                  case nir_op_pack_64_2x32_split:
      ntt_MOV(c, ureg_writemask(dst, TGSI_WRITEMASK_XZ),
            ureg_swizzle(src[0],
      ntt_MOV(c, ureg_writemask(dst, TGSI_WRITEMASK_YW),
            ureg_swizzle(src[1],
            case nir_op_unpack_64_2x32_split_x:
      ntt_MOV(c, dst, ureg_swizzle(src[0],
                     case nir_op_unpack_64_2x32_split_y:
      ntt_MOV(c, dst, ureg_swizzle(src[0],
                     case nir_op_b32csel:
      if (nir_src_bit_size(instr->src[1].src) == 64) {
      ntt_UCMP(c, dst, ureg_swizzle(src[0],
                  } else {
                     case nir_op_fcsel:
      /* If CMP isn't supported, then the flags that enable NIR to generate
   * this opcode should also not be set.
                  /* Implement this as CMP(-abs(src0), src1, src2). */
               case nir_op_fcsel_gt:
      /* If CMP isn't supported, then the flags that enable NIR to generate
   * these opcodes should also not be set.
                              case nir_op_fcsel_ge:
      /* If CMP isn't supported, then the flags that enable NIR to generate
   * these opcodes should also not be set.
                  /* Implement this as if !(src0 < 0.0) was identical to src0 >= 0.0. */
               case nir_op_frexp_sig:
   case nir_op_frexp_exp:
                  case nir_op_ldexp:
      assert(dst_64); /* 32bit handled in table. */
   ntt_DLDEXP(c, dst, src[0],
                           case nir_op_vec4:
   case nir_op_vec3:
   case nir_op_vec2:
            default:
      fprintf(stderr, "Unknown NIR opcode: %s\n", nir_op_infos[instr->op].name);
                     }
      static struct ureg_src
   ntt_ureg_src_indirect(struct ntt_compile *c, struct ureg_src usrc,
         {
      if (nir_src_is_const(src)) {
      usrc.Index += ntt_src_as_uint(c, src);
      } else {
            }
      static struct ureg_dst
   ntt_ureg_dst_indirect(struct ntt_compile *c, struct ureg_dst dst,
         {
      if (nir_src_is_const(src)) {
      dst.Index += ntt_src_as_uint(c, src);
      } else {
            }
      static struct ureg_src
   ntt_ureg_src_dimension_indirect(struct ntt_compile *c, struct ureg_src usrc,
         {
      if (nir_src_is_const(src)) {
         }
   else
   {
      return ureg_src_dimension_indirect(usrc,
               }
      static struct ureg_dst
   ntt_ureg_dst_dimension_indirect(struct ntt_compile *c, struct ureg_dst udst,
         {
      if (nir_src_is_const(src)) {
         } else {
      return ureg_dst_dimension_indirect(udst,
               }
   /* Some load operations in NIR will have a fractional offset that we need to
   * swizzle down before storing to the result register.
   */
   static struct ureg_src
   ntt_shift_by_frac(struct ureg_src src, unsigned frac, unsigned num_components)
   {
      return ureg_swizzle(src,
                        }
         static void
   ntt_emit_load_ubo(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
      int bit_size = instr->def.bit_size;
                              if (nir_src_is_const(instr->src[0])) {
         } else {
      /* virglrenderer requires that indirect UBO references have the UBO
   * array's base index in the Index field, not added to the indrect
   * address.
   *
   * Many nir intrinsics have a base address const value for the start of
   * their array indirection, but load_ubo doesn't.  We fake it by
   * subtracting it off here.
   */
   addr_temp = ntt_temp(c);
   ntt_UADD(c, addr_temp, ntt_get_src(c, instr->src[0]), ureg_imm1i(c->ureg, -c->first_ubo));
   src = ureg_src_dimension_indirect(src,
                     if (instr->intrinsic == nir_intrinsic_load_ubo_vec4) {
      /* !PIPE_CAP_LOAD_CONSTBUF: Just emit it as a vec4 reference to the const
   * file.
   */
            if (nir_src_is_const(instr->src[1])) {
         } else {
                  int start_component = nir_intrinsic_component(instr);
   if (bit_size == 64)
            src = ntt_shift_by_frac(src, start_component,
               } else {
      /* PIPE_CAP_LOAD_CONSTBUF: Not necessarily vec4 aligned, emit a
   * TGSI_OPCODE_LOAD instruction from the const file.
   */
   struct ntt_insn *insn =
      ntt_insn(c, TGSI_OPCODE_LOAD,
            ntt_get_dest(c, &instr->def),
   insn->is_mem = true;
   insn->tex_target = 0;
   insn->mem_qualifier = 0;
         }
      static unsigned
   ntt_get_access_qualifier(nir_intrinsic_instr *instr)
   {
      enum gl_access_qualifier access = nir_intrinsic_access(instr);
            if (access & ACCESS_COHERENT)
         if (access & ACCESS_VOLATILE)
         if (access & ACCESS_RESTRICT)
               }
      static unsigned
   ntt_translate_atomic_op(nir_atomic_op op)
   {
      switch (op) {
   case nir_atomic_op_iadd: return TGSI_OPCODE_ATOMUADD;
   case nir_atomic_op_fadd: return TGSI_OPCODE_ATOMFADD;
   case nir_atomic_op_imin: return TGSI_OPCODE_ATOMIMIN;
   case nir_atomic_op_imax: return TGSI_OPCODE_ATOMIMAX;
   case nir_atomic_op_umin: return TGSI_OPCODE_ATOMUMIN;
   case nir_atomic_op_umax: return TGSI_OPCODE_ATOMUMAX;
   case nir_atomic_op_iand: return TGSI_OPCODE_ATOMAND;
   case nir_atomic_op_ixor: return TGSI_OPCODE_ATOMXOR;
   case nir_atomic_op_ior:  return TGSI_OPCODE_ATOMOR;
   case nir_atomic_op_xchg: return TGSI_OPCODE_ATOMXCHG;
   default: unreachable("invalid atomic");
      }
      static void
   ntt_emit_mem(struct ntt_compile *c, nir_intrinsic_instr *instr,
         {
      bool is_store = (instr->intrinsic == nir_intrinsic_store_ssbo ||
         bool is_load = (instr->intrinsic == nir_intrinsic_atomic_counter_read ||
               unsigned opcode;
   struct ureg_src src[4];
   int num_src = 0;
   int next_src;
            struct ureg_src memory;
   switch (mode) {
   case nir_var_mem_ssbo:
      memory = ntt_ureg_src_indirect(c, ureg_src_register(TGSI_FILE_BUFFER,
               next_src = 1;
      case nir_var_mem_shared:
      memory = ureg_src_register(TGSI_FILE_MEMORY, 0);
   next_src = 0;
      case nir_var_uniform: { /* HW atomic buffers */
      nir_src src = instr->src[0];
   uint32_t offset = (ntt_extract_const_src_offset(&src) +
            memory = ureg_src_register(TGSI_FILE_HW_ATOMIC, offset);
   /* ntt_ureg_src_indirect, except dividing by 4 */
   if (nir_src_is_const(src)) {
         } else {
      addr_temp = ntt_temp(c);
   ntt_USHR(c, addr_temp, ntt_get_src(c, src), ureg_imm1i(c->ureg, 2));
      }
   memory = ureg_src_dimension(memory, nir_intrinsic_base(instr));
   next_src = 0;
               default:
                  if (is_store) {
      src[num_src++] = ntt_get_src(c, instr->src[next_src + 1]); /* offset */
      } else {
      src[num_src++] = memory;
   if (instr->intrinsic != nir_intrinsic_get_ssbo_size) {
      src[num_src++] = ntt_get_src(c, instr->src[next_src++]); /* offset */
   switch (instr->intrinsic) {
   case nir_intrinsic_atomic_counter_inc:
      src[num_src++] = ureg_imm1i(c->ureg, 1);
      case nir_intrinsic_atomic_counter_post_dec:
      src[num_src++] = ureg_imm1i(c->ureg, -1);
      default:
      if (!is_load)
                              switch (instr->intrinsic) {
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_shared_atomic:
      opcode = ntt_translate_atomic_op(nir_intrinsic_atomic_op(instr));
      case nir_intrinsic_atomic_counter_add:
   case nir_intrinsic_atomic_counter_inc:
   case nir_intrinsic_atomic_counter_post_dec:
      opcode = TGSI_OPCODE_ATOMUADD;
      case nir_intrinsic_atomic_counter_min:
      opcode = TGSI_OPCODE_ATOMIMIN;
      case nir_intrinsic_atomic_counter_max:
      opcode = TGSI_OPCODE_ATOMIMAX;
      case nir_intrinsic_atomic_counter_and:
      opcode = TGSI_OPCODE_ATOMAND;
      case nir_intrinsic_atomic_counter_or:
      opcode = TGSI_OPCODE_ATOMOR;
      case nir_intrinsic_atomic_counter_xor:
      opcode = TGSI_OPCODE_ATOMXOR;
      case nir_intrinsic_atomic_counter_exchange:
      opcode = TGSI_OPCODE_ATOMXCHG;
      case nir_intrinsic_atomic_counter_comp_swap:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_shared_atomic_swap:
      opcode = TGSI_OPCODE_ATOMCAS;
   src[num_src++] = ntt_get_src(c, instr->src[next_src++]);
      case nir_intrinsic_atomic_counter_read:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_shared:
      opcode = TGSI_OPCODE_LOAD;
      case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
      opcode = TGSI_OPCODE_STORE;
      case nir_intrinsic_get_ssbo_size:
      opcode = TGSI_OPCODE_RESQ;
      default:
                  unsigned qualifier = 0;
   if (mode == nir_var_mem_ssbo &&
      instr->intrinsic != nir_intrinsic_get_ssbo_size) {
               struct ureg_dst dst;
   if (is_store) {
               unsigned write_mask = nir_intrinsic_write_mask(instr);
   if (nir_src_bit_size(instr->src[0]) == 64)
            } else {
                  struct ntt_insn *insn = ntt_insn(c, opcode, dst, src[0], src[1], src[2], src[3]);
   insn->tex_target = TGSI_TEXTURE_BUFFER;
   insn->mem_qualifier = qualifier;
   insn->mem_format = 0; /* unused */
      }
      static void
   ntt_emit_image_load_store(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
      unsigned op;
   struct ureg_src srcs[4];
   int num_src = 0;
   enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
                              struct ureg_src resource;
   switch (instr->intrinsic) {
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
      resource = ntt_get_src(c, instr->src[0]);
      default:
      resource = ntt_ureg_src_indirect(c, ureg_src_register(TGSI_FILE_IMAGE, 0),
                     struct ureg_dst dst;
   if (instr->intrinsic == nir_intrinsic_image_store ||
      instr->intrinsic == nir_intrinsic_bindless_image_store) {
      } else {
      srcs[num_src++] = resource;
      }
            if (instr->intrinsic != nir_intrinsic_image_size &&
      instr->intrinsic != nir_intrinsic_image_samples &&
   instr->intrinsic != nir_intrinsic_bindless_image_size &&
   instr->intrinsic != nir_intrinsic_bindless_image_samples) {
            if (dim == GLSL_SAMPLER_DIM_MS) {
      temp = ntt_temp(c);
   ntt_MOV(c, temp, coord);
   ntt_MOV(c, ureg_writemask(temp, TGSI_WRITEMASK_W),
            }
            if (instr->intrinsic != nir_intrinsic_image_load &&
      instr->intrinsic != nir_intrinsic_bindless_image_load) {
   srcs[num_src++] = ntt_get_src(c, instr->src[3]); /* data */
   if (instr->intrinsic == nir_intrinsic_image_atomic_swap ||
      instr->intrinsic == nir_intrinsic_bindless_image_atomic_swap)
               switch (instr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_bindless_image_load:
      op = TGSI_OPCODE_LOAD;
      case nir_intrinsic_image_store:
   case nir_intrinsic_bindless_image_store:
      op = TGSI_OPCODE_STORE;
      case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size:
      op = TGSI_OPCODE_RESQ;
      case nir_intrinsic_image_samples:
   case nir_intrinsic_bindless_image_samples:
      op = TGSI_OPCODE_RESQ;
   opcode_dst = ureg_writemask(ntt_temp(c), TGSI_WRITEMASK_W);
      case nir_intrinsic_image_atomic:
   case nir_intrinsic_bindless_image_atomic:
      op = ntt_translate_atomic_op(nir_intrinsic_atomic_op(instr));
      case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_atomic_swap:
      op = TGSI_OPCODE_ATOMCAS;
      default:
                  struct ntt_insn *insn = ntt_insn(c, op, opcode_dst, srcs[0], srcs[1], srcs[2], srcs[3]);
   insn->tex_target = target;
   insn->mem_qualifier = ntt_get_access_qualifier(instr);
   insn->mem_format = nir_intrinsic_format(instr);
            if (instr->intrinsic == nir_intrinsic_image_samples ||
      instr->intrinsic == nir_intrinsic_bindless_image_samples)
   }
      static void
   ntt_emit_load_input(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
      uint32_t frac = nir_intrinsic_component(instr);
   uint32_t num_components = instr->num_components;
   unsigned base = nir_intrinsic_base(instr);
   struct ureg_src input;
   nir_io_semantics semantics = nir_intrinsic_io_semantics(instr);
            if (c->s->info.stage == MESA_SHADER_VERTEX) {
      input = ureg_DECL_vs_input(c->ureg, base);
   for (int i = 1; i < semantics.num_slots; i++)
      } else if (c->s->info.stage != MESA_SHADER_FRAGMENT) {
      unsigned semantic_name, semantic_index;
   ntt_get_gl_varying_semantic(c, semantics.location,
            /* XXX: ArrayID is used in r600 gs inputs */
            input = ureg_DECL_input_layout(c->ureg,
                                 semantic_name,
   semantic_index,
      } else {
                  if (is_64)
                     switch (instr->intrinsic) {
   case nir_intrinsic_load_input:
      input = ntt_ureg_src_indirect(c, input, instr->src[0], 0);
   ntt_store(c, &instr->def, input);
         case nir_intrinsic_load_per_vertex_input:
      input = ntt_ureg_src_indirect(c, input, instr->src[1], 0);
   input = ntt_ureg_src_dimension_indirect(c, input, instr->src[0]);
   ntt_store(c, &instr->def, input);
         case nir_intrinsic_load_interpolated_input: {
               nir_intrinsic_instr *bary_instr =
            switch (bary_instr->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_sample:
      /* For these, we know that the barycentric load matches the
   * interpolation on the input declaration, so we can use it directly.
   */
               case nir_intrinsic_load_barycentric_centroid:
      /* If the input was declared centroid, then there's no need to
   * emit the extra TGSI interp instruction, we can just read the
   * input.
   */
   if (c->centroid_inputs & (1ull << nir_intrinsic_base(instr))) {
         } else {
                     case nir_intrinsic_load_barycentric_at_sample:
      /* We stored the sample in the fake "bary" dest. */
   ntt_INTERP_SAMPLE(c, ntt_get_dest(c, &instr->def), input,
               case nir_intrinsic_load_barycentric_at_offset:
      /* We stored the offset in the fake "bary" dest. */
   ntt_INTERP_OFFSET(c, ntt_get_dest(c, &instr->def), input,
               default:
         }
               default:
            }
      static void
   ntt_emit_store_output(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
               if (src.File == TGSI_FILE_OUTPUT) {
      /* If our src is the output file, that's an indication that we were able
   * to emit the output stores in the generating instructions and we have
   * nothing to do here.
   */
               uint32_t frac;
            if (instr->intrinsic == nir_intrinsic_store_per_vertex_output) {
      out = ntt_ureg_dst_indirect(c, out, instr->src[2]);
      } else {
                  uint8_t swizzle[4] = { 0, 0, 0, 0 };
   for (int i = frac; i <= 4; i++) {
      if (out.WriteMask & (1 << i))
                           }
      static void
   ntt_emit_load_output(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
               /* ntt_try_store_in_tgsi_output() optimization is not valid if normal
   * load_output is present.
   */
   assert(c->s->info.stage != MESA_SHADER_VERTEX &&
            uint32_t frac;
            if (instr->intrinsic == nir_intrinsic_load_per_vertex_output) {
      out = ntt_ureg_dst_indirect(c, out, instr->src[1]);
      } else {
                  struct ureg_dst dst = ntt_get_dest(c, &instr->def);
            /* Don't swizzling unavailable channels of the output in the writemasked-out
   * components. Avoids compile failures in virglrenderer with
   * TESS_LEVEL_INNER.
   */
   int fill_channel = ffs(dst.WriteMask) - 1;
   uint8_t swizzles[4] = { 0, 1, 2, 3 };
   for (int i = 0; i < 4; i++)
      if (!(dst.WriteMask & (1 << i)))
               if (semantics.fb_fetch_output)
         else
      }
      static void
   ntt_emit_load_sysval(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
      gl_system_value sysval = nir_system_value_from_intrinsic(instr->intrinsic);
   enum tgsi_semantic semantic = tgsi_get_sysval_semantic(sysval);
            /* virglrenderer doesn't like references to channels of the sysval that
   * aren't defined, even if they aren't really read.  (GLSL compile fails on
   * gl_NumWorkGroups.w, for example).
   */
   uint32_t write_mask = BITSET_MASK(instr->def.num_components);
            /* TGSI and NIR define these intrinsics as always loading ints, but they can
   * still appear on hardware with non-native-integers fragment shaders using
   * the draw path (i915g).  In that case, having called nir_lower_int_to_float
   * means that we actually want floats instead.
   */
   if (!c->native_integers) {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_instance_id:
                  default:
                        }
      static void
   ntt_emit_barrier(struct ntt_compile *c, nir_intrinsic_instr *intr)
   {
               if (nir_intrinsic_memory_scope(intr) != SCOPE_NONE) {
      nir_variable_mode modes = nir_intrinsic_memory_modes(intr);
            if (modes & nir_var_image)
            if (modes & nir_var_mem_shared)
            /* Atomic counters are lowered to SSBOs, there's no NIR mode corresponding
   * exactly to atomics. Take the closest match.
   */
   if (modes & nir_var_mem_ssbo)
            if (modes & nir_var_mem_global)
            /* Hack for virglrenderer: the GLSL specific memory barrier functions,
   * memoryBarrier{Buffer,Image,Shared,AtomicCounter}(), are only
   * available in compute shaders prior to GLSL 4.30.  In other stages,
   * it needs to use the full memoryBarrier().  It may be possible to
   * make them available via #extension directives in older versions,
   * but it's confusingly underspecified, and Mesa/virglrenderer don't
   * currently agree on how to do it.  So, just promote partial memory
   * barriers back to full ones outside of compute shaders when asked.
   */
   if (membar && !compute &&
      c->options->non_compute_membar_needs_all_modes) {
   membar |= TGSI_MEMBAR_SHADER_BUFFER |
            TGSI_MEMBAR_ATOMIC_BUFFER |
            /* If we only need workgroup scope (not device-scope), we might be able to
   * optimize a bit.
   */
   if (membar && compute &&
                           /* Only emit a memory barrier if there are any relevant modes */
   if (membar)
               if (nir_intrinsic_execution_scope(intr) != SCOPE_NONE) {
      assert(compute || c->s->info.stage == MESA_SHADER_TESS_CTRL);
         }
      static void
   ntt_emit_intrinsic(struct ntt_compile *c, nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
      ntt_emit_load_ubo(c, instr);
               case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_invocation_id:
   case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_point_coord:
   case nir_intrinsic_load_front_face:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_sample_mask_in:
   case nir_intrinsic_load_helper_invocation:
   case nir_intrinsic_load_tess_coord:
   case nir_intrinsic_load_patch_vertices_in:
   case nir_intrinsic_load_primitive_id:
   case nir_intrinsic_load_tess_level_outer:
   case nir_intrinsic_load_tess_level_inner:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_workgroup_size:
   case nir_intrinsic_load_subgroup_size:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_load_subgroup_eq_mask:
   case nir_intrinsic_load_subgroup_ge_mask:
   case nir_intrinsic_load_subgroup_gt_mask:
   case nir_intrinsic_load_subgroup_lt_mask:
   case nir_intrinsic_load_subgroup_le_mask:
      ntt_emit_load_sysval(c, instr);
         case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
      ntt_emit_load_input(c, instr);
         case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      ntt_emit_store_output(c, instr);
         case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
      ntt_emit_load_output(c, instr);
         case nir_intrinsic_demote:
      ntt_DEMOTE(c);
         case nir_intrinsic_discard:
      ntt_KILL(c);
         case nir_intrinsic_discard_if: {
               if (c->native_integers) {
      struct ureg_dst temp = ureg_writemask(ntt_temp(c), 1);
   ntt_AND(c, temp, cond, ureg_imm1f(c->ureg, 1.0));
      } else {
      /* For !native_integers, the bool got lowered to 1.0 or 0.0. */
      }
               case nir_intrinsic_is_helper_invocation:
      ntt_READ_HELPER(c, ntt_get_dest(c, &instr->def));
         case nir_intrinsic_vote_all:
      ntt_VOTE_ALL(c, ntt_get_dest(c, &instr->def), ntt_get_src(c,instr->src[0]));
      case nir_intrinsic_vote_any:
      ntt_VOTE_ANY(c, ntt_get_dest(c, &instr->def), ntt_get_src(c, instr->src[0]));
      case nir_intrinsic_vote_ieq:
      ntt_VOTE_EQ(c, ntt_get_dest(c, &instr->def), ntt_get_src(c, instr->src[0]));
      case nir_intrinsic_ballot:
      ntt_BALLOT(c, ntt_get_dest(c, &instr->def), ntt_get_src(c, instr->src[0]));
      case nir_intrinsic_read_first_invocation:
      ntt_READ_FIRST(c, ntt_get_dest(c, &instr->def), ntt_get_src(c, instr->src[0]));
      case nir_intrinsic_read_invocation:
      ntt_READ_INVOC(c, ntt_get_dest(c, &instr->def), ntt_get_src(c, instr->src[0]), ntt_get_src(c, instr->src[1]));
         case nir_intrinsic_load_ssbo:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_get_ssbo_size:
      ntt_emit_mem(c, instr, nir_var_mem_ssbo);
         case nir_intrinsic_load_shared:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      ntt_emit_mem(c, instr, nir_var_mem_shared);
         case nir_intrinsic_atomic_counter_read:
   case nir_intrinsic_atomic_counter_add:
   case nir_intrinsic_atomic_counter_inc:
   case nir_intrinsic_atomic_counter_post_dec:
   case nir_intrinsic_atomic_counter_min:
   case nir_intrinsic_atomic_counter_max:
   case nir_intrinsic_atomic_counter_and:
   case nir_intrinsic_atomic_counter_or:
   case nir_intrinsic_atomic_counter_xor:
   case nir_intrinsic_atomic_counter_exchange:
   case nir_intrinsic_atomic_counter_comp_swap:
      ntt_emit_mem(c, instr, nir_var_uniform);
      case nir_intrinsic_atomic_counter_pre_dec:
      unreachable("Should be lowered by ntt_lower_atomic_pre_dec()");
         case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_samples:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
      ntt_emit_image_load_store(c, instr);
         case nir_intrinsic_barrier:
      ntt_emit_barrier(c, instr);
         case nir_intrinsic_end_primitive:
      ntt_ENDPRIM(c, ureg_imm1u(c->ureg, nir_intrinsic_stream_id(instr)));
         case nir_intrinsic_emit_vertex:
      ntt_EMIT(c, ureg_imm1u(c->ureg, nir_intrinsic_stream_id(instr)));
            /* In TGSI we don't actually generate the barycentric coords, and emit
   * interp intrinsics later.  However, we do need to store the
   * load_barycentric_at_* argument so that we can use it at that point.
      case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
         case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
      ntt_store(c, &instr->def, ntt_get_src(c, instr->src[0]));
         case nir_intrinsic_shader_clock:
      ntt_CLOCK(c, ntt_get_dest(c, &instr->def));
         case nir_intrinsic_decl_reg:
   case nir_intrinsic_load_reg:
   case nir_intrinsic_load_reg_indirect:
   case nir_intrinsic_store_reg:
   case nir_intrinsic_store_reg_indirect:
      /* fully consumed */
         default:
      fprintf(stderr, "Unknown intrinsic: ");
   nir_print_instr(&instr->instr, stderr);
   fprintf(stderr, "\n");
         }
      struct ntt_tex_operand_state {
      struct ureg_src srcs[4];
      };
      static void
   ntt_push_tex_arg(struct ntt_compile *c,
                     {
      int tex_src = nir_tex_instr_src_index(instr, tex_src_type);
   if (tex_src < 0)
                     /* virglrenderer workaround that's hard to do in tgsi_translate: Make sure
   * that TG4's immediate offset arg is float-typed.
   */
   if (instr->op == nir_texop_tg4 && tex_src_type == nir_tex_src_backend2 &&
      nir_src_is_const(*src)) {
   nir_const_value *consts = nir_src_as_const_value(*src);
   s->srcs[s->i++] = ureg_imm4f(c->ureg,
                                          }
      static void
   ntt_emit_texture(struct ntt_compile *c, nir_tex_instr *instr)
   {
      struct ureg_dst dst = ntt_get_dest(c, &instr->def);
   enum tgsi_texture_type target = tgsi_texture_type_from_sampler_dim(instr->sampler_dim, instr->is_array, instr->is_shadow);
            int tex_handle_src = nir_tex_instr_src_index(instr, nir_tex_src_texture_handle);
            struct ureg_src sampler;
   if (tex_handle_src >= 0 && sampler_handle_src >= 0) {
      /* It seems we can't get separate tex/sampler on GL, just use one of the handles */
   sampler = ntt_get_src(c, instr->src[tex_handle_src].src);
      } else {
      assert(tex_handle_src == -1 && sampler_handle_src == -1);
   sampler = ureg_DECL_sampler(c->ureg, instr->sampler_index);
   int sampler_src = nir_tex_instr_src_index(instr, nir_tex_src_sampler_offset);
   if (sampler_src >= 0) {
      struct ureg_src reladdr = ntt_get_src(c, instr->src[sampler_src].src);
                  switch (instr->op) {
   case nir_texop_tex:
      if (nir_tex_instr_src_size(instr, nir_tex_instr_src_index(instr, nir_tex_src_backend1)) >
      MAX2(instr->coord_components, 2) + instr->is_shadow)
      else
            case nir_texop_txf:
   case nir_texop_txf_ms:
               if (c->has_txf_lz) {
      int lod_src = nir_tex_instr_src_index(instr, nir_tex_src_lod);
   if (lod_src >= 0 &&
      nir_src_is_const(instr->src[lod_src].src) &&
   ntt_src_as_uint(c, instr->src[lod_src].src) == 0) {
         }
      case nir_texop_txl:
      tex_opcode = TGSI_OPCODE_TXL;
      case nir_texop_txb:
      tex_opcode = TGSI_OPCODE_TXB;
      case nir_texop_txd:
      tex_opcode = TGSI_OPCODE_TXD;
      case nir_texop_txs:
      tex_opcode = TGSI_OPCODE_TXQ;
      case nir_texop_tg4:
      tex_opcode = TGSI_OPCODE_TG4;
      case nir_texop_query_levels:
      tex_opcode = TGSI_OPCODE_TXQ;
      case nir_texop_lod:
      tex_opcode = TGSI_OPCODE_LODQ;
      case nir_texop_texture_samples:
      tex_opcode = TGSI_OPCODE_TXQS;
      default:
                  struct ntt_tex_operand_state s = { .i = 0 };
   ntt_push_tex_arg(c, instr, nir_tex_src_backend1, &s);
            /* non-coord arg for TXQ */
   if (tex_opcode == TGSI_OPCODE_TXQ) {
      ntt_push_tex_arg(c, instr, nir_tex_src_lod, &s);
   /* virglrenderer mistakenly looks at .w instead of .x, so make sure it's
   * scalar
   */
               if (s.i > 1) {
      if (tex_opcode == TGSI_OPCODE_TEX)
         if (tex_opcode == TGSI_OPCODE_TXB)
         if (tex_opcode == TGSI_OPCODE_TXL)
               if (instr->op == nir_texop_txd) {
      /* Derivs appear in their own src args */
   int ddx = nir_tex_instr_src_index(instr, nir_tex_src_ddx);
   int ddy = nir_tex_instr_src_index(instr, nir_tex_src_ddy);
   s.srcs[s.i++] = ntt_get_src(c, instr->src[ddx].src);
               if (instr->op == nir_texop_tg4 && target != TGSI_TEXTURE_SHADOWCUBE_ARRAY) {
      if (c->screen->get_param(c->screen,
            sampler = ureg_scalar(sampler, instr->component);
      } else {
                              enum tgsi_return_type tex_type;
   switch (instr->dest_type) {
   case nir_type_float32:
      tex_type = TGSI_RETURN_TYPE_FLOAT;
      case nir_type_int32:
      tex_type = TGSI_RETURN_TYPE_SINT;
      case nir_type_uint32:
      tex_type = TGSI_RETURN_TYPE_UINT;
      default:
                  struct ureg_dst tex_dst;
   if (instr->op == nir_texop_query_levels)
         else
            while (s.i < 4)
            struct ntt_insn *insn = ntt_insn(c, tex_opcode, tex_dst, s.srcs[0], s.srcs[1], s.srcs[2], s.srcs[3]);
   insn->tex_target = target;
   insn->tex_return_type = tex_type;
            int tex_offset_src = nir_tex_instr_src_index(instr, nir_tex_src_offset);
   if (tex_offset_src >= 0) {
               insn->tex_offset[0].File = offset.File;
   insn->tex_offset[0].Index = offset.Index;
   insn->tex_offset[0].SwizzleX = offset.SwizzleX;
   insn->tex_offset[0].SwizzleY = offset.SwizzleY;
   insn->tex_offset[0].SwizzleZ = offset.SwizzleZ;
               if (nir_tex_instr_has_explicit_tg4_offsets(instr)) {
      for (uint8_t i = 0; i < 4; ++i) {
      struct ureg_src imm = ureg_imm2i(c->ureg, instr->tg4_offsets[i][0], instr->tg4_offsets[i][1]);
   insn->tex_offset[i].File = imm.File;
   insn->tex_offset[i].Index = imm.Index;
   insn->tex_offset[i].SwizzleX = imm.SwizzleX;
   insn->tex_offset[i].SwizzleY = imm.SwizzleY;
                  if (instr->op == nir_texop_query_levels)
      }
      static void
   ntt_emit_jump(struct ntt_compile *c, nir_jump_instr *jump)
   {
      switch (jump->type) {
   case nir_jump_break:
      ntt_BRK(c);
         case nir_jump_continue:
      ntt_CONT(c);
         default:
      fprintf(stderr, "Unknown jump instruction: ");
   nir_print_instr(&jump->instr, stderr);
   fprintf(stderr, "\n");
         }
      static void
   ntt_emit_ssa_undef(struct ntt_compile *c, nir_undef_instr *instr)
   {
      /* Nothing to do but make sure that we have some storage to deref. */
      }
      static void
   ntt_emit_instr(struct ntt_compile *c, nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_deref:
      /* ignored, will be walked by nir_intrinsic_image_*_deref. */
         case nir_instr_type_alu:
      ntt_emit_alu(c, nir_instr_as_alu(instr));
         case nir_instr_type_intrinsic:
      ntt_emit_intrinsic(c, nir_instr_as_intrinsic(instr));
         case nir_instr_type_load_const:
      /* Nothing to do here, as load consts are done directly from
   * ntt_get_src() (since many constant NIR srcs will often get folded
   * directly into a register file index instead of as a TGSI src).
   */
         case nir_instr_type_tex:
      ntt_emit_texture(c, nir_instr_as_tex(instr));
         case nir_instr_type_jump:
      ntt_emit_jump(c, nir_instr_as_jump(instr));
         case nir_instr_type_undef:
      ntt_emit_ssa_undef(c, nir_instr_as_undef(instr));
         default:
      fprintf(stderr, "Unknown NIR instr type: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\n");
         }
      static void
   ntt_emit_if(struct ntt_compile *c, nir_if *if_stmt)
   {
      if (c->native_integers)
         else
                     if (!nir_cf_list_is_empty_block(&if_stmt->else_list)) {
      ntt_ELSE(c);
                  }
      static void
   ntt_emit_loop(struct ntt_compile *c, nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
   ntt_BGNLOOP(c);
   ntt_emit_cf_list(c, &loop->body);
      }
      static void
   ntt_emit_block(struct ntt_compile *c, nir_block *block)
   {
      struct ntt_block *ntt_block = ntt_block_from_nir(c, block);
            nir_foreach_instr(instr, block) {
               /* Sanity check that we didn't accidentally ureg_OPCODE() instead of ntt_OPCODE(). */
   if (ureg_get_instruction_number(c->ureg) != 0) {
      fprintf(stderr, "Emitted ureg insn during: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\n");
                  /* Set up the if condition for ntt_emit_if(), which we have to do before
   * freeing up the temps (the "if" is treated as inside the block for liveness
   * purposes, despite not being an instruction)
   *
   * Note that, while IF and UIF are supposed to look at only .x, virglrenderer
   * looks at all of .xyzw.  No harm in working around the bug.
   */
   nir_if *nif = nir_block_get_following_if(block);
   if (nif)
      }
      static void
   ntt_emit_cf_list(struct ntt_compile *c, struct exec_list *list)
   {
      foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block:
                  case nir_cf_node_if:
                  case nir_cf_node_loop:
                  default:
               }
      static void
   ntt_emit_block_ureg(struct ntt_compile *c, struct nir_block *block)
   {
               /* Emit the ntt insns to tgsi_ureg. */
   util_dynarray_foreach(&ntt_block->insns, struct ntt_insn, insn) {
      const struct tgsi_opcode_info *opcode_info =
            switch (insn->opcode) {
   case TGSI_OPCODE_UIF:
                  case TGSI_OPCODE_IF:
                  case TGSI_OPCODE_ELSE:
      ureg_fixup_label(c->ureg, c->current_if_else, ureg_get_instruction_number(c->ureg));
   ureg_ELSE(c->ureg, &c->cf_label);
               case TGSI_OPCODE_ENDIF:
      ureg_fixup_label(c->ureg, c->current_if_else, ureg_get_instruction_number(c->ureg));
               case TGSI_OPCODE_BGNLOOP:
      /* GLSL-to-TGSI never set the begin/end labels to anything, even though nvfx
   * does reference BGNLOOP's.  Follow the former behavior unless something comes up
   * with a need.
   */
               case TGSI_OPCODE_ENDLOOP:
                  default:
      if (insn->is_tex) {
      int num_offsets = 0;
   for (int i = 0; i < ARRAY_SIZE(insn->tex_offset); i++) {
      if (insn->tex_offset[i].File != TGSI_FILE_NULL)
      }
   ureg_tex_insn(c->ureg, insn->opcode,
               insn->dst, opcode_info->num_dst,
   insn->tex_target, insn->tex_return_type,
      } else if (insn->is_mem) {
      ureg_memory_insn(c->ureg, insn->opcode,
                  insn->dst, opcode_info->num_dst,
   insn->src, opcode_info->num_src,
   } else {
      ureg_insn(c->ureg, insn->opcode,
            insn->dst, opcode_info->num_dst,
            }
      static void
   ntt_emit_if_ureg(struct ntt_compile *c, nir_if *if_stmt)
   {
               int if_stack = c->current_if_else;
            /* Either the then or else block includes the ENDIF, which will fix up the
   * IF(/ELSE)'s label for jumping
   */
   ntt_emit_cf_list_ureg(c, &if_stmt->then_list);
               }
      static void
   ntt_emit_cf_list_ureg(struct ntt_compile *c, struct exec_list *list)
   {
      foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block:
                  case nir_cf_node_if:
                  case nir_cf_node_loop:
      /* GLSL-to-TGSI never set the begin/end labels to anything, even though nvfx
   * does reference BGNLOOP's.  Follow the former behavior unless something comes up
   * with a need.
   */
               default:
               }
      static void
   ntt_emit_impl(struct ntt_compile *c, nir_function_impl *impl)
   {
               c->ssa_temp = rzalloc_array(c, struct ureg_src, impl->ssa_alloc);
            /* Set up the struct ntt_blocks to put insns in */
   c->blocks = _mesa_pointer_hash_table_create(c);
   nir_foreach_block(block, impl) {
      struct ntt_block *ntt_block = rzalloc(c->blocks, struct ntt_block);
   util_dynarray_init(&ntt_block->insns, ntt_block);
                           c->cur_block = ntt_block_from_nir(c, nir_start_block(impl));
   ntt_setup_inputs(c);
   ntt_setup_outputs(c);
            /* Emit the ntt insns */
            /* Don't do optimized RA if the driver requests it, unless the number of
   * temps is too large to be covered by the 16 bit signed int that TGSI
   * allocates for the register index */
   if (!c->options->unoptimized_ra || c->num_temps > 0x7fff)
         else
            /* Turn the ntt insns into actual TGSI tokens */
            ralloc_free(c->liveness);
         }
      static int
   type_size(const struct glsl_type *type, bool bindless)
   {
         }
      /* Allow vectorizing of ALU instructions, but avoid vectorizing past what we
   * can handle for 64-bit values in TGSI.
   */
   static uint8_t
   ntt_should_vectorize_instr(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
                     switch (alu->op) {
   case nir_op_ibitfield_extract:
   case nir_op_ubitfield_extract:
   case nir_op_bitfield_insert:
      /* virglrenderer only looks at the .x channel of the offset/bits operands
   * when translating to GLSL.  tgsi.rst doesn't seem to require scalar
   * offset/bits operands.
   *
   * https://gitlab.freedesktop.org/virgl/virglrenderer/-/issues/195
   */
         default:
                  int src_bit_size = nir_src_bit_size(alu->src[0].src);
            if (src_bit_size == 64 || dst_bit_size == 64) {
      /* Avoid vectorizing 64-bit instructions at all.  Despite tgsi.rst
   * claiming support, virglrenderer generates bad shaders on the host when
   * presented with them.  Maybe we can make virgl avoid tickling the
   * virglrenderer bugs, but given that glsl-to-TGSI didn't generate vector
   * 64-bit instrs in the first place, I don't see much reason to care about
   * this.
   */
                  }
      static bool
   ntt_should_vectorize_io(unsigned align, unsigned bit_size,
                     {
      if (bit_size != 32)
            /* Our offset alignment should aways be at least 4 bytes */
   if (align < 4)
            /* No wrapping off the end of a TGSI reg.  We could do a bit better by
   * looking at low's actual offset.  XXX: With LOAD_CONSTBUF maybe we don't
   * need this restriction.
   */
   unsigned worst_start_component = align == 4 ? 3 : align / 4;
   if (worst_start_component + num_components > 4)
               }
      static nir_variable_mode
   ntt_no_indirects_mask(nir_shader *s, struct pipe_screen *screen)
   {
      unsigned pipe_stage = pipe_shader_type_from_mesa(s->info.stage);
            if (!screen->get_shader_param(screen, pipe_stage,
                        if (!screen->get_shader_param(screen, pipe_stage,
                        if (!screen->get_shader_param(screen, pipe_stage,
                           }
      static void
   ntt_optimize_nir(struct nir_shader *s, struct pipe_screen *screen,
         {
      bool progress;
   unsigned pipe_stage = pipe_shader_type_from_mesa(s->info.stage);
   unsigned control_flow_depth =
      screen->get_shader_param(screen, pipe_stage,
      do {
               NIR_PASS_V(s, nir_lower_vars_to_ssa);
            NIR_PASS(progress, s, nir_copy_prop);
   NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   NIR_PASS(progress, s, nir_opt_remove_phis);
   NIR_PASS(progress, s, nir_opt_conditional_discard);
   NIR_PASS(progress, s, nir_opt_dce);
   NIR_PASS(progress, s, nir_opt_dead_cf);
   NIR_PASS(progress, s, nir_opt_cse);
   NIR_PASS(progress, s, nir_opt_find_array_copies);
   NIR_PASS(progress, s, nir_opt_copy_prop_vars);
            NIR_PASS(progress, s, nir_opt_if, nir_opt_if_aggressive_last_continue | nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, s, nir_opt_peephole_select,
         NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   nir_load_store_vectorize_options vectorize_opts = {
      .modes = nir_var_mem_ubo,
   .callback = ntt_should_vectorize_io,
      };
   NIR_PASS(progress, s, nir_opt_load_store_vectorize, &vectorize_opts);
   NIR_PASS(progress, s, nir_opt_shrink_stores, true);
   NIR_PASS(progress, s, nir_opt_shrink_vectors);
   NIR_PASS(progress, s, nir_opt_trivial_continues);
   NIR_PASS(progress, s, nir_opt_vectorize, ntt_should_vectorize_instr, NULL);
   NIR_PASS(progress, s, nir_opt_undef);
            /* Try to fold addressing math into ubo_vec4's base to avoid load_consts
   * and ALU ops for it.
   */
   nir_opt_offsets_options offset_options = {
                              /* unused intrinsics */
   .uniform_max = 0,
               if (options->ubo_vec4_max)
                           }
      /* Scalarizes all 64-bit ALU ops.  Note that we only actually need to
   * scalarize vec3/vec4s, should probably fix that.
   */
   static bool
   scalarize_64bit(const nir_instr *instr, const void *data)
   {
               return (alu->def.bit_size == 64 ||
      }
      static bool
   nir_to_tgsi_lower_64bit_intrinsic(nir_builder *b, nir_intrinsic_instr *instr)
   {
               switch (instr->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_ssbo:
         default:
                  if (instr->num_components <= 2)
            bool has_dest = nir_intrinsic_infos[instr->intrinsic].has_dest;
   if (has_dest) {
      if (instr->def.bit_size != 64)
      } else  {
      if (nir_src_bit_size(instr->src[0]) != 64)
               nir_intrinsic_instr *first =
         nir_intrinsic_instr *second =
            switch (instr->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_store_ssbo:
            default: {
      nir_io_semantics semantics = nir_intrinsic_io_semantics(second);
   semantics.location++;
   semantics.num_slots--;
            nir_intrinsic_set_base(second, nir_intrinsic_base(second) + 1);
      }
            first->num_components = 2;
   second->num_components -= 2;
   if (has_dest) {
      first->def.num_components = 2;
               nir_builder_instr_insert(b, &first->instr);
            if (has_dest) {
      /* Merge the two loads' results back into a vector. */
   nir_scalar channels[4] = {
      nir_get_scalar(&first->def, 0),
   nir_get_scalar(&first->def, 1),
   nir_get_scalar(&second->def, 0),
      };
   nir_def *new = nir_vec_scalars(b, channels, instr->num_components);
      } else {
      /* Split the src value across the two stores. */
            nir_def *src0 = instr->src[0].ssa;
   nir_scalar channels[4] = { 0 };
   for (int i = 0; i < instr->num_components; i++)
            nir_intrinsic_set_write_mask(first, nir_intrinsic_write_mask(instr) & 3);
            nir_src_rewrite(&first->src[0], nir_vec_scalars(b, channels, 2));
   nir_src_rewrite(&second->src[0],
               int offset_src = -1;
            switch (instr->intrinsic) {
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_ubo:
      offset_src = 1;
      case nir_intrinsic_load_ubo_vec4:
      offset_src = 1;
   offset_amount = 1;
      case nir_intrinsic_store_ssbo:
      offset_src = 2;
      default:
         }
   if (offset_src != -1) {
      b->cursor = nir_before_instr(&second->instr);
   nir_def *second_offset =
                     /* DCE stores we generated with no writemask (nothing else does this
   * currently).
   */
   if (!has_dest) {
      if (nir_intrinsic_write_mask(first) == 0)
         if (nir_intrinsic_write_mask(second) == 0)
                           }
      static bool
   nir_to_tgsi_lower_64bit_load_const(nir_builder *b, nir_load_const_instr *instr)
   {
               if (instr->def.bit_size != 64 || num_components <= 2)
                     nir_load_const_instr *first =
         nir_load_const_instr *second =
            first->value[0] = instr->value[0];
   first->value[1] = instr->value[1];
   second->value[0] = instr->value[2];
   if (num_components == 4)
            nir_builder_instr_insert(b, &first->instr);
            nir_def *channels[4] = {
      nir_channel(b, &first->def, 0),
   nir_channel(b, &first->def, 1),
   nir_channel(b, &second->def, 0),
      };
   nir_def *new = nir_vec(b, channels, num_components);
   nir_def_rewrite_uses(&instr->def, new);
               }
      static bool
   nir_to_tgsi_lower_64bit_to_vec2_instr(nir_builder *b, nir_instr *instr,
         {
      switch (instr->type) {
   case nir_instr_type_load_const:
            case nir_instr_type_intrinsic:
         default:
            }
      static bool
   nir_to_tgsi_lower_64bit_to_vec2(nir_shader *s)
   {
      return nir_shader_instructions_pass(s,
                        }
      struct ntt_lower_tex_state {
      nir_scalar channels[8];
      };
      static void
   nir_to_tgsi_lower_tex_instr_arg(nir_builder *b,
                     {
      int tex_src = nir_tex_instr_src_index(instr, tex_src_type);
   if (tex_src < 0)
            nir_def *def = instr->src[tex_src].src.ssa;
   for (int i = 0; i < def->num_components; i++) {
                     }
      /**
   * Merges together a vec4 of tex coordinate/compare/bias/lod into a backend tex
   * src.  This lets NIR handle the coalescing of the vec4 rather than trying to
   * manage it on our own, and may lead to more vectorization.
   */
   static bool
   nir_to_tgsi_lower_tex_instr(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_tex)
                     if (nir_tex_instr_src_index(tex, nir_tex_src_coord) < 0)
                              nir_to_tgsi_lower_tex_instr_arg(b, tex, nir_tex_src_coord, &s);
   /* We always have at least two slots for the coordinate, even on 1D. */
            nir_to_tgsi_lower_tex_instr_arg(b, tex, nir_tex_src_comparator, &s);
                     /* XXX: LZ */
   nir_to_tgsi_lower_tex_instr_arg(b, tex, nir_tex_src_lod, &s);
   nir_to_tgsi_lower_tex_instr_arg(b, tex, nir_tex_src_projector, &s);
            /* No need to pack undefs in unused channels of the tex instr */
   while (!s.channels[s.i - 1].def)
            /* Instead of putting undefs in the unused slots of the vecs, just put in
   * another used channel.  Otherwise, we'll get unnecessary moves into
   * registers.
   */
   assert(s.channels[0].def != NULL);
   for (int i = 1; i < s.i; i++) {
      if (!s.channels[i].def)
               nir_tex_instr_add_src(tex, nir_tex_src_backend1,
         if (s.i > 4)
      nir_tex_instr_add_src(tex, nir_tex_src_backend2,
            }
      static bool
   nir_to_tgsi_lower_tex(nir_shader *s)
   {
      return nir_shader_instructions_pass(s,
                        }
      static void
   ntt_fix_nir_options(struct pipe_screen *screen, struct nir_shader *s,
         {
      const struct nir_shader_compiler_options *options = s->options;
   bool lower_fsqrt =
      !screen->get_shader_param(screen, pipe_shader_type_from_mesa(s->info.stage),
         bool force_indirect_unrolling_sampler =
                     if (!options->lower_extract_byte ||
      !options->lower_extract_word ||
   !options->lower_insert_byte ||
   !options->lower_insert_word ||
   !options->lower_fdph ||
   !options->lower_flrp64 ||
   !options->lower_fmod ||
   !options->lower_uadd_carry ||
   !options->lower_usub_borrow ||
   !options->lower_uadd_sat ||
   !options->lower_usub_sat ||
   !options->lower_uniforms_to_ubo ||
   !options->lower_vector_cmp ||
   options->has_rotate8 ||
   options->has_rotate16 ||
   options->has_rotate32 ||
   options->lower_fsqrt != lower_fsqrt ||
   options->force_indirect_unrolling != no_indirects_mask ||
   force_indirect_unrolling_sampler) {
   nir_shader_compiler_options *new_options = ralloc(s, nir_shader_compiler_options);
            new_options->lower_extract_byte = true;
   new_options->lower_extract_word = true;
   new_options->lower_insert_byte = true;
   new_options->lower_insert_word = true;
   new_options->lower_fdph = true;
   new_options->lower_flrp64 = true;
   new_options->lower_fmod = true;
   new_options->lower_uadd_carry = true;
   new_options->lower_usub_borrow = true;
   new_options->lower_uadd_sat = true;
   new_options->lower_usub_sat = true;
   new_options->lower_uniforms_to_ubo = true;
   new_options->lower_vector_cmp = true;
   new_options->lower_fsqrt = lower_fsqrt;
   new_options->has_rotate8 = false;
   new_options->has_rotate16 = false;
   new_options->has_rotate32 = false;
   new_options->force_indirect_unrolling = no_indirects_mask;
                  }
      static bool
   ntt_lower_atomic_pre_dec_filter(const nir_instr *instr, const void *_data)
   {
      return (instr->type == nir_instr_type_intrinsic &&
      }
      static nir_def *
   ntt_lower_atomic_pre_dec_lower(nir_builder *b, nir_instr *instr, void *_data)
   {
               nir_def *old_result = &intr->def;
               }
      static bool
   ntt_lower_atomic_pre_dec(nir_shader *s)
   {
      return nir_shader_lower_instructions(s,
            }
      /* Lowers texture projectors if we can't do them as TGSI_OPCODE_TXP. */
   static void
   nir_to_tgsi_lower_txp(nir_shader *s)
   {
      nir_lower_tex_options lower_tex_options = {
                  nir_foreach_block(block, nir_shader_get_entrypoint(s)) {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_tex)
                                 bool has_compare = nir_tex_instr_src_index(tex, nir_tex_src_comparator) >= 0;
                  /* We can do TXP for any tex (not txg) where we can fit all the
   * coordinates and comparator and projector in one vec4 without any
   * other modifiers to add on.
   *
   * nir_lower_tex() only handles the lowering on a sampler-dim basis, so
   * if we get any funny projectors then we just blow them all away.
   */
   if (tex->op != nir_texop_tex || has_lod || has_offset || (tex->coord_components >= 3 && has_compare))
                  /* nir_lower_tex must be run even if no options are set, because we need the
   * LOD to be set for query_levels and for non-fragment shaders.
   */
      }
      static bool
   nir_lower_primid_sysval_to_input_filter(const nir_instr *instr, const void *_data)
   {
      return (instr->type == nir_instr_type_intrinsic &&
      }
      static nir_def *
   nir_lower_primid_sysval_to_input_lower(nir_builder *b, nir_instr *instr, void *data)
   {
      nir_variable *var = nir_get_variable_with_location(b->shader, nir_var_shader_in,
            nir_io_semantics semantics = {
      .location = var->data.location,
      };
   return nir_load_input(b, 1, 32, nir_imm_int(b, 0),
            }
      static bool
   nir_lower_primid_sysval_to_input(nir_shader *s)
   {
      return nir_shader_lower_instructions(s,
            }
      const void *
   nir_to_tgsi(struct nir_shader *s,
         {
      static const struct nir_to_tgsi_options default_ntt_options = {0};
      }
      /* Prevent lower_vec_to_mov from coalescing 64-to-32 conversions and comparisons
   * into unsupported channels of registers.
   */
   static bool
   ntt_vec_to_mov_writemask_cb(const nir_instr *instr, unsigned writemask, UNUSED const void *_data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
   int dst_32 = alu->def.bit_size == 32;
            if (src_64 && dst_32) {
               if (num_srcs == 2 || nir_op_infos[alu->op].output_type == nir_type_bool32) {
      /* TGSI's 64 bit compares storing to 32-bit are weird and write .xz
   * instead of .xy.  Just support scalar compares storing to .x,
   * GLSL-to-TGSI only ever emitted scalar ops anyway.
      if (writemask != TGSI_WRITEMASK_X)
         } else {
      /* TGSI's 64-to-32-bit conversions can only store to .xy (since a TGSI
   * register can only store a dvec2).  Don't try to coalesce to write to
   * .zw.
   */
   if (writemask & ~(TGSI_WRITEMASK_XY))
                     }
      /**
   * Translates the NIR shader to TGSI.
   *
   * This requires some lowering of the NIR shader to prepare it for translation.
   * We take ownership of the NIR shader passed, returning a reference to the new
   * TGSI tokens instead.  If you need to keep the NIR, then pass us a clone.
   */
   const void *nir_to_tgsi_options(struct nir_shader *s,
               {
      struct ntt_compile *c;
   const void *tgsi_tokens;
   nir_variable_mode no_indirects_mask = ntt_no_indirects_mask(s, screen);
   bool native_integers = screen->get_shader_param(screen,
                                 /* Lower array indexing on FS inputs.  Since we don't set
   * ureg->supports_any_inout_decl_range, the TGSI input decls will be split to
   * elements by ureg, and so dynamically indexing them would be invalid.
   * Ideally we would set that ureg flag based on
   * PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE, but can't due to mesa/st
   * splitting NIR VS outputs to elements even if the FS doesn't get the
   * corresponding splitting, and virgl depends on TGSI across link boundaries
   * having matching declarations.
   */
   if (s->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(s, nir_lower_indirect_derefs, nir_var_shader_in, UINT32_MAX);
               /* Lower tesslevel indirect derefs for tessellation shader.
   * tesslevels are now a compact array variable and nir expects a constant
   * array index into the compact array variable.
   */
   if (s->info.stage == MESA_SHADER_TESS_CTRL ||
      s->info.stage == MESA_SHADER_TESS_EVAL) {
               NIR_PASS_V(s, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            nir_to_tgsi_lower_txp(s);
            /* While TGSI can represent PRIMID as either an input or a system value,
   * glsl-to-tgsi had the GS (not TCS or TES) primid as an input, and drivers
   * depend on that.
   */
   if (s->info.stage == MESA_SHADER_GEOMETRY)
            if (s->info.num_abos)
            if (!original_options->lower_uniforms_to_ubo) {
      NIR_PASS_V(s, nir_lower_uniforms_to_ubo,
                     /* Do lowering so we can directly translate f64/i64 NIR ALU ops to TGSI --
   * TGSI stores up to a vec2 in each slot, so to avoid a whole bunch of op
   * duplication logic we just make it so that we only see vec2s.
   */
   NIR_PASS_V(s, nir_lower_alu_to_scalar, scalarize_64bit, NULL);
            if (!screen->get_param(screen, PIPE_CAP_LOAD_CONSTBUF))
                              /* Lower demote_if to if (cond) { demote } because TGSI doesn't have a DEMOTE_IF. */
                     bool progress;
   do {
      progress = false;
   NIR_PASS(progress, s, nir_opt_algebraic_late);
   if (progress) {
      NIR_PASS_V(s, nir_copy_prop);
   NIR_PASS_V(s, nir_opt_dce);
                           if (screen->get_shader_param(screen,
                     } else {
      NIR_PASS_V(s, nir_lower_int_to_float);
   NIR_PASS_V(s, nir_lower_bool_to_float,
         /* bool_to_float generates MOVs for b2f32 that we want to clean up. */
   NIR_PASS_V(s, nir_copy_prop);
               nir_move_options move_all =
      nir_move_const_undef | nir_move_load_ubo | nir_move_load_input |
                  NIR_PASS_V(s, nir_convert_from_ssa, true);
            /* locals_to_reg_intrinsics will leave dead derefs that are good to clean up.
   */
   NIR_PASS_V(s, nir_lower_locals_to_regs, 32);
            /* See comment in ntt_get_alu_src for supported modifiers */
            if (NIR_DEBUG(TGSI)) {
      fprintf(stderr, "NIR before translation to TGSI:\n");
               c = rzalloc(NULL, struct ntt_compile);
   c->screen = screen;
            c->needs_texcoord_semantic =
         c->has_txf_lz =
            c->s = s;
   c->native_integers = native_integers;
   c->ureg = ureg_create(pipe_shader_type_from_mesa(s->info.stage));
   ureg_setup_shader_info(c->ureg, &s->info);
   if (s->info.use_legacy_math_rules && screen->get_param(screen, PIPE_CAP_LEGACY_MATH_RULES))
            if (s->info.stage == MESA_SHADER_FRAGMENT) {
      /* The draw module's polygon stipple layer doesn't respect the chosen
   * coordinate mode, so leave it as unspecified unless we're actually
   * reading the position in the shader already.  See
   * gl-2.1-polygon-stipple-fs on softpipe.
   */
   if ((s->info.inputs_read & VARYING_BIT_POS) ||
      BITSET_TEST(s->info.system_values_read, SYSTEM_VALUE_FRAG_COORD)) {
   ureg_property(c->ureg, TGSI_PROPERTY_FS_COORD_ORIGIN,
                        ureg_property(c->ureg, TGSI_PROPERTY_FS_COORD_PIXEL_CENTER,
                     }
   /* Emit the main function */
   nir_function_impl *impl = nir_shader_get_entrypoint(c->s);
   ntt_emit_impl(c, impl);
                     if (NIR_DEBUG(TGSI)) {
      fprintf(stderr, "TGSI after translation from NIR:\n");
                        ralloc_free(c);
               }
      static const nir_shader_compiler_options nir_to_tgsi_compiler_options = {
      .fdot_replicates = true,
   .fuse_ffma32 = true,
   .fuse_ffma64 = true,
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .lower_fdph = true,
   .lower_flrp64 = true,
   .lower_fmod = true,
   .lower_uniforms_to_ubo = true,
   .lower_uadd_carry = true,
   .lower_usub_borrow = true,
   .lower_uadd_sat = true,
   .lower_usub_sat = true,
   .lower_vector_cmp = true,
   .lower_int64_options = nir_lower_imul_2x32_64,
            /* TGSI doesn't have a semantic for local or global index, just local and
   * workgroup id.
   */
      };
      /* Returns a default compiler options for drivers with only nir-to-tgsi-based
   * NIR support.
   */
   const void *
   nir_to_tgsi_get_compiler_options(struct pipe_screen *pscreen,
               {
      assert(ir == PIPE_SHADER_IR_NIR);
      }
      /** Helper for getting TGSI tokens to store for a pipe_shader_state CSO. */
   const void *
   pipe_shader_state_to_tgsi_tokens(struct pipe_screen *screen,
         {
      if (cso->type == PIPE_SHADER_IR_NIR) {
         } else {
      assert(cso->type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
         }
