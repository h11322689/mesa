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
      #include <assert.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <stdlib.h>
      #include "compiler/shader_enums.h"
   #include "nir/nir.h"
   #include "rogue.h"
   #include "util/macros.h"
      /**
   * \file rogue_build_data.c
   *
   * \brief Contains functions to collect build data for the driver.
   */
      /* N.B. This will all be hoisted into the driver. */
      /**
   * \brief Allocates the coefficient registers that will contain the iterator
   * data for the fragment shader input varyings.
   *
   * \param[in] args The iterator argument data.
   * \return The total number of coefficient registers required by the iterators.
   */
   static unsigned alloc_iterator_regs(struct rogue_iterator_args *args)
   {
               for (unsigned u = 0; u < args->num_fpu_iterators; ++u) {
      /* Ensure there aren't any gaps. */
            args->base[u] = coeffs;
                  }
      /**
   * \brief Reserves an iterator for a fragment shader input varying,
   * and calculates its setup data.
   *
   * \param[in] args The iterator argument data.
   * \param[in] i The iterator index.
   * \param[in] type The interpolation type of the varying.
   * \param[in] f16 Whether the data type is F16 or F32.
   * \param[in] components The number of components in the varying.
   */
   static void reserve_iterator(struct rogue_iterator_args *args,
                           {
                        /* The first iterator (W) *must* be INTERP_MODE_NOPERSPECTIVE. */
   assert(i > 0 || type == INTERP_MODE_NOPERSPECTIVE);
            switch (type) {
   /* Default interpolation is smooth. */
   case INTERP_MODE_NONE:
      data.shademodel = ROGUE_PDSINST_DOUTI_SHADEMODEL_GOURUAD;
   data.perspective = true;
         case INTERP_MODE_NOPERSPECTIVE:
      data.shademodel = ROGUE_PDSINST_DOUTI_SHADEMODEL_GOURUAD;
   data.perspective = false;
         default:
                  /* Number of components in this varying
   * (corresponds to ROGUE_PDSINST_DOUTI_SIZE_1..4D).
   */
            /* TODO: Investigate F16 support. */
   assert(!f16);
            /* Offsets within the vertex. */
   data.f32_offset = 2 * i;
            ROGUE_PDSINST_DOUT_FIELDS_DOUTI_SRC_pack(&args->fpu_iterators[i], &data);
   args->destination[i] = i;
   args->base[i] = ~0;
   args->components[i] = components;
      }
      static inline unsigned nir_count_variables_with_modes(const nir_shader *nir,
         {
               nir_foreach_variable_with_modes (var, nir, mode) {
                     }
      /**
   * \brief Collects the fragment shader I/O data to feed-back to the driver.
   *
   * \sa #collect_io_data()
   *
   * \param[in] common_data Common build data.
   * \param[in] fs_data Fragment-specific build data.
   * \param[in] nir NIR fragment shader.
   */
   static void collect_io_data_fs(struct rogue_common_build_data *common_data,
               {
      unsigned num_inputs = nir_count_variables_with_modes(nir, nir_var_shader_in);
            /* Process inputs (if present). */
   if (num_inputs) {
      /* If the fragment shader has inputs, the first iterator
   * must be used for the W component.
   */
   reserve_iterator(&fs_data->iterator_args,
                              nir_foreach_shader_in_variable (var, nir) {
      unsigned i = (var->data.location - VARYING_SLOT_VAR0) + 1;
   unsigned components = glsl_get_components(var->type);
                  /* Check that arguments are either F16 or F32. */
                  /* Check input location. */
                              common_data->coeffs = alloc_iterator_regs(&fs_data->iterator_args);
   assert(common_data->coeffs);
                  }
      /**
   * \brief Allocates the vertex shader outputs.
   *
   * \param[in] outputs The vertex shader output data.
   * \return The total number of vertex outputs required.
   */
   static unsigned alloc_vs_outputs(struct rogue_vertex_outputs *outputs)
   {
               for (unsigned u = 0; u < outputs->num_output_vars; ++u) {
      /* Ensure there aren't any gaps. */
            outputs->base[u] = vs_outputs;
                  }
      /**
   * \brief Counts the varyings used by the vertex shader.
   *
   * \param[in] outputs The vertex shader output data.
   * \return The number of varyings used.
   */
   static unsigned count_vs_varyings(struct rogue_vertex_outputs *outputs)
   {
               /* Skip the position. */
   for (unsigned u = 1; u < outputs->num_output_vars; ++u)
               }
      /**
   * \brief Reserves space for a vertex shader input.
   *
   * \param[in] inputs The vertex input data.
   * \param[in] i The vertex input index.
   * \param[in] components The number of components in the input.
   */
   static void reserve_vs_input(struct rogue_vertex_inputs *inputs,
               {
                        inputs->base[i] = ~0;
   inputs->components[i] = components;
      }
      /**
   * \brief Reserves space for a vertex shader output.
   *
   * \param[in] outputs The vertex output data.
   * \param[in] i The vertex output index.
   * \param[in] components The number of components in the output.
   */
   static void reserve_vs_output(struct rogue_vertex_outputs *outputs,
               {
                        outputs->base[i] = ~0;
   outputs->components[i] = components;
      }
      /**
   * \brief Collects the vertex shader I/O data to feed-back to the driver.
   *
   * \sa #collect_io_data()
   *
   * \param[in] common_data Common build data.
   * \param[in] vs_data Vertex-specific build data.
   * \param[in] nir NIR vertex shader.
   */
   static void collect_io_data_vs(struct rogue_common_build_data *common_data,
               {
      ASSERTED bool out_pos_present = false;
   ASSERTED unsigned num_outputs =
                     /* We should always have at least a position variable. */
            nir_foreach_shader_out_variable (var, nir) {
               /* Check that outputs are F32. */
   /* TODO: Support other types. */
   assert(glsl_get_base_type(var->type) == GLSL_TYPE_FLOAT);
            if (var->data.location == VARYING_SLOT_POS) {
                        } else if ((var->data.location >= VARYING_SLOT_VAR0) &&
            unsigned i = (var->data.location - VARYING_SLOT_VAR0) + 1;
      } else {
                     /* Always need the output position to be present. */
            vs_data->num_vertex_outputs = alloc_vs_outputs(&vs_data->outputs);
   assert(vs_data->num_vertex_outputs);
   assert(vs_data->num_vertex_outputs <
               }
      /**
   * \brief Collects I/O data to feed-back to the driver.
   *
   * Collects the inputs/outputs/memory required, and feeds that back to the
   * driver. Done at this stage rather than at the start of rogue_to_binary, so
   * that all the I/O of all the shader stages is known before backend
   * compilation, which would let us do things like cull unused inputs.
   *
   * \param[in] ctx Shared multi-stage build context.
   * \param[in] nir NIR shader.
   */
   PUBLIC
   void rogue_collect_io_data(struct rogue_build_ctx *ctx, nir_shader *nir)
   {
      gl_shader_stage stage = nir->info.stage;
            /* Collect stage-specific data. */
   switch (stage) {
   case MESA_SHADER_FRAGMENT:
            case MESA_SHADER_VERTEX:
            default:
                     }
      /**
   * \brief Returns the allocated coefficient register index for a component of an
   * input varying location.
   *
   * \param[in] args The allocated iterator argument data.
   * \param[in] location The input varying location, or ~0 for the W coefficient.
   * \param[in] component The requested component.
   * \return The coefficient register index.
   */
   PUBLIC
   unsigned rogue_coeff_index_fs(struct rogue_iterator_args *args,
               {
               /* Special case: W coefficient. */
   if (location == ~0) {
      /* The W component shouldn't be the only one. */
   assert(args->num_fpu_iterators > 1);
   assert(args->destination[0] == 0);
               i = (location - VARYING_SLOT_VAR0) + 1;
   assert(location >= VARYING_SLOT_VAR0 && location <= VARYING_SLOT_VAR31);
   assert(i < args->num_fpu_iterators);
   assert(component < args->components[i]);
               }
      /**
   * \brief Returns the allocated vertex output index for a component of an input
   * varying location.
   *
   * \param[in] outputs The vertex output data.
   * \param[in] location The output varying location.
   * \param[in] component The requested component.
   * \return The vertex output index.
   */
   PUBLIC
   unsigned rogue_output_index_vs(struct rogue_vertex_outputs *outputs,
               {
               if (location == VARYING_SLOT_POS) {
      /* Always at location 0. */
   assert(outputs->base[0] == 0);
      } else if ((location >= VARYING_SLOT_VAR0) &&
               } else {
                  assert(i < outputs->num_output_vars);
   assert(component < outputs->components[i]);
               }
