   /*
   * Copyright (c) 2017 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include <string.h>
      #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "util/bitscan.h"
   #include "compiler/nir/nir.h"
   #include "pipe/p_state.h"
   #include "nir_legacy.h"
         #include "ppir.h"
      static void *ppir_node_create_ssa(ppir_block *block, ppir_op op, nir_def *ssa)
   {
      ppir_node *node = ppir_node_create(block, op, ssa->index, 0);
   if (!node)
            ppir_dest *dest = ppir_node_get_dest(node);
   dest->type = ppir_target_ssa;
   dest->ssa.num_components = ssa->num_components;
            if (node->type == ppir_node_type_load ||
      node->type == ppir_node_type_store)
            }
      static void *ppir_node_create_reg(ppir_block *block, ppir_op op,
         {
      ppir_node *node = ppir_node_create(block, op, def->index, mask);
   if (!node)
                     list_for_each_entry(ppir_reg, r, &block->comp->reg_list, list) {
      if (r->index == def->index) {
      dest->reg = r;
                  dest->type = ppir_target_register;
            if (node->type == ppir_node_type_load ||
      node->type == ppir_node_type_store)
            }
      static void *ppir_node_create_dest(ppir_block *block, ppir_op op,
         {
               if (dest) {
      if (dest->is_ssa)
         else
                  }
      static void ppir_node_add_src(ppir_compiler *comp, ppir_node *node,
         {
               if (ns->is_ssa) {
      child = comp->var_nodes[ns->ssa->index];
   if (child->op != ppir_op_undef)
      }
   else {
      nir_reg_src *rs = &ns->reg;
   while (mask) {
      int swizzle = ps->swizzle[u_bit_scan(&mask)];
   child = comp->var_nodes[(rs->handle->index << 2) + swizzle];
   /* Reg is read before it was written, create a dummy node for it */
   if (!child) {
      child = ppir_node_create_reg(node->block, ppir_op_dummy, rs->handle,
            }
   /* Don't add dummies or recursive deps for ops like r1 = r1 + ssa1 */
   if (child && node != child && child->op != ppir_op_dummy)
                  assert(child);
      }
      static int nir_to_ppir_opcodes[nir_num_opcodes] = {
      [nir_op_mov] = ppir_op_mov,
   [nir_op_fmul] = ppir_op_mul,
   [nir_op_fabs] = ppir_op_abs,
   [nir_op_fneg] = ppir_op_neg,
   [nir_op_fadd] = ppir_op_add,
   [nir_op_fsum3] = ppir_op_sum3,
   [nir_op_fsum4] = ppir_op_sum4,
   [nir_op_frsq] = ppir_op_rsqrt,
   [nir_op_flog2] = ppir_op_log2,
   [nir_op_fexp2] = ppir_op_exp2,
   [nir_op_fsqrt] = ppir_op_sqrt,
   [nir_op_fsin] = ppir_op_sin,
   [nir_op_fcos] = ppir_op_cos,
   [nir_op_fmax] = ppir_op_max,
   [nir_op_fmin] = ppir_op_min,
   [nir_op_frcp] = ppir_op_rcp,
   [nir_op_ffloor] = ppir_op_floor,
   [nir_op_fceil] = ppir_op_ceil,
   [nir_op_ffract] = ppir_op_fract,
   [nir_op_sge] = ppir_op_ge,
   [nir_op_slt] = ppir_op_lt,
   [nir_op_seq] = ppir_op_eq,
   [nir_op_sne] = ppir_op_ne,
   [nir_op_fcsel] = ppir_op_select,
   [nir_op_inot] = ppir_op_not,
   [nir_op_ftrunc] = ppir_op_trunc,
   [nir_op_fsat] = ppir_op_sat,
   [nir_op_fddx] = ppir_op_ddx,
      };
      static bool ppir_emit_alu(ppir_block *block, nir_instr *ni)
   {
      nir_alu_instr *instr = nir_instr_as_alu(ni);
   nir_def *def = &instr->def;
            if (op == ppir_op_unsupported) {
      ppir_error("unsupported nir_op: %s\n", nir_op_infos[instr->op].name);
      }
            /* Don't try to translate folded fsat since their source won't be valid */
   if (instr->op == nir_op_fsat && nir_legacy_fsat_folds(instr))
            /* Skip folded fabs/fneg since we do not have dead code elimination */
   if ((instr->op == nir_op_fabs || instr->op == nir_op_fneg) &&
      nir_legacy_float_mod_folds(instr)) {
   /* Add parent node as a the folded def node to keep
   * the dependency chain */
   nir_alu_src *ns = &instr->src[0];
   ppir_node *parent = block->comp->var_nodes[ns->src.ssa->index];
   assert(parent);
   block->comp->var_nodes[def->index] = parent;
               ppir_alu_node *node = ppir_node_create_dest(block, op, &legacy_dest.dest,
         if (!node)
            ppir_dest *pd = &node->dest;
   if (legacy_dest.fsat)
            unsigned src_mask;
   switch (op) {
   case ppir_op_sum3:
      src_mask = 0b0111;
      case ppir_op_sum4:
      src_mask = 0b1111;
      default:
      src_mask = pd->write_mask;
               unsigned num_child = nir_op_infos[instr->op].num_inputs;
            for (int i = 0; i < num_child; i++) {
      nir_legacy_alu_src ns = nir_legacy_chase_alu_src(instr->src + i, true);
   ppir_src *ps = node->src + i;
   memcpy(ps->swizzle, ns.swizzle, sizeof(ps->swizzle));
            ps->absolute = ns.fabs;
               list_addtail(&node->node.list, &block->node_list);
      }
      static ppir_block *ppir_block_create(ppir_compiler *comp);
      static bool ppir_emit_discard_block(ppir_compiler *comp)
   {
      ppir_block *block = ppir_block_create(comp);
   ppir_discard_node *discard;
   if (!block)
            comp->discard_block = block;
            discard = ppir_node_create(block, ppir_op_discard, -1, 0);
   if (discard)
         else
               }
      static ppir_node *ppir_emit_discard_if(ppir_block *block, nir_instr *ni)
   {
      nir_intrinsic_instr *instr = nir_instr_as_intrinsic(ni);
   ppir_node *node;
   ppir_compiler *comp = block->comp;
            if (!comp->discard_block && !ppir_emit_discard_block(comp))
            node = ppir_node_create(block, ppir_op_branch, -1, 0);
   if (!node)
                  /* second src and condition will be updated during lowering */
   nir_legacy_src legacy_src = nir_legacy_chase_src(&instr->src[0]);
   ppir_node_add_src(block->comp, node, &branch->src[0],
         branch->num_src = 1;
               }
      static ppir_node *ppir_emit_discard(ppir_block *block, nir_instr *ni)
   {
                  }
      static bool ppir_emit_intrinsic(ppir_block *block, nir_instr *ni)
   {
      ppir_node *node;
   nir_intrinsic_instr *instr = nir_instr_as_intrinsic(ni);
   unsigned mask = 0;
   ppir_load_node *lnode;
            switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg:
   case nir_intrinsic_store_reg:
      /* Nothing to do for these */
         case nir_intrinsic_load_reg: {
      nir_legacy_dest legacy_dest = nir_legacy_chase_dest(&instr->def);
   lnode = ppir_node_create_dest(block, ppir_op_dummy, &legacy_dest, mask);
               case nir_intrinsic_load_input: {
               nir_legacy_dest legacy_dest = nir_legacy_chase_dest(&instr->def);
   lnode = ppir_node_create_dest(block, ppir_op_load_varying, &legacy_dest, mask);
   if (!lnode)
            lnode->num_components = instr->num_components;
   lnode->index = nir_intrinsic_base(instr) * 4 + nir_intrinsic_component(instr);
   if (nir_src_is_const(instr->src[0]))
         else {
      lnode->num_src = 1;
   nir_legacy_src legacy_src = nir_legacy_chase_src(instr->src);
      }
   list_addtail(&lnode->node.list, &block->node_list);
               case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_point_coord:
   case nir_intrinsic_load_front_face: {
               ppir_op op;
   switch (instr->intrinsic) {
   case nir_intrinsic_load_frag_coord:
      op = ppir_op_load_fragcoord;
      case nir_intrinsic_load_point_coord:
      op = ppir_op_load_pointcoord;
      case nir_intrinsic_load_front_face:
      op = ppir_op_load_frontface;
      default:
      unreachable("bad intrinsic");
               nir_legacy_dest legacy_dest = nir_legacy_chase_dest(&instr->def);
   lnode = ppir_node_create_dest(block, op, &legacy_dest, mask);
   if (!lnode)
            lnode->num_components = instr->num_components;
   list_addtail(&lnode->node.list, &block->node_list);
               case nir_intrinsic_load_uniform: {
               nir_legacy_dest legacy_dest = nir_legacy_chase_dest(&instr->def);
   lnode = ppir_node_create_dest(block, ppir_op_load_uniform, &legacy_dest, mask);
   if (!lnode)
            lnode->num_components = instr->num_components;
   lnode->index = nir_intrinsic_base(instr);
   if (nir_src_is_const(instr->src[0]))
         else {
      lnode->num_src = 1;
   nir_legacy_src legacy_src = nir_legacy_chase_src(instr->src);
               list_addtail(&lnode->node.list, &block->node_list);
               case nir_intrinsic_store_output: {
      /* In simple cases where the store_output is ssa, that register
   * can be directly marked as the output.
   * If discard is used or the source is not ssa, things can get a
   * lot more complicated, so don't try to optimize those and fall
   * back to inserting a mov at the end.
   * If the source node will only be able to output to pipeline
   * registers, fall back to the mov as well. */
   assert(nir_src_is_const(instr->src[1]) &&
            nir_io_semantics io = nir_intrinsic_io_semantics(instr);
   unsigned offset = nir_src_as_uint(instr->src[1]);
   unsigned slot = io.location + offset;
   ppir_output_type out_type = ppir_nir_output_to_ppir(slot,
         if (out_type == ppir_output_invalid) {
      ppir_debug("Unsupported output type: %d\n", slot);
               if (!block->comp->uses_discard) {
      node = block->comp->var_nodes[instr->src->ssa->index];
   assert(node);
   switch (node->op) {
   case ppir_op_load_uniform:
   case ppir_op_load_texture:
   case ppir_op_dummy:
   case ppir_op_const:
         default: {
      ppir_dest *dest = ppir_node_get_dest(node);
   dest->ssa.out_type = out_type;
   node->is_out = 1;
   return true;
                  alu_node = ppir_node_create_dest(block, ppir_op_mov, NULL, 0);
   if (!alu_node)
            ppir_dest *dest = ppir_node_get_dest(&alu_node->node);
   dest->type = ppir_target_ssa;
   dest->ssa.num_components = instr->num_components;
   dest->ssa.index = 0;
   dest->write_mask = u_bit_consecutive(0, instr->num_components);
                     for (int i = 0; i < instr->num_components; i++)
            nir_legacy_src legacy_src = nir_legacy_chase_src(instr->src);
   ppir_node_add_src(block->comp, &alu_node->node, alu_node->src, &legacy_src,
                     list_addtail(&alu_node->node.list, &block->node_list);
               case nir_intrinsic_discard:
      node = ppir_emit_discard(block, ni);
   list_addtail(&node->list, &block->node_list);
         case nir_intrinsic_discard_if:
      node = ppir_emit_discard_if(block, ni);
   list_addtail(&node->list, &block->node_list);
         default:
      ppir_error("unsupported nir_intrinsic_instr %s\n",
               }
      static bool ppir_emit_load_const(ppir_block *block, nir_instr *ni)
   {
      nir_load_const_instr *instr = nir_instr_as_load_const(ni);
   ppir_const_node *node = ppir_node_create_ssa(block, ppir_op_const, &instr->def);
   if (!node)
                     for (int i = 0; i < instr->def.num_components; i++)
                  list_addtail(&node->node.list, &block->node_list);
      }
      static bool ppir_emit_ssa_undef(ppir_block *block, nir_instr *ni)
   {
      nir_undef_instr *undef = nir_instr_as_undef(ni);
   ppir_node *node = ppir_node_create_ssa(block, ppir_op_undef, &undef->def);
   if (!node)
                  ppir_dest *dest = &alu->dest;
            list_addtail(&node->list, &block->node_list);
      }
      static bool ppir_emit_tex(ppir_block *block, nir_instr *ni)
   {
      nir_tex_instr *instr = nir_instr_as_tex(ni);
            switch (instr->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
         default:
      ppir_error("unsupported texop %d\n", instr->op);
               switch (instr->sampler_dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_3D:
   case GLSL_SAMPLER_DIM_CUBE:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_EXTERNAL:
         default:
      ppir_error("unsupported sampler dim: %d\n", instr->sampler_dim);
                        unsigned mask = 0;
            nir_legacy_dest legacy_dest = nir_legacy_chase_dest(&instr->def);
   node = ppir_node_create_dest(block, ppir_op_load_texture, &legacy_dest, mask);
   if (!node)
            node->sampler = instr->texture_index;
            for (int i = 0; i < instr->coord_components; i++)
                     for (int i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_backend1:
      perspective = true;
      case nir_tex_src_coord: {
      nir_src *ns = &instr->src[i].src;
   ppir_node *child = block->comp->var_nodes[ns->ssa->index];
   if (child->op == ppir_op_load_varying) {
      /* If the successor is load_texture, promote it to load_coords */
   nir_tex_src *nts = (nir_tex_src *)ns;
   if (nts->src_type == nir_tex_src_coord ||
                     /* src[0] is not used by the ld_tex instruction but ensures
   * correct scheduling due to the pipeline dependency */
   nir_legacy_src legacy_src = nir_legacy_chase_src(&instr->src[i].src);
   ppir_node_add_src(block->comp, &node->node, &node->src[0], &legacy_src,
         node->num_src++;
      }
   case nir_tex_src_bias:
   case nir_tex_src_lod:
      node->lod_bias_en = true;
   node->explicit_lod = (instr->src[i].src_type == nir_tex_src_lod);
   nir_legacy_src legacy_src = nir_legacy_chase_src(&instr->src[i].src);
   ppir_node_add_src(block->comp, &node->node, &node->src[1], &legacy_src, 1);
   node->num_src++;
      default:
      ppir_error("unsupported texture source type\n");
                                    ppir_node *src_coords = ppir_node_get_src(&node->node, 0)->node;
            if (src_coords && ppir_node_has_single_src_succ(src_coords) &&
      (src_coords->op == ppir_op_load_coords))
      else {
      /* Create load_coords node */
   load = ppir_node_create(block, ppir_op_load_coords_reg, -1, 0);
   if (!load)
                  load->src = node->src[0];
   load->num_src = 1;
            ppir_debug("%s create load_coords node %d for %d\n",
            ppir_node_foreach_pred_safe((&node->node), dep) {
      ppir_node *pred = dep->pred;
   ppir_node_remove_dep(dep);
      }
                        if (perspective) {
      if (instr->coord_components == 3)
         else
               load->sampler_dim = instr->sampler_dim;
   node->src[0].type = load->dest.type = ppir_target_pipeline;
               }
      static ppir_block *ppir_get_block(ppir_compiler *comp, nir_block *nblock)
   {
                  }
      static bool ppir_emit_jump(ppir_block *block, nir_instr *ni)
   {
      ppir_node *node;
   ppir_compiler *comp = block->comp;
   ppir_branch_node *branch;
   ppir_block *jump_block;
            switch (jump->type) {
   case nir_jump_break: {
      assert(comp->current_block->successors[0]);
   assert(!comp->current_block->successors[1]);
      }
   break;
   case nir_jump_continue:
         break;
   default:
      ppir_error("nir_jump_instr not support\n");
                        node = ppir_node_create(block, ppir_op_branch, -1, 0);
   if (!node)
                  /* Unconditional */
   branch->num_src = 0;
            list_addtail(&node->list, &block->node_list);
      }
      static bool (*ppir_emit_instr[nir_instr_type_phi])(ppir_block *, nir_instr *) = {
      [nir_instr_type_alu]        = ppir_emit_alu,
   [nir_instr_type_intrinsic]  = ppir_emit_intrinsic,
   [nir_instr_type_load_const] = ppir_emit_load_const,
   [nir_instr_type_undef]      = ppir_emit_ssa_undef,
   [nir_instr_type_tex]        = ppir_emit_tex,
      };
      static ppir_block *ppir_block_create(ppir_compiler *comp)
   {
      ppir_block *block = rzalloc(comp, ppir_block);
   if (!block)
            list_inithead(&block->node_list);
                        }
      static bool ppir_emit_block(ppir_compiler *comp, nir_block *nblock)
   {
                                 nir_foreach_instr(instr, nblock) {
      assert(instr->type < nir_instr_type_phi);
   if (!ppir_emit_instr[instr->type](block, instr))
                  }
      static bool ppir_emit_cf_list(ppir_compiler *comp, struct exec_list *list);
      static bool ppir_emit_if(ppir_compiler *comp, nir_if *if_stmt)
   {
      ppir_node *node;
   ppir_branch_node *else_branch, *after_branch;
   nir_block *nir_else_block = nir_if_first_else_block(if_stmt);
   bool empty_else_block =
      (nir_else_block == nir_if_last_else_block(if_stmt) &&
               node = ppir_node_create(block, ppir_op_branch, -1, 0);
   if (!node)
         else_branch = ppir_node_to_branch(node);
   nir_legacy_src legacy_src = nir_legacy_chase_src(&if_stmt->condition);
   ppir_node_add_src(block->comp, node, &else_branch->src[0],
         else_branch->num_src = 1;
   /* Negate condition to minimize branching. We're generating following:
   * current_block: { ...; if (!statement) branch else_block; }
   * then_block: { ...; branch after_block; }
   * else_block: { ... }
   * after_block: { ... }
   *
   * or if else list is empty:
   * block: { if (!statement) branch else_block; }
   * then_block: { ... }
   * else_block: after_block: { ... }
   */
   else_branch->negate = true;
            if (!ppir_emit_cf_list(comp, &if_stmt->then_list))
            if (empty_else_block) {
      nir_block *nblock = nir_if_last_else_block(if_stmt);
   assert(nblock->successors[0]);
   assert(!nblock->successors[1]);
   else_branch->target = ppir_get_block(comp, nblock->successors[0]);
   /* Add empty else block to the list */
   list_addtail(&block->successors[1]->list, &comp->block_list);
                        nir_block *last_then_block = nir_if_last_then_block(if_stmt);
   assert(last_then_block->successors[0]);
   assert(!last_then_block->successors[1]);
   block = ppir_get_block(comp, last_then_block);
   node = ppir_node_create(block, ppir_op_branch, -1, 0);
   if (!node)
         after_branch = ppir_node_to_branch(node);
   /* Unconditional */
   after_branch->num_src = 0;
   after_branch->target = ppir_get_block(comp, last_then_block->successors[0]);
   /* Target should be after_block, will fixup later */
            if (!ppir_emit_cf_list(comp, &if_stmt->else_list))
               }
      static bool ppir_emit_loop(ppir_compiler *comp, nir_loop *nloop)
   {
      assert(!nir_loop_has_continue_construct(nloop));
   ppir_block *save_loop_cont_block = comp->loop_cont_block;
   ppir_block *block;
   ppir_branch_node *loop_branch;
   nir_block *loop_last_block;
                     if (!ppir_emit_cf_list(comp, &nloop->body))
            loop_last_block = nir_loop_last_block(nloop);
   block = ppir_get_block(comp, loop_last_block);
   node = ppir_node_create(block, ppir_op_branch, -1, 0);
   if (!node)
         loop_branch = ppir_node_to_branch(node);
   /* Unconditional */
   loop_branch->num_src = 0;
   loop_branch->target = comp->loop_cont_block;
                                 }
      static bool ppir_emit_function(ppir_compiler *comp, nir_function_impl *nfunc)
   {
      ppir_error("function nir_cf_node not support\n");
      }
      static bool ppir_emit_cf_list(ppir_compiler *comp, struct exec_list *list)
   {
      foreach_list_typed(nir_cf_node, node, node, list) {
               switch (node->type) {
   case nir_cf_node_block:
      ret = ppir_emit_block(comp, nir_cf_node_as_block(node));
      case nir_cf_node_if:
      ret = ppir_emit_if(comp, nir_cf_node_as_if(node));
      case nir_cf_node_loop:
      ret = ppir_emit_loop(comp, nir_cf_node_as_loop(node));
      case nir_cf_node_function:
      ret = ppir_emit_function(comp, nir_cf_node_as_function(node));
      default:
      ppir_error("unknown NIR node type %d\n", node->type);
               if (!ret)
                  }
      static ppir_compiler *ppir_compiler_create(void *prog, unsigned num_ssa)
   {
      ppir_compiler *comp = rzalloc_size(
         if (!comp)
            list_inithead(&comp->block_list);
   list_inithead(&comp->reg_list);
   comp->reg_num = 0;
            comp->var_nodes = (ppir_node **)(comp + 1);
               }
      static void ppir_add_ordering_deps(ppir_compiler *comp)
   {
      /* Some intrinsics do not have explicit dependencies and thus depend
   * on instructions order. Consider discard_if and the is_end node as
   * example. If we don't add fake dependency of discard_if to is_end,
   * scheduler may put the is_end first and since is_end terminates
   * shader on Utgard PP, rest of it will never be executed.
   * Add fake dependencies for discard/branch/store to preserve
   * instruction order.
   *
   * TODO: scheduler should schedule discard_if as early as possible otherwise
   * we may end up with suboptimal code for cases like this:
   *
   * s3 = s1 < s2
   * discard_if s3
   * s4 = s1 + s2
   * store s4
   *
   * In this case store depends on discard_if and s4, but since dependencies can
   * be scheduled in any order it can result in code like this:
   *
   * instr1: s3 = s1 < s3
   * instr2: s4 = s1 + s2
   * instr3: discard_if s3
   * instr4: store s4
   */
   list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      ppir_node *prev_node = NULL;
   list_for_each_entry_rev(ppir_node, node, &block->node_list, list) {
      if (prev_node && ppir_node_is_root(node) && node->op != ppir_op_const) {
         }
   if (node->is_out ||
      node->op == ppir_op_discard ||
   node->op == ppir_op_store_temp ||
   node->op == ppir_op_branch) {
               }
      static void ppir_print_shader_db(struct nir_shader *nir, ppir_compiler *comp,
         {
      const struct shader_info *info = &nir->info;
   char *shaderdb;
   ASSERTED int ret = asprintf(&shaderdb,
                              "%s shader: %d inst, %d loops, %d:%d spills:fills\n",
               if (lima_debug & LIMA_DEBUG_SHADERDB)
            util_debug_message(debug, SHADER_INFO, "%s", shaderdb);
      }
      static void ppir_add_write_after_read_deps(ppir_compiler *comp)
   {
      list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_reg, reg, &comp->reg_list, list) {
      ppir_node *write = NULL;
   list_for_each_entry_rev(ppir_node, node, &block->node_list, list) {
      for (int i = 0; i < ppir_node_get_src_num(node); i++) {
      ppir_src *src = ppir_node_get_src(node, i);
   if (src && src->type == ppir_target_register &&
      src->reg == reg &&
   write) {
   ppir_debug("Adding dep %d for write %d\n", node->index, write->index);
         }
   ppir_dest *dest = ppir_node_get_dest(node);
   if (dest && dest->type == ppir_target_register &&
      dest->reg == reg)
            }
      bool ppir_compile_nir(struct lima_fs_compiled_shader *prog, struct nir_shader *nir,
               {
      nir_function_impl *func = nir_shader_get_entrypoint(nir);
   ppir_compiler *comp = ppir_compiler_create(prog, func->ssa_alloc);
   if (!comp)
            comp->ra = ra;
   comp->uses_discard = nir->info.fs.uses_discard;
            /* 1st pass: create ppir blocks */
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(nblock, impl) {
      ppir_block *block = ppir_block_create(comp);
   if (!block)
         block->index = nblock->index;
                  /* 2nd pass: populate successors */
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(nblock, impl) {
                     for (int i = 0; i < 2; i++) {
      if (nblock->successors[i])
                              /* -1 means reg is not written by the shader */
   for (int i = 0; i < ppir_output_num; i++)
            nir_foreach_reg_decl(decl, func) {
      ppir_reg *r = rzalloc(comp, ppir_reg);
   if (!r)
            r->index = decl->def.index;
   r->num_components = nir_intrinsic_num_components(decl);
   r->is_head = false;
   list_addtail(&r->list, &comp->reg_list);
               if (!ppir_emit_cf_list(comp, &func->body))
            /* If we have discard block add it to the very end */
   if (comp->discard_block)
                     if (!ppir_lower_prog(comp))
            ppir_add_ordering_deps(comp);
                     if (!ppir_node_to_instr(comp))
            if (!ppir_schedule_prog(comp))
            if (!ppir_regalloc_prog(comp))
            if (!ppir_codegen_prog(comp))
                     _mesa_hash_table_u64_destroy(comp->blocks);
   ralloc_free(comp);
         err_out0:
      _mesa_hash_table_u64_destroy(comp->blocks);
   ralloc_free(comp);
      }
   