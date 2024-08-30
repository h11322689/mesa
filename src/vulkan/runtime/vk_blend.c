   /*
   * Copyright 2023 Valve Corporation
   * Copyright 2021 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "vk_blend.h"
   #include "util/macros.h"
      enum pipe_logicop
   vk_logic_op_to_pipe(VkLogicOp in)
   {
      switch (in) {
   case VK_LOGIC_OP_CLEAR:
         case VK_LOGIC_OP_AND:
         case VK_LOGIC_OP_AND_REVERSE:
         case VK_LOGIC_OP_COPY:
         case VK_LOGIC_OP_AND_INVERTED:
         case VK_LOGIC_OP_NO_OP:
         case VK_LOGIC_OP_XOR:
         case VK_LOGIC_OP_OR:
         case VK_LOGIC_OP_NOR:
         case VK_LOGIC_OP_EQUIVALENT:
         case VK_LOGIC_OP_INVERT:
         case VK_LOGIC_OP_OR_REVERSE:
         case VK_LOGIC_OP_COPY_INVERTED:
         case VK_LOGIC_OP_OR_INVERTED:
         case VK_LOGIC_OP_NAND:
         case VK_LOGIC_OP_SET:
         default:
            }
      enum pipe_blend_func
   vk_blend_op_to_pipe(VkBlendOp in)
   {
      switch (in) {
   case VK_BLEND_OP_ADD:
         case VK_BLEND_OP_SUBTRACT:
         case VK_BLEND_OP_REVERSE_SUBTRACT:
         case VK_BLEND_OP_MIN:
         case VK_BLEND_OP_MAX:
         default:
            }
      enum pipe_blendfactor
   vk_blend_factor_to_pipe(enum VkBlendFactor vk_factor)
   {
      switch (vk_factor) {
   case VK_BLEND_FACTOR_ZERO:
         case VK_BLEND_FACTOR_ONE:
         case VK_BLEND_FACTOR_SRC_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
         case VK_BLEND_FACTOR_DST_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
         case VK_BLEND_FACTOR_SRC_ALPHA:
         case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
         case VK_BLEND_FACTOR_DST_ALPHA:
         case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
         case VK_BLEND_FACTOR_CONSTANT_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
         case VK_BLEND_FACTOR_CONSTANT_ALPHA:
         case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
         case VK_BLEND_FACTOR_SRC1_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
         case VK_BLEND_FACTOR_SRC1_ALPHA:
         case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
         case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
         default:
            }
