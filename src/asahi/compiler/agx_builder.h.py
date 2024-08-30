   template = """/*
   * Copyright 2021 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #ifndef _AGX_BUILDER_
   #define _AGX_BUILDER_
      #include "agx_compiler.h"
      static inline agx_instr *
   agx_alloc_instr(agx_builder *b, enum agx_opcode op, uint8_t nr_dests, uint8_t nr_srcs)
   {
      size_t size = sizeof(agx_instr);
   size += sizeof(agx_index) * nr_dests;
            agx_instr *I = (agx_instr *) rzalloc_size(b->shader, size);
   I->dest = (agx_index *) (I + 1);
            I->op = op;
   I->nr_dests = nr_dests;
   I->nr_srcs = nr_srcs;
      }
      % for opcode in opcodes:
   <%
      op = opcodes[opcode]
   dests = op.dests
   srcs = op.srcs
   imms = op.imms
   suffix = "_to" if dests > 0 else ""
   nr_dests = "nr_dests" if op.variable_dests else str(dests)
      %>
      static inline agx_instr *
   agx_${opcode}${suffix}(agx_builder *b
      % if op.variable_dests:
         % endif
      % for dest in range(dests):
         % endfor
      % if op.variable_srcs:
         % endif
      % for src in range(srcs):
         % endfor
      % for imm in imms:
         % endfor
      ) {
            % for dest in range(dests):
         % endfor
      % for src in range(srcs):
         % endfor
      % for imm in imms:
         % endfor
         agx_builder_insert(&b->cursor, I);
      }
      % if dests == 1 and not op.variable_srcs and not op.variable_dests:
   static inline agx_index
   agx_${opcode}(agx_builder *b
      % if srcs == 0:
         % endif
      % for src in range(srcs):
         % endfor
      % for imm in imms:
         % endfor
      ) {
   <%
      args = ["tmp"]
   args += ["src" + str(i) for i in range(srcs)]
      %>
   % if srcs == 0:
         % else:
         % endif
      agx_${opcode}_to(b, ${", ".join(args)});
      }
   % endif
      % endfor
      /* Convenience methods */
      enum agx_bitop_table {
      AGX_BITOP_NOT = 0x5,
   AGX_BITOP_XOR = 0x6,
   AGX_BITOP_AND = 0x8,
   AGX_BITOP_MOV = 0xA,
      };
      static inline agx_instr *
   agx_fmov_to(agx_builder *b, agx_index dst0, agx_index src0)
   {
         }
      static inline agx_instr *
   agx_push_exec(agx_builder *b, unsigned n)
   {
         }
      static inline agx_instr *
   agx_ushr_to(agx_builder *b, agx_index dst, agx_index s0, agx_index s1)
   {
         }
      static inline agx_index
   agx_ushr(agx_builder *b, agx_index s0, agx_index s1)
   {
      agx_index tmp = agx_temp(b->shader, s0.size);
   agx_ushr_to(b, tmp, s0, s1);
      }
      #endif
   """
      from mako.template import Template
   from agx_opcodes import opcodes
      print(Template(template).render(opcodes=opcodes))
