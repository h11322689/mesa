   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "compiler/shader_enums.h"
   #include "compiler/spirv/nir_spirv.h"
   #include "nir/nir.h"
   #include "rogue.h"
   #include "rogue_builder.h"
   #include "util/macros.h"
   /* FIXME: Remove once the compiler/driver interface is finalised. */
   #include "vulkan/vulkan_core.h"
      /**
   * \file rogue_compile.c
   *
   * \brief Contains NIR to Rogue translation functions, and Rogue passes.
   */
      /* For ALU scalars */
   /* TODO: expand and use these helpers. */
   static rogue_ref nir_ssa_reg(rogue_shader *shader,
                     {
      if (num_components > 1) {
      return rogue_ref_regarray(
                  }
      static rogue_ref nir_ssa_regarray(rogue_shader *shader,
                     {
      return rogue_ref_regarray(
      }
      static rogue_ref nir_ssa_reg_alu_src(rogue_shader *shader,
                     {
      unsigned index = alu->src[src_num].src.ssa->index;
   unsigned num_components = alu->src[src_num].src.ssa->num_components;
                     return vec ? nir_ssa_regarray(shader, index, num_components, component)
      }
      static rogue_ref
   nir_ssa_reg_alu_dst(rogue_shader *shader, const nir_alu_instr *alu, bool vec)
   {
      unsigned num_components = alu->def.num_components;
                     return vec ? nir_ssa_regarray(shader, index, num_components, 0)
      }
      static void trans_nir_jump_return(rogue_builder *b, nir_jump_instr *jump)
   {
         }
      static void trans_nir_jump(rogue_builder *b, nir_jump_instr *jump)
   {
      switch (jump->type) {
   case nir_jump_return:
            default:
                     }
      static void trans_nir_load_const(rogue_builder *b,
         {
      unsigned dst_index = load_const->def.index;
   unsigned bit_size = load_const->def.bit_size;
   switch (bit_size) {
   case 32: {
      rogue_reg *dst = rogue_ssa_reg(b->shader, dst_index);
   uint32_t imm = nir_const_value_as_uint(load_const->value[0], 32);
                        case 64: {
      uint64_t imm = nir_const_value_as_uint(load_const->value[0], 64);
            rogue_regarray *dst[2] = {
      rogue_ssa_vec_regarray(b->shader, 1, dst_index, 0),
               rogue_MOV(b, rogue_ref_regarray(dst[0]), rogue_ref_imm(imm_2x32[0]));
                        default:
            }
      static void trans_nir_intrinsic_load_input_fs(rogue_builder *b,
         {
               unsigned load_size = intr->def.num_components;
                     struct nir_io_semantics io_semantics = nir_intrinsic_io_semantics(intr);
   unsigned component = nir_intrinsic_component(intr);
   unsigned coeff_index = rogue_coeff_index_fs(&fs_data->iterator_args,
                        rogue_regarray *coeffs = rogue_coeff_regarray(b->shader,
               rogue_regarray *wcoeffs =
            rogue_instr *instr = &rogue_FITRP_PIXEL(b,
                                          }
      static void trans_nir_intrinsic_load_input_vs(rogue_builder *b,
         {
      struct pvr_pipeline_layout *pipeline_layout =
            ASSERTED unsigned load_size = intr->def.num_components;
                     struct nir_io_semantics io_semantics = nir_intrinsic_io_semantics(intr);
   unsigned input = io_semantics.location - VERT_ATTRIB_GENERIC0;
   unsigned component = nir_intrinsic_component(intr);
            if (pipeline_layout) {
      rogue_vertex_inputs *vs_inputs = &b->shader->ctx->stage_data.vs.inputs;
   assert(input < vs_inputs->num_input_vars);
               } else {
      /* Dummy defaults for offline compiler. */
   /* TODO: Load these from an offline description
   * if using the offline compiler.
            nir_shader *nir = b->shader->ctx->nir[MESA_SHADER_VERTEX];
            /* Process inputs. */
   nir_foreach_shader_in_variable (var, nir) {
      unsigned input_components = glsl_get_components(var->type);
   unsigned bit_size =
                        /* Check input location. */
                  if (var->data.location == io_semantics.location) {
      assert(component < input_components);
   vtxin_index += reg_count * component;
                                       rogue_reg *src = rogue_vtxin_reg(b->shader, vtxin_index);
   rogue_instr *instr =
            }
      static void trans_nir_intrinsic_load_input(rogue_builder *b,
         {
      switch (b->shader->stage) {
   case MESA_SHADER_FRAGMENT:
            case MESA_SHADER_VERTEX:
            default:
                     }
      static void trans_nir_intrinsic_store_output_fs(rogue_builder *b,
         {
      ASSERTED unsigned store_size = nir_src_num_components(intr->src[0]);
            /* TODO: When hoisting I/O allocation to the driver, check if this is
   * correct.
   */
            rogue_reg *dst = rogue_pixout_reg(b->shader, pixout_index);
            rogue_instr *instr =
            }
      static void trans_nir_intrinsic_store_output_vs(rogue_builder *b,
         {
               ASSERTED unsigned store_size = nir_src_num_components(intr->src[0]);
            struct nir_io_semantics io_semantics = nir_intrinsic_io_semantics(intr);
   unsigned component = nir_intrinsic_component(intr);
   unsigned vtxout_index = rogue_output_index_vs(&vs_data->outputs,
                  rogue_reg *dst = rogue_vtxout_reg(b->shader, vtxout_index);
            rogue_instr *instr =
            }
      static void trans_nir_intrinsic_store_output(rogue_builder *b,
         {
      switch (b->shader->stage) {
   case MESA_SHADER_FRAGMENT:
            case MESA_SHADER_VERTEX:
            default:
                     }
      static inline gl_shader_stage
   pvr_stage_to_mesa(enum pvr_stage_allocation pvr_stage)
   {
      switch (pvr_stage) {
   case PVR_STAGE_ALLOCATION_VERTEX_GEOMETRY:
            case PVR_STAGE_ALLOCATION_FRAGMENT:
            case PVR_STAGE_ALLOCATION_COMPUTE:
            default:
                     }
      static inline enum pvr_stage_allocation
   mesa_stage_to_pvr(gl_shader_stage mesa_stage)
   {
      switch (mesa_stage) {
   case MESA_SHADER_VERTEX:
            case MESA_SHADER_FRAGMENT:
            case MESA_SHADER_COMPUTE:
            default:
                     }
   static bool descriptor_is_dynamic(VkDescriptorType type)
   {
      return (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      }
      static void
   trans_nir_intrinsic_load_vulkan_descriptor(rogue_builder *b,
         {
      rogue_instr *instr;
   unsigned desc_set = nir_src_comp_as_uint(intr->src[0], 0);
   unsigned binding = nir_src_comp_as_uint(intr->src[0], 1);
   ASSERTED VkDescriptorType desc_type = nir_src_comp_as_uint(intr->src[0], 2);
            struct pvr_pipeline_layout *pipeline_layout =
            unsigned desc_set_table_sh_reg;
   unsigned desc_set_offset;
            if (pipeline_layout) {
      /* Fetch shared registers containing descriptor set table address. */
   enum pvr_stage_allocation pvr_stage = mesa_stage_to_pvr(b->shader->stage);
   assert(pipeline_layout->sh_reg_layout_per_stage[pvr_stage]
         desc_set_table_sh_reg =
                  /* Calculate offset for the descriptor set. */
   assert(desc_set < pipeline_layout->set_count);
   desc_set_offset = desc_set * sizeof(pvr_dev_addr_t); /* Table is an array
            const struct pvr_descriptor_set_layout *set_layout =
         const struct pvr_descriptor_set_layout_mem_layout *mem_layout =
            /* Calculate offset for the descriptor/binding. */
            const struct pvr_descriptor_set_layout_binding *binding_layout =
                  /* TODO: Handle secondaries. */
   /* TODO: Handle bindings having multiple descriptors
   * (VkDescriptorSetLayoutBinding->descriptorCount).
            if (descriptor_is_dynamic(binding_layout->type))
         else
            desc_offset +=
               } else {
      /* Dummy defaults for offline compiler. */
   /* TODO: Load these from an offline pipeline description
   * if using the offline compiler.
   */
   desc_set_table_sh_reg = 0;
   desc_set_offset = desc_set * sizeof(pvr_dev_addr_t);
               unsigned desc_set_table_addr_idx = b->shader->ctx->next_ssa_idx++;
   rogue_ssa_vec_regarray(b->shader, 2, desc_set_table_addr_idx, 0);
   rogue_regarray *desc_set_table_addr_2x32[2] = {
      rogue_ssa_vec_regarray(b->shader, 1, desc_set_table_addr_idx, 0),
               instr = &rogue_MOV(b,
                     rogue_ref_regarray(desc_set_table_addr_2x32[0]),
   rogue_add_instr_comment(instr, "desc_set_table_addr_lo");
   instr =
      &rogue_MOV(
      b,
   rogue_ref_regarray(desc_set_table_addr_2x32[1]),
   rogue_ref_reg(rogue_shared_reg(b->shader, desc_set_table_sh_reg + 1)))
            /* Offset the descriptor set table address to access the descriptor set. */
   unsigned desc_set_table_addr_offset_idx = b->shader->ctx->next_ssa_idx++;
   rogue_regarray *desc_set_table_addr_offset_64 =
         rogue_regarray *desc_set_table_addr_offset_2x32[2] = {
      rogue_ssa_vec_regarray(b->shader, 1, desc_set_table_addr_offset_idx, 0),
               unsigned desc_set_table_addr_offset_lo_idx = b->shader->ctx->next_ssa_idx++;
            rogue_reg *desc_set_table_addr_offset_lo =
         rogue_reg *desc_set_table_addr_offset_hi =
            rogue_MOV(b,
                        rogue_ADD64(b,
               rogue_ref_regarray(desc_set_table_addr_offset_2x32[0]),
   rogue_ref_regarray(desc_set_table_addr_offset_2x32[1]),
   rogue_ref_io(ROGUE_IO_NONE),
   rogue_ref_regarray(desc_set_table_addr_2x32[0]),
   rogue_ref_regarray(desc_set_table_addr_2x32[1]),
            unsigned desc_set_addr_idx = b->shader->ctx->next_ssa_idx++;
   rogue_regarray *desc_set_addr_64 =
         rogue_regarray *desc_set_addr_2x32[2] = {
      rogue_ssa_vec_regarray(b->shader, 1, desc_set_addr_idx, 0),
      };
   instr = &rogue_LD(b,
                     rogue_ref_regarray(desc_set_addr_64),
   rogue_ref_drc(0),
            /* Offset the descriptor set address to access the descriptor. */
   unsigned desc_addr_offset_idx = b->shader->ctx->next_ssa_idx++;
   rogue_regarray *desc_addr_offset_64 =
         rogue_regarray *desc_addr_offset_2x32[2] = {
      rogue_ssa_vec_regarray(b->shader, 1, desc_addr_offset_idx, 0),
               unsigned desc_addr_offset_lo_idx = b->shader->ctx->next_ssa_idx++;
            rogue_reg *desc_addr_offset_val_lo =
         rogue_reg *desc_addr_offset_val_hi =
            rogue_MOV(b,
                        rogue_ADD64(b,
               rogue_ref_regarray(desc_addr_offset_2x32[0]),
   rogue_ref_regarray(desc_addr_offset_2x32[1]),
   rogue_ref_io(ROGUE_IO_NONE),
   rogue_ref_regarray(desc_set_addr_2x32[0]),
   rogue_ref_regarray(desc_set_addr_2x32[1]),
            unsigned desc_addr_idx = intr->def.index;
   rogue_regarray *desc_addr_64 =
         instr = &rogue_LD(b,
                     rogue_ref_regarray(desc_addr_64),
   rogue_ref_drc(0),
      }
      static void trans_nir_intrinsic_load_global_constant(rogue_builder *b,
         {
      /* 64-bit source address. */
   unsigned src_index = intr->src[0].ssa->index;
            /*** TODO NEXT: this could be either a reg or regarray. ***/
            /* TODO NEXT: src[1] should be depending on ssa vec size for burst loads */
   rogue_instr *instr = &rogue_LD(b,
                                    }
      static void trans_nir_intrinsic(rogue_builder *b, nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_input:
            case nir_intrinsic_store_output:
            case nir_intrinsic_load_vulkan_descriptor:
            case nir_intrinsic_load_global_constant:
            default:
                     }
      static void trans_nir_alu_pack_unorm_4x8(rogue_builder *b, nir_alu_instr *alu)
   {
      rogue_ref dst = nir_ssa_reg_alu_dst(b->shader, alu, false);
            rogue_alu_instr *pck_u8888 = rogue_PCK_U8888(b, dst, src);
   rogue_set_instr_repeat(&pck_u8888->instr, 4);
      }
      static void trans_nir_alu_fmul(rogue_builder *b, nir_alu_instr *alu)
   {
      rogue_ref dst = nir_ssa_reg_alu_dst(b->shader, alu, false);
   rogue_ref src0 = nir_ssa_reg_alu_src(b->shader, alu, 0, false);
               }
      static void trans_nir_alu_ffma(rogue_builder *b, nir_alu_instr *alu)
   {
      rogue_ref dst = nir_ssa_reg_alu_dst(b->shader, alu, false);
   rogue_ref src0 = nir_ssa_reg_alu_src(b->shader, alu, 0, false);
   rogue_ref src1 = nir_ssa_reg_alu_src(b->shader, alu, 1, false);
               }
      static void trans_nir_alu_vecN(rogue_builder *b, nir_alu_instr *alu, unsigned n)
   {
      unsigned dst_index = alu->def.index;
   rogue_regarray *dst;
            for (unsigned u = 0; u < n; ++u) {
      dst = rogue_ssa_vec_regarray(b->shader, 1, dst_index, u);
   src = rogue_ssa_reg(b->shader, alu->src[u].src.ssa->index);
         }
      static void trans_nir_alu_iadd64(rogue_builder *b, nir_alu_instr *alu)
   {
      unsigned dst_index = alu->def.index;
   rogue_regarray *dst[2] = {
      rogue_ssa_vec_regarray(b->shader, 1, dst_index, 0),
               unsigned src_index[2] = { alu->src[0].src.ssa->index,
         rogue_regarray *src[2][2] = {
      [0] = {
      rogue_ssa_vec_regarray(b->shader, 1, src_index[0], 0),
      },
   [1] = {
      rogue_ssa_vec_regarray(b->shader, 1, src_index[1], 0),
                  rogue_ADD64(b,
               rogue_ref_regarray(dst[0]),
   rogue_ref_regarray(dst[1]),
   rogue_ref_io(ROGUE_IO_NONE),
   rogue_ref_regarray(src[0][0]),
   rogue_ref_regarray(src[0][1]),
      }
      static void trans_nir_alu_iadd(rogue_builder *b, nir_alu_instr *alu)
   {
               switch (bit_size) {
            case 64:
            default:
                     }
      static void trans_nir_alu(rogue_builder *b, nir_alu_instr *alu)
   {
      switch (alu->op) {
   case nir_op_pack_unorm_4x8:
      return trans_nir_alu_pack_unorm_4x8(b, alu);
         case nir_op_fmul:
            case nir_op_ffma:
            case nir_op_vec4:
            case nir_op_iadd:
            default:
                     }
      PUBLIC
   unsigned rogue_count_used_regs(const rogue_shader *shader,
         {
      unsigned reg_count;
   if (rogue_reg_infos[class].num) {
      reg_count = __bitset_count(shader->regs_used[class],
      } else {
               #ifndef NDEBUG
      /* Check that registers are contiguous. */
   rogue_foreach_reg (reg, shader, class) {
            #endif /* NDEBUG */
            }
      static inline void rogue_feedback_used_regs(rogue_build_ctx *ctx,
         {
      /* TODO NEXT: Use this counting method elsewhere as well. */
   ctx->common_data[shader->stage].temps =
         ctx->common_data[shader->stage].internals =
      }
      static bool ssa_def_cb(nir_def *ssa, void *state)
   {
               if (ssa->num_components == 1) {
      if (ssa->bit_size == 32) {
         } else if (ssa->bit_size == 64) {
            } else {
                  /* Keep track of the last SSA index so we can use more. */
               }
      /**
   * \brief Translates a NIR shader to Rogue.
   *
   * \param[in] ctx Shared multi-stage build context.
   * \param[in] nir NIR shader.
   * \return A rogue_shader* if successful, or NULL if unsuccessful.
   */
   PUBLIC
   rogue_shader *rogue_nir_to_rogue(rogue_build_ctx *ctx, const nir_shader *nir)
   {
      gl_shader_stage stage = nir->info.stage;
   rogue_shader *shader = rogue_shader_create(ctx, stage);
   if (!shader)
                     /* Make sure we only have a single function. */
            rogue_builder b;
                     /* Go through SSA used by NIR and "reserve" them so that sub-arrays won't be
   * declared before the parent arrays. */
   nir_foreach_block_unstructured (block, entry) {
      nir_foreach_instr (instr, block) {
      if (instr->type == nir_instr_type_load_const) {
      nir_load_const_instr *load_const = nir_instr_as_load_const(instr);
   if (load_const->def.num_components > 1)
      }
         }
            /* Translate shader entrypoint. */
   nir_foreach_block (block, entry) {
               nir_foreach_instr (instr, block) {
      switch (instr->type) {
   case nir_instr_type_alu:
                  case nir_instr_type_intrinsic:
                  case nir_instr_type_load_const:
                  case nir_instr_type_jump:
                  default:
                        /* Apply passes. */
                        }
      /**
   * \brief Performs Rogue passes on a shader.
   *
   * \param[in] shader The shader.
   */
   PUBLIC
   void rogue_shader_passes(rogue_shader *shader)
   {
               if (ROGUE_DEBUG(IR_PASSES))
            /* Passes */
   ROGUE_PASS_V(shader, rogue_constreg);
   ROGUE_PASS_V(shader, rogue_copy_prop);
   ROGUE_PASS_V(shader, rogue_dce);
   ROGUE_PASS_V(shader, rogue_lower_pseudo_ops);
   ROGUE_PASS_V(shader, rogue_schedule_wdf, false);
   ROGUE_PASS_V(shader, rogue_schedule_uvsw, false);
   ROGUE_PASS_V(shader, rogue_trim);
   ROGUE_PASS_V(shader, rogue_regalloc);
   ROGUE_PASS_V(shader, rogue_lower_late_ops);
   ROGUE_PASS_V(shader, rogue_dce);
            if (ROGUE_DEBUG(IR))
      }
