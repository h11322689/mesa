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
      #include "util/u_math.h"
   #include "util/ralloc.h"
   #include "util/bitscan.h"
      #include "ppir.h"
      const ppir_op_info ppir_op_infos[] = {
      [ppir_op_unsupported] = {
         },
   [ppir_op_mov] = {
      .name = "mov",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_SCL_MUL,
   PPIR_INSTR_SLOT_ALU_VEC_ADD, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_abs] = {
         },
   [ppir_op_neg] = {
         },
   [ppir_op_sat] = {
         },
   [ppir_op_mul] = {
      .name = "mul",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_add] = {
      .name = "add",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_sum3] = {
      .name = "sum3",
   .slots = (int []) {
            },
   [ppir_op_sum4] = {
      .name = "sum4",
   .slots = (int []) {
            },
   [ppir_op_rsqrt] = {
      .name = "rsqrt",
   .slots = (int []) {
            },
   [ppir_op_log2] = {
      .name = "log2",
   .slots = (int []) {
            },
   [ppir_op_exp2] = {
      .name = "exp2",
   .slots = (int []) {
            },
   [ppir_op_sqrt] = {
      .name = "sqrt",
   .slots = (int []) {
            },
   [ppir_op_sin] = {
      .name = "sin",
   .slots = (int []) {
            },
   [ppir_op_cos] = {
      .name = "cos",
   .slots = (int []) {
            },
   [ppir_op_max] = {
      .name = "max",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_SCL_MUL,
   PPIR_INSTR_SLOT_ALU_VEC_ADD, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_min] = {
      .name = "min",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_SCL_MUL,
   PPIR_INSTR_SLOT_ALU_VEC_ADD, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_floor] = {
      .name = "floor",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_ceil] = {
      .name = "ceil",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_fract] = {
      .name = "fract",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_ddx] = {
      .name = "ddx",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_ddy] = {
      .name = "ddy",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_and] = {
      .name = "and",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_or] = {
      .name = "or",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_xor] = {
      .name = "xor",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_not] = {
      .name = "not",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_VEC_MUL,
         },
   [ppir_op_lt] = {
         },
   [ppir_op_le] = {
         },
   [ppir_op_gt] = {
      .name = "gt",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_SCL_ADD,
   PPIR_INSTR_SLOT_ALU_VEC_MUL, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_ge] = {
      .name = "ge",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_SCL_ADD,
   PPIR_INSTR_SLOT_ALU_VEC_MUL, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_eq] = {
      .name = "eq",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_SCL_ADD,
   PPIR_INSTR_SLOT_ALU_VEC_MUL, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_ne] = {
      .name = "ne",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_MUL, PPIR_INSTR_SLOT_ALU_SCL_ADD,
   PPIR_INSTR_SLOT_ALU_VEC_MUL, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_select] = {
      .name = "select",
   .slots = (int []) {
      PPIR_INSTR_SLOT_ALU_SCL_ADD, PPIR_INSTR_SLOT_ALU_VEC_ADD,
         },
   [ppir_op_rcp] = {
      .name = "rcp",
   .slots = (int []) {
            },
   [ppir_op_load_varying] = {
      .name = "ld_var",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_coords] = {
      .name = "ld_coords",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_coords_reg] = {
      .name = "ld_coords_reg",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_fragcoord] = {
      .name = "ld_fragcoord",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_pointcoord] = {
      .name = "ld_pointcoord",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_frontface] = {
      .name = "ld_frontface",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_uniform] = {
      .name = "ld_uni",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_load_texture] = {
      .name = "ld_tex",
   .type = ppir_node_type_load_texture,
   .slots = (int []) {
            },
   [ppir_op_load_temp] = {
      .name = "ld_temp",
   .type = ppir_node_type_load,
   .slots = (int []) {
            },
   [ppir_op_const] = {
      .name = "const",
      },
   [ppir_op_store_temp] = {
      .name = "st_temp",
   .type = ppir_node_type_store,
   .slots = (int []) {
            },
   [ppir_op_discard] = {
      .name = "discard",
   .type = ppir_node_type_discard,
   .slots = (int []) {
            },
   [ppir_op_branch] = {
      .name = "branch",
   .type = ppir_node_type_branch,
   .slots = (int []) {
            },
   [ppir_op_undef] = {
      .name = "undef",
   .type = ppir_node_type_alu,
   .slots = (int []) {
            },
   [ppir_op_dummy] = {
      .name = "dummy",
   .type = ppir_node_type_alu,
   .slots = (int []) {
               };
      void *ppir_node_create(ppir_block *block, ppir_op op, int index, unsigned mask)
   {
      ppir_compiler *comp = block->comp;
   static const int node_size[] = {
      [ppir_node_type_alu] = sizeof(ppir_alu_node),
   [ppir_node_type_const] = sizeof(ppir_const_node),
   [ppir_node_type_load] = sizeof(ppir_load_node),
   [ppir_node_type_store] = sizeof(ppir_store_node),
   [ppir_node_type_load_texture] = sizeof(ppir_load_texture_node),
   [ppir_node_type_discard] = sizeof(ppir_discard_node),
               ppir_node_type type = ppir_op_infos[op].type;
   int size = node_size[type];
   ppir_node *node = rzalloc_size(block, size);
   if (!node)
            list_inithead(&node->succ_list);
            if (index >= 0) {
      if (mask) {
      /* reg has 4 slots for each component write node */
   while (mask)
            } else {
      comp->var_nodes[index] = node;
         }
   else
            node->op = op;
   node->type = type;
   node->index = comp->cur_index++;
               }
      void ppir_node_add_dep(ppir_node *succ, ppir_node *pred,
         {
      /* don't add dep for two nodes from different block */
   if (succ->block != pred->block) {
      pred->succ_different_block = true;
               /* don't add duplicated dep */
   ppir_node_foreach_pred(succ, dep) {
      if (dep->pred == pred)
               ppir_dep *dep = ralloc(succ, ppir_dep);
   dep->pred = pred;
   dep->succ = succ;
   dep->type = type;
   list_addtail(&dep->pred_link, &succ->pred_list);
      }
      void ppir_node_remove_dep(ppir_dep *dep)
   {
      list_del(&dep->succ_link);
   list_del(&dep->pred_link);
      }
      static void _ppir_node_replace_child(ppir_src *src, ppir_node *old_child, ppir_node *new_child)
   {
      ppir_dest *od = ppir_node_get_dest(old_child);
   if (ppir_node_target_equal(src, od)) {
            }
      void ppir_node_replace_child(ppir_node *parent, ppir_node *old_child, ppir_node *new_child)
   {
      switch (parent->type) {
   case ppir_node_type_alu:
   {
      ppir_alu_node *alu = ppir_node_to_alu(parent);
   for (int i = 0; i < alu->num_src; i++)
            }
   case ppir_node_type_branch:
   {
      ppir_branch_node *branch = ppir_node_to_branch(parent);
   for (int i = 0; i < 2; i++)
            }
   case ppir_node_type_load:
   {
      ppir_load_node *load = ppir_node_to_load(parent);
   _ppir_node_replace_child(&load->src, old_child, new_child);
      }
   case ppir_node_type_load_texture:
   {
      ppir_load_texture_node *load_texture = ppir_node_to_load_texture(parent);
   for (int i = 0; i < load_texture->num_src; i++)
            }
   case ppir_node_type_store:
   {
      ppir_store_node *store = ppir_node_to_store(parent);
   _ppir_node_replace_child(&store->src, old_child, new_child);
      }
   default:
      ppir_debug("unknown node type in %s\n", __func__);
         }
      void ppir_node_replace_pred(ppir_dep *dep, ppir_node *new_pred)
   {
      list_del(&dep->succ_link);
   dep->pred = new_pred;
      }
      ppir_dep *ppir_dep_for_pred(ppir_node *node, ppir_node *pred)
   {
      if (!pred)
            if (node->block != pred->block)
            ppir_node_foreach_pred(node, dep) {
      if (dep->pred == pred)
      }
      }
      void ppir_node_replace_all_succ(ppir_node *dst, ppir_node *src)
   {
      ppir_node_foreach_succ_safe(src, dep) {
      ppir_node_replace_pred(dep, dst);
         }
      void ppir_node_delete(ppir_node *node)
   {
      ppir_node_foreach_succ_safe(node, dep)
            ppir_node_foreach_pred_safe(node, dep)
            list_del(&node->list);
      }
      static void ppir_node_print_dest(ppir_dest *dest)
   {
      switch (dest->type) {
   case ppir_target_ssa:
      printf("ssa%d", dest->ssa.index);
      case ppir_target_pipeline:
      printf("pipeline %d", dest->pipeline);
      case ppir_target_register:
      printf("reg %d", dest->reg->index);
         }
      static void ppir_node_print_src(ppir_src *src)
   {
      switch (src->type) {
   case ppir_target_ssa: {
      if (src->node)
         else
            }
   case ppir_target_pipeline:
      if (src->node)
         else
            case ppir_target_register:
      printf("reg %d", src->reg->index);
         }
      static void ppir_node_print_node(ppir_node *node, int space)
   {
      for (int i = 0; i < space; i++)
            printf("%s%d: %s %s: ", node->printed && !ppir_node_is_leaf(node) ? "+" : "",
            ppir_dest *dest = ppir_node_get_dest(node);
   if (dest) {
      printf("dest: ");
               if (ppir_node_get_src_num(node) > 0) {
         }
   for (int i = 0; i < ppir_node_get_src_num(node); i++) {
      ppir_node_print_src(ppir_node_get_src(node, i));
   if (i != (ppir_node_get_src_num(node) - 1))
      }
            if (!node->printed) {
      ppir_node_foreach_pred(node, dep) {
      ppir_node *pred = dep->pred;
                     }
      void ppir_node_print_prog(ppir_compiler *comp)
   {
      if (!(lima_debug & LIMA_DEBUG_PP))
            list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_node, node, &block->node_list, list) {
                     printf("========prog========\n");
   list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      printf("-------block %3d-------\n", block->index);
   list_for_each_entry(ppir_node, node, &block->node_list, list) {
      if (ppir_node_is_root(node))
         }
      }
      static ppir_node *ppir_node_insert_mov_local(ppir_node *node)
   {
      ppir_node *move = ppir_node_create(node->block, ppir_op_mov, -1, 0);
   if (unlikely(!move))
            ppir_dest *dest = ppir_node_get_dest(node);
   ppir_alu_node *alu = ppir_node_to_alu(move);
   alu->dest = *dest;
   alu->num_src = 1;
            for (int s = 0; s < 4; s++)
            ppir_node_replace_all_succ(move, node);
   ppir_node_add_dep(move, node, ppir_dep_src);
            if (node->is_out) {
      node->is_out = false;
                  }
      ppir_node *ppir_node_insert_mov(ppir_node *old)
   {
      ppir_node *move = ppir_node_insert_mov_local(old);
            list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      if (old->block == block)
         list_for_each_entry_safe(ppir_node, node, &block->node_list, list) {
      for (int i = 0; i < ppir_node_get_src_num(node); i++){
      ppir_src *src = ppir_node_get_src(node, i);
   if (!src)
         if (src->node == old)
                        }
      bool ppir_node_has_single_src_succ(ppir_node *node)
   {
      if (ppir_node_has_single_succ(node) &&
      list_first_entry(&node->succ_list,
               int cnt = 0;
   ppir_node_foreach_succ(node, dep) {
      if (dep->type != ppir_dep_src)
                        }
