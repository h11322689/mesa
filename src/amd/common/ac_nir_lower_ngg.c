   /*
   * Copyright © 2021 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_nir.h"
   #include "amdgfxregs.h"
   #include "nir_builder.h"
   #include "nir_xfb_info.h"
   #include "util/u_math.h"
   #include "util/u_vector.h"
      #define SPECIAL_MS_OUT_MASK \
      (BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_COUNT) | \
   BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_INDICES) | \
         #define MS_PRIM_ARG_EXP_MASK \
      (VARYING_BIT_LAYER | \
   VARYING_BIT_VIEWPORT | \
         #define MS_VERT_ARG_EXP_MASK \
      (VARYING_BIT_CULL_DIST0 | \
   VARYING_BIT_CULL_DIST1 | \
   VARYING_BIT_CLIP_DIST0 | \
   VARYING_BIT_CLIP_DIST1 | \
         enum {
      nggc_passflag_used_by_pos = 1,
   nggc_passflag_used_by_other = 2,
      };
      typedef struct
   {
      nir_def *ssa;
      } reusable_nondeferred_variable;
      typedef struct
   {
      gl_varying_slot slot;
      } vs_output;
      typedef struct
   {
      nir_alu_type types[VARYING_SLOT_MAX][4];
   nir_alu_type types_16bit_lo[16][4];
      } shader_output_types;
      typedef struct
   {
               nir_variable *position_value_var;
   nir_variable *prim_exp_arg_var;
   nir_variable *es_accepted_var;
   nir_variable *gs_accepted_var;
   nir_variable *gs_exported_var;
                              bool early_prim_export;
   bool streamout_enabled;
   bool has_user_edgeflags;
            /* LDS params */
            uint64_t inputs_needed_by_pos;
            nir_instr *compact_arg_stores[4];
   nir_intrinsic_instr *overwrite_args;
            /* clip distance */
   nir_variable *clip_vertex_var;
   nir_variable *clipdist_neg_mask_var;
            /* outputs */
   nir_def *outputs[VARYING_SLOT_MAX][4];
   nir_def *outputs_16bit_lo[16][4];
   nir_def *outputs_16bit_hi[16][4];
      } lower_ngg_nogs_state;
      typedef struct
   {
      /* output stream index, 2 bit per component */
   uint8_t stream;
   /* Bitmask of components used: 4 bits per slot, 1 bit per component. */
      } gs_output_info;
      typedef struct
   {
               nir_function_impl *impl;
   int const_out_vtxcnt[4];
   int const_out_prmcnt[4];
   unsigned max_num_waves;
   unsigned num_vertices_per_primitive;
   nir_def *lds_addr_gs_out_vtx;
   nir_def *lds_addr_gs_scratch;
   unsigned lds_bytes_per_gs_out_vertex;
   unsigned lds_offs_primflags;
   bool output_compile_time_known;
   bool streamout_enabled;
   /* 32 bit outputs */
   nir_def *outputs[VARYING_SLOT_MAX][4];
   gs_output_info output_info[VARYING_SLOT_MAX];
   /* 16 bit outputs */
   nir_def *outputs_16bit_hi[16][4];
   nir_def *outputs_16bit_lo[16][4];
   gs_output_info output_info_16bit_hi[16];
   gs_output_info output_info_16bit_lo[16];
   /* output types for both 32bit and 16bit */
   shader_output_types output_types;
   /* Count per stream. */
   nir_def *vertex_count[4];
      } lower_ngg_gs_state;
      /* LDS layout of Mesh Shader workgroup info. */
   enum {
      /* DW0: number of primitives */
   lds_ms_num_prims = 0,
   /* DW1: number of vertices */
   lds_ms_num_vtx = 4,
   /* DW2: workgroup index within the current dispatch */
   lds_ms_wg_index = 8,
   /* DW3: number of API workgroups in flight */
      };
      /* Potential location for Mesh Shader outputs. */
   typedef enum {
      ms_out_mode_lds,
   ms_out_mode_scratch_ring,
   ms_out_mode_attr_ring,
      } ms_out_mode;
      typedef struct
   {
      uint64_t mask; /* Mask of output locations */
      } ms_out_part;
      typedef struct
   {
      /* Mesh shader LDS layout. For details, see ms_calculate_output_layout. */
   struct {
      uint32_t workgroup_info_addr;
   ms_out_part vtx_attr;
   ms_out_part prm_attr;
   uint32_t indices_addr;
   uint32_t cull_flags_addr;
               /* VRAM "mesh shader scratch ring" layout for outputs that don't fit into the LDS.
   * Not to be confused with scratch memory.
   */
   struct {
      ms_out_part vtx_attr;
               /* VRAM attributes ring (GFX11 only) for all non-position outputs.
   * GFX11 doesn't have to reload attributes from this ring at the end of the shader.
   */
   struct {
      ms_out_part vtx_attr;
               /* Outputs without cross-invocation access can be stored in variables. */
   struct {
      ms_out_part vtx_attr;
         } ms_out_mem_layout;
      typedef struct
   {
      enum amd_gfx_level gfx_level;
   bool fast_launch_2;
   bool vert_multirow_export;
            ms_out_mem_layout layout;
   uint64_t per_vertex_outputs;
   uint64_t per_primitive_outputs;
            unsigned wave_size;
   unsigned api_workgroup_size;
            nir_def *workgroup_index;
   nir_variable *out_variables[VARYING_SLOT_MAX * 4];
   nir_variable *primitive_count_var;
            /* True if the lowering needs to insert the layer output. */
   bool insert_layer_output;
   /* True if cull flags are used */
            struct {
      /* Bitmask of components used: 4 bits per slot, 1 bit per component. */
               /* Used by outputs export. */
   nir_def *outputs[VARYING_SLOT_MAX][4];
   uint32_t clipdist_enable_mask;
   const uint8_t *vs_output_param_offset;
            /* True if the lowering needs to insert shader query. */
      } lower_ngg_ms_state;
      /* Per-vertex LDS layout of culling shaders */
   enum {
      /* Position of the ES vertex (at the beginning for alignment reasons) */
   lds_es_pos_x = 0,
   lds_es_pos_y = 4,
   lds_es_pos_z = 8,
            /* 1 when the vertex is accepted, 0 if it should be culled */
   lds_es_vertex_accepted = 16,
   /* ID of the thread which will export the current thread's vertex */
   lds_es_exporter_tid = 17,
   /* bit i is set when the i'th clip distance of a vertex is negative */
   lds_es_clipdist_neg_mask = 18,
   /* TES only, relative patch ID, less than max workgroup size */
            /* Repacked arguments - also listed separately for VS and TES */
      };
      typedef struct {
      nir_def *num_repacked_invocations;
      } wg_repack_result;
      /**
   * Computes a horizontal sum of 8-bit packed values loaded from LDS.
   *
   * Each lane N will sum packed bytes 0 to N-1.
   * We only care about the results from up to wave_id+1 lanes.
   * (Other lanes are not deactivated but their calculation is not used.)
   */
   static nir_def *
   summarize_repack(nir_builder *b, nir_def *packed_counts, unsigned num_lds_dwords)
   {
      /* We'll use shift to filter out the bytes not needed by the current lane.
   *
   * Need to shift by: num_lds_dwords * 4 - lane_id (in bytes).
   * However, two shifts are needed because one can't go all the way,
   * so the shift amount is half that (and in bits).
   *
   * When v_dot4_u32_u8 is available, we right-shift a series of 0x01 bytes.
   * This will yield 0x01 at wanted byte positions and 0x00 at unwanted positions,
   * therefore v_dot can get rid of the unneeded values.
   * This sequence is preferable because it better hides the latency of the LDS.
   *
   * If the v_dot instruction can't be used, we left-shift the packed bytes.
   * This will shift out the unneeded bytes and shift in zeroes instead,
   * then we sum them using v_sad_u8.
            nir_def *lane_id = nir_load_subgroup_invocation(b);
   nir_def *shift = nir_iadd_imm(b, nir_imul_imm(b, lane_id, -4u), num_lds_dwords * 16);
            if (num_lds_dwords == 1) {
               /* Broadcast the packed data we read from LDS (to the first 16 lanes, but we only care up to num_waves). */
            /* Horizontally add the packed bytes. */
   if (use_dot) {
         } else {
      nir_def *sad_op = nir_ishl(b, nir_ishl(b, packed, shift), shift);
         } else if (num_lds_dwords == 2) {
               /* Broadcast the packed data we read from LDS (to the first 16 lanes, but we only care up to num_waves). */
   nir_def *packed_dw0 = nir_lane_permute_16_amd(b, nir_unpack_64_2x32_split_x(b, packed_counts), nir_imm_int(b, 0), nir_imm_int(b, 0));
            /* Horizontally add the packed bytes. */
   if (use_dot) {
      nir_def *sum = nir_udot_4x8_uadd(b, packed_dw0, nir_unpack_64_2x32_split_x(b, dot_op), nir_imm_int(b, 0));
      } else {
      nir_def *sad_op = nir_ishl(b, nir_ishl(b, nir_pack_64_2x32_split(b, packed_dw0, packed_dw1), shift), shift);
   nir_def *sum = nir_sad_u8x4(b, nir_unpack_64_2x32_split_x(b, sad_op), nir_imm_int(b, 0), nir_imm_int(b, 0));
         } else {
            }
      /**
   * Repacks invocations in the current workgroup to eliminate gaps between them.
   *
   * Uses 1 dword of LDS per 4 waves (1 byte of LDS per wave).
   * Assumes that all invocations in the workgroup are active (exec = -1).
   */
   static wg_repack_result
   repack_invocations_in_workgroup(nir_builder *b, nir_def *input_bool,
               {
      /* Input boolean: 1 if the current invocation should survive the repack. */
            /* STEP 1. Count surviving invocations in the current wave.
   *
   * Implemented by a scalar instruction that simply counts the number of bits set in a 32/64-bit mask.
            nir_def *input_mask = nir_ballot(b, 1, wave_size, input_bool);
            /* If we know at compile time that the workgroup has only 1 wave, no further steps are necessary. */
   if (max_num_waves == 1) {
      wg_repack_result r = {
      .num_repacked_invocations = surviving_invocations_in_current_wave,
      };
               /* STEP 2. Waves tell each other their number of surviving invocations.
   *
   * Each wave activates only its first lane (exec = 1), which stores the number of surviving
   * invocations in that wave into the LDS, then reads the numbers from every wave.
   *
   * The workgroup size of NGG shaders is at most 256, which means
   * the maximum number of waves is 4 in Wave64 mode and 8 in Wave32 mode.
   * Each wave writes 1 byte, so it's up to 8 bytes, so at most 2 dwords are necessary.
            const unsigned num_lds_dwords = DIV_ROUND_UP(max_num_waves, 4);
            nir_def *wave_id = nir_load_subgroup_id(b);
   nir_def *lds_offset = nir_iadd(b, lds_addr_base, wave_id);
   nir_def *dont_care = nir_undef(b, 1, num_lds_dwords * 32);
                     nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            nir_def *packed_counts =
                              /* STEP 3. Compute the repacked invocation index and the total number of surviving invocations.
   *
   * By now, every wave knows the number of surviving invocations in all waves.
   * Each number is 1 byte, and they are packed into up to 2 dwords.
   *
   * Each lane N will sum the number of surviving invocations from waves 0 to N-1.
   * If the workgroup has M waves, then each wave will use only its first M+1 lanes for this.
   * (Other lanes are not deactivated but their calculation is not used.)
   *
   * - We read the sum from the lane whose id is the current wave's id.
   *   Add the masked bitcount to this, and we get the repacked invocation index.
   * - We read the sum from the lane whose id is the number of waves in the workgroup.
   *   This is the total number of surviving invocations in the workgroup.
            nir_def *num_waves = nir_load_num_subgroups(b);
            nir_def *wg_repacked_index_base = nir_read_invocation(b, sum, wave_id);
   nir_def *wg_num_repacked_invocations = nir_read_invocation(b, sum, num_waves);
            wg_repack_result r = {
      .num_repacked_invocations = wg_num_repacked_invocations,
                  }
      static nir_def *
   pervertex_lds_addr(nir_builder *b, nir_def *vertex_idx, unsigned per_vtx_bytes)
   {
         }
      static nir_def *
   emit_pack_ngg_prim_exp_arg(nir_builder *b, unsigned num_vertices_per_primitives,
         {
               for (unsigned i = 0; i < num_vertices_per_primitives; ++i) {
      assert(vertex_indices[i]);
               if (is_null_prim) {
      if (is_null_prim->bit_size == 1)
         assert(is_null_prim->bit_size == 32);
                  }
      static void
   alloc_vertices_and_primitives(nir_builder *b,
               {
      /* The caller should only call this conditionally on wave 0.
   *
   * Send GS Alloc Request message from the first wave of the group to SPI.
   * Message payload (in the m0 register) is:
   * - bits 0..10: number of vertices in group
   * - bits 12..22: number of primitives in group
            nir_def *m0 = nir_ior(b, nir_ishl_imm(b, num_prim, 12), num_vtx);
      }
      static void
   alloc_vertices_and_primitives_gfx10_workaround(nir_builder *b,
               {
      /* HW workaround for a GPU hang with 100% culling on GFX10.
   * We always have to export at least 1 primitive.
   * Export a degenerate triangle using vertex 0 for all 3 vertices.
   *
   * NOTE: We rely on the caller to set the vertex count also to 0 when the primitive count is 0.
   */
   nir_def *is_prim_cnt_0 = nir_ieq_imm(b, num_prim, 0);
   nir_if *if_prim_cnt_0 = nir_push_if(b, is_prim_cnt_0);
   {
      nir_def *one = nir_imm_int(b, 1);
            nir_def *tid = nir_load_subgroup_invocation(b);
   nir_def *is_thread_0 = nir_ieq_imm(b, tid, 0);
   nir_if *if_thread_0 = nir_push_if(b, is_thread_0);
   {
      /* The vertex indices are 0, 0, 0. */
   nir_export_amd(b, nir_imm_zero(b, 4, 32),
                        /* The HW culls primitives with NaN. -1 is also NaN and can save
   * a dword in binary code by inlining constant.
   */
   nir_export_amd(b, nir_imm_ivec4(b, -1, -1, -1, -1),
                  }
      }
   nir_push_else(b, if_prim_cnt_0);
   {
         }
      }
      static void
   ngg_nogs_init_vertex_indices_vars(nir_builder *b, nir_function_impl *impl, lower_ngg_nogs_state *s)
   {
      for (unsigned v = 0; v < s->options->num_vertices_per_primitive; ++v) {
               nir_def *vtx = s->options->passthrough ?
      nir_ubfe_imm(b, nir_load_packed_passthrough_primitive_amd(b),
                           }
      static nir_def *
   emit_ngg_nogs_prim_exp_arg(nir_builder *b, lower_ngg_nogs_state *s)
   {
      if (s->options->passthrough) {
         } else {
               for (unsigned v = 0; v < s->options->num_vertices_per_primitive; ++v)
                  }
      static nir_def *
   has_input_vertex(nir_builder *b)
   {
         }
      static nir_def *
   has_input_primitive(nir_builder *b)
   {
      return nir_is_subgroup_invocation_lt_amd(b,
      }
      static void
   nogs_prim_gen_query(nir_builder *b, lower_ngg_nogs_state *s)
   {
      if (!s->options->has_gen_prim_query)
            nir_if *if_shader_query = nir_push_if(b, nir_load_prim_gen_query_enabled_amd(b));
   {
      /* Activate only 1 lane and add the number of primitives to query result. */
   nir_if *if_elected = nir_push_if(b, nir_elect(b, 1));
   {
      /* Number of input primitives in the current wave. */
                  /* Add to stream 0 primitive generated counter. */
      }
      }
      }
      static void
   emit_ngg_nogs_prim_export(nir_builder *b, lower_ngg_nogs_state *s, nir_def *arg)
   {
      nir_if *if_gs_thread = nir_push_if(b, nir_load_var(b, s->gs_exported_var));
   {
      if (!arg)
            /* pack user edge flag info into arg */
   if (s->has_user_edgeflags) {
      /* Workgroup barrier: wait for ES threads store user edge flags to LDS */
   nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                                       unsigned edge_flag_offset = 0;
   if (s->streamout_enabled) {
      unsigned packed_location =
      util_bitcount64(b->shader->info.outputs_written &
                  for (int i = 0; i < s->options->num_vertices_per_primitive; i++) {
      nir_def *vtx_idx = nir_load_var(b, s->gs_vtx_indices_vars[i]);
   nir_def *addr = pervertex_lds_addr(b, vtx_idx, s->pervertex_lds_bytes);
   nir_def *edge = nir_load_shared(b, 1, 32, addr, .base = edge_flag_offset);
      }
                  }
      }
      static void
   emit_ngg_nogs_prim_id_store_shared(nir_builder *b, lower_ngg_nogs_state *s)
   {
      nir_def *gs_thread =
            nir_if *if_gs_thread = nir_push_if(b, gs_thread);
   {
      /* Copy Primitive IDs from GS threads to the LDS address
   * corresponding to the ES thread of the provoking vertex.
   * It will be exported as a per-vertex attribute.
   */
   nir_def *gs_vtx_indices[3];
   for (unsigned i = 0; i < s->options->num_vertices_per_primitive; i++)
            nir_def *provoking_vertex = nir_load_provoking_vtx_in_prim_amd(b);
   nir_def *provoking_vtx_idx = nir_select_from_ssa_def_array(
            nir_def *prim_id = nir_load_primitive_id(b);
            /* primitive id is always at last of a vertex */
      }
      }
      static void
   emit_store_ngg_nogs_es_primitive_id(nir_builder *b, lower_ngg_nogs_state *s)
   {
               if (b->shader->info.stage == MESA_SHADER_VERTEX) {
      /* LDS address where the primitive ID is stored */
   nir_def *thread_id_in_threadgroup = nir_load_local_invocation_index(b);
   nir_def *addr =
            /* Load primitive ID from LDS */
      } else if (b->shader->info.stage == MESA_SHADER_TESS_EVAL) {
      /* Just use tess eval primitive ID, which is the same as the patch ID. */
                        /* Update outputs_written to reflect that the pass added a new output. */
      }
      static void
   add_clipdist_bit(nir_builder *b, nir_def *dist, unsigned index, nir_variable *mask)
   {
      nir_def *is_neg = nir_flt_imm(b, dist, 0);
   nir_def *neg_mask = nir_ishl_imm(b, nir_b2i32(b, is_neg), index);
   neg_mask = nir_ior(b, neg_mask, nir_load_var(b, mask));
      }
      static bool
   remove_culling_shader_output(nir_builder *b, nir_instr *instr, void *state)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     /* These are not allowed in VS / TES */
   assert(intrin->intrinsic != nir_intrinsic_store_per_vertex_output &&
            /* We are only interested in output stores now */
   if (intrin->intrinsic != nir_intrinsic_store_output)
                     /* no indirect output */
            unsigned writemask = nir_intrinsic_write_mask(intrin);
   unsigned component = nir_intrinsic_component(intrin);
            /* Position output - store the value to a variable, remove output store */
   nir_io_semantics io_sem = nir_intrinsic_io_semantics(intrin);
   switch (io_sem.location) {
   case VARYING_SLOT_POS:
      ac_nir_store_var_components(b, s->position_value_var, store_val, component, writemask);
      case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1: {
      unsigned base = io_sem.location == VARYING_SLOT_CLIP_DIST1 ? 4 : 0;
            /* valid clipdist component mask */
   unsigned mask = (s->options->clipdist_enable_mask >> base) & writemask;
   u_foreach_bit(i, mask) {
      add_clipdist_bit(b, nir_channel(b, store_val, i), base + i,
            }
      }
   case VARYING_SLOT_CLIP_VERTEX:
      ac_nir_store_var_components(b, s->clip_vertex_var, store_val, component, writemask);
      default:
                  /* Remove all output stores */
   nir_instr_remove(instr);
      }
      static void
   remove_culling_shader_outputs(nir_shader *culling_shader, lower_ngg_nogs_state *s)
   {
      nir_shader_instructions_pass(culling_shader, remove_culling_shader_output,
            /* Remove dead code resulting from the deleted outputs. */
   bool progress;
   do {
      progress = false;
   NIR_PASS(progress, culling_shader, nir_opt_dead_write_vars);
   NIR_PASS(progress, culling_shader, nir_opt_dce);
         }
      static void
   rewrite_uses_to_var(nir_builder *b, nir_def *old_def, nir_variable *replacement_var, unsigned replacement_var_channel)
   {
      if (old_def->parent_instr->type == nir_instr_type_load_const)
            b->cursor = nir_after_instr(old_def->parent_instr);
   if (b->cursor.instr->type == nir_instr_type_phi)
            nir_def *pos_val_rep = nir_load_var(b, replacement_var);
            if (old_def->num_components > 1) {
      /* old_def uses a swizzled vector component.
   * There is no way to replace the uses of just a single vector component,
   * so instead create a new vector and replace all uses of the old vector.
   */
   nir_def *old_def_elements[NIR_MAX_VEC_COMPONENTS] = {0};
   for (unsigned j = 0; j < old_def->num_components; ++j)
                        }
      static bool
   remove_extra_pos_output(nir_builder *b, nir_instr *instr, void *state)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     /* These are not allowed in VS / TES */
   assert(intrin->intrinsic != nir_intrinsic_store_per_vertex_output &&
            /* We are only interested in output stores now */
   if (intrin->intrinsic != nir_intrinsic_store_output)
            nir_io_semantics io_sem = nir_intrinsic_io_semantics(intrin);
   if (io_sem.location != VARYING_SLOT_POS)
                     /* In case other outputs use what we calculated for pos,
   * try to avoid calculating it again by rewriting the usages
   * of the store components here.
   */
   nir_def *store_val = intrin->src[0].ssa;
                     if (store_val->parent_instr->type == nir_instr_type_alu) {
      nir_alu_instr *alu = nir_instr_as_alu(store_val->parent_instr);
   if (nir_op_is_vec_or_mov(alu->op)) {
               unsigned num_vec_src = 0;
   if (alu->op == nir_op_mov)
         else if (alu->op == nir_op_vec2)
         else if (alu->op == nir_op_vec3)
         else if (alu->op == nir_op_vec4)
                  /* Remember the current components whose uses we wish to replace.
   * This is needed because rewriting one source can affect the others too.
   */
   nir_def *vec_comps[NIR_MAX_VEC_COMPONENTS] = {0};
                  for (unsigned i = 0; i < num_vec_src; i++)
      } else {
            } else {
                     }
      static void
   remove_extra_pos_outputs(nir_shader *shader, lower_ngg_nogs_state *s)
   {
      nir_shader_instructions_pass(shader, remove_extra_pos_output,
            }
      static bool
   remove_compacted_arg(lower_ngg_nogs_state *s, nir_builder *b, unsigned idx)
   {
      nir_instr *store_instr = s->compact_arg_stores[idx];
   if (!store_instr)
            /* Simply remove the store. */
            /* Find the intrinsic that overwrites the shader arguments,
   * and change its corresponding source.
   * This will cause NIR's DCE to recognize the load and its phis as dead.
   */
   b->cursor = nir_before_instr(&s->overwrite_args->instr);
   nir_def *undef_arg = nir_undef(b, 1, 32);
            s->compact_arg_stores[idx] = NULL;
      }
      static bool
   cleanup_culling_shader_after_dce(nir_shader *shader,
               {
      bool uses_vs_vertex_id = false;
   bool uses_vs_instance_id = false;
   bool uses_tes_u = false;
   bool uses_tes_v = false;
   bool uses_tes_rel_patch_id = false;
            bool progress = false;
            nir_foreach_block_reverse_safe(block, function_impl) {
      nir_foreach_instr_reverse_safe(instr, block) {
                              switch (intrin->intrinsic) {
   case nir_intrinsic_sendmsg_amd:
         case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_vertex_id_zero_base:
      uses_vs_vertex_id = true;
      case nir_intrinsic_load_instance_id:
      uses_vs_instance_id = true;
      case nir_intrinsic_load_input:
      if (s->options->instance_rate_inputs & BITFIELD_BIT(nir_intrinsic_base(intrin)))
         else
            case nir_intrinsic_load_tess_coord:
      uses_tes_u = uses_tes_v = true;
      case nir_intrinsic_load_tess_rel_patch_id_amd:
      uses_tes_rel_patch_id = true;
      case nir_intrinsic_load_primitive_id:
      if (shader->info.stage == MESA_SHADER_TESS_EVAL)
            default:
                                 if (shader->info.stage == MESA_SHADER_VERTEX) {
      if (!uses_vs_vertex_id)
         if (!uses_vs_instance_id)
      } else if (shader->info.stage == MESA_SHADER_TESS_EVAL) {
      if (!uses_tes_u)
         if (!uses_tes_v)
         if (!uses_tes_rel_patch_id)
         if (!uses_tes_patch_id)
                  }
      /**
   * Perform vertex compaction after culling.
   *
   * 1. Repack surviving ES invocations (this determines which lane will export which vertex)
   * 2. Surviving ES vertex invocations store their data to LDS
   * 3. Emit GS_ALLOC_REQ
   * 4. Repacked invocations load the vertex data from LDS
   * 5. GS threads update their vertex indices
   */
   static void
   compact_vertices_after_culling(nir_builder *b,
                                 lower_ngg_nogs_state *s,
   nir_variable **repacked_variables,
   nir_variable **gs_vtxaddr_vars,
   nir_def *invocation_index,
   {
      nir_variable *es_accepted_var = s->es_accepted_var;
   nir_variable *gs_accepted_var = s->gs_accepted_var;
   nir_variable *position_value_var = s->position_value_var;
            nir_if *if_es_accepted = nir_push_if(b, nir_load_var(b, es_accepted_var));
   {
               /* Store the exporter thread's index to the LDS space of the current thread so GS threads can load it */
            /* Store the current thread's position output to the exporter thread's LDS space */
   nir_def *pos = nir_load_var(b, position_value_var);
            /* Store the current thread's repackable arguments to the exporter thread's LDS space */
   for (unsigned i = 0; i < num_repacked_variables; ++i) {
                                 /* TES rel patch id does not cost extra dword */
   if (b->shader->info.stage == MESA_SHADER_TESS_EVAL) {
      nir_def *arg_val = nir_load_var(b, s->repacked_rel_patch_id);
   nir_intrinsic_instr *store =
                        }
            /* TODO: Consider adding a shortcut exit.
   * Waves that have no vertices and primitives left can s_endpgm right here.
            nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            nir_def *es_survived = nir_ilt(b, invocation_index, num_live_vertices_in_workgroup);
   nir_if *if_packed_es_thread = nir_push_if(b, es_survived);
   {
      /* Read position from the current ES thread's LDS space (written by the exported vertex's ES thread) */
   nir_def *exported_pos = nir_load_shared(b, 4, 32, es_vertex_lds_addr, .base = lds_es_pos_x);
            /* Read the repacked arguments */
   for (unsigned i = 0; i < num_repacked_variables; ++i) {
      nir_def *arg_val = nir_load_shared(b, 1, 32, es_vertex_lds_addr, .base = lds_es_arg_0 + 4u * i);
               if (b->shader->info.stage == MESA_SHADER_TESS_EVAL) {
      nir_def *arg_val = nir_load_shared(b, 1, 8, es_vertex_lds_addr,
               }
   nir_push_else(b, if_packed_es_thread);
   {
      nir_store_var(b, position_value_var, nir_undef(b, 4, 32), 0xfu);
   for (unsigned i = 0; i < num_repacked_variables; ++i)
      }
            nir_if *if_gs_accepted = nir_push_if(b, nir_load_var(b, gs_accepted_var));
   {
               /* Load the index of the ES threads that will export the current GS thread's vertices */
   for (unsigned v = 0; v < s->options->num_vertices_per_primitive; ++v) {
      nir_def *vtx_addr = nir_load_var(b, gs_vtxaddr_vars[v]);
   nir_def *exporter_vtx_idx = nir_load_shared(b, 1, 8, vtx_addr, .base = lds_es_exporter_tid);
   exporter_vtx_indices[v] = nir_u2u32(b, exporter_vtx_idx);
               nir_def *prim_exp_arg =
      emit_pack_ngg_prim_exp_arg(b, s->options->num_vertices_per_primitive,
         }
               }
      static void
   analyze_shader_before_culling_walk(nir_def *ssa,
               {
      nir_instr *instr = ssa->parent_instr;
   uint8_t old_pass_flags = instr->pass_flags;
            if (instr->pass_flags == old_pass_flags)
            switch (instr->type) {
   case nir_instr_type_intrinsic: {
               /* VS input loads and SSBO loads are actually VRAM reads on AMD HW. */
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_input: {
      nir_io_semantics in_io_sem = nir_intrinsic_io_semantics(intrin);
   uint64_t in_mask = UINT64_C(1) << (uint64_t) in_io_sem.location;
   if (instr->pass_flags & nggc_passflag_used_by_pos)
         else if (instr->pass_flags & nggc_passflag_used_by_other)
            }
   default:
                     }
   case nir_instr_type_alu: {
      nir_alu_instr *alu = nir_instr_as_alu(instr);
            for (unsigned i = 0; i < num_srcs; ++i) {
                     }
   case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
            for (unsigned i = 0; i < num_srcs; ++i) {
                     }
   case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
   nir_foreach_phi_src_safe(phi_src, phi) {
                     }
   default:
            }
      static void
   analyze_shader_before_culling(nir_shader *shader, lower_ngg_nogs_state *s)
   {
      /* LCSSA is needed to get correct results from divergence analysis. */
   nir_convert_to_lcssa(shader, true, true);
   /* We need divergence info for culling shaders. */
            nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
                                       nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  nir_io_semantics io_sem = nir_intrinsic_io_semantics(intrin);
   nir_def *store_val = intrin->src[0].ssa;
   uint8_t flag = io_sem.location == VARYING_SLOT_POS ? nggc_passflag_used_by_pos : nggc_passflag_used_by_other;
               }
      static nir_def *
   find_reusable_ssa_def(nir_instr *instr)
   {
      /* Find instructions whose SSA definitions are used by both
   * the top and bottom parts of the shader (before and after culling).
   * Only in this case, it makes sense for the bottom part
   * to try to reuse these from the top part.
   */
   if ((instr->pass_flags & nggc_passflag_used_by_both) != nggc_passflag_used_by_both)
            switch (instr->type) {
   case nir_instr_type_alu: {
      nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (alu->def.divergent)
         /* Ignore uniform floats because they regress VGPR usage too much */
   if (nir_op_infos[alu->op].output_type & nir_type_float)
            }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (!nir_intrinsic_can_reorder(intrin) ||
         !nir_intrinsic_infos[intrin->intrinsic].has_dest ||
            }
   case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
   if (phi->def.divergent)
            }
   default:
            }
      static const struct glsl_type *
   glsl_uint_type_for_ssa(nir_def *ssa)
   {
      enum glsl_base_type base_type = GLSL_TYPE_UINT;
   switch (ssa->bit_size) {
   case 8: base_type = GLSL_TYPE_UINT8; break;
   case 16: base_type = GLSL_TYPE_UINT16; break;
   case 32: base_type = GLSL_TYPE_UINT; break;
   case 64: base_type = GLSL_TYPE_UINT64; break;
   default: return NULL;
            return ssa->num_components == 1
            }
      /**
   * Save the reusable SSA definitions to variables so that the
   * bottom shader part can reuse them from the top part.
   *
   * 1. We create a new function temporary variable for reusables,
   *    and insert a store+load.
   * 2. The shader is cloned (the top part is created), then the
   *    control flow is reinserted (for the bottom part.)
   * 3. For reusables, we delete the variable stores from the
   *    bottom part. This will make them use the variables from
   *    the top part and DCE the redundant instructions.
   */
   static void
   save_reusable_variables(nir_builder *b, lower_ngg_nogs_state *s)
   {
      ASSERTED int vec_ok = u_vector_init(&s->reusable_nondeferred_variables, 4, sizeof(reusable_nondeferred_variable));
            /* Upper limit on reusable uniforms in order to reduce SGPR spilling. */
            nir_block *block = nir_start_block(b->impl);
   while (block) {
      /* Process the instructions in the current block. */
   nir_foreach_instr_safe(instr, block) {
      /* Determine if we can reuse the current SSA value.
   * When vertex compaction is used, it is possible that the same shader invocation
   * processes a different vertex in the top and bottom part of the shader.
   * Therefore, we only reuse uniform values.
   */
   nir_def *ssa = find_reusable_ssa_def(instr);
                  /* Determine a suitable type for the SSA value. */
   const struct glsl_type *t = glsl_uint_type_for_ssa(ssa);
                  if (!ssa->divergent) {
                                                /* Create a new NIR variable where we store the reusable value.
   * Then, we reload the variable and replace the uses of the value
   * with the reloaded variable.
   */
                  b->cursor = instr->type == nir_instr_type_phi
               nir_store_var(b, saved->var, saved->ssa, BITFIELD_MASK(ssa->num_components));
   nir_def *reloaded = nir_load_var(b, saved->var);
               /* Look at the next CF node. */
   nir_cf_node *next_cf_node = nir_cf_node_next(&block->cf_node);
   if (next_cf_node) {
                     /* Don't reuse if we're in divergent control flow.
   *
   * Thanks to vertex repacking, the same shader invocation may process a different vertex
   * in the top and bottom part, and it's even possible that this different vertex was initially
   * processed in a different wave. So the two parts may take a different divergent code path.
   * Therefore, these variables in divergent control flow may stay undefined.
   *
   * Note that this problem doesn't exist if vertices are not repacked or if the
   * workgroup only has a single wave.
   */
   bool next_is_divergent_if =
                  if (next_is_loop || next_is_divergent_if) {
      block = nir_cf_node_cf_tree_next(next_cf_node);
                  /* Go to the next block. */
         }
      /**
   * Reuses suitable variables from the top part of the shader,
   * by deleting their stores from the bottom part.
   */
   static void
   apply_reusable_variables(nir_builder *b, lower_ngg_nogs_state *s)
   {
      if (!u_vector_length(&s->reusable_nondeferred_variables)) {
      u_vector_finish(&s->reusable_nondeferred_variables);
               nir_foreach_block_reverse_safe(block, b->impl) {
      nir_foreach_instr_reverse_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                  /* When we found any of these intrinsics, it means
   * we reached the top part and we must stop.
   */
                  if (intrin->intrinsic != nir_intrinsic_store_deref)
         nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  reusable_nondeferred_variable *saved;
   u_vector_foreach(saved, &s->reusable_nondeferred_variables) {
      if (saved->var == deref->var) {
                           done:
      }
      static void
   cull_primitive_accepted(nir_builder *b, void *state)
   {
                        /* Store the accepted state to LDS for ES threads */
   for (unsigned vtx = 0; vtx < s->options->num_vertices_per_primitive; ++vtx)
      }
      static void
   clipdist_culling_es_part(nir_builder *b, lower_ngg_nogs_state *s,
         {
      /* no gl_ClipDistance used but we have user defined clip plane */
   if (s->options->user_clip_plane_enable_mask && !s->has_clipdist) {
      /* use gl_ClipVertex if defined */
   nir_variable *clip_vertex_var =
      b->shader->info.outputs_written & BITFIELD64_BIT(VARYING_SLOT_CLIP_VERTEX) ?
               /* clip against user defined clip planes */
   for (unsigned i = 0; i < 8; i++) {
                     nir_def *plane = nir_load_user_clip_plane(b, .ucp_id = i);
   nir_def *dist = nir_fdot(b, clip_vertex, plane);
                           /* store clipdist_neg_mask to LDS for culling latter in gs thread */
   if (s->has_clipdist) {
      nir_def *mask = nir_load_var(b, s->clipdist_neg_mask_var);
   nir_store_shared(b, nir_u2u8(b, mask), es_vertex_lds_addr,
         }
      static unsigned
   ngg_nogs_get_culling_pervertex_lds_size(gl_shader_stage stage,
                     {
      /* Culling shaders must repack some variables because
   * the same shader invocation may process different vertices
   * before and after the culling algorithm.
            unsigned num_repacked;
   if (stage == MESA_SHADER_VERTEX) {
      /* Vertex shaders repack:
   * - Vertex ID
   * - Instance ID (only if used)
   */
      } else {
      /* Tess eval shaders repack:
   * - U, V coordinates
   * - primitive ID (aka. patch id, only if used)
   * - relative patch id (not included here because doesn't need a dword)
   */
   assert(stage == MESA_SHADER_TESS_EVAL);
               if (num_repacked_variables)
            /* one odd dword to reduce LDS bank conflict */
      }
      static void
   add_deferred_attribute_culling(nir_builder *b, nir_cf_list *original_extracted_cf, lower_ngg_nogs_state *s)
   {
      bool uses_instance_id = BITSET_TEST(b->shader->info.system_values_read, SYSTEM_VALUE_INSTANCE_ID);
            unsigned num_repacked_variables;
   unsigned pervertex_lds_bytes =
      ngg_nogs_get_culling_pervertex_lds_size(b->shader->info.stage,
                              /* Create some helper variables. */
   nir_variable *gs_vtxaddr_vars[3] = {
      nir_local_variable_create(impl, glsl_uint_type(), "gs_vtx0_addr"),
   nir_local_variable_create(impl, glsl_uint_type(), "gs_vtx1_addr"),
               nir_variable *repacked_variables[3] = {
      nir_local_variable_create(impl, glsl_uint_type(), "repacked_var_0"),
   nir_local_variable_create(impl, glsl_uint_type(), "repacked_var_1"),
               /* Relative patch ID is a special case because it doesn't need an extra dword, repack separately. */
            if (s->options->clipdist_enable_mask ||
      s->options->user_clip_plane_enable_mask) {
   s->clip_vertex_var =
         s->clipdist_neg_mask_var =
            /* init mask to 0 */
               /* Top part of the culling shader (aka. position shader part)
   *
   * We clone the full ES shader and emit it here, but we only really care
   * about its position output, so we delete every other output from this part.
   * The position output is stored into a temporary variable, and reloaded later.
            nir_def *es_thread = has_input_vertex(b);
   nir_if *if_es_thread = nir_push_if(b, es_thread);
   {
      /* Initialize the position output variable to zeroes, in case not all VS/TES invocations store the output.
   * The spec doesn't require it, but we use (0, 0, 0, 1) because some games rely on that.
   */
            /* Now reinsert a clone of the shader code */
   struct hash_table *remap_table = _mesa_pointer_hash_table_create(NULL);
   nir_cf_list_clone_and_reinsert(original_extracted_cf, &if_es_thread->cf_node, b->cursor, remap_table);
   _mesa_hash_table_destroy(remap_table, NULL);
            /* Remember the current thread's shader arguments */
   if (b->shader->info.stage == MESA_SHADER_VERTEX) {
      nir_store_var(b, repacked_variables[0], nir_load_vertex_id_zero_base(b), 0x1u);
   if (uses_instance_id)
      } else if (b->shader->info.stage == MESA_SHADER_TESS_EVAL) {
      nir_store_var(b, s->repacked_rel_patch_id, nir_load_tess_rel_patch_id_amd(b), 0x1u);
   nir_def *tess_coord = nir_load_tess_coord(b);
   nir_store_var(b, repacked_variables[0], nir_channel(b, tess_coord, 0), 0x1u);
   nir_store_var(b, repacked_variables[1], nir_channel(b, tess_coord, 1), 0x1u);
   if (uses_tess_primitive_id)
      } else {
            }
            nir_store_var(b, s->es_accepted_var, es_thread, 0x1u);
   nir_def *gs_thread = has_input_primitive(b);
            /* Remove all non-position outputs, and put the position output into the variable. */
   nir_metadata_preserve(impl, nir_metadata_none);
   remove_culling_shader_outputs(b->shader, s);
                     /* Run culling algorithms if culling is enabled.
   *
   * NGG culling can be enabled or disabled in runtime.
   * This is determined by a SGPR shader argument which is accessed
   * by the following NIR intrinsic.
            nir_if *if_cull_en = nir_push_if(b, nir_load_cull_any_enabled_amd(b));
   {
      nir_def *invocation_index = nir_load_local_invocation_index(b);
            /* ES invocations store their vertex data to LDS for GS threads to read. */
   if_es_thread = nir_push_if(b, es_thread);
   if_es_thread->control = nir_selection_control_divergent_always_taken;
   {
      /* Store position components that are relevant to culling in LDS */
   nir_def *pre_cull_pos = nir_load_var(b, s->position_value_var);
   nir_def *pre_cull_w = nir_channel(b, pre_cull_pos, 3);
   nir_store_shared(b, pre_cull_w, es_vertex_lds_addr, .base = lds_es_pos_w);
   nir_def *pre_cull_x_div_w = nir_fdiv(b, nir_channel(b, pre_cull_pos, 0), pre_cull_w);
                                 /* For clipdist culling */
      }
            nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            nir_store_var(b, s->gs_accepted_var, nir_imm_false(b), 0x1u);
            /* GS invocations load the vertex data and perform the culling. */
   nir_if *if_gs_thread = nir_push_if(b, gs_thread);
   {
      /* Load vertex indices from input VGPRs */
   nir_def *vtx_idx[3] = {0};
   for (unsigned vertex = 0; vertex < s->options->num_vertices_per_primitive;
                           /* Load W positions of vertices first because the culling code will use these first */
   for (unsigned vtx = 0; vtx < s->options->num_vertices_per_primitive; ++vtx) {
      s->vtx_addr[vtx] = pervertex_lds_addr(b, vtx_idx[vtx], pervertex_lds_bytes);
   pos[vtx][3] = nir_load_shared(b, 1, 32, s->vtx_addr[vtx], .base = lds_es_pos_w);
               /* Load the X/W, Y/W positions of vertices */
   for (unsigned vtx = 0; vtx < s->options->num_vertices_per_primitive; ++vtx) {
      nir_def *xy = nir_load_shared(b, 2, 32, s->vtx_addr[vtx], .base = lds_es_pos_x);
   pos[vtx][0] = nir_channel(b, xy, 0);
               nir_def *accepted_by_clipdist;
   if (s->has_clipdist) {
      nir_def *clipdist_neg_mask = nir_imm_intN_t(b, 0xff, 8);
   for (unsigned vtx = 0; vtx < s->options->num_vertices_per_primitive; ++vtx) {
      nir_def *mask =
      nir_load_shared(b, 1, 8, s->vtx_addr[vtx],
         }
   /* primitive is culled if any plane's clipdist of all vertices are negative */
      } else {
                  /* See if the current primitive is accepted */
   ac_nir_cull_primitive(b, accepted_by_clipdist, pos,
            }
            nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
                     /* ES invocations load their accepted flag from LDS. */
   if_es_thread = nir_push_if(b, es_thread);
   if_es_thread->control = nir_selection_control_divergent_always_taken;
   {
      nir_def *accepted = nir_load_shared(b, 1, 8u, es_vertex_lds_addr, .base = lds_es_vertex_accepted, .align_mul = 4u);
   nir_def *accepted_bool = nir_ine_imm(b, nir_u2u32(b, accepted), 0);
      }
                     /* Repack the vertices that survived the culling. */
   wg_repack_result rep = repack_invocations_in_workgroup(b, es_accepted, lds_scratch_base,
               nir_def *num_live_vertices_in_workgroup = rep.num_repacked_invocations;
            /* If all vertices are culled, set primitive count to 0 as well. */
   nir_def *num_exported_prims = nir_load_workgroup_num_input_primitives_amd(b);
   nir_def *fully_culled = nir_ieq_imm(b, num_live_vertices_in_workgroup, 0u);
   num_exported_prims = nir_bcsel(b, fully_culled, nir_imm_int(b, 0u), num_exported_prims);
            nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, nir_load_subgroup_id(b), 0));
   {
      /* Tell the final vertex and primitive count to the HW. */
   if (s->options->gfx_level == GFX10) {
      alloc_vertices_and_primitives_gfx10_workaround(
      } else {
      alloc_vertices_and_primitives(
         }
            /* Vertex compaction. */
   compact_vertices_after_culling(b, s,
                        }
   nir_push_else(b, if_cull_en);
   {
      /* When culling is disabled, we do the same as we would without culling. */
   nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, nir_load_subgroup_id(b), 0));
   {
      nir_def *vtx_cnt = nir_load_workgroup_num_input_vertices_amd(b);
   nir_def *prim_cnt = nir_load_workgroup_num_input_primitives_amd(b);
      }
   nir_pop_if(b, if_wave_0);
      }
            /* Update shader arguments.
   *
   * The registers which hold information about the subgroup's
   * vertices and primitives are updated here, so the rest of the shader
   * doesn't need to worry about the culling.
   *
   * These "overwrite" intrinsics must be at top level control flow,
   * otherwise they can mess up the backend (eg. ACO's SSA).
   *
   * TODO:
   * A cleaner solution would be to simply replace all usages of these args
   * with the load of the variables.
   * However, this wouldn't work right now because the backend uses the arguments
   * for purposes not expressed in NIR, eg. VS input loads, etc.
   * This can change if VS input loads and other stuff are lowered to eg. load_buffer_amd.
            if (b->shader->info.stage == MESA_SHADER_VERTEX)
      s->overwrite_args =
      nir_overwrite_vs_arguments_amd(b,
   else if (b->shader->info.stage == MESA_SHADER_TESS_EVAL)
      s->overwrite_args =
      nir_overwrite_tes_arguments_amd(b,
         else
      }
      static void
   ngg_nogs_store_edgeflag_to_lds(nir_builder *b, lower_ngg_nogs_state *s)
   {
      if (!s->outputs[VARYING_SLOT_EDGE][0])
            /* clamp user edge flag to 1 for latter bit operations */
   nir_def *edgeflag = s->outputs[VARYING_SLOT_EDGE][0];
            /* user edge flag is stored at the beginning of a vertex if streamout is not enabled */
   unsigned offset = 0;
   if (s->streamout_enabled) {
      unsigned packed_location =
                     nir_def *tid = nir_load_local_invocation_index(b);
               }
      static void
   ngg_nogs_store_xfb_outputs_to_lds(nir_builder *b, lower_ngg_nogs_state *s)
   {
               uint64_t xfb_outputs = 0;
   unsigned xfb_outputs_16bit = 0;
   uint8_t xfb_mask[VARYING_SLOT_MAX] = {0};
   uint8_t xfb_mask_16bit_lo[16] = {0};
            /* Get XFB output mask for each slot. */
   for (int i = 0; i < info->output_count; i++) {
               if (out->location < VARYING_SLOT_VAR0_16BIT) {
      xfb_outputs |= BITFIELD64_BIT(out->location);
      } else {
                     if (out->high_16bits)
         else
                  nir_def *tid = nir_load_local_invocation_index(b);
            u_foreach_bit64(slot, xfb_outputs) {
      unsigned packed_location =
                     /* Clear unused components. */
   for (unsigned i = 0; i < 4; i++) {
      if (!s->outputs[slot][i])
               while (mask) {
      int start, count;
   u_bit_scan_consecutive_range(&mask, &start, &count);
   /* Outputs here are sure to be 32bit.
   *
   * 64bit outputs have been lowered to two 32bit. As 16bit outputs:
   *   Vulkan does not allow streamout outputs less than 32bit.
   *   OpenGL puts 16bit outputs in VARYING_SLOT_VAR0_16BIT.
   */
   nir_def *store_val = nir_vec(b, &s->outputs[slot][start], (unsigned)count);
                  unsigned num_32bit_outputs = util_bitcount64(b->shader->info.outputs_written);
   u_foreach_bit64(slot, xfb_outputs_16bit) {
      unsigned packed_location = num_32bit_outputs +
            unsigned mask_lo = xfb_mask_16bit_lo[slot];
            /* Clear unused components. */
   for (unsigned i = 0; i < 4; i++) {
      if (!s->outputs_16bit_lo[slot][i])
         if (!s->outputs_16bit_hi[slot][i])
               nir_def **outputs_lo = s->outputs_16bit_lo[slot];
   nir_def **outputs_hi = s->outputs_16bit_hi[slot];
            unsigned mask = mask_lo | mask_hi;
   while (mask) {
                     nir_def *values[4] = {0};
   for (int c = start; c < start + count; ++c) {
                     /* extend 8/16 bit to 32 bit, 64 bit has been lowered */
               nir_def *store_val = nir_vec(b, values, (unsigned)count);
            }
      static void
   ngg_build_streamout_buffer_info(nir_builder *b,
                                 nir_xfb_info *info,
   bool has_xfb_prim_query,
   nir_def *scratch_base,
   nir_def *tid_in_tg,
   {
               /* For radeonsi which pass this value by arg when VS. Streamout need accurate
   * num-vert-per-prim for writing correct amount of data to buffer.
   */
   nir_def *num_vert_per_prim = nir_load_num_vertices_per_primitive_amd(b);
   for (unsigned buffer = 0; buffer < 4; buffer++) {
      if (!(info->buffers_written & BITFIELD_BIT(buffer)))
                     prim_stride_ret[buffer] =
                     nir_if *if_invocation_0 = nir_push_if(b, nir_ieq_imm(b, tid_in_tg, 0));
   {
      nir_def *workgroup_buffer_sizes[4];
   for (unsigned buffer = 0; buffer < 4; buffer++) {
      if (info->buffers_written & BITFIELD_BIT(buffer)) {
      nir_def *buffer_size = nir_channel(b, so_buffer_ret[buffer], 2);
   /* In radeonsi, we may not know if a feedback buffer has been bound when
   * compile time, so have to check buffer size in runtime to disable the
   * GDS update for unbind buffer to prevent the case that previous draw
   * compiled with streamout but does not bind feedback buffer miss update
   * GDS which will affect current draw's streamout.
   */
   nir_def *buffer_valid = nir_ine_imm(b, buffer_size, 0);
   nir_def *inc_buffer_size =
         workgroup_buffer_sizes[buffer] =
      } else
               nir_def *ordered_id = nir_load_ordered_id_amd(b);
   /* Get current global offset of buffer and increase by amount of
   * workgroup buffer size. This is an ordered operation sorted by
   * ordered_id; Each buffer info is in a channel of a vec4.
   */
   nir_def *buffer_offsets =
      nir_ordered_xfb_counter_add_amd(b, ordered_id, nir_vec(b, workgroup_buffer_sizes, 4),
               nir_def *emit_prim[4];
            nir_def *any_overflow = nir_imm_false(b);
            for (unsigned buffer = 0; buffer < 4; buffer++) {
                              /* Only consider overflow for valid feedback buffers because
   * otherwise the ordered operation above (GDS atomic return) might
   * return non-zero offsets for invalid buffers.
   */
   nir_def *buffer_valid = nir_ine_imm(b, buffer_size, 0);
                  nir_def *remain_size = nir_isub(b, buffer_size, buffer_offset);
                  any_overflow = nir_ior(b, any_overflow, overflow);
                  unsigned stream = info->buffer_to_stream[buffer];
   /* when previous workgroup overflow, we can't emit any primitive */
   emit_prim[stream] = nir_bcsel(
      b, overflow, nir_imm_int(b, 0),
               /* Save to LDS for being accessed by other waves in this workgroup. */
               /* We have to fix up the streamout offsets if we overflowed because they determine
   * the vertex count for DrawTransformFeedback.
   */
   nir_if *if_any_overflow = nir_push_if(b, any_overflow);
   {
      nir_xfb_counter_sub_amd(b, nir_vec(b, overflow_amount, 4),
            }
            /* Save to LDS for being accessed by other waves in this workgroup. */
   for (unsigned stream = 0; stream < 4; stream++) {
                                 /* Update shader query. */
   if (has_xfb_prim_query) {
      nir_if *if_shader_query = nir_push_if(b, nir_load_prim_xfb_query_enabled_amd(b));
   {
      for (unsigned stream = 0; stream < 4; stream++) {
      if (info->streams_written & BITFIELD_BIT(stream))
         }
         }
            nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                        /* Fetch the per-buffer offsets in all waves. */
   for (unsigned buffer = 0; buffer < 4; buffer++) {
      if (!(info->buffers_written & BITFIELD_BIT(buffer)))
            buffer_offsets_ret[buffer] =
               /* Fetch the per-stream emit prim in all waves. */
   for (unsigned stream = 0; stream < 4; stream++) {
      if (!(info->streams_written & BITFIELD_BIT(stream)))
            emit_prim_ret[stream] =
         }
      static void
   ngg_build_streamout_vertex(nir_builder *b, nir_xfb_info *info,
                           {
      nir_def *vtx_buffer_offsets[4];
   for (unsigned buffer = 0; buffer < 4; buffer++) {
      if (!(info->buffers_written & BITFIELD_BIT(buffer)))
            nir_def *offset = nir_imul_imm(b, vtx_buffer_idx, info->buffers[buffer].stride);
               for (unsigned i = 0; i < info->output_count; i++) {
      nir_xfb_output_info *out = info->outputs + i;
   if (!out->component_mask || info->buffer_to_stream[out->buffer] != stream)
            unsigned base;
   if (out->location >= VARYING_SLOT_VAR0_16BIT) {
      base =
      util_bitcount64(b->shader->info.outputs_written) +
   util_bitcount(b->shader->info.outputs_written_16bit &
   } else {
      base =
      util_bitcount64(b->shader->info.outputs_written &
            unsigned offset = (base * 4 + out->component_offset) * 4;
                     nir_def *out_data =
            /* Up-scaling 16bit outputs to 32bit.
   *
   * OpenGL ES will put 16bit medium precision varyings to VARYING_SLOT_VAR0_16BIT.
   * We need to up-scaling them to 32bit when streamout to buffer.
   *
   * Vulkan does not allow 8/16bit varyings to be streamout.
   */
   if (out->location >= VARYING_SLOT_VAR0_16BIT) {
                     for (int j = 0; j < count; j++) {
      unsigned c = out->component_offset + j;
                  if (out->high_16bits) {
      v = nir_unpack_32_2x16_split_y(b, v);
      } else {
                        t = nir_alu_type_get_base_type(t);
                           nir_def *zero = nir_imm_int(b, 0);
   nir_store_buffer_amd(b, out_data, so_buffer[out->buffer],
                        vtx_buffer_offsets[out->buffer],
      }
      static void
   ngg_nogs_build_streamout(nir_builder *b, lower_ngg_nogs_state *s)
   {
                        /* Get global buffer offset where this workgroup will stream out data to. */
   nir_def *generated_prim = nir_load_workgroup_num_input_primitives_amd(b);
   nir_def *gen_prim_per_stream[4] = {generated_prim, 0, 0, 0};
   nir_def *emit_prim_per_stream[4] = {0};
   nir_def *buffer_offsets[4] = {0};
   nir_def *so_buffer[4] = {0};
   nir_def *prim_stride[4] = {0};
   nir_def *tid_in_tg = nir_load_local_invocation_index(b);
   ngg_build_streamout_buffer_info(b, info, s->options->has_xfb_prim_query,
                              /* Write out primitive data */
   nir_if *if_emit = nir_push_if(b, nir_ilt(b, tid_in_tg, emit_prim_per_stream[0]));
   {
      unsigned vtx_lds_stride = (b->shader->num_outputs * 4 + 1) * 4;
   nir_def *num_vert_per_prim = nir_load_num_vertices_per_primitive_amd(b);
            for (unsigned i = 0; i < s->options->num_vertices_per_primitive; i++) {
      nir_if *if_valid_vertex =
         {
      nir_def *vtx_lds_idx = nir_load_var(b, s->gs_vtx_indices_vars[i]);
   nir_def *vtx_lds_addr = pervertex_lds_addr(b, vtx_lds_idx, vtx_lds_stride);
   ngg_build_streamout_vertex(b, info, 0, so_buffer, buffer_offsets,
            }
         }
            /* Wait streamout memory ops done before export primitive, otherwise it
   * may not finish when shader ends.
   *
   * If a shader has no param exports, rasterization can start before
   * the shader finishes and thus memory stores might not finish before
   * the pixel shader starts.
   *
   * TODO: we only need this when no param exports.
   *
   * TODO: not sure if we need this barrier when late prim export, as I
   *       can't observe test fail without this barrier.
   */
      }
      static unsigned
   ngg_nogs_get_pervertex_lds_size(gl_shader_stage stage,
                           {
               if (streamout_enabled) {
      /* The extra dword is used to avoid LDS bank conflicts and store the primitive id.
   * TODO: only alloc space for outputs that really need streamout.
   */
               bool need_prim_id_store_shared = export_prim_id && stage == MESA_SHADER_VERTEX;
   if (need_prim_id_store_shared || has_user_edgeflags) {
      unsigned size = 0;
   if (need_prim_id_store_shared)
         if (has_user_edgeflags)
            /* pad to odd dwords to avoid LDS bank conflict */
                           }
      static void
   ngg_nogs_gather_outputs(nir_builder *b, struct exec_list *cf_list, lower_ngg_nogs_state *s)
   {
      /* Assume:
   * - the shader used nir_lower_io_to_temporaries
   * - 64-bit outputs are lowered
   * - no indirect indexing is present
   */
   struct nir_cf_node *first_node =
            for (nir_block *block = nir_cf_node_cf_tree_first(first_node); block != NULL;
      block = nir_block_cf_tree_next(block)) {
   nir_foreach_instr_safe (instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                                          nir_def **output;
   nir_alu_type *type;
   if (slot >= VARYING_SLOT_VAR0_16BIT) {
      unsigned index = slot - VARYING_SLOT_VAR0_16BIT;
   if (sem.high_16bits) {
      output = s->outputs_16bit_hi[index];
      } else {
      output = s->outputs_16bit_lo[index];
         } else {
      output = s->outputs[slot];
               unsigned component = nir_intrinsic_component(intrin);
                  u_foreach_bit (i, write_mask) {
      unsigned c = component + i;
   output[c] = nir_channel(b, intrin->src[0].ssa, i);
               /* remove all store output instructions */
            }
      static unsigned
   gather_vs_outputs(nir_builder *b, vs_output *outputs,
                     const uint8_t *param_offsets,
   {
      unsigned num_outputs = 0;
   u_foreach_bit64 (slot, b->shader->info.outputs_written) {
      if (param_offsets[slot] > AC_EXP_PARAM_OFFSET_31)
                     /* skip output if no one written before */
   if (!output[0] && !output[1] && !output[2] && !output[3])
            outputs[num_outputs].slot = slot;
   for (int i = 0; i < 4; i++) {
      nir_def *chan = output[i];
   /* RADV implements 16-bit outputs as 32-bit with VARYING_SLOT_VAR0-31. */
      }
               u_foreach_bit (i, b->shader->info.outputs_written_16bit) {
      unsigned slot = VARYING_SLOT_VAR0_16BIT + i;
   if (param_offsets[slot] > AC_EXP_PARAM_OFFSET_31)
            nir_def **output_lo = data_16bit_lo[i];
            /* skip output if no one written before */
   if (!output_lo[0] && !output_lo[1] && !output_lo[2] && !output_lo[3] &&
                  vs_output *output = &outputs[num_outputs++];
            nir_def *undef = nir_undef(b, 1, 16);
   for (int j = 0; j < 4; j++) {
      nir_def *lo = output_lo[j] ? output_lo[j] : undef;
   nir_def *hi = output_hi[j] ? output_hi[j] : undef;
   if (output_lo[j] || output_hi[j])
         else
                     }
      static void
   create_vertex_param_phis(nir_builder *b, unsigned num_outputs, vs_output *outputs)
   {
               for (unsigned i = 0; i < num_outputs; i++) {
      for (unsigned j = 0; j < 4; j++) {
      if (outputs[i].chan[j])
            }
      static void
   export_vertex_params_gfx11(nir_builder *b, nir_def *export_tid, nir_def *num_export_threads,
               {
               /* We should always store full vec4s in groups of 8 lanes for the best performance even if
   * some of them are garbage or have unused components, so align the number of export threads
   * to 8.
   */
   num_export_threads = nir_iand_imm(b, nir_iadd_imm(b, num_export_threads, 7), ~7);
   if (!export_tid)
         else
            nir_def *attr_offset = nir_load_ring_attr_offset_amd(b);
   nir_def *vindex = nir_load_local_invocation_index(b);
   nir_def *voffset = nir_imm_int(b, 0);
                     for (unsigned i = 0; i < num_outputs; i++) {
      gl_varying_slot slot = outputs[i].slot;
            /* Since vs_output_param_offset[] can map multiple varying slots to
   * the same param export index (that's radeonsi-specific behavior),
   * we need to do this so as not to emit duplicated exports.
   */
   if (exported_params & BITFIELD_BIT(offset))
            nir_def *comp[4];
   for (unsigned j = 0; j < 4; j++)
         nir_store_buffer_amd(b, nir_vec(b, comp, 4), attr_rsrc, voffset, attr_offset, vindex,
                                    }
      static bool must_wait_attr_ring(enum amd_gfx_level gfx_level, bool has_param_exports)
   {
         }
      static void
   export_pos0_wait_attr_ring(nir_builder *b, nir_if *if_es_thread, nir_def *outputs[VARYING_SLOT_MAX][4], const ac_nir_lower_ngg_options *options)
   {
               /* Create phi for the position output values. */
   vs_output pos_output = {
      .slot = VARYING_SLOT_POS,
   .chan = {
      outputs[VARYING_SLOT_POS][0],
   outputs[VARYING_SLOT_POS][1],
   outputs[VARYING_SLOT_POS][2],
         };
                     /* Wait for attribute stores to finish. */
   nir_barrier(b, .execution_scope = SCOPE_SUBGROUP,
                        /* Export just the pos0 output. */
   nir_if *if_export_empty_pos = nir_push_if(b, if_es_thread->condition.ssa);
   {
      nir_def *pos_output_array[VARYING_SLOT_MAX][4] = {0};
            ac_nir_export_position(b, options->gfx_level,
                        }
      }
      static void
   nogs_export_vertex_params(nir_builder *b, nir_function_impl *impl,
               {
      if (!s->options->has_param_exports)
            if (s->options->gfx_level >= GFX11) {
      /* Export varyings for GFX11+ */
   vs_output outputs[64];
   const unsigned num_outputs =
      gather_vs_outputs(b, outputs,
                           if (!num_outputs)
            b->cursor = nir_after_cf_node(&if_es_thread->cf_node);
            b->cursor = nir_after_impl(impl);
   if (!num_es_threads)
            export_vertex_params_gfx11(b, NULL, num_es_threads, num_outputs, outputs,
      } else {
      ac_nir_export_parameters(b, s->options->vs_output_param_offset,
                           }
      void
   ac_nir_lower_ngg_nogs(nir_shader *shader, const ac_nir_lower_ngg_options *options)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   assert(impl);
   assert(options->max_workgroup_size && options->wave_size);
            nir_variable *position_value_var = nir_local_variable_create(impl, glsl_vec4_type(), "position_value");
   nir_variable *prim_exp_arg_var = nir_local_variable_create(impl, glsl_uint_type(), "prim_exp_arg");
   nir_variable *es_accepted_var =
         nir_variable *gs_accepted_var =
                  bool streamout_enabled = shader->xfb_info && !options->disable_streamout;
   bool has_user_edgeflags =
         /* streamout need to be done before either prim or vertex export. Because when no
   * param export, rasterization can start right after prim and vertex export,
   * which left streamout buffer writes un-finished.
   *
   * Always use late prim export when user edge flags are enabled.
   * This is because edge flags are written by ES threads but they
   * are exported by GS threads as part of th primitive export.
   */
   bool early_prim_export =
            lower_ngg_nogs_state state = {
      .options = options,
   .early_prim_export = early_prim_export,
   .streamout_enabled = streamout_enabled,
   .position_value_var = position_value_var,
   .prim_exp_arg_var = prim_exp_arg_var,
   .es_accepted_var = es_accepted_var,
   .gs_accepted_var = gs_accepted_var,
   .gs_exported_var = gs_exported_var,
   .max_num_waves = DIV_ROUND_UP(options->max_workgroup_size, options->wave_size),
               const bool need_prim_id_store_shared =
            if (options->export_primitive_id) {
      nir_variable *prim_id_var = nir_variable_create(shader, nir_var_shader_out, glsl_uint_type(), "ngg_prim_id");
   prim_id_var->data.location = VARYING_SLOT_PRIMITIVE_ID;
   prim_id_var->data.driver_location = VARYING_SLOT_PRIMITIVE_ID;
   prim_id_var->data.interpolation = INTERP_MODE_NONE;
               nir_builder builder = nir_builder_create(impl);
            if (options->can_cull) {
      analyze_shader_before_culling(shader, &state);
               nir_cf_list extracted;
   nir_cf_extract(&extracted, nir_before_impl(impl),
                           /* Emit primitives generated query code here, so that
   * it executes before culling and isn't in the extracted CF.
   */
            /* Whether a shader invocation should export a primitive,
   * initialize to all invocations that have an input primitive.
   */
            if (!options->can_cull) {
      /* Newer chips can use PRIMGEN_PASSTHRU_NO_MSG to skip gs_alloc_req for NGG passthrough. */
   if (!(options->passthrough && options->family >= CHIP_NAVI23)) {
      /* Allocate export space on wave 0 - confirm to the HW that we want to use all possible space */
   nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, nir_load_subgroup_id(b), 0));
   {
      nir_def *vtx_cnt = nir_load_workgroup_num_input_vertices_amd(b);
   nir_def *prim_cnt = nir_load_workgroup_num_input_primitives_amd(b);
      }
               /* Take care of early primitive export, otherwise just pack the primitive export argument */
   if (state.early_prim_export)
         else
      } else {
      add_deferred_attribute_culling(b, &extracted, &state);
            if (state.early_prim_export)
            /* Wait for culling to finish using LDS. */
   if (need_prim_id_store_shared || has_user_edgeflags) {
      nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                              /* determine the LDS vertex stride */
   state.pervertex_lds_bytes =
      ngg_nogs_get_pervertex_lds_size(shader->info.stage,
                           if (need_prim_id_store_shared) {
               /* Wait for GS threads to store primitive ID in LDS. */
   nir_barrier(b, .execution_scope = SCOPE_WORKGROUP, .memory_scope = SCOPE_WORKGROUP,
               nir_def *es_thread =
            /* Calculate the bit count here instead of below for lower SGPR usage and better ALU
   * scheduling.
   */
   nir_def *num_es_threads = NULL;
   if (state.options->gfx_level >= GFX11 && options->can_cull) {
      nir_def *es_accepted_mask =
                     nir_if *if_es_thread = nir_push_if(b, es_thread);
   {
      /* Run the actual shader */
   nir_cf_reinsert(&extracted, b->cursor);
            if (options->export_primitive_id)
      }
            if (options->can_cull) {
      /* Replace uniforms. */
            /* Remove the redundant position output. */
            /* After looking at the performance in apps eg. Doom Eternal, and The Witcher 3,
   * it seems that it's best to put the position export always at the end, and
   * then let ACO schedule it up (slightly) only when early prim export is used.
   */
            nir_def *pos_val = nir_load_var(b, state.position_value_var);
   for (int i = 0; i < 4; i++)
               /* Gather outputs data and types */
   b->cursor = nir_after_cf_list(&if_es_thread->then_list);
            if (state.has_user_edgeflags)
            if (state.streamout_enabled) {
      /* TODO: support culling after streamout. */
                     b->cursor = nir_after_impl(impl);
               /* Take care of late primitive export */
   if (!state.early_prim_export) {
      b->cursor = nir_after_impl(impl);
               uint64_t export_outputs = shader->info.outputs_written | VARYING_BIT_POS;
   if (options->kill_pointsize)
            const bool wait_attr_ring = must_wait_attr_ring(options->gfx_level, options->has_param_exports);
   if (wait_attr_ring)
                     ac_nir_export_position(b, options->gfx_level,
                                       if (wait_attr_ring)
            nir_metadata_preserve(impl, nir_metadata_none);
            /* Cleanup */
   nir_opt_dead_write_vars(shader);
   nir_lower_vars_to_ssa(shader);
   nir_remove_dead_variables(shader, nir_var_function_temp, NULL);
   nir_lower_alu_to_scalar(shader, NULL, NULL);
            if (options->can_cull) {
      /* It's beneficial to redo these opts after splitting the shader. */
   nir_opt_sink(shader, nir_move_load_input | nir_move_const_undef | nir_move_copies);
               bool progress;
   do {
      progress = false;
   NIR_PASS(progress, shader, nir_opt_undef);
   NIR_PASS(progress, shader, nir_opt_dce);
            if (options->can_cull)
         }
      /**
   * Return the address of the LDS storage reserved for the N'th vertex,
   * where N is in emit order, meaning:
   * - during the finale, N is the invocation_index (within the workgroup)
   * - during vertex emit, i.e. while the API GS shader invocation is running,
   *   N = invocation_index * gs_max_out_vertices + emit_idx
   *   where emit_idx is the vertex index in the current API GS invocation.
   *
   * Goals of the LDS memory layout:
   * 1. Eliminate bank conflicts on write for geometry shaders that have all emits
   *    in uniform control flow
   * 2. Eliminate bank conflicts on read for export if, additionally, there is no
   *    culling
   * 3. Agnostic to the number of waves (since we don't know it before compiling)
   * 4. Allow coalescing of LDS instructions (ds_write_b128 etc.)
   * 5. Avoid wasting memory.
   *
   * We use an AoS layout due to point 4 (this also helps point 3). In an AoS
   * layout, elimination of bank conflicts requires that each vertex occupy an
   * odd number of dwords. We use the additional dword to store the output stream
   * index as well as a flag to indicate whether this vertex ends a primitive
   * for rasterization.
   *
   * Swizzling is required to satisfy points 1 and 2 simultaneously.
   *
   * Vertices are stored in export order (gsthread * gs_max_out_vertices + emitidx).
   * Indices are swizzled in groups of 32, which ensures point 1 without
   * disturbing point 2.
   *
   * \return an LDS pointer to type {[N x i32], [4 x i8]}
   */
   static nir_def *
   ngg_gs_out_vertex_addr(nir_builder *b, nir_def *out_vtx_idx, lower_ngg_gs_state *s)
   {
               /* gs_max_out_vertices = 2^(write_stride_2exp) * some odd number */
   if (write_stride_2exp) {
      nir_def *row = nir_ushr_imm(b, out_vtx_idx, 5);
   nir_def *swizzle = nir_iand_imm(b, row, (1u << write_stride_2exp) - 1u);
               nir_def *out_vtx_offs = nir_imul_imm(b, out_vtx_idx, s->lds_bytes_per_gs_out_vertex);
      }
      static nir_def *
   ngg_gs_emit_vertex_addr(nir_builder *b, nir_def *gs_vtx_idx, lower_ngg_gs_state *s)
   {
      nir_def *tid_in_tg = nir_load_local_invocation_index(b);
   nir_def *gs_out_vtx_base = nir_imul_imm(b, tid_in_tg, b->shader->info.gs.vertices_out);
               }
      static void
   ngg_gs_clear_primflags(nir_builder *b, nir_def *num_vertices, unsigned stream, lower_ngg_gs_state *s)
   {
      char name[32];
   snprintf(name, sizeof(name), "clear_primflag_idx_%u", stream);
            nir_def *zero_u8 = nir_imm_zero(b, 1, 8);
            nir_loop *loop = nir_push_loop(b);
   {
      nir_def *clear_primflag_idx = nir_load_var(b, clear_primflag_idx_var);
   nir_if *if_break = nir_push_if(b, nir_uge_imm(b, clear_primflag_idx, b->shader->info.gs.vertices_out));
   {
         }
   nir_push_else(b, if_break);
   {
      nir_def *emit_vtx_addr = ngg_gs_emit_vertex_addr(b, clear_primflag_idx, s);
   nir_store_shared(b, zero_u8, emit_vtx_addr, .base = s->lds_offs_primflags + stream);
      }
      }
      }
      static bool
   lower_ngg_gs_store_output(nir_builder *b, nir_intrinsic_instr *intrin, lower_ngg_gs_state *s)
   {
      assert(nir_src_is_const(intrin->src[1]) && !nir_src_as_uint(intrin->src[1]));
            unsigned writemask = nir_intrinsic_write_mask(intrin);
   unsigned component_offset = nir_intrinsic_component(intrin);
                     nir_def *store_val = intrin->src[0].ssa;
            /* Small bitsize components consume the same amount of space as 32-bit components,
   * but 64-bit ones consume twice as many. (Vulkan spec 15.1.5)
   *
   * 64-bit IO has been lowered to multi 32-bit IO.
   */
   assert(store_val->bit_size <= 32);
            /* Get corresponding output variable and usage info. */
   nir_def **output;
   nir_alu_type *type;
   gs_output_info *info;
   if (location >= VARYING_SLOT_VAR0_16BIT) {
      unsigned index = location - VARYING_SLOT_VAR0_16BIT;
            if (io_sem.high_16bits) {
      output = s->outputs_16bit_hi[index];
   type = s->output_types.types_16bit_hi[index];
      } else {
      output = s->outputs_16bit_lo[index];
   type = s->output_types.types_16bit_lo[index];
         } else {
      assert(location < VARYING_SLOT_MAX);
   output = s->outputs[location];
   type = s->output_types.types[location];
               for (unsigned comp = 0; comp < store_val->num_components; ++comp) {
      if (!(writemask & (1 << comp)))
         unsigned stream = (io_sem.gs_streams >> (comp * 2)) & 0x3;
   if (!(b->shader->info.gs.active_stream_mask & (1 << stream)))
                     /* The same output component should always belong to the same stream. */
   assert(!(info->components_mask & (1 << component)) ||
            /* Components of the same output slot may belong to different streams. */
   info->stream |= stream << (component * 2);
            /* If type is set multiple times, the value must be same. */
   assert(type[component] == nir_type_invalid || type[component] == src_type);
            /* Assume we have called nir_lower_io_to_temporaries which store output in the
   * same block as EmitVertex, so we don't need to use nir_variable for outputs.
   */
               nir_instr_remove(&intrin->instr);
      }
      static unsigned
   gs_output_component_mask_with_stream(gs_output_info *info, unsigned stream)
   {
      unsigned mask = info->components_mask;
   if (!mask)
            /* clear component when not requested stream */
   for (int i = 0; i < 4; i++) {
      if (((info->stream >> (i * 2)) & 3) != stream)
                  }
      static bool
   lower_ngg_gs_emit_vertex_with_counter(nir_builder *b, nir_intrinsic_instr *intrin, lower_ngg_gs_state *s)
   {
               unsigned stream = nir_intrinsic_stream_id(intrin);
   if (!(b->shader->info.gs.active_stream_mask & (1 << stream))) {
      nir_instr_remove(&intrin->instr);
               nir_def *gs_emit_vtx_idx = intrin->src[0].ssa;
   nir_def *current_vtx_per_prim = intrin->src[1].ssa;
            u_foreach_bit64(slot, b->shader->info.outputs_written) {
      unsigned packed_location = util_bitcount64((b->shader->info.outputs_written & BITFIELD64_MASK(slot)));
   gs_output_info *info = &s->output_info[slot];
            unsigned mask = gs_output_component_mask_with_stream(info, stream);
   while (mask) {
      int start, count;
   u_bit_scan_consecutive_range(&mask, &start, &count);
   nir_def *values[4] = {0};
   for (int c = start; c < start + count; ++c) {
      if (!output[c]) {
      /* no one write to this output before */
                     /* extend 8/16 bit to 32 bit, 64 bit has been lowered */
               nir_def *store_val = nir_vec(b, values, (unsigned)count);
   nir_store_shared(b, store_val, gs_emit_vtx_addr,
                     /* Clear all outputs (they are undefined after emit_vertex) */
               /* Store 16bit outputs to LDS. */
   unsigned num_32bit_outputs = util_bitcount64(b->shader->info.outputs_written);
   u_foreach_bit(slot, b->shader->info.outputs_written_16bit) {
      unsigned packed_location = num_32bit_outputs +
            unsigned mask_lo = gs_output_component_mask_with_stream(s->output_info_16bit_lo + slot, stream);
   unsigned mask_hi = gs_output_component_mask_with_stream(s->output_info_16bit_hi + slot, stream);
            nir_def **output_lo = s->outputs_16bit_lo[slot];
   nir_def **output_hi = s->outputs_16bit_hi[slot];
            while (mask) {
      int start, count;
   u_bit_scan_consecutive_range(&mask, &start, &count);
   nir_def *values[4] = {0};
   for (int c = start; c < start + count; ++c) {
                                 nir_def *store_val = nir_vec(b, values, (unsigned)count);
   nir_store_shared(b, store_val, gs_emit_vtx_addr,
                     /* Clear all outputs (they are undefined after emit_vertex) */
   memset(s->outputs_16bit_lo[slot], 0, sizeof(s->outputs_16bit_lo[slot]));
               /* Calculate and store per-vertex primitive flags based on vertex counts:
   * - bit 0: whether this vertex finishes a primitive (a real primitive, not the strip)
   * - bit 1: whether the primitive index is odd (if we are emitting triangle strips, otherwise always 0)
   *          only set when the vertex also finishes the primitive
   * - bit 2: whether vertex is live (if culling is enabled: set after culling, otherwise always 1)
            nir_def *vertex_live_flag =
      !stream && s->options->can_cull
               nir_def *completes_prim = nir_ige_imm(b, current_vtx_per_prim, s->num_vertices_per_primitive - 1);
            nir_def *prim_flag = nir_ior(b, vertex_live_flag, complete_flag);
   if (s->num_vertices_per_primitive == 3) {
      nir_def *odd = nir_iand(b, current_vtx_per_prim, complete_flag);
   nir_def *odd_flag = nir_ishl_imm(b, odd, 1);
               nir_store_shared(b, nir_u2u8(b, prim_flag), gs_emit_vtx_addr,
                  nir_instr_remove(&intrin->instr);
      }
      static bool
   lower_ngg_gs_end_primitive_with_counter(nir_builder *b, nir_intrinsic_instr *intrin, UNUSED lower_ngg_gs_state *s)
   {
               /* These are not needed, we can simply remove them */
   nir_instr_remove(&intrin->instr);
      }
      static bool
   lower_ngg_gs_set_vertex_and_primitive_count(nir_builder *b, nir_intrinsic_instr *intrin, lower_ngg_gs_state *s)
   {
               unsigned stream = nir_intrinsic_stream_id(intrin);
   if (stream > 0 && !(b->shader->info.gs.active_stream_mask & (1 << stream))) {
      nir_instr_remove(&intrin->instr);
               s->vertex_count[stream] = intrin->src[0].ssa;
            /* Clear the primitive flags of non-emitted vertices */
   if (!nir_src_is_const(intrin->src[0]) || nir_src_as_uint(intrin->src[0]) < b->shader->info.gs.vertices_out)
            nir_instr_remove(&intrin->instr);
      }
      static bool
   lower_ngg_gs_intrinsic(nir_builder *b, nir_instr *instr, void *state)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intrin->intrinsic == nir_intrinsic_store_output)
         else if (intrin->intrinsic == nir_intrinsic_emit_vertex_with_counter)
         else if (intrin->intrinsic == nir_intrinsic_end_primitive_with_counter)
         else if (intrin->intrinsic == nir_intrinsic_set_vertex_and_primitive_count)
               }
      static void
   lower_ngg_gs_intrinsics(nir_shader *shader, lower_ngg_gs_state *s)
   {
         }
      static void
   ngg_gs_export_primitives(nir_builder *b, nir_def *max_num_out_prims, nir_def *tid_in_tg,
               {
               /* Only bit 0 matters here - set it to 1 when the primitive should be null */
            nir_def *vtx_indices[3] = {0};
   vtx_indices[s->num_vertices_per_primitive - 1] = exporter_tid_in_tg;
   if (s->num_vertices_per_primitive >= 2)
         if (s->num_vertices_per_primitive == 3)
            if (s->num_vertices_per_primitive == 3) {
      /* API GS outputs triangle strips, but NGG HW understands triangles.
   * We already know the triangles due to how we set the primitive flags, but we need to
   * make sure the vertex order is so that the front/back is correct, and the provoking vertex is kept.
            nir_def *is_odd = nir_ubfe_imm(b, primflag_0, 1, 1);
   nir_def *provoking_vertex_index = nir_load_provoking_vtx_in_prim_amd(b);
            vtx_indices[0] = nir_bcsel(b, provoking_vertex_first, vtx_indices[0],
         vtx_indices[1] = nir_bcsel(b, provoking_vertex_first,
               vtx_indices[2] = nir_bcsel(b, provoking_vertex_first,
               nir_def *arg = emit_pack_ngg_prim_exp_arg(b, s->num_vertices_per_primitive, vtx_indices,
         ac_nir_export_primitive(b, arg, NULL);
      }
      static void
   ngg_gs_export_vertices(nir_builder *b, nir_def *max_num_out_vtx, nir_def *tid_in_tg,
         {
      nir_if *if_vtx_export_thread = nir_push_if(b, nir_ilt(b, tid_in_tg, max_num_out_vtx));
            if (!s->output_compile_time_known) {
      /* Vertex compaction.
   * The current thread will export a vertex that was live in another invocation.
   * Load the index of the vertex that the current thread will have to export.
   */
   nir_def *exported_vtx_idx = nir_load_shared(b, 1, 8, out_vtx_lds_addr, .base = s->lds_offs_primflags + 1);
               u_foreach_bit64(slot, b->shader->info.outputs_written) {
      unsigned packed_location =
            gs_output_info *info = &s->output_info[slot];
            while (mask) {
      int start, count;
   u_bit_scan_consecutive_range(&mask, &start, &count);
   nir_def *load =
      nir_load_shared(b, count, 32, exported_out_vtx_lds_addr,
               for (int i = 0; i < count; i++)
                  /* 16bit outputs */
   unsigned num_32bit_outputs = util_bitcount64(b->shader->info.outputs_written);
   u_foreach_bit(i, b->shader->info.outputs_written_16bit) {
      unsigned packed_location = num_32bit_outputs +
            gs_output_info *info_lo = s->output_info_16bit_lo + i;
   gs_output_info *info_hi = s->output_info_16bit_hi + i;
   unsigned mask_lo = gs_output_component_mask_with_stream(info_lo, 0);
   unsigned mask_hi = gs_output_component_mask_with_stream(info_hi, 0);
            while (mask) {
      int start, count;
   u_bit_scan_consecutive_range(&mask, &start, &count);
   nir_def *load =
      nir_load_shared(b, count, 32, exported_out_vtx_lds_addr,
               for (int j = 0; j < count; j++) {
                                    if (mask_hi & BITFIELD_BIT(comp))
                     uint64_t export_outputs = b->shader->info.outputs_written | VARYING_BIT_POS;
   if (s->options->kill_pointsize)
            const bool wait_attr_ring = must_wait_attr_ring(s->options->gfx_level, s->options->has_param_exports);
   if (wait_attr_ring)
            ac_nir_export_position(b, s->options->gfx_level,
                                       if (s->options->has_param_exports) {
               if (s->options->gfx_level >= GFX11) {
      vs_output outputs[64];
   unsigned num_outputs = gather_vs_outputs(b, outputs,
                        if (num_outputs) {
                     export_vertex_params_gfx11(b, tid_in_tg, max_num_out_vtx, num_outputs, outputs,
         } else {
      ac_nir_export_parameters(b, s->options->vs_output_param_offset,
                                    if (wait_attr_ring)
      }
      static void
   ngg_gs_setup_vertex_compaction(nir_builder *b, nir_def *vertex_live, nir_def *tid_in_tg,
         {
      assert(vertex_live->bit_size == 1);
   nir_if *if_vertex_live = nir_push_if(b, vertex_live);
   {
      /* Setup the vertex compaction.
   * Save the current thread's id for the thread which will export the current vertex.
   * We reuse stream 1 of the primitive flag of the other thread's vertex for storing this.
            nir_def *exporter_lds_addr = ngg_gs_out_vertex_addr(b, exporter_tid_in_tg, s);
   nir_def *tid_in_tg_u8 = nir_u2u8(b, tid_in_tg);
      }
      }
      static nir_def *
   ngg_gs_load_out_vtx_primflag(nir_builder *b, unsigned stream, nir_def *tid_in_tg,
               {
               nir_if *if_outvtx_thread = nir_push_if(b, nir_ilt(b, tid_in_tg, max_num_out_vtx));
   nir_def *primflag = nir_load_shared(b, 1, 8, vtx_lds_addr,
         primflag = nir_u2u32(b, primflag);
               }
      static void
   ngg_gs_out_prim_all_vtxptr(nir_builder *b, nir_def *last_vtxidx, nir_def *last_vtxptr,
               {
      unsigned last_vtx = s->num_vertices_per_primitive - 1;
            bool primitive_is_triangle = s->num_vertices_per_primitive == 3;
   nir_def *is_odd = primitive_is_triangle ?
            for (unsigned i = 0; i < s->num_vertices_per_primitive - 1; i++) {
               /* Need to swap vertex 0 and vertex 1 when vertex 2 index is odd to keep
   * CW/CCW order for correct front/back face culling.
   */
   if (primitive_is_triangle)
                  }
      static nir_def *
   ngg_gs_cull_primitive(nir_builder *b, nir_def *tid_in_tg, nir_def *max_vtxcnt,
               {
      /* we haven't enabled point culling, if enabled this function could be further optimized */
            /* save the primflag so that we don't need to load it from LDS again */
   nir_variable *primflag_var = nir_local_variable_create(s->impl, glsl_uint_type(), "primflag");
            /* last bit of primflag indicate if this is the final vertex of a primitive */
   nir_def *is_end_prim_vtx = nir_i2b(b, nir_iand_imm(b, out_vtx_primflag_0, 1));
   nir_def *has_output_vertex = nir_ilt(b, tid_in_tg, max_vtxcnt);
            nir_if *if_prim_enable = nir_push_if(b, prim_enable);
   {
      /* Calculate the LDS address of every vertex in the current primitive. */
   nir_def *vtxptr[3];
            /* Load the positions from LDS. */
   nir_def *pos[3][4];
   for (unsigned i = 0; i < s->num_vertices_per_primitive; i++) {
      /* VARYING_SLOT_POS == 0, so base won't count packed location */
   pos[i][3] = nir_load_shared(b, 1, 32, vtxptr[i], .base = 12); /* W */
   nir_def *xy = nir_load_shared(b, 2, 32, vtxptr[i], .base = 0, .align_mul = 4);
                  pos[i][0] = nir_fdiv(b, pos[i][0], pos[i][3]);
               /* TODO: support clipdist culling in GS */
            nir_def *accepted = ac_nir_cull_primitive(
            nir_if *if_rejected = nir_push_if(b, nir_inot(b, accepted));
   {
      /* clear the primflag if rejected */
                     }
      }
            /* Wait for LDS primflag access done. */
   nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                        /* only dead vertex need a chance to relive */
   nir_def *vtx_is_dead = nir_ieq_imm(b, nir_load_var(b, primflag_var), 0);
   nir_def *vtx_update_primflag = nir_iand(b, vtx_is_dead, has_output_vertex);
   nir_if *if_update_primflag = nir_push_if(b, vtx_update_primflag);
   {
      /* get succeeding vertices' primflag to detect this vertex's liveness */
   for (unsigned i = 1; i < s->num_vertices_per_primitive; i++) {
      nir_def *vtxidx = nir_iadd_imm(b, tid_in_tg, i);
   nir_def *not_overflow = nir_ilt(b, vtxidx, max_vtxcnt);
   nir_if *if_not_overflow = nir_push_if(b, not_overflow);
   {
      nir_def *vtxptr = ngg_gs_out_vertex_addr(b, vtxidx, s);
   nir_def *vtx_primflag =
                  /* if succeeding vertex is alive end of primitive vertex, need to set current
   * thread vertex's liveness flag (bit 2)
   */
   nir_def *has_prim = nir_i2b(b, nir_iand_imm(b, vtx_primflag, 1));
                  /* update this vertex's primflag */
   nir_def *primflag = nir_load_var(b, primflag_var);
   primflag = nir_ior(b, primflag, vtx_live_flag);
      }
         }
               }
      static void
   ngg_gs_build_streamout(nir_builder *b, lower_ngg_gs_state *s)
   {
               nir_def *tid_in_tg = nir_load_local_invocation_index(b);
   nir_def *max_vtxcnt = nir_load_workgroup_num_input_vertices_amd(b);
   nir_def *out_vtx_lds_addr = ngg_gs_out_vertex_addr(b, tid_in_tg, s);
   nir_def *prim_live[4] = {0};
   nir_def *gen_prim[4] = {0};
   nir_def *export_seq[4] = {0};
   nir_def *out_vtx_primflag[4] = {0};
   for (unsigned stream = 0; stream < 4; stream++) {
      if (!(info->streams_written & BITFIELD_BIT(stream)))
            out_vtx_primflag[stream] =
            /* Check bit 0 of primflag for primitive alive, it's set for every last
   * vertex of a primitive.
   */
            unsigned scratch_stride = ALIGN(s->max_num_waves, 4);
   nir_def *scratch_base =
            /* We want to export primitives to streamout buffer in sequence,
   * but not all vertices are alive or mark end of a primitive, so
   * there're "holes". We don't need continuous invocations to write
   * primitives to streamout buffer like final vertex export, so
   * just repack to get the sequence (export_seq) is enough, no need
   * to do compaction.
   *
   * Use separate scratch space for each stream to avoid barrier.
   * TODO: we may further reduce barriers by writing to all stream
   * LDS at once, then we only need one barrier instead of one each
   * stream..
   */
   wg_repack_result rep =
                  /* nir_intrinsic_set_vertex_and_primitive_count can also get primitive count of
   * current wave, but still need LDS to sum all wave's count to get workgroup count.
   * And we need repack to export primitive to streamout buffer anyway, so do here.
   */
   gen_prim[stream] = rep.num_repacked_invocations;
               /* Workgroup barrier: wait for LDS scratch reads finish. */
   nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                        /* Get global buffer offset where this workgroup will stream out data to. */
   nir_def *emit_prim[4] = {0};
   nir_def *buffer_offsets[4] = {0};
   nir_def *so_buffer[4] = {0};
   nir_def *prim_stride[4] = {0};
   ngg_build_streamout_buffer_info(b, info, s->options->has_xfb_prim_query,
                  for (unsigned stream = 0; stream < 4; stream++) {
      if (!(info->streams_written & BITFIELD_BIT(stream)))
            nir_def *can_emit = nir_ilt(b, export_seq[stream], emit_prim[stream]);
   nir_if *if_emit = nir_push_if(b, nir_iand(b, can_emit, prim_live[stream]));
   {
      /* Get streamout buffer vertex index for the first vertex of this primitive. */
                  /* Get all vertices' lds address of this primitive. */
   nir_def *exported_vtx_lds_addr[3];
   ngg_gs_out_prim_all_vtxptr(b, tid_in_tg, out_vtx_lds_addr,
                  /* Write all vertices of this primitive to streamout buffer. */
   for (unsigned i = 0; i < s->num_vertices_per_primitive; i++) {
      ngg_build_streamout_vertex(b, info, stream, so_buffer,
                           }
         }
      static void
   ngg_gs_finale(nir_builder *b, lower_ngg_gs_state *s)
   {
      nir_def *tid_in_tg = nir_load_local_invocation_index(b);
   nir_def *max_vtxcnt = nir_load_workgroup_num_input_vertices_amd(b);
   nir_def *max_prmcnt = max_vtxcnt; /* They are currently practically the same; both RADV and RadeonSI do this. */
            if (s->output_compile_time_known) {
      /* When the output is compile-time known, the GS writes all possible vertices and primitives it can.
   * The gs_alloc_req needs to happen on one wave only, otherwise the HW hangs.
   */
   nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, nir_load_subgroup_id(b), 0));
   alloc_vertices_and_primitives(b, max_vtxcnt, max_prmcnt);
                                 if (s->output_compile_time_known) {
      ngg_gs_export_primitives(b, max_vtxcnt, tid_in_tg, tid_in_tg, out_vtx_primflag_0, s);
   ngg_gs_export_vertices(b, max_vtxcnt, tid_in_tg, out_vtx_lds_addr, s);
               /* cull primitives */
   if (s->options->can_cull) {
               /* culling code will update the primflag */
   nir_def *updated_primflag =
                                       /* When the output vertex count is not known at compile time:
   * There may be gaps between invocations that have live vertices, but NGG hardware
   * requires that the invocations that export vertices are packed (ie. compact).
   * To ensure this, we need to repack invocations that have a live vertex.
   */
   nir_def *vertex_live = nir_ine_imm(b, out_vtx_primflag_0, 0);
   wg_repack_result rep = repack_invocations_in_workgroup(b, vertex_live, s->lds_addr_gs_scratch,
            nir_def *workgroup_num_vertices = rep.num_repacked_invocations;
            /* When the workgroup emits 0 total vertices, we also must export 0 primitives (otherwise the HW can hang). */
   nir_def *any_output = nir_ine_imm(b, workgroup_num_vertices, 0);
            /* Allocate export space. We currently don't compact primitives, just use the maximum number. */
   nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, nir_load_subgroup_id(b), 0));
   {
      if (s->options->gfx_level == GFX10)
         else
      }
            /* Vertex compaction. This makes sure there are no gaps between threads that export vertices. */
            /* Workgroup barrier: wait for all LDS stores to finish. */
   nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            ngg_gs_export_primitives(b, max_prmcnt, tid_in_tg, exporter_tid_in_tg, out_vtx_primflag_0, s);
      }
      void
   ac_nir_lower_ngg_gs(nir_shader *shader, const ac_nir_lower_ngg_options *options)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
            lower_ngg_gs_state state = {
      .options = options,
   .impl = impl,
   .max_num_waves = DIV_ROUND_UP(options->max_workgroup_size, options->wave_size),
   .lds_offs_primflags = options->gs_out_vtx_bytes,
   .lds_bytes_per_gs_out_vertex = options->gs_out_vtx_bytes + 4u,
               if (!options->can_cull) {
      nir_gs_count_vertices_and_primitives(shader, state.const_out_vtxcnt,
         state.output_compile_time_known =
      state.const_out_vtxcnt[0] == shader->info.gs.vertices_out &&
            if (shader->info.gs.output_primitive == MESA_PRIM_POINTS)
         else if (shader->info.gs.output_primitive == MESA_PRIM_LINE_STRIP)
         else if (shader->info.gs.output_primitive == MESA_PRIM_TRIANGLE_STRIP)
         else
            /* Extract the full control flow. It is going to be wrapped in an if statement. */
   nir_cf_list extracted;
   nir_cf_extract(&extracted, nir_before_impl(impl),
            nir_builder builder = nir_builder_at(nir_before_impl(impl));
            /* Workgroup barrier: wait for ES threads */
   nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            state.lds_addr_gs_out_vtx = nir_load_lds_ngg_gs_out_vertex_base_amd(b);
            /* Wrap the GS control flow. */
            nir_cf_reinsert(&extracted, b->cursor);
   b->cursor = nir_after_cf_list(&if_gs_thread->then_list);
            /* Workgroup barrier: wait for all GS threads to finish */
   nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            if (state.streamout_enabled)
            /* Lower the GS intrinsics */
            if (!state.vertex_count[0]) {
      fprintf(stderr, "Could not find set_vertex_and_primitive_count for stream 0. This would hang your GPU.");
               /* Emit shader queries */
   b->cursor = nir_after_cf_list(&if_gs_thread->then_list);
   ac_nir_gs_shader_query(b,
                        state.options->has_gen_prim_query,
   state.options->gfx_level < GFX11,
                  /* Emit the finale sequence */
   ngg_gs_finale(b, &state);
            /* Cleanup */
   nir_lower_vars_to_ssa(shader);
   nir_remove_dead_variables(shader, nir_var_function_temp, NULL);
      }
      unsigned
   ac_ngg_nogs_get_pervertex_lds_size(gl_shader_stage stage,
                                    unsigned shader_num_outputs,
      {
      /* for culling time lds layout only */
   unsigned culling_pervertex_lds_bytes = can_cull ?
      ngg_nogs_get_culling_pervertex_lds_size(
         unsigned pervertex_lds_bytes =
      ngg_nogs_get_pervertex_lds_size(stage, shader_num_outputs, streamout_enabled,
            }
      unsigned
   ac_ngg_get_scratch_lds_size(gl_shader_stage stage,
                           {
      unsigned scratch_lds_size = 0;
            if (stage == MESA_SHADER_VERTEX || stage == MESA_SHADER_TESS_EVAL) {
      if (streamout_enabled) {
      /* 4 dwords for 4 streamout buffer offset, 1 dword for emit prim count */
      } else if (can_cull) {
            } else {
               scratch_lds_size = ALIGN(max_num_waves, 4u);
   /* streamout take 8 dwords for buffer offset and emit vertex per stream */
   if (streamout_enabled)
                  }
      static void
   ms_store_prim_indices(nir_builder *b,
                     {
               if (s->layout.var.prm_attr.mask & BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_INDICES)) {
      for (unsigned c = 0; c < s->vertices_per_prim; ++c)
                     if (!offset_src)
               }
      static void
   ms_store_cull_flag(nir_builder *b,
                     {
      assert(val->num_components == 1);
            if (s->layout.var.prm_attr.mask & BITFIELD64_BIT(VARYING_SLOT_CULL_PRIMITIVE)) {
      nir_store_var(b, s->out_variables[VARYING_SLOT_CULL_PRIMITIVE * 4], nir_b2i32(b, val), 0x1);
               if (!offset_src)
               }
      static nir_def *
   ms_arrayed_output_base_addr(nir_builder *b,
                     {
      /* Address offset of the array item (vertex or primitive). */
   unsigned arr_index_stride = num_arrayed_outputs * 16u;
            /* IO address offset within the vertex or primitive data. */
   unsigned io_offset = driver_location * 16u;
               }
      static void
   update_ms_output_info_slot(lower_ngg_ms_state *s,
               {
      while (components_mask) {
               components_mask >>= 4;
         }
      static void
   update_ms_output_info(nir_intrinsic_instr *intrin,
               {
      nir_io_semantics io_sem = nir_intrinsic_io_semantics(intrin);
   nir_src *base_offset_src = nir_get_io_offset_src(intrin);
   uint32_t write_mask = nir_intrinsic_write_mask(intrin);
            nir_def *store_val = intrin->src[0].ssa;
   write_mask = util_widen_mask(write_mask, DIV_ROUND_UP(store_val->bit_size, 32));
            if (nir_src_is_const(*base_offset_src)) {
      /* Simply mark the components of the current slot as used. */
   unsigned base_off = nir_src_as_uint(*base_offset_src);
      } else {
      /* Indirect offset: mark the components of all slots as used. */
   for (unsigned base_off = 0; base_off < io_sem.num_slots; ++base_off)
         }
      static nir_def *
   regroup_store_val(nir_builder *b, nir_def *store_val)
   {
      /* Vulkan spec 15.1.4-15.1.5:
   *
   * The shader interface consists of output slots with 4x 32-bit components.
   * Small bitsize components consume the same space as 32-bit components,
   * but 64-bit ones consume twice as much.
   *
   * The same output slot may consist of components of different bit sizes.
   * Therefore for simplicity we don't store small bitsize components
   * contiguously, but pad them instead. In practice, they are converted to
   * 32-bit and then stored contiguously.
            if (store_val->bit_size < 32) {
      assert(store_val->num_components <= 4);
   nir_def *comps[4] = {0};
   for (unsigned c = 0; c < store_val->num_components; ++c)
                        }
      static nir_def *
   regroup_load_val(nir_builder *b, nir_def *load, unsigned dest_bit_size)
   {
      if (dest_bit_size == load->bit_size)
            /* Small bitsize components are not stored contiguously, take care of that here. */
   unsigned num_components = load->num_components;
   assert(num_components <= 4);
   nir_def *components[4] = {0};
   for (unsigned i = 0; i < num_components; ++i)
               }
      static const ms_out_part *
   ms_get_out_layout_part(unsigned location,
                     {
               if (info->per_primitive_outputs & mask) {
      if (mask & s->layout.lds.prm_attr.mask) {
      *out_mode = ms_out_mode_lds;
      } else if (mask & s->layout.scratch_ring.prm_attr.mask) {
      *out_mode = ms_out_mode_scratch_ring;
      } else if (mask & s->layout.attr_ring.prm_attr.mask) {
      *out_mode = ms_out_mode_attr_ring;
      } else if (mask & s->layout.var.prm_attr.mask) {
      *out_mode = ms_out_mode_var;
         } else {
      if (mask & s->layout.lds.vtx_attr.mask) {
      *out_mode = ms_out_mode_lds;
      } else if (mask & s->layout.scratch_ring.vtx_attr.mask) {
      *out_mode = ms_out_mode_scratch_ring;
      } else if (mask & s->layout.attr_ring.vtx_attr.mask) {
      *out_mode = ms_out_mode_attr_ring;
      } else if (mask & s->layout.var.vtx_attr.mask) {
      *out_mode = ms_out_mode_var;
                     }
      static void
   ms_store_arrayed_output_intrin(nir_builder *b,
               {
               if (location == VARYING_SLOT_PRIMITIVE_INDICES) {
      /* EXT_mesh_shader primitive indices: array of vectors.
   * They don't count as per-primitive outputs, but the array is indexed
   * by the primitive index, so they are practically per-primitive.
   *
   * The max vertex count is 256, so these indices always fit 8 bits.
   * To reduce LDS use, store these as a flat array of 8-bit values.
   */
   assert(nir_src_is_const(*nir_get_io_offset_src(intrin)));
   assert(nir_src_as_uint(*nir_get_io_offset_src(intrin)) == 0);
            nir_def *store_val = intrin->src[0].ssa;
   nir_def *arr_index = nir_get_io_arrayed_index_src(intrin)->ssa;
   nir_def *offset = nir_imul_imm(b, arr_index, s->vertices_per_prim);
   ms_store_prim_indices(b, store_val, offset, s);
      } else if (location == VARYING_SLOT_CULL_PRIMITIVE) {
      /* EXT_mesh_shader cull primitive: per-primitive bool.
   * To reduce LDS use, store these as an array of 8-bit values.
   */
   assert(nir_src_is_const(*nir_get_io_offset_src(intrin)));
   assert(nir_src_as_uint(*nir_get_io_offset_src(intrin)) == 0);
   assert(nir_intrinsic_component(intrin) == 0);
            nir_def *store_val = intrin->src[0].ssa;
   nir_def *arr_index = nir_get_io_arrayed_index_src(intrin)->ssa;
   nir_def *offset = nir_imul_imm(b, arr_index, s->vertices_per_prim);
   ms_store_cull_flag(b, store_val, offset, s);
               ms_out_mode out_mode;
   const ms_out_part *out = ms_get_out_layout_part(location, &b->shader->info, &out_mode, s);
            /* We compact the LDS size (we don't reserve LDS space for outputs which can
   * be stored in variables), so we can't rely on the original driver_location.
   * Instead, we compute the first free location based on the output mask.
   */
   unsigned driver_location = util_bitcount64(out->mask & u_bit_consecutive64(0, location));
   unsigned component_offset = nir_intrinsic_component(intrin);
   unsigned write_mask = nir_intrinsic_write_mask(intrin);
   unsigned num_outputs = util_bitcount64(out->mask);
            nir_def *store_val = regroup_store_val(b, intrin->src[0].ssa);
   nir_def *arr_index = nir_get_io_arrayed_index_src(intrin)->ssa;
   nir_def *base_addr = ms_arrayed_output_base_addr(b, arr_index, driver_location, num_outputs);
   nir_def *base_offset = nir_get_io_offset_src(intrin)->ssa;
   nir_def *base_addr_off = nir_imul_imm(b, base_offset, 16u);
            if (out_mode == ms_out_mode_lds) {
      nir_store_shared(b, store_val, addr, .base = const_off,
            } else if (out_mode == ms_out_mode_scratch_ring) {
      nir_def *ring = nir_load_ring_mesh_scratch_amd(b);
   nir_def *off = nir_load_ring_mesh_scratch_offset_amd(b);
   nir_def *zero = nir_imm_int(b, 0);
   nir_store_buffer_amd(b, store_val, ring, addr, off, zero,
                        } else if (out_mode == ms_out_mode_attr_ring) {
      /* GFX11+: Store params straight to the attribute ring.
   *
   * Even though the access pattern may not be the most optimal,
   * this is still much better than reserving LDS and losing waves.
   * (Also much better than storing and reloading from the scratch ring.)
   */
   const nir_io_semantics io_sem = nir_intrinsic_io_semantics(intrin);
   unsigned param_offset = s->vs_output_param_offset[io_sem.location];
   nir_def *ring = nir_load_ring_attr_amd(b);
   nir_def *soffset = nir_load_ring_attr_offset_amd(b);
   nir_store_buffer_amd(b, store_val, ring, base_addr_off, soffset, arr_index,
                        } else if (out_mode == ms_out_mode_var) {
      if (store_val->bit_size > 32) {
      /* Split 64-bit store values to 32-bit components. */
   store_val = nir_bitcast_vector(b, store_val, 32);
   /* Widen the write mask so it is in 32-bit components. */
               u_foreach_bit(comp, write_mask) {
      nir_def *val = nir_channel(b, store_val, comp);
   unsigned idx = location * 4 + comp + component_offset;
         } else {
            }
      static nir_def *
   ms_load_arrayed_output(nir_builder *b,
                        nir_def *arr_index,
   nir_def *base_offset,
   unsigned location,
      {
      ms_out_mode out_mode;
            unsigned component_addr_off = component_offset * 4;
   unsigned num_outputs = util_bitcount64(out->mask);
            /* Use compacted driver location instead of the original. */
            nir_def *base_addr = ms_arrayed_output_base_addr(b, arr_index, driver_location, num_outputs);
   nir_def *base_addr_off = nir_imul_imm(b, base_offset, 16);
            if (out_mode == ms_out_mode_lds) {
      return nir_load_shared(b, num_components, load_bit_size, addr, .align_mul = 16,
            } else if (out_mode == ms_out_mode_scratch_ring) {
      nir_def *ring = nir_load_ring_mesh_scratch_amd(b);
   nir_def *off = nir_load_ring_mesh_scratch_offset_amd(b);
   nir_def *zero = nir_imm_int(b, 0);
   return nir_load_buffer_amd(b, num_components, load_bit_size, ring, addr, off, zero,
                  } else if (out_mode == ms_out_mode_var) {
      nir_def *arr[8] = {0};
   unsigned num_32bit_components = num_components * load_bit_size / 32;
   for (unsigned comp = 0; comp < num_32bit_components; ++comp) {
      unsigned idx = location * 4 + comp + component_addr_off;
      }
   if (load_bit_size > 32)
            } else {
            }
      static nir_def *
   ms_load_arrayed_output_intrin(nir_builder *b,
               {
      nir_def *arr_index = nir_get_io_arrayed_index_src(intrin)->ssa;
            unsigned location = nir_intrinsic_io_semantics(intrin).location;
   unsigned component_offset = nir_intrinsic_component(intrin);
   unsigned bit_size = intrin->def.bit_size;
   unsigned num_components = intrin->def.num_components;
            nir_def *load =
      ms_load_arrayed_output(b, arr_index, base_offset, location, component_offset,
            }
      static nir_def *
   lower_ms_load_workgroup_index(nir_builder *b,
               {
         }
      static nir_def *
   lower_ms_set_vertex_and_primitive_count(nir_builder *b,
               {
      /* If either the number of vertices or primitives is zero, set both of them to zero. */
   nir_def *num_vtx = nir_read_first_invocation(b, intrin->src[0].ssa);
   nir_def *num_prm = nir_read_first_invocation(b, intrin->src[1].ssa);
   nir_def *zero = nir_imm_int(b, 0);
   nir_def *is_either_zero = nir_ieq(b, nir_umin(b, num_vtx, num_prm), zero);
   num_vtx = nir_bcsel(b, is_either_zero, zero, num_vtx);
            nir_store_var(b, s->vertex_count_var, num_vtx, 0x1);
               }
      static nir_def *
   update_ms_barrier(nir_builder *b,
               {
      /* Output loads and stores are lowered to shared memory access,
   * so we have to update the barriers to also reflect this.
   */
   unsigned mem_modes = nir_intrinsic_memory_modes(intrin);
   if (mem_modes & nir_var_shader_out)
         else
                        }
      static nir_def *
   lower_ms_intrinsic(nir_builder *b, nir_instr *instr, void *state)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     switch (intrin->intrinsic) {
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_primitive_output:
      ms_store_arrayed_output_intrin(b, intrin, s);
      case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_primitive_output:
         case nir_intrinsic_barrier:
         case nir_intrinsic_load_workgroup_index:
         case nir_intrinsic_set_vertex_and_primitive_count:
         default:
            }
      static bool
   filter_ms_intrinsic(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   return intrin->intrinsic == nir_intrinsic_store_output ||
         intrin->intrinsic == nir_intrinsic_load_output ||
   intrin->intrinsic == nir_intrinsic_store_per_vertex_output ||
   intrin->intrinsic == nir_intrinsic_load_per_vertex_output ||
   intrin->intrinsic == nir_intrinsic_store_per_primitive_output ||
   intrin->intrinsic == nir_intrinsic_load_per_primitive_output ||
   intrin->intrinsic == nir_intrinsic_barrier ||
      }
      static void
   lower_ms_intrinsics(nir_shader *shader, lower_ngg_ms_state *s)
   {
         }
      static void
   ms_emit_arrayed_outputs(nir_builder *b,
                     {
               u_foreach_bit64(slot, mask) {
      /* Should not occur here, handled separately. */
                     while (component_mask) {
                     nir_def *load =
                  for (int i = 0; i < num_components; i++)
            }
      static void
   ms_create_same_invocation_vars(nir_builder *b, lower_ngg_ms_state *s)
   {
      /* Initialize NIR variables for same-invocation outputs. */
            u_foreach_bit64(slot, same_invocation_output_mask) {
      for (unsigned comp = 0; comp < 4; ++comp) {
      unsigned idx = slot * 4 + comp;
            }
      static void
   ms_emit_legacy_workgroup_index(nir_builder *b, lower_ngg_ms_state *s)
   {
      /* Workgroup ID should have been lowered to workgroup index. */
            /* No need to do anything if the shader doesn't use the workgroup index. */
   if (!BITSET_TEST(b->shader->info.system_values_read, SYSTEM_VALUE_WORKGROUP_INDEX))
                     /* Legacy fast launch mode (FAST_LAUNCH=1):
   *
   * The HW doesn't support a proper workgroup index for vertex processing stages,
   * so we use the vertex ID which is equivalent to the index of the current workgroup
   * within the current dispatch.
   *
   * Due to the register programming of mesh shaders, this value is only filled for
   * the first invocation of the first wave. To let other waves know, we use LDS.
   */
            if (s->api_workgroup_size <= s->wave_size) {
      /* API workgroup is small, so we don't need to use LDS. */
   s->workgroup_index = nir_read_first_invocation(b, workgroup_index);
                        nir_def *zero = nir_imm_int(b, 0);
   nir_def *dont_care = nir_undef(b, 1, 32);
            /* Use elect to make sure only 1 invocation uses LDS. */
   nir_if *if_elected = nir_push_if(b, nir_elect(b, 1));
   {
      nir_def *wave_id = nir_load_subgroup_id(b);
   nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, wave_id, 0));
   {
      nir_store_shared(b, workgroup_index, zero, .base = workgroup_index_lds_addr);
   nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                  }
   nir_push_else(b, if_wave_0);
   {
      nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                        }
               }
            workgroup_index = nir_if_phi(b, workgroup_index, dont_care);
      }
      static void
   set_ms_final_output_counts(nir_builder *b,
                     {
      /* The spec allows the numbers to be divergent, and in that case we need to
   * use the values from the first invocation. Also the HW requires us to set
   * both to 0 if either was 0.
   *
   * These are already done by the lowering.
   */
   nir_def *num_prm = nir_load_var(b, s->primitive_count_var);
            if (s->hw_workgroup_size <= s->wave_size) {
      /* Single-wave mesh shader workgroup. */
   alloc_vertices_and_primitives(b, num_vtx, num_prm);
   *out_num_prm = num_prm;
   *out_num_vtx = num_vtx;
               /* Multi-wave mesh shader workgroup:
   * We need to use LDS to distribute the correct values to the other waves.
   *
   * TODO:
   * If we can prove that the values are workgroup-uniform, we can skip this
   * and just use whatever the current wave has. However, NIR divergence analysis
   * currently doesn't support this.
                     nir_if *if_wave_0 = nir_push_if(b, nir_ieq_imm(b, nir_load_subgroup_id(b), 0));
   {
      nir_if *if_elected = nir_push_if(b, nir_elect(b, 1));
   {
      nir_store_shared(b, nir_vec2(b, num_prm, num_vtx), zero,
      }
            nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                           }
   nir_push_else(b, if_wave_0);
   {
      nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                        nir_def *prm_vtx = NULL;
   nir_def *dont_care_2x32 = nir_undef(b, 2, 32);
   nir_if *if_elected = nir_push_if(b, nir_elect(b, 1));
   {
      prm_vtx = nir_load_shared(b, 2, 32, zero,
      }
            prm_vtx = nir_if_phi(b, prm_vtx, dont_care_2x32);
   num_prm = nir_read_first_invocation(b, nir_channel(b, prm_vtx, 0));
            nir_store_var(b, s->primitive_count_var, num_prm, 0x1);
      }
            *out_num_prm = nir_load_var(b, s->primitive_count_var);
      }
      static void
   ms_emit_attribute_ring_output_stores(nir_builder *b, const uint64_t outputs_mask,
         {
      if (!outputs_mask)
            nir_def *ring = nir_load_ring_attr_amd(b);
   nir_def *off = nir_load_ring_attr_offset_amd(b);
            u_foreach_bit64 (slot, outputs_mask) {
      if (s->vs_output_param_offset[slot] > AC_EXP_PARAM_OFFSET_31)
            nir_def *soffset = nir_iadd_imm(b, off, s->vs_output_param_offset[slot] * 16 * 32);
   nir_def *store_val = nir_undef(b, 4, 32);
   unsigned store_val_components = 0;
   for (unsigned c = 0; c < 4; ++c) {
      if (s->outputs[slot][c]) {
      store_val = nir_vector_insert_imm(b, store_val, s->outputs[slot][c], c);
                  store_val = nir_trim_vector(b, store_val, store_val_components);
   nir_store_buffer_amd(b, store_val, ring, zero, soffset, idx,
               }
      static nir_def *
   ms_prim_exp_arg_ch1(nir_builder *b, nir_def *invocation_index, nir_def *num_vtx, lower_ngg_ms_state *s)
   {
      /* Primitive connectivity data: describes which vertices the primitive uses. */
   nir_def *prim_idx_addr = nir_imul_imm(b, invocation_index, s->vertices_per_prim);
   nir_def *indices_loaded = NULL;
            if (s->layout.var.prm_attr.mask & BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_INDICES)) {
      nir_def *indices[3] = {0};
   for (unsigned c = 0; c < s->vertices_per_prim; ++c)
            } else {
      indices_loaded = nir_load_shared(b, s->vertices_per_prim, 8, prim_idx_addr, .base = s->layout.lds.indices_addr);
               if (s->uses_cull_flags) {
      nir_def *loaded_cull_flag = NULL;
   if (s->layout.var.prm_attr.mask & BITFIELD64_BIT(VARYING_SLOT_CULL_PRIMITIVE))
         else
                        nir_def *indices[3];
            for (unsigned i = 0; i < s->vertices_per_prim; ++i) {
      indices[i] = nir_channel(b, indices_loaded, i);
                  }
      static nir_def *
   ms_prim_exp_arg_ch2(nir_builder *b, uint64_t outputs_mask, lower_ngg_ms_state *s)
   {
               if (outputs_mask) {
      /* When layer, viewport etc. are per-primitive, they need to be encoded in
   * the primitive export instruction's second channel. The encoding is:
   *
   * --- GFX10.3 ---
   * bits 31..30: VRS rate Y
   * bits 29..28: VRS rate X
   * bits 23..20: viewport
   * bits 19..17: layer
   *
   * --- GFX11 ---
   * bits 31..28: VRS rate enum
   * bits 23..20: viewport
   * bits 12..00: layer
   */
            if (outputs_mask & VARYING_BIT_LAYER) {
      nir_def *layer =
                     if (outputs_mask & VARYING_BIT_VIEWPORT) {
      nir_def *view = nir_ishl_imm(b, s->outputs[VARYING_SLOT_VIEWPORT][0], 20);
               if (outputs_mask & VARYING_BIT_PRIMITIVE_SHADING_RATE) {
      nir_def *rate = s->outputs[VARYING_SLOT_PRIMITIVE_SHADING_RATE][0];
                     }
      static void
   ms_prim_gen_query(nir_builder *b,
                     {
      if (!s->has_query)
            nir_if *if_invocation_index_zero = nir_push_if(b, nir_ieq_imm(b, invocation_index, 0));
   {
      nir_if *if_shader_query = nir_push_if(b, nir_load_prim_gen_query_enabled_amd(b));
   {
         }
      }
      }
      static void
   ms_invocation_query(nir_builder *b,
               {
      if (!s->has_query)
            nir_if *if_invocation_index_zero = nir_push_if(b, nir_ieq_imm(b, invocation_index, 0));
   {
      nir_if *if_pipeline_query = nir_push_if(b, nir_load_pipeline_stat_query_enabled_amd(b));
   {
         }
      }
      }
      static void
   emit_ms_vertex(nir_builder *b, nir_def *index, nir_def *row, bool exports, bool parameters,
         {
               if (exports) {
      ac_nir_export_position(b, s->gfx_level, s->clipdist_enable_mask,
                     if (parameters) {
      /* Export generic attributes on GFX10.3
   * (On GFX11 they are already stored in the attribute ring.)
   */
   if (s->has_param_exports && s->gfx_level == GFX10_3) {
      ac_nir_export_parameters(b, s->vs_output_param_offset, per_vertex_outputs, 0, s->outputs,
               /* GFX11+: also store special outputs to the attribute ring so PS can load them. */
   if (s->gfx_level >= GFX11 && (per_vertex_outputs & MS_VERT_ARG_EXP_MASK))
         }
      static void
   emit_ms_primitive(nir_builder *b, nir_def *index, nir_def *row, bool exports, bool parameters,
         {
               /* Insert layer output store if the pipeline uses multiview but the API shader doesn't write it. */
   if (s->insert_layer_output)
            if (exports) {
      const uint64_t outputs_mask = per_primitive_outputs & MS_PRIM_ARG_EXP_MASK;
   nir_def *num_vtx = nir_load_var(b, s->vertex_count_var);
   nir_def *prim_exp_arg_ch1 = ms_prim_exp_arg_ch1(b, index, num_vtx, s);
            nir_def *prim_exp_arg = prim_exp_arg_ch2 ?
                        if (parameters) {
      /* Export generic attributes on GFX10.3
   * (On GFX11 they are already stored in the attribute ring.)
   */
   if (s->has_param_exports && s->gfx_level == GFX10_3) {
      ac_nir_export_parameters(b, s->vs_output_param_offset, per_primitive_outputs, 0,
               /* GFX11+: also store special outputs to the attribute ring so PS can load them. */
   if (s->gfx_level >= GFX11)
         }
      static void
   emit_ms_outputs(nir_builder *b, nir_def *invocation_index, nir_def *row_start,
                  nir_def *count, bool exports, bool parameters, uint64_t mask,
      {
      if (cb == &emit_ms_primitive ? s->prim_multirow_export : s->vert_multirow_export) {
      assert(s->hw_workgroup_size % s->wave_size == 0);
            nir_loop *row_loop = nir_push_loop(b);
   {
               nir_phi_instr *index = nir_phi_instr_create(b->shader);
   nir_phi_instr *row = nir_phi_instr_create(b->shader);
                                 nir_if *if_break = nir_push_if(b, nir_uge(b, &index->def, count));
   {
                                 nir_block *body = nir_cursor_current_block(b->cursor);
   nir_phi_instr_add_src(index, body,
                        nir_instr_insert_before_cf_list(&row_loop->body, &row->instr);
      }
      } else {
      nir_def *has_output = nir_ilt(b, invocation_index, count);
   nir_if *if_has_output = nir_push_if(b, has_output);
   {
         }
         }
      static void
   emit_ms_finale(nir_builder *b, lower_ngg_ms_state *s)
   {
      /* We assume there is always a single end block in the shader. */
   nir_block *last_block = nir_impl_last_block(b->impl);
            nir_barrier(b, .execution_scope=SCOPE_WORKGROUP, .memory_scope=SCOPE_WORKGROUP,
            nir_def *num_prm;
                                       nir_def *row_start = NULL;
   if (s->fast_launch_2)
            /* Load vertex/primitive attributes from shared memory and
   * emit store_output intrinsics for them.
   *
   * Contrary to the semantics of the API mesh shader, these are now
   * compliant with NGG HW semantics, meaning that these store the
   * current thread's vertex attributes in a way the HW can export.
            uint64_t per_vertex_outputs =
         uint64_t per_primitive_outputs =
            /* Insert layer output store if the pipeline uses multiview but the API shader doesn't write it. */
   if (s->insert_layer_output) {
      b->shader->info.outputs_written |= VARYING_BIT_LAYER;
   b->shader->info.per_primitive_outputs |= VARYING_BIT_LAYER;
               const bool has_special_param_exports =
      (per_vertex_outputs & MS_VERT_ARG_EXP_MASK) ||
                  /* Export vertices. */
   if ((per_vertex_outputs & ~VARYING_BIT_POS) || !wait_attr_ring) {
      emit_ms_outputs(b, invocation_index, row_start, num_vtx, !wait_attr_ring, true,
               /* Export primitives. */
   if (per_primitive_outputs || !wait_attr_ring) {
      emit_ms_outputs(b, invocation_index, row_start, num_prm, !wait_attr_ring, true,
               /* When we need to wait for attribute ring stores, we emit both position and primitive
   * export instructions after a barrier to make sure both per-vertex and per-primitive
   * attribute ring stores are finished before the GPU starts rasterization.
   */
   if (wait_attr_ring) {
      /* Wait for attribute stores to finish. */
   nir_barrier(b, .execution_scope = SCOPE_SUBGROUP,
                        /* Position/primitive export only */
   emit_ms_outputs(b, invocation_index, row_start, num_vtx, true, false,
         emit_ms_outputs(b, invocation_index, row_start, num_prm, true, false,
         }
      static void
   handle_smaller_ms_api_workgroup(nir_builder *b,
         {
      if (s->api_workgroup_size >= s->hw_workgroup_size)
            /* Handle barriers manually when the API workgroup
   * size is less than the HW workgroup size.
   *
   * The problem is that the real workgroup launched on NGG HW
   * will be larger than the size specified by the API, and the
   * extra waves need to keep up with barriers in the API waves.
   *
   * There are 2 different cases:
   * 1. The whole API workgroup fits in a single wave.
   *    We can shrink the barriers to subgroup scope and
   *    don't need to insert any extra ones.
   * 2. The API workgroup occupies multiple waves, but not
   *    all. In this case, we emit code that consumes every
   *    barrier on the extra waves.
   */
   assert(s->hw_workgroup_size % s->wave_size == 0);
   bool scan_barriers = ALIGN(s->api_workgroup_size, s->wave_size) < s->hw_workgroup_size;
   bool can_shrink_barriers = s->api_workgroup_size <= s->wave_size;
            unsigned api_waves_in_flight_addr = s->layout.lds.workgroup_info_addr + lds_ms_num_api_waves;
            /* Scan the shader for workgroup barriers. */
   if (scan_barriers) {
               nir_foreach_block(block, b->impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   bool is_workgroup_barrier =
                                 if (can_shrink_barriers) {
      /* Every API invocation runs in the first wave.
   * In this case, we can change the barriers to subgroup scope
   * and avoid adding additional barriers.
   */
   nir_intrinsic_set_memory_scope(intrin, SCOPE_SUBGROUP);
      } else {
                                    /* Extract the full control flow of the shader. */
   nir_cf_list extracted;
   nir_cf_extract(&extracted, nir_before_impl(b->impl),
                  /* Wrap the shader in an if to ensure that only the necessary amount of lanes run it. */
   nir_def *invocation_index = nir_load_local_invocation_index(b);
            if (need_additional_barriers) {
      /* First invocation stores 0 to number of API waves in flight. */
   nir_if *if_first_in_workgroup = nir_push_if(b, nir_ieq_imm(b, invocation_index, 0));
   {
         }
            nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                           nir_def *has_api_ms_invocation = nir_ult_imm(b, invocation_index, s->api_workgroup_size);
   nir_if *if_has_api_ms_invocation = nir_push_if(b, has_api_ms_invocation);
   {
      nir_cf_reinsert(&extracted, b->cursor);
            if (need_additional_barriers) {
      /* One invocation in each API wave decrements the number of API waves in flight. */
   nir_if *if_elected_again = nir_push_if(b, nir_elect(b, 1));
   {
      nir_shared_atomic(b, 32, zero, nir_imm_int(b, -1u),
                           nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                              }
            if (need_additional_barriers) {
      /* Make sure that waves that don't run any API invocations execute
   * the same amount of barriers as those that do.
   *
   * We do this by executing a barrier until the number of API waves
   * in flight becomes zero.
   */
   nir_def *has_api_ms_ballot = nir_ballot(b, 1, s->wave_size, has_api_ms_invocation);
   nir_def *wave_has_no_api_ms = nir_ieq_imm(b, has_api_ms_ballot, 0);
   nir_if *if_wave_has_no_api_ms = nir_push_if(b, wave_has_no_api_ms);
   {
      nir_if *if_elected = nir_push_if(b, nir_elect(b, 1));
   {
      nir_loop *loop = nir_push_loop(b);
   {
      nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                        nir_def *loaded = nir_load_shared(b, 1, 32, zero, .base = api_waves_in_flight_addr);
   nir_if *if_break = nir_push_if(b, nir_ieq_imm(b, loaded, 0));
   {
         }
      }
      }
      }
         }
      static void
   ms_move_output(ms_out_part *from, ms_out_part *to)
   {
      uint64_t loc = util_logbase2_64(from->mask);
   uint64_t bit = BITFIELD64_BIT(loc);
   from->mask ^= bit;
      }
      static void
   ms_calculate_arrayed_output_layout(ms_out_mem_layout *l,
               {
      uint32_t lds_vtx_attr_size = util_bitcount64(l->lds.vtx_attr.mask) * max_vertices * 16;
   uint32_t lds_prm_attr_size = util_bitcount64(l->lds.prm_attr.mask) * max_primitives * 16;
   l->lds.prm_attr.addr = ALIGN(l->lds.vtx_attr.addr + lds_vtx_attr_size, 16);
            uint32_t scratch_ring_vtx_attr_size =
         l->scratch_ring.prm_attr.addr =
      }
      static ms_out_mem_layout
   ms_calculate_output_layout(enum amd_gfx_level gfx_level, unsigned api_shared_size,
                     {
      /* These outputs always need export instructions and can't use the attributes ring. */
   const uint64_t always_export_mask =
      VARYING_BIT_POS | VARYING_BIT_CULL_DIST0 | VARYING_BIT_CULL_DIST1 | VARYING_BIT_CLIP_DIST0 |
   VARYING_BIT_CLIP_DIST1 | VARYING_BIT_PSIZ | VARYING_BIT_VIEWPORT |
   VARYING_BIT_PRIMITIVE_SHADING_RATE | VARYING_BIT_LAYER |
   BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_COUNT) |
         const bool use_attr_ring = gfx_level >= GFX11;
   const uint64_t attr_ring_per_vertex_output_mask =
         const uint64_t attr_ring_per_primitive_output_mask =
            const uint64_t lds_per_vertex_output_mask =
      per_vertex_output_mask & ~attr_ring_per_vertex_output_mask & cross_invocation_output_access &
      const uint64_t lds_per_primitive_output_mask =
      per_primitive_output_mask & ~attr_ring_per_primitive_output_mask &
         const bool cross_invocation_indices =
         const bool cross_invocation_cull_primitive =
            /* Shared memory used by the API shader. */
            /* GFX11+: use attribute ring for all generic attributes. */
   l.attr_ring.vtx_attr.mask = attr_ring_per_vertex_output_mask;
            /* Outputs without cross-invocation access can be stored in variables. */
   l.var.vtx_attr.mask =
         l.var.prm_attr.mask = per_primitive_output_mask & ~attr_ring_per_primitive_output_mask &
            /* Workgroup information, see ms_workgroup_* for the layout. */
   l.lds.workgroup_info_addr = ALIGN(l.lds.total_size, 16);
            /* Per-vertex and per-primitive output attributes.
   * Outputs without cross-invocation access are not included here.
   * First, try to put all outputs into LDS (shared memory).
   * If they don't fit, try to move them to VRAM one by one.
   */
   l.lds.vtx_attr.addr = ALIGN(l.lds.total_size, 16);
   l.lds.vtx_attr.mask = lds_per_vertex_output_mask;
   l.lds.prm_attr.mask = lds_per_primitive_output_mask;
            /* NGG shaders can only address up to 32K LDS memory.
   * The spec requires us to allow the application to use at least up to 28K
   * shared memory. Additionally, we reserve 2K for driver internal use
   * (eg. primitive indices and such, see below).
   *
   * Move the outputs that do not fit LDS, to VRAM.
   * Start with per-primitive attributes, because those are grouped at the end.
   */
   const unsigned usable_lds_kbytes =
         while (l.lds.total_size >= usable_lds_kbytes * 1024) {
      if (l.lds.prm_attr.mask)
         else if (l.lds.vtx_attr.mask)
         else
                        if (cross_invocation_indices) {
      /* Indices: flat array of 8-bit vertex indices for each primitive. */
   l.lds.indices_addr = ALIGN(l.lds.total_size, 16);
               if (cross_invocation_cull_primitive) {
      /* Cull flags: array of 8-bit cull flags for each primitive, 1=cull, 0=keep. */
   l.lds.cull_flags_addr = ALIGN(l.lds.total_size, 16);
               /* NGG is only allowed to address up to 32K of LDS. */
   assert(l.lds.total_size <= 32 * 1024);
      }
      void
   ac_nir_lower_ngg_ms(nir_shader *shader,
                     enum amd_gfx_level gfx_level,
   uint32_t clipdist_enable_mask,
   const uint8_t *vs_output_param_offset,
   bool has_param_exports,
   bool *out_needs_scratch_ring,
   unsigned wave_size,
   unsigned hw_workgroup_size,
   {
      unsigned vertices_per_prim =
            uint64_t per_vertex_outputs =
         uint64_t per_primitive_outputs =
            /* Whether the shader uses CullPrimitiveEXT */
   bool uses_cull = shader->info.outputs_written & BITFIELD64_BIT(VARYING_SLOT_CULL_PRIMITIVE);
   /* Can't handle indirect register addressing, pretend as if they were cross-invocation. */
   uint64_t cross_invocation_access = shader->info.mesh.ms_cross_invocation_output_access |
            unsigned max_vertices = shader->info.mesh.max_vertices_out;
            ms_out_mem_layout layout = ms_calculate_output_layout(
      gfx_level, shader->info.shared_size, per_vertex_outputs, per_primitive_outputs,
         shader->info.shared_size = layout.lds.total_size;
            /* The workgroup size that is specified by the API shader may be different
   * from the size of the workgroup that actually runs on the HW, due to the
   * limitations of NGG: max 0/1 vertex and 0/1 primitive per lane is allowed.
   *
   * Therefore, we must make sure that when the API workgroup size is smaller,
   * we don't run the API shader on more HW invocations than is necessary.
   */
   unsigned api_workgroup_size = shader->info.workgroup_size[0] *
                  lower_ngg_ms_state state = {
      .layout = layout,
   .wave_size = wave_size,
   .per_vertex_outputs = per_vertex_outputs,
   .per_primitive_outputs = per_primitive_outputs,
   .vertices_per_prim = vertices_per_prim,
   .api_workgroup_size = api_workgroup_size,
   .hw_workgroup_size = hw_workgroup_size,
   .insert_layer_output = multiview && !(shader->info.outputs_written & VARYING_BIT_LAYER),
   .uses_cull_flags = uses_cull,
   .gfx_level = gfx_level,
   .fast_launch_2 = fast_launch_2,
   .vert_multirow_export = fast_launch_2 && max_vertices > hw_workgroup_size,
   .prim_multirow_export = fast_launch_2 && max_primitives > hw_workgroup_size,
   .clipdist_enable_mask = clipdist_enable_mask,
   .vs_output_param_offset = vs_output_param_offset,
   .has_param_exports = has_param_exports,
               nir_function_impl *impl = nir_shader_get_entrypoint(shader);
            state.vertex_count_var =
         state.primitive_count_var =
            nir_builder builder = nir_builder_at(nir_before_impl(impl));
            handle_smaller_ms_api_workgroup(b, &state);
   if (!fast_launch_2)
         ms_create_same_invocation_vars(b, &state);
                     emit_ms_finale(b, &state);
            /* Cleanup */
   nir_lower_vars_to_ssa(shader);
   nir_remove_dead_variables(shader, nir_var_function_temp, NULL);
   nir_lower_alu_to_scalar(shader, NULL, NULL);
            /* Optimize load_local_invocation_index. When the API workgroup is smaller than the HW workgroup,
   * local_invocation_id isn't initialized for all lanes and we can't perform this optimization for
   * all load_local_invocation_index.
   */
   if (fast_launch_2 && api_workgroup_size == hw_workgroup_size &&
      ((shader->info.workgroup_size[0] == 1) + (shader->info.workgroup_size[1] == 1) +
   (shader->info.workgroup_size[2] == 1)) == 2) {
   nir_lower_compute_system_values_options csv_options = {
         };
                  }
