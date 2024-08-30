   /*
   * Copyright © 2014 Intel Corporation
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
   *
   * This code is based on original work by Ilia Mirkin.
   */
      /**
   * \file gfx6_gs_visitor.cpp
   *
   * Gfx6 geometry shader implementation
   */
      #include "gfx6_gs_visitor.h"
   #include "brw_eu.h"
   #include "brw_prim.h"
      namespace brw {
      void
   gfx6_gs_visitor::emit_prolog()
   {
               /* Gfx6 geometry shaders require to allocate an initial VUE handle via
   * FF_SYNC message, however the documentation remarks that only one thread
   * can write to the URB simultaneously and the FF_SYNC message provides the
   * synchronization mechanism for this, so using this message effectively
   * stalls the thread until it is its turn to write to the URB. Because of
   * this, the best way to implement geometry shader algorithms in gfx6 is to
   * execute the algorithm before the FF_SYNC message to maximize parallelism.
   *
   * To achieve this we buffer the geometry shader outputs for each emitted
   * vertex in vertex_output during operation. Then, when we have processed
   * the last vertex (that is, at thread end time), we send the FF_SYNC
   * message to allocate the initial VUE handle and write all buffered vertex
   * data to the URB in one go.
   *
   * For each emitted vertex, vertex_output will hold vue_map.num_slots
   * data items plus one additional item to hold required flags
   * (PrimType, PrimStart, PrimEnd, as expected by the URB_WRITE message)
   * which come right after the data items for that vertex. Vertex data and
   * flags for the next vertex come right after the data items and flags for
   * the previous vertex.
   */
   this->current_annotation = "gfx6 prolog";
   this->vertex_output = src_reg(this,
                     this->vertex_output_offset = src_reg(this, glsl_type::uint_type);
            /* MRF 1 will be the header for all messages (FF_SYNC and URB_WRITES),
   * so initialize it once to R0.
   */
   vec4_instruction *inst = emit(MOV(dst_reg(MRF, 1),
                        /* This will be used as a temporary to store writeback data of FF_SYNC
   * and URB_WRITE messages.
   */
            /* This will be used to know when we are processing the first vertex of
   * a primitive. We will set this to URB_WRITE_PRIM_START only when we know
   * that we are processing the first vertex in the primitive and to zero
   * otherwise. This way we can use its value directly in the URB write
   * headers.
   */
   this->first_vertex = src_reg(this, glsl_type::uint_type);
            /* The FF_SYNC message requires to know the number of primitives generated,
   * so keep a counter for this.
   */
   this->prim_count = src_reg(this, glsl_type::uint_type);
            if (gs_prog_data->num_transform_feedback_bindings) {
      /* Create a virtual register to hold destination indices in SOL */
   this->destination_indices = src_reg(this, glsl_type::uvec4_type);
   /* Create a virtual register to hold number of written primitives */
   this->sol_prim_written = src_reg(this, glsl_type::uint_type);
   /* Create a virtual register to hold Streamed Vertex Buffer Indices */
   this->svbi = src_reg(this, glsl_type::uvec4_type);
   /* Create a virtual register to hold max values of SVBI */
   this->max_svbi = src_reg(this, glsl_type::uvec4_type);
   emit(MOV(dst_reg(this->max_svbi),
               /* PrimitveID is delivered in r0.1 of the thread payload. If the program
   * needs it we have to move it to a separate register where we can map
   * the attribute.
   *
   * Notice that we cannot use a virtual register for this, because we need to
   * map all input attributes to hardware registers in setup_payload(),
   * which happens before virtual registers are mapped to hardware registers.
   * We could work around that issue if we were able to compute the first
   * non-payload register here and move the PrimitiveID information to that
   * register, but we can't because at this point we don't know the final
   * number uniforms that will be included in the payload.
   *
   * So, what we do is to place PrimitiveID information in r1, which is always
   * delivered as part of the payload, but its only populated with data
   * relevant for transform feedback when we set GFX6_GS_SVBI_PAYLOAD_ENABLE
   * in the 3DSTATE_GS state packet. That information can be obtained by other
   * means though, so we can safely use r1 for this purpose.
   */
   if (gs_prog_data->include_primitive_id) {
      this->primitive_id =
               }
      void
   gfx6_gs_visitor::gs_emit_vertex(int stream_id)
   {
               /* Buffer all output slots for this vertex in vertex_output */
   for (int slot = 0; slot < prog_data->vue_map.num_slots; ++slot) {
      int varying = prog_data->vue_map.slot_to_varying[slot];
   if (varying != VARYING_SLOT_PSIZ) {
      dst_reg dst(this->vertex_output);
   dst.reladdr = ralloc(mem_ctx, src_reg);
   memcpy(dst.reladdr, &this->vertex_output_offset, sizeof(src_reg));
      } else {
      /* The PSIZ slot can pack multiple varyings in different channels
   * and emit_urb_slot() will produce a MOV instruction for each of
   * them. Since we are writing to an array, that will translate to
   * possibly multiple MOV instructions with an array destination and
   * each will generate a scratch write with the same offset into
   * scratch space (thus, each one overwriting the previous). This is
   * not what we want. What we will do instead is emit PSIZ to a
   * a regular temporary register, then move that register into the
   * array. This way we only have one instruction with an array
   * destination and we only produce a single scratch write.
   */
   dst_reg tmp = dst_reg(src_reg(this, glsl_type::uvec4_type));
   emit_urb_slot(tmp, varying);
   dst_reg dst(this->vertex_output);
   dst.reladdr = ralloc(mem_ctx, src_reg);
   memcpy(dst.reladdr, &this->vertex_output_offset, sizeof(src_reg));
   vec4_instruction *inst = emit(MOV(dst, src_reg(tmp)));
               emit(ADD(dst_reg(this->vertex_output_offset),
               /* Now buffer flags for this vertex */
   dst_reg dst(this->vertex_output);
   dst.reladdr = ralloc(mem_ctx, src_reg);
   memcpy(dst.reladdr, &this->vertex_output_offset, sizeof(src_reg));
   if (nir->info.gs.output_primitive == GL_POINTS) {
      /* If we are outputting points, then every vertex has PrimStart and
   * PrimEnd set.
   */
   emit(MOV(dst, brw_imm_d((_3DPRIM_POINTLIST << URB_WRITE_PRIM_TYPE_SHIFT) |
            } else {
      /* Otherwise, we can only set the PrimStart flag, which we have stored
   * in the first_vertex register. We will have to wait until we execute
   * EndPrimitive() or we end the thread to set the PrimEnd flag on a
   * vertex.
   */
   emit(OR(dst, this->first_vertex,
         brw_imm_ud(gs_prog_data->output_topology <<
      }
   emit(ADD(dst_reg(this->vertex_output_offset),
      }
      void
   gfx6_gs_visitor::gs_end_primitive()
   {
      this->current_annotation = "gfx6 end primitive";
   /* Calling EndPrimitive() is optional for point output. In this case we set
   * the PrimEnd flag when we process EmitVertex().
   */
   if (nir->info.gs.output_primitive == GL_POINTS)
            /* Otherwise we know that the last vertex we have processed was the last
   * vertex in the primitive and we need to set its PrimEnd flag, so do this
   * unless we haven't emitted that vertex at all (vertex_count != 0).
   *
   * Notice that we have already incremented vertex_count when we processed
   * the last emit_vertex, so we need to take that into account in the
   * comparison below (hence the num_output_vertices + 1 in the comparison
   * below).
   */
   unsigned num_output_vertices = nir->info.gs.vertices_out;
   emit(CMP(dst_null_ud(), this->vertex_count,
         vec4_instruction *inst = emit(CMP(dst_null_ud(),
               inst->predicate = BRW_PREDICATE_NORMAL;
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      /* vertex_output_offset is already pointing at the first entry of the
   * next vertex. So subtract 1 to modify the flags for the previous
   * vertex.
   */
   src_reg offset(this, glsl_type::uint_type);
            src_reg dst(this->vertex_output);
   dst.reladdr = ralloc(mem_ctx, src_reg);
            emit(OR(dst_reg(dst), dst, brw_imm_d(URB_WRITE_PRIM_END)));
            /* Set the first vertex flag to indicate that the next vertex will start
   * a primitive.
   */
      }
      }
      void
   gfx6_gs_visitor::emit_urb_write_header(int mrf)
   {
      this->current_annotation = "gfx6 urb header";
   /* Compute offset of the flags for the current vertex in vertex_output and
   * write them in dw2 of the message header.
   *
   * Notice that by the time that emit_thread_end() calls here
   * vertex_output_offset should point to the first data item of the current
   * vertex in vertex_output, thus we only need to add the number of output
   * slots per vertex to that offset to obtain the flags data offset.
   */
   src_reg flags_offset(this, glsl_type::uint_type);
   emit(ADD(dst_reg(flags_offset),
                  src_reg flags_data(this->vertex_output);
   flags_data.reladdr = ralloc(mem_ctx, src_reg);
               }
      static unsigned
   align_interleaved_urb_mlen(unsigned mlen)
   {
      /* URB data written (does not include the message header reg) must
   * be a multiple of 256 bits, or 2 VS registers.  See vol5c.5,
   * section 5.4.3.2.2: URB_INTERLEAVED.
   */
   if ((mlen % 2) != 1)
            }
      void
   gfx6_gs_visitor::emit_snb_gs_urb_write_opcode(bool complete, int base_mrf,
         {
               if (!complete) {
      /* If the vertex is not complete we don't have to do anything special */
   inst = emit(VEC4_GS_OPCODE_URB_WRITE);
      } else {
      /* Otherwise we always request to allocate a new VUE handle. If this is
   * the last write before the EOT message and the new handle never gets
   * used it will be dereferenced when we send the EOT message. This is
   * necessary to avoid different setups for the EOT message (one for the
   * case when there is no output and another for the case when there is)
   * which would require to end the program with an IF/ELSE/ENDIF block,
   * something we do not want.
   */
   inst = emit(VEC4_GS_OPCODE_URB_WRITE_ALLOCATE);
   inst->urb_write_flags = BRW_URB_WRITE_COMPLETE;
   inst->dst = dst_reg(MRF, base_mrf);
               inst->base_mrf = base_mrf;
   inst->mlen = align_interleaved_urb_mlen(last_mrf - base_mrf);
      }
      void
   gfx6_gs_visitor::emit_thread_end()
   {
      /* Make sure the current primitive is ended: we know it is not ended when
   * first_vertex is not zero. This is only relevant for outputs other than
   * points because in the point case we set PrimEnd on all vertices.
   */
   if (nir->info.gs.output_primitive != GL_POINTS) {
      emit(CMP(dst_null_ud(), this->first_vertex, brw_imm_ud(0u), BRW_CONDITIONAL_Z));
   emit(IF(BRW_PREDICATE_NORMAL));
   gs_end_primitive();
               /* Here we have to:
   * 1) Emit an FF_SYNC message to obtain an initial VUE handle.
   * 2) Loop over all buffered vertex data and write it to corresponding
   *    URB entries.
   * 3) Allocate new VUE handles for all vertices other than the first.
   * 4) Send a final EOT message.
            /* MRF 0 is reserved for the debugger, so start with message header
   * in MRF 1.
   */
            /* In the process of generating our URB write message contents, we
   * may need to unspill a register or load from an array.  Those
   * reads would use MRFs 21..23
   */
            /* Issue the FF_SYNC message and obtain the initial VUE handle. */
            vec4_instruction *inst = NULL;
   if (gs_prog_data->num_transform_feedback_bindings) {
      src_reg sol_temp(this, glsl_type::uvec4_type);
   emit(GS_OPCODE_FF_SYNC_SET_PRIMITIVES,
      dst_reg(this->svbi),
   this->vertex_count,
   this->prim_count,
      inst = emit(GS_OPCODE_FF_SYNC,
      } else {
      inst = emit(GS_OPCODE_FF_SYNC,
      }
            emit(CMP(dst_null_ud(), this->vertex_count, brw_imm_ud(0u), BRW_CONDITIONAL_G));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      /* Loop over all buffered vertices and emit URB write messages */
   this->current_annotation = "gfx6 thread end: urb writes init";
   src_reg vertex(this, glsl_type::uint_type);
   emit(MOV(dst_reg(vertex), brw_imm_ud(0u)));
            this->current_annotation = "gfx6 thread end: urb writes";
   emit(BRW_OPCODE_DO);
   {
      emit(CMP(dst_null_d(), vertex, this->vertex_count, BRW_CONDITIONAL_GE));
                                 /* Then add vertex data to the message in interleaved fashion */
   int slot = 0;
   bool complete = false;
                     /* URB offset is in URB row increments, and each of our MRFs is half
   * of one of those, since we're doing interleaved writes.
                  for (; slot < prog_data->vue_map.num_slots; ++slot) {
                     /* Compute offset of this slot for the current vertex
   * in vertex_output
   */
   src_reg data(this->vertex_output);
                        /* Copy this slot to the appropriate message register */
   dst_reg reg = dst_reg(MRF, mrf);
   reg.type = output_reg[varying][0].type;
                                             /* If this was max_usable_mrf, we can't fit anything more into
   * this URB WRITE. Same if we reached the max. message length.
   */
   if (mrf > max_usable_mrf ||
      align_interleaved_urb_mlen(mrf - base_mrf + 1) > BRW_MAX_MSG_LENGTH) {
   slot++;
                  complete = slot >= prog_data->vue_map.num_slots;
               /* Skip over the flags data item so that vertex_output_offset points
   * to the first data item of the next vertex, so that we can start
   * writing the next vertex.
   */
                     }
            if (gs_prog_data->num_transform_feedback_bindings)
      }
            /* Finally, emit EOT message.
   *
   * In gfx6 we need to end the thread differently depending on whether we have
   * emitted at least one vertex or not. In case we did, the EOT message must
   * always include the COMPLETE flag or else the GPU hangs. If we have not
   * produced any output we can't use the COMPLETE flag.
   *
   * However, this would lead us to end the program with an ENDIF opcode,
   * which we want to avoid, so what we do is that we always request a new
   * VUE handle every time, even if GS produces no output.
   * With this we make sure that whether we have emitted at least one vertex
   * or none at all, we have to finish the thread without writing to the URB,
   * which works for both cases by setting the COMPLETE and UNUSED flags in
   * the EOT message.
   */
            if (gs_prog_data->num_transform_feedback_bindings) {
      /* When emitting EOT, set SONumPrimsWritten Increment Value. */
   src_reg data(this, glsl_type::uint_type);
   emit(AND(dst_reg(data), this->sol_prim_written, brw_imm_ud(0xffffu)));
   emit(SHL(dst_reg(data), data, brw_imm_ud(16u)));
               inst = emit(GS_OPCODE_THREAD_END);
   inst->urb_write_flags = BRW_URB_WRITE_COMPLETE | BRW_URB_WRITE_UNUSED;
   inst->base_mrf = base_mrf;
      }
      void
   gfx6_gs_visitor::setup_payload()
   {
               /* Attributes are going to be interleaved, so one register contains two
   * attribute slots.
   */
            /* If a geometry shader tries to read from an input that wasn't written by
   * the vertex shader, that produces undefined results, but it shouldn't
   * crash anything.  So initialize attribute_map to zeros--that ensures that
   * these undefined results are read from r0.
   */
                     /* The payload always contains important data in r0. */
            /* r1 is always part of the payload and it holds information relevant
   * for transform feedback when we set the GFX6_GS_SVBI_PAYLOAD_ENABLE bit in
   * the 3DSTATE_GS packet. We will overwrite it with the PrimitiveID
   * information (and move the original value to a virtual register if
   * necessary).
   */
   if (gs_prog_data->include_primitive_id)
                                       }
      void
   gfx6_gs_visitor::xfb_write()
   {
               switch (gs_prog_data->output_topology) {
   case _3DPRIM_POINTLIST:
      num_verts = 1;
      case _3DPRIM_LINELIST:
   case _3DPRIM_LINESTRIP:
   case _3DPRIM_LINELOOP:
      num_verts = 2;
      case _3DPRIM_TRILIST:
   case _3DPRIM_TRIFAN:
   case _3DPRIM_TRISTRIP:
   case _3DPRIM_RECTLIST:
      num_verts = 3;
      case _3DPRIM_QUADLIST:
   case _3DPRIM_QUADSTRIP:
   case _3DPRIM_POLYGON:
      num_verts = 3;
      default:
                           emit(MOV(dst_reg(this->vertex_output_offset), brw_imm_ud(0u)));
            /* Check that at least one primitive can be written
   *
   * Note: since we use the binding table to keep track of buffer offsets
   * and stride, the GS doesn't need to keep track of a separate pointer
   * into each buffer; it uses a single pointer which increments by 1 for
   * each vertex.  So we use SVBI0 for this pointer, regardless of whether
   * transform feedback is in interleaved or separate attribs mode.
   */
   src_reg sol_temp(this, glsl_type::uvec4_type);
            /* Compare SVBI calculated number with the maximum value, which is
   * in R1.4 (previously saved in this->max_svbi) for gfx6.
   */
   emit(CMP(dst_null_d(), sol_temp, this->max_svbi, BRW_CONDITIONAL_LE));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      vec4_instruction *inst = emit(MOV(dst_reg(destination_indices),
                                    emit(ADD(dst_reg(this->destination_indices),
            }
            /* Write transform feedback data for all processed vertices. */
   for (int i = 0; i < (int)nir->info.gs.vertices_out; i++) {
      emit(MOV(dst_reg(sol_temp), brw_imm_d(i)));
   emit(CMP(dst_null_d(), sol_temp, this->vertex_count,
         emit(IF(BRW_PREDICATE_NORMAL));
   {
         }
         }
      void
   gfx6_gs_visitor::xfb_program(unsigned vertex, unsigned num_verts)
   {
      unsigned binding;
   unsigned num_bindings = gs_prog_data->num_transform_feedback_bindings;
            /* Check for buffer overflow: we need room to write the complete primitive
   * (all vertices). Otherwise, avoid writing any vertices for it
   */
   emit(ADD(dst_reg(sol_temp), this->sol_prim_written, brw_imm_ud(1u)));
   emit(MUL(dst_reg(sol_temp), sol_temp, brw_imm_ud(num_verts)));
   emit(ADD(dst_reg(sol_temp), sol_temp, this->svbi));
   emit(CMP(dst_null_d(), sol_temp, this->max_svbi, BRW_CONDITIONAL_LE));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      /* Avoid overwriting MRF 1 as it is used as URB write message header */
            this->current_annotation = "gfx6: emit SOL vertex data";
   /* For each vertex, generate code to output each varying using the
   * appropriate binding table entry.
   */
   for (binding = 0; binding < num_bindings; ++binding) {
                     /* Set up the correct destination index for this vertex */
   vec4_instruction *inst = emit(GS_OPCODE_SVB_SET_DST_INDEX,
                        /* From the Sandybridge PRM, Volume 2, Part 1, Section 4.5.1:
   *
   *   "Prior to End of Thread with a URB_WRITE, the kernel must
   *   ensure that all writes are complete by sending the final
   *   write as a committed write."
   */
                  /* Compute offset of this varying for the current vertex
   * in vertex_output
   */
   this->current_annotation = output_reg_annotation[varying];
   src_reg data(this->vertex_output);
   data.reladdr = ralloc(mem_ctx, src_reg);
   int offset = get_vertex_output_offset_for_varying(vertex, varying);
   emit(MOV(dst_reg(this->vertex_output_offset), brw_imm_d(offset)));
   memcpy(data.reladdr, &this->vertex_output_offset, sizeof(src_reg));
                  /* Write data */
   inst = emit(GS_OPCODE_SVB_WRITE, mrf_reg, data, sol_temp);
                  if (final_write) {
      /* This is the last vertex of the primitive, then increment
   * SO num primitive counter and destination indices.
   */
   emit(ADD(dst_reg(this->destination_indices),
               emit(ADD(dst_reg(this->sol_prim_written),
            }
      }
      }
      int
   gfx6_gs_visitor::get_vertex_output_offset_for_varying(int vertex, int varying)
   {
      /* Find the output slot assigned to this varying.
   *
   * VARYING_SLOT_LAYER and VARYING_SLOT_VIEWPORT are packed in the same slot
   * as VARYING_SLOT_PSIZ.
   */
   if (varying == VARYING_SLOT_LAYER || varying == VARYING_SLOT_VIEWPORT)
                  if (slot < 0) {
      /* This varying does not exist in the VUE so we are not writing to it
   * and its value is undefined. We still want to return a valid offset
   * into vertex_output though, to prevent any out-of-bound accesses into
   * the vertex_output array. Since the value for this varying is undefined
   * we don't really care for the value we assign to it, so any offset
   * within the limits of vertex_output will do.
   */
                  }
      } /* namespace brw */
