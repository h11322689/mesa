   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir/nir.h"
   #include "nir/nir_serialize.h"
   #include "glsl_types.h"
   #include "nir_types.h"
   #include "clc.h"
   #include "clc_helpers.h"
   #include "nir_clc_helpers.h"
   #include "spirv/nir_spirv.h"
   #include "util/u_debug.h"
      #include <stdlib.h>
      enum clc_debug_flags {
      CLC_DEBUG_DUMP_SPIRV = 1 << 0,
      };
      static const struct debug_named_value clc_debug_options[] = {
      { "dump_spirv",  CLC_DEBUG_DUMP_SPIRV, "Dump spirv blobs" },
   { "verbose",  CLC_DEBUG_VERBOSE, NULL },
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(debug_clc, "CLC_DEBUG", clc_debug_options, 0)
      static void
   clc_print_kernels_info(const struct clc_parsed_spirv *obj)
   {
      fprintf(stdout, "Kernels:\n");
   for (unsigned i = 0; i < obj->num_kernels; i++) {
      const struct clc_kernel_arg *args = obj->kernels[i].args;
            fprintf(stdout, "\tvoid %s(", obj->kernels[i].name);
   for (unsigned j = 0; j < obj->kernels[i].num_args; j++) {
      if (!first)
                        switch (args[j].address_qualifier) {
   case CLC_KERNEL_ARG_ADDRESS_GLOBAL:
      fprintf(stdout, "__global ");
      case CLC_KERNEL_ARG_ADDRESS_LOCAL:
      fprintf(stdout, "__local ");
      case CLC_KERNEL_ARG_ADDRESS_CONSTANT:
      fprintf(stdout, "__constant ");
      default:
                  if (args[j].type_qualifier & CLC_KERNEL_ARG_TYPE_VOLATILE)
         if (args[j].type_qualifier & CLC_KERNEL_ARG_TYPE_CONST)
                           }
         }
      struct clc_libclc {
         };
      struct clc_libclc *
   clc_libclc_new(const struct clc_logger *logger, const struct clc_libclc_options *options)
   {
      struct clc_libclc *ctx = rzalloc(NULL, struct clc_libclc);
   if (!ctx) {
      clc_error(logger, "D3D12: failed to allocate a clc_libclc");
               const struct spirv_to_nir_options libclc_spirv_options = {
      .environment = NIR_SPIRV_OPENCL,
   .create_library = true,
   .constant_addr_format = nir_address_format_32bit_index_offset_pack64,
   .global_addr_format = nir_address_format_32bit_index_offset_pack64,
   .shared_addr_format = nir_address_format_32bit_offset_as_64bit,
   .temp_addr_format = nir_address_format_32bit_offset_as_64bit,
   .float_controls_execution_mode = FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32,
   .caps = {
      .address = true,
   .float64 = true,
   .int8 = true,
   .int16 = true,
   .int64 = true,
                  glsl_type_singleton_init_or_ref();
   bool optimize = options && options->optimize;
   nir_shader *s =
         if (!s) {
      clc_error(logger, "D3D12: spirv_to_nir failed on libclc blob");
   ralloc_free(ctx);
               ralloc_steal(ctx, s);
               }
      void clc_free_libclc(struct clc_libclc *ctx)
   {
      ralloc_free(ctx);
      }
      const nir_shader *clc_libclc_get_clc_shader(struct clc_libclc *ctx)
   {
         }
      void clc_libclc_serialize(struct clc_libclc *context,
               {
      struct blob tmp;
   blob_init(&tmp);
               }
      void clc_libclc_free_serialized(void *serialized)
   {
         }
      struct clc_libclc *
   clc_libclc_deserialize(const void *serialized, size_t serialized_size)
   {
      struct clc_libclc *ctx = rzalloc(NULL, struct clc_libclc);
   if (!ctx) {
                           struct blob_reader tmp;
            nir_shader *s = nir_deserialize(NULL, NULL, &tmp);
   if (!s) {
      ralloc_free(ctx);
               ralloc_steal(ctx, s);
               }
      bool
   clc_compile_c_to_spir(const struct clc_compile_args *args,
               {
         }
      void
   clc_free_spir(struct clc_binary *spir)
   {
         }
      bool
   clc_compile_spir_to_spirv(const struct clc_binary *in_spir,
               {
      if (clc_spir_to_spirv(in_spir, logger, out_spirv) < 0)
            if (debug_get_option_debug_clc() & CLC_DEBUG_DUMP_SPIRV)
               }
      void
   clc_free_spirv(struct clc_binary *spirv)
   {
         }
      bool
   clc_compile_c_to_spirv(const struct clc_compile_args *args,
               {
      if (clc_c_to_spirv(args, logger, out_spirv) < 0)
            if (debug_get_option_debug_clc() & CLC_DEBUG_DUMP_SPIRV)
               }
      bool
   clc_link_spirv(const struct clc_linker_args *args,
               {
      if (clc_link_spirv_binaries(args, logger, out_spirv) < 0)
            if (debug_get_option_debug_clc() & CLC_DEBUG_DUMP_SPIRV)
               }
      bool
   clc_parse_spirv(const struct clc_binary *in_spirv,
               {
      if (!clc_spirv_get_kernels_info(in_spirv,
      &out_data->kernels,
   &out_data->num_kernels,
   &out_data->spec_constants,
   &out_data->num_spec_constants,
   logger))
         if (debug_get_option_debug_clc() & CLC_DEBUG_VERBOSE)
               }
      void clc_free_parsed_spirv(struct clc_parsed_spirv *data)
   {
         }
      bool
   clc_specialize_spirv(const struct clc_binary *in_spirv,
                     {
      if (!clc_spirv_specialize(in_spirv, parsed_data, consts, out_spirv))
            if (debug_get_option_debug_clc() & CLC_DEBUG_DUMP_SPIRV)
               }
