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
      #include "rogue.h"
   #include "rogue_builder.h"
   #include "util/macros.h"
      /**
   * \file rogue_builder.c
   *
   * \brief Contains helper functions for building Rogue shaders.
   */
      /**
   * \brief Inserts an instruction at the current builder context position.
   *
   * \param[in] b The builder context.
   * \param[in] instr The instruction to insert.
   */
   static inline void rogue_builder_insert_instr(rogue_builder *b,
         {
      rogue_instr_insert(instr, b->cursor);
      }
      /* ALU instructions */
      static inline rogue_alu_instr *rogue_build_alu(rogue_builder *b,
                                 {
      rogue_alu_instr *alu =
            for (unsigned i = 0; i < num_dsts; ++i) {
      alu->dst[i].ref = dsts[i];
               for (unsigned i = 0; i < num_srcs; ++i) {
      alu->src[i].ref = srcs[i];
               rogue_builder_insert_instr(b, &alu->instr);
      }
      static inline rogue_alu_instr *rogue_build_alu11(rogue_builder *b,
                     {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0 };
      }
      static inline rogue_alu_instr *rogue_build_alu12(rogue_builder *b,
                           {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0, src1 };
      }
      static inline rogue_alu_instr *rogue_build_alu13(rogue_builder *b,
                                 {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0, src1, src2 };
      }
      static inline rogue_alu_instr *rogue_build_alu22(rogue_builder *b,
                                 {
      rogue_ref dsts[] = { dst0, dst1 };
   rogue_ref srcs[] = { src0, src1 };
      }
      static inline rogue_alu_instr *rogue_build_alu23(rogue_builder *b,
                                       {
      rogue_ref dsts[] = { dst0, dst1 };
   rogue_ref srcs[] = { src0, src1, src2 };
      }
      static inline rogue_alu_instr *rogue_build_alu35(rogue_builder *b,
                                                   enum rogue_alu_op op,
   {
      rogue_ref dsts[] = { dst0, dst1, dst2 };
   rogue_ref srcs[] = { src0, src1, src2, src3, src4 };
      }
      /* TODO: Static inline in rogue.h? */
   #define ROGUE_BUILDER_DEFINE_ALU11(op)                             \
      PUBLIC                                                          \
   rogue_alu_instr *rogue_##op(rogue_builder *b,                   \
               {                                                               \
      assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_dsts == 1); \
   assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_srcs == 1); \
            #define ROGUE_BUILDER_DEFINE_ALU12(op)                                  \
      PUBLIC                                                               \
   rogue_alu_instr *rogue_##op(rogue_builder *b,                        \
                     {                                                                    \
      assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_dsts == 1);      \
   assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_srcs == 2);      \
            #define ROGUE_BUILDER_DEFINE_ALU13(op)                                        \
      PUBLIC                                                                     \
   rogue_alu_instr *rogue_##op(rogue_builder *b,                              \
                           {                                                                          \
      assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_dsts == 1);            \
   assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_srcs == 3);            \
            #define ROGUE_BUILDER_DEFINE_ALU22(op)                                        \
      PUBLIC                                                                     \
   rogue_alu_instr *rogue_##op(rogue_builder *b,                              \
                           {                                                                          \
      assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_dsts == 2);            \
   assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_srcs == 2);            \
            #define ROGUE_BUILDER_DEFINE_ALU23(op)                             \
      PUBLIC                                                          \
   rogue_alu_instr *rogue_##op(rogue_builder *b,                   \
                                 {                                                               \
      assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_dsts == 2); \
   assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_srcs == 3); \
   return rogue_build_alu23(b,                                  \
                           ROGUE_ALU_OP_##op,                  \
            #define ROGUE_BUILDER_DEFINE_ALU35(op)                             \
      PUBLIC                                                          \
   rogue_alu_instr *rogue_##op(rogue_builder *b,                   \
                              rogue_ref dst0,                     \
   rogue_ref dst1,                     \
   rogue_ref dst2,                     \
      {                                                               \
      assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_dsts == 3); \
   assert(rogue_alu_op_infos[ROGUE_ALU_OP_##op].num_srcs == 5); \
   return rogue_build_alu35(b,                                  \
                           ROGUE_ALU_OP_##op,                  \
   dst0,                               \
   dst1,                               \
   dst2,                               \
            #include "rogue_alu_instrs.def"
      static inline rogue_backend_instr *rogue_build_backend(rogue_builder *b,
                                 {
      rogue_backend_instr *backend =
            for (unsigned i = 0; i < num_dsts; ++i) {
      backend->dst[i].ref = dsts[i];
               for (unsigned i = 0; i < num_srcs; ++i) {
      backend->src[i].ref = srcs[i];
               rogue_builder_insert_instr(b, &backend->instr);
      }
      static inline rogue_backend_instr *
   rogue_build_backend00(rogue_builder *b, enum rogue_backend_op op)
   {
         }
      static inline rogue_backend_instr *
   rogue_build_backend02(rogue_builder *b,
                     {
      rogue_ref srcs[] = { src0, src1 };
      }
      static inline rogue_backend_instr *
   rogue_build_backend11(rogue_builder *b,
                     {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0 };
      }
      static inline rogue_backend_instr *
   rogue_build_backend13(rogue_builder *b,
                        enum rogue_backend_op op,
      {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0, src1, src2 };
      }
      static inline rogue_backend_instr *
   rogue_build_backend14(rogue_builder *b,
                        enum rogue_backend_op op,
   rogue_ref dst0,
      {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0, src1, src2, src3 };
      }
      static inline rogue_backend_instr *
   rogue_build_backend06(rogue_builder *b,
                        enum rogue_backend_op op,
   rogue_ref src0,
   rogue_ref src1,
      {
      rogue_ref srcs[] = { src0, src1, src2, src3, src4, src5 };
      }
      static inline rogue_backend_instr *
   rogue_build_backend16(rogue_builder *b,
                        enum rogue_backend_op op,
   rogue_ref dst0,
   rogue_ref src0,
   rogue_ref src1,
      {
      rogue_ref dsts[] = { dst0 };
   rogue_ref srcs[] = { src0, src1, src2, src3, src4, src5 };
      }
      #define ROGUE_BUILDER_DEFINE_BACKEND00(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b)                       \
   {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 0); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 0); \
            #define ROGUE_BUILDER_DEFINE_BACKEND02(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b,                       \
               {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 0); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 2); \
            #define ROGUE_BUILDER_DEFINE_BACKEND11(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b,                       \
               {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 1); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 1); \
            #define ROGUE_BUILDER_DEFINE_BACKEND13(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b,                       \
                           {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 1); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 3); \
   return rogue_build_backend13(b,                                      \
                                    #define ROGUE_BUILDER_DEFINE_BACKEND14(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b,                       \
                                 {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 1); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 4); \
   return rogue_build_backend14(b,                                      \
                              ROGUE_BACKEND_OP_##op,                  \
         #define ROGUE_BUILDER_DEFINE_BACKEND06(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b,                       \
                                 rogue_ref src0,                         \
   {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 0); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 6); \
   return rogue_build_backend06(b,                                      \
                              ROGUE_BACKEND_OP_##op,                  \
   src0,                                   \
         #define ROGUE_BUILDER_DEFINE_BACKEND16(op)                                 \
      PUBLIC                                                                  \
   rogue_backend_instr *rogue_##op(rogue_builder *b,                       \
                                 rogue_ref dst0,                         \
   rogue_ref src0,                         \
   {                                                                       \
      assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_dsts == 1); \
   assert(rogue_backend_op_infos[ROGUE_BACKEND_OP_##op].num_srcs == 6); \
   return rogue_build_backend16(b,                                      \
                              ROGUE_BACKEND_OP_##op,                  \
   dst0,                                   \
   src0,                                   \
         #include "rogue_backend_instrs.def"
      static inline rogue_ctrl_instr *rogue_build_ctrl(rogue_builder *b,
                                       {
      rogue_ctrl_instr *ctrl =
                     for (unsigned i = 0; i < num_dsts; ++i) {
      ctrl->dst[i].ref = dsts[i];
               for (unsigned i = 0; i < num_srcs; ++i) {
      ctrl->src[i].ref = srcs[i];
               rogue_builder_insert_instr(b, &ctrl->instr);
      }
      static inline rogue_ctrl_instr *
   rogue_build_ctrlb(rogue_builder *b, enum rogue_ctrl_op op, rogue_block *block)
   {
         }
      static inline rogue_ctrl_instr *rogue_build_ctrl00(rogue_builder *b,
         {
         }
      static inline rogue_ctrl_instr *
   rogue_build_ctrl01(rogue_builder *b, enum rogue_ctrl_op op, rogue_ref src0)
   {
      rogue_ref srcs[] = { src0 };
      }
      #define ROGUE_BUILDER_DEFINE_CTRLB(op)                                \
      PUBLIC                                                             \
   rogue_ctrl_instr *rogue_##op(rogue_builder *b, rogue_block *block) \
   {                                                                  \
      assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].has_target);     \
   assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].num_dsts == 0);  \
   assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].num_srcs == 0);  \
            #define ROGUE_BUILDER_DEFINE_CTRL00(op)                              \
      PUBLIC                                                            \
   rogue_ctrl_instr *rogue_##op(rogue_builder *b)                    \
   {                                                                 \
      assert(!rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].has_target);   \
   assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].num_dsts == 0); \
   assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].num_srcs == 0); \
            #define ROGUE_BUILDER_DEFINE_CTRL01(op)                              \
      PUBLIC                                                            \
   rogue_ctrl_instr *rogue_##op(rogue_builder *b, rogue_ref src0)    \
   {                                                                 \
      assert(!rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].has_target);   \
   assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].num_dsts == 0); \
   assert(rogue_ctrl_op_infos[ROGUE_CTRL_OP_##op].num_srcs == 1); \
            #include "rogue_ctrl_instrs.def"
      static inline rogue_bitwise_instr *rogue_build_bitwise(rogue_builder *b,
                                 {
      rogue_bitwise_instr *bitwise =
            for (unsigned i = 0; i < num_dsts; ++i) {
      bitwise->dst[i].ref = dsts[i];
               for (unsigned i = 0; i < num_srcs; ++i) {
      bitwise->src[i].ref = srcs[i];
               rogue_builder_insert_instr(b, &bitwise->instr);
      }
      static inline rogue_bitwise_instr *
   rogue_build_bitwise22(rogue_builder *b,
                        enum rogue_bitwise_op op,
      {
      rogue_ref dsts[] = { dst0, dst1 };
   rogue_ref srcs[] = { src0, src1 };
      }
      #define ROGUE_BUILDER_DEFINE_BITWISE22(op)                                 \
      PUBLIC                                                                  \
   rogue_bitwise_instr *rogue_##op(rogue_builder *b,                       \
                           {                                                                       \
      assert(rogue_bitwise_op_infos[ROGUE_BITWISE_OP_##op].num_dsts == 2); \
   assert(rogue_bitwise_op_infos[ROGUE_BITWISE_OP_##op].num_srcs == 2); \
   return rogue_build_bitwise22(b,                                      \
                                    #include "rogue_bitwise_instrs.def"
