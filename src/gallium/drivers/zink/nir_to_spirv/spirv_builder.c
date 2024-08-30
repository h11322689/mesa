   /*
   * Copyright 2018 Collabora Ltd.
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "spirv_builder.h"
      #include "util/macros.h"
   #include "util/set.h"
   #include "util/ralloc.h"
   #include "util/u_bitcast.h"
   #include "util/u_memory.h"
   #include "util/half_float.h"
   #include "util/hash_table.h"
   #define XXH_INLINE_ALL
   #include "util/xxhash.h"
   #include "vk_util.h"
      #include <stdbool.h>
   #include <inttypes.h>
   #include <string.h>
      static bool
   spirv_buffer_grow(struct spirv_buffer *b, void *mem_ctx, size_t needed)
   {
               uint32_t *new_words = reralloc_size(mem_ctx, b->words,
         if (!new_words)
            b->words = new_words;
   b->room = new_room;
      }
      static inline bool
   spirv_buffer_prepare(struct spirv_buffer *b, void *mem_ctx, size_t needed)
   {
      needed += b->num_words;
   if (b->room >= b->num_words + needed)
               }
      static inline uint32_t
   spirv_buffer_emit_word(struct spirv_buffer *b, uint32_t word)
   {
      assert(b->num_words < b->room);
   b->words[b->num_words] = word;
      }
      static int
   spirv_buffer_emit_string(struct spirv_buffer *b, void *mem_ctx,
         {
      int pos = 0;
   uint32_t word = 0;
   while (str[pos] != '\0') {
      word |= str[pos] << (8 * (pos % 4));
   if (++pos % 4 == 0) {
      spirv_buffer_prepare(b, mem_ctx, 1);
   spirv_buffer_emit_word(b, word);
                  spirv_buffer_prepare(b, mem_ctx, 1);
               }
      void
   spirv_builder_emit_cap(struct spirv_builder *b, SpvCapability cap)
   {
      if (!b->caps)
            assert(b->caps);
      }
      void
   spirv_builder_emit_extension(struct spirv_builder *b, const char *name)
   {
      size_t pos = b->extensions.num_words;
   spirv_buffer_prepare(&b->extensions, b->mem_ctx, 1);
   spirv_buffer_emit_word(&b->extensions, SpvOpExtension);
   int len = spirv_buffer_emit_string(&b->extensions, b->mem_ctx, name);
      }
      void
   spirv_builder_emit_source(struct spirv_builder *b, SpvSourceLanguage lang,
         {
      spirv_buffer_prepare(&b->debug_names, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->debug_names, SpvOpSource | (3 << 16));
   spirv_buffer_emit_word(&b->debug_names, lang);
      }
      void
   spirv_builder_emit_mem_model(struct spirv_builder *b,
               {
      spirv_buffer_prepare(&b->memory_model, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->memory_model, SpvOpMemoryModel | (3 << 16));
   spirv_buffer_emit_word(&b->memory_model, addr_model);
      }
      void
   spirv_builder_emit_entry_point(struct spirv_builder *b,
                     {
      size_t pos = b->entry_points.num_words;
   spirv_buffer_prepare(&b->entry_points, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->entry_points, SpvOpEntryPoint);
   spirv_buffer_emit_word(&b->entry_points, exec_model);
   spirv_buffer_emit_word(&b->entry_points, entry_point);
   int len = spirv_buffer_emit_string(&b->entry_points, b->mem_ctx, name);
   b->entry_points.words[pos] |= (3 + len + num_interfaces) << 16;
   spirv_buffer_prepare(&b->entry_points, b->mem_ctx, num_interfaces);
   for (int i = 0; i < num_interfaces; ++i)
      }
      uint32_t
   spirv_builder_emit_exec_mode_literal(struct spirv_builder *b, SpvId entry_point,
         {
      spirv_buffer_prepare(&b->exec_modes, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->exec_modes, SpvOpExecutionMode | (4 << 16));
   spirv_buffer_emit_word(&b->exec_modes, entry_point);
   spirv_buffer_emit_word(&b->exec_modes, exec_mode);
      }
      void
   spirv_builder_emit_exec_mode_literal3(struct spirv_builder *b, SpvId entry_point,
         {
      spirv_buffer_prepare(&b->exec_modes, b->mem_ctx, 6);
   spirv_buffer_emit_word(&b->exec_modes, SpvOpExecutionMode | (6 << 16));
   spirv_buffer_emit_word(&b->exec_modes, entry_point);
   spirv_buffer_emit_word(&b->exec_modes, exec_mode);
   for (unsigned i = 0; i < 3; i++)
      }
      void
   spirv_builder_emit_exec_mode_id3(struct spirv_builder *b, SpvId entry_point,
         {
      spirv_buffer_prepare(&b->exec_modes, b->mem_ctx, 6);
   spirv_buffer_emit_word(&b->exec_modes, SpvOpExecutionModeId | (6 << 16));
   spirv_buffer_emit_word(&b->exec_modes, entry_point);
   spirv_buffer_emit_word(&b->exec_modes, exec_mode);
   for (unsigned i = 0; i < 3; i++)
      }
      void
   spirv_builder_emit_exec_mode(struct spirv_builder *b, SpvId entry_point,
         {
      spirv_buffer_prepare(&b->exec_modes, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->exec_modes, SpvOpExecutionMode | (3 << 16));
   spirv_buffer_emit_word(&b->exec_modes, entry_point);
      }
      void
   spirv_builder_emit_name(struct spirv_builder *b, SpvId target,
         {
      size_t pos = b->debug_names.num_words;
   spirv_buffer_prepare(&b->debug_names, b->mem_ctx, 2);
   spirv_buffer_emit_word(&b->debug_names, SpvOpName);
   spirv_buffer_emit_word(&b->debug_names, target);
   int len = spirv_buffer_emit_string(&b->debug_names, b->mem_ctx, name);
      }
      static void
   emit_decoration(struct spirv_builder *b, SpvId target,
               {
      int words = 3 + num_extra_operands;
   spirv_buffer_prepare(&b->decorations, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->decorations, SpvOpDecorate | (words << 16));
   spirv_buffer_emit_word(&b->decorations, target);
   spirv_buffer_emit_word(&b->decorations, decoration);
   for (int i = 0; i < num_extra_operands; ++i)
      }
      void
   spirv_builder_emit_decoration(struct spirv_builder *b, SpvId target,
         {
         }
      void
   spirv_builder_emit_rounding_mode(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { rounding };
      }
      void
   spirv_builder_emit_input_attachment_index(struct spirv_builder *b, SpvId target, uint32_t id)
   {
      uint32_t args[] = { id };
      }
      void
   spirv_builder_emit_specid(struct spirv_builder *b, SpvId target, uint32_t id)
   {
      uint32_t args[] = { id };
      }
      void
   spirv_builder_emit_location(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { location };
      }
      void
   spirv_builder_emit_component(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { component };
      }
      void
   spirv_builder_emit_builtin(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { builtin };
      }
      void
   spirv_builder_emit_vertex(struct spirv_builder *b, uint32_t stream, bool multistream)
   {
      unsigned words = 1;
   SpvOp op = SpvOpEmitVertex;
   if (multistream) {
      op = SpvOpEmitStreamVertex;
      }
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, op | (words << 16));
   if (multistream)
      }
      void
   spirv_builder_end_primitive(struct spirv_builder *b, uint32_t stream, bool multistream)
   {
      unsigned words = 1;
   SpvOp op = SpvOpEndPrimitive;
   if (multistream || stream > 0) {
      op = SpvOpEndStreamPrimitive;
      }
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, op | (words << 16));
   if (multistream || stream > 0)
      }
      void
   spirv_builder_emit_descriptor_set(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { descriptor_set };
   emit_decoration(b, target, SpvDecorationDescriptorSet, args,
      }
      void
   spirv_builder_emit_binding(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { binding };
      }
      void
   spirv_builder_emit_array_stride(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { stride };
      }
      void
   spirv_builder_emit_offset(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { offset };
      }
      void
   spirv_builder_emit_xfb_buffer(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { buffer };
      }
      void
   spirv_builder_emit_xfb_stride(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { stride };
      }
      void
   spirv_builder_emit_index(struct spirv_builder *b, SpvId target, int index)
   {
      uint32_t args[] = { index };
      }
      void
   spirv_builder_emit_stream(struct spirv_builder *b, SpvId target, int stream)
   {
      uint32_t args[] = { stream };
      }
      static void
   emit_member_decoration(struct spirv_builder *b, SpvId target, uint32_t member,
               {
      int words = 4 + num_extra_operands;
   spirv_buffer_prepare(&b->decorations, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->decorations,
         spirv_buffer_emit_word(&b->decorations, target);
   spirv_buffer_emit_word(&b->decorations, member);
   spirv_buffer_emit_word(&b->decorations, decoration);
   for (int i = 0; i < num_extra_operands; ++i)
      }
      void
   spirv_builder_emit_member_offset(struct spirv_builder *b, SpvId target,
         {
      uint32_t args[] = { offset };
   emit_member_decoration(b, target, member, SpvDecorationOffset,
      }
      SpvId
   spirv_builder_emit_undef(struct spirv_builder *b, SpvId result_type)
   {
      SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->instructions, SpvOpUndef | (3 << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
      }
      void
   spirv_builder_function(struct spirv_builder *b, SpvId result,
                     {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 5);
   spirv_buffer_emit_word(&b->instructions, SpvOpFunction | (5 << 16));
   spirv_buffer_emit_word(&b->instructions, return_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, function_control);
      }
      void
   spirv_builder_function_end(struct spirv_builder *b)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 1);
      }
      SpvId
   spirv_builder_function_call(struct spirv_builder *b, SpvId result_type,
               {
               int words = 4 + num_arguments;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions,
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
            for (int i = 0; i < num_arguments; ++i)
               }
         void
   spirv_builder_label(struct spirv_builder *b, SpvId label)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 2);
   spirv_buffer_emit_word(&b->instructions, SpvOpLabel | (2 << 16));
      }
      void
   spirv_builder_return(struct spirv_builder *b)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 1);
      }
      SpvId
   spirv_builder_emit_load(struct spirv_builder *b, SpvId result_type,
         {
         }
      SpvId
   spirv_builder_emit_load_aligned(struct spirv_builder *b, SpvId result_type, SpvId pointer, unsigned alignment)
   {
         }
      void
   spirv_builder_emit_store(struct spirv_builder *b, SpvId pointer, SpvId object)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->instructions, SpvOpStore | (3 << 16));
   spirv_buffer_emit_word(&b->instructions, pointer);
      }
      void
   spirv_builder_emit_store_aligned(struct spirv_builder *b, SpvId pointer, SpvId object, unsigned alignment)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 5);
   spirv_buffer_emit_word(&b->instructions, SpvOpStore | (5 << 16));
   spirv_buffer_emit_word(&b->instructions, pointer);
   spirv_buffer_emit_word(&b->instructions, object);
   spirv_buffer_emit_word(&b->instructions, SpvMemoryAccessAlignedMask);
      }
      void
   spirv_builder_emit_atomic_store(struct spirv_builder *b, SpvId pointer, SpvScope scope,
         {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 5);
   spirv_buffer_emit_word(&b->instructions, SpvOpAtomicStore | (5 << 16));
   spirv_buffer_emit_word(&b->instructions, pointer);
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, scope));
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, semantics));
      }
      SpvId
   spirv_builder_emit_access_chain(struct spirv_builder *b, SpvId result_type,
               {
      assert(base);
   assert(result_type);
            int words = 4 + num_indexes;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpAccessChain | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, base);
   for (int i = 0; i < num_indexes; ++i) {
      assert(indexes[i]);
      }
      }
      void
   spirv_builder_emit_interlock(struct spirv_builder *b, bool end)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 1);
      }
         SpvId
   spirv_builder_emit_unop_const(struct spirv_builder *b, SpvOp op, SpvId result_type, uint64_t operand)
   {
      SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->instructions, op | (4 << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, operand));
      }
      SpvId
   spirv_builder_emit_unop(struct spirv_builder *b, SpvOp op, SpvId result_type,
         {
      struct spirv_buffer *buf = op == SpvOpSpecConstant ? &b->types_const_defs : &b->instructions;
   SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(buf, b->mem_ctx, 4);
   spirv_buffer_emit_word(buf, op | (4 << 16));
   spirv_buffer_emit_word(buf, result_type);
   spirv_buffer_emit_word(buf, result);
   spirv_buffer_emit_word(buf, operand);
      }
      SpvId
   spirv_builder_emit_binop(struct spirv_builder *b, SpvOp op, SpvId result_type,
         {
      SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, 5);
   spirv_buffer_emit_word(&b->instructions, op | (5 << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, operand0);
   spirv_buffer_emit_word(&b->instructions, operand1);
      }
      SpvId
   spirv_builder_emit_triop(struct spirv_builder *b, SpvOp op, SpvId result_type,
         {
               SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(buf, b->mem_ctx, 6);
   spirv_buffer_emit_word(buf, op | (6 << 16));
   spirv_buffer_emit_word(buf, result_type);
   spirv_buffer_emit_word(buf, result);
   spirv_buffer_emit_word(buf, operand0);
   spirv_buffer_emit_word(buf, operand1);
   spirv_buffer_emit_word(buf, operand2);
      }
      SpvId
   spirv_builder_emit_quadop(struct spirv_builder *b, SpvOp op, SpvId result_type,
         {
               SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(buf, b->mem_ctx, 7);
   spirv_buffer_emit_word(buf, op | (7 << 16));
   spirv_buffer_emit_word(buf, result_type);
   spirv_buffer_emit_word(buf, result);
   spirv_buffer_emit_word(buf, operand0);
   spirv_buffer_emit_word(buf, operand1);
   spirv_buffer_emit_word(buf, operand2);
   spirv_buffer_emit_word(buf, operand3);
      }
      SpvId
   spirv_builder_emit_hexop(struct spirv_builder *b, SpvOp op, SpvId result_type,
               {
               SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(buf, b->mem_ctx, 9);
   spirv_buffer_emit_word(buf, op | (9 << 16));
   spirv_buffer_emit_word(buf, result_type);
   spirv_buffer_emit_word(buf, result);
   spirv_buffer_emit_word(buf, operand0);
   spirv_buffer_emit_word(buf, operand1);
   spirv_buffer_emit_word(buf, operand2);
   spirv_buffer_emit_word(buf, operand3);
   spirv_buffer_emit_word(buf, operand4);
   spirv_buffer_emit_word(buf, operand5);
      }
      SpvId
   spirv_builder_emit_composite_extract(struct spirv_builder *b, SpvId result_type,
               {
               assert(num_indexes > 0);
   int words = 4 + num_indexes;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions,
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, composite);
   for (int i = 0; i < num_indexes; ++i)
            }
      SpvId
   spirv_builder_emit_composite_construct(struct spirv_builder *b,
                     {
               assert(num_constituents > 0);
   int words = 3 + num_constituents;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions,
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   for (int i = 0; i < num_constituents; ++i)
            }
      SpvId
   spirv_builder_emit_vector_shuffle(struct spirv_builder *b, SpvId result_type,
                     {
               assert(num_components > 0);
   int words = 5 + num_components;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpVectorShuffle | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, vector_1);
   spirv_buffer_emit_word(&b->instructions, vector_2);
   for (int i = 0; i < num_components; ++i)
            }
      SpvId
   spirv_builder_emit_vector_extract(struct spirv_builder *b, SpvId result_type,
               {
               int words = 5;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpVectorExtractDynamic | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, vector_1);
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, component));
      }
      SpvId
   spirv_builder_emit_vector_insert(struct spirv_builder *b, SpvId result_type,
                     {
               int words = 6;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpVectorInsertDynamic | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, vector_1);
   spirv_buffer_emit_word(&b->instructions, component);
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, index));
      }
      void
   spirv_builder_emit_branch(struct spirv_builder *b, SpvId label)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 2);
   spirv_buffer_emit_word(&b->instructions, SpvOpBranch | (2 << 16));
      }
      void
   spirv_builder_emit_selection_merge(struct spirv_builder *b, SpvId merge_block,
         {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->instructions, SpvOpSelectionMerge | (3 << 16));
   spirv_buffer_emit_word(&b->instructions, merge_block);
      }
      void
   spirv_builder_loop_merge(struct spirv_builder *b, SpvId merge_block,
         {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->instructions, SpvOpLoopMerge | (4 << 16));
   spirv_buffer_emit_word(&b->instructions, merge_block);
   spirv_buffer_emit_word(&b->instructions, cont_target);
      }
      void
   spirv_builder_emit_branch_conditional(struct spirv_builder *b, SpvId condition,
         {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->instructions, SpvOpBranchConditional | (4 << 16));
   spirv_buffer_emit_word(&b->instructions, condition);
   spirv_buffer_emit_word(&b->instructions, true_label);
      }
      SpvId
   spirv_builder_emit_phi(struct spirv_builder *b, SpvId result_type,
         {
               assert(num_vars > 0);
   int words = 3 + 2 * num_vars;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpPhi | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   *position = b->instructions.num_words;
   for (int i = 0; i < 2 * num_vars; ++i)
            }
      void
   spirv_builder_set_phi_operand(struct spirv_builder *b, size_t position,
         {
      b->instructions.words[position + index * 2 + 0] = variable;
      }
      void
   spirv_builder_emit_kill(struct spirv_builder *b)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 1);
      }
      void
   spirv_builder_emit_terminate(struct spirv_builder *b)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 1);
      }
      void
   spirv_builder_emit_demote(struct spirv_builder *b)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 1);
      }
      SpvId
   spirv_is_helper_invocation(struct spirv_builder *b)
   {
      SpvId result = spirv_builder_new_id(b);
            int words = 3;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpIsHelperInvocationEXT | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
      }
      SpvId
   spirv_builder_emit_vote(struct spirv_builder *b, SpvOp op, SpvId src)
   {
      return spirv_builder_emit_binop(b, op, spirv_builder_type_bool(b),
      }
      static SpvId
   sparse_wrap_result_type(struct spirv_builder *b, SpvId result_type)
   {
      SpvId types[2];
   types[0] = spirv_builder_type_uint(b, 32);
   types[1] = result_type;
      }
      SpvId
   spirv_builder_emit_image_sample(struct spirv_builder *b,
                                 SpvId result_type,
   SpvId sampled_image,
   SpvId coordinate,
   bool proj,
   SpvId lod,
   SpvId bias,
   SpvId dref,
   SpvId dx,
   {
               int operands = 5;
   int opcode;
   if (sparse) {
      opcode = SpvOpImageSparseSampleImplicitLod;
   if (proj)
         if (lod || (dx && dy))
         if (dref) {
      opcode += SpvOpImageSparseSampleDrefImplicitLod - SpvOpImageSparseSampleImplicitLod;
      }
      } else {
      opcode = SpvOpImageSampleImplicitLod;
   if (proj)
         if (lod || (dx && dy))
         if (dref) {
      opcode += SpvOpImageSampleDrefImplicitLod - SpvOpImageSampleImplicitLod;
                  SpvImageOperandsMask operand_mask = SpvImageOperandsMaskNone;
   SpvId extra_operands[6];
   int num_extra_operands = 1;
   if (bias) {
      extra_operands[num_extra_operands++] = bias;
      }
   if (lod) {
      extra_operands[num_extra_operands++] = lod;
      } else if (dx && dy) {
      extra_operands[num_extra_operands++] = dx;
   extra_operands[num_extra_operands++] = dy;
      }
   assert(!(const_offset && offset));
   if (const_offset) {
      extra_operands[num_extra_operands++] = const_offset;
      } else if (offset) {
      extra_operands[num_extra_operands++] = offset;
      }
   if (min_lod) {
      extra_operands[num_extra_operands++] = min_lod;
               /* finalize num_extra_operands / extra_operands */
            spirv_buffer_prepare(&b->instructions, b->mem_ctx, operands + num_extra_operands);
   spirv_buffer_emit_word(&b->instructions, opcode | ((operands + num_extra_operands) << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, sampled_image);
   spirv_buffer_emit_word(&b->instructions, coordinate);
   if (dref)
         for (int i = 0; i < num_extra_operands; ++i)
            }
      SpvId
   spirv_builder_emit_image(struct spirv_builder *b, SpvId result_type,
         {
      SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->instructions, SpvOpImage | (4 << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, sampled_image);
      }
      SpvId
   spirv_builder_emit_image_texel_pointer(struct spirv_builder *b,
                           {
      SpvId pointer_type = spirv_builder_type_pointer(b,
                  }
      SpvId
   spirv_builder_emit_image_read(struct spirv_builder *b,
                                 SpvId result_type,
   SpvId image,
   {
               SpvImageOperandsMask operand_mask = SpvImageOperandsMaskNone;
   SpvId extra_operands[5];
   int num_extra_operands = 1;
   if (sparse)
         if (lod) {
      extra_operands[num_extra_operands++] = lod;
      }
   if (sample) {
      extra_operands[num_extra_operands++] = sample;
      }
   if (offset) {
      extra_operands[num_extra_operands++] = offset;
      }
   /* finalize num_extra_operands / extra_operands */
            spirv_buffer_prepare(&b->instructions, b->mem_ctx, 5 + num_extra_operands);
   spirv_buffer_emit_word(&b->instructions, (sparse ? SpvOpImageSparseRead : SpvOpImageRead) |
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, image);
   spirv_buffer_emit_word(&b->instructions, coordinate);
   for (int i = 0; i < num_extra_operands; ++i)
            }
      void
   spirv_builder_emit_image_write(struct spirv_builder *b,
                                 SpvId image,
   {
      SpvImageOperandsMask operand_mask = SpvImageOperandsMaskNone;
   SpvId extra_operands[5];
   int num_extra_operands = 1;
   if (lod) {
      extra_operands[num_extra_operands++] = lod;
      }
   if (sample) {
      extra_operands[num_extra_operands++] = sample;
      }
   if (offset) {
      extra_operands[num_extra_operands++] = offset;
      }
   /* finalize num_extra_operands / extra_operands */
            spirv_buffer_prepare(&b->instructions, b->mem_ctx, 4 + num_extra_operands);
   spirv_buffer_emit_word(&b->instructions, SpvOpImageWrite |
         spirv_buffer_emit_word(&b->instructions, image);
   spirv_buffer_emit_word(&b->instructions, coordinate);
   spirv_buffer_emit_word(&b->instructions, texel);
   for (int i = 0; i < num_extra_operands; ++i)
      }
      SpvId
   spirv_builder_emit_image_gather(struct spirv_builder *b,
                                 SpvId result_type,
   SpvId image,
   SpvId coordinate,
   SpvId component,
   SpvId lod,
   {
      SpvId result = spirv_builder_new_id(b);
            SpvImageOperandsMask operand_mask = SpvImageOperandsMaskNone;
   SpvId extra_operands[4];
   int num_extra_operands = 1;
   if (lod) {
      extra_operands[num_extra_operands++] = lod;
      }
   if (sample) {
      extra_operands[num_extra_operands++] = sample;
      }
   assert(!(const_offset && offset));
   if (const_offset) {
      extra_operands[num_extra_operands++] = const_offset;
      } else if (offset) {
      extra_operands[num_extra_operands++] = offset;
      }
   if (dref)
         if (sparse)
         /* finalize num_extra_operands / extra_operands */
            spirv_buffer_prepare(&b->instructions, b->mem_ctx, 6 + num_extra_operands);
   spirv_buffer_emit_word(&b->instructions, op |
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, image);
   spirv_buffer_emit_word(&b->instructions, coordinate);
   if (dref)
         else
         for (int i = 0; i < num_extra_operands; ++i)
            }
      SpvId
   spirv_builder_emit_image_fetch(struct spirv_builder *b,
                                 SpvId result_type,
   SpvId image,
   SpvId coordinate,
   {
               SpvImageOperandsMask operand_mask = SpvImageOperandsMaskNone;
   SpvId extra_operands[4];
   int num_extra_operands = 1;
   if (lod) {
      extra_operands[num_extra_operands++] = lod;
      }
   if (sample) {
      extra_operands[num_extra_operands++] = sample;
      }
   assert(!(const_offset && offset));
   if (const_offset) {
      extra_operands[num_extra_operands++] = const_offset;
      } else if (offset) {
      extra_operands[num_extra_operands++] = offset;
      }
   if (sparse)
            /* finalize num_extra_operands / extra_operands */
            spirv_buffer_prepare(&b->instructions, b->mem_ctx, 5 + num_extra_operands);
   spirv_buffer_emit_word(&b->instructions, (sparse ? SpvOpImageSparseFetch : SpvOpImageFetch) |
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, image);
   spirv_buffer_emit_word(&b->instructions, coordinate);
   for (int i = 0; i < num_extra_operands; ++i)
            }
      SpvId
   spirv_builder_emit_image_query_size(struct spirv_builder *b,
                     {
      int opcode = SpvOpImageQuerySize;
   int words = 4;
   if (lod) {
      words++;
               SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, opcode | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
            if (lod)
               }
      SpvId
   spirv_builder_emit_image_query_levels(struct spirv_builder *b,
               {
         }
      SpvId
   spirv_builder_emit_image_query_lod(struct spirv_builder *b,
                     {
      int opcode = SpvOpImageQueryLod;
            SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, opcode | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, image);
               }
      SpvId
   spirv_builder_emit_ext_inst(struct spirv_builder *b, SpvId result_type,
               {
               int words = 5 + num_args;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions, SpvOpExtInst | (words << 16));
   spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   spirv_buffer_emit_word(&b->instructions, set);
   spirv_buffer_emit_word(&b->instructions, instruction);
   for (int i = 0; i < num_args; ++i)
            }
      struct spirv_type {
      SpvOp op;
   uint32_t args[8];
               };
      static uint32_t
   non_aggregate_type_hash(const void *arg)
   {
               uint32_t hash = 0;
   hash = XXH32(&type->op, sizeof(type->op), hash);
   hash = XXH32(type->args, sizeof(uint32_t) * type->num_args, hash);
      }
      static bool
   non_aggregate_type_equals(const void *a, const void *b)
   {
               if (ta->op != tb->op)
            assert(ta->num_args == tb->num_args);
      }
      static SpvId
   get_type_def(struct spirv_builder *b, SpvOp op, const uint32_t args[],
         {
      /* According to the SPIR-V specification:
   *
   *   "Two different type <id>s form, by definition, two different types. It
   *    is valid to declare multiple aggregate type <id>s having the same
   *    opcode and operands. This is to allow multiple instances of aggregate
   *    types with the same structure to be decorated differently. (Different
   *    decorations are not required; two different aggregate type <id>s are
   *    allowed to have identical declarations and decorations, and will still
   *    be two different types.) Non-aggregate types are different: It is
   *    invalid to declare multiple type <id>s for the same scalar, vector, or
   *    matrix type. That is, non-aggregate type declarations must all have
   *    different opcodes or operands. (Note that non-aggregate types cannot
   *    be decorated in ways that affect their type.)"
   *
   *  ..so, we need to prevent the same non-aggregate type to be re-defined
   *  with a new <id>. We do this by putting the definitions in a hash-map, so
   *  we can easily look up and reuse them.
            struct spirv_type key;
   assert(num_args <= ARRAY_SIZE(key.args));
   key.op = op;
   memcpy(&key.args, args, sizeof(uint32_t) * num_args);
            struct hash_entry *entry;
   if (b->types) {
      entry = _mesa_hash_table_search(b->types, &key);
   if (entry)
      } else {
      b->types = _mesa_hash_table_create(b->mem_ctx,
                           struct spirv_type *type = rzalloc(b->mem_ctx, struct spirv_type);
   if (!type)
            type->op = op;
   memcpy(&type->args, args, sizeof(uint32_t) * num_args);
            type->type = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, 2 + num_args);
   spirv_buffer_emit_word(&b->types_const_defs, op | ((2 + num_args) << 16));
   spirv_buffer_emit_word(&b->types_const_defs, type->type);
   for (int i = 0; i < num_args; ++i)
            entry = _mesa_hash_table_insert(b->types, type, type);
               }
      SpvId
   spirv_builder_type_void(struct spirv_builder *b)
   {
         }
      SpvId
   spirv_builder_type_bool(struct spirv_builder *b)
   {
         }
      SpvId
   spirv_builder_type_int(struct spirv_builder *b, unsigned width)
   {
      uint32_t args[] = { width, 1 };
   if (width == 8)
         else if (width == 16)
         else if (width == 64)
            }
      SpvId
   spirv_builder_type_uint(struct spirv_builder *b, unsigned width)
   {
      uint32_t args[] = { width, 0 };
   if (width == 8)
         else if (width == 16)
         else if (width == 64)
            }
      SpvId
   spirv_builder_type_float(struct spirv_builder *b, unsigned width)
   {
      uint32_t args[] = { width };
   if (width == 16)
         else if (width == 64)
            }
      SpvId
   spirv_builder_type_image(struct spirv_builder *b, SpvId sampled_type,
               {
      assert(sampled < 3);
   uint32_t args[] = {
      sampled_type, dim, depth ? 1 : 0, arrayed ? 1 : 0, ms ? 1 : 0, sampled,
      };
   if (sampled == 2 && ms && dim != SpvDimSubpassData)
            }
      SpvId
   spirv_builder_emit_sampled_image(struct spirv_builder *b, SpvId result_type, SpvId image, SpvId sampler)
   {
         }
      SpvId
   spirv_builder_type_sampled_image(struct spirv_builder *b, SpvId image_type)
   {
      uint32_t args[] = { image_type };
      }
      SpvId
   spirv_builder_type_sampler(struct spirv_builder *b)
   {
      uint32_t args[1] = {0};
      }
      SpvId
   spirv_builder_type_pointer(struct spirv_builder *b,
         {
      uint32_t args[] = { storage_class, type };
      }
      SpvId
   spirv_builder_type_vector(struct spirv_builder *b, SpvId component_type,
         {
      assert(component_count > 1);
   uint32_t args[] = { component_type, component_count };
      }
      SpvId
   spirv_builder_type_matrix(struct spirv_builder *b, SpvId component_type,
         {
      assert(component_count > 1);
   uint32_t args[] = { component_type, component_count };
      }
      SpvId
   spirv_builder_type_runtime_array(struct spirv_builder *b, SpvId component_type)
   {
      SpvId type = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->types_const_defs, SpvOpTypeRuntimeArray | (3 << 16));
   spirv_buffer_emit_word(&b->types_const_defs, type);
   spirv_buffer_emit_word(&b->types_const_defs, component_type);
      }
      SpvId
   spirv_builder_type_array(struct spirv_builder *b, SpvId component_type,
         {
      SpvId type = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->types_const_defs, SpvOpTypeArray | (4 << 16));
   spirv_buffer_emit_word(&b->types_const_defs, type);
   spirv_buffer_emit_word(&b->types_const_defs, component_type);
   spirv_buffer_emit_word(&b->types_const_defs, length);
      }
      SpvId
   spirv_builder_type_struct(struct spirv_builder *b, const SpvId member_types[],
         {
      int words = 2 + num_member_types;
   SpvId type = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->types_const_defs, SpvOpTypeStruct | (words << 16));
   spirv_buffer_emit_word(&b->types_const_defs, type);
   for (int i = 0; i < num_member_types; ++i)
            }
      SpvId
   spirv_builder_type_function(struct spirv_builder *b, SpvId return_type,
               {
      int words = 3 + num_parameter_types;
   SpvId type = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->types_const_defs, SpvOpTypeFunction | (words << 16));
   spirv_buffer_emit_word(&b->types_const_defs, type);
   spirv_buffer_emit_word(&b->types_const_defs, return_type);
   for (int i = 0; i < num_parameter_types; ++i)
            }
      struct spirv_const {
      SpvOp op, type;
   uint32_t args[8];
               };
      static uint32_t
   const_hash(const void *arg)
   {
               uint32_t hash = 0;
   hash = XXH32(&key->op, sizeof(key->op), hash);
   hash = XXH32(&key->type, sizeof(key->type), hash);
   hash = XXH32(key->args, sizeof(uint32_t) * key->num_args, hash);
      }
      static bool
   const_equals(const void *a, const void *b)
   {
               if (ca->op != cb->op ||
      ca->type != cb->type)
         assert(ca->num_args == cb->num_args);
      }
      static SpvId
   get_const_def(struct spirv_builder *b, SpvOp op, SpvId type,
         {
      struct spirv_const key;
   assert(num_args <= ARRAY_SIZE(key.args));
   key.op = op;
   key.type = type;
   memcpy(&key.args, args, sizeof(uint32_t) * num_args);
            struct hash_entry *entry;
   if (b->consts) {
      entry = _mesa_hash_table_search(b->consts, &key);
   if (entry)
      } else {
      b->consts = _mesa_hash_table_create(b->mem_ctx, const_hash,
                     struct spirv_const *cnst = rzalloc(b->mem_ctx, struct spirv_const);
   if (!cnst)
            cnst->op = op;
   cnst->type = type;
   memcpy(&cnst->args, args, sizeof(uint32_t) * num_args);
            cnst->result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, 3 + num_args);
   spirv_buffer_emit_word(&b->types_const_defs, op | ((3 + num_args) << 16));
   spirv_buffer_emit_word(&b->types_const_defs, type);
   spirv_buffer_emit_word(&b->types_const_defs, cnst->result);
   for (int i = 0; i < num_args; ++i)
            entry = _mesa_hash_table_insert(b->consts, cnst, cnst);
               }
      static SpvId
   emit_constant_32(struct spirv_builder *b, SpvId type, uint32_t val)
   {
      uint32_t args[] = { val };
      }
      static SpvId
   emit_constant_64(struct spirv_builder *b, SpvId type, uint64_t val)
   {
      uint32_t args[] = { val & UINT32_MAX, val >> 32 };
      }
      SpvId
   spirv_builder_const_bool(struct spirv_builder *b, bool val)
   {
      return get_const_def(b, val ? SpvOpConstantTrue : SpvOpConstantFalse,
      }
      SpvId
   spirv_builder_const_int(struct spirv_builder *b, int width, int64_t val)
   {
      assert(width >= 8);
   SpvId type = spirv_builder_type_int(b, width);
   if (width <= 32)
         else
      }
      SpvId
   spirv_builder_const_uint(struct spirv_builder *b, int width, uint64_t val)
   {
      assert(width >= 8);
   if (width == 8)
         else if (width == 16)
         else if (width == 64)
         SpvId type = spirv_builder_type_uint(b, width);
   if (width <= 32)
         else
      }
      SpvId
   spirv_builder_spec_const_uint(struct spirv_builder *b, int width)
   {
      assert(width <= 32);
   SpvId const_type = spirv_builder_type_uint(b, width);
   SpvId result = spirv_builder_new_id(b);
   spirv_buffer_prepare(&b->types_const_defs, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->types_const_defs, SpvOpSpecConstant | (4 << 16));
   spirv_buffer_emit_word(&b->types_const_defs, const_type);
   spirv_buffer_emit_word(&b->types_const_defs, result);
   /* this is the default value for spec constants;
   * if any users need a different default, add a param to pass for it
   */
   spirv_buffer_emit_word(&b->types_const_defs, 1);
      }
      SpvId
   spirv_builder_const_float(struct spirv_builder *b, int width, double val)
   {
      assert(width >= 16);
   SpvId type = spirv_builder_type_float(b, width);
   if (width == 16) {
      spirv_builder_emit_cap(b, SpvCapabilityFloat16);
      } else if (width == 32)
         else if (width == 64) {
      spirv_builder_emit_cap(b, SpvCapabilityFloat64);
                  }
      SpvId
   spirv_builder_const_composite(struct spirv_builder *b, SpvId result_type,
               {
      return get_const_def(b, SpvOpConstantComposite, result_type,
            }
      SpvId
   spirv_builder_spec_const_composite(struct spirv_builder *b, SpvId result_type,
               {
               assert(num_constituents > 0);
   int words = 3 + num_constituents;
   spirv_buffer_prepare(&b->instructions, b->mem_ctx, words);
   spirv_buffer_emit_word(&b->instructions,
         spirv_buffer_emit_word(&b->instructions, result_type);
   spirv_buffer_emit_word(&b->instructions, result);
   for (int i = 0; i < num_constituents; ++i)
            }
      SpvId
   spirv_builder_emit_var(struct spirv_builder *b, SpvId type,
         {
      assert(storage_class != SpvStorageClassGeneric);
   struct spirv_buffer *buf = storage_class != SpvStorageClassFunction ?
            SpvId ret = spirv_builder_new_id(b);
   spirv_buffer_prepare(buf, b->mem_ctx, 4);
   spirv_buffer_emit_word(buf, SpvOpVariable | (4 << 16));
   spirv_buffer_emit_word(buf, type);
   spirv_buffer_emit_word(buf, ret);
   spirv_buffer_emit_word(buf, storage_class);
      }
      void
   spirv_builder_emit_memory_barrier(struct spirv_builder *b, SpvScope scope, SpvMemorySemanticsMask semantics)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 3);
   spirv_buffer_emit_word(&b->instructions, SpvOpMemoryBarrier | (3 << 16));
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, scope));
      }
      void
   spirv_builder_emit_control_barrier(struct spirv_builder *b, SpvScope scope, SpvScope mem_scope, SpvMemorySemanticsMask semantics)
   {
      spirv_buffer_prepare(&b->instructions, b->mem_ctx, 4);
   spirv_buffer_emit_word(&b->instructions, SpvOpControlBarrier | (4 << 16));
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, scope));
   spirv_buffer_emit_word(&b->instructions, spirv_builder_const_uint(b, 32, mem_scope));
      }
      SpvId
   spirv_builder_import(struct spirv_builder *b, const char *name)
   {
      SpvId result = spirv_builder_new_id(b);
   size_t pos = b->imports.num_words;
   spirv_buffer_prepare(&b->imports, b->mem_ctx, 2);
   spirv_buffer_emit_word(&b->imports, SpvOpExtInstImport);
   spirv_buffer_emit_word(&b->imports, result);
   int len = spirv_buffer_emit_string(&b->imports, b->mem_ctx, name);
   b->imports.words[pos] |= (2 + len) << 16;
      }
      size_t
   spirv_builder_get_num_words(struct spirv_builder *b)
   {
      const size_t header_size = 5;
   const size_t caps_size = b->caps ? b->caps->entries * 2 : 0;
   return header_size + caps_size +
         b->extensions.num_words +
   b->imports.num_words +
   b->memory_model.num_words +
   b->entry_points.num_words +
   b->exec_modes.num_words +
   b->debug_names.num_words +
   b->decorations.num_words +
   b->types_const_defs.num_words +
      }
      size_t
   spirv_builder_get_words(struct spirv_builder *b, uint32_t *words,
               {
               size_t written  = 0;
   words[written++] = SpvMagicNumber;
   words[written++] = spirv_version;
   words[written++] = 0;
   words[written++] = b->prev_id + 1;
            if (b->caps) {
      set_foreach(b->caps, entry) {
      words[written++] = SpvOpCapability | (2 << 16);
                  const struct spirv_buffer *buffers[] = {
      &b->extensions,
   &b->imports,
   &b->memory_model,
   &b->entry_points,
   &b->exec_modes,
   &b->debug_names,
   &b->decorations,
               for (int i = 0; i < ARRAY_SIZE(buffers); ++i) {
               if (buffer == &b->exec_modes && *tcs_vertices_out_word > 0)
            memcpy(words + written, buffer->words,
            }
   typed_memcpy(&words[written], b->instructions.words, b->local_vars_begin);
   written += b->local_vars_begin;
   typed_memcpy(&words[written], b->local_vars.words, b->local_vars.num_words);
   written += b->local_vars.num_words;
   typed_memcpy(&words[written], &b->instructions.words[b->local_vars_begin], (b->instructions.num_words - b->local_vars_begin));
            assert(written == spirv_builder_get_num_words(b));
      }
      void
   spirv_builder_begin_local_vars(struct spirv_builder *b)
   {
         }
