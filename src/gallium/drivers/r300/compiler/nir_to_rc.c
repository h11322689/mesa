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
   #include "nir_to_rc.h"
   #include "r300_nir.h"
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
      struct ntr_insn {
      enum tgsi_opcode opcode;
   struct ureg_dst dst[2];
   struct ureg_src src[4];
   enum tgsi_texture_type tex_target;
   enum tgsi_return_type tex_return_type;
            unsigned mem_qualifier;
            bool is_tex : 1;
      };
      struct ntr_block {
      /* Array of struct ntr_insn */
   struct util_dynarray insns;
   int start_ip;
      };
      struct ntr_reg_interval {
         };
      struct ntr_compile {
      nir_shader *s;
   nir_function_impl *impl;
   const struct nir_to_rc_options *options;
   struct pipe_screen *screen;
            bool addr_declared[3];
            /* if condition set up at the end of a block, for ntr_emit_if(). */
            /* TGSI temps for our NIR SSA and register values. */
   struct ureg_dst *reg_temp;
                     /* Map from nir_block to ntr_block */
   struct hash_table *blocks;
   struct ntr_block *cur_block;
   unsigned current_if_else;
            /* Whether we're currently emitting instructiosn for a precise NIR instruction. */
            unsigned num_temps;
            /* Mappings from driver_location to TGSI input/output number.
   *
   * We'll be declaring TGSI input/outputs in an arbitrary order, and they get
   * their numbers assigned incrementally, unlike inputs or constants.
   */
   struct ureg_src *input_index_map;
               };
      static struct ureg_dst
   ntr_temp(struct ntr_compile *c)
   {
         }
      static struct ntr_block *
   ntr_block_from_nir(struct ntr_compile *c, struct nir_block *block)
   {
      struct hash_entry *entry = _mesa_hash_table_search(c->blocks, block);
      }
      static void ntr_emit_cf_list(struct ntr_compile *c, struct exec_list *list);
   static void ntr_emit_cf_list_ureg(struct ntr_compile *c, struct exec_list *list);
      static struct ntr_insn *
   ntr_insn(struct ntr_compile *c, enum tgsi_opcode opcode,
            struct ureg_dst dst,
      {
      struct ntr_insn insn = {
      .opcode = opcode,
   .dst = { dst, ureg_dst_undef() },
   .src = { src0, src1, src2, src3 },
      };
   util_dynarray_append(&c->cur_block->insns, struct ntr_insn, insn);
      }
      #define OP00( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c)                                     \
   {                                                                                      \
         }
      #define OP01( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c,                                     \
         {                                                                                      \
         }
         #define OP10( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c,                                     \
         {                                                                                      \
         }
      #define OP11( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c,                                     \
               {                                                                                      \
         }
      #define OP12( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c,                                     \
                     {                                                                                      \
         }
      #define OP13( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c,                                     \
                           {                                                                                      \
         }
      #define OP14( op )                                                                     \
   static inline void ntr_##op(struct ntr_compile *c,                                     \
                        struct ureg_dst dst,                                              \
      {                                                                                      \
         }
      /* We hand-craft our tex instructions */
   #define OP12_TEX(op)
   #define OP14_TEX(op)
      /* Use a template include to generate a correctly-typed ntr_OP()
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
   ntr_src_as_uint(struct ntr_compile *c, nir_src src)
   {
      uint32_t val = nir_src_as_uint(src);
   if (val >= fui(1.0))
            }
      /* Per-channel masks of def/use within the block, and the per-channel
   * livein/liveout for the block as a whole.
   */
   struct ntr_live_reg_block_state {
         };
      struct ntr_live_reg_state {
                        /* Used in propagate_across_edge() */
                        };
      static void
   ntr_live_reg_mark_use(struct ntr_compile *c, struct ntr_live_reg_block_state *bs,
         {
               c->liveness[index].start = MIN2(c->liveness[index].start, ip);
         }
   static void
   ntr_live_reg_setup_def_use(struct ntr_compile *c, nir_function_impl *impl, struct ntr_live_reg_state *state)
   {
      for (int i = 0; i < impl->num_blocks; i++) {
      state->blocks[i].def = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].defin = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].defout = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].use = rzalloc_array(state->blocks, uint8_t, c->num_temps);
   state->blocks[i].livein = rzalloc_array(state->blocks, uint8_t, c->num_temps);
               int ip = 0;
   nir_foreach_block(block, impl) {
      struct ntr_live_reg_block_state *bs = &state->blocks[block->index];
                     util_dynarray_foreach(&ntr_block->insns, struct ntr_insn, insn) {
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
   ntr_live_regs(struct ntr_compile *c, nir_function_impl *impl)
   {
                        struct ntr_live_reg_state state = {
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
   struct ntr_block *ntr_block = ntr_block_from_nir(c, block);
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
   ntr_ra_check(struct ntr_compile *c, unsigned *ra_map, BITSET_WORD *released, int ip, unsigned index)
   {
      if (index < c->first_non_array_temp)
            if (c->liveness[index].start == ip && ra_map[index] == ~0)
            if (c->liveness[index].end == ip && !BITSET_TEST(released, index)) {
      ureg_release_temporary(c->ureg, ureg_dst_register(TGSI_FILE_TEMPORARY, ra_map[index]));
         }
      static void
   ntr_allocate_regs(struct ntr_compile *c, nir_function_impl *impl)
   {
               unsigned *ra_map = ralloc_array(c, unsigned, c->num_temps);
            /* No RA on NIR array regs */
   for (int i = 0; i < c->first_non_array_temp; i++)
            for (int i = c->first_non_array_temp; i < c->num_temps; i++)
            int ip = 0;
   nir_foreach_block(block, impl) {
               for (int i = 0; i < c->num_temps; i++)
            util_dynarray_foreach(&ntr_block->insns, struct ntr_insn, insn) {
                     for (int i = 0; i < opcode_info->num_src; i++) {
      if (insn->src[i].File == TGSI_FILE_TEMPORARY) {
      ntr_ra_check(c, ra_map, released, ip, insn->src[i].Index);
                  if (insn->is_tex) {
      for (int i = 0; i < ARRAY_SIZE(insn->tex_offset); i++) {
      if (insn->tex_offset[i].File == TGSI_FILE_TEMPORARY) {
      ntr_ra_check(c, ra_map, released, ip, insn->tex_offset[i].Index);
                     for (int i = 0; i < opcode_info->num_dst; i++) {
      if (insn->dst[i].File == TGSI_FILE_TEMPORARY) {
      ntr_ra_check(c, ra_map, released, ip, insn->dst[i].Index);
         }
               for (int i = 0; i < c->num_temps; i++)
         }
      static void
   ntr_allocate_regs_unoptimized(struct ntr_compile *c, nir_function_impl *impl)
   {
      for (int i = c->first_non_array_temp; i < c->num_temps; i++)
      }
      /* TGSI varying declarations have a component usage mask associated (used by
   * r600 and svga).
   */
   static uint32_t
   ntr_tgsi_var_usage_mask(const struct nir_variable *var)
   {
      const struct glsl_type *type_without_array =
         unsigned num_components = glsl_get_vector_elements(type_without_array);
   if (num_components == 0) /* structs */
               }
      static struct ureg_dst
   ntr_output_decl(struct ntr_compile *c, nir_intrinsic_instr *instr, uint32_t *frac)
   {
      nir_io_semantics semantics = nir_intrinsic_io_semantics(instr);
   int base = nir_intrinsic_base(instr);
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
               tgsi_get_gl_varying_semantic(semantics.location, true,
            uint32_t usage_mask = u_bit_consecutive(*frac, instr->num_components);
   uint32_t gs_streams = semantics.gs_streams;
   for (int i = 0; i < 4; i++) {
      if (!(usage_mask & (1 << i)))
               /* No driver appears to use array_id of outputs. */
            /* This bit is lost in the i/o semantics, but it's unused in in-tree
   * drivers.
   */
            out = ureg_DECL_output_layout(c->ureg,
                                 semantic_name, semantic_index,
               unsigned write_mask;
   if (nir_intrinsic_has_write_mask(instr))
         else
            write_mask = write_mask << *frac;
      }
      static bool
   ntr_try_store_in_tgsi_output_with_use(struct ntr_compile *c,
               {
               if (nir_src_is_if(src))
            if (nir_src_parent_instr(src)->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(nir_src_parent_instr(src));
   if (intr->intrinsic != nir_intrinsic_store_output ||
      !nir_src_is_const(intr->src[1])) {
               uint32_t frac;
   *dst = ntr_output_decl(c, intr, &frac);
               }
      /* If this reg is used only for storing an output, then in the simple
   * cases we can write directly to the TGSI output instead of having
   * store_output emit its own MOV.
   */
   static bool
   ntr_try_store_reg_in_tgsi_output(struct ntr_compile *c, struct ureg_dst *dst,
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
   ntr_try_store_ssa_in_tgsi_output(struct ntr_compile *c, struct ureg_dst *dst,
         {
               if (!list_is_singular(&def->uses))
            nir_foreach_use_including_if(use, def) {
         }
      }
      static void
   ntr_setup_inputs(struct ntr_compile *c)
   {
      if (c->s->info.stage != MESA_SHADER_FRAGMENT)
            unsigned num_inputs = 0;
            nir_foreach_shader_in_variable(var, c->s) {
      const struct glsl_type *type = var->type;
   unsigned array_len =
                                 nir_foreach_shader_in_variable(var, c->s) {
      const struct glsl_type *type = var->type;
   unsigned array_len =
            unsigned interpolation = TGSI_INTERPOLATE_CONSTANT;
   unsigned sample_loc;
            if (c->s->info.stage == MESA_SHADER_FRAGMENT) {
      interpolation =
      tgsi_get_interp_mode(var->data.interpolation,
               if (var->data.location == VARYING_SLOT_POS)
               unsigned semantic_name, semantic_index;
   tgsi_get_gl_varying_semantic(var->data.location, true,
            if (var->data.sample) {
         } else if (var->data.centroid) {
      sample_loc = TGSI_INTERPOLATE_LOC_CENTROID;
   c->centroid_inputs |= (BITSET_MASK(array_len) <<
      } else {
                  unsigned array_id = 0;
   if (glsl_type_is_array(type))
                     decl = ureg_DECL_fs_input_centroid_layout(c->ureg,
                                                if (semantic_name == TGSI_SEMANTIC_FACE) {
      struct ureg_dst temp = ntr_temp(c);
   /* tgsi docs say that floating point FACE will be positive for
   * frontface and negative for backface, but realistically
   * GLSL-to-TGSI had been doing MOV_SAT to turn it into 0.0 vs 1.0.
   * Copy that behavior, since some drivers (r300) have been doing a
   * 0.0 vs 1.0 backface (and I don't think anybody has a non-1.0
   * front face).
   */
   temp.Saturate = true;
   ntr_MOV(c, temp, decl);
               for (unsigned i = 0; i < array_len; i++) {
      c->input_index_map[var->data.driver_location + i] = decl;
            }
      static int
   ntr_sort_by_location(const nir_variable *a, const nir_variable *b)
   {
         }
      /**
   * Workaround for virglrenderer requiring that TGSI FS output color variables
   * are declared in order.  Besides, it's a lot nicer to read the TGSI this way.
   */
   static void
   ntr_setup_outputs(struct ntr_compile *c)
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
   ntr_setup_uniforms(struct ntr_compile *c)
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
                  /* lower_uniforms_to_ubo lowered non-sampler uniforms to UBOs, so CB0
   * size declaration happens with other UBOs below.
   */
                        unsigned ubo_sizes[PIPE_MAX_CONSTANT_BUFFERS] = {0};
   nir_foreach_variable_with_modes(var, c->s, nir_var_mem_ubo) {
      int ubo = var->data.driver_location;
   if (ubo == -1)
            if (!(ubo == 0 && c->s->info.first_ubo_is_default_ubo))
            unsigned size = glsl_get_explicit_size(var->interface_type, false);
               for (int i = 0; i < ARRAY_SIZE(ubo_sizes); i++) {
      if (ubo_sizes[i])
         }
      static void
   ntr_setup_registers(struct ntr_compile *c)
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
            /* We already handled arrays */
   if (num_array_elems == 0) {
                     if (!ntr_try_store_reg_in_tgsi_output(c, &decl, nir_reg)) {
         }
            }
      static struct ureg_src
   ntr_get_load_const_src(struct ntr_compile *c, nir_load_const_instr *instr)
   {
               float values[4];
   assert(instr->def.bit_size == 32);
   for (int i = 0; i < num_components; i++)
               }
      static struct ureg_src
   ntr_reladdr(struct ntr_compile *c, struct ureg_src addr, int addr_index)
   {
               for (int i = 0; i <= addr_index; i++) {
      if (!c->addr_declared[i]) {
      c->addr_reg[i] = ureg_writemask(ureg_DECL_address(c->ureg),
                        ntr_ARL(c, c->addr_reg[addr_index], addr);
      }
      /* Forward declare for recursion with indirects */
   static struct ureg_src
   ntr_get_src(struct ntr_compile *c, nir_src src);
      static struct ureg_src
   ntr_get_chased_src(struct ntr_compile *c, nir_legacy_src *src)
   {
      if (src->is_ssa) {
      if (src->ssa->parent_instr->type == nir_instr_type_load_const)
               } else {
      struct ureg_dst reg_temp = c->reg_temp[src->reg.handle->index];
            if (src->reg.indirect) {
      struct ureg_src offset = ntr_get_src(c, nir_src_for_ssa(src->reg.indirect));
   return ureg_src_indirect(ureg_src(reg_temp),
      } else {
               }
      static struct ureg_src
   ntr_get_src(struct ntr_compile *c, nir_src src)
   {
      nir_legacy_src chased = nir_legacy_chase_src(&src);
      }
      static struct ureg_src
   ntr_get_alu_src(struct ntr_compile *c, nir_alu_instr *instr, int i)
   {
      /* We only support 32-bit float modifiers.  The only other modifier type
   * officially supported by TGSI is 32-bit integer negates, but even those are
   * broken on virglrenderer, so skip lowering all integer and f64 float mods.
   *
   * The options->lower_fabs requests that we not have native source modifiers
   * for fabs, and instead emit MAX(a,-a) for nir_op_fabs.
   */
   nir_legacy_alu_src src =
                  usrc = ureg_swizzle(usrc,
                              if (src.fabs)
         if (src.fneg)
               }
      /* Reswizzles a source so that the unset channels in the write mask still refer
   * to one of the channels present in the write mask.
   */
   static struct ureg_src
   ntr_swizzle_for_write_mask(struct ureg_src src, uint32_t write_mask)
   {
      assert(write_mask);
   int first_chan = ffs(write_mask) - 1;
   return ureg_swizzle(src,
                        }
      static struct ureg_dst
   ntr_get_ssa_def_decl(struct ntr_compile *c, nir_def *ssa)
   {
               struct ureg_dst dst;
   if (!ntr_try_store_ssa_in_tgsi_output(c, &dst, ssa))
                        }
      static struct ureg_dst
   ntr_get_chased_dest_decl(struct ntr_compile *c, nir_legacy_dest *dest)
   {
      if (dest->is_ssa)
         else
      }
      static struct ureg_dst
   ntr_get_chased_dest(struct ntr_compile *c, nir_legacy_dest *dest)
   {
               if (!dest->is_ssa) {
               if (dest->reg.indirect) {
      struct ureg_src offset = ntr_get_src(c, nir_src_for_ssa(dest->reg.indirect));
                     }
      static struct ureg_dst
   ntr_get_dest(struct ntr_compile *c, nir_def *def)
   {
      nir_legacy_dest chased = nir_legacy_chase_dest(def);
      }
      static struct ureg_dst
   ntr_get_alu_dest(struct ntr_compile *c, nir_def *def)
   {
      nir_legacy_alu_dest chased = nir_legacy_chase_alu_dest(def);
            if (chased.fsat)
            /* Only registers get write masks */
   if (chased.dest.is_ssa)
               }
      /* For an SSA dest being populated by a constant src, replace the storage with
   * a copy of the ureg_src.
   */
   static void
   ntr_store_def(struct ntr_compile *c, nir_def *def, struct ureg_src src)
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
   ntr_store(struct ntr_compile *c, nir_def *def, struct ureg_src src)
   {
               if (chased.is_ssa)
         else {
      struct ureg_dst dst = ntr_get_chased_dest(c, &chased);
         }
      static void
   ntr_emit_scalar(struct ntr_compile *c, unsigned tgsi_op,
                     {
               /* POW is the only 2-operand scalar op. */
   if (tgsi_op != TGSI_OPCODE_POW)
            for (i = 0; i < 4; i++) {
      if (dst.WriteMask & (1 << i)) {
      ntr_insn(c, tgsi_op,
            ureg_writemask(dst, 1 << i),
   ureg_scalar(src0, i),
         }
      static void
   ntr_emit_alu(struct ntr_compile *c, nir_alu_instr *instr)
   {
      struct ureg_src src[4];
   struct ureg_dst dst;
   unsigned i;
            /* Don't try to translate folded fsat since their source won't be valid */
   if (instr->op == nir_op_fsat && nir_legacy_fsat_folds(instr))
                     assert(num_srcs <= ARRAY_SIZE(src));
   for (i = 0; i < num_srcs; i++)
         for (; i < ARRAY_SIZE(src); i++)
                     static enum tgsi_opcode op_map[] = {
               [nir_op_fdot2_replicated] = TGSI_OPCODE_DP2,
   [nir_op_fdot3_replicated] = TGSI_OPCODE_DP3,
   [nir_op_fdot4_replicated] = TGSI_OPCODE_DP4,
   [nir_op_ffloor] = TGSI_OPCODE_FLR,
   [nir_op_ffract] = TGSI_OPCODE_FRC,
   [nir_op_fceil] = TGSI_OPCODE_CEIL,
            [nir_op_slt] = TGSI_OPCODE_SLT,
   [nir_op_sge] = TGSI_OPCODE_SGE,
   [nir_op_seq] = TGSI_OPCODE_SEQ,
            [nir_op_ftrunc] = TGSI_OPCODE_TRUNC,
   [nir_op_fddx] = TGSI_OPCODE_DDX,
   [nir_op_fddy] = TGSI_OPCODE_DDY,
   [nir_op_fddx_coarse] = TGSI_OPCODE_DDX,
   [nir_op_fddy_coarse] = TGSI_OPCODE_DDY,
   [nir_op_fadd] = TGSI_OPCODE_ADD,
            [nir_op_fmin] = TGSI_OPCODE_MIN,
   [nir_op_fmax] = TGSI_OPCODE_MAX,
               if (instr->op < ARRAY_SIZE(op_map) && op_map[instr->op] > 0) {
      /* The normal path for NIR to TGSI ALU op translation */
   ntr_insn(c, op_map[instr->op],
      } else {
               /* TODO: Use something like the ntr_store() path for the MOV calls so we
   * don't emit extra MOVs for swizzles/srcmods of inputs/const/imm.
            switch (instr->op) {
   case nir_op_fabs:
      /* Try to eliminate */
                  if (c->options->lower_fabs)
         else
               case nir_op_fsat:
                  case nir_op_fneg:
      /* Try to eliminate */
                                 /* NOTE: TGSI 32-bit math ops have the old "one source channel
   * replicated to all dst channels" behavior, while 64 is normal mapping
   * of src channels to dst.
      case nir_op_frcp:
                  case nir_op_frsq:
                  case nir_op_fexp2:
                  case nir_op_flog2:
                  case nir_op_fsin:
                  case nir_op_fcos:
                  case nir_op_fsub:
                  case nir_op_fmod:
                  case nir_op_fpow:
                  case nir_op_flrp:
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
               case nir_op_vec4:
   case nir_op_vec3:
   case nir_op_vec2:
            default:
      fprintf(stderr, "Unknown NIR opcode: %s\n", nir_op_infos[instr->op].name);
                     }
      static struct ureg_src
   ntr_ureg_src_indirect(struct ntr_compile *c, struct ureg_src usrc,
         {
      if (nir_src_is_const(src)) {
      usrc.Index += ntr_src_as_uint(c, src);
      } else {
            }
      static struct ureg_dst
   ntr_ureg_dst_indirect(struct ntr_compile *c, struct ureg_dst dst,
         {
      if (nir_src_is_const(src)) {
      dst.Index += ntr_src_as_uint(c, src);
      } else {
            }
      static struct ureg_dst
   ntr_ureg_dst_dimension_indirect(struct ntr_compile *c, struct ureg_dst udst,
         {
      if (nir_src_is_const(src)) {
         } else {
      return ureg_dst_dimension_indirect(udst,
               }
   /* Some load operations in NIR will have a fractional offset that we need to
   * swizzle down before storing to the result register.
   */
   static struct ureg_src
   ntr_shift_by_frac(struct ureg_src src, unsigned frac, unsigned num_components)
   {
      return ureg_swizzle(src,
                        }
         static void
   ntr_emit_load_ubo(struct ntr_compile *c, nir_intrinsic_instr *instr)
   {
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
   addr_temp = ntr_temp(c);
   ntr_UADD(c, addr_temp, ntr_get_src(c, instr->src[0]), ureg_imm1i(c->ureg, -c->first_ubo));
   src = ureg_src_dimension_indirect(src,
                     /* !PIPE_CAP_LOAD_CONSTBUF: Just emit it as a vec4 reference to the const
   * file.
   */
            if (nir_src_is_const(instr->src[1])) {
         } else {
                                       }
      static void
   ntr_emit_load_input(struct ntr_compile *c, nir_intrinsic_instr *instr)
   {
      uint32_t frac = nir_intrinsic_component(instr);
   uint32_t num_components = instr->num_components;
   unsigned base = nir_intrinsic_base(instr);
   struct ureg_src input;
            if (c->s->info.stage == MESA_SHADER_VERTEX) {
      input = ureg_DECL_vs_input(c->ureg, base);
   for (int i = 1; i < semantics.num_slots; i++)
      } else {
                           switch (instr->intrinsic) {
   case nir_intrinsic_load_input:
      input = ntr_ureg_src_indirect(c, input, instr->src[0], 0);
   ntr_store(c, &instr->def, input);
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
   ntr_INTERP_SAMPLE(c, ntr_get_dest(c, &instr->def), input,
               case nir_intrinsic_load_barycentric_at_offset:
      /* We stored the offset in the fake "bary" dest. */
   ntr_INTERP_OFFSET(c, ntr_get_dest(c, &instr->def), input,
               default:
         }
               default:
            }
      static void
   ntr_emit_store_output(struct ntr_compile *c, nir_intrinsic_instr *instr)
   {
               if (src.File == TGSI_FILE_OUTPUT) {
      /* If our src is the output file, that's an indication that we were able
   * to emit the output stores in the generating instructions and we have
   * nothing to do here.
   */
               uint32_t frac;
            if (instr->intrinsic == nir_intrinsic_store_per_vertex_output) {
      out = ntr_ureg_dst_indirect(c, out, instr->src[2]);
      } else {
                  uint8_t swizzle[4] = { 0, 0, 0, 0 };
   for (int i = frac; i < 4; i++) {
      if (out.WriteMask & (1 << i))
                           }
      static void
   ntr_emit_load_output(struct ntr_compile *c, nir_intrinsic_instr *instr)
   {
               /* ntr_try_store_in_tgsi_output() optimization is not valid if normal
   * load_output is present.
   */
   assert(c->s->info.stage != MESA_SHADER_VERTEX &&
            uint32_t frac;
            if (instr->intrinsic == nir_intrinsic_load_per_vertex_output) {
      out = ntr_ureg_dst_indirect(c, out, instr->src[1]);
      } else {
                  struct ureg_dst dst = ntr_get_dest(c, &instr->def);
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
   ntr_emit_load_sysval(struct ntr_compile *c, nir_intrinsic_instr *instr)
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
   switch (instr->intrinsic) {
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_instance_id:
      ntr_U2F(c, ntr_get_dest(c, &instr->def), sv);
         default:
                     }
      static void
   ntr_emit_intrinsic(struct ntr_compile *c, nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
      ntr_emit_load_ubo(c, instr);
               case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_invocation_id:
   case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_point_coord:
   case nir_intrinsic_load_front_face:
      ntr_emit_load_sysval(c, instr);
         case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
      ntr_emit_load_input(c, instr);
         case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      ntr_emit_store_output(c, instr);
         case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
      ntr_emit_load_output(c, instr);
         case nir_intrinsic_discard:
      ntr_KILL(c);
         case nir_intrinsic_discard_if: {
      struct ureg_src cond = ureg_scalar(ntr_get_src(c, instr->src[0]), 0);
   /* For !native_integers, the bool got lowered to 1.0 or 0.0. */
   ntr_KILL_IF(c, ureg_negate(cond));
      }
      /* In TGSI we don't actually generate the barycentric coords, and emit
   * interp intrinsics later.  However, we do need to store the
   * load_barycentric_at_* argument so that we can use it at that point.
      case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
         case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
      ntr_store(c, &instr->def, ntr_get_src(c, instr->src[0]));
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
      struct ntr_tex_operand_state {
      struct ureg_src srcs[4];
      };
      static void
   ntr_push_tex_arg(struct ntr_compile *c,
                     {
      int tex_src = nir_tex_instr_src_index(instr, tex_src_type);
   if (tex_src < 0)
            nir_src *src = &instr->src[tex_src].src;
      }
      static void
   ntr_emit_texture(struct ntr_compile *c, nir_tex_instr *instr)
   {
      struct ureg_dst dst = ntr_get_dest(c, &instr->def);
   enum tgsi_texture_type target = tgsi_texture_type_from_sampler_dim(instr->sampler_dim, instr->is_array, instr->is_shadow);
            int tex_handle_src = nir_tex_instr_src_index(instr, nir_tex_src_texture_handle);
            struct ureg_src sampler;
   if (tex_handle_src >= 0 && sampler_handle_src >= 0) {
      /* It seems we can't get separate tex/sampler on GL, just use one of the handles */
   sampler = ntr_get_src(c, instr->src[tex_handle_src].src);
      } else {
      assert(tex_handle_src == -1 && sampler_handle_src == -1);
   sampler = ureg_DECL_sampler(c->ureg, instr->sampler_index);
   int sampler_src = nir_tex_instr_src_index(instr, nir_tex_src_sampler_offset);
   if (sampler_src >= 0) {
      struct ureg_src reladdr = ntr_get_src(c, instr->src[sampler_src].src);
                  switch (instr->op) {
   case nir_texop_tex:
      if (nir_tex_instr_src_size(instr, nir_tex_instr_src_index(instr, nir_tex_src_backend1)) >
      MAX2(instr->coord_components, 2) + instr->is_shadow)
      else
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
                  struct ntr_tex_operand_state s = { .i = 0 };
   ntr_push_tex_arg(c, instr, nir_tex_src_backend1, &s);
            /* non-coord arg for TXQ */
   if (tex_opcode == TGSI_OPCODE_TXQ) {
      ntr_push_tex_arg(c, instr, nir_tex_src_lod, &s);
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
   s.srcs[s.i++] = ntr_get_src(c, instr->src[ddx].src);
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
            struct ntr_insn *insn = ntr_insn(c, tex_opcode, tex_dst, s.srcs[0], s.srcs[1], s.srcs[2], s.srcs[3]);
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
   ntr_emit_jump(struct ntr_compile *c, nir_jump_instr *jump)
   {
      switch (jump->type) {
   case nir_jump_break:
      ntr_BRK(c);
         case nir_jump_continue:
      ntr_CONT(c);
         default:
      fprintf(stderr, "Unknown jump instruction: ");
   nir_print_instr(&jump->instr, stderr);
   fprintf(stderr, "\n");
         }
      static void
   ntr_emit_ssa_undef(struct ntr_compile *c, nir_undef_instr *instr)
   {
      /* Nothing to do but make sure that we have some storage to deref. */
      }
      static void
   ntr_emit_instr(struct ntr_compile *c, nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_deref:
      /* ignored, will be walked by nir_intrinsic_image_*_deref. */
         case nir_instr_type_alu:
      ntr_emit_alu(c, nir_instr_as_alu(instr));
         case nir_instr_type_intrinsic:
      ntr_emit_intrinsic(c, nir_instr_as_intrinsic(instr));
         case nir_instr_type_load_const:
      /* Nothing to do here, as load consts are done directly from
   * ntr_get_src() (since many constant NIR srcs will often get folded
   * directly into a register file index instead of as a TGSI src).
   */
         case nir_instr_type_tex:
      ntr_emit_texture(c, nir_instr_as_tex(instr));
         case nir_instr_type_jump:
      ntr_emit_jump(c, nir_instr_as_jump(instr));
         case nir_instr_type_undef:
      ntr_emit_ssa_undef(c, nir_instr_as_undef(instr));
         default:
      fprintf(stderr, "Unknown NIR instr type: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\n");
         }
      static void
   ntr_emit_if(struct ntr_compile *c, nir_if *if_stmt)
   {
                        if (!nir_cf_list_is_empty_block(&if_stmt->else_list)) {
      ntr_ELSE(c);
                  }
      static void
   ntr_emit_loop(struct ntr_compile *c, nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
   ntr_BGNLOOP(c);
   ntr_emit_cf_list(c, &loop->body);
      }
      static void
   ntr_emit_block(struct ntr_compile *c, nir_block *block)
   {
      struct ntr_block *ntr_block = ntr_block_from_nir(c, block);
            nir_foreach_instr(instr, block) {
               /* Sanity check that we didn't accidentally ureg_OPCODE() instead of ntr_OPCODE(). */
   if (ureg_get_instruction_number(c->ureg) != 0) {
      fprintf(stderr, "Emitted ureg insn during: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\n");
                  /* Set up the if condition for ntr_emit_if(), which we have to do before
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
   ntr_emit_cf_list(struct ntr_compile *c, struct exec_list *list)
   {
      foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block:
                  case nir_cf_node_if:
                  case nir_cf_node_loop:
                  default:
               }
      static void
   ntr_emit_block_ureg(struct ntr_compile *c, struct nir_block *block)
   {
               /* Emit the ntr insns to tgsi_ureg. */
   util_dynarray_foreach(&ntr_block->insns, struct ntr_insn, insn) {
      const struct tgsi_opcode_info *opcode_info =
            switch (insn->opcode) {
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
      } else {
      ureg_insn(c->ureg, insn->opcode,
            insn->dst, opcode_info->num_dst,
            }
      static void
   ntr_emit_if_ureg(struct ntr_compile *c, nir_if *if_stmt)
   {
               int if_stack = c->current_if_else;
            /* Either the then or else block includes the ENDIF, which will fix up the
   * IF(/ELSE)'s label for jumping
   */
   ntr_emit_cf_list_ureg(c, &if_stmt->then_list);
               }
      static void
   ntr_emit_cf_list_ureg(struct ntr_compile *c, struct exec_list *list)
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
   ntr_emit_impl(struct ntr_compile *c, nir_function_impl *impl)
   {
               c->ssa_temp = rzalloc_array(c, struct ureg_src, impl->ssa_alloc);
            /* Set up the struct ntr_blocks to put insns in */
   c->blocks = _mesa_pointer_hash_table_create(c);
   nir_foreach_block(block, impl) {
      struct ntr_block *ntr_block = rzalloc(c->blocks, struct ntr_block);
   util_dynarray_init(&ntr_block->insns, ntr_block);
                           c->cur_block = ntr_block_from_nir(c, nir_start_block(impl));
   ntr_setup_inputs(c);
   ntr_setup_outputs(c);
            /* Emit the ntr insns */
            /* Don't do optimized RA if the driver requests it, unless the number of
   * temps is too large to be covered by the 16 bit signed int that TGSI
   * allocates for the register index */
   if (!c->options->unoptimized_ra || c->num_temps > 0x7fff)
         else
            /* Turn the ntr insns into actual TGSI tokens */
            ralloc_free(c->liveness);
         }
      static int
   type_size(const struct glsl_type *type, bool bindless)
   {
         }
      /* Allow vectorizing of ALU instructions.
   */
   static uint8_t
   ntr_should_vectorize_instr(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
               }
      static bool
   ntr_should_vectorize_io(unsigned align, unsigned bit_size,
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
   ntr_no_indirects_mask(nir_shader *s, struct pipe_screen *screen)
   {
      unsigned pipe_stage = pipe_shader_type_from_mesa(s->info.stage);
            if (!screen->get_shader_param(screen, pipe_stage,
                        if (!screen->get_shader_param(screen, pipe_stage,
                        if (!screen->get_shader_param(screen, pipe_stage,
                           }
      struct ntr_lower_tex_state {
      nir_scalar channels[8];
      };
      static void
   nir_to_rc_lower_tex_instr_arg(nir_builder *b,
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
   nir_to_rc_lower_tex_instr(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_tex)
                     if (nir_tex_instr_src_index(tex, nir_tex_src_coord) < 0)
                              nir_to_rc_lower_tex_instr_arg(b, tex, nir_tex_src_coord, &s);
   /* We always have at least two slots for the coordinate, even on 1D. */
            nir_to_rc_lower_tex_instr_arg(b, tex, nir_tex_src_comparator, &s);
                     /* XXX: LZ */
   nir_to_rc_lower_tex_instr_arg(b, tex, nir_tex_src_lod, &s);
   nir_to_rc_lower_tex_instr_arg(b, tex, nir_tex_src_projector, &s);
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
   nir_to_rc_lower_tex(nir_shader *s)
   {
      return nir_shader_instructions_pass(s,
                        }
      /* Lowers texture projectors if we can't do them as TGSI_OPCODE_TXP. */
   static void
   nir_to_rc_lower_txp(nir_shader *s)
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
      const void *
   nir_to_rc(struct nir_shader *s,
         {
      static const struct nir_to_rc_options default_ntr_options = {0};
      }
      /**
   * Translates the NIR shader to TGSI.
   *
   * This requires some lowering of the NIR shader to prepare it for translation.
   * We take ownership of the NIR shader passed, returning a reference to the new
   * TGSI tokens instead.  If you need to keep the NIR, then pass us a clone.
   */
   const void *nir_to_rc_options(struct nir_shader *s,
               {
      struct ntr_compile *c;
   const void *tgsi_tokens;
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
               NIR_PASS_V(s, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            nir_to_rc_lower_txp(s);
            if (!s->options->lower_uniforms_to_ubo) {
      NIR_PASS_V(s, nir_lower_uniforms_to_ubo,
                     if (!screen->get_param(screen, PIPE_CAP_LOAD_CONSTBUF))
            bool progress;
            /* Clean up after triginometric input normalization. */
   NIR_PASS_V(s, nir_opt_vectorize, ntr_should_vectorize_instr, NULL);
   do {
      progress = false;
      } while (progress);
   NIR_PASS_V(s, nir_copy_prop);
   NIR_PASS_V(s, nir_opt_cse);
   NIR_PASS_V(s, nir_opt_dce);
                     /* Lower demote_if to if (cond) { demote } because TGSI doesn't have a DEMOTE_IF. */
                     do {
      progress = false;
   NIR_PASS(progress, s, nir_opt_algebraic_late);
   if (progress) {
      NIR_PASS_V(s, nir_copy_prop);
   NIR_PASS_V(s, nir_opt_dce);
                  if (s->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(s, r300_nir_prepare_presubtract);
               NIR_PASS_V(s, nir_lower_int_to_float);
   NIR_PASS_V(s, nir_lower_bool_to_float,
         /* bool_to_float generates MOVs for b2f32 that we want to clean up. */
   NIR_PASS_V(s, nir_copy_prop);
            nir_move_options move_all =
      nir_move_const_undef | nir_move_load_ubo | nir_move_load_input |
         NIR_PASS_V(s, nir_opt_move, move_all);
            NIR_PASS_V(s, nir_convert_from_ssa, true);
            /* locals_to_reg_intrinsics will leave dead derefs that are good to clean up.
   */
   NIR_PASS_V(s, nir_lower_locals_to_regs, 32);
            /* See comment in ntr_get_alu_src for supported modifiers */
            if (NIR_DEBUG(TGSI)) {
      fprintf(stderr, "NIR before translation to TGSI:\n");
               c = rzalloc(NULL, struct ntr_compile);
   c->screen = screen;
            c->s = s;
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
   ntr_emit_impl(c, impl);
                     if (NIR_DEBUG(TGSI)) {
      fprintf(stderr, "TGSI after translation from NIR:\n");
                        ralloc_free(c);
               }
