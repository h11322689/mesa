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
      #include "nir_to_dxil.h"
      #include "dxil_container.h"
   #include "dxil_dump.h"
   #include "dxil_enums.h"
   #include "dxil_function.h"
   #include "dxil_module.h"
   #include "dxil_nir.h"
   #include "dxil_signature.h"
      #include "nir/nir_builder.h"
   #include "nir_deref.h"
   #include "util/ralloc.h"
   #include "util/u_debug.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
      #include "git_sha1.h"
      #include "vulkan/vulkan_core.h"
      #include <stdint.h>
      int debug_dxil = 0;
      static const struct debug_named_value
   dxil_debug_options[] = {
      { "verbose", DXIL_DEBUG_VERBOSE, NULL },
   { "dump_blob",  DXIL_DEBUG_DUMP_BLOB , "Write shader blobs" },
   { "trace",  DXIL_DEBUG_TRACE , "Trace instruction conversion" },
   { "dump_module", DXIL_DEBUG_DUMP_MODULE, "dump module tree to stderr"},
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(debug_dxil, "DXIL_DEBUG", dxil_debug_options, 0)
      static void
   log_nir_instr_unsupported(const struct dxil_logger *logger,
         {
      char *msg = NULL;
   char *instr_str = nir_instr_as_str(instr, NULL);
   asprintf(&msg, "%s: %s\n", message_prefix, instr_str);
   ralloc_free(instr_str);
   assert(msg);
   logger->log(logger->priv, msg);
      }
      static void
   default_logger_func(void *priv, const char *msg)
   {
      fprintf(stderr, "%s", msg);
      }
      static const struct dxil_logger default_logger = { .priv = NULL, .log = default_logger_func };
      #define TRACE_CONVERSION(instr) \
      if (debug_dxil & DXIL_DEBUG_TRACE) \
      do { \
      fprintf(stderr, "Convert '"); \
   nir_print_instr(instr, stderr); \
         static const nir_shader_compiler_options
   nir_options = {
      .lower_ineg = true,
   .lower_fneg = true,
   .lower_ffma16 = true,
   .lower_ffma32 = true,
   .lower_isign = true,
   .lower_fsign = true,
   .lower_iabs = true,
   .lower_fmod = true,
   .lower_fpow = true,
   .lower_scmp = true,
   .lower_ldexp = true,
   .lower_flrp16 = true,
   .lower_flrp32 = true,
   .lower_flrp64 = true,
   .lower_bitfield_extract = true,
   .lower_ifind_msb = true,
   .lower_ufind_msb = true,
   .lower_extract_word = true,
   .lower_extract_byte = true,
   .lower_insert_word = true,
   .lower_insert_byte = true,
   .lower_all_io_to_elements = true,
   .lower_all_io_to_temps = true,
   .lower_hadd = true,
   .lower_uadd_sat = true,
   .lower_usub_sat = true,
   .lower_iadd_sat = true,
   .lower_uadd_carry = true,
   .lower_usub_borrow = true,
   .lower_mul_high = true,
   .lower_pack_half_2x16 = true,
   .lower_pack_unorm_4x8 = true,
   .lower_pack_snorm_4x8 = true,
   .lower_pack_snorm_2x16 = true,
   .lower_pack_unorm_2x16 = true,
   .lower_pack_64_2x32_split = true,
   .lower_pack_32_2x16_split = true,
   .lower_pack_64_4x16 = true,
   .lower_unpack_64_2x32_split = true,
   .lower_unpack_32_2x16_split = true,
   .lower_unpack_half_2x16 = true,
   .lower_unpack_snorm_2x16 = true,
   .lower_unpack_snorm_4x8 = true,
   .lower_unpack_unorm_2x16 = true,
   .lower_unpack_unorm_4x8 = true,
   .lower_interpolate_at = true,
   .has_fsub = true,
   .has_isub = true,
   .has_bfe = true,
   .has_find_msb_rev = true,
   .vertex_id_zero_based = true,
   .lower_base_vertex = true,
   .lower_helper_invocation = true,
   .has_cs_global_id = true,
   .lower_mul_2x32_64 = true,
   .lower_doubles_options =
      nir_lower_drcp |
   nir_lower_dsqrt |
   nir_lower_drsq |
   nir_lower_dfract |
   nir_lower_dtrunc |
   nir_lower_dfloor |
   nir_lower_dceil |
      .max_unroll_iterations = 32, /* arbitrary */
   .force_indirect_unrolling = (nir_var_shader_in | nir_var_shader_out),
   .lower_device_index_to_zero = true,
   .linker_ignore_precision = true,
   .support_16bit_alu = true,
      };
      const nir_shader_compiler_options*
   dxil_get_base_nir_compiler_options(void)
   {
         }
      void
   dxil_get_nir_compiler_options(nir_shader_compiler_options *options,
                     {
      *options = nir_options;
   if (!(supported_int_sizes & 64)) {
      options->lower_pack_64_2x32_split = false;
   options->lower_unpack_64_2x32_split = false;
      }
   if (!(supported_float_sizes & 64))
         if (shader_model_max >= SHADER_MODEL_6_4) {
      options->has_sdot_4x8 = true;
         }
      static bool
   emit_llvm_ident(struct dxil_module *m)
   {
      const struct dxil_mdnode *compiler = dxil_get_metadata_string(m, "Mesa version " PACKAGE_VERSION MESA_GIT_SHA1);
   if (!compiler)
            const struct dxil_mdnode *llvm_ident = dxil_get_metadata_node(m, &compiler, 1);
   return llvm_ident &&
      }
      static bool
   emit_named_version(struct dxil_module *m, const char *name,
         {
      const struct dxil_mdnode *major_node = dxil_get_metadata_int32(m, major);
   const struct dxil_mdnode *minor_node = dxil_get_metadata_int32(m, minor);
   const struct dxil_mdnode *version_nodes[] = { major_node, minor_node };
   const struct dxil_mdnode *version = dxil_get_metadata_node(m, version_nodes,
            }
      static const char *
   get_shader_kind_str(enum dxil_shader_kind kind)
   {
      switch (kind) {
   case DXIL_PIXEL_SHADER:
         case DXIL_VERTEX_SHADER:
         case DXIL_GEOMETRY_SHADER:
         case DXIL_HULL_SHADER:
         case DXIL_DOMAIN_SHADER:
         case DXIL_COMPUTE_SHADER:
         default:
            }
      static bool
   emit_dx_shader_model(struct dxil_module *m)
   {
      const struct dxil_mdnode *type_node = dxil_get_metadata_string(m, get_shader_kind_str(m->shader_kind));
   const struct dxil_mdnode *major_node = dxil_get_metadata_int32(m, m->major_version);
   const struct dxil_mdnode *minor_node = dxil_get_metadata_int32(m, m->minor_version);
   const struct dxil_mdnode *shader_model[] = { type_node, major_node,
                  return dxil_add_metadata_named_node(m, "dx.shaderModel",
      }
      enum {
      DXIL_TYPED_BUFFER_ELEMENT_TYPE_TAG = 0,
      };
      enum dxil_intr {
      DXIL_INTR_LOAD_INPUT = 4,
   DXIL_INTR_STORE_OUTPUT = 5,
   DXIL_INTR_FABS = 6,
            DXIL_INTR_ISFINITE = 10,
            DXIL_INTR_FCOS = 12,
            DXIL_INTR_FEXP2 = 21,
   DXIL_INTR_FRC = 22,
            DXIL_INTR_SQRT = 24,
   DXIL_INTR_RSQRT = 25,
   DXIL_INTR_ROUND_NE = 26,
   DXIL_INTR_ROUND_NI = 27,
   DXIL_INTR_ROUND_PI = 28,
            DXIL_INTR_BFREV = 30,
   DXIL_INTR_COUNTBITS = 31,
   DXIL_INTR_FIRSTBIT_LO = 32,
   DXIL_INTR_FIRSTBIT_HI = 33,
            DXIL_INTR_FMAX = 35,
   DXIL_INTR_FMIN = 36,
   DXIL_INTR_IMAX = 37,
   DXIL_INTR_IMIN = 38,
   DXIL_INTR_UMAX = 39,
                     DXIL_INTR_IBFE = 51,
   DXIL_INTR_UBFE = 52,
            DXIL_INTR_CREATE_HANDLE = 57,
            DXIL_INTR_SAMPLE = 60,
   DXIL_INTR_SAMPLE_BIAS = 61,
   DXIL_INTR_SAMPLE_LEVEL = 62,
   DXIL_INTR_SAMPLE_GRAD = 63,
   DXIL_INTR_SAMPLE_CMP = 64,
            DXIL_INTR_TEXTURE_LOAD = 66,
            DXIL_INTR_BUFFER_LOAD = 68,
            DXIL_INTR_TEXTURE_SIZE = 72,
   DXIL_INTR_TEXTURE_GATHER = 73,
            DXIL_INTR_TEXTURE2DMS_GET_SAMPLE_POSITION = 75,
   DXIL_INTR_RENDER_TARGET_GET_SAMPLE_POSITION = 76,
            DXIL_INTR_ATOMIC_BINOP = 78,
   DXIL_INTR_ATOMIC_CMPXCHG = 79,
   DXIL_INTR_BARRIER = 80,
            DXIL_INTR_DISCARD = 82,
   DXIL_INTR_DDX_COARSE = 83,
   DXIL_INTR_DDY_COARSE = 84,
   DXIL_INTR_DDX_FINE = 85,
            DXIL_INTR_EVAL_SNAPPED = 87,
   DXIL_INTR_EVAL_SAMPLE_INDEX = 88,
            DXIL_INTR_SAMPLE_INDEX = 90,
            DXIL_INTR_THREAD_ID = 93,
   DXIL_INTR_GROUP_ID = 94,
   DXIL_INTR_THREAD_ID_IN_GROUP = 95,
            DXIL_INTR_EMIT_STREAM = 97,
                     DXIL_INTR_MAKE_DOUBLE = 101,
            DXIL_INTR_LOAD_OUTPUT_CONTROL_POINT = 103,
   DXIL_INTR_LOAD_PATCH_CONSTANT = 104,
   DXIL_INTR_DOMAIN_LOCATION = 105,
   DXIL_INTR_STORE_PATCH_CONSTANT = 106,
   DXIL_INTR_OUTPUT_CONTROL_POINT_ID = 107,
            DXIL_INTR_WAVE_IS_FIRST_LANE = 110,
   DXIL_INTR_WAVE_GET_LANE_INDEX = 111,
   DXIL_INTR_WAVE_GET_LANE_COUNT = 112,
   DXIL_INTR_WAVE_ANY_TRUE = 113,
   DXIL_INTR_WAVE_ALL_TRUE = 114,
   DXIL_INTR_WAVE_ACTIVE_ALL_EQUAL = 115,
   DXIL_INTR_WAVE_ACTIVE_BALLOT = 116,
   DXIL_INTR_WAVE_READ_LANE_AT = 117,
   DXIL_INTR_WAVE_READ_LANE_FIRST = 118,
   DXIL_INTR_WAVE_ACTIVE_OP = 119,
   DXIL_INTR_WAVE_ACTIVE_BIT = 120,
   DXIL_INTR_WAVE_PREFIX_OP = 121,
   DXIL_INTR_QUAD_READ_LANE_AT = 122,
            DXIL_INTR_LEGACY_F32TOF16 = 130,
            DXIL_INTR_ATTRIBUTE_AT_VERTEX = 137,
            DXIL_INTR_RAW_BUFFER_LOAD = 139,
            DXIL_INTR_DOT4_ADD_I8_PACKED = 163,
            DXIL_INTR_ANNOTATE_HANDLE = 216,
   DXIL_INTR_CREATE_HANDLE_FROM_BINDING = 217,
            DXIL_INTR_IS_HELPER_LANE = 221,
      };
      enum dxil_atomic_op {
      DXIL_ATOMIC_ADD = 0,
   DXIL_ATOMIC_AND = 1,
   DXIL_ATOMIC_OR = 2,
   DXIL_ATOMIC_XOR = 3,
   DXIL_ATOMIC_IMIN = 4,
   DXIL_ATOMIC_IMAX = 5,
   DXIL_ATOMIC_UMIN = 6,
   DXIL_ATOMIC_UMAX = 7,
      };
      static enum dxil_atomic_op
   nir_atomic_to_dxil_atomic(nir_atomic_op op)
   {
      switch (op) {
   case nir_atomic_op_iadd: return DXIL_ATOMIC_ADD;
   case nir_atomic_op_iand: return DXIL_ATOMIC_AND;
   case nir_atomic_op_ior: return DXIL_ATOMIC_OR;
   case nir_atomic_op_ixor: return DXIL_ATOMIC_XOR;
   case nir_atomic_op_imin: return DXIL_ATOMIC_IMIN;
   case nir_atomic_op_imax: return DXIL_ATOMIC_IMAX;
   case nir_atomic_op_umin: return DXIL_ATOMIC_UMIN;
   case nir_atomic_op_umax: return DXIL_ATOMIC_UMAX;
   case nir_atomic_op_xchg: return DXIL_ATOMIC_EXCHANGE;
   default: unreachable("Unsupported atomic op");
      }
      static enum dxil_rmw_op
   nir_atomic_to_dxil_rmw(nir_atomic_op op)
   {
      switch (op) {
   case nir_atomic_op_iadd: return DXIL_RMWOP_ADD;
   case nir_atomic_op_iand: return DXIL_RMWOP_AND;
   case nir_atomic_op_ior: return DXIL_RMWOP_OR;
   case nir_atomic_op_ixor: return DXIL_RMWOP_XOR;
   case nir_atomic_op_imin: return DXIL_RMWOP_MIN;
   case nir_atomic_op_imax: return DXIL_RMWOP_MAX;
   case nir_atomic_op_umin: return DXIL_RMWOP_UMIN;
   case nir_atomic_op_umax: return DXIL_RMWOP_UMAX;
   case nir_atomic_op_xchg: return DXIL_RMWOP_XCHG;
   default: unreachable("Unsupported atomic op");
      }
      typedef struct {
      unsigned id;
   unsigned binding;
   unsigned size;
      } resource_array_layout;
      static void
   fill_resource_metadata(struct dxil_module *m, const struct dxil_mdnode **fields,
               {
      const struct dxil_type *pointer_type = dxil_module_get_pointer_type(m, struct_type);
            fields[0] = dxil_get_metadata_int32(m, layout->id); // resource ID
   fields[1] = dxil_get_metadata_value(m, pointer_type, pointer_undef); // global constant symbol
   fields[2] = dxil_get_metadata_string(m, name ? name : ""); // name
   fields[3] = dxil_get_metadata_int32(m, layout->space); // space ID
   fields[4] = dxil_get_metadata_int32(m, layout->binding); // lower bound
      }
      static const struct dxil_mdnode *
   emit_srv_metadata(struct dxil_module *m, const struct dxil_type *elem_type,
                     {
                        fill_resource_metadata(m, fields, elem_type, name, layout);
   fields[6] = dxil_get_metadata_int32(m, res_kind); // resource shape
   fields[7] = dxil_get_metadata_int1(m, 0); // sample count
   if (res_kind != DXIL_RESOURCE_KIND_RAW_BUFFER &&
      res_kind != DXIL_RESOURCE_KIND_STRUCTURED_BUFFER) {
   metadata_tag_nodes[0] = dxil_get_metadata_int32(m, DXIL_TYPED_BUFFER_ELEMENT_TYPE_TAG);
   metadata_tag_nodes[1] = dxil_get_metadata_int32(m, comp_type);
      } else if (res_kind == DXIL_RESOURCE_KIND_RAW_BUFFER)
         else
               }
      static const struct dxil_mdnode *
   emit_uav_metadata(struct dxil_module *m, const struct dxil_type *struct_type,
                     {
                        fill_resource_metadata(m, fields, struct_type, name, layout);
   fields[6] = dxil_get_metadata_int32(m, res_kind); // resource shape
   fields[7] = dxil_get_metadata_int1(m, false); // globally-coherent
   fields[8] = dxil_get_metadata_int1(m, false); // has counter
   fields[9] = dxil_get_metadata_int1(m, false); // is ROV
   if (res_kind != DXIL_RESOURCE_KIND_RAW_BUFFER &&
      res_kind != DXIL_RESOURCE_KIND_STRUCTURED_BUFFER) {
   metadata_tag_nodes[0] = dxil_get_metadata_int32(m, DXIL_TYPED_BUFFER_ELEMENT_TYPE_TAG);
   metadata_tag_nodes[1] = dxil_get_metadata_int32(m, comp_type);
      } else if (res_kind == DXIL_RESOURCE_KIND_RAW_BUFFER)
         else
               }
      static const struct dxil_mdnode *
   emit_cbv_metadata(struct dxil_module *m, const struct dxil_type *struct_type,
               {
               fill_resource_metadata(m, fields, struct_type, name, layout);
   fields[6] = dxil_get_metadata_int32(m, size); // constant buffer size
               }
      static const struct dxil_mdnode *
   emit_sampler_metadata(struct dxil_module *m, const struct dxil_type *struct_type,
         {
      const struct dxil_mdnode *fields[8];
            fill_resource_metadata(m, fields, struct_type, var->name, layout);
   enum dxil_sampler_kind sampler_kind = glsl_sampler_type_is_shadow(type) ?
         fields[6] = dxil_get_metadata_int32(m, sampler_kind); // sampler kind
               }
         #define MAX_SRVS 128
   #define MAX_UAVS 64
   #define MAX_CBVS 64 // ??
   #define MAX_SAMPLERS 64 // ??
      struct dxil_def {
         };
      struct ntd_context {
      void *ralloc_ctx;
   const struct nir_to_dxil_options *opts;
                     struct util_dynarray srv_metadata_nodes;
            struct util_dynarray uav_metadata_nodes;
   const struct dxil_value *ssbo_handles[MAX_UAVS];
   const struct dxil_value *image_handles[MAX_UAVS];
            struct util_dynarray cbv_metadata_nodes;
            struct util_dynarray sampler_metadata_nodes;
                     const struct dxil_mdnode *shader_property_nodes[6];
            struct dxil_def *defs;
   unsigned num_defs;
            const struct dxil_value **sharedvars;
   const struct dxil_value **scratchvars;
            nir_variable *ps_front_face;
            nir_function *tess_ctrl_patch_constant_func;
            struct dxil_func_def *main_func_def;
   struct dxil_func_def *tess_ctrl_patch_constant_func_def;
            BITSET_WORD *float_types;
               };
      static const char*
   unary_func_name(enum dxil_intr intr)
   {
      switch (intr) {
   case DXIL_INTR_COUNTBITS:
   case DXIL_INTR_FIRSTBIT_HI:
   case DXIL_INTR_FIRSTBIT_SHI:
   case DXIL_INTR_FIRSTBIT_LO:
         case DXIL_INTR_ISFINITE:
   case DXIL_INTR_ISNORMAL:
         default:
            }
      static const struct dxil_value *
   emit_unary_call(struct ntd_context *ctx, enum overload_type overload,
               {
      const struct dxil_func *func = dxil_get_function(&ctx->mod,
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, intr);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   op0
               }
      static const struct dxil_value *
   emit_binary_call(struct ntd_context *ctx, enum overload_type overload,
               {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.binary", overload);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, intr);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   op0,
   op1
               }
      static const struct dxil_value *
   emit_tertiary_call(struct ntd_context *ctx, enum overload_type overload,
                     enum dxil_intr intr,
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.tertiary", overload);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, intr);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   op0,
   op1,
   op2
               }
      static const struct dxil_value *
   emit_quaternary_call(struct ntd_context *ctx, enum overload_type overload,
                        enum dxil_intr intr,
      {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.quaternary", overload);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, intr);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   op0,
   op1,
   op2,
   op3
               }
      static const struct dxil_value *
   emit_threadid_call(struct ntd_context *ctx, const struct dxil_value *comp)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.threadId", DXIL_I32);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   comp
               }
      static const struct dxil_value *
   emit_threadidingroup_call(struct ntd_context *ctx,
         {
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   comp
               }
      static const struct dxil_value *
   emit_flattenedthreadidingroup_call(struct ntd_context *ctx)
   {
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         if (!opcode)
            const struct dxil_value *args[] = {
   opcode
               }
      static const struct dxil_value *
   emit_groupid_call(struct ntd_context *ctx, const struct dxil_value *comp)
   {
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   comp
               }
      static const struct dxil_value *
   emit_raw_bufferload_call(struct ntd_context *ctx,
                           const struct dxil_value *handle,
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.rawBufferLoad", overload);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         const struct dxil_value *args[] = {
      opcode, handle, coord[0], coord[1],
   dxil_module_get_int8_const(&ctx->mod, (1 << component_count) - 1),
                  }
      static const struct dxil_value *
   emit_bufferload_call(struct ntd_context *ctx,
                     {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.bufferLoad", overload);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
                     }
      static bool
   emit_raw_bufferstore_call(struct ntd_context *ctx,
                           const struct dxil_value *handle,
   const struct dxil_value *coord[2],
   {
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         const struct dxil_value *args[] = {
      opcode, handle, coord[0], coord[1],
   value[0], value[1], value[2], value[3],
   write_mask,
               return dxil_emit_call_void(&ctx->mod, func,
      }
      static bool
   emit_bufferstore_call(struct ntd_context *ctx,
                        const struct dxil_value *handle,
      {
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         const struct dxil_value *args[] = {
      opcode, handle, coord[0], coord[1],
   value[0], value[1], value[2], value[3],
               return dxil_emit_call_void(&ctx->mod, func,
      }
      static const struct dxil_value *
   emit_textureload_call(struct ntd_context *ctx,
                     {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.textureLoad", overload);
   if (!func)
         const struct dxil_type *int_type = dxil_module_get_int_type(&ctx->mod, 32);
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         const struct dxil_value *args[] = { opcode, handle,
      /*lod_or_sample*/ int_undef,
   coord[0], coord[1], coord[2],
            }
      static bool
   emit_texturestore_call(struct ntd_context *ctx,
                        const struct dxil_value *handle,
      {
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod,
         const struct dxil_value *args[] = {
      opcode, handle, coord[0], coord[1], coord[2],
   value[0], value[1], value[2], value[3],
               return dxil_emit_call_void(&ctx->mod, func,
      }
      static const struct dxil_value *
   emit_atomic_binop(struct ntd_context *ctx,
                     const struct dxil_value *handle,
   {
               if (!func)
            const struct dxil_value *opcode =
         const struct dxil_value *atomic_op_value =
         const struct dxil_value *args[] = {
      opcode, handle, atomic_op_value,
                  }
      static const struct dxil_value *
   emit_atomic_cmpxchg(struct ntd_context *ctx,
                     const struct dxil_value *handle,
   {
      const struct dxil_func *func =
            if (!func)
            const struct dxil_value *opcode =
         const struct dxil_value *args[] = {
                     }
      static const struct dxil_value *
   emit_createhandle_call_pre_6_6(struct ntd_context *ctx,
                                 enum dxil_resource_class resource_class,
   unsigned lower_bound,
   {
      const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_CREATE_HANDLE);
   const struct dxil_value *resource_class_value = dxil_module_get_int8_const(&ctx->mod, resource_class);
   const struct dxil_value *resource_range_id_value = dxil_module_get_int32_const(&ctx->mod, resource_range_id);
   const struct dxil_value *non_uniform_resource_index_value = dxil_module_get_int1_const(&ctx->mod, non_uniform_resource_index);
   if (!opcode || !resource_class_value || !resource_range_id_value ||
      !non_uniform_resource_index_value)
         const struct dxil_value *args[] = {
      opcode,
   resource_class_value,
   resource_range_id_value,
   resource_range_index,
               const struct dxil_func *func =
            if (!func)
               }
      static const struct dxil_value *
   emit_annotate_handle(struct ntd_context *ctx,
               {
      const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_ANNOTATE_HANDLE);
   if (!opcode)
            const struct dxil_value *args[] = {
      opcode,
   unannotated_handle,
               const struct dxil_func *func =
            if (!func)
               }
      static const struct dxil_value *
   emit_annotate_handle_from_metadata(struct ntd_context *ctx,
                     {
         const struct util_dynarray *mdnodes;
   switch (resource_class) {
   case DXIL_RESOURCE_CLASS_SRV:
      mdnodes = &ctx->srv_metadata_nodes;
      case DXIL_RESOURCE_CLASS_UAV:
      mdnodes = &ctx->uav_metadata_nodes;
      case DXIL_RESOURCE_CLASS_CBV:
      mdnodes = &ctx->cbv_metadata_nodes;
      case DXIL_RESOURCE_CLASS_SAMPLER:
      mdnodes = &ctx->sampler_metadata_nodes;
      default:
                  const struct dxil_mdnode *mdnode = *util_dynarray_element(mdnodes, const struct dxil_mdnode *, resource_range_id);
   const struct dxil_value *res_props = dxil_module_get_res_props_const(&ctx->mod, resource_class, mdnode);
   if (!res_props)
               }
      static const struct dxil_value *
   emit_createhandle_and_annotate(struct ntd_context *ctx,
                                 enum dxil_resource_class resource_class,
   unsigned lower_bound,
   {
      const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_CREATE_HANDLE_FROM_BINDING);
   const struct dxil_value *res_bind = dxil_module_get_res_bind_const(&ctx->mod, lower_bound, upper_bound, space, resource_class);
   const struct dxil_value *non_uniform_resource_index_value = dxil_module_get_int1_const(&ctx->mod, non_uniform_resource_index);
   if (!opcode || !res_bind || !non_uniform_resource_index_value)
            const struct dxil_value *args[] = {
      opcode,
   res_bind,
   resource_range_index,
               const struct dxil_func *func =
            if (!func)
            const struct dxil_value *unannotated_handle = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!unannotated_handle)
               }
      static const struct dxil_value *
   emit_createhandle_call(struct ntd_context *ctx,
                        enum dxil_resource_class resource_class,
   unsigned lower_bound,
   unsigned upper_bound,
      {
      if (ctx->mod.minor_version < 6)
         else
      }
      static const struct dxil_value *
   emit_createhandle_call_const_index(struct ntd_context *ctx,
                                    enum dxil_resource_class resource_class,
      {
         const struct dxil_value *resource_range_index_value = dxil_module_get_int32_const(&ctx->mod, resource_range_index);
   if (!resource_range_index_value)
            return emit_createhandle_call(ctx, resource_class, lower_bound, upper_bound, space,
            }
      static const struct dxil_value *
   emit_createhandle_heap(struct ntd_context *ctx,
                     {
      if (is_sampler)
         else
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_CREATE_HANDLE_FROM_HEAP);
   const struct dxil_value *sampler = dxil_module_get_int1_const(&ctx->mod, is_sampler);
   const struct dxil_value *non_uniform_resource_index_value = dxil_module_get_int1_const(&ctx->mod, non_uniform_resource_index);
   if (!opcode || !sampler || !non_uniform_resource_index_value)
            const struct dxil_value *args[] = {
      opcode,
   resource_range_index,
   sampler,
               const struct dxil_func *func =
            if (!func)
               }
      static void
   add_resource(struct ntd_context *ctx, enum dxil_resource_type type,
               {
      struct dxil_resource_v0 *resource_v0 = NULL;
   struct dxil_resource_v1 *resource_v1 = NULL;
   if (ctx->mod.minor_validator >= 6) {
      resource_v1 = util_dynarray_grow(&ctx->resources, struct dxil_resource_v1, 1);
      } else {
         }
   resource_v0->resource_type = type;
   resource_v0->space = layout->space;
   resource_v0->lower_bound = layout->binding;
   if (layout->size == 0 || (uint64_t)layout->size + layout->binding >= UINT_MAX)
         else
         if (type == DXIL_RES_UAV_TYPED ||
      type == DXIL_RES_UAV_RAW ||
   type == DXIL_RES_UAV_STRUCTURED) {
   uint32_t new_uav_count = ctx->num_uavs + layout->size;
   if (layout->size == 0 || new_uav_count < ctx->num_uavs)
         else
         if (ctx->mod.minor_validator >= 6 && ctx->num_uavs > 8)
               if (resource_v1) {
      resource_v1->resource_kind = kind;
   /* No flags supported yet */
         }
      static const struct dxil_value *
   emit_createhandle_call_dynamic(struct ntd_context *ctx,
                                 {
      unsigned offset = 0;
            unsigned num_srvs = util_dynarray_num_elements(&ctx->srv_metadata_nodes, const struct dxil_mdnode *);
   unsigned num_uavs = util_dynarray_num_elements(&ctx->uav_metadata_nodes, const struct dxil_mdnode *);
   unsigned num_cbvs = util_dynarray_num_elements(&ctx->cbv_metadata_nodes, const struct dxil_mdnode *);
            switch (resource_class) {
   case DXIL_RESOURCE_CLASS_UAV:
      offset = num_srvs + num_samplers + num_cbvs;
   count = num_uavs;
      case DXIL_RESOURCE_CLASS_SRV:
      offset = num_samplers + num_cbvs;
   count = num_srvs;
      case DXIL_RESOURCE_CLASS_SAMPLER:
      offset = num_cbvs;
   count = num_samplers;
      case DXIL_RESOURCE_CLASS_CBV:
      offset = 0;
   count = num_cbvs;
               unsigned resource_element_size = ctx->mod.minor_validator >= 6 ?
         assert(offset + count <= ctx->resources.size / resource_element_size);
   for (unsigned i = offset; i < offset + count; ++i) {
      const struct dxil_resource_v0 *resource = (const struct dxil_resource_v0 *)((const char *)ctx->resources.data + resource_element_size * i);
   if (resource->space == space &&
      resource->lower_bound <= binding &&
   resource->upper_bound >= binding) {
   return emit_createhandle_call(ctx, resource_class, resource->lower_bound,
                                       }
      static bool
   emit_srv(struct ntd_context *ctx, nir_variable *var, unsigned count)
   {
      unsigned id = util_dynarray_num_elements(&ctx->srv_metadata_nodes, const struct dxil_mdnode *);
   unsigned binding = var->data.binding;
            enum dxil_component_type comp_type;
   enum dxil_resource_kind res_kind;
   enum dxil_resource_type res_type;
   if (var->data.mode == nir_var_mem_ssbo) {
      comp_type = DXIL_COMP_TYPE_INVALID;
   res_kind = DXIL_RESOURCE_KIND_RAW_BUFFER;
      } else {
      comp_type = dxil_get_comp_type(var->type);
   res_kind = dxil_get_resource_kind(var->type);
      }
            if (glsl_type_is_array(var->type))
            const struct dxil_mdnode *srv_meta = emit_srv_metadata(&ctx->mod, res_type_as_type, var->name,
            if (!srv_meta)
            util_dynarray_append(&ctx->srv_metadata_nodes, const struct dxil_mdnode *, srv_meta);
   add_resource(ctx, res_type, res_kind, &layout);
   if (res_type == DXIL_RES_SRV_RAW)
               }
      static bool
   emit_globals(struct ntd_context *ctx, unsigned size)
   {
      nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_ssbo)
            if (!size)
            const struct dxil_type *struct_type = dxil_module_get_res_type(&ctx->mod,
         if (!struct_type)
            const struct dxil_type *array_type =
         if (!array_type)
            resource_array_layout layout = {0, 0, size, 0};
   const struct dxil_mdnode *uav_meta =
      emit_uav_metadata(&ctx->mod, array_type,
                  if (!uav_meta)
            util_dynarray_append(&ctx->uav_metadata_nodes, const struct dxil_mdnode *, uav_meta);
   if (ctx->mod.minor_validator < 6 &&
      util_dynarray_num_elements(&ctx->uav_metadata_nodes, const struct dxil_mdnode *) > 8)
      /* Handles to UAVs used for kernel globals are created on-demand */
   add_resource(ctx, DXIL_RES_UAV_RAW, DXIL_RESOURCE_KIND_RAW_BUFFER, &layout);
   ctx->mod.raw_and_structured_buffers = true;
      }
      static bool
   emit_uav(struct ntd_context *ctx, unsigned binding, unsigned space, unsigned count,
               {
      unsigned id = util_dynarray_num_elements(&ctx->uav_metadata_nodes, const struct dxil_mdnode *);
            const struct dxil_type *res_type = dxil_module_get_res_type(&ctx->mod, res_kind, comp_type, num_comps, true /* readwrite */);
   res_type = dxil_module_get_array_type(&ctx->mod, res_type, count);
   const struct dxil_mdnode *uav_meta = emit_uav_metadata(&ctx->mod, res_type, name,
            if (!uav_meta)
            util_dynarray_append(&ctx->uav_metadata_nodes, const struct dxil_mdnode *, uav_meta);
   if (ctx->mod.minor_validator < 6 &&
      util_dynarray_num_elements(&ctx->uav_metadata_nodes, const struct dxil_mdnode *) > 8)
         add_resource(ctx, res_kind == DXIL_RESOURCE_KIND_RAW_BUFFER ? DXIL_RES_UAV_RAW : DXIL_RES_UAV_TYPED, res_kind, &layout);
   if (res_kind == DXIL_RESOURCE_KIND_RAW_BUFFER)
         if (ctx->mod.shader_kind != DXIL_PIXEL_SHADER &&
      ctx->mod.shader_kind != DXIL_COMPUTE_SHADER)
            }
      static bool
   emit_uav_var(struct ntd_context *ctx, nir_variable *var, unsigned count)
   {
      unsigned binding, space;
   if (ctx->opts->environment == DXIL_ENVIRONMENT_GL) {
      /* For GL, the image intrinsics are already lowered, using driver_location
   * as the 0-based image index. Use space 1 so that we can keep using these
   * NIR constants without having to remap them, and so they don't overlap
   * SSBOs, which are also 0-based UAV bindings.
   */
   binding = var->data.driver_location;
      } else {
      binding = var->data.binding;
      }
   enum dxil_component_type comp_type = dxil_get_comp_type(var->type);
   enum dxil_resource_kind res_kind = dxil_get_resource_kind(var->type);
            return emit_uav(ctx, binding, space, count, comp_type,
            }
      static const struct dxil_value *
   get_value_for_const(struct dxil_module *mod, nir_const_value *c, const struct dxil_type *type)
   {
      if (type == mod->int1_type) return dxil_module_get_int1_const(mod, c->b);
   if (type == mod->float32_type) return dxil_module_get_float_const(mod, c->f32);
   if (type == mod->int32_type) return dxil_module_get_int32_const(mod, c->i32);
   if (type == mod->int16_type) {
      mod->feats.min_precision = true;
      }
   if (type == mod->int64_type) {
      mod->feats.int64_ops = true;
      }
   if (type == mod->float16_type) {
      mod->feats.min_precision = true;
      }
   if (type == mod->float64_type) {
      mod->feats.doubles = true;
      }
      }
      static const struct dxil_type *
   get_type_for_glsl_base_type(struct dxil_module *mod, enum glsl_base_type type)
   {
      uint32_t bit_size = glsl_base_type_bit_size(type);
   if (nir_alu_type_get_base_type(nir_get_nir_type_for_glsl_base_type(type)) == nir_type_float)
            }
      static const struct dxil_type *
   get_type_for_glsl_type(struct dxil_module *mod, const struct glsl_type *type)
   {
      if (glsl_type_is_scalar(type))
            if (glsl_type_is_vector(type))
      return dxil_module_get_vector_type(mod, get_type_for_glsl_base_type(mod, glsl_get_base_type(type)),
         if (glsl_type_is_array(type))
      return dxil_module_get_array_type(mod, get_type_for_glsl_type(mod, glsl_get_array_element(type)),
         assert(glsl_type_is_struct(type));
   uint32_t size = glsl_get_length(type);
   const struct dxil_type **fields = calloc(sizeof(const struct dxil_type *), size);
   for (uint32_t i = 0; i < size; ++i)
         const struct dxil_type *ret = dxil_module_get_struct_type(mod, glsl_get_type_name(type), fields, size);
   free((void *)fields);
      }
      static const struct dxil_value *
   get_value_for_const_aggregate(struct dxil_module *mod, nir_constant *c, const struct glsl_type *type)
   {
      const struct dxil_type *dxil_type = get_type_for_glsl_type(mod, type);
   if (glsl_type_is_vector_or_scalar(type)) {
      const struct dxil_type *element_type = get_type_for_glsl_base_type(mod, glsl_get_base_type(type));
   const struct dxil_value *elements[NIR_MAX_VEC_COMPONENTS];
   for (uint32_t i = 0; i < glsl_get_vector_elements(type); ++i)
         if (glsl_type_is_scalar(type))
                     uint32_t num_values = glsl_get_length(type);
   assert(num_values == c->num_elements);
   const struct dxil_value **values = calloc(sizeof(const struct dxil_value *), num_values);
   const struct dxil_value *ret;
   if (glsl_type_is_array(type)) {
      const struct glsl_type *element_type = glsl_get_array_element(type);
   for (uint32_t i = 0; i < num_values; ++i)
            } else {
      for (uint32_t i = 0; i < num_values; ++i)
            }
   free((void *)values);
      }
      static bool
   emit_global_consts(struct ntd_context *ctx)
   {
      uint32_t index = 0;
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_constant) {
      assert(var->constant_initializer);
                        nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_constant) {
      if (!var->name)
            const struct dxil_value *agg_vals =
         if (!agg_vals)
            const struct dxil_value *gvar = dxil_add_global_ptr_var(&ctx->mod, var->name,
                     if (!gvar)
                           }
      static bool
   emit_shared_vars(struct ntd_context *ctx)
   {
      uint32_t index = 0;
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_shared)
                     nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_shared) {
      if (!var->name)
         const struct dxil_value *gvar = dxil_add_global_ptr_var(&ctx->mod, var->name,
                     if (!gvar)
                           }
      static bool
   emit_cbv(struct ntd_context *ctx, unsigned binding, unsigned space,
         {
                        const struct dxil_type *float32 = dxil_module_get_float_type(&ctx->mod, 32);
   const struct dxil_type *array_type = dxil_module_get_array_type(&ctx->mod, float32, size);
   const struct dxil_type *buffer_type = dxil_module_get_struct_type(&ctx->mod, name,
         // All ubo[1]s should have been lowered to ubo with static indexing
   const struct dxil_type *final_type = count != 1 ? dxil_module_get_array_type(&ctx->mod, buffer_type, count) : buffer_type;
   resource_array_layout layout = {idx, binding, count, space};
   const struct dxil_mdnode *cbv_meta = emit_cbv_metadata(&ctx->mod, final_type,
            if (!cbv_meta)
            util_dynarray_append(&ctx->cbv_metadata_nodes, const struct dxil_mdnode *, cbv_meta);
               }
      static bool
   emit_ubo_var(struct ntd_context *ctx, nir_variable *var)
   {
      unsigned count = 1;
   if (glsl_type_is_array(var->type))
            char *name = var->name;
   char temp_name[30];
   if (name && strlen(name) == 0) {
      snprintf(temp_name, sizeof(temp_name), "__unnamed_ubo_%d",
                     const struct glsl_type *type = glsl_without_array(var->type);
   assert(glsl_type_is_struct(type) || glsl_type_is_interface(type));
            return emit_cbv(ctx, var->data.binding, var->data.descriptor_set,
      }
      static bool
   emit_sampler(struct ntd_context *ctx, nir_variable *var, unsigned count)
   {
      unsigned id = util_dynarray_num_elements(&ctx->sampler_metadata_nodes, const struct dxil_mdnode *);
   unsigned binding = var->data.binding;
   resource_array_layout layout = {id, binding, count, var->data.descriptor_set};
   const struct dxil_type *int32_type = dxil_module_get_int_type(&ctx->mod, 32);
            if (glsl_type_is_array(var->type))
                     if (!sampler_meta)
            util_dynarray_append(&ctx->sampler_metadata_nodes, const struct dxil_mdnode *, sampler_meta);
               }
      static bool
   emit_static_indexing_handles(struct ntd_context *ctx)
   {
      /* Vulkan always uses dynamic handles, from instructions in the NIR */
   if (ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN)
            unsigned last_res_class = -1;
            unsigned resource_element_size = ctx->mod.minor_validator >= 6 ?
         for (struct dxil_resource_v0 *res = (struct dxil_resource_v0 *)ctx->resources.data;
      res < (struct dxil_resource_v0 *)((char *)ctx->resources.data + ctx->resources.size);
   res = (struct dxil_resource_v0 *)((char *)res + resource_element_size)) {
   enum dxil_resource_class res_class;
   const struct dxil_value **handle_array;
   switch (res->resource_type) {
   case DXIL_RES_SRV_TYPED:
   case DXIL_RES_SRV_RAW:
   case DXIL_RES_SRV_STRUCTURED:
      res_class = DXIL_RESOURCE_CLASS_SRV;
   handle_array = ctx->srv_handles;
      case DXIL_RES_CBV:
      res_class = DXIL_RESOURCE_CLASS_CBV;
   handle_array = ctx->cbv_handles;
      case DXIL_RES_SAMPLER:
      res_class = DXIL_RESOURCE_CLASS_SAMPLER;
   handle_array = ctx->sampler_handles;
      case DXIL_RES_UAV_RAW:
      res_class = DXIL_RESOURCE_CLASS_UAV;
   handle_array = ctx->ssbo_handles;
      case DXIL_RES_UAV_TYPED:
   case DXIL_RES_UAV_STRUCTURED:
   case DXIL_RES_UAV_STRUCTURED_WITH_COUNTER:
      res_class = DXIL_RESOURCE_CLASS_UAV;
   handle_array = ctx->image_handles;
      default:
                  if (last_res_class != res_class)
         else
                  if (res->space > 1)
         assert(res->space == 0 ||
      (res->space == 1 &&
               /* CL uses dynamic handles for the "globals" UAV array, but uses static
   * handles for UBOs, textures, and samplers.
   */
   if (ctx->opts->environment == DXIL_ENVIRONMENT_CL &&
                  for (unsigned i = res->lower_bound; i <= res->upper_bound; ++i) {
      handle_array[i] = emit_createhandle_call_const_index(ctx,
                                             if (!handle_array[i])
         }
      }
      static const struct dxil_mdnode *
   emit_gs_state(struct ntd_context *ctx)
   {
      const struct dxil_mdnode *gs_state_nodes[5];
            gs_state_nodes[0] = dxil_get_metadata_int32(&ctx->mod, dxil_get_input_primitive(s->info.gs.input_primitive));
   gs_state_nodes[1] = dxil_get_metadata_int32(&ctx->mod, s->info.gs.vertices_out);
   gs_state_nodes[2] = dxil_get_metadata_int32(&ctx->mod, MAX2(s->info.gs.active_stream_mask, 1));
   gs_state_nodes[3] = dxil_get_metadata_int32(&ctx->mod, dxil_get_primitive_topology(s->info.gs.output_primitive));
            for (unsigned i = 0; i < ARRAY_SIZE(gs_state_nodes); ++i) {
      if (!gs_state_nodes[i])
                  }
      static enum dxil_tessellator_domain
   get_tessellator_domain(enum tess_primitive_mode primitive_mode)
   {
      switch (primitive_mode) {
   case TESS_PRIMITIVE_QUADS: return DXIL_TESSELLATOR_DOMAIN_QUAD;
   case TESS_PRIMITIVE_TRIANGLES: return DXIL_TESSELLATOR_DOMAIN_TRI;
   case TESS_PRIMITIVE_ISOLINES: return DXIL_TESSELLATOR_DOMAIN_ISOLINE;
   default:
            }
      static enum dxil_tessellator_partitioning
   get_tessellator_partitioning(enum gl_tess_spacing spacing)
   {
      switch (spacing) {
   default:
   case TESS_SPACING_EQUAL:
         case TESS_SPACING_FRACTIONAL_EVEN:
         case TESS_SPACING_FRACTIONAL_ODD:
            }
      static enum dxil_tessellator_output_primitive
   get_tessellator_output_primitive(const struct shader_info *info)
   {
      if (info->tess.point_mode)
         if (info->tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
         /* Note: GL tessellation domain is inverted from D3D, which means triangle
   * winding needs to be inverted.
   */
   if (info->tess.ccw)
            }
      static const struct dxil_mdnode *
   emit_hs_state(struct ntd_context *ctx)
   {
               hs_state_nodes[0] = dxil_get_metadata_func(&ctx->mod, ctx->tess_ctrl_patch_constant_func_def->func);
   hs_state_nodes[1] = dxil_get_metadata_int32(&ctx->mod, ctx->tess_input_control_point_count);
   hs_state_nodes[2] = dxil_get_metadata_int32(&ctx->mod, ctx->shader->info.tess.tcs_vertices_out);
   hs_state_nodes[3] = dxil_get_metadata_int32(&ctx->mod, get_tessellator_domain(ctx->shader->info.tess._primitive_mode));
   hs_state_nodes[4] = dxil_get_metadata_int32(&ctx->mod, get_tessellator_partitioning(ctx->shader->info.tess.spacing));
   hs_state_nodes[5] = dxil_get_metadata_int32(&ctx->mod, get_tessellator_output_primitive(&ctx->shader->info));
               }
      static const struct dxil_mdnode *
   emit_ds_state(struct ntd_context *ctx)
   {
               ds_state_nodes[0] = dxil_get_metadata_int32(&ctx->mod, get_tessellator_domain(ctx->shader->info.tess._primitive_mode));
               }
      static const struct dxil_mdnode *
   emit_threads(struct ntd_context *ctx)
   {
      const nir_shader *s = ctx->shader;
   const struct dxil_mdnode *threads_x = dxil_get_metadata_int32(&ctx->mod, MAX2(s->info.workgroup_size[0], 1));
   const struct dxil_mdnode *threads_y = dxil_get_metadata_int32(&ctx->mod, MAX2(s->info.workgroup_size[1], 1));
   const struct dxil_mdnode *threads_z = dxil_get_metadata_int32(&ctx->mod, MAX2(s->info.workgroup_size[2], 1));
   if (!threads_x || !threads_y || !threads_z)
            const struct dxil_mdnode *threads_nodes[] = { threads_x, threads_y, threads_z };
      }
      static int64_t
   get_module_flags(struct ntd_context *ctx)
   {
      /* See the DXIL documentation for the definition of these flags:
   *
   * https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/DXIL.rst#shader-flags
            uint64_t flags = 0;
   if (ctx->mod.feats.doubles)
         if (ctx->shader->info.stage == MESA_SHADER_FRAGMENT &&
      ctx->shader->info.fs.early_fragment_tests)
      if (ctx->mod.raw_and_structured_buffers)
         if (ctx->mod.feats.min_precision)
         if (ctx->mod.feats.dx11_1_double_extensions)
         if (ctx->mod.feats.array_layer_from_vs_or_ds)
         if (ctx->mod.feats.inner_coverage)
         if (ctx->mod.feats.stencil_ref)
         if (ctx->mod.feats.tiled_resources)
         if (ctx->mod.feats.typed_uav_load_additional_formats)
         if (ctx->mod.feats.use_64uavs)
         if (ctx->mod.feats.uavs_at_every_stage)
         if (ctx->mod.feats.cs_4x_raw_sb)
         if (ctx->mod.feats.rovs)
         if (ctx->mod.feats.wave_ops)
         if (ctx->mod.feats.int64_ops)
         if (ctx->mod.feats.view_id)
         if (ctx->mod.feats.barycentrics)
         if (ctx->mod.feats.native_low_precision)
         if (ctx->mod.feats.shading_rate)
         if (ctx->mod.feats.raytracing_tier_1_1)
         if (ctx->mod.feats.sampler_feedback)
         if (ctx->mod.feats.atomic_int64_typed)
         if (ctx->mod.feats.atomic_int64_tgsm)
         if (ctx->mod.feats.derivatives_in_mesh_or_amp)
         if (ctx->mod.feats.resource_descriptor_heap_indexing)
         if (ctx->mod.feats.sampler_descriptor_heap_indexing)
         if (ctx->mod.feats.atomic_int64_heap_resource)
         if (ctx->mod.feats.advanced_texture_ops)
         if (ctx->mod.feats.writable_msaa)
            if (ctx->opts->disable_math_refactoring)
            /* Work around https://github.com/microsoft/DirectXShaderCompiler/issues/4616
   * When targeting SM6.7 and with at least one UAV, if no other flags are present,
   * set the resources-may-not-alias flag, or else the DXIL validator may end up
   * with uninitialized memory which will fail validation, due to missing that flag.
   */
   if (flags == 0 && ctx->mod.minor_version >= 7 && ctx->num_uavs > 0)
               }
      static const struct dxil_mdnode *
   emit_entrypoint(struct ntd_context *ctx,
                  const struct dxil_func *func, const char *name,
      {
      char truncated_name[254] = { 0 };
            const struct dxil_mdnode *func_md = dxil_get_metadata_func(&ctx->mod, func);
   const struct dxil_mdnode *name_md = dxil_get_metadata_string(&ctx->mod, truncated_name);
   const struct dxil_mdnode *nodes[] = {
      func_md,
   name_md,
   signatures,
   resources,
      };
   return dxil_get_metadata_node(&ctx->mod, nodes,
      }
      static const struct dxil_mdnode *
   emit_resources(struct ntd_context *ctx)
   {
      bool emit_resources = false;
   const struct dxil_mdnode *resources_nodes[] = {
               #define ARRAY_AND_SIZE(arr) arr.data, util_dynarray_num_elements(&arr, const struct dxil_mdnode *)
         if (ctx->srv_metadata_nodes.size) {
      resources_nodes[0] = dxil_get_metadata_node(&ctx->mod, ARRAY_AND_SIZE(ctx->srv_metadata_nodes));
               if (ctx->uav_metadata_nodes.size) {
      resources_nodes[1] = dxil_get_metadata_node(&ctx->mod, ARRAY_AND_SIZE(ctx->uav_metadata_nodes));
               if (ctx->cbv_metadata_nodes.size) {
      resources_nodes[2] = dxil_get_metadata_node(&ctx->mod, ARRAY_AND_SIZE(ctx->cbv_metadata_nodes));
               if (ctx->sampler_metadata_nodes.size) {
      resources_nodes[3] = dxil_get_metadata_node(&ctx->mod, ARRAY_AND_SIZE(ctx->sampler_metadata_nodes));
            #undef ARRAY_AND_SIZE
         return emit_resources ?
      }
      static bool
   emit_tag(struct ntd_context *ctx, enum dxil_shader_tag tag,
         {
      const struct dxil_mdnode *tag_node = dxil_get_metadata_int32(&ctx->mod, tag);
   if (!tag_node || !value_node)
         assert(ctx->num_shader_property_nodes <= ARRAY_SIZE(ctx->shader_property_nodes) - 2);
   ctx->shader_property_nodes[ctx->num_shader_property_nodes++] = tag_node;
               }
      static bool
   emit_metadata(struct ntd_context *ctx)
   {
      /* DXIL versions are 1.x for shader model 6.x */
   assert(ctx->mod.major_version == 6);
   unsigned dxilMajor = 1;
   unsigned dxilMinor = ctx->mod.minor_version;
   unsigned valMajor = ctx->mod.major_validator;
   unsigned valMinor = ctx->mod.minor_validator;
   if (!emit_llvm_ident(&ctx->mod) ||
      !emit_named_version(&ctx->mod, "dx.version", dxilMajor, dxilMinor) ||
   !emit_named_version(&ctx->mod, "dx.valver", valMajor, valMinor) ||
   !emit_dx_shader_model(&ctx->mod))
         const struct dxil_func_def *main_func_def = ctx->main_func_def;
   if (!main_func_def)
                           const struct dxil_mdnode *main_entrypoint = dxil_get_metadata_func(&ctx->mod, main_func);
            const struct dxil_mdnode *node4 = dxil_get_metadata_int32(&ctx->mod, 0);
   const struct dxil_mdnode *nodes_4_27_27[] = {
         };
   const struct dxil_mdnode *node28 = dxil_get_metadata_node(&ctx->mod, nodes_4_27_27,
                     const struct dxil_mdnode *node3 = dxil_get_metadata_int32(&ctx->mod, 1);
   const struct dxil_mdnode *main_type_annotation_nodes[] = {
         };
   const struct dxil_mdnode *main_type_annotation = dxil_get_metadata_node(&ctx->mod, main_type_annotation_nodes,
            if (ctx->mod.shader_kind == DXIL_GEOMETRY_SHADER) {
      if (!emit_tag(ctx, DXIL_SHADER_TAG_GS_STATE, emit_gs_state(ctx)))
      } else if (ctx->mod.shader_kind == DXIL_HULL_SHADER) {
      ctx->tess_input_control_point_count = 32;
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_shader_in) {
      if (nir_is_arrayed_io(var, MESA_SHADER_TESS_CTRL)) {
      ctx->tess_input_control_point_count = glsl_array_size(var->type);
                  if (!emit_tag(ctx, DXIL_SHADER_TAG_HS_STATE, emit_hs_state(ctx)))
      } else if (ctx->mod.shader_kind == DXIL_DOMAIN_SHADER) {
      if (!emit_tag(ctx, DXIL_SHADER_TAG_DS_STATE, emit_ds_state(ctx)))
      } else if (ctx->mod.shader_kind == DXIL_COMPUTE_SHADER) {
      if (!emit_tag(ctx, DXIL_SHADER_TAG_NUM_THREADS, emit_threads(ctx)))
         if (ctx->mod.minor_version >= 6 &&
      ctx->shader->info.subgroup_size >= SUBGROUP_SIZE_REQUIRE_8 &&
   !emit_tag(ctx, DXIL_SHADER_TAG_WAVE_SIZE,
                  uint64_t flags = get_module_flags(ctx);
   if (flags != 0) {
      if (!emit_tag(ctx, DXIL_SHADER_TAG_FLAGS, dxil_get_metadata_int64(&ctx->mod, flags)))
      }
   const struct dxil_mdnode *shader_properties = NULL;
   if (ctx->num_shader_property_nodes > 0) {
      shader_properties = dxil_get_metadata_node(&ctx->mod, ctx->shader_property_nodes,
         if (!shader_properties)
               nir_function_impl *entry_func_impl = nir_shader_get_entrypoint(ctx->shader);
   const struct dxil_mdnode *dx_entry_point = emit_entrypoint(ctx, main_func,
         if (!dx_entry_point)
            if (resources_node) {
      const struct dxil_mdnode *dx_resources = resources_node;
   dxil_add_metadata_named_node(&ctx->mod, "dx.resources",
               if (ctx->mod.minor_version >= 2 &&
      dxil_nir_analyze_io_dependencies(&ctx->mod, ctx->shader)) {
   const struct dxil_type *i32_type = dxil_module_get_int_type(&ctx->mod, 32);
   if (!i32_type)
            const struct dxil_type *array_type = dxil_module_get_array_type(&ctx->mod, i32_type, ctx->mod.serialized_dependency_table_size);
   if (!array_type)
            const struct dxil_value **array_entries = malloc(sizeof(const struct value *) * ctx->mod.serialized_dependency_table_size);
   if (!array_entries)
            for (uint32_t i = 0; i < ctx->mod.serialized_dependency_table_size; ++i)
         const struct dxil_value *array_val = dxil_module_get_array_const(&ctx->mod, array_type, array_entries);
            const struct dxil_mdnode *view_id_state_val = dxil_get_metadata_value(&ctx->mod, array_type, array_val);
   if (!view_id_state_val)
                                 const struct dxil_mdnode *dx_type_annotations[] = { main_type_annotation };
   return dxil_add_metadata_named_node(&ctx->mod, "dx.typeAnnotations",
                        }
      static const struct dxil_value *
   bitcast_to_int(struct ntd_context *ctx, unsigned bit_size,
         {
      const struct dxil_type *type = dxil_module_get_int_type(&ctx->mod, bit_size);
   if (!type)
               }
      static const struct dxil_value *
   bitcast_to_float(struct ntd_context *ctx, unsigned bit_size,
         {
      const struct dxil_type *type = dxil_module_get_float_type(&ctx->mod, bit_size);
   if (!type)
               }
      static bool
   is_phi_src(nir_def *ssa)
   {
      nir_foreach_use(src, ssa)
      if (nir_src_parent_instr(src)->type == nir_instr_type_phi)
         }
      static void
   store_ssa_def(struct ntd_context *ctx, nir_def *ssa, unsigned chan,
         {
      assert(ssa->index < ctx->num_defs);
   assert(chan < ssa->num_components);
   /* Insert bitcasts for phi srcs in the parent block */
   if (is_phi_src(ssa)) {
      /* Prefer ints over floats if it could be both or if we have no type info */
   nir_alu_type expect_type =
      BITSET_TEST(ctx->int_types, ssa->index) ? nir_type_int :
   (BITSET_TEST(ctx->float_types, ssa->index) ? nir_type_float :
      assert(ssa->bit_size != 1 || expect_type == nir_type_int);
   if (ssa->bit_size != 1 && expect_type != dxil_type_to_nir_type(dxil_value_get_type(value)))
      value = dxil_emit_cast(&ctx->mod, DXIL_CAST_BITCAST,
                  if (ssa->bit_size == 64) {
      if (expect_type == nir_type_int)
         if (expect_type == nir_type_float)
         }
      }
      static void
   store_def(struct ntd_context *ctx, nir_def *def, unsigned chan,
         {
      const struct dxil_type *type = dxil_value_get_type(value);
   if (type == ctx->mod.float64_type)
         if (type == ctx->mod.float16_type ||
      type == ctx->mod.int16_type)
      if (type == ctx->mod.int64_type)
            }
      static void
   store_alu_dest(struct ntd_context *ctx, nir_alu_instr *alu, unsigned chan,
         {
         }
      static const struct dxil_value *
   get_src_ssa(struct ntd_context *ctx, const nir_def *ssa, unsigned chan)
   {
      assert(ssa->index < ctx->num_defs);
   assert(chan < ssa->num_components);
   assert(ctx->defs[ssa->index].chans[chan]);
      }
      static const struct dxil_value *
   get_src(struct ntd_context *ctx, nir_src *src, unsigned chan,
         {
                        switch (nir_alu_type_get_base_type(type)) {
   case nir_type_int:
   case nir_type_uint: {
      const struct dxil_type *expect_type =  dxil_module_get_int_type(&ctx->mod, bit_size);
   /* nohing to do */
   if (dxil_value_type_equal_to(value, expect_type)) {
      assert(bit_size != 64 || ctx->mod.feats.int64_ops);
      }
   if (bit_size == 64) {
      assert(ctx->mod.feats.doubles);
      }
   if (bit_size == 16)
         assert(dxil_value_type_bitsize_equal_to(value, bit_size));
   return bitcast_to_int(ctx,  bit_size, value);
         case nir_type_float:
      assert(nir_src_bit_size(*src) >= 16);
   if (dxil_value_type_equal_to(value, dxil_module_get_float_type(&ctx->mod, bit_size))) {
      assert(nir_src_bit_size(*src) != 64 || ctx->mod.feats.doubles);
      }
   if (bit_size == 64) {
      assert(ctx->mod.feats.int64_ops);
      }
   if (bit_size == 16)
         assert(dxil_value_type_bitsize_equal_to(value, bit_size));
         case nir_type_bool:
      if (!dxil_value_type_bitsize_equal_to(value, 1)) {
      return dxil_emit_cast(&ctx->mod, DXIL_CAST_TRUNC,
      }
         default:
            }
      static const struct dxil_value *
   get_alu_src(struct ntd_context *ctx, nir_alu_instr *alu, unsigned src)
   {
      unsigned chan = alu->src[src].swizzle[0];
   return get_src(ctx, &alu->src[src].src, chan,
      }
      static bool
   emit_binop(struct ntd_context *ctx, nir_alu_instr *alu,
               {
               enum dxil_opt_flags flags = 0;
   if (is_float_op && !alu->exact)
            const struct dxil_value *v = dxil_emit_binop(&ctx->mod, opcode, op0, op1, flags);
   if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_shift(struct ntd_context *ctx, nir_alu_instr *alu,
               {
      unsigned op0_bit_size = nir_src_bit_size(alu->src[0].src);
            uint64_t shift_mask = op0_bit_size - 1;
   if (!nir_src_is_const(alu->src[1].src)) {
      if (op0_bit_size != op1_bit_size) {
      const struct dxil_type *type =
         enum dxil_cast_opcode cast_op =
            }
   op1 = dxil_emit_binop(&ctx->mod, DXIL_BINOP_AND,
                  } else {
      uint64_t val = nir_scalar_as_uint(
                     const struct dxil_value *v =
         if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_cmp(struct ntd_context *ctx, nir_alu_instr *alu,
               {
      const struct dxil_value *v = dxil_emit_cmp(&ctx->mod, pred, op0, op1);
   if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static enum dxil_cast_opcode
   get_cast_op(nir_alu_instr *alu)
   {
      unsigned dst_bits = alu->def.bit_size;
            switch (alu->op) {
   /* bool -> int */
   case nir_op_b2i16:
   case nir_op_b2i32:
   case nir_op_b2i64:
            /* float -> float */
   case nir_op_f2f16_rtz:
   case nir_op_f2f16:
   case nir_op_f2fmp:
   case nir_op_f2f32:
   case nir_op_f2f64:
      assert(dst_bits != src_bits);
   if (dst_bits < src_bits)
         else
         /* int -> int */
   case nir_op_i2i1:
   case nir_op_i2i16:
   case nir_op_i2imp:
   case nir_op_i2i32:
   case nir_op_i2i64:
      assert(dst_bits != src_bits);
   if (dst_bits < src_bits)
         else
         /* uint -> uint */
   case nir_op_u2u1:
   case nir_op_u2u16:
   case nir_op_u2u32:
   case nir_op_u2u64:
      assert(dst_bits != src_bits);
   if (dst_bits < src_bits)
         else
         /* float -> int */
   case nir_op_f2i16:
   case nir_op_f2imp:
   case nir_op_f2i32:
   case nir_op_f2i64:
            /* float -> uint */
   case nir_op_f2u16:
   case nir_op_f2ump:
   case nir_op_f2u32:
   case nir_op_f2u64:
            /* int -> float */
   case nir_op_i2f16:
   case nir_op_i2fmp:
   case nir_op_i2f32:
   case nir_op_i2f64:
            /* uint -> float */
   case nir_op_u2f16:
   case nir_op_u2fmp:
   case nir_op_u2f32:
   case nir_op_u2f64:
            default:
            }
      static const struct dxil_type *
   get_cast_dest_type(struct ntd_context *ctx, nir_alu_instr *alu)
   {
      unsigned dst_bits = alu->def.bit_size;
   switch (nir_alu_type_get_base_type(nir_op_infos[alu->op].output_type)) {
   case nir_type_bool:
      assert(dst_bits == 1);
      case nir_type_int:
   case nir_type_uint:
            case nir_type_float:
            default:
            }
      static bool
   is_double(nir_alu_type alu_type, unsigned bit_size)
   {
      return nir_alu_type_get_base_type(alu_type) == nir_type_float &&
      }
      static bool
   emit_cast(struct ntd_context *ctx, nir_alu_instr *alu,
         {
      enum dxil_cast_opcode opcode = get_cast_op(alu);
   const struct dxil_type *type = get_cast_dest_type(ctx, alu);
   if (!type)
            const nir_op_info *info = &nir_op_infos[alu->op];
   switch (opcode) {
   case DXIL_CAST_UITOFP:
   case DXIL_CAST_SITOFP:
      if (is_double(info->output_type, alu->def.bit_size))
            case DXIL_CAST_FPTOUI:
   case DXIL_CAST_FPTOSI:
      if (is_double(info->input_types[0], nir_src_bit_size(alu->src[0].src)))
            default:
                  if (alu->def.bit_size == 16) {
      switch (alu->op) {
   case nir_op_f2fmp:
   case nir_op_i2imp:
   case nir_op_f2imp:
   case nir_op_f2ump:
   case nir_op_i2fmp:
   case nir_op_u2fmp:
         default:
                     const struct dxil_value *v = dxil_emit_cast(&ctx->mod, opcode, type,
         if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static enum overload_type
   get_overload(nir_alu_type alu_type, unsigned bit_size)
   {
      switch (nir_alu_type_get_base_type(alu_type)) {
   case nir_type_int:
   case nir_type_uint:
      switch (bit_size) {
   case 1: return DXIL_I1;
   case 16: return DXIL_I16;
   case 32: return DXIL_I32;
   case 64: return DXIL_I64;
   default:
            case nir_type_float:
      switch (bit_size) {
   case 16: return DXIL_F16;
   case 32: return DXIL_F32;
   case 64: return DXIL_F64;
   default:
            case nir_type_invalid:
         default:
            }
      static enum overload_type
   get_ambiguous_overload(struct ntd_context *ctx, nir_intrinsic_instr *intr,
         {
      if (BITSET_TEST(ctx->int_types, intr->def.index))
         if (BITSET_TEST(ctx->float_types, intr->def.index))
            }
      static enum overload_type
   get_ambiguous_overload_alu_type(struct ntd_context *ctx, nir_intrinsic_instr *intr,
         {
         }
      static bool
   emit_unary_intin(struct ntd_context *ctx, nir_alu_instr *alu,
         {
      const nir_op_info *info = &nir_op_infos[alu->op];
   unsigned src_bits = nir_src_bit_size(alu->src[0].src);
            const struct dxil_value *v = emit_unary_call(ctx, overload, intr, op);
   if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_binary_intin(struct ntd_context *ctx, nir_alu_instr *alu,
               {
      const nir_op_info *info = &nir_op_infos[alu->op];
   assert(info->output_type == info->input_types[0]);
   assert(info->output_type == info->input_types[1]);
   unsigned dst_bits = alu->def.bit_size;
   assert(nir_src_bit_size(alu->src[0].src) == dst_bits);
   assert(nir_src_bit_size(alu->src[1].src) == dst_bits);
            const struct dxil_value *v = emit_binary_call(ctx, overload, intr,
         if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_tertiary_intin(struct ntd_context *ctx, nir_alu_instr *alu,
                     enum dxil_intr intr,
   {
      const nir_op_info *info = &nir_op_infos[alu->op];
   unsigned dst_bits = alu->def.bit_size;
   assert(nir_src_bit_size(alu->src[0].src) == dst_bits);
   assert(nir_src_bit_size(alu->src[1].src) == dst_bits);
            assert(get_overload(info->output_type, dst_bits) == get_overload(info->input_types[0], dst_bits));
   assert(get_overload(info->output_type, dst_bits) == get_overload(info->input_types[1], dst_bits));
                     const struct dxil_value *v = emit_tertiary_call(ctx, overload, intr,
         if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_bitfield_insert(struct ntd_context *ctx, nir_alu_instr *alu,
                           {
      /* DXIL is width, offset, insert, base, NIR is base, insert, offset, width */
   const struct dxil_value *v = emit_quaternary_call(ctx, DXIL_I32, DXIL_INTR_BFI,
         if (!v)
            /* DXIL uses the 5 LSB from width/offset. Special-case width >= 32 == copy insert. */
   const struct dxil_value *compare_width = dxil_emit_cmp(&ctx->mod, DXIL_ICMP_SGE,
         v = dxil_emit_select(&ctx->mod, compare_width, insert, v);
   store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_dot4add_packed(struct ntd_context *ctx, nir_alu_instr *alu,
                     enum dxil_intr intr,
   {
      const struct dxil_func *f = dxil_get_function(&ctx->mod, "dx.op.dot4AddPacked", DXIL_I32);
   if (!f)
         const struct dxil_value *srcs[] = { dxil_module_get_int32_const(&ctx->mod, intr), accum, src0, src1 };
   const struct dxil_value *v = dxil_emit_call(&ctx->mod, f, srcs, ARRAY_SIZE(srcs));
   if (!v)
            store_alu_dest(ctx, alu, 0, v);
      }
      static bool emit_select(struct ntd_context *ctx, nir_alu_instr *alu,
                     {
      assert(sel);
   assert(val_true);
            const struct dxil_value *v = dxil_emit_select(&ctx->mod, sel, val_true, val_false);
   if (!v)
            store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_b2f16(struct ntd_context *ctx, nir_alu_instr *alu, const struct dxil_value *val)
   {
                        const struct dxil_value *c1 = dxil_module_get_float16_const(m, 0x3C00);
            if (!c0 || !c1)
               }
      static bool
   emit_b2f32(struct ntd_context *ctx, nir_alu_instr *alu, const struct dxil_value *val)
   {
                        const struct dxil_value *c1 = dxil_module_get_float_const(m, 1.0f);
            if (!c0 || !c1)
               }
      static bool
   emit_b2f64(struct ntd_context *ctx, nir_alu_instr *alu, const struct dxil_value *val)
   {
                        const struct dxil_value *c1 = dxil_module_get_double_const(m, 1.0);
            if (!c0 || !c1)
            ctx->mod.feats.doubles = 1;
      }
      static bool
   emit_f16tof32(struct ntd_context *ctx, nir_alu_instr *alu, const struct dxil_value *val, bool shift)
   {
      if (shift) {
      val = dxil_emit_binop(&ctx->mod, DXIL_BINOP_LSHR, val,
         if (!val)
               const struct dxil_func *func = dxil_get_function(&ctx->mod,
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_LEGACY_F16TOF32);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   val
            const struct dxil_value *v = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!v)
         store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_f32tof16(struct ntd_context *ctx, nir_alu_instr *alu, const struct dxil_value *val0, const struct dxil_value *val1)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod,
               if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_LEGACY_F32TOF16);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   val0
            const struct dxil_value *v = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!v)
            if (!nir_src_is_const(alu->src[1].src) || nir_src_as_int(alu->src[1].src) != 0) {
      args[1] = val1;
   const struct dxil_value *v_high = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!v_high)
            v_high = dxil_emit_binop(&ctx->mod, DXIL_BINOP_SHL, v_high,
         if (!v_high)
            v = dxil_emit_binop(&ctx->mod, DXIL_BINOP_OR, v, v_high, 0);
   if (!v)
               store_alu_dest(ctx, alu, 0, v);
      }
      static bool
   emit_vec(struct ntd_context *ctx, nir_alu_instr *alu, unsigned num_inputs)
   {
      for (unsigned i = 0; i < num_inputs; i++) {
      const struct dxil_value *src =
         if (!src)
               }
      }
      static bool
   emit_make_double(struct ntd_context *ctx, nir_alu_instr *alu)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.makeDouble", DXIL_F64);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_MAKE_DOUBLE);
   if (!opcode)
            const struct dxil_value *args[3] = {
      opcode,
   get_src(ctx, &alu->src[0].src, alu->src[0].swizzle[0], nir_type_uint32),
      };
   if (!args[1] || !args[2])
            const struct dxil_value *v = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!v)
         store_def(ctx, &alu->def, 0, v);
      }
      static bool
   emit_split_double(struct ntd_context *ctx, nir_alu_instr *alu)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.splitDouble", DXIL_F64);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_SPLIT_DOUBLE);
   if (!opcode)
            const struct dxil_value *args[] = {
      opcode,
      };
   if (!args[1])
            const struct dxil_value *v = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!v)
            const struct dxil_value *hi = dxil_emit_extractval(&ctx->mod, v, 0);
   const struct dxil_value *lo = dxil_emit_extractval(&ctx->mod, v, 1);
   if (!hi || !lo)
            store_def(ctx, &alu->def, 0, hi);
   store_def(ctx, &alu->def, 1, lo);
      }
      static bool
   emit_alu(struct ntd_context *ctx, nir_alu_instr *alu)
   {
      /* handle vec-instructions first; they are the only ones that produce
   * vector results.
   */
   switch (alu->op) {
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16:
         case nir_op_mov: {
         assert(alu->def.num_components == 1);
   store_ssa_def(ctx, &alu->def, 0, get_src_ssa(ctx,
               case nir_op_pack_double_2x32_dxil:
         case nir_op_unpack_double_2x32_dxil:
         case nir_op_bcsel: {
      /* Handled here to avoid type forced bitcast to int, since bcsel is used for ints and floats.
   * Ideally, the back-typing got both sources to match, but if it didn't, explicitly get src1's type */
   const struct dxil_value *src1 = get_src_ssa(ctx, alu->src[1].src.ssa, alu->src[1].swizzle[0]);
   nir_alu_type src1_type = dxil_type_to_nir_type(dxil_value_get_type(src1));
   return emit_select(ctx, alu,
                  }
   default:
      /* silence warnings */
               /* other ops should be scalar */
   const struct dxil_value *src[4];
   assert(nir_op_infos[alu->op].num_inputs <= 4);
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      src[i] = get_alu_src(ctx, alu, i);
   if (!src[i])
               switch (alu->op) {
   case nir_op_iadd:
            case nir_op_isub:
            case nir_op_imul:
            case nir_op_fdiv:
      if (alu->def.bit_size == 64)
               case nir_op_idiv:
   case nir_op_udiv:
      if (nir_src_is_const(alu->src[1].src)) {
      /* It's illegal to emit a literal divide by 0 in DXIL */
   nir_scalar divisor = nir_scalar_chase_alu_src(nir_get_scalar(&alu->def, 0), 1);
   if (nir_scalar_as_int(divisor) == 0) {
      store_alu_dest(ctx, alu, 0,
               }
         case nir_op_irem: return emit_binop(ctx, alu, DXIL_BINOP_SREM, src[0], src[1]);
   case nir_op_imod: return emit_binop(ctx, alu, DXIL_BINOP_UREM, src[0], src[1]);
   case nir_op_umod: return emit_binop(ctx, alu, DXIL_BINOP_UREM, src[0], src[1]);
   case nir_op_ishl: return emit_shift(ctx, alu, DXIL_BINOP_SHL, src[0], src[1]);
   case nir_op_ishr: return emit_shift(ctx, alu, DXIL_BINOP_ASHR, src[0], src[1]);
   case nir_op_ushr: return emit_shift(ctx, alu, DXIL_BINOP_LSHR, src[0], src[1]);
   case nir_op_iand: return emit_binop(ctx, alu, DXIL_BINOP_AND, src[0], src[1]);
   case nir_op_ior:  return emit_binop(ctx, alu, DXIL_BINOP_OR, src[0], src[1]);
   case nir_op_ixor: return emit_binop(ctx, alu, DXIL_BINOP_XOR, src[0], src[1]);
   case nir_op_inot: {
      unsigned bit_size = alu->def.bit_size;
   intmax_t val = bit_size == 1 ? 1 : -1;
   const struct dxil_value *negative_one = dxil_module_get_int_const(&ctx->mod, val, bit_size);
      }
   case nir_op_ieq:  return emit_cmp(ctx, alu, DXIL_ICMP_EQ, src[0], src[1]);
   case nir_op_ine:  return emit_cmp(ctx, alu, DXIL_ICMP_NE, src[0], src[1]);
   case nir_op_ige:  return emit_cmp(ctx, alu, DXIL_ICMP_SGE, src[0], src[1]);
   case nir_op_uge:  return emit_cmp(ctx, alu, DXIL_ICMP_UGE, src[0], src[1]);
   case nir_op_ilt:  return emit_cmp(ctx, alu, DXIL_ICMP_SLT, src[0], src[1]);
   case nir_op_ult:  return emit_cmp(ctx, alu, DXIL_ICMP_ULT, src[0], src[1]);
   case nir_op_feq:  return emit_cmp(ctx, alu, DXIL_FCMP_OEQ, src[0], src[1]);
   case nir_op_fneu: return emit_cmp(ctx, alu, DXIL_FCMP_UNE, src[0], src[1]);
   case nir_op_flt:  return emit_cmp(ctx, alu, DXIL_FCMP_OLT, src[0], src[1]);
   case nir_op_fge:  return emit_cmp(ctx, alu, DXIL_FCMP_OGE, src[0], src[1]);
   case nir_op_ftrunc: return emit_unary_intin(ctx, alu, DXIL_INTR_ROUND_Z, src[0]);
   case nir_op_fabs: return emit_unary_intin(ctx, alu, DXIL_INTR_FABS, src[0]);
   case nir_op_fcos: return emit_unary_intin(ctx, alu, DXIL_INTR_FCOS, src[0]);
   case nir_op_fsin: return emit_unary_intin(ctx, alu, DXIL_INTR_FSIN, src[0]);
   case nir_op_fceil: return emit_unary_intin(ctx, alu, DXIL_INTR_ROUND_PI, src[0]);
   case nir_op_fexp2: return emit_unary_intin(ctx, alu, DXIL_INTR_FEXP2, src[0]);
   case nir_op_flog2: return emit_unary_intin(ctx, alu, DXIL_INTR_FLOG2, src[0]);
   case nir_op_ffloor: return emit_unary_intin(ctx, alu, DXIL_INTR_ROUND_NI, src[0]);
   case nir_op_ffract: return emit_unary_intin(ctx, alu, DXIL_INTR_FRC, src[0]);
   case nir_op_fisnormal: return emit_unary_intin(ctx, alu, DXIL_INTR_ISNORMAL, src[0]);
            case nir_op_fddx:
   case nir_op_fddx_coarse: return emit_unary_intin(ctx, alu, DXIL_INTR_DDX_COARSE, src[0]);
   case nir_op_fddx_fine: return emit_unary_intin(ctx, alu, DXIL_INTR_DDX_FINE, src[0]);
   case nir_op_fddy:
   case nir_op_fddy_coarse: return emit_unary_intin(ctx, alu, DXIL_INTR_DDY_COARSE, src[0]);
            case nir_op_fround_even: return emit_unary_intin(ctx, alu, DXIL_INTR_ROUND_NE, src[0]);
   case nir_op_frcp: {
      const struct dxil_value *one;
   switch (alu->def.bit_size) {
   case 16:
      one = dxil_module_get_float16_const(&ctx->mod, 0x3C00);
      case 32:
      one = dxil_module_get_float_const(&ctx->mod, 1.0f);
      case 64:
      one = dxil_module_get_double_const(&ctx->mod, 1.0);
      default: unreachable("Invalid float size");
   }
      }
   case nir_op_fsat: return emit_unary_intin(ctx, alu, DXIL_INTR_SATURATE, src[0]);
   case nir_op_bit_count: return emit_unary_intin(ctx, alu, DXIL_INTR_COUNTBITS, src[0]);
   case nir_op_bitfield_reverse: return emit_unary_intin(ctx, alu, DXIL_INTR_BFREV, src[0]);
   case nir_op_ufind_msb_rev: return emit_unary_intin(ctx, alu, DXIL_INTR_FIRSTBIT_HI, src[0]);
   case nir_op_ifind_msb_rev: return emit_unary_intin(ctx, alu, DXIL_INTR_FIRSTBIT_SHI, src[0]);
   case nir_op_find_lsb: return emit_unary_intin(ctx, alu, DXIL_INTR_FIRSTBIT_LO, src[0]);
   case nir_op_imax: return emit_binary_intin(ctx, alu, DXIL_INTR_IMAX, src[0], src[1]);
   case nir_op_imin: return emit_binary_intin(ctx, alu, DXIL_INTR_IMIN, src[0], src[1]);
   case nir_op_umax: return emit_binary_intin(ctx, alu, DXIL_INTR_UMAX, src[0], src[1]);
   case nir_op_umin: return emit_binary_intin(ctx, alu, DXIL_INTR_UMIN, src[0], src[1]);
   case nir_op_frsq: return emit_unary_intin(ctx, alu, DXIL_INTR_RSQRT, src[0]);
   case nir_op_fsqrt: return emit_unary_intin(ctx, alu, DXIL_INTR_SQRT, src[0]);
   case nir_op_fmax: return emit_binary_intin(ctx, alu, DXIL_INTR_FMAX, src[0], src[1]);
   case nir_op_fmin: return emit_binary_intin(ctx, alu, DXIL_INTR_FMIN, src[0], src[1]);
   case nir_op_ffma:
      if (alu->def.bit_size == 64)
               case nir_op_ibfe: return emit_tertiary_intin(ctx, alu, DXIL_INTR_IBFE, src[2], src[1], src[0]);
   case nir_op_ubfe: return emit_tertiary_intin(ctx, alu, DXIL_INTR_UBFE, src[2], src[1], src[0]);
            case nir_op_unpack_half_2x16_split_x: return emit_f16tof32(ctx, alu, src[0], false);
   case nir_op_unpack_half_2x16_split_y: return emit_f16tof32(ctx, alu, src[0], true);
            case nir_op_sdot_4x8_iadd: return emit_dot4add_packed(ctx, alu, DXIL_INTR_DOT4_ADD_I8_PACKED, src[0], src[1], src[2]);
            case nir_op_i2i1:
   case nir_op_u2u1:
   case nir_op_b2i16:
   case nir_op_i2i16:
   case nir_op_i2imp:
   case nir_op_f2i16:
   case nir_op_f2imp:
   case nir_op_f2u16:
   case nir_op_f2ump:
   case nir_op_u2u16:
   case nir_op_u2f16:
   case nir_op_u2fmp:
   case nir_op_i2f16:
   case nir_op_i2fmp:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16:
   case nir_op_f2fmp:
   case nir_op_b2i32:
   case nir_op_f2f32:
   case nir_op_f2i32:
   case nir_op_f2u32:
   case nir_op_i2f32:
   case nir_op_i2i32:
   case nir_op_u2f32:
   case nir_op_u2u32:
   case nir_op_b2i64:
   case nir_op_f2f64:
   case nir_op_f2i64:
   case nir_op_f2u64:
   case nir_op_i2f64:
   case nir_op_i2i64:
   case nir_op_u2f64:
   case nir_op_u2u64:
            case nir_op_b2f16: return emit_b2f16(ctx, alu, src[0]);
   case nir_op_b2f32: return emit_b2f32(ctx, alu, src[0]);
   case nir_op_b2f64: return emit_b2f64(ctx, alu, src[0]);
   default:
      log_nir_instr_unsupported(ctx->logger, "Unimplemented ALU instruction",
               }
      static const struct dxil_value *
   load_ubo(struct ntd_context *ctx, const struct dxil_value *handle,
         {
               const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_CBUFFER_LOAD_LEGACY);
   if (!opcode)
            const struct dxil_value *args[] = {
                  const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.cbufferLoadLegacy", overload);
   if (!func)
            }
      static bool
   emit_barrier_impl(struct ntd_context *ctx, nir_variable_mode modes, mesa_scope execution_scope, mesa_scope mem_scope)
   {
      const struct dxil_value *opcode, *mode;
   const struct dxil_func *func;
            if (execution_scope == SCOPE_WORKGROUP)
                     if ((modes & (nir_var_mem_ssbo | nir_var_mem_global | nir_var_image)) &&
      (mem_scope > SCOPE_WORKGROUP || !is_compute)) {
      } else {
                  if ((modes & nir_var_mem_shared) && is_compute)
            func = dxil_get_function(&ctx->mod, "dx.op.barrier", DXIL_NONE);
   if (!func)
            opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_BARRIER);
   if (!opcode)
            mode = dxil_module_get_int32_const(&ctx->mod, flags);
   if (!mode)
                     return dxil_emit_call_void(&ctx->mod, func,
      }
      static bool
   emit_barrier(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      return emit_barrier_impl(ctx,
      nir_intrinsic_memory_modes(intr),
   nir_intrinsic_execution_scope(intr),
   }
      static bool
   emit_load_global_invocation_id(struct ntd_context *ctx,
         {
               for (int i = 0; i < nir_intrinsic_dest_components(intr); i++) {
      if (comps & (1 << i)) {
      const struct dxil_value *idx = dxil_module_get_int32_const(&ctx->mod, i);
   if (!idx)
                                       }
      }
      static bool
   emit_load_local_invocation_id(struct ntd_context *ctx,
         {
               for (int i = 0; i < nir_intrinsic_dest_components(intr); i++) {
      if (comps & (1 << i)) {
      const struct dxil_value
         if (!idx)
         const struct dxil_value
         if (!threadidingroup)
               }
      }
      static bool
   emit_load_local_invocation_index(struct ntd_context *ctx,
         {
      const struct dxil_value
         if (!flattenedthreadidingroup)
         store_def(ctx, &intr->def, 0, flattenedthreadidingroup);
         }
      static bool
   emit_load_local_workgroup_id(struct ntd_context *ctx,
         {
               for (int i = 0; i < nir_intrinsic_dest_components(intr); i++) {
      if (comps & (1 << i)) {
      const struct dxil_value *idx = dxil_module_get_int32_const(&ctx->mod, i);
   if (!idx)
         const struct dxil_value *groupid = emit_groupid_call(ctx, idx);
   if (!groupid)
               }
      }
      static const struct dxil_value *
   call_unary_external_function(struct ntd_context *ctx,
                     {
      const struct dxil_func *func =
         if (!func)
            const struct dxil_value *opcode =
         if (!opcode)
                        }
      static bool
   emit_load_unary_external_function(struct ntd_context *ctx,
                     {
      const struct dxil_value *value = call_unary_external_function(ctx, name, dxil_intr,
                     }
      static bool
   emit_load_sample_mask_in(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *value = call_unary_external_function(ctx,
            /* Mask coverage with (1 << sample index). Note, done as an AND to handle extrapolation cases. */
   if (ctx->mod.info.has_per_sample_input) {
      value = dxil_emit_binop(&ctx->mod, DXIL_BINOP_AND, value,
      dxil_emit_binop(&ctx->mod, DXIL_BINOP_SHL,
                  store_def(ctx, &intr->def, 0, value);
      }
      static bool
   emit_load_tess_coord(struct ntd_context *ctx,
         {
      const struct dxil_func *func =
         if (!func)
            const struct dxil_value *opcode =
         if (!opcode)
            unsigned num_coords = ctx->shader->info.tess._primitive_mode == TESS_PRIMITIVE_TRIANGLES ? 3 : 2;
   for (unsigned i = 0; i < num_coords; ++i) {
               const struct dxil_value *component = dxil_module_get_int32_const(&ctx->mod, component_idx);
   if (!component)
                     const struct dxil_value *value =
                     for (unsigned i = num_coords; i < intr->def.num_components; ++i) {
      const struct dxil_value *value = dxil_module_get_float_const(&ctx->mod, 0.0f);
                  }
      static const struct dxil_value *
   get_int32_undef(struct dxil_module *m)
   {
      const struct dxil_type *int32_type =
         if (!int32_type)
               }
      static const struct dxil_value *
   get_resource_handle(struct ntd_context *ctx, nir_src *src, enum dxil_resource_class class,
         {
      /* This source might be one of:
   * 1. Constant resource index - just look it up in precomputed handle arrays
   *    If it's null in that array, create a handle, and store the result
   * 2. A handle from load_vulkan_descriptor - just get the stored SSA value
   * 3. Dynamic resource index - create a handle for it here
   */
   assert(src->ssa->num_components == 1 && src->ssa->bit_size == 32);
   nir_const_value *const_block_index = nir_src_as_const_value(*src);
   const struct dxil_value **handle_entry = NULL;
   if (const_block_index) {
      assert(ctx->opts->environment != DXIL_ENVIRONMENT_VULKAN);
   switch (kind) {
   case DXIL_RESOURCE_KIND_CBUFFER:
      handle_entry = &ctx->cbv_handles[const_block_index->u32];
      case DXIL_RESOURCE_KIND_RAW_BUFFER:
      if (class == DXIL_RESOURCE_CLASS_UAV)
         else
            case DXIL_RESOURCE_KIND_SAMPLER:
      handle_entry = &ctx->sampler_handles[const_block_index->u32];
      default:
      if (class == DXIL_RESOURCE_CLASS_UAV)
         else
                        if (handle_entry && *handle_entry)
            if (nir_src_as_deref(*src) ||
      ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN) {
               unsigned space = 0;
   if (ctx->opts->environment == DXIL_ENVIRONMENT_GL &&
      class == DXIL_RESOURCE_CLASS_UAV) {
   if (kind == DXIL_RESOURCE_KIND_RAW_BUFFER)
         else
               /* The base binding here will almost always be zero. The only cases where we end
   * up in this type of dynamic indexing are:
   * 1. GL UBOs
   * 2. GL SSBOs
   * 2. CL SSBOs
   * In all cases except GL UBOs, the resources are a single zero-based array.
   * In that case, the base is 1, because uniforms use 0 and cannot by dynamically
   * indexed. All other cases should either fall into static indexing (first early return),
   * deref-based dynamic handle creation (images, or Vulkan textures/samplers), or
   * load_vulkan_descriptor handle creation.
   */
   unsigned base_binding = 0;
   if (ctx->opts->environment == DXIL_ENVIRONMENT_GL &&
      class == DXIL_RESOURCE_CLASS_CBV)
         const struct dxil_value *value = get_src(ctx, src, 0, nir_type_uint);
   const struct dxil_value *handle = emit_createhandle_call_dynamic(ctx, class,
         if (handle_entry)
               }
      static const struct dxil_value *
   create_image_handle(struct ntd_context *ctx, nir_intrinsic_instr *image_intr)
   {
      const struct dxil_value *unannotated_handle =
         const struct dxil_value *res_props =
            if (!unannotated_handle || !res_props)
               }
      static const struct dxil_value *
   create_srv_handle(struct ntd_context *ctx, nir_tex_instr *tex, nir_src *src)
   {
      const struct dxil_value *unannotated_handle =
         const struct dxil_value *res_props =
            if (!unannotated_handle || !res_props)
               }
      static const struct dxil_value *
   create_sampler_handle(struct ntd_context *ctx, bool is_shadow, nir_src *src)
   {
      const struct dxil_value *unannotated_handle =
         const struct dxil_value *res_props =
            if (!unannotated_handle || !res_props)
               }
      static bool
   emit_load_ssbo(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
               enum dxil_resource_class class = DXIL_RESOURCE_CLASS_UAV;
   if (ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN) {
      nir_variable *var = nir_get_binding_variable(ctx->shader, nir_chase_binding(intr->src[0]));
   if (var && var->data.access & ACCESS_NON_WRITEABLE)
               const struct dxil_value *handle = get_resource_handle(ctx, &intr->src[0], class, DXIL_RESOURCE_KIND_RAW_BUFFER);
   const struct dxil_value *offset =
         if (!int32_undef || !handle || !offset)
            assert(nir_src_bit_size(intr->src[0]) == 32);
            const struct dxil_value *coord[2] = {
      offset,
               enum overload_type overload = get_ambiguous_overload_alu_type(ctx, intr, nir_type_uint);
   const struct dxil_value *load = ctx->mod.minor_version >= 2 ?
      emit_raw_bufferload_call(ctx, handle, coord,
                        if (!load)
            for (int i = 0; i < nir_intrinsic_dest_components(intr); i++) {
      const struct dxil_value *val =
         if (!val)
            }
   if (intr->def.bit_size == 16)
            }
      static bool
   emit_store_ssbo(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value* handle = get_resource_handle(ctx, &intr->src[1], DXIL_RESOURCE_CLASS_UAV, DXIL_RESOURCE_KIND_RAW_BUFFER);
   const struct dxil_value *offset =
         if (!handle || !offset)
            unsigned num_components = nir_src_num_components(intr->src[0]);
   assert(num_components <= 4);
   if (nir_src_bit_size(intr->src[0]) == 16)
            nir_alu_type type =
         const struct dxil_value *value[4] = { 0 };
   for (unsigned i = 0; i < num_components; ++i) {
      value[i] = get_src(ctx, &intr->src[0], i, type);
   if (!value[i])
               const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[2] = {
      offset,
               enum overload_type overload = get_overload(type, intr->src[0].ssa->bit_size);
   if (num_components < 4) {
      const struct dxil_value *value_undef = dxil_module_get_undef(&ctx->mod, dxil_value_get_type(value[0]));
   if (!value_undef)
            for (int i = num_components; i < 4; ++i)
               const struct dxil_value *write_mask =
         if (!write_mask)
            return ctx->mod.minor_version >= 2 ?
      emit_raw_bufferstore_call(ctx, handle, coord, value, write_mask, overload, intr->src[0].ssa->bit_size / 8) :
   }
      static bool
   emit_load_ubo_vec4(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *handle = get_resource_handle(ctx, &intr->src[0], DXIL_RESOURCE_CLASS_CBV, DXIL_RESOURCE_KIND_CBUFFER);
   const struct dxil_value *offset =
            if (!handle || !offset)
            enum overload_type overload = get_ambiguous_overload_alu_type(ctx, intr, nir_type_uint);
   const struct dxil_value *agg = load_ubo(ctx, handle, offset, overload);
   if (!agg)
            unsigned first_component = nir_intrinsic_has_component(intr) ?
         for (unsigned i = 0; i < intr->def.num_components; i++)
      store_def(ctx, &intr->def, i,
         if (intr->def.bit_size == 16)
            }
      /* Need to add patch-ness as a matching parameter, since driver_location is *not* unique
   * between control points and patch variables in HS/DS
   */
   static nir_variable *
   find_patch_matching_variable_by_driver_location(nir_shader *s, nir_variable_mode mode, unsigned driver_location, bool patch)
   {
      nir_foreach_variable_with_modes(var, s, mode) {
      if (var->data.driver_location == driver_location &&
      var->data.patch == patch)
   }
      }
      static bool
   emit_store_output_via_intrinsic(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      assert(intr->intrinsic == nir_intrinsic_store_output ||
         bool is_patch_constant = intr->intrinsic == nir_intrinsic_store_output &&
         nir_alu_type out_type = nir_intrinsic_src_type(intr);
   enum overload_type overload = get_overload(out_type, intr->src[0].ssa->bit_size);
   const struct dxil_func *func = dxil_get_function(&ctx->mod, is_patch_constant ?
      "dx.op.storePatchConstant" : "dx.op.storeOutput",
         if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, is_patch_constant ?
         const struct dxil_value *output_id = dxil_module_get_int32_const(&ctx->mod, nir_intrinsic_base(intr));
            /* NIR has these as 1 row, N cols, but DXIL wants them as N rows, 1 col. We muck with these in the signature
   * generation, so muck with them here too.
   */
   nir_io_semantics semantics = nir_intrinsic_io_semantics(intr);
   bool is_tess_level = is_patch_constant &&
                  const struct dxil_value *row = NULL;
   const struct dxil_value *col = NULL;
   if (is_tess_level)
         else
            bool success = true;
            nir_variable *var = find_patch_matching_variable_by_driver_location(ctx->shader, nir_var_shader_out, nir_intrinsic_base(intr), is_patch_constant);
   unsigned var_base_component = var->data.location_frac;
            if (ctx->mod.minor_validator >= 5) {
      struct dxil_signature_record *sig_rec = is_patch_constant ?
      &ctx->mod.patch_consts[nir_intrinsic_base(intr)] :
      unsigned comp_size = intr->src[0].ssa->bit_size == 64 ? 2 : 1;
   unsigned comp_mask = 0;
   if (is_tess_level)
         else if (comp_size == 1)
         else {
      for (unsigned i = 0; i < intr->num_components; ++i)
      if ((writemask & (1 << i)))
   }
   for (unsigned r = 0; r < sig_rec->num_elements; ++r)
            if (!nir_src_is_const(intr->src[row_index])) {
      struct dxil_psv_signature_element *psv_rec = is_patch_constant ?
      &ctx->mod.psv_patch_consts[nir_intrinsic_base(intr)] :
                     for (unsigned i = 0; i < intr->num_components && success; ++i) {
      if (writemask & (1 << i)) {
      if (is_tess_level)
         else
         const struct dxil_value *value = get_src(ctx, &intr->src[0], i, out_type);
                  const struct dxil_value *args[] = {
         };
                     }
      static bool
   emit_load_input_via_intrinsic(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      bool attr_at_vertex = false;
   if (ctx->mod.shader_kind == DXIL_PIXEL_SHADER &&
      ctx->opts->interpolate_at_vertex &&
   ctx->opts->provoking_vertex != 0 &&
   (nir_intrinsic_dest_type(intr) & nir_type_float)) {
                        bool is_patch_constant = (ctx->mod.shader_kind == DXIL_DOMAIN_SHADER &&
                              unsigned opcode_val;
   const char *func_name;
   if (attr_at_vertex) {
      opcode_val = DXIL_INTR_ATTRIBUTE_AT_VERTEX;
   func_name = "dx.op.attributeAtVertex";
   if (ctx->mod.minor_validator >= 6)
      } else if (is_patch_constant) {
      opcode_val = DXIL_INTR_LOAD_PATCH_CONSTANT;
      } else if (is_output_control_point) {
      opcode_val = DXIL_INTR_LOAD_OUTPUT_CONTROL_POINT;
      } else {
      opcode_val = DXIL_INTR_LOAD_INPUT;
               const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, opcode_val);
   if (!opcode)
            const struct dxil_value *input_id = dxil_module_get_int32_const(&ctx->mod,
      is_patch_constant || is_output_control_point ?
      nir_intrinsic_base(intr) :
   if (!input_id)
            bool is_per_vertex =
      intr->intrinsic == nir_intrinsic_load_per_vertex_input ||
      int row_index = is_per_vertex ? 1 : 0;
   const struct dxil_value *vertex_id = NULL;
   if (!is_patch_constant) {
      if (is_per_vertex) {
         } else if (attr_at_vertex) {
         } else {
      const struct dxil_type *int32_type = dxil_module_get_int_type(&ctx->mod, 32);
                     }
   if (!vertex_id)
               /* NIR has these as 1 row, N cols, but DXIL wants them as N rows, 1 col. We muck with these in the signature
   * generation, so muck with them here too.
   */
   nir_io_semantics semantics = nir_intrinsic_io_semantics(intr);
   bool is_tess_level = is_patch_constant &&
                  const struct dxil_value *row = NULL;
   const struct dxil_value *comp = NULL;
   if (is_tess_level)
         else
            nir_alu_type out_type = nir_intrinsic_dest_type(intr);
                     if (!func)
            nir_variable *var = find_patch_matching_variable_by_driver_location(ctx->shader, nir_var_shader_in, nir_intrinsic_base(intr), is_patch_constant);
   unsigned var_base_component = var ? var->data.location_frac : 0;
            if (ctx->mod.minor_validator >= 5 &&
      !is_output_control_point &&
   intr->intrinsic != nir_intrinsic_load_output) {
   struct dxil_signature_record *sig_rec = is_patch_constant ?
      &ctx->mod.patch_consts[nir_intrinsic_base(intr)] :
      unsigned comp_size = intr->def.bit_size == 64 ? 2 : 1;
   unsigned comp_mask = (1 << (intr->num_components * comp_size)) - 1;
   comp_mask <<= (var_base_component * comp_size);
   if (is_tess_level)
         for (unsigned r = 0; r < sig_rec->num_elements; ++r)
            if (!nir_src_is_const(intr->src[row_index])) {
      struct dxil_psv_signature_element *psv_rec = is_patch_constant ?
      &ctx->mod.psv_patch_consts[nir_intrinsic_base(intr)] :
                     for (unsigned i = 0; i < intr->num_components; ++i) {
      if (is_tess_level)
         else
            if (!row || !comp)
            const struct dxil_value *args[] = {
                  unsigned num_args = ARRAY_SIZE(args) - (is_patch_constant ? 1 : 0);
   const struct dxil_value *retval = dxil_emit_call(&ctx->mod, func, args, num_args);
   if (!retval)
            }
      }
      static bool
   emit_load_interpolated_input(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
                        unsigned opcode_val;
   const char *func_name;
   unsigned num_args;
   switch (barycentric->intrinsic) {
   case nir_intrinsic_load_barycentric_at_offset:
      opcode_val = DXIL_INTR_EVAL_SNAPPED;
   func_name = "dx.op.evalSnapped";
   num_args = 6;
   for (unsigned i = 0; i < 2; ++i) {
      const struct dxil_value *float_offset = get_src(ctx, &barycentric->src[0], i, nir_type_float);
   /* GLSL uses [-0.5f, 0.5f), DXIL uses (-8, 7) */
   const struct dxil_value *offset_16 = dxil_emit_binop(&ctx->mod,
         args[i + 4] = dxil_emit_cast(&ctx->mod, DXIL_CAST_FPTOSI,
      }
      case nir_intrinsic_load_barycentric_pixel:
      opcode_val = DXIL_INTR_EVAL_SNAPPED;
   func_name = "dx.op.evalSnapped";
   num_args = 6;
   args[4] = args[5] = dxil_module_get_int32_const(&ctx->mod, 0);
      case nir_intrinsic_load_barycentric_at_sample:
      opcode_val = DXIL_INTR_EVAL_SAMPLE_INDEX;
   func_name = "dx.op.evalSampleIndex";
   num_args = 5;
   args[4] = get_src(ctx, &barycentric->src[0], 0, nir_type_int);
      case nir_intrinsic_load_barycentric_centroid:
      opcode_val = DXIL_INTR_EVAL_CENTROID;
   func_name = "dx.op.evalCentroid";
   num_args = 4;
      default:
         }
   args[0] = dxil_module_get_int32_const(&ctx->mod, opcode_val);
   args[1] = dxil_module_get_int32_const(&ctx->mod, nir_intrinsic_base(intr));
                     if (!func)
            nir_variable *var = find_patch_matching_variable_by_driver_location(ctx->shader, nir_var_shader_in, nir_intrinsic_base(intr), false);
   unsigned var_base_component = var ? var->data.location_frac : 0;
            if (ctx->mod.minor_validator >= 5) {
      struct dxil_signature_record *sig_rec =
         unsigned comp_size = intr->def.bit_size == 64 ? 2 : 1;
   unsigned comp_mask = (1 << (intr->num_components * comp_size)) - 1;
   comp_mask <<= (var_base_component * comp_size);
   for (unsigned r = 0; r < sig_rec->num_elements; ++r)
            if (!nir_src_is_const(intr->src[1])) {
      struct dxil_psv_signature_element *psv_rec =
                        for (unsigned i = 0; i < intr->num_components; ++i) {
               const struct dxil_value *retval = dxil_emit_call(&ctx->mod, func, args, num_args);
   if (!retval)
            }
      }
      static const struct dxil_value *
   deref_to_gep(struct ntd_context *ctx, nir_deref_instr *deref)
   {
      nir_deref_path path;
   nir_deref_path_init(&path, deref, ctx->ralloc_ctx);
   assert(path.path[0]->deref_type == nir_deref_type_var);
   uint32_t count = 0;
   while (path.path[count])
            const struct dxil_value **gep_indices = ralloc_array(ctx->ralloc_ctx,
               nir_variable *var = path.path[0]->var;
   const struct dxil_value **var_array;
   switch (deref->modes) {
   case nir_var_mem_constant: var_array = ctx->consts; break;
   case nir_var_mem_shared: var_array = ctx->sharedvars; break;
   case nir_var_function_temp: var_array = ctx->scratchvars; break;
   default: unreachable("Invalid deref mode");
   }
            for (uint32_t i = 0; i < count; ++i)
               }
      static bool
   emit_load_deref(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *ptr = deref_to_gep(ctx, nir_src_as_deref(intr->src[0]));
   if (!ptr)
            const struct dxil_value *retval =
         if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      static bool
   emit_store_deref(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   const struct dxil_value *ptr = deref_to_gep(ctx, deref);
   if (!ptr)
            const struct dxil_value *value = get_src(ctx, &intr->src[1], 0, nir_get_nir_type_for_glsl_type(deref->type));
      }
      static bool
   emit_atomic_deref(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *ptr = deref_to_gep(ctx, nir_src_as_deref(intr->src[0]));
   if (!ptr)
            const struct dxil_value *value = get_src(ctx, &intr->src[1], 0, nir_type_uint);
   if (!value)
            enum dxil_rmw_op dxil_op = nir_atomic_to_dxil_rmw(nir_intrinsic_atomic_op(intr));
   const struct dxil_value *retval = dxil_emit_atomicrmw(&ctx->mod, value, ptr, dxil_op, false,
               if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      static bool
   emit_atomic_deref_swap(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *ptr = deref_to_gep(ctx, nir_src_as_deref(intr->src[0]));
   if (!ptr)
            const struct dxil_value *cmp = get_src(ctx, &intr->src[1], 0, nir_type_uint);
   const struct dxil_value *value = get_src(ctx, &intr->src[2], 0, nir_type_uint);
   if (!value)
            const struct dxil_value *retval = dxil_emit_cmpxchg(&ctx->mod, cmp, value, ptr, false,
               if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      static bool
   emit_discard_if_with_value(struct ntd_context *ctx, const struct dxil_value *value)
   {
      const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_DISCARD);
   if (!opcode)
            const struct dxil_value *args[] = {
   opcode,
   value
            const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.discard", DXIL_NONE);
   if (!func)
               }
      static bool
   emit_discard_if(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *value = get_src(ctx, &intr->src[0], 0, nir_type_bool);
   if (!value)
               }
      static bool
   emit_discard(struct ntd_context *ctx)
   {
      const struct dxil_value *value = dxil_module_get_int1_const(&ctx->mod, true);
      }
      static bool
   emit_emit_vertex(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_EMIT_STREAM);
   const struct dxil_value *stream_id = dxil_module_get_int8_const(&ctx->mod, nir_intrinsic_stream_id(intr));
   if (!opcode || !stream_id)
            const struct dxil_value *args[] = {
   opcode,
   stream_id
            const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.emitStream", DXIL_NONE);
   if (!func)
               }
      static bool
   emit_end_primitive(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_CUT_STREAM);
   const struct dxil_value *stream_id = dxil_module_get_int8_const(&ctx->mod, nir_intrinsic_stream_id(intr));
   if (!opcode || !stream_id)
            const struct dxil_value *args[] = {
   opcode,
   stream_id
            const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.cutStream", DXIL_NONE);
   if (!func)
               }
      static bool
   emit_image_store(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *handle = intr->intrinsic == nir_intrinsic_bindless_image_store ?
      create_image_handle(ctx, intr) :
      if (!handle)
            bool is_array = false;
   if (intr->intrinsic == nir_intrinsic_image_deref_store)
         else
            const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[3] = { int32_undef, int32_undef, int32_undef };
   enum glsl_sampler_dim image_dim = intr->intrinsic == nir_intrinsic_image_deref_store ?
      glsl_get_sampler_dim(nir_src_as_deref(intr->src[0])->type) :
      unsigned num_coords = glsl_get_sampler_dim_coordinate_components(image_dim);
   if (is_array)
            assert(num_coords <= nir_src_num_components(intr->src[1]));
   for (unsigned i = 0; i < num_coords; ++i) {
      coord[i] = get_src(ctx, &intr->src[1], i, nir_type_uint);
   if (!coord[i])
               nir_alu_type in_type = nir_intrinsic_src_type(intr);
            assert(nir_src_bit_size(intr->src[3]) == 32);
   unsigned num_components = nir_src_num_components(intr->src[3]);
   assert(num_components <= 4);
   const struct dxil_value *value[4];
   for (unsigned i = 0; i < num_components; ++i) {
      value[i] = get_src(ctx, &intr->src[3], i, in_type);
   if (!value[i])
               for (int i = num_components; i < 4; ++i)
            const struct dxil_value *write_mask =
         if (!write_mask)
            if (image_dim == GLSL_SAMPLER_DIM_BUF) {
      coord[1] = int32_undef;
      } else
      }
      static bool
   emit_image_load(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *handle = intr->intrinsic == nir_intrinsic_bindless_image_load ?
      create_image_handle(ctx, intr) :
      if (!handle)
            bool is_array = false;
   if (intr->intrinsic == nir_intrinsic_image_deref_load)
         else
            const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[3] = { int32_undef, int32_undef, int32_undef };
   enum glsl_sampler_dim image_dim = intr->intrinsic == nir_intrinsic_image_deref_load ?
      glsl_get_sampler_dim(nir_src_as_deref(intr->src[0])->type) :
      unsigned num_coords = glsl_get_sampler_dim_coordinate_components(image_dim);
   if (is_array)
            assert(num_coords <= nir_src_num_components(intr->src[1]));
   for (unsigned i = 0; i < num_coords; ++i) {
      coord[i] = get_src(ctx, &intr->src[1], i, nir_type_uint);
   if (!coord[i])
               nir_alu_type out_type = nir_intrinsic_dest_type(intr);
            const struct dxil_value *load_result;
   if (image_dim == GLSL_SAMPLER_DIM_BUF) {
      coord[1] = int32_undef;
      } else
            if (!load_result)
            assert(intr->def.bit_size == 32);
   unsigned num_components = intr->def.num_components;
   assert(num_components <= 4);
   for (unsigned i = 0; i < num_components; ++i) {
      const struct dxil_value *component = dxil_emit_extractval(&ctx->mod, load_result, i);
   if (!component)
                     if (util_format_get_nr_components(nir_intrinsic_format(intr)) > 1)
               }
      static bool
   emit_image_atomic(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *handle = intr->intrinsic == nir_intrinsic_bindless_image_atomic ?
      create_image_handle(ctx, intr) :
      if (!handle)
            bool is_array = false;
   if (intr->intrinsic == nir_intrinsic_image_deref_atomic)
         else
            const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[3] = { int32_undef, int32_undef, int32_undef };
   enum glsl_sampler_dim image_dim = intr->intrinsic == nir_intrinsic_image_deref_atomic ?
      glsl_get_sampler_dim(nir_src_as_deref(intr->src[0])->type) :
      unsigned num_coords = glsl_get_sampler_dim_coordinate_components(image_dim);
   if (is_array)
            assert(num_coords <= nir_src_num_components(intr->src[1]));
   for (unsigned i = 0; i < num_coords; ++i) {
      coord[i] = get_src(ctx, &intr->src[1], i, nir_type_uint);
   if (!coord[i])
               nir_atomic_op nir_op = nir_intrinsic_atomic_op(intr);
   enum dxil_atomic_op dxil_op = nir_atomic_to_dxil_atomic(nir_op);
   nir_alu_type type = nir_atomic_op_type(nir_op);
   const struct dxil_value *value = get_src(ctx, &intr->src[3], 0, type);
   if (!value)
            const struct dxil_value *retval =
            if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      static bool
   emit_image_atomic_comp_swap(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *handle = intr->intrinsic == nir_intrinsic_bindless_image_atomic_swap ?
      create_image_handle(ctx, intr) :
      if (!handle)
            bool is_array = false;
   if (intr->intrinsic == nir_intrinsic_image_deref_atomic_swap)
         else
            const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[3] = { int32_undef, int32_undef, int32_undef };
   enum glsl_sampler_dim image_dim = intr->intrinsic == nir_intrinsic_image_deref_atomic_swap ?
      glsl_get_sampler_dim(nir_src_as_deref(intr->src[0])->type) :
      unsigned num_coords = glsl_get_sampler_dim_coordinate_components(image_dim);
   if (is_array)
            assert(num_coords <= nir_src_num_components(intr->src[1]));
   for (unsigned i = 0; i < num_coords; ++i) {
      coord[i] = get_src(ctx, &intr->src[1], i, nir_type_uint);
   if (!coord[i])
               const struct dxil_value *cmpval = get_src(ctx, &intr->src[3], 0, nir_type_uint);
   const struct dxil_value *newval = get_src(ctx, &intr->src[4], 0, nir_type_uint);
   if (!cmpval || !newval)
            const struct dxil_value *retval =
            if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      struct texop_parameters {
      const struct dxil_value *tex;
   const struct dxil_value *sampler;
   const struct dxil_value *bias, *lod_or_sample, *min_lod;
   const struct dxil_value *coord[4], *offset[3], *dx[3], *dy[3];
   const struct dxil_value *cmp;
      };
      static const struct dxil_value *
   emit_texture_size(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.getDimensions", DXIL_NONE);
   if (!func)
            const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_TEXTURE_SIZE),
   params->tex,
                  }
      static bool
   emit_image_size(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value *handle = intr->intrinsic == nir_intrinsic_bindless_image_size ?
      create_image_handle(ctx, intr) :
      if (!handle)
            enum glsl_sampler_dim sampler_dim = intr->intrinsic == nir_intrinsic_image_deref_size ?
      glsl_get_sampler_dim(nir_src_as_deref(intr->src[0])->type) :
      const struct dxil_value *lod = sampler_dim == GLSL_SAMPLER_DIM_BUF ?
      dxil_module_get_undef(&ctx->mod, dxil_module_get_int_type(&ctx->mod, 32)) :
      if (!lod)
            struct texop_parameters params = {
      .tex = handle,
      };
   const struct dxil_value *dimensions = emit_texture_size(ctx, &params);
   if (!dimensions)
            for (unsigned i = 0; i < intr->def.num_components; ++i) {
      const struct dxil_value *retval = dxil_emit_extractval(&ctx->mod, dimensions, i);
                  }
      static bool
   emit_get_ssbo_size(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      enum dxil_resource_class class = DXIL_RESOURCE_CLASS_UAV;
   if (ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN) {
      nir_variable *var = nir_get_binding_variable(ctx->shader, nir_chase_binding(intr->src[0]));
   if (var && var->data.access & ACCESS_NON_WRITEABLE)
               const struct dxil_value *handle = get_resource_handle(ctx, &intr->src[0], class, DXIL_RESOURCE_KIND_RAW_BUFFER);
   if (!handle)
            struct texop_parameters params = {
      .tex = handle,
   .lod_or_sample = dxil_module_get_undef(
               const struct dxil_value *dimensions = emit_texture_size(ctx, &params);
   if (!dimensions)
            const struct dxil_value *retval = dxil_emit_extractval(&ctx->mod, dimensions, 0);
               }
      static bool
   emit_ssbo_atomic(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      nir_atomic_op nir_op = nir_intrinsic_atomic_op(intr);
   enum dxil_atomic_op dxil_op = nir_atomic_to_dxil_atomic(nir_op);
   nir_alu_type type = nir_atomic_op_type(nir_op);
   const struct dxil_value* handle = get_resource_handle(ctx, &intr->src[0], DXIL_RESOURCE_CLASS_UAV, DXIL_RESOURCE_KIND_RAW_BUFFER);
   const struct dxil_value *offset =
         const struct dxil_value *value =
            if (!value || !handle || !offset)
            const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[3] = {
                  const struct dxil_value *retval =
            if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      static bool
   emit_ssbo_atomic_comp_swap(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_value* handle = get_resource_handle(ctx, &intr->src[0], DXIL_RESOURCE_CLASS_UAV, DXIL_RESOURCE_KIND_RAW_BUFFER);
   const struct dxil_value *offset =
         const struct dxil_value *cmpval =
         const struct dxil_value *newval =
            if (!cmpval || !newval || !handle || !offset)
            const struct dxil_value *int32_undef = get_int32_undef(&ctx->mod);
   if (!int32_undef)
            const struct dxil_value *coord[3] = {
                  const struct dxil_value *retval =
            if (!retval)
            store_def(ctx, &intr->def, 0, retval);
      }
      static bool
   emit_vulkan_resource_index(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
               bool const_index = nir_src_is_const(intr->src[0]);
   if (const_index) {
                  const struct dxil_value *index_value = dxil_module_get_int32_const(&ctx->mod, binding);
   if (!index_value)
            if (!const_index) {
      const struct dxil_value *offset = get_src(ctx, &intr->src[0], 0, nir_type_uint32);
   if (!offset)
            index_value = dxil_emit_binop(&ctx->mod, DXIL_BINOP_ADD, index_value, offset, 0);
   if (!index_value)
               store_def(ctx, &intr->def, 0, index_value);
   store_def(ctx, &intr->def, 1, dxil_module_get_int32_const(&ctx->mod, 0));
      }
      static bool
   emit_load_vulkan_descriptor(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      nir_intrinsic_instr* index = nir_src_as_intrinsic(intr->src[0]);
            enum dxil_resource_class resource_class;
   enum dxil_resource_kind resource_kind;
   switch (nir_intrinsic_desc_type(intr)) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      resource_class = DXIL_RESOURCE_CLASS_CBV;
   resource_kind = DXIL_RESOURCE_KIND_CBUFFER;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      resource_class = DXIL_RESOURCE_CLASS_UAV;
   resource_kind = DXIL_RESOURCE_KIND_RAW_BUFFER;
      default:
      unreachable("unknown descriptor type");
               if (index && index->intrinsic == nir_intrinsic_vulkan_resource_index) {
      unsigned binding = nir_intrinsic_binding(index);
            /* The descriptor_set field for variables is only 5 bits. We shouldn't have intrinsics trying to go beyond that. */
            nir_variable *var = nir_get_binding_variable(ctx->shader, nir_chase_binding(intr->src[0]));
   if (resource_class == DXIL_RESOURCE_CLASS_UAV &&
                  const struct dxil_value *index_value = get_src(ctx, &intr->src[0], 0, nir_type_uint32);
   if (!index_value)
               } else {
      const struct dxil_value *heap_index_value = get_src(ctx, &intr->src[0], 0, nir_type_uint32);
   if (!heap_index_value)
         const struct dxil_value *unannotated_handle = emit_createhandle_heap(ctx, heap_index_value, false, true);
   const struct dxil_value *res_props = dxil_module_get_buffer_res_props_const(&ctx->mod, resource_class, resource_kind);
   if (!unannotated_handle || !res_props)
                     store_ssa_def(ctx, &intr->def, 0, handle);
               }
      static bool
   emit_load_sample_pos_from_id(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.renderTargetGetSamplePosition", DXIL_NONE);
   if (!func)
            const struct dxil_value *opcode = dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_RENDER_TARGET_GET_SAMPLE_POSITION);
   if (!opcode)
            const struct dxil_value *args[] = {
      opcode,
      };
   if (!args[1])
            const struct dxil_value *v = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!v)
            for (unsigned i = 0; i < 2; ++i) {
      /* GL coords go from 0 -> 1, D3D from -0.5 -> 0.5 */
   const struct dxil_value *coord = dxil_emit_binop(&ctx->mod, DXIL_BINOP_ADD,
      dxil_emit_extractval(&ctx->mod, v, i),
         }
      }
      static bool
   emit_load_sample_id(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      assert(ctx->mod.info.has_per_sample_input ||
            if (ctx->mod.info.has_per_sample_input)
      return emit_load_unary_external_function(ctx, intr, "dx.op.sampleIndex",
         store_def(ctx, &intr->def, 0, dxil_module_get_int32_const(&ctx->mod, 0));
      }
      static bool
   emit_read_first_invocation(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      ctx->mod.feats.wave_ops = 1;
   const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.waveReadLaneFirst",
         const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_WAVE_READ_LANE_FIRST),
      };
   if (!func || !args[0] || !args[1])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static bool
   emit_read_invocation(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      ctx->mod.feats.wave_ops = 1;
   bool quad = intr->intrinsic == nir_intrinsic_quad_broadcast;
   const struct dxil_func *func = dxil_get_function(&ctx->mod, quad ? "dx.op.quadReadLaneAt" : "dx.op.waveReadLaneAt",
         const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, quad ? DXIL_INTR_QUAD_READ_LANE_AT : DXIL_INTR_WAVE_READ_LANE_AT),
   get_src(ctx, &intr->src[0], 0, nir_type_uint),
      };
   if (!func || !args[0] || !args[1] || !args[2])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static bool
   emit_vote_eq(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      ctx->mod.feats.wave_ops = 1;
   nir_alu_type alu_type = intr->intrinsic == nir_intrinsic_vote_ieq ? nir_type_int : nir_type_float;
   const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.waveActiveAllEqual",
         const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_WAVE_ACTIVE_ALL_EQUAL),
      };
   if (!func || !args[0] || !args[1])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static bool
   emit_vote(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      ctx->mod.feats.wave_ops = 1;
   bool any = intr->intrinsic == nir_intrinsic_vote_any;
   const struct dxil_func *func = dxil_get_function(&ctx->mod,
               const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, any ? DXIL_INTR_WAVE_ANY_TRUE : DXIL_INTR_WAVE_ALL_TRUE),
      };
   if (!func || !args[0] || !args[1])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static bool
   emit_ballot(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      ctx->mod.feats.wave_ops = 1;
   const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.waveActiveBallot", DXIL_NONE);
   const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_WAVE_ACTIVE_BALLOT),
      };
   if (!func || !args[0] || !args[1])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         for (uint32_t i = 0; i < 4; ++i)
            }
      static bool
   emit_quad_op(struct ntd_context *ctx, nir_intrinsic_instr *intr, enum dxil_quad_op_kind op)
   {
      ctx->mod.feats.wave_ops = 1;
   const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.quadOp",
         const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_QUAD_OP),
   get_src(ctx, intr->src, 0, nir_type_uint),
      };
   if (!func || !args[0] || !args[1] || !args[2])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static enum dxil_wave_bit_op_kind
   get_reduce_bit_op(nir_op op)
   {
      switch (op) {
   case nir_op_ior: return DXIL_WAVE_BIT_OP_OR;
   case nir_op_ixor: return DXIL_WAVE_BIT_OP_XOR;
   case nir_op_iand: return DXIL_WAVE_BIT_OP_AND;
   default:
            }
      static bool
   emit_reduce_bitwise(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      enum dxil_wave_bit_op_kind wave_bit_op = get_reduce_bit_op(nir_intrinsic_reduction_op(intr));
   const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.waveActiveBit",
         const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_WAVE_ACTIVE_BIT),
   get_src(ctx, intr->src, 0, nir_type_uint),
      };
   if (!func || !args[0] || !args[1] || !args[2])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static enum dxil_wave_op_kind
   get_reduce_op(nir_op op)
   {
      switch (op) {
   case nir_op_iadd:
   case nir_op_fadd:
         case nir_op_imul:
   case nir_op_fmul:
         case nir_op_imax:
   case nir_op_umax:
   case nir_op_fmax:
         case nir_op_imin:
   case nir_op_umin:
   case nir_op_fmin:
         default:
            }
      static bool
   emit_reduce(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      ctx->mod.feats.wave_ops = 1;
   bool is_prefix = intr->intrinsic == nir_intrinsic_exclusive_scan;
   nir_op reduction_op = (nir_op)nir_intrinsic_reduction_op(intr);
   switch (reduction_op) {
   case nir_op_ior:
   case nir_op_ixor:
   case nir_op_iand:
      assert(!is_prefix);
      default:
         }
   nir_alu_type alu_type = nir_op_infos[reduction_op].input_types[0];
   enum dxil_wave_op_kind wave_op = get_reduce_op(reduction_op);
   const struct dxil_func *func = dxil_get_function(&ctx->mod, is_prefix ? "dx.op.wavePrefixOp" : "dx.op.waveActiveOp",
         bool is_unsigned = alu_type == nir_type_uint;
   const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, is_prefix ? DXIL_INTR_WAVE_PREFIX_OP : DXIL_INTR_WAVE_ACTIVE_OP),
   get_src(ctx, intr->src, 0, alu_type),
   dxil_module_get_int8_const(&ctx->mod, wave_op),
      };
   if (!func || !args[0] || !args[1] || !args[2] || !args[3])
            const struct dxil_value *ret = dxil_emit_call(&ctx->mod, func, args, ARRAY_SIZE(args));
   if (!ret)
         store_def(ctx, &intr->def, 0, ret);
      }
      static bool
   emit_intrinsic(struct ntd_context *ctx, nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_global_invocation_id_zero_base:
         case nir_intrinsic_load_local_invocation_id:
         case nir_intrinsic_load_local_invocation_index:
         case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_workgroup_id_zero_base:
         case nir_intrinsic_load_ssbo:
         case nir_intrinsic_store_ssbo:
         case nir_intrinsic_load_deref:
         case nir_intrinsic_store_deref:
         case nir_intrinsic_deref_atomic:
         case nir_intrinsic_deref_atomic_swap:
         case nir_intrinsic_load_ubo_vec4:
         case nir_intrinsic_load_primitive_id:
      return emit_load_unary_external_function(ctx, intr, "dx.op.primitiveID",
      case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_id_no_per_sample:
         case nir_intrinsic_load_invocation_id:
      switch (ctx->mod.shader_kind) {
   case DXIL_HULL_SHADER:
      return emit_load_unary_external_function(ctx, intr, "dx.op.outputControlPointID",
      case DXIL_GEOMETRY_SHADER:
      return emit_load_unary_external_function(ctx, intr, "dx.op.gsInstanceID",
      default:
            case nir_intrinsic_load_view_index:
      ctx->mod.feats.view_id = true;
   return emit_load_unary_external_function(ctx, intr, "dx.op.viewID",
      case nir_intrinsic_load_sample_mask_in:
         case nir_intrinsic_load_tess_coord:
         case nir_intrinsic_discard_if:
   case nir_intrinsic_demote_if:
         case nir_intrinsic_discard:
   case nir_intrinsic_demote:
         case nir_intrinsic_emit_vertex:
         case nir_intrinsic_end_primitive:
         case nir_intrinsic_barrier:
         case nir_intrinsic_ssbo_atomic:
         case nir_intrinsic_ssbo_atomic_swap:
         case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_bindless_image_atomic:
         case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_atomic_swap:
         case nir_intrinsic_image_store:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_bindless_image_store:
         case nir_intrinsic_image_load:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_bindless_image_load:
         case nir_intrinsic_image_size:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_bindless_image_size:
         case nir_intrinsic_get_ssbo_size:
         case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
         case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
            case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_pixel:
      /* Emit nothing, we only support these as inputs to load_interpolated_input */
      case nir_intrinsic_load_interpolated_input:
      return emit_load_interpolated_input(ctx, intr);
         case nir_intrinsic_vulkan_resource_index:
         case nir_intrinsic_load_vulkan_descriptor:
            case nir_intrinsic_load_sample_pos_from_id:
            case nir_intrinsic_is_helper_invocation:
      return emit_load_unary_external_function(
      case nir_intrinsic_elect:
      ctx->mod.feats.wave_ops = 1;
   return emit_load_unary_external_function(
      case nir_intrinsic_load_subgroup_size:
      ctx->mod.feats.wave_ops = 1;
   return emit_load_unary_external_function(
      case nir_intrinsic_load_subgroup_invocation:
      ctx->mod.feats.wave_ops = 1;
   return emit_load_unary_external_function(
         case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq:
         case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all:
            case nir_intrinsic_ballot:
            case nir_intrinsic_read_first_invocation:
         case nir_intrinsic_read_invocation:
   case nir_intrinsic_shuffle:
   case nir_intrinsic_quad_broadcast:
            case nir_intrinsic_quad_swap_horizontal:
         case nir_intrinsic_quad_swap_vertical:
         case nir_intrinsic_quad_swap_diagonal:
            case nir_intrinsic_reduce:
   case nir_intrinsic_exclusive_scan:
            case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_workgroup_size:
   default:
      log_nir_instr_unsupported(
               }
      static const struct dxil_type *
   dxil_type_for_const(struct ntd_context *ctx, nir_def *def)
   {
      if (BITSET_TEST(ctx->int_types, def->index) ||
      !BITSET_TEST(ctx->float_types, def->index))
         }
      static bool
   emit_load_const(struct ntd_context *ctx, nir_load_const_instr *load_const)
   {
      for (uint32_t i = 0; i < load_const->def.num_components; ++i) {
      const struct dxil_type *type = dxil_type_for_const(ctx, &load_const->def);
      }
      }
      static bool
   emit_deref(struct ntd_context* ctx, nir_deref_instr* instr)
   {
      /* There's two possible reasons we might be walking through derefs:
   * 1. Computing an index to be used for a texture/sampler/image binding, which
   *    can only do array indexing and should compute the indices along the way with
   *    array-of-array sizes.
   * 2. Storing an index to be used in a GEP for access to a variable.
   */
   nir_variable *var = nir_deref_instr_get_variable(instr);
            bool is_aoa_size =
      glsl_type_is_sampler(glsl_without_array(var->type)) ||
   glsl_type_is_image(glsl_without_array(var->type)) ||
         if (!is_aoa_size) {
      /* Just store the values, we'll use these to build a GEP in the load or store */
   switch (instr->deref_type) {
   case nir_deref_type_var:
      store_def(ctx, &instr->def, 0, dxil_module_get_int_const(&ctx->mod, 0, instr->def.bit_size));
      case nir_deref_type_array:
      store_def(ctx, &instr->def, 0, get_src(ctx, &instr->arr.index, 0, nir_type_int));
      case nir_deref_type_struct:
      store_def(ctx, &instr->def, 0, dxil_module_get_int_const(&ctx->mod, instr->strct.index, 32));
      default:
                     /* In the CL environment, there's nothing to emit. Any references to
   * derefs will emit the necessary logic to handle scratch/shared GEP addressing
   */
   if (ctx->opts->environment == DXIL_ENVIRONMENT_CL)
            const struct glsl_type *type = instr->type;
   const struct dxil_value *binding;
   unsigned binding_val = ctx->opts->environment == DXIL_ENVIRONMENT_GL ?
            if (instr->deref_type == nir_deref_type_var) {
         } else {
      const struct dxil_value *base = get_src(ctx, &instr->parent, 0, nir_type_uint32);
   const struct dxil_value *offset = get_src(ctx, &instr->arr.index, 0, nir_type_uint32);
   if (!base || !offset)
            if (glsl_type_is_array(instr->type)) {
      offset = dxil_emit_binop(&ctx->mod, DXIL_BINOP_MUL, offset,
         if (!offset)
      }
               if (!binding)
            /* Haven't finished chasing the deref chain yet, just store the value */
   if (glsl_type_is_array(type)) {
      store_def(ctx, &instr->def, 0, binding);
               assert(glsl_type_is_sampler(type) || glsl_type_is_image(type) || glsl_type_is_texture(type));
   enum dxil_resource_class res_class;
   if (glsl_type_is_image(type))
         else if (glsl_type_is_sampler(type))
         else
            unsigned descriptor_set = ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN ?
         const struct dxil_value *handle = emit_createhandle_call_dynamic(ctx, res_class,
         if (!handle)
            store_ssa_def(ctx, &instr->def, 0, handle);
      }
      static bool
   emit_cond_branch(struct ntd_context *ctx, const struct dxil_value *cond,
         {
      assert(cond);
   assert(true_block >= 0);
   assert(false_block >= 0);
      }
      static bool
   emit_branch(struct ntd_context *ctx, int block)
   {
      assert(block >= 0);
      }
      static bool
   emit_jump(struct ntd_context *ctx, nir_jump_instr *instr)
   {
      switch (instr->type) {
   case nir_jump_break:
   case nir_jump_continue:
      assert(instr->instr.block->successors[0]);
   assert(!instr->instr.block->successors[1]);
         default:
            }
      struct phi_block {
      unsigned num_components;
      };
      static bool
   emit_phi(struct ntd_context *ctx, nir_phi_instr *instr)
   {
      const struct dxil_type *type = NULL;
   nir_foreach_phi_src(src, instr) {
      /* All sources have the same type, just use the first one */
   type = dxil_value_get_type(ctx->defs[src->src.ssa->index].chans[0]);
               struct phi_block *vphi = ralloc(ctx->phis, struct phi_block);
            for (unsigned i = 0; i < vphi->num_components; ++i) {
      struct dxil_instr *phi = vphi->comp[i] = dxil_emit_phi(&ctx->mod, type);
   if (!phi)
            }
   _mesa_hash_table_insert(ctx->phis, instr, vphi);
      }
      static bool
   fixup_phi(struct ntd_context *ctx, nir_phi_instr *instr,
         {
      const struct dxil_value *values[16];
   unsigned blocks[16];
   for (unsigned i = 0; i < vphi->num_components; ++i) {
      size_t num_incoming = 0;
   nir_foreach_phi_src(src, instr) {
      const struct dxil_value *val = get_src_ssa(ctx, src->src.ssa, i);
   values[num_incoming] = val;
   blocks[num_incoming] = src->pred->index;
   ++num_incoming;
   if (num_incoming == ARRAY_SIZE(values)) {
      if (!dxil_phi_add_incoming(vphi->comp[i], values, blocks,
                     }
   if (num_incoming > 0 && !dxil_phi_add_incoming(vphi->comp[i], values,
            }
      }
      static unsigned
   get_n_src(struct ntd_context *ctx, const struct dxil_value **values,
         {
      unsigned num_components = nir_src_num_components(src->src);
                     for (i = 0; i < num_components; ++i) {
      values[i] = get_src(ctx, &src->src, i, type);
   if (!values[i])
                  }
      #define PAD_SRC(ctx, array, components, undef) \
      for (unsigned i = components; i < ARRAY_SIZE(array); ++i) { \
               static const struct dxil_value *
   emit_sample(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.sample", params->overload);
   if (!func)
            const struct dxil_value *args[11] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_SAMPLE),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
                  }
      static const struct dxil_value *
   emit_sample_bias(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.sampleBias", params->overload);
   if (!func)
                     const struct dxil_value *args[12] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_SAMPLE_BIAS),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
                  }
      static const struct dxil_value *
   emit_sample_level(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.sampleLevel", params->overload);
   if (!func)
                     const struct dxil_value *args[11] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_SAMPLE_LEVEL),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
                  }
      static const struct dxil_value *
   emit_sample_cmp(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func;
            func = dxil_get_function(&ctx->mod, "dx.op.sampleCmp", DXIL_F32);
            if (!func)
            const struct dxil_value *args[12] = {
      dxil_module_get_int32_const(&ctx->mod, opcode),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
                  }
      static const struct dxil_value *
   emit_sample_cmp_level_zero(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func;
            func = dxil_get_function(&ctx->mod, "dx.op.sampleCmpLevelZero", DXIL_F32);
            if (!func)
            const struct dxil_value *args[11] = {
      dxil_module_get_int32_const(&ctx->mod, opcode),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
                  }
      static const struct dxil_value *
   emit_sample_cmp_level(struct ntd_context *ctx, struct texop_parameters *params)
   {
      ctx->mod.feats.advanced_texture_ops = true;
   const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.sampleCmpLevel", params->overload);
   if (!func)
                     const struct dxil_value *args[12] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_SAMPLE_CMP_LEVEL),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
                  }
      static const struct dxil_value *
   emit_sample_grad(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.sampleGrad", params->overload);
   if (!func)
            const struct dxil_value *args[17] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_SAMPLE_GRAD),
   params->tex, params->sampler,
   params->coord[0], params->coord[1], params->coord[2], params->coord[3],
   params->offset[0], params->offset[1], params->offset[2],
   params->dx[0], params->dx[1], params->dx[2],
   params->dy[0], params->dy[1], params->dy[2],
                  }
      static const struct dxil_value *
   emit_texel_fetch(struct ntd_context *ctx, struct texop_parameters *params)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.textureLoad", params->overload);
   if (!func)
            if (!params->lod_or_sample)
            const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_TEXTURE_LOAD),
   params->tex,
   params->lod_or_sample, params->coord[0], params->coord[1], params->coord[2],
                  }
      static const struct dxil_value *
   emit_texture_lod(struct ntd_context *ctx, struct texop_parameters *params, bool clamped)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod, "dx.op.calculateLOD", DXIL_F32);
   if (!func)
            const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, DXIL_INTR_TEXTURE_LOD),
   params->tex,
   params->sampler,
   params->coord[0],
   params->coord[1],
   params->coord[2],
                  }
      static const struct dxil_value *
   emit_texture_gather(struct ntd_context *ctx, struct texop_parameters *params, unsigned component)
   {
      const struct dxil_func *func = dxil_get_function(&ctx->mod,
         if (!func)
            const struct dxil_value *args[] = {
      dxil_module_get_int32_const(&ctx->mod, params->cmp ? 
         params->tex,
   params->sampler,
   params->coord[0],
   params->coord[1],
   params->coord[2],
   params->coord[3],
   params->offset[0],
   params->offset[1],
   dxil_module_get_int32_const(&ctx->mod, component),
                  }
      static bool
   emit_tex(struct ntd_context *ctx, nir_tex_instr *instr)
   {
      struct texop_parameters params;
   memset(&params, 0, sizeof(struct texop_parameters));
   if (ctx->opts->environment != DXIL_ENVIRONMENT_VULKAN) {
      params.tex = ctx->srv_handles[instr->texture_index];
               const struct dxil_type *int_type = dxil_module_get_int_type(&ctx->mod, 32);
   const struct dxil_type *float_type = dxil_module_get_float_type(&ctx->mod, 32);
   const struct dxil_value *int_undef = dxil_module_get_undef(&ctx->mod, int_type);
            unsigned coord_components = 0, offset_components = 0, dx_components = 0, dy_components = 0;
            bool lod_is_zero = false;
   for (unsigned i = 0; i < instr->num_srcs; i++) {
               switch (instr->src[i].src_type) {
   case nir_tex_src_coord:
      coord_components = get_n_src(ctx, params.coord, ARRAY_SIZE(params.coord),
         if (!coord_components)
               case nir_tex_src_offset:
      offset_components = get_n_src(ctx, params.offset, ARRAY_SIZE(params.offset),
                        /* Dynamic offsets were only allowed with gather, until "advanced texture ops" in SM7 */
   if (!nir_src_is_const(instr->src[i].src) && instr->op != nir_texop_tg4)
               case nir_tex_src_bias:
      assert(instr->op == nir_texop_txb);
   assert(nir_src_num_components(instr->src[i].src) == 1);
   params.bias = get_src(ctx, &instr->src[i].src, 0, nir_type_float);
   if (!params.bias)
               case nir_tex_src_lod:
      assert(nir_src_num_components(instr->src[i].src) == 1);
   if (instr->op == nir_texop_txf_ms) {
      assert(nir_src_as_int(instr->src[i].src) == 0);
               /* Buffers don't have a LOD */
   if (instr->sampler_dim != GLSL_SAMPLER_DIM_BUF)
         else
                        if (nir_src_is_const(instr->src[i].src) && nir_src_as_float(instr->src[i].src) == 0.0f)
               case nir_tex_src_min_lod:
      assert(nir_src_num_components(instr->src[i].src) == 1);
   params.min_lod = get_src(ctx, &instr->src[i].src, 0, type);
   if (!params.min_lod)
               case nir_tex_src_comparator:
      assert(nir_src_num_components(instr->src[i].src) == 1);
   params.cmp = get_src(ctx, &instr->src[i].src, 0, nir_type_float);
   if (!params.cmp)
               case nir_tex_src_ddx:
      dx_components = get_n_src(ctx, params.dx, ARRAY_SIZE(params.dx),
         if (!dx_components)
               case nir_tex_src_ddy:
      dy_components = get_n_src(ctx, params.dy, ARRAY_SIZE(params.dy),
         if (!dy_components)
               case nir_tex_src_ms_index:
      params.lod_or_sample = get_src(ctx, &instr->src[i].src, 0, nir_type_int);
   if (!params.lod_or_sample)
               case nir_tex_src_texture_deref:
      assert(ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN);
               case nir_tex_src_sampler_deref:
      assert(ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN);
               case nir_tex_src_texture_offset:
      params.tex = emit_createhandle_call_dynamic(ctx, DXIL_RESOURCE_CLASS_SRV,
      0, instr->texture_index,
   dxil_emit_binop(&ctx->mod, DXIL_BINOP_ADD,
      get_src(ctx, &instr->src[i].src, 0, nir_type_uint),
               case nir_tex_src_sampler_offset:
      if (nir_tex_instr_need_sampler(instr)) {
      params.sampler = emit_createhandle_call_dynamic(ctx, DXIL_RESOURCE_CLASS_SAMPLER,
      0, instr->sampler_index,
   dxil_emit_binop(&ctx->mod, DXIL_BINOP_ADD,
      get_src(ctx, &instr->src[i].src, 0, nir_type_uint),
                  case nir_tex_src_texture_handle:
                  case nir_tex_src_sampler_handle:
      if (nir_tex_instr_need_sampler(instr))
               case nir_tex_src_projector:
            default:
      fprintf(stderr, "texture source: %d\n", instr->src[i].src_type);
                  assert(params.tex != NULL);
   assert(instr->op == nir_texop_txf ||
         instr->op == nir_texop_txf_ms ||
            PAD_SRC(ctx, params.coord, coord_components, float_undef);
   PAD_SRC(ctx, params.offset, offset_components, int_undef);
            const struct dxil_value *sample = NULL;
   switch (instr->op) {
   case nir_texop_txb:
      sample = emit_sample_bias(ctx, &params);
         case nir_texop_tex:
      if (params.cmp != NULL) {
      sample = emit_sample_cmp(ctx, &params);
      } else if (ctx->mod.shader_kind == DXIL_PIXEL_SHADER) {
      sample = emit_sample(ctx, &params);
      }
   params.lod_or_sample = dxil_module_get_float_const(&ctx->mod, 0);
   lod_is_zero = true;
      case nir_texop_txl:
      if (lod_is_zero && params.cmp != NULL && ctx->opts->shader_model_max < SHADER_MODEL_6_7) {
      /* Prior to SM 6.7, if the level is constant 0.0, ignore the LOD argument,
   * so level-less DXIL instructions are used. This is needed to avoid emitting
   * dx.op.sampleCmpLevel, which would not be available.
   */
      } else {
      if (params.cmp != NULL)
         else
      }
         case nir_texop_txd:
      PAD_SRC(ctx, params.dx, dx_components, float_undef);
   PAD_SRC(ctx, params.dy, dy_components,float_undef);
   sample = emit_sample_grad(ctx, &params);
         case nir_texop_txf:
   case nir_texop_txf_ms:
      if (instr->sampler_dim == GLSL_SAMPLER_DIM_BUF) {
      params.coord[1] = int_undef;
      } else {
      PAD_SRC(ctx, params.coord, coord_components, int_undef);
      }
         case nir_texop_txs:
      sample = emit_texture_size(ctx, &params);
         case nir_texop_tg4:
      sample = emit_texture_gather(ctx, &params, instr->component);
         case nir_texop_lod:
      sample = emit_texture_lod(ctx, &params, true);
   store_def(ctx, &instr->def, 0, sample);
   sample = emit_texture_lod(ctx, &params, false);
   store_def(ctx, &instr->def, 1, sample);
         case nir_texop_query_levels: {
      params.lod_or_sample = dxil_module_get_int_const(&ctx->mod, 0, 32);
   sample = emit_texture_size(ctx, &params);
   const struct dxil_value *retval = dxil_emit_extractval(&ctx->mod, sample, 3);
   store_def(ctx, &instr->def, 0, retval);
               case nir_texop_texture_samples: {
      params.lod_or_sample = int_undef;
   sample = emit_texture_size(ctx, &params);
   const struct dxil_value *retval = dxil_emit_extractval(&ctx->mod, sample, 3);
   store_def(ctx, &instr->def, 0, retval);
               default:
      fprintf(stderr, "texture op: %d\n", instr->op);
               if (!sample)
            for (unsigned i = 0; i < instr->def.num_components; ++i) {
      const struct dxil_value *retval = dxil_emit_extractval(&ctx->mod, sample, i);
                  }
      static bool
   emit_undefined(struct ntd_context *ctx, nir_undef_instr *undef)
   {
      for (unsigned i = 0; i < undef->def.num_components; ++i)
            }
      static bool emit_instr(struct ntd_context *ctx, struct nir_instr* instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
         case nir_instr_type_intrinsic:
         case nir_instr_type_load_const:
         case nir_instr_type_deref:
         case nir_instr_type_jump:
         case nir_instr_type_phi:
         case nir_instr_type_tex:
         case nir_instr_type_undef:
         default:
      log_nir_instr_unsupported(ctx->logger, "Unimplemented instruction type",
               }
         static bool
   emit_block(struct ntd_context *ctx, struct nir_block *block)
   {
      assert(block->index < ctx->mod.cur_emitting_func->num_basic_block_ids);
            nir_foreach_instr(instr, block) {
               if (!emit_instr(ctx, instr))  {
            }
      }
      static bool
   emit_cf_list(struct ntd_context *ctx, struct exec_list *list);
      static bool
   emit_if(struct ntd_context *ctx, struct nir_if *if_stmt)
   {
      assert(nir_src_num_components(if_stmt->condition) == 1);
   const struct dxil_value *cond = get_src(ctx, &if_stmt->condition, 0,
         if (!cond)
            /* prepare blocks */
   nir_block *then_block = nir_if_first_then_block(if_stmt);
   assert(nir_if_last_then_block(if_stmt)->successors[0]);
   assert(!nir_if_last_then_block(if_stmt)->successors[1]);
            nir_block *else_block = NULL;
   int else_succ = -1;
   if (!exec_list_is_empty(&if_stmt->else_list)) {
      else_block = nir_if_first_else_block(if_stmt);
   assert(nir_if_last_else_block(if_stmt)->successors[0]);
   assert(!nir_if_last_else_block(if_stmt)->successors[1]);
               if (!emit_cond_branch(ctx, cond, then_block->index,
                  /* handle then-block */
   if (!emit_cf_list(ctx, &if_stmt->then_list) ||
      (!nir_block_ends_in_jump(nir_if_last_then_block(if_stmt)) &&
   !emit_branch(ctx, then_succ)))
         if (else_block) {
      /* handle else-block */
   if (!emit_cf_list(ctx, &if_stmt->else_list) ||
      (!nir_block_ends_in_jump(nir_if_last_else_block(if_stmt)) &&
   !emit_branch(ctx, else_succ)))
               }
      static bool
   emit_loop(struct ntd_context *ctx, nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
   nir_block *first_block = nir_loop_first_block(loop);
            assert(last_block->successors[0]);
            if (!emit_branch(ctx, first_block->index))
            if (!emit_cf_list(ctx, &loop->body))
            /* If the loop's last block doesn't explicitly jump somewhere, then there's
   * an implicit continue that should take it back to the first loop block
   */
   nir_instr *last_instr = nir_block_last_instr(last_block);
   if ((!last_instr || last_instr->type != nir_instr_type_jump) &&
      !emit_branch(ctx, first_block->index))
            }
      static bool
   emit_cf_list(struct ntd_context *ctx, struct exec_list *list)
   {
      foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block:
      if (!emit_block(ctx, nir_cf_node_as_block(node)))
               case nir_cf_node_if:
      if (!emit_if(ctx, nir_cf_node_as_if(node)))
               case nir_cf_node_loop:
      if (!emit_loop(ctx, nir_cf_node_as_loop(node)))
               default:
      unreachable("unsupported cf-list node");
         }
      }
      static void
   insert_sorted_by_binding(struct exec_list *var_list, nir_variable *new_var)
   {
      nir_foreach_variable_in_list(var, var_list) {
      if (var->data.binding > new_var->data.binding) {
      exec_node_insert_node_before(&var->node, &new_var->node);
         }
      }
         static void
   sort_uniforms_by_binding_and_remove_structs(nir_shader *s)
   {
      struct exec_list new_list;
            nir_foreach_variable_with_modes_safe(var, s, nir_var_uniform) {
      exec_node_remove(&var->node);
   const struct glsl_type *type = glsl_without_array(var->type);
   if (!glsl_type_is_struct(type))
      }
      }
      static bool
   emit_cbvs(struct ntd_context *ctx)
   {
      if (ctx->opts->environment != DXIL_ENVIRONMENT_GL) {
      nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_ubo) {
      if (!emit_ubo_var(ctx, var))
         } else {
      if (ctx->shader->info.num_ubos) {
      const unsigned ubo_size = 16384 /*4096 vec4's*/;
   bool has_ubo0 = !ctx->opts->no_ubo0;
   bool has_state_vars = ctx->opts->last_ubo_is_not_arrayed;
                  if (has_ubo0 &&
      !emit_cbv(ctx, 0, 0, ubo_size, 1, "__ubo_uniforms"))
      if (ubo1_array_size &&
      !emit_cbv(ctx, 1, 0, ubo_size, ubo1_array_size, "__ubos"))
      if (has_state_vars &&
      !emit_cbv(ctx, ctx->shader->info.num_ubos - 1, 0, ubo_size, 1, "__ubo_state_vars"))
                  }
      static bool
   emit_scratch(struct ntd_context *ctx, nir_function_impl *impl)
   {
      uint32_t index = 0;
   nir_foreach_function_temp_variable(var, impl)
            if (ctx->scratchvars)
                     nir_foreach_function_temp_variable(var, impl) {
      const struct dxil_type *type = get_type_for_glsl_type(&ctx->mod, var->type);
   const struct dxil_value *length = dxil_module_get_int32_const(&ctx->mod, 1);
   const struct dxil_value *ptr = dxil_emit_alloca(&ctx->mod, type, length, 16);
   if (!ptr)
                           }
      static bool
   emit_function(struct ntd_context *ctx, nir_function *func, nir_function_impl *impl)
   {
      assert(func->num_params == 0);
            const char *attr_keys[2] = { NULL };
   const char *attr_values[2] = { NULL };
   if (ctx->shader->info.float_controls_execution_mode &
      (FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32 | FLOAT_CONTROLS_DENORM_PRESERVE_FP32))
      if (ctx->shader->info.float_controls_execution_mode & FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32)
         else if (ctx->shader->info.float_controls_execution_mode & FLOAT_CONTROLS_DENORM_PRESERVE_FP32)
            const struct dxil_type *void_type = dxil_module_get_void_type(&ctx->mod);
   const struct dxil_type *func_type = dxil_module_add_function_type(&ctx->mod, void_type, NULL, 0);
   struct dxil_func_def *func_def = dxil_add_function_def(&ctx->mod, func->name, func_type, impl->num_blocks, attr_keys, attr_values);
   if (!func_def)
            if (func->is_entrypoint)
         else if (func == ctx->tess_ctrl_patch_constant_func)
            ctx->defs = rzalloc_array(ctx->ralloc_ctx, struct dxil_def, impl->ssa_alloc);
   ctx->float_types = rzalloc_array(ctx->ralloc_ctx, BITSET_WORD, BITSET_WORDS(impl->ssa_alloc));
   ctx->int_types = rzalloc_array(ctx->ralloc_ctx, BITSET_WORD, BITSET_WORDS(impl->ssa_alloc));
   if (!ctx->defs || !ctx->float_types || !ctx->int_types)
                  ctx->phis = _mesa_pointer_hash_table_create(ctx->ralloc_ctx);
   if (!ctx->phis)
                     if (!emit_scratch(ctx, impl))
            if (!emit_static_indexing_handles(ctx))
            if (!emit_cf_list(ctx, &impl->body))
            hash_table_foreach(ctx->phis, entry) {
      if (!fixup_phi(ctx, (nir_phi_instr *)entry->key,
                     if (!dxil_emit_ret_void(&ctx->mod))
            ralloc_free(ctx->defs);
   ctx->defs = NULL;
   _mesa_hash_table_destroy(ctx->phis, NULL);
      }
      static bool
   emit_module(struct ntd_context *ctx, const struct nir_to_dxil_options *opts)
   {
      /* The validator forces us to emit resources in a specific order:
   * CBVs, Samplers, SRVs, UAVs. While we are at it also remove
   * stale struct uniforms, they are lowered but might not have been removed */
            /* CBVs */
   if (!emit_cbvs(ctx))
            /* Samplers */
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_uniform) {
      unsigned count = glsl_type_get_sampler_count(var->type);
   assert(count == 0 || glsl_type_is_bare_sampler(glsl_without_array(var->type)));
   if (count > 0 && !emit_sampler(ctx, var, count))
               /* SRVs */
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_uniform) {
      unsigned count = glsl_type_get_texture_count(var->type);
   assert(count == 0 || glsl_type_is_texture(glsl_without_array(var->type)));
   if (count > 0 && !emit_srv(ctx, var, count))
               /* Handle read-only SSBOs as SRVs */
   if (ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN) {
      nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_ssbo) {
      if ((var->data.access & ACCESS_NON_WRITEABLE) != 0) {
      unsigned count = 1;
   if (glsl_type_is_array(var->type))
         if (!emit_srv(ctx, var, count))
                     if (!emit_shared_vars(ctx))
         if (!emit_global_consts(ctx))
            /* UAVs */
   if (ctx->shader->info.stage == MESA_SHADER_KERNEL) {
      if (!emit_globals(ctx, opts->num_kernel_globals))
         } else if (ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN) {
      /* Handle read/write SSBOs as UAVs */
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_mem_ssbo) {
      if ((var->data.access & ACCESS_NON_WRITEABLE) == 0) {
      unsigned count = 1;
   if (glsl_type_is_array(var->type))
         if (!emit_uav(ctx, var->data.binding, var->data.descriptor_set,
                              } else {
      for (unsigned i = 0; i < ctx->shader->info.num_ssbos; ++i) {
      char name[64];
   snprintf(name, sizeof(name), "__ssbo%d", i);
   if (!emit_uav(ctx, i, 0, 1, DXIL_COMP_TYPE_INVALID, 1,
            }
   /* To work around a WARP bug, bind these descriptors a second time in descriptor
   * space 2. Space 0 will be used for static indexing, while space 2 will be used
   * for dynamic indexing. Space 0 will be individual SSBOs in the DXIL shader, while
   * space 2 will be a single array.
   */
   if (ctx->shader->info.num_ssbos &&
      !emit_uav(ctx, 0, 2, ctx->shader->info.num_ssbos, DXIL_COMP_TYPE_INVALID, 1,
                  nir_foreach_image_variable(var, ctx->shader) {
      if (!emit_uav_var(ctx, var, glsl_type_get_image_count(var->type)))
               ctx->mod.info.has_per_sample_input =
      BITSET_TEST(ctx->shader->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID) ||
   ctx->shader->info.fs.uses_sample_shading ||
      if (!ctx->mod.info.has_per_sample_input && ctx->shader->info.stage == MESA_SHADER_FRAGMENT) {
      nir_foreach_variable_with_modes(var, ctx->shader, nir_var_shader_in | nir_var_system_value) {
      if (var->data.sample) {
      ctx->mod.info.has_per_sample_input = true;
                     /* From the Vulkan spec 1.3.238, section 15.8:
   * When Sample Shading is enabled, the x and y components of FragCoord reflect the location 
   * of one of the samples corresponding to the shader invocation.
   * 
   * In other words, if the fragment shader is executing per-sample, then the position variable
   * should always be per-sample, 
   * 
   * Also:
   * The Centroid interpolation decoration is ignored, but allowed, on FragCoord.
   */
   if (ctx->opts->environment == DXIL_ENVIRONMENT_VULKAN) {
      nir_variable *pos_var = nir_find_variable_with_location(ctx->shader, nir_var_shader_in, VARYING_SLOT_POS);
   if (pos_var) {
      if (ctx->mod.info.has_per_sample_input)
                        unsigned input_clip_size = ctx->mod.shader_kind == DXIL_PIXEL_SHADER ?
                  nir_foreach_function_with_impl(func, impl, ctx->shader) {
      if (!emit_function(ctx, func, impl))
               if (ctx->shader->info.stage == MESA_SHADER_FRAGMENT) {
      nir_foreach_variable_with_modes(var, ctx->shader, nir_var_shader_out) {
      if (var->data.location == FRAG_RESULT_STENCIL) {
               } else if (ctx->shader->info.stage == MESA_SHADER_VERTEX ||
            if (ctx->shader->info.outputs_written &
      (VARYING_BIT_VIEWPORT | VARYING_BIT_LAYER))
   } else if (ctx->shader->info.stage == MESA_SHADER_GEOMETRY ||
            if (ctx->shader->info.inputs_read &
      (VARYING_BIT_VIEWPORT | VARYING_BIT_LAYER))
            if (ctx->mod.feats.native_low_precision && ctx->mod.minor_version < 2) {
      ctx->logger->log(ctx->logger->priv,
                     return emit_metadata(ctx) &&
      }
      static unsigned int
   get_dxil_shader_kind(struct nir_shader *s)
   {
      switch (s->info.stage) {
   case MESA_SHADER_VERTEX:
         case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
         case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_KERNEL:
   case MESA_SHADER_COMPUTE:
         default:
      unreachable("unknown shader stage in nir_to_dxil");
         }
      static unsigned
   lower_bit_size_callback(const nir_instr* instr, void *data)
   {
      if (instr->type != nir_instr_type_alu)
                  if (nir_op_infos[alu->op].is_conversion)
            if (nir_op_is_vec_or_mov(alu->op))
            unsigned num_inputs = nir_op_infos[alu->op].num_inputs;
   const struct nir_to_dxil_options *opts = (const struct nir_to_dxil_options*)data;
            unsigned ret = 0;
   for (unsigned i = 0; i < num_inputs; i++) {
      unsigned bit_size = nir_src_bit_size(alu->src[i].src);
   if (bit_size != 1 && bit_size < min_bit_size)
                  }
      static bool
   vectorize_filter(
      unsigned align_mul,
   unsigned align_offset,
   unsigned bit_size,
   unsigned num_components,
   nir_intrinsic_instr *low, nir_intrinsic_instr *high,
      {
         }
      struct lower_mem_bit_sizes_data {
      const nir_shader_compiler_options *nir_options;
      };
      static nir_mem_access_size_align
   lower_mem_access_bit_sizes_cb(nir_intrinsic_op intrin,
                                 uint8_t bytes,
   {
      const struct lower_mem_bit_sizes_data *data = cb_data;
   unsigned max_bit_size = 32;
   unsigned min_bit_size = data->dxil_options->lower_int16 ? 32 : 16;
   unsigned closest_bit_size = MAX2(min_bit_size, MIN2(max_bit_size, bit_size_in));
   if (intrin == nir_intrinsic_load_ubo) {
      /* UBO loads can be done at whatever (supported) bit size, but require 16 byte
   * alignment and can load up to 16 bytes per instruction. However this pass requires
   * loading 16 bytes of data to get 16-byte alignment. We're going to run lower_ubo_vec4
   * which can deal with unaligned vec4s, so for this pass let's just deal with bit size
   * and total size restrictions. */
   return (nir_mem_access_size_align) {
      .align = closest_bit_size / 8,
   .bit_size = closest_bit_size,
                  assert(intrin == nir_intrinsic_load_ssbo || intrin == nir_intrinsic_store_ssbo);
   uint32_t align = nir_combined_align(align_mul, align_offset);
   if (align < min_bit_size / 8) {
      /* Unaligned load/store, use the minimum bit size, up to 4 components */
   unsigned ideal_num_components = intrin == nir_intrinsic_load_ssbo ?
      DIV_ROUND_UP(bytes * 8, min_bit_size) :
      return (nir_mem_access_size_align) {
      .align = min_bit_size / 8,
   .bit_size = min_bit_size,
                  /* Increase/decrease bit size to try to get closer to the requested byte size/align */
   unsigned bit_size = closest_bit_size;
   unsigned target = MIN2(bytes, align);
   while (target < bit_size / 8 && bit_size > min_bit_size)
         while (target > bit_size / 8 * 4 && bit_size < max_bit_size)
            /* This is the best we can do */
   return (nir_mem_access_size_align) {
      .align = bit_size / 8,
   .bit_size = bit_size,
         }
      static void
   optimize_nir(struct nir_shader *s, const struct nir_to_dxil_options *opts)
   {
      bool progress;
   do {
      progress = false;
   NIR_PASS_V(s, nir_lower_vars_to_ssa);
   NIR_PASS(progress, s, nir_lower_indirect_derefs, nir_var_function_temp, 4);
   NIR_PASS(progress, s, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS(progress, s, nir_copy_prop);
   NIR_PASS(progress, s, nir_opt_copy_prop_vars);
   NIR_PASS(progress, s, nir_lower_bit_size, lower_bit_size_callback, (void*)opts);
   NIR_PASS(progress, s, dxil_nir_lower_8bit_conv);
   if (opts->lower_int16)
         NIR_PASS(progress, s, nir_opt_remove_phis);
   NIR_PASS(progress, s, nir_opt_dce);
   NIR_PASS(progress, s, nir_opt_if, nir_opt_if_aggressive_last_continue | nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, s, nir_opt_dead_cf);
   NIR_PASS(progress, s, nir_opt_cse);
   NIR_PASS(progress, s, nir_opt_peephole_select, 8, true, true);
   NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, dxil_nir_algebraic);
   if (s->options->lower_int64_options)
         NIR_PASS(progress, s, nir_lower_alu);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   NIR_PASS(progress, s, nir_opt_undef);
   NIR_PASS(progress, s, nir_lower_undef_to_zero);
   NIR_PASS(progress, s, nir_opt_deref);
   NIR_PASS(progress, s, dxil_nir_lower_upcast_phis, opts->lower_int16 ? 32 : 16);
   NIR_PASS(progress, s, nir_lower_64bit_phis);
   NIR_PASS(progress, s, nir_lower_phis_to_scalar, true);
   NIR_PASS(progress, s, nir_opt_loop_unroll);
   NIR_PASS(progress, s, nir_lower_pack);
               do {
      progress = false;
         }
      static
   void dxil_fill_validation_state(struct ntd_context *ctx,
         {
      unsigned resource_element_size = ctx->mod.minor_validator >= 6 ?
         state->num_resources = ctx->resources.size / resource_element_size;
   state->resources.v0 = (struct dxil_resource_v0*)ctx->resources.data;
   state->state.psv1.psv0.max_expected_wave_lane_count = UINT_MAX;
   state->state.psv1.shader_stage = (uint8_t)ctx->mod.shader_kind;
   state->state.psv1.uses_view_id = (uint8_t)ctx->mod.feats.view_id;
   state->state.psv1.sig_input_elements = (uint8_t)ctx->mod.num_sig_inputs;
   state->state.psv1.sig_output_elements = (uint8_t)ctx->mod.num_sig_outputs;
            switch (ctx->mod.shader_kind) {
   case DXIL_VERTEX_SHADER:
      state->state.psv1.psv0.vs.output_position_present = ctx->mod.info.has_out_position;
      case DXIL_PIXEL_SHADER:
      /* TODO: handle depth outputs */
   state->state.psv1.psv0.ps.depth_output = ctx->mod.info.has_out_depth;
   state->state.psv1.psv0.ps.sample_frequency =
            case DXIL_COMPUTE_SHADER:
      state->state.num_threads_x = MAX2(ctx->shader->info.workgroup_size[0], 1);
   state->state.num_threads_y = MAX2(ctx->shader->info.workgroup_size[1], 1);
   state->state.num_threads_z = MAX2(ctx->shader->info.workgroup_size[2], 1);
      case DXIL_GEOMETRY_SHADER:
      state->state.psv1.max_vertex_count = ctx->shader->info.gs.vertices_out;
   state->state.psv1.psv0.gs.input_primitive = dxil_get_input_primitive(ctx->shader->info.gs.input_primitive);
   state->state.psv1.psv0.gs.output_toplology = dxil_get_primitive_topology(ctx->shader->info.gs.output_primitive);
   state->state.psv1.psv0.gs.output_stream_mask = MAX2(ctx->shader->info.gs.active_stream_mask, 1);
   state->state.psv1.psv0.gs.output_position_present = ctx->mod.info.has_out_position;
      case DXIL_HULL_SHADER:
      state->state.psv1.psv0.hs.input_control_point_count = ctx->tess_input_control_point_count;
   state->state.psv1.psv0.hs.output_control_point_count = ctx->shader->info.tess.tcs_vertices_out;
   state->state.psv1.psv0.hs.tessellator_domain = get_tessellator_domain(ctx->shader->info.tess._primitive_mode);
   state->state.psv1.psv0.hs.tessellator_output_primitive = get_tessellator_output_primitive(&ctx->shader->info);
   state->state.psv1.sig_patch_const_or_prim_vectors = ctx->mod.num_psv_patch_consts;
      case DXIL_DOMAIN_SHADER:
      state->state.psv1.psv0.ds.input_control_point_count = ctx->shader->info.tess.tcs_vertices_out;
   state->state.psv1.psv0.ds.tessellator_domain = get_tessellator_domain(ctx->shader->info.tess._primitive_mode);
   state->state.psv1.psv0.ds.output_position_present = ctx->mod.info.has_out_position;
   state->state.psv1.sig_patch_const_or_prim_vectors = ctx->mod.num_psv_patch_consts;
      default:
            }
      static nir_variable *
   add_sysvalue(struct ntd_context *ctx,
               {
         nir_variable *var = rzalloc(ctx->shader, nir_variable);
   if (!var)
         var->data.driver_location = driver_location;
   var->data.location = value;
   var->type = glsl_uint_type();
   var->name = name;
   var->data.mode = nir_var_system_value;
   var->data.interpolation = INTERP_MODE_FLAT;
      }
      static bool
   append_input_or_sysvalue(struct ntd_context *ctx,
               {
      if (input_loc >= 0) {
      /* Check inputs whether a variable is available the corresponds
   * to the sysvalue */
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_shader_in) {
      if (var->data.location == input_loc) {
      ctx->system_value[sv_slot] = var;
                     ctx->system_value[sv_slot] = add_sysvalue(ctx, sv_slot, name, driver_location);
   if (!ctx->system_value[sv_slot])
            nir_shader_add_variable(ctx->shader, ctx->system_value[sv_slot]);
      }
      struct sysvalue_name {
      gl_system_value value;
   int slot;
   char *name;
      } possible_sysvalues[] = {
      {SYSTEM_VALUE_VERTEX_ID_ZERO_BASE, -1, "SV_VertexID", MESA_SHADER_NONE},
   {SYSTEM_VALUE_INSTANCE_ID, -1, "SV_InstanceID", MESA_SHADER_NONE},
   {SYSTEM_VALUE_FRONT_FACE, VARYING_SLOT_FACE, "SV_IsFrontFace", MESA_SHADER_NONE},
   {SYSTEM_VALUE_PRIMITIVE_ID, VARYING_SLOT_PRIMITIVE_ID, "SV_PrimitiveID", MESA_SHADER_GEOMETRY},
      };
      static bool
   allocate_sysvalues(struct ntd_context *ctx)
   {
      unsigned driver_location = 0;
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_shader_in)
         nir_foreach_variable_with_modes(var, ctx->shader, nir_var_system_value)
            if (ctx->shader->info.stage == MESA_SHADER_FRAGMENT &&
      !BITSET_TEST(ctx->shader->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID)) {
            /* "var->data.sample = true" sometimes just mean, "I want per-sample
   * shading", which explains why we can end up with vars having flat
   * interpolation with the per-sample bit set. If there's only such
   * type of variables, we need to tell DXIL that we read SV_SampleIndex
   * to make DXIL validation happy.
   */
   nir_foreach_variable_with_modes(var, ctx->shader, nir_var_shader_in) {
      bool var_can_be_sample_rate = !var->data.centroid && var->data.interpolation != INTERP_MODE_FLAT;
   /* If there's an input that will actually force sample-rate shading, then we don't
   * need SV_SampleIndex. */
   if (var->data.sample && var_can_be_sample_rate) {
      need_sample_id = false;
      }
   /* If there's an input that wants to be sample-rate, but can't be, then we might
   * need SV_SampleIndex. */
   if (var->data.sample && !var_can_be_sample_rate)
               if (need_sample_id)
               for (unsigned i = 0; i < ARRAY_SIZE(possible_sysvalues); ++i) {
      struct sysvalue_name *info = &possible_sysvalues[i];
   if (info->only_in_shader != MESA_SHADER_NONE &&
      info->only_in_shader != ctx->shader->info.stage)
      if (BITSET_TEST(ctx->shader->info.system_values_read, info->value)) {
      if (!append_input_or_sysvalue(ctx, info->slot,
                     }
      }
      static int
   type_size_vec4(const struct glsl_type *type, bool bindless)
   {
         }
      static const unsigned dxil_validator_min_capable_version = DXIL_VALIDATOR_1_4;
   static const unsigned dxil_validator_max_capable_version = DXIL_VALIDATOR_1_7;
   static const unsigned dxil_min_shader_model = SHADER_MODEL_6_0;
   static const unsigned dxil_max_shader_model = SHADER_MODEL_6_7;
      bool
   nir_to_dxil(struct nir_shader *s, const struct nir_to_dxil_options *opts,
         {
      assert(opts);
   bool retval = true;
   debug_dxil = (int)debug_get_option_debug_dxil();
            if (opts->shader_model_max < dxil_min_shader_model) {
      debug_printf("D3D12: cannot support emitting shader models lower than %d.%d\n",
                           if (opts->shader_model_max > dxil_max_shader_model) {
      debug_printf("D3D12: cannot support emitting higher than shader model %d.%d\n",
                           if (opts->validator_version_max != NO_DXIL_VALIDATION &&
      opts->validator_version_max < dxil_validator_min_capable_version) {
   debug_printf("D3D12: Invalid validator version %d.%d, must be 1.4 or greater\n",
      opts->validator_version_max >> 16,
                  /* If no validation, write a blob as if it was going to be validated by the newest understood validator.
   * Same if the validator is newer than we know how to write for.
   */
   uint32_t validator_version =
      opts->validator_version_max == NO_DXIL_VALIDATION ||
   opts->validator_version_max > dxil_validator_max_capable_version ?
         struct ntd_context *ctx = calloc(1, sizeof(*ctx));
   if (!ctx)
            ctx->opts = opts;
   ctx->shader = s;
            ctx->ralloc_ctx = ralloc_context(NULL);
   if (!ctx->ralloc_ctx) {
      retval = false;
               util_dynarray_init(&ctx->srv_metadata_nodes, ctx->ralloc_ctx);
   util_dynarray_init(&ctx->uav_metadata_nodes, ctx->ralloc_ctx);
   util_dynarray_init(&ctx->cbv_metadata_nodes, ctx->ralloc_ctx);
   util_dynarray_init(&ctx->sampler_metadata_nodes, ctx->ralloc_ctx);
   util_dynarray_init(&ctx->resources, ctx->ralloc_ctx);
   dxil_module_init(&ctx->mod, ctx->ralloc_ctx);
   ctx->mod.shader_kind = get_dxil_shader_kind(s);
   ctx->mod.major_version = 6;
   /* Use the highest shader model that's supported and can be validated */
   ctx->mod.minor_version =
         ctx->mod.major_validator = validator_version >> 16;
            if (s->info.stage <= MESA_SHADER_FRAGMENT) {
      uint64_t in_mask =
      s->info.stage == MESA_SHADER_VERTEX ?
      uint64_t out_mask =
      s->info.stage == MESA_SHADER_FRAGMENT ?
                           NIR_PASS_V(s, dxil_nir_lower_fquantize2f16);
   NIR_PASS_V(s, nir_lower_frexp);
   NIR_PASS_V(s, nir_lower_flrp, 16 | 32 | 64, true);
   NIR_PASS_V(s, nir_lower_io, nir_var_shader_in | nir_var_shader_out, type_size_vec4, nir_lower_io_lower_64bit_to_32);
   NIR_PASS_V(s, dxil_nir_ensure_position_writes);
   NIR_PASS_V(s, dxil_nir_lower_system_values);
            /* Do a round of optimization to try to vectorize loads/stores. Otherwise the addresses used for loads
   * might be too opaque for the pass to see that they're next to each other. */
            /* Vectorize UBO/SSBO accesses aggressively. This can help increase alignment to enable us to do better
   * chunking of loads and stores after lowering bit sizes. Ignore load/store size limitations here, we'll
   * address them with lower_mem_access_bit_sizes */
   nir_load_store_vectorize_options vectorize_opts = {
      .callback = vectorize_filter,
      };
            /* Now that they're bloated to the max, address bit size restrictions and overall size limitations for
   * a single load/store op. */
   struct lower_mem_bit_sizes_data mem_size_data = { s->options, opts };
   nir_lower_mem_access_bit_sizes_options mem_size_options = {
      .modes = nir_var_mem_ubo | nir_var_mem_ssbo,
   .callback = lower_mem_access_bit_sizes_cb,
   .may_lower_unaligned_stores_to_atomics = true,
      };
            /* Lastly, conver byte-address UBO loads to vec-addressed. This pass can also deal with selecting sub-
   * components from the load and dealing with vec-straddling loads. */
            if (opts->shader_model_max < SHADER_MODEL_6_6) {
      /* In a later pass, load_helper_invocation will be lowered to sample mask based fallback,
   * so both load- and is- will be emulated eventually.
   */
               if (ctx->mod.shader_kind == DXIL_HULL_SHADER)
            if (ctx->mod.shader_kind == DXIL_HULL_SHADER ||
      ctx->mod.shader_kind == DXIL_DOMAIN_SHADER) {
   /* Make sure any derefs are gone after lower_io before updating tess level vars */
   NIR_PASS_V(s, nir_opt_dce);
                        NIR_PASS_V(s, nir_remove_dead_variables,
            if (!allocate_sysvalues(ctx))
            NIR_PASS_V(s, dxil_nir_lower_sysval_to_load_input, ctx->system_value);
            /* This needs to be after any copy prop is done to prevent these movs from being erased */
   NIR_PASS_V(s, dxil_nir_move_consts);
                     if (debug_dxil & DXIL_DEBUG_VERBOSE)
            if (!emit_module(ctx, opts)) {
      debug_printf("D3D12: dxil_container_add_module failed\n");
   retval = false;
               if (debug_dxil & DXIL_DEBUG_DUMP_MODULE) {
      struct dxil_dumper *dumper = dxil_dump_create();
   dxil_dump_module(dumper, &ctx->mod);
   fprintf(stderr, "\n");
   dxil_dump_buf_to_file(dumper, stderr);
   fprintf(stderr, "\n\n");
               struct dxil_container container;
   dxil_container_init(&container);
   /* Native low precision disables min-precision */
   if (ctx->mod.feats.native_low_precision)
         if (!dxil_container_add_features(&container, &ctx->mod.feats)) {
      debug_printf("D3D12: dxil_container_add_features failed\n");
   retval = false;
               if (!dxil_container_add_io_signature(&container,
                              debug_printf("D3D12: failed to write input signature\n");
   retval = false;
               if (!dxil_container_add_io_signature(&container,
                              debug_printf("D3D12: failed to write output signature\n");
   retval = false;
               if ((ctx->mod.shader_kind == DXIL_HULL_SHADER ||
      ctx->mod.shader_kind == DXIL_DOMAIN_SHADER) &&
   !dxil_container_add_io_signature(&container,
                           debug_printf("D3D12: failed to write patch constant signature\n");
   retval = false;
               struct dxil_validation_state validation_state;
   memset(&validation_state, 0, sizeof(validation_state));
            if (!dxil_container_add_state_validation(&container,&ctx->mod,
            debug_printf("D3D12: failed to write state-validation\n");
   retval = false;
               if (!dxil_container_add_module(&container, &ctx->mod)) {
      debug_printf("D3D12: failed to write module\n");
   retval = false;
               if (!dxil_container_write(&container, blob)) {
      debug_printf("D3D12: dxil_container_write failed\n");
   retval = false;
      }
            if (debug_dxil & DXIL_DEBUG_DUMP_BLOB) {
      static int shader_id = 0;
   char buffer[64];
   snprintf(buffer, sizeof(buffer), "shader_%s_%d.blob",
         debug_printf("Try to write blob to %s\n", buffer);
   FILE *f = fopen(buffer, "wb");
   if (f) {
      fwrite(blob->data, 1, blob->size, f);
               out:
      dxil_module_release(&ctx->mod);
   ralloc_free(ctx->ralloc_ctx);
   free(ctx);
      }
