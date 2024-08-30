   /*
   * Copyright © 2018 Intel Corporation
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
   */
      #include "nir_xfb_info.h"
      #include "util/u_dynarray.h"
   #include <util/u_math.h>
      static void
   add_var_xfb_varying(nir_xfb_info *xfb,
                     nir_xfb_varyings_info *varyings,
   {
      if (varyings == NULL)
                     varying->type = type;
   varying->buffer = buffer;
   varying->offset = offset;
      }
      static nir_xfb_info *
   nir_xfb_info_create(void *mem_ctx, uint16_t output_count)
   {
         }
      static size_t
   nir_xfb_varyings_info_size(uint16_t varying_count)
   {
         }
      static nir_xfb_varyings_info *
   nir_xfb_varyings_info_create(void *mem_ctx, uint16_t varying_count)
   {
         }
      static void
   add_var_xfb_outputs(nir_xfb_info *xfb,
                     nir_xfb_varyings_info *varyings,
   nir_variable *var,
   unsigned buffer,
   unsigned *location,
   {
      /* If this type contains a 64-bit value, align to 8 bytes */
   if (glsl_type_contains_64bit(type))
            if (glsl_type_is_array_or_matrix(type) && !var->data.compact) {
               const struct glsl_type *child_type = glsl_get_array_element(type);
   if (!glsl_type_is_array(child_type) &&
               add_var_xfb_varying(xfb, varyings, buffer, *offset, type);
               for (unsigned i = 0; i < length; i++)
      add_var_xfb_outputs(xfb, varyings, var, buffer, location, offset,
   } else if (glsl_type_is_struct_or_ifc(type)) {
      unsigned length = glsl_get_length(type);
   for (unsigned i = 0; i < length; i++) {
      const struct glsl_type *child_type = glsl_get_struct_field(type, i);
   add_var_xfb_outputs(xfb, varyings, var, buffer, location, offset,
         } else {
      assert(buffer < NIR_MAX_XFB_BUFFERS);
   if (xfb->buffers_written & (1 << buffer)) {
      assert(xfb->buffers[buffer].stride == var->data.xfb.stride);
      } else {
      xfb->buffers_written |= (1 << buffer);
   xfb->buffers[buffer].stride = var->data.xfb.stride;
               assert(var->data.stream < NIR_MAX_XFB_STREAMS);
            unsigned comp_slots;
   if (var->data.compact) {
      /* This only happens for clip/cull which are float arrays */
   assert(glsl_without_array(type) == glsl_float_type());
   assert(var->data.location == VARYING_SLOT_CLIP_DIST0 ||
            } else {
                              /* Ensure that we don't have, for instance, a dvec2 with a
   * location_frac of 2 which would make it crass a location boundary
   * even though it fits in a single slot.  However, you can have a
   * dvec3 which crosses the slot boundary with a location_frac of 2.
   */
   assert(DIV_ROUND_UP(var->data.location_frac + comp_slots, 4) ==
               assert(var->data.location_frac + comp_slots <= 8);
   uint8_t comp_mask = ((1 << comp_slots) - 1) << var->data.location_frac;
            if (!varying_added) {
                  while (comp_mask) {
               output->buffer = buffer;
   output->offset = *offset;
   output->location = *location;
                  *offset += util_bitcount(output->component_mask) * 4;
   (*location)++;
   comp_mask >>= 4;
            }
      static int
   compare_xfb_varying_offsets(const void *_a, const void *_b)
   {
               if (a->buffer != b->buffer)
               }
      static int
   compare_xfb_output_offsets(const void *_a, const void *_b)
   {
                  }
      void
   nir_shader_gather_xfb_info(nir_shader *shader)
   {
         }
      void
   nir_gather_xfb_info_with_varyings(nir_shader *shader,
               {
      assert(shader->info.stage == MESA_SHADER_VERTEX ||
                  /* Compute the number of outputs we have.  This is simply the number of
   * cumulative locations consumed by all the variables.  If a location is
   * represented by multiple variables, then they each count separately in
   * number of outputs.  This is only an estimate as some variables may have
   * an xfb_buffer but not an output so it may end up larger than we need but
   * it should be good enough for allocation.
   */
   unsigned num_outputs = 0;
   unsigned num_varyings = 0;
   nir_xfb_varyings_info *varyings_info = NULL;
   nir_foreach_shader_out_variable(var, shader) {
      if (var->data.explicit_xfb_buffer) {
      num_outputs += glsl_count_attribute_slots(var->type, false);
         }
   if (num_outputs == 0 || num_varyings == 0)
            nir_xfb_info *xfb = nir_xfb_info_create(shader, num_outputs);
   if (varyings_info_out != NULL) {
      *varyings_info_out = nir_xfb_varyings_info_create(mem_ctx, num_varyings);
               /* Walk the list of outputs and add them to the array */
   nir_foreach_shader_out_variable(var, shader) {
      if (!var->data.explicit_xfb_buffer)
                     /* In order to know if we have a array of blocks can't be done just by
   * checking if we have an interface type and is an array, because due
   * splitting we could end on a case were we received a split struct
   * that contains an array.
   */
   bool is_array_block = var->interface_type != NULL &&
                  if (var->data.explicit_offset && !is_array_block) {
      unsigned offset = var->data.offset;
   add_var_xfb_outputs(xfb, varyings_info, var, var->data.xfb.buffer,
      } else if (is_array_block) {
               unsigned aoa_size = glsl_get_aoa_size(var->type);
   const struct glsl_type *itype = var->interface_type;
   unsigned nfields = glsl_get_length(itype);
   for (unsigned b = 0; b < aoa_size; b++) {
      for (unsigned f = 0; f < nfields; f++) {
      int foffset = glsl_get_struct_field_offset(itype, f);
   const struct glsl_type *ftype = glsl_get_struct_field(itype, f);
   if (foffset < 0) {
                        unsigned offset = foffset;
   add_var_xfb_outputs(xfb, varyings_info, var, var->data.xfb.buffer + b,
                        /* Everything is easier in the state setup code if outputs and varyings are
   * sorted in order of output offset (and buffer for varyings).
   */
   qsort(xfb->outputs, xfb->output_count, sizeof(xfb->outputs[0]),
            if (varyings_info != NULL) {
      qsort(varyings_info->varyings, varyings_info->varying_count,
                  #ifndef NDEBUG
      /* Finally, do a sanity check */
   unsigned max_offset[NIR_MAX_XFB_BUFFERS] = { 0 };
   for (unsigned i = 0; i < xfb->output_count; i++) {
      assert(xfb->outputs[i].offset >= max_offset[xfb->outputs[i].buffer]);
   assert(xfb->outputs[i].component_mask != 0);
   unsigned slots = util_bitcount(xfb->outputs[i].component_mask);
         #endif
         ralloc_free(shader->xfb_info);
      }
      static int
   get_xfb_out_sort_index(const nir_xfb_output_info *a)
   {
      /* Return the maximum number to put dummy components at the end. */
   if (!a->component_mask)
            return ((uint32_t)a->buffer << 26) | /* 2 bits for the buffer */
         /* 10 bits for the component location (256 * 4) */
   (((uint32_t)a->location * 4 + a->component_offset) << 16) |
      }
      static int
   compare_xfb_out(const void *pa, const void *pb)
   {
      const nir_xfb_output_info *a = (const nir_xfb_output_info *)pa;
               }
      /**
   * Gather transform feedback info from lowered IO intrinsics.
   */
   void
   nir_gather_xfb_info_from_intrinsics(nir_shader *nir)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   uint8_t buffer_to_stream[MAX_XFB_BUFFERS] = { 0 };
   uint8_t buffer_mask = 0;
            /* Gather xfb outputs. */
            nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic ||
                                    while (wr_mask) {
      unsigned i = u_bit_scan(&wr_mask);
                  if (xfb.out[index % 2].num_components) {
                     out.component_offset = index;
   out.component_mask =
         out.location = sem.location;
   out.high_16bits = sem.high_16bits;
                        uint8_t stream = (sem.gs_streams >> (i * 2)) & 0x3;
                        /* No elements before component_offset are allowed to be set. */
                        nir_xfb_output_info *outputs = (nir_xfb_output_info *)array.data;
            if (!count)
            if (count > 1) {
      /* Sort outputs by buffer, location, and component. */
            /* Merge outputs referencing the same slot. */
   for (int i = 0; i < count - 1; i++) {
                              /* Outputs referencing the same buffer and location are contiguous. */
   for (int j = i + 1;
      j < count &&
   cur->buffer == outputs[j].buffer &&
   cur->location == outputs[j].location &&
   cur->high_16bits == outputs[j].high_16bits;
   j++) {
   if (outputs[j].component_mask &&
      outputs[j].offset - outputs[j].component_offset * 4 ==
         unsigned merged_offset = MIN2(cur->component_offset,
                        /* The component mask should have no holes after merging. */
   if (util_is_power_of_two_nonzero((merged_mask >> merged_offset) + 1)) {
      /* Merge outputs. */
   cur->component_offset = merged_offset;
   cur->component_mask = merged_mask;
   cur->offset = (uint32_t)cur->offset -
               /* Disable the other output. */
                        /* Sort outputs again to put disabled outputs at the end. */
            /* Remove disabled outputs. */
   for (int i = count - 1; i >= 0 && !outputs[i].component_mask; i--)
               for (unsigned i = 0; i < count; i++)
            /* Create nir_xfb_info. */
   nir_xfb_info *info = nir_xfb_info_create(nir, count);
   if (!info) {
      util_dynarray_fini(&array);
               /* Fill nir_xfb_info. */
   info->buffers_written = buffer_mask;
   info->streams_written = stream_mask;
   memcpy(info->buffer_to_stream, buffer_to_stream, sizeof(buffer_to_stream));
   info->output_count = count;
            /* Set strides. */
   for (unsigned i = 0; i < MAX_XFB_BUFFERS; i++) {
      if (buffer_mask & BITFIELD_BIT(i))
               /* Set varying_count. */
   for (unsigned i = 0; i < count; i++)
            /* Replace original xfb info. */
   ralloc_free(nir->xfb_info);
               }
      void
   nir_print_xfb_info(nir_xfb_info *info, FILE *fp)
   {
      fprintf(fp, "buffers_written: 0x%x\n", info->buffers_written);
            for (unsigned i = 0; i < NIR_MAX_XFB_BUFFERS; i++) {
      if (BITFIELD_BIT(i) & info->buffers_written) {
      fprintf(fp, "buffer%u: stride=%u varying_count=%u stream=%u\n", i,
         info->buffers[i].stride,
                           for (unsigned i = 0; i < info->output_count; i++) {
      fprintf(fp, "output%u: buffer=%u, offset=%u, location=%u, high_16bits=%u, "
               i, info->outputs[i].buffer,
   info->outputs[i].offset,
   info->outputs[i].location,
   info->outputs[i].high_16bits,
         }
