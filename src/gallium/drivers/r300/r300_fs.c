   /*
   * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
   *                Joakim Sindholt <opensource@zhasha.com>
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
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_ureg.h"
      #include "r300_cb.h"
   #include "r300_context.h"
   #include "r300_emit.h"
   #include "r300_screen.h"
   #include "r300_fs.h"
   #include "r300_reg.h"
   #include "r300_texture.h"
   #include "r300_tgsi_to_rc.h"
      #include "compiler/radeon_compiler.h"
      /* Convert info about FS input semantics to r300_shader_semantics. */
   void r300_shader_read_fs_inputs(struct tgsi_shader_info* info,
         {
      int i;
                     for (i = 0; i < info->num_inputs; i++) {
               switch (info->input_semantic_name[i]) {
         case TGSI_SEMANTIC_COLOR:
                        case TGSI_SEMANTIC_PCOORD:
                  case TGSI_SEMANTIC_TEXCOORD:
      assert(index < ATTR_TEXCOORD_COUNT);
                     case TGSI_SEMANTIC_GENERIC:
      assert(index < ATTR_GENERIC_COUNT);
                     case TGSI_SEMANTIC_FOG:
                        case TGSI_SEMANTIC_POSITION:
                        case TGSI_SEMANTIC_FACE:
                        default:
               }
      static void find_output_registers(struct r300_fragment_program_compiler * compiler,
         {
               /* Mark the outputs as not present initially */
   compiler->OutputColor[0] = shader->info.num_outputs;
   compiler->OutputColor[1] = shader->info.num_outputs;
   compiler->OutputColor[2] = shader->info.num_outputs;
   compiler->OutputColor[3] = shader->info.num_outputs;
            /* Now see where they really are. */
   for(i = 0; i < shader->info.num_outputs; ++i) {
      switch(shader->info.output_semantic_name[i]) {
         case TGSI_SEMANTIC_COLOR:
      compiler->OutputColor[shader->info.output_semantic_index[i]] = i;
      case TGSI_SEMANTIC_POSITION:
               }
      static void allocate_hardware_inputs(
      struct r300_fragment_program_compiler * c,
   void (*allocate)(void * data, unsigned input, unsigned hwreg),
      {
      struct r300_shader_semantics* inputs =
                  /* Allocate input registers. */
   for (i = 0; i < ATTR_COLOR_COUNT; i++) {
      if (inputs->color[i] != ATTR_UNUSED) {
            }
   if (inputs->face != ATTR_UNUSED) {
         }
   for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
      if (inputs->generic[i] != ATTR_UNUSED) {
            }
   for (i = 0; i < ATTR_TEXCOORD_COUNT; i++) {
      if (inputs->texcoord[i] != ATTR_UNUSED) {
            }
   if (inputs->pcoord != ATTR_UNUSED) {
         }
   if (inputs->fog != ATTR_UNUSED) {
         }
   if (inputs->wpos != ATTR_UNUSED) {
            }
      void r300_fragment_program_get_external_state(
      struct r300_context* r300,
      {
      struct r300_textures_state *texstate = r300->textures_state.state;
                     for (i = 0; i < texstate->sampler_state_count; i++) {
      struct r300_sampler_state *s = texstate->sampler_states[i];
   struct r300_sampler_view *v = texstate->sampler_views[i];
            if (!s || !v) {
                           if (s->state.compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
                  /* Fortunately, no need to translate this. */
            /* Pass texture swizzling to the compiler, some lowering passes need it. */
   if (state->unit[i].compare_mode_enabled) {
         state->unit[i].texture_swizzle =
                  /* XXX this should probably take into account STR, not just S. */
   if (t->tex.is_npot) {
         switch (s->state.wrap_s) {
   case PIPE_TEX_WRAP_REPEAT:
                  case PIPE_TEX_WRAP_MIRROR_REPEAT:
                  case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
                  default:
                  if (t->b.target == PIPE_TEXTURE_3D)
         }
      static void r300_translate_fragment_shader(
      struct r300_context* r300,
   struct r300_fragment_shader_code* shader,
         static void r300_dummy_fragment_shader(
      struct r300_context* r300,
      {
      struct pipe_shader_state state;
   struct ureg_program *ureg;
   struct ureg_dst out;
            /* Make a simple fragment shader which outputs (0, 0, 0, 1) */
   ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
            ureg_MOV(ureg, out, imm);
                     shader->dummy = true;
               }
      static void r300_emit_fs_code_to_buffer(
      struct r300_context *r300,
      {
      struct rX00_fragment_program_code *generic_code = &shader->code;
   unsigned imm_count = shader->immediates_count;
   unsigned imm_first = shader->externals_count;
   unsigned imm_end = generic_code->constants.Count;
   struct rc_constant *constants = generic_code->constants.Constants;
   unsigned i;
            if (r300->screen->caps.is_r500) {
               shader->cb_code_size = 19 +
                        NEW_CB(shader->cb_code, shader->cb_code_size);
   OUT_CB_REG(R500_US_CONFIG, R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO);
   OUT_CB_REG(R500_US_PIXSIZE, code->max_temp_idx);
   OUT_CB_REG(R500_US_FC_CTRL, code->us_fc_ctrl);
   for(i = 0; i < code->int_constant_count; i++){
               }
   OUT_CB_REG(R500_US_CODE_RANGE,
         OUT_CB_REG(R500_US_CODE_OFFSET, 0);
   OUT_CB_REG(R500_US_CODE_ADDR,
            OUT_CB_REG(R500_GA_US_VECTOR_INDEX, R500_GA_US_VECTOR_INDEX_TYPE_INSTR);
   OUT_CB_ONE_REG(R500_GA_US_VECTOR_DATA, (code->inst_end + 1) * 6);
   for (i = 0; i <= code->inst_end; i++) {
         OUT_CB(code->inst[i].inst0);
   OUT_CB(code->inst[i].inst1);
   OUT_CB(code->inst[i].inst2);
   OUT_CB(code->inst[i].inst3);
   OUT_CB(code->inst[i].inst4);
            /* Emit immediates. */
   if (imm_count) {
         for(i = imm_first; i < imm_end; ++i) {
                        OUT_CB_REG(R500_GA_US_VECTOR_INDEX,
               OUT_CB_ONE_REG(R500_GA_US_VECTOR_DATA, 4);
            } else { /* r300 */
      struct r300_fragment_program_code *code = &generic_code->code.r300;
   unsigned int alu_length = code->alu.length;
   unsigned int alu_iterations = ((alu_length - 1) / 64) + 1;
   unsigned int tex_length = code->tex.length;
   unsigned int tex_iterations =
         unsigned int iterations =
                  shader->cb_code_size = 15 +
         /* R400_US_CODE_BANK */
   (r300->screen->caps.is_r400 ? 2 * (iterations + 1): 0) +
   /* R400_US_CODE_EXT */
   (r300->screen->caps.is_r400 ? 2 : 0) +
   /* R300_US_ALU_{RGB,ALPHA}_{INST,ADDR}_0, R400_US_ALU_EXT_ADDR_0 */
   (code->r390_mode ? (5 * alu_iterations) : 4) +
   /* R400_US_ALU_EXT_ADDR_[0-63] */
   (code->r390_mode ? (code->alu.length) : 0) +
   /* R300_US_ALU_{RGB,ALPHA}_{INST,ADDR}_0 */
   code->alu.length * 4 +
   /* R300_US_TEX_INST_0, R300_US_TEX_INST_[0-31] */
                     OUT_CB_REG(R300_US_CONFIG, code->config);
   OUT_CB_REG(R300_US_PIXSIZE, code->pixsize);
            if (code->r390_mode) {
         } else if (r300->screen->caps.is_r400) {
         /* This register appears to affect shaders even if r390_mode is
   * disabled, so it needs to be set to 0 for shaders that
   * don't use r390_mode. */
            OUT_CB_REG_SEQ(R300_US_CODE_ADDR_0, 4);
            do {
         unsigned int bank_alu_length = (alu_length < 64 ? alu_length : 64);
   unsigned int bank_alu_offset = bank * 64;
                  if (r300->screen->caps.is_r400) {
                        if (bank_alu_length > 0) {
                                                                                          if (code->r390_mode) {
      OUT_CB_REG_SEQ(R400_US_ALU_EXT_ADDR_0, bank_alu_length);
   for (i = 0; i < bank_alu_length; i++)
                  if (bank_tex_length > 0) {
                        alu_length -= bank_alu_length;
   tex_length -= bank_tex_length;
            /* R400_US_CODE_BANK needs to be reset to 0, otherwise some shaders
         if (r300->screen->caps.is_r400) {
         OUT_CB_REG(R400_US_CODE_BANK,
            /* Emit immediates. */
   if (imm_count) {
         for(i = imm_first; i < imm_end; ++i) {
                        OUT_CB_REG_SEQ(R300_PFS_PARAM_0_X + i * 16, 4);
   OUT_CB(pack_float24(data[0]));
   OUT_CB(pack_float24(data[1]));
   OUT_CB(pack_float24(data[2]));
                     OUT_CB_REG(R300_FG_DEPTH_SRC, shader->fg_depth_src);
   OUT_CB_REG(R300_US_W_FMT, shader->us_out_w);
      }
      static void r300_translate_fragment_shader(
      struct r300_context* r300,
   struct r300_fragment_shader_code* shader,
      {
      struct r300_fragment_program_compiler compiler;
   struct tgsi_to_rc ttr;
   int wpos, face;
            tgsi_scan_shader(tokens, &shader->info);
            wpos = shader->inputs.wpos;
            /* Setup the compiler. */
   memset(&compiler, 0, sizeof(compiler));
   rc_init(&compiler.Base, &r300->fs_regalloc_state);
            compiler.code = &shader->code;
   compiler.state = shader->compare_state;
   if (!shader->dummy)
         compiler.Base.is_r500 = r300->screen->caps.is_r500;
   compiler.Base.is_r400 = r300->screen->caps.is_r400;
   compiler.Base.disable_optimizations = DBG_ON(r300, DBG_NO_OPT);
   compiler.Base.has_half_swizzles = true;
   compiler.Base.has_presub = true;
   compiler.Base.has_omod = true;
   compiler.Base.max_temp_regs =
         compiler.Base.max_constants = compiler.Base.is_r500 ? 256 : 32;
   compiler.Base.max_alu_insts =
         compiler.Base.max_tex_insts =
         compiler.AllocateHwInputs = &allocate_hardware_inputs;
                     shader->write_all =
            if (compiler.Base.Debug & RC_DBG_LOG) {
      DBG(r300, DBG_FP, "r300: Initial fragment program\n");
               /* Translate TGSI to our internal representation */
   ttr.compiler = &compiler.Base;
                     if (ttr.error) {
      fprintf(stderr, "r300 FP: Cannot translate a shader. "
         r300_dummy_fragment_shader(r300, shader);
               if (!r300->screen->caps.is_r500 ||
      compiler.Base.Program.Constants.Count > 200) {
               /**
   * Transform the program to support WPOS.
   *
   * Introduce a small fragment at the start of the program that will be
   * the only code that directly reads the WPOS input.
   * All other code pieces that reference that input will be rewritten
   * to read from a newly allocated temporary. */
   if (wpos != ATTR_UNUSED) {
      /* Moving the input to some other reg is not really necessary. */
               if (face != ATTR_UNUSED) {
                  /* Invoke the compiler */
            if (compiler.Base.Error) {
      fprintf(stderr, "r300 FP: Compiler Error:\n%sUsing a dummy shader"
            if (shader->dummy) {
         fprintf(stderr, "r300 FP: Cannot compile the dummy shader! "
                  rc_destroy(&compiler.Base);
   r300_dummy_fragment_shader(r300, shader);
               /* Shaders with zero instructions are invalid,
   * use the dummy shader instead. */
   if (shader->code.code.r500.inst_end == -1) {
      rc_destroy(&compiler.Base);
   r300_dummy_fragment_shader(r300, shader);
               /* Initialize numbers of constants for each type. */
   shader->externals_count = 0;
   for (i = 0;
         i < shader->code.constants.Count &&
         }
   shader->immediates_count = 0;
            for (i = shader->externals_count; i < shader->code.constants.Count; i++) {
      switch (shader->code.constants.Constants[i].Type) {
         case RC_CONSTANT_IMMEDIATE:
      ++shader->immediates_count;
      case RC_CONSTANT_STATE:
      ++shader->rc_state_count;
      default:
               /* Setup shader depth output. */
   if (shader->code.writes_depth) {
      shader->fg_depth_src = R300_FG_DEPTH_SRC_SHADER;
      } else {
      shader->fg_depth_src = R300_FG_DEPTH_SRC_SCAN;
               /* And, finally... */
            /* Build the command buffer. */
      }
      bool r300_pick_fragment_shader(struct r300_context *r300,
               {
               if (!fs->first) {
      /* Build the fragment shader for the first time. */
            memcpy(&fs->shader->compare_state, state, sizeof(*state));
   r300_translate_fragment_shader(r300, fs->shader, fs->state.tokens);
         } else {
      /* Check if the currently-bound shader has been compiled
         if (memcmp(&fs->shader->compare_state, state, sizeof(*state)) != 0) {
         /* Search for the right shader. */
   ptr = fs->first;
   while (ptr) {
      if (memcmp(&ptr->compare_state, state, sizeof(*state)) == 0) {
      if (fs->shader != ptr) {
         fs->shader = ptr;
   }
   /* The currently-bound one is OK. */
                        /* Not found, gotta compile a new one. */
   ptr = CALLOC_STRUCT(r300_fragment_shader_code);
                  memcpy(&ptr->compare_state, state, sizeof(*state));
   r300_translate_fragment_shader(r300, ptr, fs->state.tokens);
                  }
