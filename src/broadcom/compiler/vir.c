   /*
   * Copyright © 2016-2017 Broadcom
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
      #include "broadcom/common/v3d_device_info.h"
   #include "v3d_compiler.h"
   #include "util/u_prim.h"
   #include "compiler/nir/nir_schedule.h"
   #include "compiler/nir/nir_builder.h"
      int
   vir_get_nsrc(struct qinst *inst)
   {
         switch (inst->qpu.type) {
   case V3D_QPU_INSTR_TYPE_BRANCH:
         case V3D_QPU_INSTR_TYPE_ALU:
            if (inst->qpu.alu.add.op != V3D_QPU_A_NOP)
                     }
      /**
   * Returns whether the instruction has any side effects that must be
   * preserved.
   */
   bool
   vir_has_side_effects(struct v3d_compile *c, struct qinst *inst)
   {
         switch (inst->qpu.type) {
   case V3D_QPU_INSTR_TYPE_BRANCH:
         case V3D_QPU_INSTR_TYPE_ALU:
            switch (inst->qpu.alu.add.op) {
   case V3D_QPU_A_SETREVF:
   case V3D_QPU_A_SETMSF:
   case V3D_QPU_A_VPMSETUP:
   case V3D_QPU_A_STVPMV:
   case V3D_QPU_A_STVPMD:
   case V3D_QPU_A_STVPMP:
   case V3D_QPU_A_VPMWT:
   case V3D_QPU_A_TMUWT:
                              switch (inst->qpu.alu.mul.op) {
   case V3D_QPU_M_MULTOP:
         default:
               if (inst->qpu.sig.ldtmu ||
         inst->qpu.sig.ldvary ||
   inst->qpu.sig.ldtlbu ||
   inst->qpu.sig.ldtlb ||
   inst->qpu.sig.wrtmuc ||
   inst->qpu.sig.thrsw) {
            /* ldunifa works like ldunif: it reads an element and advances the
      * pointer, so each read has a side effect (we don't care for ldunif
   * because we reconstruct the uniform stream buffer after compiling
   * with the surviving uniforms), so allowing DCE to remove
   * one would break follow-up loads. We could fix this by emitting a
   * unifa for each ldunifa, but each unifa requires 3 delay slots
   * before a ldunifa, so that would be quite expensive.
      if (inst->qpu.sig.ldunifa || inst->qpu.sig.ldunifarf)
            }
      bool
   vir_is_raw_mov(struct qinst *inst)
   {
         if (inst->qpu.type != V3D_QPU_INSTR_TYPE_ALU ||
         (inst->qpu.alu.mul.op != V3D_QPU_M_FMOV &&
   inst->qpu.alu.mul.op != V3D_QPU_M_MOV)) {
            if (inst->qpu.alu.add.output_pack != V3D_QPU_PACK_NONE ||
         inst->qpu.alu.mul.output_pack != V3D_QPU_PACK_NONE) {
            if (inst->qpu.alu.add.a.unpack != V3D_QPU_UNPACK_NONE ||
         inst->qpu.alu.add.b.unpack != V3D_QPU_UNPACK_NONE ||
   inst->qpu.alu.mul.a.unpack != V3D_QPU_UNPACK_NONE ||
   inst->qpu.alu.mul.b.unpack != V3D_QPU_UNPACK_NONE) {
            if (inst->qpu.flags.ac != V3D_QPU_COND_NONE ||
                  }
      bool
   vir_is_add(struct qinst *inst)
   {
         return (inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
   }
      bool
   vir_is_mul(struct qinst *inst)
   {
         return (inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
   }
      bool
   vir_is_tex(const struct v3d_device_info *devinfo, struct qinst *inst)
   {
         if (inst->dst.file == QFILE_MAGIC)
            if (inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
         inst->qpu.alu.add.op == V3D_QPU_A_TMUWT) {
            }
      bool
   vir_writes_r3_implicitly(const struct v3d_device_info *devinfo,
         {
         if (!devinfo->has_accumulators)
            for (int i = 0; i < vir_get_nsrc(inst); i++) {
            switch (inst->src[i].file) {
   case QFILE_VPM:
         default:
               if (devinfo->ver < 41 && (inst->qpu.sig.ldvary ||
                                    }
      bool
   vir_writes_r4_implicitly(const struct v3d_device_info *devinfo,
         {
         if (!devinfo->has_accumulators)
            switch (inst->dst.file) {
   case QFILE_MAGIC:
            switch (inst->dst.index) {
   case V3D_QPU_WADDR_RECIP:
   case V3D_QPU_WADDR_RSQRT:
   case V3D_QPU_WADDR_EXP:
   case V3D_QPU_WADDR_LOG:
   case V3D_QPU_WADDR_SIN:
            default:
                  if (devinfo->ver < 41 && inst->qpu.sig.ldtmu)
            }
      void
   vir_set_unpack(struct qinst *inst, int src,
         {
                  if (vir_is_add(inst)) {
            if (src == 0)
            } else {
            assert(vir_is_mul(inst));
   if (src == 0)
            }
      void
   vir_set_pack(struct qinst *inst, enum v3d_qpu_output_pack pack)
   {
         if (vir_is_add(inst)) {
         } else {
               }
      void
   vir_set_cond(struct qinst *inst, enum v3d_qpu_cond cond)
   {
         if (vir_is_add(inst)) {
         } else {
               }
      enum v3d_qpu_cond
   vir_get_cond(struct qinst *inst)
   {
                  if (vir_is_add(inst))
         else if (vir_is_mul(inst))
         else /* NOP */
   }
      void
   vir_set_pf(struct v3d_compile *c, struct qinst *inst, enum v3d_qpu_pf pf)
   {
         c->flags_temp = -1;
   if (vir_is_add(inst)) {
         } else {
               }
      void
   vir_set_uf(struct v3d_compile *c, struct qinst *inst, enum v3d_qpu_uf uf)
   {
         c->flags_temp = -1;
   if (vir_is_add(inst)) {
         } else {
               }
      #if 0
   uint8_t
   vir_channels_written(struct qinst *inst)
   {
         if (vir_is_mul(inst)) {
            switch (inst->dst.pack) {
   case QPU_PACK_MUL_NOP:
   case QPU_PACK_MUL_8888:
         case QPU_PACK_MUL_8A:
         case QPU_PACK_MUL_8B:
         case QPU_PACK_MUL_8C:
         case QPU_PACK_MUL_8D:
      } else {
            switch (inst->dst.pack) {
   case QPU_PACK_A_NOP:
   case QPU_PACK_A_8888:
   case QPU_PACK_A_8888_SAT:
   case QPU_PACK_A_32_SAT:
         case QPU_PACK_A_8A:
   case QPU_PACK_A_8A_SAT:
         case QPU_PACK_A_8B:
   case QPU_PACK_A_8B_SAT:
         case QPU_PACK_A_8C:
   case QPU_PACK_A_8C_SAT:
         case QPU_PACK_A_8D:
   case QPU_PACK_A_8D_SAT:
         case QPU_PACK_A_16A:
   case QPU_PACK_A_16A_SAT:
         case QPU_PACK_A_16B:
   case QPU_PACK_A_16B_SAT:
      }
   }
   #endif
      struct qreg
   vir_get_temp(struct v3d_compile *c)
   {
                  reg.file = QFILE_TEMP;
            if (c->num_temps > c->defs_array_size) {
                           c->defs = reralloc(c, c->defs, struct qinst *,
                        c->spillable = reralloc(c, c->spillable,
                           }
      struct qinst *
   vir_add_inst(enum v3d_qpu_add_op op, struct qreg dst, struct qreg src0, struct qreg src1)
   {
                  inst->qpu = v3d_qpu_nop();
            inst->dst = dst;
   inst->src[0] = src0;
   inst->src[1] = src1;
                     }
      struct qinst *
   vir_mul_inst(enum v3d_qpu_mul_op op, struct qreg dst, struct qreg src0, struct qreg src1)
   {
                  inst->qpu = v3d_qpu_nop();
            inst->dst = dst;
   inst->src[0] = src0;
   inst->src[1] = src1;
                     }
      struct qinst *
   vir_branch_inst(struct v3d_compile *c, enum v3d_qpu_branch_cond cond)
   {
                  inst->qpu = v3d_qpu_nop();
   inst->qpu.type = V3D_QPU_INSTR_TYPE_BRANCH;
   inst->qpu.branch.cond = cond;
   inst->qpu.branch.msfign = V3D_QPU_MSFIGN_NONE;
   inst->qpu.branch.bdi = V3D_QPU_BRANCH_DEST_REL;
   inst->qpu.branch.ub = true;
            inst->dst = vir_nop_reg();
                     }
      static void
   vir_emit(struct v3d_compile *c, struct qinst *inst)
   {
                  switch (c->cursor.mode) {
   case vir_cursor_add:
               case vir_cursor_addtail:
                        c->cursor = vir_after_inst(inst);
   }
      /* Updates inst to write to a new temporary, emits it, and notes the def. */
   struct qreg
   vir_emit_def(struct v3d_compile *c, struct qinst *inst)
   {
                  /* If we're emitting an instruction that's a def, it had better be
      * writing a register.
      if (inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU) {
            assert(inst->qpu.alu.add.op == V3D_QPU_A_NOP ||
                              if (inst->dst.file == QFILE_TEMP)
                     }
      struct qinst *
   vir_emit_nondef(struct v3d_compile *c, struct qinst *inst)
   {
         if (inst->dst.file == QFILE_TEMP)
                     }
      struct qblock *
   vir_new_block(struct v3d_compile *c)
   {
                           block->predecessors = _mesa_set_create(block,
                           }
      void
   vir_set_emit_block(struct v3d_compile *c, struct qblock *block)
   {
         c->cur_block = block;
   c->cursor = vir_after_block(block);
   }
      struct qblock *
   vir_entry_block(struct v3d_compile *c)
   {
         }
      struct qblock *
   vir_exit_block(struct v3d_compile *c)
   {
         }
      void
   vir_link_blocks(struct qblock *predecessor, struct qblock *successor)
   {
         _mesa_set_add(successor->predecessors, predecessor);
   if (predecessor->successors[0]) {
               } else {
         }
      const struct v3d_compiler *
   v3d_compiler_init(const struct v3d_device_info *devinfo,
         {
         struct v3d_compiler *compiler = rzalloc(NULL, struct v3d_compiler);
   if (!compiler)
            compiler->devinfo = devinfo;
            if (!vir_init_reg_sets(compiler)) {
                        }
      void
   v3d_compiler_free(const struct v3d_compiler *compiler)
   {
         }
      struct v3d_compiler_strategy {
         const char *name;
   uint32_t max_threads;
   uint32_t min_threads;
   bool disable_general_tmu_sched;
   bool disable_gcm;
   bool disable_loop_unrolling;
   bool disable_ubo_load_sorting;
   bool move_buffer_loads;
   bool disable_tmu_pipelining;
   };
      static struct v3d_compile *
   vir_compile_init(const struct v3d_compiler *compiler,
                  struct v3d_key *key,
   nir_shader *s,
   void (*debug_output)(const char *msg,
         void *debug_output_data,
   int program_id, int variant_id,
      {
                  c->compiler = compiler;
   c->devinfo = compiler->devinfo;
   c->key = key;
   c->program_id = program_id;
   c->variant_id = variant_id;
   c->compile_strategy_idx = compile_strategy_idx;
   c->threads = strategy->max_threads;
   c->debug_output = debug_output;
   c->debug_output_data = debug_output_data;
   c->compilation_result = V3D_COMPILATION_SUCCEEDED;
   c->min_threads_for_reg_alloc = strategy->min_threads;
   c->max_tmu_spills = strategy->max_tmu_spills;
   c->fallback_scheduler = fallback_scheduler;
   c->disable_general_tmu_sched = strategy->disable_general_tmu_sched;
   c->disable_tmu_pipelining = strategy->disable_tmu_pipelining;
   c->disable_constant_ubo_load_sorting = strategy->disable_ubo_load_sorting;
   c->move_buffer_loads = strategy->move_buffer_loads;
   c->disable_gcm = strategy->disable_gcm;
   c->disable_loop_unrolling = V3D_DBG(NO_LOOP_UNROLL)
               s = nir_shader_clone(c, s);
            list_inithead(&c->blocks);
            c->output_position_index = -1;
            c->def_ht = _mesa_hash_table_create(c, _mesa_hash_pointer,
            c->tmu.outstanding_regs = _mesa_pointer_set_create(c);
            }
      static int
   type_size_vec4(const struct glsl_type *type, bool bindless)
   {
         }
      static enum nir_lower_tex_packing
   lower_tex_packing_cb(const nir_tex_instr *tex, const void *data)
   {
               int sampler_index = nir_tex_instr_need_sampler(tex) ?
            assert(sampler_index < c->key->num_samplers_used);
   return c->key->sampler[sampler_index].return_size == 16 ?
      }
      static void
   v3d_lower_nir(struct v3d_compile *c)
   {
         struct nir_lower_tex_options tex_options = {
                                 .lower_rect = false, /* XXX: Use this on V3D 3.x */
   .lower_txp = ~0,
   /* Apply swizzles to all samplers. */
               /* Lower the format swizzle and (for 32-bit returns)
      * ARB_texture_swizzle-style swizzle.
      assert(c->key->num_tex_used <= ARRAY_SIZE(c->key->tex));
   for (int i = 0; i < c->key->num_tex_used; i++) {
                        tex_options.lower_tex_packing_cb = lower_tex_packing_cb;
            NIR_PASS(_, c->s, nir_lower_tex, &tex_options);
            if (c->s->info.zero_initialize_shared_memory &&
         c->s->info.shared_size > 0) {
      /* All our BOs allocate full pages, so the underlying allocation
   * for shared memory will always be a multiple of 4KB. This
   * ensures that we can do an exact number of full chunk_size
   * writes to initialize the memory independently of the actual
   * shared_size used by the shader, which is a requirement of
   * the initialization pass.
   */
   const unsigned chunk_size = 16; /* max single store size */
                        NIR_PASS(_, c->s, nir_lower_vars_to_scratch,
            nir_var_function_temp,
      }
      static void
   v3d_set_prog_data_uniforms(struct v3d_compile *c,
         {
         int count = c->num_uniforms;
            ulist->count = count;
   ulist->data = ralloc_array(prog_data, uint32_t, count);
   memcpy(ulist->data, c->uniform_data,
         ulist->contents = ralloc_array(prog_data, enum quniform_contents, count);
   memcpy(ulist->contents, c->uniform_contents,
   }
      static void
   v3d_vs_set_prog_data(struct v3d_compile *c,
         {
         /* The vertex data gets format converted by the VPM so that
      * each attribute channel takes up a VPM column.  Precompute
   * the sizes for the shader record.
      for (int i = 0; i < ARRAY_SIZE(prog_data->vattr_sizes); i++) {
                        memset(prog_data->driver_location_map, -1,
            nir_foreach_shader_in_variable(var, c->s) {
                        prog_data->uses_vid = BITSET_TEST(c->s->info.system_values_read,
                        prog_data->uses_biid = BITSET_TEST(c->s->info.system_values_read,
            prog_data->uses_iid = BITSET_TEST(c->s->info.system_values_read,
                        if (prog_data->uses_vid)
         if (prog_data->uses_biid)
         if (prog_data->uses_iid)
            /* Input/output segment size are in sectors (8 rows of 32 bits per
      * channel).
      prog_data->vpm_input_size = align(prog_data->vpm_input_size, 8) / 8;
            /* Set us up for shared input/output segments.  This is apparently
      * necessary for our VCM setup to avoid varying corruption.
   *
   * FIXME: initial testing on V3D 7.1 seems to work fine when using
   * separate segments. So we could try to reevaluate in the future, if
   * there is any advantage of using separate segments.
      prog_data->separate_segments = false;
   prog_data->vpm_output_size = MAX2(prog_data->vpm_output_size,
                  /* Compute VCM cache size.  We set up our program to take up less than
      * half of the VPM, so that any set of bin and render programs won't
   * run out of space.  We need space for at least one input segment,
   * and then allocate the rest to output segments (one for the current
   * program, the rest to VCM).  The valid range of the VCM cache size
   * field is 1-4 16-vertex batches, but GFXH-1744 limits us to 2-4
   * batches.
      assert(c->devinfo->vpm_size);
   int sector_size = V3D_CHANNELS * sizeof(uint32_t) * 8;
   int vpm_size_in_sectors = c->devinfo->vpm_size / sector_size;
   int half_vpm = vpm_size_in_sectors / 2;
   int vpm_output_sectors = half_vpm - prog_data->vpm_input_size;
   int vpm_output_batches = vpm_output_sectors / prog_data->vpm_output_size;
   assert(vpm_output_batches >= 2);
   }
      static void
   v3d_gs_set_prog_data(struct v3d_compile *c,
         {
         prog_data->num_inputs = c->num_inputs;
   memcpy(prog_data->input_slots, c->input_slots,
            /* gl_PrimitiveIdIn is written by the GBG into the first word of the
      * VPM output header automatically and the shader will overwrite
   * it after reading it if necessary, so it doesn't add to the VPM
   * size requirements.
      prog_data->uses_pid = BITSET_TEST(c->s->info.system_values_read,
            /* Output segment size is in sectors (8 rows of 32 bits per channel) */
            /* Compute SIMD dispatch width and update VPM output size accordingly
      * to ensure we can fit our program in memory. Available widths are
   * 16, 8, 4, 1.
   *
   * Notice that at draw time we will have to consider VPM memory
   * requirements from other stages and choose a smaller dispatch
   * width if needed to fit the program in VPM memory.
      prog_data->simd_width = 16;
   while ((prog_data->simd_width > 1 && prog_data->vpm_output_size > 16) ||
            prog_data->simd_width == 2) {
   prog_data->simd_width >>= 1;
      }
   assert(prog_data->vpm_output_size <= 16);
            prog_data->out_prim_type = c->s->info.gs.output_primitive;
            prog_data->writes_psiz =
   }
      static void
   v3d_set_fs_prog_data_inputs(struct v3d_compile *c,
         {
         prog_data->num_inputs = c->num_inputs;
   memcpy(prog_data->input_slots, c->input_slots,
            STATIC_ASSERT(ARRAY_SIZE(prog_data->flat_shade_flags) >
         for (int i = 0; i < V3D_MAX_FS_INPUTS; i++) {
                                             }
      static void
   v3d_fs_set_prog_data(struct v3d_compile *c,
         {
         v3d_set_fs_prog_data_inputs(c, prog_data);
   prog_data->writes_z = c->writes_z;
   prog_data->writes_z_from_fep = c->writes_z_from_fep;
   prog_data->disable_ez = !c->s->info.fs.early_fragment_tests;
   prog_data->uses_center_w = c->uses_center_w;
   prog_data->uses_implicit_point_line_varyings =
         prog_data->lock_scoreboard_on_first_thrsw =
         prog_data->force_per_sample_msaa = c->s->info.fs.uses_sample_shading;
   }
      static void
   v3d_cs_set_prog_data(struct v3d_compile *c,
         {
                  prog_data->local_size[0] = c->s->info.workgroup_size[0];
   prog_data->local_size[1] = c->s->info.workgroup_size[1];
            }
      static void
   v3d_set_prog_data(struct v3d_compile *c,
         {
         prog_data->threads = c->threads;
   prog_data->single_seg = !c->last_thrsw;
   prog_data->spill_size = c->spill_size;
   prog_data->tmu_spills = c->spills;
   prog_data->tmu_fills = c->fills;
   prog_data->tmu_count = c->tmu.total_count;
   prog_data->qpu_read_stalls = c->qpu_inst_stalled_count;
   prog_data->compile_strategy_idx = c->compile_strategy_idx;
   prog_data->tmu_dirty_rcl = c->tmu_dirty_rcl;
   prog_data->has_control_barrier = c->s->info.uses_control_barrier;
                     switch (c->s->info.stage) {
   case MESA_SHADER_VERTEX:
               case MESA_SHADER_GEOMETRY:
               case MESA_SHADER_FRAGMENT:
               case MESA_SHADER_COMPUTE:
               default:
         }
      static uint64_t *
   v3d_return_qpu_insts(struct v3d_compile *c, uint32_t *final_assembly_size)
   {
                  uint64_t *qpu_insts = malloc(*final_assembly_size);
   if (!qpu_insts)
                              }
      static void
   v3d_nir_lower_vs_early(struct v3d_compile *c)
   {
         /* Split our I/O vars and dead code eliminate the unused
      * components.
      NIR_PASS(_, c->s, nir_lower_io_to_scalar_early,
         uint64_t used_outputs[4] = {0};
   for (int i = 0; i < c->vs_key->num_used_outputs; i++) {
            int slot = v3d_slot_get_slot(c->vs_key->used_outputs[i]);
      }
   NIR_PASS(_, c->s, nir_remove_unused_io_vars,
         NIR_PASS(_, c->s, nir_lower_global_vars_to_local);
   v3d_optimize_nir(c, c->s);
            /* This must go before nir_lower_io */
   if (c->vs_key->per_vertex_point_size)
            NIR_PASS(_, c->s, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
               /* clean up nir_lower_io's deref_var remains and do a constant folding pass
      * on the code it generated.
      NIR_PASS(_, c->s, nir_opt_dce);
   }
      static void
   v3d_nir_lower_gs_early(struct v3d_compile *c)
   {
         /* Split our I/O vars and dead code eliminate the unused
      * components.
      NIR_PASS(_, c->s, nir_lower_io_to_scalar_early,
         uint64_t used_outputs[4] = {0};
   for (int i = 0; i < c->gs_key->num_used_outputs; i++) {
            int slot = v3d_slot_get_slot(c->gs_key->used_outputs[i]);
      }
   NIR_PASS(_, c->s, nir_remove_unused_io_vars,
         NIR_PASS(_, c->s, nir_lower_global_vars_to_local);
   v3d_optimize_nir(c, c->s);
            /* This must go before nir_lower_io */
   if (c->gs_key->per_vertex_point_size)
            NIR_PASS(_, c->s, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
               /* clean up nir_lower_io's deref_var remains and do a constant folding pass
      * on the code it generated.
      NIR_PASS(_, c->s, nir_opt_dce);
   }
      static void
   v3d_fixup_fs_output_types(struct v3d_compile *c)
   {
         nir_foreach_shader_out_variable(var, c->s) {
                     switch (var->data.location) {
   case FRAG_RESULT_COLOR:
               case FRAG_RESULT_DATA0:
   case FRAG_RESULT_DATA1:
   case FRAG_RESULT_DATA2:
   case FRAG_RESULT_DATA3:
                        if (c->fs_key->int_color_rb & mask) {
            var->type =
      } else if (c->fs_key->uint_color_rb & mask) {
            var->type =
   }
      static void
   v3d_nir_lower_fs_early(struct v3d_compile *c)
   {
         if (c->fs_key->int_color_rb || c->fs_key->uint_color_rb)
                     if (c->fs_key->line_smoothing) {
            NIR_PASS(_, c->s, v3d_nir_lower_line_smooth);
   NIR_PASS(_, c->s, nir_lower_global_vars_to_local);
      }
      static void
   v3d_nir_lower_gs_late(struct v3d_compile *c)
   {
         if (c->key->ucp_enables) {
                        /* Note: GS output scalarizing must happen after nir_lower_clip_gs. */
   }
      static void
   v3d_nir_lower_vs_late(struct v3d_compile *c)
   {
         if (c->key->ucp_enables) {
            NIR_PASS(_, c->s, nir_lower_clip_vs, c->key->ucp_enables,
                     /* Note: VS output scalarizing must happen after nir_lower_clip_vs. */
   }
      static void
   v3d_nir_lower_fs_late(struct v3d_compile *c)
   {
         /* In OpenGL the fragment shader can't read gl_ClipDistance[], but
      * Vulkan allows it, in which case the SPIR-V compiler will declare
   * VARING_SLOT_CLIP_DIST0 as compact array variable. Pass true as
   * the last parameter to always operate with a compact array in both
   * OpenGL and Vulkan so we do't have to care about the API we
   * are using.
      if (c->key->ucp_enables)
            }
      static uint32_t
   vir_get_max_temps(struct v3d_compile *c)
   {
         int max_ip = 0;
   vir_for_each_inst_inorder(inst, c)
                     for (int t = 0; t < c->num_temps; t++) {
            for (int i = c->temp_start[t]; (i < c->temp_end[t] &&
                  if (i > max_ip)
            uint32_t max_temps = 0;
   for (int i = 0; i < max_ip; i++)
                     }
      enum v3d_dependency_class {
         };
      static bool
   v3d_intrinsic_dependency_cb(nir_intrinsic_instr *intr,
               {
                  switch (intr->intrinsic) {
   case nir_intrinsic_store_output:
            /* Writing to location 0 overwrites the value passed in for
   * gl_PrimitiveID on geometry shaders
   */
                                                      uint64_t offset =
                                          case nir_intrinsic_load_primitive_id:
                                       default:
                  }
      static unsigned
   v3d_instr_delay_cb(nir_instr *instr, void *data)
   {
               switch (instr->type) {
   case nir_instr_type_undef:
   case nir_instr_type_load_const:
   case nir_instr_type_alu:
   case nir_instr_type_deref:
   case nir_instr_type_jump:
   case nir_instr_type_parallel_copy:
   case nir_instr_type_call:
   case nir_instr_type_phi:
            /* We should not use very large delays for TMU instructions. Typically,
   * thread switches will be sufficient to hide all or most of the latency,
   * so we typically only need a little bit of extra room. If we over-estimate
   * the latency here we may end up unnecessarily delaying the critical path in
   * the shader, which would have a negative effect in performance, so here
   * we are trying to strike a balance based on empirical testing.
   */
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (!c->disable_general_tmu_sched) {
      switch (intr->intrinsic) {
   case nir_intrinsic_decl_reg:
   case nir_intrinsic_load_reg:
   case nir_intrinsic_store_reg:
         case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_image_load:
         case nir_intrinsic_load_ubo:
      if (nir_src_is_divergent(intr->src[1]))
            default:
            } else {
      switch (intr->intrinsic) {
   case nir_intrinsic_decl_reg:
   case nir_intrinsic_load_reg:
   case nir_intrinsic_store_reg:
         default:
            }
               case nir_instr_type_tex:
                     }
      static bool
   should_split_wrmask(const nir_instr *instr, const void *data)
   {
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
         default:
         }
      static nir_intrinsic_instr *
   nir_instr_as_constant_ubo_load(nir_instr *inst)
   {
         if (inst->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(inst);
   if (intr->intrinsic != nir_intrinsic_load_ubo)
            assert(nir_src_is_const(intr->src[0]));
   if (!nir_src_is_const(intr->src[1]))
            }
      static bool
   v3d_nir_sort_constant_ubo_load(nir_block *block, nir_intrinsic_instr *ref)
   {
                  nir_instr *ref_inst = &ref->instr;
   uint32_t ref_offset = nir_src_as_uint(ref->src[1]);
            /* Go through all instructions after ref searching for constant UBO
      * loads for the same UBO index.
      bool seq_break = false;
   nir_instr *inst = &ref->instr;
   nir_instr *next_inst = NULL;
   while (true) {
                                                                                                   /* If there are any UBO loads that are not constant or that
   * use a different UBO index in between the reference load and
   * any other constant load for the same index, they would break
   * the unifa sequence. We will flag that so we can then move
   * all constant UBO loads for the reference index before these
   * and not just the ones that are not ordered to avoid breaking
   * the sequence and reduce unifa writes.
   */
   if (!nir_src_is_const(intr->src[1])) {
                              assert(nir_src_is_const(intr->src[0]));
   uint32_t index = nir_src_as_uint(intr->src[0]);
   if (index != ref_index) {
                        /* Only move loads with an offset that is close enough to the
   * reference offset, since otherwise we would not be able to
   * skip the unifa write for them. See ntq_emit_load_ubo_unifa.
                        /* We will move this load if its offset is smaller than ref's
   * (in which case we will move it before ref) or if the offset
   * is larger than ref's but there are sequence breakers in
   * in between (in which case we will move it after ref and
   * before the sequence breakers).
                        /* Find where exactly we want to move this load:
   *
   * If we are moving it before ref, we want to check any other
   * UBO loads we placed before ref and make sure we insert this
   * one properly ordered with them. Likewise, if we are moving
   * it after ref.
   */
   nir_instr *pos = ref_inst;
   nir_instr *tmp = pos;
   do {
                                                                                                                     /* Stop if we find a unifa UBO load that breaks the
                                             uint32_t tmp_offset = nir_src_as_uint(tmp_intr->src[1]);
   if (offset < ref_offset) {
         if (tmp_offset < offset ||
         tmp_offset >= ref_offset) {
   } else {
         } else {
         if (tmp_offset > offset ||
         tmp_offset <= ref_offset) {
                     /* We can't move the UBO load before the instruction that
   * defines its constant offset. If that instruction is placed
   * in between the new location (pos) and the current location
   * of this load, we will have to move that instruction too.
   *
   * We don't care about the UBO index definition because that
   * is optimized to be reused by all UBO loads for the same
   * index and therefore is certain to be defined before the
   * first UBO load that uses it.
   */
   nir_instr *offset_inst = NULL;
   tmp = inst;
   while ((tmp = nir_instr_prev(tmp)) != NULL) {
            if (pos == tmp) {
         /* We reached the target location without
      * finding the instruction that defines the
   * offset, so that instruction must be before
   * the new position and we don't have to fix it.
      }
   if (intr->src[1].ssa->parent_instr == tmp) {
                     if (offset_inst) {
                              /* Since we are moving the instruction before its current
   * location, grab its successor before the move so that
   * we can continue the next iteration of the main loop from
                        /* Move this load to the selected location */
   exec_node_remove(&inst->node);
   if (offset < ref_offset)
                              }
      static bool
   v3d_nir_sort_constant_ubo_loads_block(struct v3d_compile *c,
         {
         bool progress = false;
   bool local_progress;
   do {
            local_progress = false;
   nir_foreach_instr_safe(inst, block) {
            nir_intrinsic_instr *intr =
         if (intr) {
                        }
      /**
   * Sorts constant UBO loads in each block by offset to maximize chances of
   * skipping unifa writes when converting to VIR. This can increase register
   * pressure.
   */
   static bool
   v3d_nir_sort_constant_ubo_loads(nir_shader *s, struct v3d_compile *c)
   {
         nir_foreach_function_impl(impl, s) {
            nir_foreach_block(block, impl) {
               }
   nir_metadata_preserve(impl,
      }
   }
      static void
   lower_load_num_subgroups(struct v3d_compile *c,
               {
         assert(c->s->info.stage == MESA_SHADER_COMPUTE);
            b->cursor = nir_after_instr(&intr->instr);
   uint32_t num_subgroups =
            DIV_ROUND_UP(c->s->info.workgroup_size[0] *
      nir_def *result = nir_imm_int(b, num_subgroups);
   nir_def_rewrite_uses(&intr->def, result);
   }
      static bool
   lower_subgroup_intrinsics(struct v3d_compile *c,
         {
         bool progress = false;
   nir_foreach_instr_safe(inst, block) {
                           nir_intrinsic_instr *intr =
                        switch (intr->intrinsic) {
   case nir_intrinsic_load_num_subgroups:
            lower_load_num_subgroups(c, b, intr);
      case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_subgroup_size:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_elect:
               default:
               }
      static bool
   v3d_nir_lower_subgroup_intrinsics(nir_shader *s, struct v3d_compile *c)
   {
         bool progress = false;
   nir_foreach_function_impl(impl, s) {
                                    nir_metadata_preserve(impl,
      }
   }
      static void
   v3d_attempt_compile(struct v3d_compile *c)
   {
         switch (c->s->info.stage) {
   case MESA_SHADER_VERTEX:
               case MESA_SHADER_GEOMETRY:
               case MESA_SHADER_FRAGMENT:
               case MESA_SHADER_COMPUTE:
         default:
                  switch (c->s->info.stage) {
   case MESA_SHADER_VERTEX:
               case MESA_SHADER_GEOMETRY:
               case MESA_SHADER_FRAGMENT:
               default:
                           switch (c->s->info.stage) {
   case MESA_SHADER_VERTEX:
               case MESA_SHADER_GEOMETRY:
               case MESA_SHADER_FRAGMENT:
               default:
                  NIR_PASS(_, c->s, v3d_nir_lower_io, c);
   NIR_PASS(_, c->s, v3d_nir_lower_txf_ms);
            NIR_PASS(_, c->s, nir_opt_idiv_const, 8);
   nir_lower_idiv_options idiv_options = {
         };
   NIR_PASS(_, c->s, nir_lower_idiv, &idiv_options);
            if (c->key->robust_uniform_access || c->key->robust_storage_access ||
         c->key->robust_image_access) {
      /* nir_lower_robust_access assumes constant buffer
   * indices on ubo/ssbo intrinsics so run copy propagation and
   * constant folding passes before we run the lowering to warrant
   * this. We also want to run the lowering before v3d_optimize to
   * clean-up redundant get_buffer_size calls produced in the pass.
                        nir_lower_robust_access_options opts = {
      .lower_image = c->key->robust_image_access,
                                                               /* Do late algebraic optimization to turn add(a, neg(b)) back into
      * subs, then the mandatory cleanup after algebraic.  Note that it may
   * produce fnegs, and if so then we need to keep running to squash
   * fneg(fneg(a)).
      bool more_late_algebraic = true;
   while (more_late_algebraic) {
            more_late_algebraic = false;
   NIR_PASS(more_late_algebraic, c->s, nir_opt_algebraic_late);
   NIR_PASS(_, c->s, nir_opt_constant_folding);
   NIR_PASS(_, c->s, nir_copy_prop);
               NIR_PASS(_, c->s, nir_lower_bool_to_int32);
   NIR_PASS(_, c->s, nir_convert_to_lcssa, true, true);
   NIR_PASS_V(c->s, nir_divergence_analysis);
            struct nir_schedule_options schedule_options = {
            /* Schedule for about half our register space, to enable more
                        /* Vertex shaders share the same memory for inputs and outputs,
   * fragment and geometry shaders do not.
   */
   .stages_with_shared_io_memory =
                                                   };
            if (!c->disable_constant_ubo_load_sorting)
            const nir_move_options buffer_opts = c->move_buffer_loads ?
         NIR_PASS(_, c->s, nir_opt_move, nir_move_load_uniform |
                           }
      uint32_t
   v3d_prog_data_size(gl_shader_stage stage)
   {
         static const int prog_data_size[] = {
            [MESA_SHADER_VERTEX] = sizeof(struct v3d_vs_prog_data),
   [MESA_SHADER_GEOMETRY] = sizeof(struct v3d_gs_prog_data),
               assert(stage >= 0 &&
                  }
      int v3d_shaderdb_dump(struct v3d_compile *c,
         {
         if (c == NULL || c->compilation_result != V3D_COMPILATION_SUCCEEDED)
            return asprintf(shaderdb_str,
                     "%s shader: %d inst, %d threads, %d loops, "
   "%d uniforms, %d max-temps, %d:%d spills:fills, "
   "%d sfu-stalls, %d inst-and-stalls, %d nops",
   vir_get_stage_name(c),
   c->qpu_inst_count,
   c->threads,
   c->loops,
   c->num_uniforms,
   vir_get_max_temps(c),
   c->spills,
   }
      /* This is a list of incremental changes to the compilation strategy
   * that will be used to try to compile the shader successfully. The
   * default strategy is to enable all optimizations which will have
   * the highest register pressure but is expected to produce most
   * optimal code. Following strategies incrementally disable specific
   * optimizations that are known to contribute to register pressure
   * in order to be able to compile the shader successfully while meeting
   * thread count requirements.
   *
   * V3D 4.1+ has a min thread count of 2, but we can use 1 here to also
   * cover previous hardware as well (meaning that we are not limiting
   * register allocation to any particular thread count). This is fine
   * because v3d_nir_to_vir will cap this to the actual minimum.
   */
   static const struct v3d_compiler_strategy strategies[] = {
         /*0*/  { "default",                        4, 4, false, false, false, false, false, false,  0 },
   /*1*/  { "disable general TMU sched",      4, 4, true,  false, false, false, false, false,  0 },
   /*2*/  { "disable gcm",                    4, 4, true,  true,  false, false, false, false,  0 },
   /*3*/  { "disable loop unrolling",         4, 4, true,  true,  true,  false, false, false,  0 },
   /*4*/  { "disable UBO load sorting",       4, 4, true,  true,  true,  true,  false, false,  0 },
   /*5*/  { "disable TMU pipelining",         4, 4, true,  true,  true,  true,  false, true,   0 },
   /*6*/  { "lower thread count",             2, 1, false, false, false, false, false, false, -1 },
   /*7*/  { "disable general TMU sched (2t)", 2, 1, true,  false, false, false, false, false, -1 },
   /*8*/  { "disable gcm (2t)",               2, 1, true,  true,  false, false, false, false, -1 },
   /*9*/  { "disable loop unrolling (2t)",    2, 1, true,  true,  true,  false, false, false, -1 },
   /*10*/ { "Move buffer loads (2t)",         2, 1, true,  true,  true,  true,  true,  false, -1 },
   /*11*/ { "disable TMU pipelining (2t)",    2, 1, true,  true,  true,  true,  true,  true,  -1 },
   };
      /**
   * If a particular optimization didn't make any progress during a compile
   * attempt disabling it alone won't allow us to compile the shader successfully,
   * since we'll end up with the same code. Detect these scenarios so we can
   * avoid wasting time with useless compiles. We should also consider if the
   * gy changes other aspects of the compilation process though, like
   * spilling, and not skip it in that case.
   */
   static bool
   skip_compile_strategy(struct v3d_compile *c, uint32_t idx)
   {
      /* We decide if we can skip a strategy based on the optimizations that
   * were active in the previous strategy, so we should only be calling this
   * for strategies after the first.
   */
            /* Don't skip a strategy that changes spilling behavior */
   if (strategies[idx].max_tmu_spills !=
      strategies[idx - 1].max_tmu_spills) {
               switch (idx) {
   /* General TMU sched.: skip if we didn't emit any TMU loads */
   case 1:
   case 7:
         /* Global code motion: skip if nir_opt_gcm didn't make any progress */
   case 2:
   case 8:
         /* Loop unrolling: skip if we didn't unroll any loops */
   case 3:
   case 9:
         /* UBO load sorting: skip if we didn't sort any loads */
   case 4:
         /* Move buffer loads: we assume any shader with difficult RA
   * most likely has UBO / SSBO loads so we never try to skip.
   * For now, we only try this for 2-thread compiles since it
   * is expected to impact instruction counts and latency.
   */
   case 10:
         assert(c->threads < 4);
   /* TMU pipelining: skip if we didn't pipeline any TMU ops */
   case 5:
   case 11:
         /* Lower thread count: skip if we already tried less that 4 threads */
   case 6:
         default:
            }
      static inline void
   set_best_compile(struct v3d_compile **best, struct v3d_compile *c)
   {
      if (*best)
            }
      uint64_t *v3d_compile(const struct v3d_compiler *compiler,
                        struct v3d_key *key,
   struct v3d_prog_data **out_prog_data,
   nir_shader *s,
   void (*debug_output)(const char *msg,
      {
                  uint32_t best_spill_fill_count = UINT32_MAX;
   struct v3d_compile *best_c = NULL;
   for (int32_t strat = 0; strat < ARRAY_SIZE(strategies); strat++) {
            /* Fallback strategy */
   if (strat > 0) {
                                 char *debug_msg;
   int ret = asprintf(&debug_msg,
                                                                                          c = vir_compile_init(compiler, key, s,
                                                            /* If we compiled without spills, choose this.
   * Otherwise if this is a 4-thread compile, choose this (these
   * have a very low cap on the allowed TMU spills so we assume
   * it will be better than a 2-thread compile without spills).
   * Otherwise, keep going while tracking the strategy with the
   * lowest spill count.
   */
   if (c->compilation_result == V3D_COMPILATION_SUCCEEDED) {
            if (c->spills == 0 ||
      strategies[strat].min_threads == 4 ||
   V3D_DBG(OPT_COMPILE_TIME)) {
      set_best_compile(&best_c, c);
   } else if (c->spills + c->fills <
                              if (V3D_DBG(PERF)) {
         char *debug_msg;
   int ret = asprintf(&debug_msg,
                        "Compiled %s prog %d/%d with %d "
   "spills and %d fills. Will try "
      if (ret >= 0) {
                           /* Only try next streategy if we failed to register allocate
   * or we had to spill.
   */
   assert(c->compilation_result ==
               /* If the best strategy was not the last, choose that */
   if (best_c && c != best_c)
            if (V3D_DBG(PERF) &&
         c->compilation_result !=
   V3D_COMPILATION_FAILED_REGISTER_ALLOCATION &&
   c->spills > 0) {
      char *debug_msg;
   int ret = asprintf(&debug_msg,
                                          if (ret >= 0) {
                     if (c->compilation_result != V3D_COMPILATION_SUCCEEDED) {
                                                                                 char *shaderdb;
   int ret = v3d_shaderdb_dump(c, &shaderdb);
   if (ret >= 0) {
                                       }
      void
   vir_remove_instruction(struct v3d_compile *c, struct qinst *qinst)
   {
         if (qinst->dst.file == QFILE_TEMP)
                     list_del(&qinst->link);
            }
      struct qreg
   vir_follow_movs(struct v3d_compile *c, struct qreg reg)
   {
         /* XXX
            while (reg.file == QFILE_TEMP &&
            c->defs[reg.index] &&
   (c->defs[reg.index]->op == QOP_MOV ||
   c->defs[reg.index]->op == QOP_FMOV) &&
   !c->defs[reg.index]->dst.pack &&
               reg.pack = pack;
   */
   }
      void
   vir_compile_destroy(struct v3d_compile *c)
   {
         /* Defuse the assert that we aren't removing the cursor's instruction.
                  vir_for_each_block(block, c) {
            while (!list_is_empty(&block->instructions)) {
            struct qinst *qinst =
                  }
      uint32_t
   vir_get_uniform_index(struct v3d_compile *c,
               {
         for (int i = 0; i < c->num_uniforms; i++) {
            if (c->uniform_contents[i] == contents &&
      c->uniform_data[i] == data) {
                     if (uniform >= c->uniform_array_size) {
                           c->uniform_data = reralloc(c, c->uniform_data,
               c->uniform_contents = reralloc(c, c->uniform_contents,
               c->uniform_contents[uniform] = contents;
            }
      /* Looks back into the current block to find the ldunif that wrote the uniform
   * at the requested index. If it finds it, it returns true and writes the
   * destination register of the ldunif instruction to 'unif'.
   *
   * This can impact register pressure and end up leading to worse code, so we
   * limit the number of instructions we are willing to look back through to
   * strike a good balance.
   */
   static bool
   try_opt_ldunif(struct v3d_compile *c, uint32_t index, struct qreg *unif)
   {
         uint32_t count = 20;
   struct qinst *prev_inst = NULL;
      #ifdef DEBUG
         /* We can only reuse a uniform if it was emitted in the same block,
      * so callers must make sure the current instruction is being emitted
   * in the current block.
      bool found = false;
   vir_for_each_inst(inst, c->cur_block) {
            if (&inst->link == c->cursor.link) {
                     #endif
            list_for_each_entry_from_rev(struct qinst, inst, c->cursor.link->prev,
                  if ((inst->qpu.sig.ldunif || inst->qpu.sig.ldunifrf) &&
      inst->uniform == index) {
                                 if (!prev_inst)
            /* Only reuse the ldunif result if it was written to a temp register,
      * otherwise there may be special restrictions (for example, ldunif
   * may write directly to unifa, which is a write-only register).
      if (prev_inst->dst.file != QFILE_TEMP)
            list_for_each_entry_from(struct qinst, inst, prev_inst->link.next,
                  if (inst->dst.file == prev_inst->dst.file &&
      inst->dst.index == prev_inst->dst.index) {
            *unif = prev_inst->dst;
   }
      struct qreg
   vir_uniform(struct v3d_compile *c,
               {
         const int num_uniforms = c->num_uniforms;
            /* If this is not the first time we see this uniform try to reuse the
      * result of the last ldunif that loaded it.
      const bool is_new_uniform = num_uniforms != c->num_uniforms;
   if (!is_new_uniform && !c->disable_ldunif_opt) {
            struct qreg ldunif_dst;
               struct qinst *inst = vir_NOP(c);
   inst->qpu.sig.ldunif = true;
   inst->uniform = index;
   inst->dst = vir_get_temp(c);
   c->defs[inst->dst.index] = inst;
   }
      #define OPTPASS(func)                                                   \
         do {                                                            \
            bool stage_progress = func(c);                          \
   if (stage_progress) {                                   \
            progress = true;                                \
   if (print_opt_debug) {                          \
         fprintf(stderr,                         \
            void
   vir_optimize(struct v3d_compile *c)
   {
         bool print_opt_debug = false;
            while (true) {
                     OPTPASS(vir_opt_copy_propagate);
   OPTPASS(vir_opt_redundant_flags);
                                    }
      const char *
   vir_get_stage_name(struct v3d_compile *c)
   {
         if (c->vs_key && c->vs_key->is_coord)
         else if (c->gs_key && c->gs_key->is_coord)
         else
   }
      static inline uint32_t
   compute_vpm_size_in_sectors(const struct v3d_device_info *devinfo)
   {
      assert(devinfo->vpm_size > 0);
   const uint32_t sector_size = V3D_CHANNELS * sizeof(uint32_t) * 8;
      }
      /* Computes various parameters affecting VPM memory configuration for programs
   * involving geometry shaders to ensure the program fits in memory and honors
   * requirements described in section "VPM usage" of the programming manual.
   */
   static bool
   compute_vpm_config_gs(struct v3d_device_info *devinfo,
                     {
      const uint32_t A = vs->separate_segments ? 1 : 0;
   const uint32_t Ad = vs->vpm_input_size;
                     /* Try to fit program into our VPM memory budget by adjusting
   * configurable parameters iteratively. We do this in two phases:
   * the first phase tries to fit the program into the total available
   * VPM memory. If we succeed at that, then the second phase attempts
   * to fit the program into half of that budget so we can run bin and
   * render programs in parallel.
   */
   struct vpm_config vpm_cfg[2];
   struct vpm_config *final_vpm_cfg = NULL;
            vpm_cfg[phase].As = 1;
   vpm_cfg[phase].Gs = 1;
   vpm_cfg[phase].Gd = gs->vpm_output_size;
            /* While there is a requirement that Vc >= [Vn / 16], this is
   * always the case when tessellation is not present because in that
   * case Vn can only be 6 at most (when input primitive is triangles
   * with adjacency).
   *
   * We always choose Vc=2. We can't go lower than this due to GFXH-1744,
   * and Broadcom has not found it worth it to increase it beyond this
   * in general. Increasing Vc also increases VPM memory pressure which
   * can turn up being detrimental for performance in some scenarios.
   */
            /* Gv is a constraint on the hardware to not exceed the
   * specified number of vertex segments per GS batch. If adding a
   * new primitive to a GS batch would result in a range of more
   * than Gv vertex segments being referenced by the batch, then
   * the hardware will flush the batch and start a new one. This
   * means that we can choose any value we want, we just need to
   * be aware that larger values improve GS batch utilization
   * at the expense of more VPM memory pressure (which can affect
   * other performance aspects, such as GS dispatch width).
   * We start with the largest value, and will reduce it if we
   * find that total memory pressure is too high.
   */
   vpm_cfg[phase].Gv = 3;
   do {
      /* When GS is present in absence of TES, then we need to satisfy
   * that Ve >= Gv. We go with the smallest value of Ve to avoid
   * increasing memory pressure.
   */
            uint32_t vpm_sectors =
      A * vpm_cfg[phase].As * Ad +
               /* Ideally we want to use no more than half of the available
   * memory so we can execute a bin and render program in parallel
   * without stalls. If we achieved that then we are done.
   */
   if (vpm_sectors <= vpm_size / 2) {
      final_vpm_cfg = &vpm_cfg[phase];
               /* At the very least, we should not allocate more than the
   * total available VPM memory. If we have a configuration that
   * succeeds at this we save it and continue to see if we can
   * meet the half-memory-use criteria too.
   */
   if (phase == 0 && vpm_sectors <= vpm_size) {
      vpm_cfg[1] = vpm_cfg[0];
               /* Try lowering Gv */
   if (vpm_cfg[phase].Gv > 0) {
      vpm_cfg[phase].Gv--;
               /* Try lowering GS dispatch width */
   if (vpm_cfg[phase].gs_width > 1) {
      do {
      vpm_cfg[phase].gs_width >>= 1;
               /* Reset Gv to max after dropping dispatch width */
   vpm_cfg[phase].Gv = 3;
               /* We ran out of options to reduce memory pressure. If we
   * are at phase 1 we have at least a valid configuration, so we
   * we use that.
   */
   if (phase == 1)
                     if (!final_vpm_cfg)
            assert(final_vpm_cfg);
   assert(final_vpm_cfg->Gd <= 16);
   assert(final_vpm_cfg->Gv < 4);
   assert(final_vpm_cfg->Ve < 4);
   assert(final_vpm_cfg->Vc >= 2 && final_vpm_cfg->Vc <= 4);
   assert(final_vpm_cfg->gs_width == 1 ||
         final_vpm_cfg->gs_width == 4 ||
            *vpm_cfg_out = *final_vpm_cfg;
      }
      bool
   v3d_compute_vpm_config(struct v3d_device_info *devinfo,
                        struct v3d_vs_prog_data *vs_bin,
   struct v3d_vs_prog_data *vs,
      {
      assert(vs && vs_bin);
            if (!gs) {
      vpm_cfg_bin->As = 1;
   vpm_cfg_bin->Ve = 0;
            vpm_cfg->As = 1;
   vpm_cfg->Ve = 0;
      } else {
      if (!compute_vpm_config_gs(devinfo, vs_bin, gs_bin, vpm_cfg_bin))
            if (!compute_vpm_config_gs(devinfo, vs, gs, vpm_cfg))
                  }
