   /*
   * Copyright (C) 2020-2023 Collabora, Ltd.
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
      #include "compiler/nir/nir_builder.h"
   #include "pan_context.h"
      struct ctx {
      struct panfrost_sysvals *sysvals;
   struct hash_table_u64 *sysval_to_id;
      };
      static unsigned
   lookup_sysval(struct hash_table_u64 *sysval_to_id,
         {
      /* Try to lookup */
            if (cached) {
      unsigned id = ((uintptr_t)cached) - 1;
   assert(id < MAX_SYSVAL_COUNT);
   assert(sysvals->sysvals[id] == sysval);
               /* Else assign */
   unsigned id = sysvals->sysval_count++;
   assert(id < MAX_SYSVAL_COUNT);
   _mesa_hash_table_u64_insert(sysval_to_id, sysval,
         sysvals->sysvals[id] = sysval;
      }
      static unsigned
   sysval_for_intrinsic(nir_intrinsic_instr *intr, unsigned *offset)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_ssbo_address:
         case nir_intrinsic_get_ssbo_size:
      *offset = 8;
         case nir_intrinsic_load_sampler_lod_parameters_pan:
      /* This is only used for a workaround on Mali-T720, where we don't
   * support dynamic samplers.
   */
         case nir_intrinsic_load_xfb_address:
            case nir_intrinsic_load_work_dim:
            case nir_intrinsic_load_sample_positions_pan:
            case nir_intrinsic_load_num_vertices:
            case nir_intrinsic_load_first_vertex:
         case nir_intrinsic_load_base_vertex:
      *offset = 4;
      case nir_intrinsic_load_base_instance:
      *offset = 8;
         case nir_intrinsic_load_draw_id:
            case nir_intrinsic_load_multisampled_pan:
            case nir_intrinsic_load_viewport_scale:
            case nir_intrinsic_load_viewport_offset:
            case nir_intrinsic_load_num_workgroups:
            case nir_intrinsic_load_workgroup_size:
            case nir_intrinsic_load_rt_conversion_pan: {
      unsigned size = nir_alu_type_get_type_size(nir_intrinsic_src_type(intr));
                        case nir_intrinsic_image_size: {
      uint32_t uindex = nir_src_as_uint(intr->src[0]);
   bool is_array = nir_intrinsic_image_array(intr);
                        default:
            }
      static bool
   lower(nir_builder *b, nir_instr *instr, void *data)
   {
      struct ctx *ctx = data;
   nir_def *old = NULL;
   unsigned sysval = ~0, offset = 0;
            if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   old = &intr->def;
            if (sysval == ~0)
      } else if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
            if (tex->op != nir_texop_txs)
            /* XXX: This is broken for dynamic indexing */
   sysval = PAN_SYSVAL(TEXTURE_SIZE,
                        } else {
                  /* Allocate a UBO for the sysvals if we haven't yet */
   if (ctx->sysvals->sysval_count == 0)
            unsigned vec4_index = lookup_sysval(ctx->sysval_to_id, ctx->sysvals, sysval);
            b->cursor = nir_after_instr(instr);
   nir_def *val = nir_load_ubo(
      b, old->num_components, old->bit_size, nir_imm_int(b, ctx->sysval_ubo),
   nir_imm_int(b, ubo_offset), .align_mul = old->bit_size / 8,
      nir_def_rewrite_uses(old, val);
      }
      bool
   panfrost_nir_lower_sysvals(nir_shader *shader, struct panfrost_sysvals *sysvals)
   {
               /* The lowerings for SSBOs, etc require constants, so fold now */
   do {
               NIR_PASS(progress, shader, nir_copy_prop);
   NIR_PASS(progress, shader, nir_opt_constant_folding);
               struct ctx ctx = {
      .sysvals = sysvals,
                        nir_shader_instructions_pass(
            _mesa_hash_table_u64_destroy(ctx.sysval_to_id);
      }
