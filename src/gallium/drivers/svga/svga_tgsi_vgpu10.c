   /**********************************************************
   * Copyright 1998-2022 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      /**
   * @file svga_tgsi_vgpu10.c
   *
   * TGSI -> VGPU10 shader translation.
   *
   * \author Mingcheng Chen
   * \author Brian Paul
   */
      #include "util/compiler.h"
   #include "pipe/p_shader_tokens.h"
   #include "pipe/p_defines.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_info.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_scan.h"
   #include "tgsi/tgsi_strings.h"
   #include "tgsi/tgsi_two_side.h"
   #include "tgsi/tgsi_aa_point.h"
   #include "tgsi/tgsi_util.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
   #include "util/u_debug.h"
   #include "util/u_pstipple.h"
      #include "svga_context.h"
   #include "svga_debug.h"
   #include "svga_link.h"
   #include "svga_shader.h"
   #include "svga_tgsi.h"
      #include "VGPU10ShaderTokens.h"
         #define INVALID_INDEX 99999
   #define MAX_INTERNAL_TEMPS 4
   #define MAX_SYSTEM_VALUES 4
   #define MAX_IMMEDIATE_COUNT \
         #define MAX_TEMP_ARRAYS 64  /* Enough? */
      /**
   * Clipping is complicated.  There's four different cases which we
   * handle during VS/GS shader translation:
   */
   enum clipping_mode
   {
      CLIP_NONE,     /**< No clipping enabled */
   CLIP_LEGACY,   /**< The shader has no clipping declarations or code but
                     CLIP_DISTANCE, /**< The shader already declares clip distance output
               CLIP_VERTEX    /**< The shader declares a clip vertex output register and
                  };
         /* Shader signature info */
   struct svga_shader_signature
   {
      SVGA3dDXShaderSignatureHeader header;
   SVGA3dDXShaderSignatureEntry inputs[PIPE_MAX_SHADER_INPUTS];
   SVGA3dDXShaderSignatureEntry outputs[PIPE_MAX_SHADER_OUTPUTS];
      };
      static inline void
   set_shader_signature_entry(SVGA3dDXShaderSignatureEntry *e,
                                 {
      e->registerIndex = index;
   e->semanticName = sgnName;
   e->mask = mask;
   e->componentType = compType;
      };
      static const SVGA3dDXSignatureSemanticName
   tgsi_semantic_to_sgn_name[TGSI_SEMANTIC_COUNT] = {
      SVGADX_SIGNATURE_SEMANTIC_NAME_POSITION,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_IS_FRONT_FACE,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_PRIMITIVE_ID,
   SVGADX_SIGNATURE_SEMANTIC_NAME_INSTANCE_ID,
   SVGADX_SIGNATURE_SEMANTIC_NAME_VERTEX_ID,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_CLIP_DISTANCE,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_VIEWPORT_ARRAY_INDEX,
   SVGADX_SIGNATURE_SEMANTIC_NAME_RENDER_TARGET_ARRAY_INDEX,
   SVGADX_SIGNATURE_SEMANTIC_NAME_SAMPLE_INDEX,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_INSTANCE_ID,
   SVGADX_SIGNATURE_SEMANTIC_NAME_VERTEX_ID,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
   SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED,
      };
         /**
   * Map tgsi semantic name to SVGA signature semantic name
   */
   static inline SVGA3dDXSignatureSemanticName
   map_tgsi_semantic_to_sgn_name(enum tgsi_semantic name)
   {
               /* Do a few asserts here to spot check the mapping */
   assert(tgsi_semantic_to_sgn_name[TGSI_SEMANTIC_PRIMID] ==
         assert(tgsi_semantic_to_sgn_name[TGSI_SEMANTIC_VIEWPORT_INDEX] ==
         assert(tgsi_semantic_to_sgn_name[TGSI_SEMANTIC_INVOCATIONID] ==
               }
      enum reemit_mode {
      REEMIT_FALSE = 0,
   REEMIT_TRUE = 1,
      };
      struct svga_raw_buf_tmp {
      bool indirect;
   unsigned buffer_index:8;
   unsigned element_index:8;
      };
      struct svga_shader_emitter_v10
   {
      /* The token output buffer */
   unsigned size;
   char *buf;
            /* Information about the shader and state (does not change) */
   struct svga_compile_key key;
   struct tgsi_shader_info info;
   unsigned unit;
            unsigned cur_tgsi_token;     /**< current tgsi token position */
   unsigned inst_start_token;
   bool discard_instruction; /**< throw away current instruction? */
   bool reemit_instruction;  /**< reemit current instruction */
   bool reemit_tgsi_instruction;  /**< reemit current tgsi instruction */
   bool skip_instruction;    /**< skip current instruction */
   bool use_sampler_state_mapping; /* use sampler state mapping */
            union tgsi_immediate_data immediates[MAX_IMMEDIATE_COUNT][4];
   double (*immediates_dbl)[2];
   unsigned num_immediates;      /**< Number of immediates emitted */
   unsigned common_immediate_pos[20];  /**< literals for common immediates */
   unsigned num_common_immediates;
   unsigned num_immediates_emitted;
   unsigned num_new_immediates;        /** pending immediates to be declared */
   unsigned immediates_block_start_token;
            unsigned num_outputs;      /**< include any extra outputs */
                              /* Temporary Registers */
   unsigned num_shader_temps; /**< num of temps used by original shader */
   unsigned internal_temp_count;  /**< currently allocated internal temps */
   struct {
         } temp_arrays[MAX_TEMP_ARRAYS];
            /** Map TGSI temp registers to VGPU10 temp array IDs and indexes */
   struct {
      unsigned arrayId, index;
                        /** Number of constants used by original shader for each constant buffer.
   * The size should probably always match with that of svga_state.constbufs.
   */
            /* Raw constant buffers */
   unsigned raw_buf_srv_start_index;  /* starting srv index for raw buffers */
   unsigned raw_bufs;                 /* raw buffers bitmask */
   unsigned raw_buf_tmp_index;        /* starting temp index for raw buffers */
   unsigned raw_buf_cur_tmp_index;    /* current temp index for raw buffers */
            /* Samplers */
   unsigned num_samplers;
   bool sampler_view[PIPE_MAX_SAMPLERS];  /**< True if sampler view exists*/
   uint8_t sampler_target[PIPE_MAX_SAMPLERS];  /**< TGSI_TEXTURE_x */
            /* Images */
   unsigned num_images;
   unsigned image_mask;
   struct tgsi_declaration_image image[PIPE_MAX_SHADER_IMAGES];
            /* Shader buffers */
   unsigned num_shader_bufs;
   unsigned raw_shaderbuf_srv_start_index;  /* starting srv index for raw shaderbuf */
            /* HW atomic buffers */
   unsigned num_atomic_bufs;
   unsigned atomic_bufs_mask;
   unsigned max_atomic_counter_index;
                     /* Index Range declaration */
   struct {
      unsigned start_index;
   unsigned count;
   bool required;
   unsigned operandType;
   unsigned size;
               /* Address regs (really implemented with temps) */
   unsigned num_address_regs;
            /* Output register usage masks */
            /* To map TGSI system value index to VGPU shader input indexes */
            struct {
      /* vertex position scale/translation */
   unsigned out_index;  /**< the real position output reg */
   unsigned tmp_index;  /**< the fake/temp position output reg */
   unsigned so_index;   /**< the non-adjusted position output reg */
   unsigned prescale_cbuf_index;  /* index to the const buf for prescale */
   unsigned prescale_scale_index, prescale_trans_index;
   unsigned num_prescale;      /* number of prescale factor in const buf */
   unsigned viewport_index;
   unsigned need_prescale:1;
               /* Shader limits */
   unsigned max_vs_inputs;
   unsigned max_vs_outputs;
            /* For vertex shaders only */
   struct {
      /* viewport constant */
            unsigned vertex_id_bias_index;
   unsigned vertex_id_sys_index;
            /* temp index of adjusted vertex attributes */
               /* For fragment shaders only */
   struct {
      unsigned color_out_index[PIPE_MAX_COLOR_BUFS];  /**< the real color output regs */
   unsigned num_color_outputs;
   unsigned color_tmp_index;  /**< fake/temp color output reg */
            /* front-face */
   unsigned face_input_index; /**< real fragment shader face reg (bool) */
            unsigned pstipple_sampler_unit;
            unsigned fragcoord_input_index;  /**< real fragment position input reg */
                     unsigned sample_pos_sys_index; /**< TGSI index of sample pos sys value */
            /** TGSI index of sample mask input sys value */
            /* layer */
   unsigned layer_input_index;    /**< TGSI index of layer */
                        /* For geometry shaders only */
   struct {
      VGPU10_PRIMITIVE prim_type;/**< VGPU10 primitive type */
   VGPU10_PRIMITIVE_TOPOLOGY prim_topology; /**< VGPU10 primitive topology */
   unsigned input_size;       /**< size of input arrays */
   unsigned prim_id_index;    /**< primitive id register index */
   unsigned max_out_vertices; /**< maximum number of output vertices */
   unsigned invocations;
            unsigned viewport_index_out_index;
               /* For tessellation control shaders only */
   struct {
      unsigned vertices_per_patch_index;     /**< vertices_per_patch system value index */
   unsigned imm_index;                    /**< immediate for tcs */
   unsigned invocation_id_sys_index;      /**< invocation id */
   unsigned invocation_id_tmp_index;
   unsigned instruction_token_pos;        /* token pos for the first instruction */
   unsigned control_point_input_index;    /* control point input register index */
   unsigned control_point_addr_index;     /* control point input address register */
   unsigned control_point_out_index;      /* control point output register index */
   unsigned control_point_tmp_index;      /* control point temporary register */
   unsigned control_point_out_count;      /* control point output count */
   bool  control_point_phase;          /* true if in control point phase */
   bool  fork_phase_add_signature;     /* true if needs to add signature in fork phase */
   unsigned patch_generic_out_count;      /* per-patch generic output count */
   unsigned patch_generic_out_index;      /* per-patch generic output register index*/
   unsigned patch_generic_tmp_index;      /* per-patch generic temporary register index*/
   unsigned prim_id_index;                /* primitive id */
   struct {
      unsigned out_index;      /* real tessinner output register */
   unsigned temp_index;     /* tessinner temp register */
      } inner;
   struct {
      unsigned out_index;      /* real tessouter output register */
   unsigned temp_index;     /* tessouter temp register */
                  /* For tessellation evaluation shaders only */
   struct {
      enum mesa_prim prim_mode;
   enum pipe_tess_spacing spacing;
   bool vertices_order_cw;
   bool point_mode;
   unsigned tesscoord_sys_index;
   unsigned swizzle_max;
   unsigned prim_id_index;                /* primitive id */
   struct {
      unsigned in_index;       /* real tessinner input register */
   unsigned temp_index;     /* tessinner temp register */
      } inner;
   struct {
      unsigned in_index;       /* real tessouter input register */
   unsigned temp_index;     /* tessouter temp register */
                  struct {
      unsigned block_width;       /* thread group size in x dimension */
   unsigned block_height;      /* thread group size in y dimension */
   unsigned block_depth;       /* thread group size in z dimension */
   unsigned thread_id_index;   /* thread id tgsi index */
   unsigned block_id_index;    /* block id tgsi index */
   bool shared_memory_declared;    /* set if shared memory is declared */
   struct {
      unsigned tgsi_index;   /* grid size tgsi index */
                  /* For vertex or geometry shaders */
   enum clipping_mode clip_mode;
   unsigned clip_dist_out_index; /**< clip distance output register index */
   unsigned clip_dist_tmp_index; /**< clip distance temporary register */
            /** Index of temporary holding the clipvertex coordinate */
   unsigned clip_vertex_out_index; /**< clip vertex output register index */
            /* user clip plane constant slot indexes */
            unsigned num_output_writes;
                     unsigned reserved_token;        /* index to the reserved token */
            /* For all shaders: const reg index for RECT coord scaling */
            /* For all shaders: const reg index for texture buffer size */
            /** Which texture units are doing shadow comparison in the shader code */
            /* VS/TCS/TES/GS/FS Linkage info */
   struct shader_linkage linkage;
            /* Shader signature */
                     /* For util_debug_message */
            /* current loop depth in shader */
      };
         static void emit_tcs_input_declarations(struct svga_shader_emitter_v10 *emit);
   static void emit_tcs_output_declarations(struct svga_shader_emitter_v10 *emit);
   static bool emit_temporaries_declaration(struct svga_shader_emitter_v10 *emit);
   static bool emit_constant_declaration(struct svga_shader_emitter_v10 *emit);
   static bool emit_sampler_declarations(struct svga_shader_emitter_v10 *emit);
   static bool emit_resource_declarations(struct svga_shader_emitter_v10 *emit);
   static bool emit_vgpu10_immediates_block(struct svga_shader_emitter_v10 *emit);
   static bool emit_index_range_declaration(struct svga_shader_emitter_v10 *emit);
   static void emit_image_declarations(struct svga_shader_emitter_v10 *emit);
   static void emit_shader_buf_declarations(struct svga_shader_emitter_v10 *emit);
   static void emit_atomic_buf_declarations(struct svga_shader_emitter_v10 *emit);
   static void emit_temp_prescale_instructions(struct svga_shader_emitter_v10 *emit);
      static bool
   emit_post_helpers(struct svga_shader_emitter_v10 *emit);
      static bool
   emit_vertex(struct svga_shader_emitter_v10 *emit,
            static bool
   emit_vgpu10_instruction(struct svga_shader_emitter_v10 *emit,
                  static void
   emit_input_declaration(struct svga_shader_emitter_v10 *emit,
                        VGPU10_OPCODE_TYPE opcodeType,
   VGPU10_OPERAND_TYPE operandType,
   VGPU10_OPERAND_INDEX_DIMENSION dim,
   unsigned index, unsigned size,
   VGPU10_SYSTEM_NAME name,
   VGPU10_OPERAND_NUM_COMPONENTS numComp,
   VGPU10_OPERAND_4_COMPONENT_SELECTION_MODE selMode,
         static bool
   emit_rawbuf_instruction(struct svga_shader_emitter_v10 *emit,
                  static void
   create_temp_array(struct svga_shader_emitter_v10 *emit,
                  static char err_buf[128];
      static bool
   expand(struct svga_shader_emitter_v10 *emit)
   {
      char *new_buf;
            if (emit->buf != err_buf)
         else
            if (!new_buf) {
      emit->ptr = err_buf;
   emit->buf = err_buf;
   emit->size = sizeof(err_buf);
               emit->size = newsize;
   emit->ptr = new_buf + (emit->ptr - emit->buf);
   emit->buf = new_buf;
      }
      /**
   * Create and initialize a new svga_shader_emitter_v10 object.
   */
   static struct svga_shader_emitter_v10 *
   alloc_emitter(void)
   {
               if (!emit)
            /* to initialize the output buffer */
   emit->size = 512;
   if (!expand(emit)) {
      FREE(emit);
      }
      }
      /**
   * Free an svga_shader_emitter_v10 object.
   */
   static void
   free_emitter(struct svga_shader_emitter_v10 *emit)
   {
      assert(emit);
   FREE(emit->buf);    /* will be NULL if translation succeeded */
      }
      static inline bool
   reserve(struct svga_shader_emitter_v10 *emit,
         {
      while (emit->ptr - emit->buf + nr_dwords * sizeof(uint32) >= emit->size) {
      if (!expand(emit))
                  }
      static bool
   emit_dword(struct svga_shader_emitter_v10 *emit, uint32 dword)
   {
      if (!reserve(emit, 1))
            *(uint32 *)emit->ptr = dword;
   emit->ptr += sizeof dword;
      }
      static bool
   emit_dwords(struct svga_shader_emitter_v10 *emit,
               {
      if (!reserve(emit, nr))
            memcpy(emit->ptr, dwords, nr * sizeof *dwords);
   emit->ptr += nr * sizeof *dwords;
      }
      /** Return the number of tokens in the emitter's buffer */
   static unsigned
   emit_get_num_tokens(const struct svga_shader_emitter_v10 *emit)
   {
         }
         /**
   * Check for register overflow.  If we overflow we'll set an
   * error flag.  This function can be called for register declarations
   * or use as src/dst instruction operands.
   * \param type  register type.  One of VGPU10_OPERAND_TYPE_x
         * \param index  the register index
   */
   static void
   check_register_index(struct svga_shader_emitter_v10 *emit,
         {
               switch (operandType) {
   case VGPU10_OPERAND_TYPE_TEMP:
   case VGPU10_OPERAND_TYPE_INDEXABLE_TEMP:
   case VGPU10_OPCODE_DCL_TEMPS:
      if (index >= VGPU10_MAX_TEMPS) {
         }
      case VGPU10_OPERAND_TYPE_CONSTANT_BUFFER:
   case VGPU10_OPCODE_DCL_CONSTANT_BUFFER:
      if (index >= VGPU10_MAX_CONSTANT_BUFFER_ELEMENT_COUNT) {
         }
      case VGPU10_OPERAND_TYPE_INPUT:
   case VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID:
   case VGPU10_OPCODE_DCL_INPUT:
   case VGPU10_OPCODE_DCL_INPUT_SGV:
   case VGPU10_OPCODE_DCL_INPUT_SIV:
   case VGPU10_OPCODE_DCL_INPUT_PS:
   case VGPU10_OPCODE_DCL_INPUT_PS_SGV:
   case VGPU10_OPCODE_DCL_INPUT_PS_SIV:
      if ((emit->unit == PIPE_SHADER_VERTEX &&
      index >= emit->max_vs_inputs) ||
   (emit->unit == PIPE_SHADER_GEOMETRY &&
   index >= emit->max_gs_inputs) ||
   (emit->unit == PIPE_SHADER_FRAGMENT &&
   index >= VGPU10_MAX_FS_INPUTS) ||
   (emit->unit == PIPE_SHADER_TESS_CTRL &&
   index >= VGPU11_MAX_HS_INPUT_CONTROL_POINTS) ||
   (emit->unit == PIPE_SHADER_TESS_EVAL &&
   index >= VGPU11_MAX_DS_INPUT_CONTROL_POINTS)) {
      }
      case VGPU10_OPERAND_TYPE_OUTPUT:
   case VGPU10_OPCODE_DCL_OUTPUT:
   case VGPU10_OPCODE_DCL_OUTPUT_SGV:
   case VGPU10_OPCODE_DCL_OUTPUT_SIV:
      /* Note: we are skipping two output indices in tcs for
   * tessinner/outer levels. Implementation will not exceed
   * number of output count but it allows index to go beyond
   * VGPU11_MAX_HS_OUTPUTS.
   * Index will never be >= index >= VGPU11_MAX_HS_OUTPUTS + 2
   */
   if ((emit->unit == PIPE_SHADER_VERTEX &&
      index >= emit->max_vs_outputs) ||
   (emit->unit == PIPE_SHADER_GEOMETRY &&
   index >= VGPU10_MAX_GS_OUTPUTS) ||
   (emit->unit == PIPE_SHADER_FRAGMENT &&
   index >= VGPU10_MAX_FS_OUTPUTS) ||
   (emit->unit == PIPE_SHADER_TESS_CTRL &&
   index >= VGPU11_MAX_HS_OUTPUTS + 2) ||
   (emit->unit == PIPE_SHADER_TESS_EVAL &&
   index >= VGPU11_MAX_DS_OUTPUTS)) {
      }
      case VGPU10_OPERAND_TYPE_SAMPLER:
   case VGPU10_OPCODE_DCL_SAMPLER:
      if (index >= VGPU10_MAX_SAMPLERS) {
         }
      case VGPU10_OPERAND_TYPE_RESOURCE:
   case VGPU10_OPCODE_DCL_RESOURCE:
      if (index >= VGPU10_MAX_RESOURCES) {
         }
      case VGPU10_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
      if (index >= MAX_IMMEDIATE_COUNT) {
         }
      case VGPU10_OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
   case VGPU10_OPERAND_TYPE_INPUT_GS_INSTANCE_ID:
   case VGPU10_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID:
   case VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT:
   case VGPU10_OPERAND_TYPE_INPUT_DOMAIN_POINT:
   case VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT:
   case VGPU10_OPERAND_TYPE_INPUT_THREAD_GROUP_ID:
   case VGPU10_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:
      /* nothing */
      default:
      assert(0);
               if (emit->register_overflow && !overflow_before) {
      debug_printf("svga: vgpu10 register overflow (reg %u, index %u)\n",
         }
         /**
   * Examine misc state to determine the clipping mode.
   */
   static void
   determine_clipping_mode(struct svga_shader_emitter_v10 *emit)
   {
      /* num_written_clipdistance in the shader info for tessellation
   * control shader is always 0 because the TGSI_PROPERTY_NUM_CLIPDIST_ENABLED
   * is not defined for this shader. So we go through all the output declarations
   * to set the num_written_clipdistance. This is just to determine the
   * clipping mode.
   */
   if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      unsigned i;
   for (i = 0; i < emit->info.num_outputs; i++) {
      if (emit->info.output_semantic_name[i] == TGSI_SEMANTIC_CLIPDIST) {
      emit->info.num_written_clipdistance =
                     if (emit->info.num_written_clipdistance > 0) {
         }
   else if (emit->info.writes_clipvertex) {
         }
   else if (emit->key.clip_plane_enable && emit->key.last_vertex_stage) {
      /*
   * Only the last shader in the vertex processing stage needs to
   * handle the legacy clip mode.
   */
      }
   else {
            }
         /**
   * For clip distance register declarations and clip distance register
   * writes we need to mask the declaration usage or instruction writemask
   * (respectively) against the set of the really-enabled clipping planes.
   *
   * The piglit test spec/glsl-1.30/execution/clipping/vs-clip-distance-enables
   * has a VS that writes to all 8 clip distance registers, but the plane enable
   * flags are a subset of that.
   *
   * This function is used to apply the plane enable flags to the register
   * declaration or instruction writemask.
   *
   * \param writemask  the declaration usage mask or instruction writemask
   * \param clip_reg_index  which clip plane register is being declared/written.
   *                        The legal values are 0 and 1 (two clip planes per
   *                        register, for a total of 8 clip planes)
   */
   static unsigned
   apply_clip_plane_mask(struct svga_shader_emitter_v10 *emit,
         {
                        /* four clip planes per clip register: */
   shift = clip_reg_index * 4;
               }
         /**
   * Translate gallium shader type into VGPU10 type.
   */
   static VGPU10_PROGRAM_TYPE
   translate_shader_type(unsigned type)
   {
      switch (type) {
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_GEOMETRY:
         case PIPE_SHADER_FRAGMENT:
         case PIPE_SHADER_TESS_CTRL:
         case PIPE_SHADER_TESS_EVAL:
         case PIPE_SHADER_COMPUTE:
         default:
      assert(!"Unexpected shader type");
         }
         /**
   * Translate a TGSI_OPCODE_x into a VGPU10_OPCODE_x
   * Note: we only need to translate the opcodes for "simple" instructions,
   * as seen below.  All other opcodes are handled/translated specially.
   */
   static VGPU10_OPCODE_TYPE
   translate_opcode(enum tgsi_opcode opcode)
   {
      switch (opcode) {
   case TGSI_OPCODE_MOV:
         case TGSI_OPCODE_MUL:
         case TGSI_OPCODE_ADD:
         case TGSI_OPCODE_DP3:
         case TGSI_OPCODE_DP4:
         case TGSI_OPCODE_MIN:
         case TGSI_OPCODE_MAX:
         case TGSI_OPCODE_MAD:
         case TGSI_OPCODE_SQRT:
         case TGSI_OPCODE_FRC:
         case TGSI_OPCODE_FLR:
         case TGSI_OPCODE_FSEQ:
         case TGSI_OPCODE_FSGE:
         case TGSI_OPCODE_FSNE:
         case TGSI_OPCODE_DDX:
         case TGSI_OPCODE_DDY:
         case TGSI_OPCODE_RET:
         case TGSI_OPCODE_DIV:
         case TGSI_OPCODE_IDIV:
         case TGSI_OPCODE_DP2:
         case TGSI_OPCODE_BRK:
         case TGSI_OPCODE_IF:
         case TGSI_OPCODE_ELSE:
         case TGSI_OPCODE_ENDIF:
         case TGSI_OPCODE_CEIL:
         case TGSI_OPCODE_I2F:
         case TGSI_OPCODE_NOT:
         case TGSI_OPCODE_TRUNC:
         case TGSI_OPCODE_SHL:
         case TGSI_OPCODE_AND:
         case TGSI_OPCODE_OR:
         case TGSI_OPCODE_XOR:
         case TGSI_OPCODE_CONT:
         case TGSI_OPCODE_EMIT:
         case TGSI_OPCODE_ENDPRIM:
         case TGSI_OPCODE_BGNLOOP:
         case TGSI_OPCODE_ENDLOOP:
         case TGSI_OPCODE_ENDSUB:
         case TGSI_OPCODE_NOP:
         case TGSI_OPCODE_END:
         case TGSI_OPCODE_F2I:
         case TGSI_OPCODE_IMAX:
         case TGSI_OPCODE_IMIN:
         case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_MOD:
         case TGSI_OPCODE_IMUL_HI:
         case TGSI_OPCODE_INEG:
         case TGSI_OPCODE_ISHR:
         case TGSI_OPCODE_ISGE:
         case TGSI_OPCODE_ISLT:
         case TGSI_OPCODE_F2U:
         case TGSI_OPCODE_UADD:
         case TGSI_OPCODE_U2F:
         case TGSI_OPCODE_UCMP:
         case TGSI_OPCODE_UMAD:
         case TGSI_OPCODE_UMAX:
         case TGSI_OPCODE_UMIN:
         case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMUL_HI:
         case TGSI_OPCODE_USEQ:
         case TGSI_OPCODE_USGE:
         case TGSI_OPCODE_USHR:
         case TGSI_OPCODE_USLT:
         case TGSI_OPCODE_USNE:
         case TGSI_OPCODE_SWITCH:
         case TGSI_OPCODE_CASE:
         case TGSI_OPCODE_DEFAULT:
         case TGSI_OPCODE_ENDSWITCH:
         case TGSI_OPCODE_FSLT:
         case TGSI_OPCODE_ROUND:
         /* Begin SM5 opcodes */
   case TGSI_OPCODE_F2D:
         case TGSI_OPCODE_D2F:
         case TGSI_OPCODE_DMUL:
         case TGSI_OPCODE_DADD:
         case TGSI_OPCODE_DMAX:
         case TGSI_OPCODE_DMIN:
         case TGSI_OPCODE_DSEQ:
         case TGSI_OPCODE_DSGE:
         case TGSI_OPCODE_DSLT:
         case TGSI_OPCODE_DSNE:
         case TGSI_OPCODE_IBFE:
         case TGSI_OPCODE_UBFE:
         case TGSI_OPCODE_BFI:
         case TGSI_OPCODE_BREV:
         case TGSI_OPCODE_POPC:
         case TGSI_OPCODE_LSB:
         case TGSI_OPCODE_IMSB:
         case TGSI_OPCODE_UMSB:
         case TGSI_OPCODE_INTERP_CENTROID:
         case TGSI_OPCODE_INTERP_SAMPLE:
         case TGSI_OPCODE_BARRIER:
         case TGSI_OPCODE_DFMA:
         case TGSI_OPCODE_FMA:
            /* DX11.1 Opcodes */
   case TGSI_OPCODE_DDIV:
         case TGSI_OPCODE_DRCP:
         case TGSI_OPCODE_D2I:
         case TGSI_OPCODE_D2U:
         case TGSI_OPCODE_I2D:
         case TGSI_OPCODE_U2D:
            case TGSI_OPCODE_SAMPLE_POS:
      /* Note: we never actually get this opcode because there's no GLSL
   * function to query multisample resource sample positions.  There's
   * only the TGSI_SEMANTIC_SAMPLEPOS system value which contains the
   * position of the current sample in the render target.
   */
      case TGSI_OPCODE_SAMPLE_INFO:
      /* NOTE: we never actually get this opcode because the GLSL compiler
   * implements the gl_NumSamples variable with a simple constant in the
   * constant buffer.
   */
      default:
      assert(!"Unexpected TGSI opcode in translate_opcode()");
         }
         /**
   * Translate a TGSI register file type into a VGPU10 operand type.
   * \param array  is the TGSI_FILE_TEMPORARY register an array?
   */
   static VGPU10_OPERAND_TYPE
   translate_register_file(enum tgsi_file_type file, bool array)
   {
      switch (file) {
   case TGSI_FILE_CONSTANT:
         case TGSI_FILE_INPUT:
         case TGSI_FILE_OUTPUT:
         case TGSI_FILE_TEMPORARY:
      return array ? VGPU10_OPERAND_TYPE_INDEXABLE_TEMP
      case TGSI_FILE_IMMEDIATE:
      /* all immediates are 32-bit values at this time so
   * VGPU10_OPERAND_TYPE_IMMEDIATE64 is not possible at this time.
   */
      case TGSI_FILE_SAMPLER:
         case TGSI_FILE_SYSTEM_VALUE:
                     default:
      assert(!"Bad tgsi register file!");
         }
         /**
   * Emit a null dst register
   */
   static void
   emit_null_dst_register(struct svga_shader_emitter_v10 *emit)
   {
               operand.value = 0;
   operand.operandType = VGPU10_OPERAND_TYPE_NULL;
               }
         /**
   * If the given register is a temporary, return the array ID.
   * Else return zero.
   */
   static unsigned
   get_temp_array_id(const struct svga_shader_emitter_v10 *emit,
         {
      if (file == TGSI_FILE_TEMPORARY) {
         }
   else {
            }
         /**
   * If the given register is a temporary, convert the index from a TGSI
   * TEMPORARY index to a VGPU10 temp index.
   */
   static unsigned
   remap_temp_index(const struct svga_shader_emitter_v10 *emit,
         {
      if (file == TGSI_FILE_TEMPORARY) {
         }
   else {
            }
         /**
   * Setup the operand0 fields related to indexing (1D, 2D, relative, etc).
   * Note: the operandType field must already be initialized.
   * \param file  the register file being accessed
   * \param indirect  using indirect addressing of the register file?
   * \param index2D  if true, 2-D indexing is being used (const or temp registers)
   * \param indirect2D  if true, 2-D indirect indexing being used (for const buf)
   */
   static VGPU10OperandToken0
   setup_operand0_indexing(struct svga_shader_emitter_v10 *emit,
                           {
      VGPU10_OPERAND_INDEX_REPRESENTATION index0Rep, index1Rep;
            /*
   * Compute index dimensions
   */
   if (operand0.operandType == VGPU10_OPERAND_TYPE_IMMEDIATE32 ||
      operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID ||
   operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_GS_INSTANCE_ID ||
   operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_THREAD_ID ||
   operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP ||
   operand0.operandType == VGPU10_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID) {
   /* there's no swizzle for in-line immediates */
   indexDim = VGPU10_OPERAND_INDEX_0D;
      }
   else if (operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_DOMAIN_POINT) {
         }
   else {
                  /*
   * Compute index representation(s) (immediate vs relative).
   */
   if (indexDim == VGPU10_OPERAND_INDEX_2D) {
      index0Rep = indirect2D ? VGPU10_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE
            index1Rep = indirect ? VGPU10_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE
      }
   else if (indexDim == VGPU10_OPERAND_INDEX_1D) {
      index0Rep = indirect ? VGPU10_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE
               }
   else {
      index0Rep = 0;
               operand0.indexDimension = indexDim;
   operand0.index0Representation = index0Rep;
               }
         /**
   * Emit the operand for expressing an address register for indirect indexing.
   * Note that the address register is really just a temp register.
   * \param addr_reg_index  which address register to use
   */
   static void
   emit_indirect_register(struct svga_shader_emitter_v10 *emit,
         {
      unsigned tmp_reg_index;
                              /* operand0 is a simple temporary register, selecting one component */
   operand0.value = 0;
   operand0.operandType = VGPU10_OPERAND_TYPE_TEMP;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
   operand0.index0Representation = VGPU10_OPERAND_INDEX_IMMEDIATE32;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SELECT_1_MODE;
   operand0.swizzleX = 0;
   operand0.swizzleY = 1;
   operand0.swizzleZ = 2;
            emit_dword(emit, operand0.value);
      }
         /**
   * Translate the dst register of a TGSI instruction and emit VGPU10 tokens.
   * \param emit  the emitter context
   * \param reg  the TGSI dst register to translate
   */
   static void
   emit_dst_register(struct svga_shader_emitter_v10 *emit,
         {
      enum tgsi_file_type file = reg->Register.File;
   unsigned index = reg->Register.Index;
   const enum tgsi_semantic sem_name = emit->info.output_semantic_name[index];
   const unsigned sem_index = emit->info.output_semantic_index[index];
   unsigned writemask = reg->Register.WriteMask;
   const bool indirect = reg->Register.Indirect;
   unsigned tempArrayId = get_temp_array_id(emit, file, index);
   bool index2d = reg->Register.Dimension || tempArrayId > 0;
            if (file == TGSI_FILE_TEMPORARY) {
                  if (file == TGSI_FILE_OUTPUT) {
      if (emit->unit == PIPE_SHADER_VERTEX ||
      emit->unit == PIPE_SHADER_GEOMETRY ||
   emit->unit == PIPE_SHADER_TESS_EVAL) {
   if (index == emit->vposition.out_index &&
      emit->vposition.tmp_index != INVALID_INDEX) {
   /* replace OUTPUT[POS] with TEMP[POS].  We need to store the
   * vertex position result in a temporary so that we can modify
   * it in the post_helper() code.
   */
   file = TGSI_FILE_TEMPORARY;
      }
   else if (sem_name == TGSI_SEMANTIC_CLIPDIST &&
            /* replace OUTPUT[CLIPDIST] with TEMP[CLIPDIST].
   * We store the clip distance in a temporary first, then
   * we'll copy it to the shadow copy and to CLIPDIST with the
   * enabled planes mask in emit_clip_distance_instructions().
   */
   file = TGSI_FILE_TEMPORARY;
      }
   else if (sem_name == TGSI_SEMANTIC_CLIPVERTEX &&
            /* replace the CLIPVERTEX output register with a temporary */
   assert(emit->clip_mode == CLIP_VERTEX);
   assert(sem_index == 0);
   file = TGSI_FILE_TEMPORARY;
      }
                     /* set the saturate modifier of the instruction
   * to clamp the vertex color.
   */
   VGPU10OpcodeToken0 *token =
            }
   else if (sem_name == TGSI_SEMANTIC_VIEWPORT_INDEX &&
            file = TGSI_FILE_TEMPORARY;
         }
   else if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (sem_name == TGSI_SEMANTIC_POSITION) {
      /* Fragment depth output register */
   operand0.value = 0;
   operand0.operandType = VGPU10_OPERAND_TYPE_OUTPUT_DEPTH;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
   operand0.numComponents = VGPU10_OPERAND_1_COMPONENT;
   emit_dword(emit, operand0.value);
      }
   else if (sem_name == TGSI_SEMANTIC_SAMPLEMASK) {
      /* Fragment sample mask output */
   operand0.value = 0;
   operand0.operandType = VGPU10_OPERAND_TYPE_OUTPUT_COVERAGE_MASK;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
   operand0.numComponents = VGPU10_OPERAND_1_COMPONENT;
   emit_dword(emit, operand0.value);
      }
   else if (index == emit->fs.color_out_index[0] &&
      emit->fs.color_tmp_index != INVALID_INDEX) {
   /* replace OUTPUT[COLOR] with TEMP[COLOR].  We need to store the
   * fragment color result in a temporary so that we can read it
   * it in the post_helper() code.
   */
   file = TGSI_FILE_TEMPORARY;
      }
   else {
      /* Typically, for fragment shaders, the output register index
   * matches the color semantic index.  But not when we write to
   * the fragment depth register.  In that case, OUT[0] will be
   * fragdepth and OUT[1] will be the 0th color output.  We need
   * to use the semantic index for color outputs.
   */
                        }
   else if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      if (index == emit->tcs.inner.tgsi_index) {
      /* replace OUTPUT[TESSLEVEL] with temp. We are storing it
   * in temporary for now so that will be store into appropriate
   * registers in post_helper() in patch constant phase.
   */
   if (emit->tcs.control_point_phase) {
      /* Discard writing into tessfactor in control point phase */
      }
   else {
      file = TGSI_FILE_TEMPORARY;
         }
   else if (index == emit->tcs.outer.tgsi_index) {
      /* replace OUTPUT[TESSLEVEL] with temp. We are storing it
   * in temporary for now so that will be store into appropriate
   * registers in post_helper().
   */
   if (emit->tcs.control_point_phase) {
      /* Discard writing into tessfactor in control point phase */
      }
   else {
      file = TGSI_FILE_TEMPORARY;
         }
   else if (index >= emit->tcs.patch_generic_out_index &&
            index < (emit->tcs.patch_generic_out_index +
   if (emit->tcs.control_point_phase) {
      /* Discard writing into generic patch constant outputs in
            }
   else {
      if (emit->reemit_instruction) {
      /* Store results of reemitted instruction in temporary register. */
   file = TGSI_FILE_TEMPORARY;
   index = emit->tcs.patch_generic_tmp_index +
         /**
   * Temporaries for patch constant data can be done
   * as indexable temporaries.
                           }
   else {
      /* If per-patch outputs is been read in shader, we
   * reemit instruction and store results in temporaries in
   * patch constant phase. */
   if (emit->info.reads_perpatch_outputs) {
                  }
   else if (reg->Register.Dimension) {
      /* Only control point outputs are declared 2D in tgsi */
   if (emit->tcs.control_point_phase) {
      if (emit->reemit_instruction) {
      /* Store results of reemitted instruction in temporary register. */
   index2d = false;
   file = TGSI_FILE_TEMPORARY;
   index = emit->tcs.control_point_tmp_index +
            }
   else {
      /* The mapped control point outputs are 1-D */
   index2d = false;
   if (emit->info.reads_pervertex_outputs) {
      /* If per-vertex outputs is been read in shader, we
   * reemit instruction and store results in temporaries
                        if (sem_name == TGSI_SEMANTIC_CLIPDIST &&
      emit->clip_dist_tmp_index != INVALID_INDEX) {
   /* replace OUTPUT[CLIPDIST] with TEMP[CLIPDIST].
   * We store the clip distance in a temporary first, then
   * we'll copy it to the shadow copy and to CLIPDIST with the
   * enabled planes mask in emit_clip_distance_instructions().
   */
   file = TGSI_FILE_TEMPORARY;
      }
   else if (sem_name == TGSI_SEMANTIC_CLIPVERTEX &&
            /* replace the CLIPVERTEX output register with a temporary */
   assert(emit->clip_mode == CLIP_VERTEX);
   assert(sem_index == 0);
   file = TGSI_FILE_TEMPORARY;
         }
   else {
      /* Discard writing into control point outputs in
                              /* init operand tokens to all zero */
                     /* the operand has a writemask */
            /* Which of the four dest components to write to. Note that we can use a
   * simple assignment here since TGSI writemasks match VGPU10 writemasks.
   */
   STATIC_ASSERT(TGSI_WRITEMASK_X == VGPU10_OPERAND_4_COMPONENT_MASK_X);
            /* translate TGSI register file type to VGPU10 operand type */
                     operand0 = setup_operand0_indexing(emit, operand0, file, indirect,
            /* Emit tokens */
   emit_dword(emit, operand0.value);
   if (tempArrayId > 0) {
                           if (indirect) {
            }
         /**
   * Check if temporary register needs to be initialize when
   * shader is not using indirect addressing for temporary and uninitialized
   * temporary is not used in loop. In these two scenarios, we cannot
   * determine if temporary is initialized or not.
   */
   static bool
   need_temp_reg_initialization(struct svga_shader_emitter_v10 *emit,
         {
      if (!(emit->info.indirect_files & (1u << TGSI_FILE_TEMPORARY))
      && emit->current_loop_depth == 0) {
   if (!emit->temp_map[index].initialized &&
      emit->temp_map[index].index < emit->num_shader_temps) {
                     }
         /**
   * Translate a src register of a TGSI instruction and emit VGPU10 tokens.
   * In quite a few cases, we do register substitution.  For example, if
   * the TGSI register is the front/back-face register, we replace that with
   * a temp register containing a value we computed earlier.
   */
   static void
   emit_src_register(struct svga_shader_emitter_v10 *emit,
         {
      enum tgsi_file_type file = reg->Register.File;
   unsigned index = reg->Register.Index;
   bool indirect = reg->Register.Indirect;
   unsigned tempArrayId = get_temp_array_id(emit, file, index);
   bool index2d = (reg->Register.Dimension ||
               unsigned index2 = tempArrayId > 0 ? tempArrayId : reg->Dimension.Index;
   bool indirect2d = reg->Dimension.Indirect;
   unsigned swizzleX = reg->Register.SwizzleX;
   unsigned swizzleY = reg->Register.SwizzleY;
   unsigned swizzleZ = reg->Register.SwizzleZ;
   unsigned swizzleW = reg->Register.SwizzleW;
   const bool absolute = reg->Register.Absolute;
   const bool negate = reg->Register.Negate;
   VGPU10OperandToken0 operand0;
                     if (emit->unit == PIPE_SHADER_FRAGMENT){
      if (file == TGSI_FILE_INPUT) {
      if (index == emit->fs.face_input_index) {
      /* Replace INPUT[FACE] with TEMP[FACE] */
   file = TGSI_FILE_TEMPORARY;
      }
   else if (index == emit->fs.fragcoord_input_index) {
      /* Replace INPUT[POSITION] with TEMP[POSITION] */
   file = TGSI_FILE_TEMPORARY;
      }
   else if (index == emit->fs.layer_input_index) {
      /* Replace INPUT[LAYER] with zero.x */
   file = TGSI_FILE_IMMEDIATE;
   index = emit->fs.layer_imm_index;
      }
   else {
      /* We remap fragment shader inputs to that FS input indexes
   * match up with VS/GS output indexes.
   */
         }
   else if (file == TGSI_FILE_SYSTEM_VALUE) {
      if (index == emit->fs.sample_pos_sys_index) {
      assert(emit->version >= 41);
   /* Current sample position is in a temp register */
   file = TGSI_FILE_TEMPORARY;
      }
   else if (index == emit->fs.sample_mask_in_sys_index) {
      /* Emitted as vCoverage0.x */
   /* According to GLSL spec, the gl_SampleMaskIn array has ceil(s / 32)
   * elements where s is the maximum number of color samples supported
   * by the implementation.
   */
   operand0.value = 0;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_COVERAGE_MASK;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SELECT_1_MODE;
   emit_dword(emit, operand0.value);
      }
   else {
      /* Map the TGSI system value to a VGPU10 input register */
   assert(index < ARRAY_SIZE(emit->system_value_indexes));
   file = TGSI_FILE_INPUT;
            }
   else if (emit->unit == PIPE_SHADER_GEOMETRY) {
      if (file == TGSI_FILE_INPUT) {
      if (index == emit->gs.prim_id_index) {
      operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
      }
      }
   else if (file == TGSI_FILE_SYSTEM_VALUE &&
            /* Emitted as vGSInstanceID0.x */
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_GS_INSTANCE_ID;
         }
   else if (emit->unit == PIPE_SHADER_VERTEX) {
      if (file == TGSI_FILE_INPUT) {
      /* if input is adjusted... */
   if ((emit->key.vs.adjust_attrib_w_1 |
      emit->key.vs.adjust_attrib_itof |
   emit->key.vs.adjust_attrib_utof |
   emit->key.vs.attrib_is_bgra |
   emit->key.vs.attrib_puint_to_snorm |
   emit->key.vs.attrib_puint_to_uscaled |
   emit->key.vs.attrib_puint_to_sscaled) & (1 << index)) {
   file = TGSI_FILE_TEMPORARY;
         }
   else if (file == TGSI_FILE_SYSTEM_VALUE) {
      if (index == emit->vs.vertex_id_sys_index &&
      emit->vs.vertex_id_tmp_index != INVALID_INDEX) {
   file = TGSI_FILE_TEMPORARY;
   index = emit->vs.vertex_id_tmp_index;
      }
   else {
      /* Map the TGSI system value to a VGPU10 input register */
   assert(index < ARRAY_SIZE(emit->system_value_indexes));
   file = TGSI_FILE_INPUT;
            }
               if (file == TGSI_FILE_SYSTEM_VALUE) {
      if (index == emit->tcs.vertices_per_patch_index) {
      /**
   * if source register is the system value for vertices_per_patch,
   * replace it with the immediate.
   */
   file = TGSI_FILE_IMMEDIATE;
   index = emit->tcs.imm_index;
      }
   else if (index == emit->tcs.invocation_id_sys_index) {
      if (emit->tcs.control_point_phase) {
      /**
   * Emitted as vOutputControlPointID.x
   */
   operand0.numComponents = VGPU10_OPERAND_1_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_MASK_MODE;
   operand0.mask = 0;
   emit_dword(emit, operand0.value);
      }
   else {
      /* There is no control point ID input declaration in
   * the patch constant phase in hull shader.
   * Since for now we are emitting all instructions in
   * the patch constant phase, we are replacing the
   * control point ID reference with the immediate 0.
   */
   file = TGSI_FILE_IMMEDIATE;
   index = emit->tcs.imm_index;
         }
   else if (index == emit->tcs.prim_id_index) {
      /**
   * Emitted as vPrim.x
   */
   operand0.numComponents = VGPU10_OPERAND_1_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID;
         }
   else if (file == TGSI_FILE_INPUT) {
      index = emit->linkage.input_map[index];
   if (!emit->tcs.control_point_phase) {
      /* Emitted as vicp */
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT;
         }
   else if (file == TGSI_FILE_OUTPUT) {
      if ((index >= emit->tcs.patch_generic_out_index &&
      index < (emit->tcs.patch_generic_out_index +
         index == emit->tcs.inner.tgsi_index ||
   index == emit->tcs.outer.tgsi_index) {
   if (emit->tcs.control_point_phase) {
         }
   else {
      /* Device doesn't allow reading from output so
   * use corresponding temporary register as source */
   file = TGSI_FILE_TEMPORARY;
   if (index == emit->tcs.inner.tgsi_index) {
         }
   else if (index == emit->tcs.outer.tgsi_index) {
         }
   else {
                        /**
   * Temporaries for patch constant data can be done
   * as indexable temporaries.
   */
   tempArrayId = get_temp_array_id(emit, file, index);
   index2d = tempArrayId > 0;
         }
   else if (index2d) {
      if (emit->tcs.control_point_phase) {
      /* Device doesn't allow reading from output so
   * use corresponding temporary register as source */
   file = TGSI_FILE_TEMPORARY;
   index2d = false;
   index = emit->tcs.control_point_tmp_index +
      }
   else {
                  }
   else if (emit->unit == PIPE_SHADER_TESS_EVAL) {
      if (file == TGSI_FILE_SYSTEM_VALUE) {
      if (index == emit->tes.tesscoord_sys_index) {
      /**
   * Emitted as vDomain
   */
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
                  /* Make sure swizzles are of those components allowed according
   * to the tessellator domain.
   */
   swizzleX = MIN2(swizzleX, emit->tes.swizzle_max);
   swizzleY = MIN2(swizzleY, emit->tes.swizzle_max);
   swizzleZ = MIN2(swizzleZ, emit->tes.swizzle_max);
      }
   else if (index == emit->tes.inner.tgsi_index) {
      file = TGSI_FILE_TEMPORARY;
      }
   else if (index == emit->tes.outer.tgsi_index) {
      file = TGSI_FILE_TEMPORARY;
      }
   else if (index == emit->tes.prim_id_index) {
      /**
   * Emitted as vPrim.x
   */
   operand0.numComponents = VGPU10_OPERAND_1_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID;
            }
   else if (file == TGSI_FILE_INPUT) {
      if (index2d) {
      /* 2D input is emitted as vcp (input control point). */
                  /* index specifies the element index and is remapped
   * to align with the tcs output index.
                     }
   else {
      if (index < emit->key.tes.tessfactor_index)
      /* index specifies the generic patch index.
                     operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT;
            }
   else if (emit->unit == PIPE_SHADER_COMPUTE) {
      if (file == TGSI_FILE_SYSTEM_VALUE) {
      if (index == emit->cs.thread_id_index) {
      operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP;
      } else if (index == emit->cs.block_id_index) {
      operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_INPUT_THREAD_GROUP_ID;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SWIZZLE_MODE;
   operand0.swizzleX = swizzleX;
   operand0.swizzleY = swizzleY;
   operand0.swizzleZ = swizzleZ;
   operand0.swizzleW = swizzleW;
   emit_dword(emit, operand0.value);
      } else if (index == emit->cs.grid_size.tgsi_index) {
      file = TGSI_FILE_IMMEDIATE;
                     if (file == TGSI_FILE_ADDRESS) {
      index = emit->address_reg_index[index];
               if (file == TGSI_FILE_CONSTANT) {
      /**
   * If this constant buffer is to be bound as srv raw buffer,
   * then we have to load the constant to a temp first before
   * it can be used as a source in the instruction.
   * This is accomplished in two passes. The first pass is to
   * identify if there is any constbuf to rawbuf translation.
   * If there isn't, emit the instruction as usual.
   * If there is, then we save the constant buffer reference info,
   * and then instead of emitting the instruction at the end
   * of the instruction, it will trigger a second pass of parsing
   * this instruction. Before it starts the parsing, it will
   * load the referenced raw buffer elements to temporaries.
   * Then it will emit the instruction that replaces the
   * constant buffer replaces with the corresponding temporaries.
   */
   if (emit->raw_bufs & (1 << index2)) {
                                                /* If it is indirect index, save the temporary
   * address index, otherwise, save the immediate index.
   */
   if (indirect) {
      emit->raw_buf_tmp[tmpIdx].element_index =
         emit->raw_buf_tmp[tmpIdx].element_rel =
      }
   else {
                        emit->raw_buf_cur_tmp_index++;
   emit->reemit_rawbuf_instruction = REEMIT_TRUE;
   emit->discard_instruction = true;
      }
   else {
      /* In the reemitting process, replace the constant buffer
   * reference with temporary.
   */
   file = TGSI_FILE_TEMPORARY;
   index = emit->raw_buf_cur_tmp_index + emit->raw_buf_tmp_index;
   index2d = false;
   indirect = false;
                     if (file == TGSI_FILE_TEMPORARY) {
      if (need_temp_reg_initialization(emit, index)) {
      emit->initialize_temp_index = index;
                  if (operand0.value == 0) {
      /* if operand0 was not set above for a special case, do the general
   * case now.
   */
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
      }
   operand0 = setup_operand0_indexing(emit, operand0, file, indirect,
            if (operand0.operandType != VGPU10_OPERAND_TYPE_IMMEDIATE32 &&
      operand0.operandType != VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID) {
   /* there's no swizzle for in-line immediates */
   if (swizzleX == swizzleY &&
      swizzleX == swizzleZ &&
   swizzleX == swizzleW) {
      }
   else {
                  operand0.swizzleX = swizzleX;
   operand0.swizzleY = swizzleY;
   operand0.swizzleZ = swizzleZ;
            if (absolute || negate) {
      operand0.extended = 1;
   operand1.extendedOperandType = VGPU10_EXTENDED_OPERAND_MODIFIER;
   if (absolute && !negate)
         if (!absolute && negate)
         if (absolute && negate)
                           /* Emit the operand tokens */
   emit_dword(emit, operand0.value);
   if (operand0.extended)
            if (operand0.operandType == VGPU10_OPERAND_TYPE_IMMEDIATE32) {
      /* Emit the four float/int in-line immediate values */
   unsigned *c;
   assert(index < ARRAY_SIZE(emit->immediates));
   assert(file == TGSI_FILE_IMMEDIATE);
   assert(swizzleX < 4);
   assert(swizzleY < 4);
   assert(swizzleZ < 4);
   assert(swizzleW < 4);
   c = (unsigned *) emit->immediates[index];
   emit_dword(emit, c[swizzleX]);
   emit_dword(emit, c[swizzleY]);
   emit_dword(emit, c[swizzleZ]);
      }
   else if (operand0.indexDimension >= VGPU10_OPERAND_INDEX_1D) {
      /* Emit the register index(es) */
   if (index2d) {
               if (indirect2d) {
                              if (indirect) {
      assert(operand0.operandType != VGPU10_OPERAND_TYPE_TEMP);
            }
         /**
   * Emit a resource operand (for use with a SAMPLE instruction).
   */
   static void
   emit_resource_register(struct svga_shader_emitter_v10 *emit,
         {
                        /* init */
            operand0.operandType = VGPU10_OPERAND_TYPE_RESOURCE;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SWIZZLE_MODE;
   operand0.swizzleX = VGPU10_COMPONENT_X;
   operand0.swizzleY = VGPU10_COMPONENT_Y;
   operand0.swizzleZ = VGPU10_COMPONENT_Z;
            emit_dword(emit, operand0.value);
      }
         /**
   * Emit a sampler operand (for use with a SAMPLE instruction).
   */
   static void
   emit_sampler_register(struct svga_shader_emitter_v10 *emit,
         {
      VGPU10OperandToken0 operand0;
                     if ((emit->shadow_compare_units & (1 << unit)) && emit->use_sampler_state_mapping)
                     /* init */
            operand0.operandType = VGPU10_OPERAND_TYPE_SAMPLER;
            emit_dword(emit, operand0.value);
      }
         /**
   * Emit an operand which reads the IS_FRONT_FACING register.
   */
   static void
   emit_face_register(struct svga_shader_emitter_v10 *emit)
   {
      VGPU10OperandToken0 operand0;
            /* init */
            operand0.operandType = VGPU10_OPERAND_TYPE_INPUT;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SELECT_1_MODE;
            operand0.swizzleX = VGPU10_COMPONENT_X;
   operand0.swizzleY = VGPU10_COMPONENT_X;
   operand0.swizzleZ = VGPU10_COMPONENT_X;
            emit_dword(emit, operand0.value);
      }
         /**
   * Emit tokens for the "rasterizer" register used by the SAMPLE_POS
   * instruction.
   */
   static void
   emit_rasterizer_register(struct svga_shader_emitter_v10 *emit)
   {
               /* init */
            /* No register index for rasterizer index (there's only one) */
   operand0.operandType = VGPU10_OPERAND_TYPE_RASTERIZER;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SWIZZLE_MODE;
   operand0.swizzleX = VGPU10_COMPONENT_X;
   operand0.swizzleY = VGPU10_COMPONENT_Y;
   operand0.swizzleZ = VGPU10_COMPONENT_Z;
               }
         /**
   * Emit tokens for the "stream" register used by the 
   * DCL_STREAM, CUT_STREAM, EMIT_STREAM instructions.
   */
   static void
   emit_stream_register(struct svga_shader_emitter_v10 *emit, unsigned index)
   {
               /* init */
            /* No register index for rasterizer index (there's only one) */
   operand0.operandType = VGPU10_OPERAND_TYPE_STREAM;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            emit_dword(emit, operand0.value);
      }
         /**
   * Emit the token for a VGPU10 opcode, with precise parameter.
   * \param saturate   clamp result to [0,1]?
   */
   static void
   emit_opcode_precise(struct svga_shader_emitter_v10 *emit,
         {
               token0.value = 0;  /* init all fields to zero */
   token0.opcodeType = vgpu10_opcode;
   token0.instructionLength = 0; /* Filled in by end_emit_instruction() */
            /* Mesa's GLSL IR -> TGSI translator will set the TGSI precise flag for
   * 'invariant' declarations.  Only set preciseValues=1 if we have SM5.
   */
                        }
         /**
   * Emit the token for a VGPU10 opcode.
   * \param saturate   clamp result to [0,1]?
   */
   static void
   emit_opcode(struct svga_shader_emitter_v10 *emit,
         {
         }
         /**
   * Emit the token for a VGPU10 resinfo instruction.
   * \param modifier   return type modifier, _uint or _rcpFloat.
   *                   TODO: We may want to remove this parameter if it will
   *                   only ever be used as _uint.
   */
   static void
   emit_opcode_resinfo(struct svga_shader_emitter_v10 *emit,
         {
               token0.value = 0;  /* init all fields to zero */
   token0.opcodeType = VGPU10_OPCODE_RESINFO;
   token0.instructionLength = 0; /* Filled in by end_emit_instruction() */
               }
         /**
   * Emit opcode tokens for a texture sample instruction.  Texture instructions
   * can be rather complicated (texel offsets, etc) so we have this specialized
   * function.
   */
   static void
   emit_sample_opcode(struct svga_shader_emitter_v10 *emit,
               {
      VGPU10OpcodeToken0 token0;
            token0.value = 0;  /* init all fields to zero */
   token0.opcodeType = vgpu10_opcode;
   token0.instructionLength = 0; /* Filled in by end_emit_instruction() */
            if (offsets[0] || offsets[1] || offsets[2]) {
      assert(offsets[0] >= VGPU10_MIN_TEXEL_FETCH_OFFSET);
   assert(offsets[1] >= VGPU10_MIN_TEXEL_FETCH_OFFSET);
   assert(offsets[2] >= VGPU10_MIN_TEXEL_FETCH_OFFSET);
   assert(offsets[0] <= VGPU10_MAX_TEXEL_FETCH_OFFSET);
   assert(offsets[1] <= VGPU10_MAX_TEXEL_FETCH_OFFSET);
            token0.extended = 1;
   token1.value = 0;
   token1.opcodeType = VGPU10_EXTENDED_OPCODE_SAMPLE_CONTROLS;
   token1.offsetU = offsets[0];
   token1.offsetV = offsets[1];
               emit_dword(emit, token0.value);
   if (token0.extended) {
            }
         /**
   * Emit a DISCARD opcode token.
   * If nonzero is set, we'll discard the fragment if the X component is not 0.
   * Otherwise, we'll discard the fragment if the X component is 0.
   */
   static void
   emit_discard_opcode(struct svga_shader_emitter_v10 *emit, bool nonzero)
   {
               opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DISCARD;
   if (nonzero)
               }
         /**
   * We need to call this before we begin emitting a VGPU10 instruction.
   */
   static void
   begin_emit_instruction(struct svga_shader_emitter_v10 *emit)
   {
      assert(emit->inst_start_token == 0);
   /* Save location of the instruction's VGPU10OpcodeToken0 token.
   * Note, we can't save a pointer because it would become invalid if
   * we have to realloc the output buffer.
   */
      }
         /**
   * We need to call this after we emit the last token of a VGPU10 instruction.
   * This function patches in the opcode token's instructionLength field.
   */
   static void
   end_emit_instruction(struct svga_shader_emitter_v10 *emit)
   {
      VGPU10OpcodeToken0 *tokens = (VGPU10OpcodeToken0 *) emit->buf;
                     if (emit->discard_instruction) {
      /* Back up the emit->ptr to where this instruction started so
   * that we discard the current instruction.
   */
      }
   else {
      /* Compute instruction length and patch that into the start of
   * the instruction.
   */
                                 emit->inst_start_token = 0; /* reset to zero for error checking */
      }
         /**
   * Return index for a free temporary register.
   */
   static unsigned
   get_temp_index(struct svga_shader_emitter_v10 *emit)
   {
      assert(emit->internal_temp_count < MAX_INTERNAL_TEMPS);
      }
         /**
   * Release the temporaries which were generated by get_temp_index().
   */
   static void
   free_temp_indexes(struct svga_shader_emitter_v10 *emit)
   {
         }
         /**
   * Create a tgsi_full_src_register.
   */
   static struct tgsi_full_src_register
   make_src_reg(enum tgsi_file_type file, unsigned index)
   {
               memset(&reg, 0, sizeof(reg));
   reg.Register.File = file;
   reg.Register.Index = index;
   reg.Register.SwizzleX = TGSI_SWIZZLE_X;
   reg.Register.SwizzleY = TGSI_SWIZZLE_Y;
   reg.Register.SwizzleZ = TGSI_SWIZZLE_Z;
   reg.Register.SwizzleW = TGSI_SWIZZLE_W;
      }
         /**
   * Create a tgsi_full_src_register with a swizzle such that all four
   * vector components have the same scalar value.
   */
   static struct tgsi_full_src_register
   make_src_scalar_reg(enum tgsi_file_type file, unsigned index, unsigned component)
   {
               assert(component >= TGSI_SWIZZLE_X);
            memset(&reg, 0, sizeof(reg));
   reg.Register.File = file;
   reg.Register.Index = index;
   reg.Register.SwizzleX =
   reg.Register.SwizzleY =
   reg.Register.SwizzleZ =
   reg.Register.SwizzleW = component;
      }
         /**
   * Create a tgsi_full_src_register for a temporary.
   */
   static struct tgsi_full_src_register
   make_src_temp_reg(unsigned index)
   {
         }
         /**
   * Create a tgsi_full_src_register for a constant.
   */
   static struct tgsi_full_src_register
   make_src_const_reg(unsigned index)
   {
         }
         /**
   * Create a tgsi_full_src_register for an immediate constant.
   */
   static struct tgsi_full_src_register
   make_src_immediate_reg(unsigned index)
   {
         }
         /**
   * Create a tgsi_full_dst_register.
   */
   static struct tgsi_full_dst_register
   make_dst_reg(enum tgsi_file_type file, unsigned index)
   {
               memset(&reg, 0, sizeof(reg));
   reg.Register.File = file;
   reg.Register.Index = index;
   reg.Register.WriteMask = TGSI_WRITEMASK_XYZW;
      }
         /**
   * Create a tgsi_full_dst_register for a temporary.
   */
   static struct tgsi_full_dst_register
   make_dst_temp_reg(unsigned index)
   {
         }
         /**
   * Create a tgsi_full_dst_register for an output.
   */
   static struct tgsi_full_dst_register
   make_dst_output_reg(unsigned index)
   {
         }
         /**
   * Create negated tgsi_full_src_register.
   */
   static struct tgsi_full_src_register
   negate_src(const struct tgsi_full_src_register *reg)
   {
      struct tgsi_full_src_register neg = *reg;
   neg.Register.Negate = !reg->Register.Negate;
      }
      /**
   * Create absolute value of a tgsi_full_src_register.
   */
   static struct tgsi_full_src_register
   absolute_src(const struct tgsi_full_src_register *reg)
   {
      struct tgsi_full_src_register absolute = *reg;
   absolute.Register.Absolute = 1;
      }
         /** Return the named swizzle term from the src register */
   static inline unsigned
   get_swizzle(const struct tgsi_full_src_register *reg, enum tgsi_swizzle term)
   {
      switch (term) {
   case TGSI_SWIZZLE_X:
         case TGSI_SWIZZLE_Y:
         case TGSI_SWIZZLE_Z:
         case TGSI_SWIZZLE_W:
         default:
      assert(!"Bad swizzle");
         }
         /**
   * Create swizzled tgsi_full_src_register.
   */
   static struct tgsi_full_src_register
   swizzle_src(const struct tgsi_full_src_register *reg,
               {
      struct tgsi_full_src_register swizzled = *reg;
   /* Note: we swizzle the current swizzle */
   swizzled.Register.SwizzleX = get_swizzle(reg, swizzleX);
   swizzled.Register.SwizzleY = get_swizzle(reg, swizzleY);
   swizzled.Register.SwizzleZ = get_swizzle(reg, swizzleZ);
   swizzled.Register.SwizzleW = get_swizzle(reg, swizzleW);
      }
         /**
   * Create swizzled tgsi_full_src_register where all the swizzle
   * terms are the same.
   */
   static struct tgsi_full_src_register
   scalar_src(const struct tgsi_full_src_register *reg, enum tgsi_swizzle swizzle)
   {
      struct tgsi_full_src_register swizzled = *reg;
   /* Note: we swizzle the current swizzle */
   swizzled.Register.SwizzleX =
   swizzled.Register.SwizzleY =
   swizzled.Register.SwizzleZ =
   swizzled.Register.SwizzleW = get_swizzle(reg, swizzle);
      }
         /**
   * Create new tgsi_full_dst_register with writemask.
   * \param mask  bitmask of TGSI_WRITEMASK_[XYZW]
   */
   static struct tgsi_full_dst_register
   writemask_dst(const struct tgsi_full_dst_register *reg, unsigned mask)
   {
      struct tgsi_full_dst_register masked = *reg;
   masked.Register.WriteMask = mask;
      }
         /**
   * Check if the register's swizzle is XXXX, YYYY, ZZZZ, or WWWW.
   */
   static bool
   same_swizzle_terms(const struct tgsi_full_src_register *reg)
   {
      return (reg->Register.SwizzleX == reg->Register.SwizzleY &&
            }
         /**
   * Search the vector for the value 'x' and return its position.
   */
   static int
   find_imm_in_vec4(const union tgsi_immediate_data vec[4],
         {
      unsigned i;
   for (i = 0; i < 4; i++) {
      if (vec[i].Int == x.Int)
      }
      }
         /**
   * Helper used by make_immediate_reg(), make_immediate_reg_4().
   */
   static int
   find_immediate(struct svga_shader_emitter_v10 *emit,
         {
      const unsigned endIndex = emit->num_immediates;
                     /* Search immediates for x, y, z, w */
   for (i = startIndex; i < endIndex; i++) {
      if (x.Int == emit->immediates[i][0].Int ||
      x.Int == emit->immediates[i][1].Int ||
   x.Int == emit->immediates[i][2].Int ||
   x.Int == emit->immediates[i][3].Int) {
         }
   /* immediate not declared yet */
      }
         /**
   * As above, but search for a double[2] pair.
   */
   static int
   find_immediate_dbl(struct svga_shader_emitter_v10 *emit,
         {
      const unsigned endIndex = emit->num_immediates;
                     /* Search immediates for x, y, z, w */
   for (i = 0; i < endIndex; i++) {
      if (x == emit->immediates_dbl[i][0] &&
      y == emit->immediates_dbl[i][1]) {
         }
   /* Should never try to use an immediate value that wasn't pre-declared */
   assert(!"find_immediate_dbl() failed!");
      }
            /**
   * Return a tgsi_full_src_register for an immediate/literal
   * union tgsi_immediate_data[4] value.
   * Note: the values must have been previously declared/allocated in
   * emit_pre_helpers().  And, all of x,y,z,w must be located in the same
   * vec4 immediate.
   */
   static struct tgsi_full_src_register
   make_immediate_reg_4(struct svga_shader_emitter_v10 *emit,
         {
      struct tgsi_full_src_register reg;
            for (i = 0; i < emit->num_common_immediates; i++) {
      /* search for first component value */
   int immpos = find_immediate(emit, imm[0], i);
                     /* find remaining components within the immediate vector */
   x = find_imm_in_vec4(emit->immediates[immpos], imm[0]);
   y = find_imm_in_vec4(emit->immediates[immpos], imm[1]);
   z = find_imm_in_vec4(emit->immediates[immpos], imm[2]);
            if (x >=0 &&  y >= 0 && z >= 0 && w >= 0) {
      /* found them all */
   memset(&reg, 0, sizeof(reg));
   reg.Register.File = TGSI_FILE_IMMEDIATE;
   reg.Register.Index = immpos;
   reg.Register.SwizzleX = x;
   reg.Register.SwizzleY = y;
   reg.Register.SwizzleZ = z;
   reg.Register.SwizzleW = w;
      }
                        /* Just return IMM[0].xxxx */
   memset(&reg, 0, sizeof(reg));
   reg.Register.File = TGSI_FILE_IMMEDIATE;
      }
         /**
   * Return a tgsi_full_src_register for an immediate/literal
   * union tgsi_immediate_data value of the form {value, value, value, value}.
   * \sa make_immediate_reg_4() regarding allowed values.
   */
   static struct tgsi_full_src_register
   make_immediate_reg(struct svga_shader_emitter_v10 *emit,
         {
      struct tgsi_full_src_register reg;
                     memset(&reg, 0, sizeof(reg));
   reg.Register.File = TGSI_FILE_IMMEDIATE;
   reg.Register.Index = immpos;
   reg.Register.SwizzleX =
   reg.Register.SwizzleY =
   reg.Register.SwizzleZ =
               }
         /**
   * Return a tgsi_full_src_register for an immediate/literal float[4] value.
   * \sa make_immediate_reg_4() regarding allowed values.
   */
   static struct tgsi_full_src_register
   make_immediate_reg_float4(struct svga_shader_emitter_v10 *emit,
         {
      union tgsi_immediate_data imm[4];
   imm[0].Float = x;
   imm[1].Float = y;
   imm[2].Float = z;
   imm[3].Float = w;
      }
         /**
   * Return a tgsi_full_src_register for an immediate/literal float value
   * of the form {value, value, value, value}.
   * \sa make_immediate_reg_4() regarding allowed values.
   */
   static struct tgsi_full_src_register
   make_immediate_reg_float(struct svga_shader_emitter_v10 *emit, float value)
   {
      union tgsi_immediate_data imm;
   imm.Float = value;
      }
         /**
   * Return a tgsi_full_src_register for an immediate/literal int[4] vector.
   */
   static struct tgsi_full_src_register
   make_immediate_reg_int4(struct svga_shader_emitter_v10 *emit,
         {
      union tgsi_immediate_data imm[4];
   imm[0].Int = x;
   imm[1].Int = y;
   imm[2].Int = z;
   imm[3].Int = w;
      }
         /**
   * Return a tgsi_full_src_register for an immediate/literal int value
   * of the form {value, value, value, value}.
   * \sa make_immediate_reg_4() regarding allowed values.
   */
   static struct tgsi_full_src_register
   make_immediate_reg_int(struct svga_shader_emitter_v10 *emit, int value)
   {
      union tgsi_immediate_data imm;
   imm.Int = value;
      }
         static struct tgsi_full_src_register
   make_immediate_reg_double(struct svga_shader_emitter_v10 *emit, double value)
   {
      struct tgsi_full_src_register reg;
                     memset(&reg, 0, sizeof(reg));
   reg.Register.File = TGSI_FILE_IMMEDIATE;
   reg.Register.Index = immpos;
   reg.Register.SwizzleX = TGSI_SWIZZLE_X;
   reg.Register.SwizzleY = TGSI_SWIZZLE_Y;
   reg.Register.SwizzleZ = TGSI_SWIZZLE_Z;
               }
         /**
   * Allocate space for a union tgsi_immediate_data[4] immediate.
   * \return  the index/position of the immediate.
   */
   static unsigned
   alloc_immediate_4(struct svga_shader_emitter_v10 *emit,
         {
      unsigned n = emit->num_immediates++;
   assert(n < ARRAY_SIZE(emit->immediates));
   emit->immediates[n][0] = imm[0];
   emit->immediates[n][1] = imm[1];
   emit->immediates[n][2] = imm[2];
   emit->immediates[n][3] = imm[3];
      }
         /**
   * Allocate space for a float[4] immediate.
   * \return  the index/position of the immediate.
   */
   static unsigned
   alloc_immediate_float4(struct svga_shader_emitter_v10 *emit,
         {
      union tgsi_immediate_data imm[4];
   imm[0].Float = x;
   imm[1].Float = y;
   imm[2].Float = z;
   imm[3].Float = w;
      }
         /**
   * Allocate space for an int[4] immediate.
   * \return  the index/position of the immediate.
   */
   static unsigned
   alloc_immediate_int4(struct svga_shader_emitter_v10 *emit,
         {
      union tgsi_immediate_data imm[4];
   imm[0].Int = x;
   imm[1].Int = y;
   imm[2].Int = z;
   imm[3].Int = w;
      }
         /**
   * Add a new immediate after the immediate block has been declared.
   * Any new immediates will be appended to the immediate block after the
   * shader has been parsed.
   * \return  the index/position of the immediate.
   */
   static unsigned
   add_immediate_int(struct svga_shader_emitter_v10 *emit, int x)
   {
      union tgsi_immediate_data imm[4];
   imm[0].Int = x;
   imm[1].Int = x+1;
   imm[2].Int = x+2;
            unsigned immpos = alloc_immediate_4(emit, imm);
               }
         static unsigned
   alloc_immediate_double2(struct svga_shader_emitter_v10 *emit,
         {
      unsigned n = emit->num_immediates++;
   assert(!emit->num_immediates_emitted);
   assert(n < ARRAY_SIZE(emit->immediates));
   emit->immediates_dbl[n][0] = x;
   emit->immediates_dbl[n][1] = y;
         }
         /**
   * Allocate a shader input to store a system value.
   */
   static unsigned
   alloc_system_value_index(struct svga_shader_emitter_v10 *emit, unsigned index)
   {
      const unsigned n = emit->linkage.input_map_max + 1 + index;
   assert(index < ARRAY_SIZE(emit->system_value_indexes));
   emit->system_value_indexes[index] = n;
      }
         /**
   * Translate a TGSI immediate value (union tgsi_immediate_data[4]) to VGPU10.
   */
   static bool
   emit_vgpu10_immediate(struct svga_shader_emitter_v10 *emit,
         {
      /* We don't actually emit any code here.  We just save the
   * immediate values and emit them later.
   */
   alloc_immediate_4(emit, imm->u);
      }
         /**
   * Emit a VGPU10_CUSTOMDATA_DCL_IMMEDIATE_CONSTANT_BUFFER block
   * containing all the immediate values previously allocated
   * with alloc_immediate_4().
   */
   static bool
   emit_vgpu10_immediates_block(struct svga_shader_emitter_v10 *emit)
   {
                        token.value = 0;
   token.opcodeType = VGPU10_OPCODE_CUSTOMDATA;
            emit->immediates_block_start_token =
            /* Note: no begin/end_emit_instruction() calls */
   emit_dword(emit, token.value);
   emit_dword(emit, 2 + 4 * emit->num_immediates);
                     emit->immediates_block_next_token =
               }
         /**
   * Reemit the immediate constant buffer block to include the new
   * immediates that are allocated after the block is declared. Those
   * immediates are used as constant indices to constant buffers.
   */
   static bool
   reemit_immediates_block(struct svga_shader_emitter_v10 *emit)
   {
      unsigned num_tokens = emit_get_num_tokens(emit);
            /* Reserve room for the new immediates */
   if (!reserve(emit, 4 * num_new_immediates))
            /* Move the tokens after the immediates block to make room for the
   * new immediates.
   */
   VGPU10ProgramToken *tokens = (VGPU10ProgramToken *)emit->buf;
   char *next = (char *) (tokens + emit->immediates_block_next_token);
   char *new_next = (char *) (tokens + emit->immediates_block_next_token +
            char *end = emit->ptr;
   unsigned len = end - next;
            /* Append the new immediates to the end of the immediates block */
   char *start = (char *) (tokens + emit->immediates_block_start_token+1);
            char *new_immediates = (char *)&emit->immediates[emit->num_immediates_emitted][0];
   *(uint32 *)start = immediates_block_size + 4 * num_new_immediates;
                        }
            /**
   * Translate a fragment shader's TGSI_INTERPOLATE_x mode to a vgpu10
   * interpolation mode.
   * \return a VGPU10_INTERPOLATION_x value
   */
   static unsigned
   translate_interpolation(const struct svga_shader_emitter_v10 *emit,
               {
      if (interp == TGSI_INTERPOLATE_COLOR) {
      interp = emit->key.fs.flatshade ?
               switch (interp) {
   case TGSI_INTERPOLATE_CONSTANT:
         case TGSI_INTERPOLATE_LINEAR:
      if (interpolate_loc == TGSI_INTERPOLATE_LOC_CENTROID) {
         } else if (interpolate_loc == TGSI_INTERPOLATE_LOC_SAMPLE &&
               } else {
         }
      case TGSI_INTERPOLATE_PERSPECTIVE:
      if (interpolate_loc == TGSI_INTERPOLATE_LOC_CENTROID) {
         } else if (interpolate_loc == TGSI_INTERPOLATE_LOC_SAMPLE &&
               } else {
         }
      default:
      assert(!"Unexpected interpolation mode");
         }
         /**
   * Translate a TGSI property to VGPU10.
   * Don't emit any instructions yet, only need to gather the primitive property
   * information.  The output primitive topology might be changed later. The
   * final property instructions will be emitted as part of the pre-helper code.
   */
   static bool
   emit_vgpu10_property(struct svga_shader_emitter_v10 *emit,
         {
      static const VGPU10_PRIMITIVE primType[] = {
      VGPU10_PRIMITIVE_POINT,           /* MESA_PRIM_POINTS */
   VGPU10_PRIMITIVE_LINE,            /* MESA_PRIM_LINES */
   VGPU10_PRIMITIVE_LINE,            /* MESA_PRIM_LINE_LOOP */
   VGPU10_PRIMITIVE_LINE,            /* MESA_PRIM_LINE_STRIP */
   VGPU10_PRIMITIVE_TRIANGLE,        /* MESA_PRIM_TRIANGLES */
   VGPU10_PRIMITIVE_TRIANGLE,        /* MESA_PRIM_TRIANGLE_STRIP */
   VGPU10_PRIMITIVE_TRIANGLE,        /* MESA_PRIM_TRIANGLE_FAN */
   VGPU10_PRIMITIVE_UNDEFINED,       /* MESA_PRIM_QUADS */
   VGPU10_PRIMITIVE_UNDEFINED,       /* MESA_PRIM_QUAD_STRIP */
   VGPU10_PRIMITIVE_UNDEFINED,       /* MESA_PRIM_POLYGON */
   VGPU10_PRIMITIVE_LINE_ADJ,        /* MESA_PRIM_LINES_ADJACENCY */
   VGPU10_PRIMITIVE_LINE_ADJ,        /* MESA_PRIM_LINE_STRIP_ADJACENCY */
   VGPU10_PRIMITIVE_TRIANGLE_ADJ,    /* MESA_PRIM_TRIANGLES_ADJACENCY */
               static const VGPU10_PRIMITIVE_TOPOLOGY primTopology[] = {
      VGPU10_PRIMITIVE_TOPOLOGY_POINTLIST,     /* MESA_PRIM_POINTS */
   VGPU10_PRIMITIVE_TOPOLOGY_LINELIST,      /* MESA_PRIM_LINES */
   VGPU10_PRIMITIVE_TOPOLOGY_LINELIST,      /* MESA_PRIM_LINE_LOOP */
   VGPU10_PRIMITIVE_TOPOLOGY_LINESTRIP,     /* MESA_PRIM_LINE_STRIP */
   VGPU10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,  /* MESA_PRIM_TRIANGLES */
   VGPU10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, /* MESA_PRIM_TRIANGLE_STRIP */
   VGPU10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, /* MESA_PRIM_TRIANGLE_FAN */
   VGPU10_PRIMITIVE_TOPOLOGY_UNDEFINED,     /* MESA_PRIM_QUADS */
   VGPU10_PRIMITIVE_TOPOLOGY_UNDEFINED,     /* MESA_PRIM_QUAD_STRIP */
   VGPU10_PRIMITIVE_TOPOLOGY_UNDEFINED,     /* MESA_PRIM_POLYGON */
   VGPU10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,  /* MESA_PRIM_LINES_ADJACENCY */
   VGPU10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,  /* MESA_PRIM_LINE_STRIP_ADJACENCY */
   VGPU10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ, /* MESA_PRIM_TRIANGLES_ADJACENCY */
               static const unsigned inputArraySize[] = {
      0,       /* VGPU10_PRIMITIVE_UNDEFINED */
   1,       /* VGPU10_PRIMITIVE_POINT */
   2,       /* VGPU10_PRIMITIVE_LINE */
   3,       /* VGPU10_PRIMITIVE_TRIANGLE */
   0,
   0,
   4,       /* VGPU10_PRIMITIVE_LINE_ADJ */
               switch (prop->Property.PropertyName) {
   case TGSI_PROPERTY_GS_INPUT_PRIM:
      assert(prop->u[0].Data < ARRAY_SIZE(primType));
   emit->gs.prim_type = primType[prop->u[0].Data];
   assert(emit->gs.prim_type != VGPU10_PRIMITIVE_UNDEFINED);
   emit->gs.input_size = inputArraySize[emit->gs.prim_type];
         case TGSI_PROPERTY_GS_OUTPUT_PRIM:
      assert(prop->u[0].Data < ARRAY_SIZE(primTopology));
   emit->gs.prim_topology = primTopology[prop->u[0].Data];
   assert(emit->gs.prim_topology != VGPU10_PRIMITIVE_TOPOLOGY_UNDEFINED);
         case TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES:
      emit->gs.max_out_vertices = prop->u[0].Data;
         case TGSI_PROPERTY_GS_INVOCATIONS:
      emit->gs.invocations = prop->u[0].Data;
         case TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS:
   case TGSI_PROPERTY_NEXT_SHADER:
   case TGSI_PROPERTY_NUM_CLIPDIST_ENABLED:
      /* no-op */
         case TGSI_PROPERTY_TCS_VERTICES_OUT:
      /* This info is already captured in the shader key */
         case TGSI_PROPERTY_TES_PRIM_MODE:
      emit->tes.prim_mode = prop->u[0].Data;
         case TGSI_PROPERTY_TES_SPACING:
      emit->tes.spacing = prop->u[0].Data;
         case TGSI_PROPERTY_TES_VERTEX_ORDER_CW:
      emit->tes.vertices_order_cw = prop->u[0].Data;
         case TGSI_PROPERTY_TES_POINT_MODE:
      emit->tes.point_mode = prop->u[0].Data;
         case TGSI_PROPERTY_CS_FIXED_BLOCK_WIDTH:
      emit->cs.block_width = prop->u[0].Data;
         case TGSI_PROPERTY_CS_FIXED_BLOCK_HEIGHT:
      emit->cs.block_height = prop->u[0].Data;
         case TGSI_PROPERTY_CS_FIXED_BLOCK_DEPTH:
      emit->cs.block_depth = prop->u[0].Data;
         case TGSI_PROPERTY_FS_EARLY_DEPTH_STENCIL:
      emit->fs.forceEarlyDepthStencil = true;
         default:
      debug_printf("Unexpected TGSI property %s\n",
                  }
         static void
   emit_property_instruction(struct svga_shader_emitter_v10 *emit,
               {
      begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   if (nData)
            }
         /**
   * Emit property instructions
   */
   static void
   emit_property_instructions(struct svga_shader_emitter_v10 *emit)
   {
                        /* emit input primitive type declaration */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_GS_INPUT_PRIMITIVE;
   opcode0.primitive = emit->gs.prim_type;
            /* emit max output vertices */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT;
            if (emit->version >= 50 && emit->gs.invocations > 0) {
      opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_GS_INSTANCE_COUNT;
         }
         /**
   * A helper function to declare tessellator domain in a hull shader or
   * in the domain shader.
   */
   static void
   emit_tessellator_domain(struct svga_shader_emitter_v10 *emit,
         {
               opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_TESS_DOMAIN;
   switch (prim_mode) {
   case MESA_PRIM_QUADS:
   case MESA_PRIM_LINES:
      opcode0.tessDomain = VGPU10_TESSELLATOR_DOMAIN_QUAD;
      case MESA_PRIM_TRIANGLES:
      opcode0.tessDomain = VGPU10_TESSELLATOR_DOMAIN_TRI;
      default:
      debug_printf("Invalid tessellator prim mode %d\n", prim_mode);
      }
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
      }
         /**
   * Emit domain shader declarations.
   */
   static void
   emit_domain_shader_declarations(struct svga_shader_emitter_v10 *emit)
   {
                        /* Emit the input control point count */
   assert(emit->key.tes.vertices_per_patch >= 0 &&
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT;
   opcode0.controlPointCount = emit->key.tes.vertices_per_patch;
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
                     /* Specify a max for swizzles of the domain point according to the
   * tessellator domain type.
   */
   emit->tes.swizzle_max = emit->tes.prim_mode == MESA_PRIM_TRIANGLES ?
      }
         /**
   * Some common values like 0.0, 1.0, 0.5, etc. are frequently needed
   * to implement some instructions.  We pre-allocate those values here
   * in the immediate constant buffer.
   */
   static void
   alloc_common_immediates(struct svga_shader_emitter_v10 *emit)
   {
               emit->common_immediate_pos[n++] =
            if (emit->info.opcode_count[TGSI_OPCODE_LIT] > 0) {
      emit->common_immediate_pos[n++] =
               emit->common_immediate_pos[n++] =
            emit->common_immediate_pos[n++] =
            if (emit->info.opcode_count[TGSI_OPCODE_IMSB] > 0 ||
      emit->info.opcode_count[TGSI_OPCODE_UMSB] > 0) {
   emit->common_immediate_pos[n++] =
               if (emit->info.opcode_count[TGSI_OPCODE_UBFE] > 0 ||
      emit->info.opcode_count[TGSI_OPCODE_IBFE] > 0 ||
   emit->info.opcode_count[TGSI_OPCODE_BFI] > 0) {
   emit->common_immediate_pos[n++] =
               if (emit->key.vs.attrib_puint_to_snorm) {
      emit->common_immediate_pos[n++] =
               if (emit->key.vs.attrib_puint_to_uscaled) {
      emit->common_immediate_pos[n++] =
               if (emit->key.vs.attrib_puint_to_sscaled) {
      emit->common_immediate_pos[n++] =
            emit->common_immediate_pos[n++] =
               if (emit->vposition.num_prescale > 1) {
      unsigned i;
   for (i = 0; i < emit->vposition.num_prescale; i+=4) {
      emit->common_immediate_pos[n++] =
                           if (emit->info.opcode_count[TGSI_OPCODE_DNEG] > 0) {
      emit->common_immediate_pos[n++] =
               if (emit->info.opcode_count[TGSI_OPCODE_DSQRT] > 0 ||
      emit->info.opcode_count[TGSI_OPCODE_DTRUNC] > 0) {
   emit->common_immediate_pos[n++] =
         emit->common_immediate_pos[n++] =
               if (emit->info.opcode_count[TGSI_OPCODE_INTERP_OFFSET] > 0) {
      emit->common_immediate_pos[n++] =
                                 for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      if (emit->key.tex[i].texel_bias) {
      /* Replace 0.0f if more immediate float value is needed */
   emit->common_immediate_pos[n++] =
                        /** TODO: allocate immediates for all possible element byte offset?
   */
   if (emit->raw_bufs) {
      unsigned i;
   for (i = 7; i < 12; i+=4) {
      emit->common_immediate_pos[n++] =
                  if (emit->info.indirect_files &
      (1 << TGSI_FILE_IMAGE | 1 << TGSI_FILE_BUFFER)) {
   unsigned i;
   for (i = 7; i < 8; i+=4) {
      emit->common_immediate_pos[n++] =
                  assert(n <= ARRAY_SIZE(emit->common_immediate_pos));
      }
         /**
   * Emit hull shader declarations.
   */
   static void
   emit_hull_shader_declarations(struct svga_shader_emitter_v10 *emit)
   {
               /* Emit the input control point count */
   assert(emit->key.tcs.vertices_per_patch > 0 &&
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT;
   opcode0.controlPointCount = emit->key.tcs.vertices_per_patch;
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
            /* Emit the output control point count */
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT;
   opcode0.controlPointCount = emit->key.tcs.vertices_out;
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
            /* Emit tessellator domain */
            /* Emit tessellator output primitive */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE;
   if (emit->key.tcs.point_mode) {
         }
   else if (emit->key.tcs.prim_mode == MESA_PRIM_LINES) {
         }
   else {
      assert(emit->key.tcs.prim_mode == MESA_PRIM_QUADS ||
            if (emit->key.tcs.vertices_order_cw)
         else
      }
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
            /* Emit tessellator partitioning */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_TESS_PARTITIONING;
   switch (emit->key.tcs.spacing) {
   case PIPE_TESS_SPACING_FRACTIONAL_ODD:
      opcode0.tessPartitioning = VGPU10_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD;
      case PIPE_TESS_SPACING_FRACTIONAL_EVEN:
      opcode0.tessPartitioning = VGPU10_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN;
      case PIPE_TESS_SPACING_EQUAL:
      opcode0.tessPartitioning = VGPU10_TESSELLATOR_PARTITIONING_INTEGER;
      default:
      debug_printf("invalid tessellator spacing %d\n", emit->key.tcs.spacing);
      }
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
                     /* Declare constant registers */
            /* Declare samplers and resources */
   emit_sampler_declarations(emit);
            /* Declare images */
            /* Declare shader buffers */
            /* Declare atomic buffers */
            int nVertices = emit->key.tcs.vertices_per_patch;
   emit->tcs.imm_index =
            /* Now, emit the constant block containing all the immediates
   * declared by shader, as well as the extra ones seen above.
   */
         }
         /**
   * A helper function to determine if control point phase is needed.
   * Returns TRUE if there is control point output.
   */
   static bool
   needs_control_point_phase(struct svga_shader_emitter_v10 *emit)
   {
                        /* If output control point count does not match the input count,
   * we need a control point phase to explicitly set the output control
   * points.
   */
   if ((emit->key.tcs.vertices_per_patch != emit->key.tcs.vertices_out) &&
      emit->key.tcs.vertices_out)
         for (i = 0; i < emit->info.num_outputs; i++) {
      switch (emit->info.output_semantic_name[i]) {
   case TGSI_SEMANTIC_PATCH:
   case TGSI_SEMANTIC_TESSOUTER:
   case TGSI_SEMANTIC_TESSINNER:
         default:
            }
      }
         /**
   * A helper function to add shader signature for passthrough control point
   * phase. This signature is also generated for passthrough control point
   * phase from HLSL compiler and is needed by Metal Renderer.
   */
   static void
   emit_passthrough_control_point_signature(struct svga_shader_emitter_v10 *emit)
   {
      struct svga_shader_signature *sgn = &emit->signature;
   SVGA3dDXShaderSignatureEntry *sgnEntry;
            for (i = 0; i < emit->info.num_inputs; i++) {
      unsigned index = emit->linkage.input_map[i];
                     set_shader_signature_entry(sgnEntry, index,
                                       set_shader_signature_entry(sgnEntry, i,
                           }
         /**
   * A helper function to emit an instruction to start the control point phase
   * in the hull shader.
   */
   static void
   emit_control_point_phase_instruction(struct svga_shader_emitter_v10 *emit)
   {
               opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_HS_CONTROL_POINT_PHASE;
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
      }
         /**
   * Start the hull shader control point phase
   */
   static bool
   emit_hull_shader_control_point_phase(struct svga_shader_emitter_v10 *emit)
   {
      /* If there is no control point output, skip the control point phase. */
   if (!needs_control_point_phase(emit)) {
      if (!emit->key.tcs.vertices_out) {
      /**
   * If the tcs does not explicitly generate any control point output
   * and the tes does not use any input control point, then
   * emit an empty control point phase with zero output control
   * point count.
                  /**
   * Since this is an empty control point phase, we will need to
   * add input signatures when we parse the tcs again in the
   * patch constant phase.
   */
      }
   else {
      /**
   * Before skipping the control point phase, add the signature for
   * the passthrough control point.
   */
      }
               /* Start the control point phase in the hull shader */
            /* Declare the output control point ID */
   if (emit->tcs.invocation_id_sys_index == INVALID_INDEX) {
      /* Add invocation id declaration if it does not exist */
               emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID,
   VGPU10_OPERAND_INDEX_0D,
   0, 1,
   VGPU10_NAME_UNDEFINED,
         if (emit->tcs.prim_id_index != INVALID_INDEX) {
      emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID,
   VGPU10_OPERAND_INDEX_0D,
   0, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_0_COMPONENT,
               }
         /**
   * Start the hull shader patch constant phase and
   * do the second pass of the tcs translation and emit
   * the relevant declarations and instructions for this phase.
   */
   static bool
   emit_hull_shader_patch_constant_phase(struct svga_shader_emitter_v10 *emit,
         {
      unsigned inst_number = 0;
   bool ret = true;
                     /* Start the patch constant phase */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_HS_FORK_PHASE;
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
            /* Set the current phase to patch constant phase */
            if (emit->tcs.prim_id_index != INVALID_INDEX) {
      emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID,
   VGPU10_OPERAND_INDEX_0D,
   0, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_0_COMPONENT,
            /* Emit declarations for this phase */
   emit->index_range.required =
                  if (emit->index_range.start_index != INVALID_INDEX) {
                  emit->index_range.required =
                  if (emit->index_range.start_index != INVALID_INDEX) {
         }
                     /* Reset the token position to the first instruction token
   * in preparation for the second pass of the shader
   */
            while (!tgsi_parse_end_of_tokens(parse)) {
               assert(parse->FullToken.Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION);
   ret = emit_vgpu10_instruction(emit, inst_number++,
            /* Usually this applies to TCS only. If shader is reading output of
   * patch constant in fork phase, we should reemit all instructions
   * which are writting into output of patch constant in fork phase
   * to store results into temporaries.
   */
   assert(!(emit->reemit_instruction && emit->reemit_rawbuf_instruction));
   if (emit->reemit_instruction) {
      assert(emit->unit == PIPE_SHADER_TESS_CTRL);
   ret = emit_vgpu10_instruction(emit, inst_number,
      } else if (emit->reemit_rawbuf_instruction) {
      ret = emit_rawbuf_instruction(emit, inst_number,
               if (!ret)
                  }
         /**
   * Emit the thread group declaration for compute shader.
   */
   static void
   emit_compute_shader_declarations(struct svga_shader_emitter_v10 *emit)
   {
               opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_THREAD_GROUP;
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, emit->cs.block_width);
   emit_dword(emit, emit->cs.block_height);
   emit_dword(emit, emit->cs.block_depth);
      }
         /**
   * Emit index range declaration.
   */
   static bool
   emit_index_range_declaration(struct svga_shader_emitter_v10 *emit)
   {
      if (emit->version < 50)
            assert(emit->index_range.start_index != INVALID_INDEX);
   assert(emit->index_range.count != 0);
   assert(emit->index_range.required);
   assert(emit->index_range.operandType != VGPU10_NUM_OPERANDS);
   assert(emit->index_range.dim != 0);
            VGPU10OpcodeToken0 opcode0;
            opcode0.value = 0;
            operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.indexDimension = emit->index_range.dim;
   operand0.operandType = emit->index_range.operandType;
   operand0.mask = VGPU10_OPERAND_4_COMPONENT_MASK_ALL;
            if (emit->index_range.dim == VGPU10_OPERAND_INDEX_2D)
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
            if (emit->index_range.dim == VGPU10_OPERAND_INDEX_2D) {
      emit_dword(emit, emit->index_range.size);
   emit_dword(emit, emit->index_range.start_index);
      }
   else {
      emit_dword(emit, emit->index_range.start_index);
                        /* Reset fields in emit->index_range struct except
   * emit->index_range.required which will be reset afterwards
   */
   emit->index_range.count = 0;
   emit->index_range.operandType = VGPU10_NUM_OPERANDS;
   emit->index_range.start_index = INVALID_INDEX;
   emit->index_range.size = 0;
               }
         /**
   * Emit a vgpu10 declaration "instruction".
   * \param index  the register index
   * \param size   array size of the operand. In most cases, it is 1,
   *               but for inputs to geometry shader, the array size varies
   *               depending on the primitive type.
   */
   static void
   emit_decl_instruction(struct svga_shader_emitter_v10 *emit,
                           {
      assert(opcode0.opcodeType);
   assert(operand0.mask ||
         (operand0.operandType == VGPU10_OPERAND_TYPE_OUTPUT) ||
   (operand0.operandType == VGPU10_OPERAND_TYPE_OUTPUT_DEPTH) ||
   (operand0.operandType == VGPU10_OPERAND_TYPE_OUTPUT_COVERAGE_MASK) ||
   (operand0.operandType == VGPU10_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID) ||
   (operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID) ||
   (operand0.operandType == VGPU10_OPERAND_TYPE_INPUT_GS_INSTANCE_ID) ||
            begin_emit_instruction(emit);
                     if (operand0.indexDimension == VGPU10_OPERAND_INDEX_1D) {
      /* Next token is the index of the register to declare */
      }
   else if (operand0.indexDimension >= VGPU10_OPERAND_INDEX_2D) {
      /* Next token is the size of the register */
            /* Followed by the index of the register */
               if (name_token.value) {
                     }
         /**
   * Emit the declaration for a shader input.
   * \param opcodeType  opcode type, one of VGPU10_OPCODE_DCL_INPUTx
   * \param operandType operand type, one of VGPU10_OPERAND_TYPE_INPUT_x
   * \param dim         index dimension
   * \param index       the input register index
   * \param size        array size of the operand. In most cases, it is 1,
   *                    but for inputs to geometry shader, the array size varies
   *                    depending on the primitive type. For tessellation control
   *                    shader, the array size is the vertex count per patch.
   * \param name        one of VGPU10_NAME_x
   * \parma numComp     number of components
   * \param selMode     component selection mode
   * \param usageMask   bitfield of VGPU10_OPERAND_4_COMPONENT_MASK_x values
   * \param interpMode  interpolation mode
   */
   static void
   emit_input_declaration(struct svga_shader_emitter_v10 *emit,
                        VGPU10_OPCODE_TYPE opcodeType,
   VGPU10_OPERAND_TYPE operandType,
   VGPU10_OPERAND_INDEX_DIMENSION dim,
   unsigned index, unsigned size,
   VGPU10_SYSTEM_NAME name,
   VGPU10_OPERAND_NUM_COMPONENTS numComp,
   VGPU10_OPERAND_4_COMPONENT_SELECTION_MODE selMode,
      {
      VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
            assert(usageMask <= VGPU10_OPERAND_4_COMPONENT_MASK_ALL);
   assert(opcodeType == VGPU10_OPCODE_DCL_INPUT ||
         opcodeType == VGPU10_OPCODE_DCL_INPUT_SIV ||
   opcodeType == VGPU10_OPCODE_DCL_INPUT_SGV ||
   opcodeType == VGPU10_OPCODE_DCL_INPUT_PS ||
   opcodeType == VGPU10_OPCODE_DCL_INPUT_PS_SIV ||
   assert(operandType == VGPU10_OPERAND_TYPE_INPUT ||
         operandType == VGPU10_OPERAND_TYPE_INPUT_GS_INSTANCE_ID ||
   operandType == VGPU10_OPERAND_TYPE_INPUT_COVERAGE_MASK ||
   operandType == VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID ||
   operandType == VGPU10_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID ||
   operandType == VGPU10_OPERAND_TYPE_INPUT_DOMAIN_POINT ||
   operandType == VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT ||
   operandType == VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT ||
   operandType == VGPU10_OPERAND_TYPE_INPUT_THREAD_ID ||
            assert(numComp <= VGPU10_OPERAND_4_COMPONENT);
   assert(selMode <= VGPU10_OPERAND_4_COMPONENT_MASK_MODE);
   assert(dim <= VGPU10_OPERAND_INDEX_3D);
   assert(name == VGPU10_NAME_UNDEFINED ||
         name == VGPU10_NAME_POSITION ||
   name == VGPU10_NAME_INSTANCE_ID ||
   name == VGPU10_NAME_VERTEX_ID ||
   name == VGPU10_NAME_PRIMITIVE_ID ||
   name == VGPU10_NAME_IS_FRONT_FACE ||
   name == VGPU10_NAME_SAMPLE_INDEX ||
            assert(interpMode == VGPU10_INTERPOLATION_UNDEFINED ||
         interpMode == VGPU10_INTERPOLATION_CONSTANT ||
   interpMode == VGPU10_INTERPOLATION_LINEAR ||
   interpMode == VGPU10_INTERPOLATION_LINEAR_CENTROID ||
   interpMode == VGPU10_INTERPOLATION_LINEAR_NOPERSPECTIVE ||
   interpMode == VGPU10_INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID ||
                              opcode0.opcodeType = opcodeType;
            operand0.operandType = operandType;
   operand0.numComponents = numComp;
   operand0.selectionMode = selMode;
   operand0.mask = usageMask;
   operand0.indexDimension = dim;
   operand0.index0Representation = VGPU10_OPERAND_INDEX_IMMEDIATE32;
   if (dim == VGPU10_OPERAND_INDEX_2D)
                              if (addSignature) {
      struct svga_shader_signature *sgn = &emit->signature;
   if (operandType == VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT) {
      /* Set patch constant signature */
   SVGA3dDXShaderSignatureEntry *sgnEntry =
         set_shader_signature_entry(sgnEntry, index,
                     } else if (operandType == VGPU10_OPERAND_TYPE_INPUT ||
            /* Set input signature */
   SVGA3dDXShaderSignatureEntry *sgnEntry =
         set_shader_signature_entry(sgnEntry, index,
                              if (emit->index_range.required) {
      /* Here, index_range declaration is only applicable for opcodeType
   * VGPU10_OPCODE_DCL_INPUT and VGPU10_OPCODE_DCL_INPUT_PS and
   * for operandType VGPU10_OPERAND_TYPE_INPUT,
   * VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT and
   * VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT.
   */
   if ((opcodeType != VGPU10_OPCODE_DCL_INPUT &&
      opcodeType != VGPU10_OPCODE_DCL_INPUT_PS) ||
   (operandType != VGPU10_OPERAND_TYPE_INPUT &&
   operandType != VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT &&
   operandType != VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT)) {
   if (emit->index_range.start_index != INVALID_INDEX) {
         }
               if (emit->index_range.operandType == VGPU10_NUM_OPERANDS) {
      /* Need record new index_range */
   emit->index_range.count = 1;
   emit->index_range.operandType = operandType;
   emit->index_range.start_index = index;
   emit->index_range.size = size;
      }
   else if (index !=
            (emit->index_range.start_index + emit->index_range.count) ||
   /* Input index is not contiguous with index range or operandType is
   * different from index range's operandType. We need to emit current
   * index_range first and then start recording next index range.
                  emit->index_range.count = 1;
   emit->index_range.operandType = operandType;
   emit->index_range.start_index = index;
   emit->index_range.size = size;
      }
   else if (emit->index_range.operandType == operandType) {
      /* Since input index is contiguous with index range and operandType
   * is same as index range's operandType, increment index range count.
   */
            }
         /**
   * Emit the declaration for a shader output.
   * \param type  one of VGPU10_OPCODE_DCL_OUTPUTx
   * \param index  the output register index
   * \param name  one of VGPU10_NAME_x
   * \param usageMask  bitfield of VGPU10_OPERAND_4_COMPONENT_MASK_x values
   */
   static void
   emit_output_declaration(struct svga_shader_emitter_v10 *emit,
                           VGPU10_OPCODE_TYPE type, unsigned index,
   {
      VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
            assert(writemask <= VGPU10_OPERAND_4_COMPONENT_MASK_ALL);
   assert(type == VGPU10_OPCODE_DCL_OUTPUT ||
         type == VGPU10_OPCODE_DCL_OUTPUT_SGV ||
   assert(name == VGPU10_NAME_UNDEFINED ||
         name == VGPU10_NAME_POSITION ||
   name == VGPU10_NAME_PRIMITIVE_ID ||
   name == VGPU10_NAME_RENDER_TARGET_ARRAY_INDEX ||
                              opcode0.opcodeType = type;
   operand0.operandType = VGPU10_OPERAND_TYPE_OUTPUT;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_MASK_MODE;
   operand0.mask = writemask;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
                              /* Capture output signature */
   if (addSignature) {
      struct svga_shader_signature *sgn = &emit->signature;
   SVGA3dDXShaderSignatureEntry *sgnEntry =
         set_shader_signature_entry(sgnEntry, index,
                           if (emit->index_range.required) {
      /* Here, index_range declaration is only applicable for opcodeType
   * VGPU10_OPCODE_DCL_OUTPUT and for operandType
   * VGPU10_OPERAND_TYPE_OUTPUT.
   */
   if (type != VGPU10_OPCODE_DCL_OUTPUT) {
      if (emit->index_range.start_index != INVALID_INDEX) {
         }
               if (emit->index_range.operandType == VGPU10_NUM_OPERANDS) {
      /* Need record new index_range */
   emit->index_range.count = 1;
   emit->index_range.operandType = VGPU10_OPERAND_TYPE_OUTPUT;
   emit->index_range.start_index = index;
   emit->index_range.size = 1;
      }
   else if (index !=
            /* Output index is not contiguous with index range. We need to
   * emit current index_range first and then start recording next
   * index range.
                  emit->index_range.count = 1;
   emit->index_range.operandType = VGPU10_OPERAND_TYPE_OUTPUT;
   emit->index_range.start_index = index;
   emit->index_range.size = 1;
      }
   else {
      /* Since output index is contiguous with index range, increment
   * index range count.
   */
            }
         /**
   * Emit the declaration for the fragment depth output.
   */
   static void
   emit_fragdepth_output_declaration(struct svga_shader_emitter_v10 *emit)
   {
      VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
                              opcode0.opcodeType = VGPU10_OPCODE_DCL_OUTPUT;
   operand0.operandType = VGPU10_OPERAND_TYPE_OUTPUT_DEPTH;
   operand0.numComponents = VGPU10_OPERAND_1_COMPONENT;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
               }
         /**
   * Emit the declaration for the fragment sample mask/coverage output.
   */
   static void
   emit_samplemask_output_declaration(struct svga_shader_emitter_v10 *emit)
   {
      VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
            assert(emit->unit == PIPE_SHADER_FRAGMENT);
                     opcode0.opcodeType = VGPU10_OPCODE_DCL_OUTPUT;
   operand0.operandType = VGPU10_OPERAND_TYPE_OUTPUT_COVERAGE_MASK;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_0D;
               }
         /**
   * Emit output declarations for fragment shader.
   */
   static void
   emit_fs_output_declarations(struct svga_shader_emitter_v10 *emit)
   {
               for (i = 0; i < emit->info.num_outputs; i++) {
      /*const unsigned usage_mask = emit->info.output_usage_mask[i];*/
   const enum tgsi_semantic semantic_name =
         const unsigned semantic_index = emit->info.output_semantic_index[i];
            if (semantic_name == TGSI_SEMANTIC_COLOR) {
                                       /* The semantic index is the shader's color output/buffer index */
   emit_output_declaration(emit,
                                    if (semantic_index == 0) {
      if (emit->key.fs.write_color0_to_n_cbufs > 1) {
      /* Emit declarations for the additional color outputs
   * for broadcasting.
   */
   unsigned j;
   for (j = 1; j < emit->key.fs.write_color0_to_n_cbufs; j++) {
      /* Allocate a new output index */
   unsigned idx = emit->info.num_outputs + j - 1;
   emit->fs.color_out_index[j] = idx;
   emit_output_declaration(emit,
                                             emit->fs.num_color_outputs =
            }
   else if (semantic_name == TGSI_SEMANTIC_POSITION) {
      /* Fragment depth output */
      }
   else if (semantic_name == TGSI_SEMANTIC_SAMPLEMASK) {
      /* Sample mask output */
      }
   else {
               }
         /**
   * Emit common output declaration for vertex processing.
   */
   static void
   emit_vertex_output_declaration(struct svga_shader_emitter_v10 *emit,
               {
      const enum tgsi_semantic semantic_name =
         const unsigned semantic_index = emit->info.output_semantic_index[index];
   unsigned name, type;
            assert(emit->unit != PIPE_SHADER_FRAGMENT &&
            switch (semantic_name) {
   case TGSI_SEMANTIC_POSITION:
      if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      /* position will be declared in control point only */
   assert(emit->tcs.control_point_phase);
   type = VGPU10_OPCODE_DCL_OUTPUT;
   name = VGPU10_NAME_UNDEFINED;
   emit_output_declaration(emit, type, index, name, final_mask, true,
            }
   else {
      type = VGPU10_OPCODE_DCL_OUTPUT_SIV;
      }
   /* Save the index of the vertex position output register */
   emit->vposition.out_index = index;
      case TGSI_SEMANTIC_CLIPDIST:
      type = VGPU10_OPCODE_DCL_OUTPUT_SIV;
   name = VGPU10_NAME_CLIP_DISTANCE;
   /* save the starting index of the clip distance output register */
   if (semantic_index == 0)
         final_mask = apply_clip_plane_mask(emit, writemask, semantic_index);
   if (final_mask == 0x0)
            case TGSI_SEMANTIC_CLIPVERTEX:
      type = VGPU10_OPCODE_DCL_OUTPUT;
   name = VGPU10_NAME_UNDEFINED;
   emit->clip_vertex_out_index = index;
      default:
      /* generic output */
   type = VGPU10_OPCODE_DCL_OUTPUT;
               emit_output_declaration(emit, type, index, name, final_mask, addSignature,
      }
         /**
   * Emit declaration for outputs in vertex shader.
   */
   static void
   emit_vs_output_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned i;
   for (i = 0; i < emit->info.num_outputs; i++) {
            }
         /**
   * A helper function to determine the writemask for an output
   * for the specified stream.
   */
   static unsigned
   output_writemask_for_stream(unsigned stream, uint8_t output_streams,
         {
      unsigned i;
            for (i = 0; i < 4; i++) {
      if ((output_streams & 0x3) == stream)
            }
      }
         /**
   * Emit declaration for outputs in geometry shader.
   */
   static void
   emit_gs_output_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned i;
   VGPU10OpcodeToken0 opcode0;
   unsigned numStreamsSupported = 1;
            if (emit->version >= 50) {
                  /**
   * Start emitting from the last stream first, so we end with
   * stream 0, so any of the auxiliary output declarations will
   * go to stream 0.
   */
               if (emit->info.num_stream_output_components[s] == 0)
            if (emit->version >= 50) {
      /* DCL_STREAM stream */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_DCL_STREAM, false);
   emit_stream_register(emit, s);
               /* emit output primitive topology declaration */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY;
   opcode0.primitiveTopology = emit->gs.prim_topology;
            for (i = 0; i < emit->info.num_outputs; i++) {
               /* find out the writemask for this stream */
                  if (writemask) {
                     /* TODO: Still need to take care of a special case where a
   *       single varying spans across multiple output registers.
   */
   switch(semantic_name) {
   case TGSI_SEMANTIC_PRIMID:
      emit_output_declaration(emit,
                           VGPU10_OPCODE_DCL_OUTPUT_SGV, i,
      case TGSI_SEMANTIC_LAYER:
      emit_output_declaration(emit,
                           VGPU10_OPCODE_DCL_OUTPUT_SIV, i,
      case TGSI_SEMANTIC_VIEWPORT_INDEX:
      emit_output_declaration(emit,
                           VGPU10_OPCODE_DCL_OUTPUT_SIV, i,
   emit->gs.viewport_index_out_index = i;
      default:
                           /* For geometry shader outputs, it is possible the same register is
   * declared multiple times for different streams. So to avoid
   * redundant signature entries, geometry shader output signature is done
   * outside of the declaration.
   */
   struct svga_shader_signature *sgn = &emit->signature;
            for (i = 0; i < emit->info.num_outputs; i++) {
      if (emit->output_usage_mask[i]) {
               sgnEntry = &sgn->outputs[sgn->header.numOutputSignatures++];
   set_shader_signature_entry(sgnEntry, i,
                              }
         /**
   * Emit the declaration for the tess inner/outer output.
   * \param opcodeType either VGPU10_OPCODE_DCL_OUTPUT_SIV or _INPUT_SIV
   * \param operandType either VGPU10_OPERAND_TYPE_OUTPUT or _INPUT
   * \param name VGPU10_NAME_FINAL_*_TESSFACTOR value
   */
   static void
   emit_tesslevel_declaration(struct svga_shader_emitter_v10 *emit,
                     {
      VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
            assert(emit->version >= 50);
   assert(name >= VGPU10_NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR ||
         (emit->key.tcs.prim_mode == MESA_PRIM_LINES &&
            assert(operandType == VGPU10_OPERAND_TYPE_OUTPUT ||
                     opcode0.opcodeType = opcodeType;
   operand0.operandType = operandType;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
   operand0.mask = VGPU10_OPERAND_4_COMPONENT_MASK_X;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_MASK_MODE;
            name_token.name = name;
            /* Capture patch constant signature */
   struct svga_shader_signature *sgn = &emit->signature;
   SVGA3dDXShaderSignatureEntry *sgnEntry =
         set_shader_signature_entry(sgnEntry, index,
                  }
         /**
   * Emit output declarations for tessellation control shader.
   */
   static void
   emit_tcs_output_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned int i;
   unsigned outputIndex = emit->num_outputs;
            /**
   * Initialize patch_generic_out_count so it won't be counted twice
   * since this function is called twice, one for control point phase
   * and another time for patch constant phase.
   */
            for (i = 0; i < emit->info.num_outputs; i++) {
      unsigned index = i;
   const enum tgsi_semantic semantic_name =
            switch (semantic_name) {
   case TGSI_SEMANTIC_TESSINNER:
               /* skip per-patch output declarations in control point phase */
                  emit->tcs.inner.out_index = outputIndex;
   switch (emit->key.tcs.prim_mode) {
   case MESA_PRIM_QUADS:
      emit_tesslevel_declaration(emit, outputIndex++,
                        emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR,
         case MESA_PRIM_TRIANGLES:
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_TRI_INSIDE_TESSFACTOR,
         case MESA_PRIM_LINES:
         default:
                     case TGSI_SEMANTIC_TESSOUTER:
               /* skip per-patch output declarations in control point phase */
                  emit->tcs.outer.out_index = outputIndex;
   switch (emit->key.tcs.prim_mode) {
   case MESA_PRIM_QUADS:
      for (int j = 0; j < 4; j++) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR + j,
   }
      case MESA_PRIM_TRIANGLES:
      for (int j = 0; j < 3; j++) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR + j,
   }
      case MESA_PRIM_LINES:
      for (int j = 0; j < 2; j++) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_LINE_DETAIL_TESSFACTOR + j,
   }
      default:
                     case TGSI_SEMANTIC_PATCH:
      if (emit->tcs.patch_generic_out_index == INVALID_INDEX)
                  /* skip per-patch output declarations in control point phase */
                  emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT, index,
                              SVGA3dDXShaderSignatureEntry *sgnEntry =
         set_shader_signature_entry(sgnEntry, index,
                                    default:
      /* save the starting index of control point outputs */
   if (emit->tcs.control_point_out_index == INVALID_INDEX)
                  /* skip control point output declarations in patch constant phase */
                                          if (emit->tcs.control_point_phase) {
      /**
   * Add missing control point output in control point phase.
   */
   if (emit->tcs.control_point_out_index == INVALID_INDEX) {
      /* use register index after tessellation factors */
   switch (emit->key.tcs.prim_mode) {
   case MESA_PRIM_QUADS:
      emit->tcs.control_point_out_index = outputIndex + 6;
      case MESA_PRIM_TRIANGLES:
      emit->tcs.control_point_out_index = outputIndex + 4;
      default:
      emit->tcs.control_point_out_index = outputIndex + 2;
      }
   emit->tcs.control_point_out_count++;
   emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT_SIV,
                                    /* If tcs does not output any control point output,
   * we can end the hull shader control point phase here
   * after emitting the default control point output.
   */
         }
   else {
      if (emit->tcs.outer.out_index == INVALID_INDEX) {
      /* since the TCS did not declare out outer tess level output register,
   * we declare it here for patch constant phase only.
   */
   emit->tcs.outer.out_index = outputIndex;
   if (emit->key.tcs.prim_mode == MESA_PRIM_QUADS) {
      for (int i = 0; i < 4; i++) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR + i,
      }
   else if (emit->key.tcs.prim_mode == MESA_PRIM_TRIANGLES) {
      for (int i = 0; i < 3; i++) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR + i,
                  if (emit->tcs.inner.out_index == INVALID_INDEX) {
      /* since the TCS did not declare out inner tess level output register,
   * we declare it here
   */
   emit->tcs.inner.out_index = outputIndex;
   if (emit->key.tcs.prim_mode == MESA_PRIM_QUADS) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR,
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR,
   }
   else if (emit->key.tcs.prim_mode == MESA_PRIM_TRIANGLES) {
      emit_tesslevel_declaration(emit, outputIndex++,
      VGPU10_OPCODE_DCL_OUTPUT_SIV, VGPU10_OPERAND_TYPE_OUTPUT,
   VGPU10_NAME_FINAL_TRI_INSIDE_TESSFACTOR,
         }
      }
         /**
   * Emit output declarations for tessellation evaluation shader.
   */
   static void
   emit_tes_output_declarations(struct svga_shader_emitter_v10 *emit)
   {
               for (i = 0; i < emit->info.num_outputs; i++) {
            }
         /**
   * Emit the declaration for a system value input/output.
   */
   static void
   emit_system_value_declaration(struct svga_shader_emitter_v10 *emit,
         {
      switch (semantic_name) {
   case TGSI_SEMANTIC_INSTANCEID:
      index = alloc_system_value_index(emit, index);
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT_SIV,
                        VGPU10_OPERAND_TYPE_INPUT,
   VGPU10_OPERAND_INDEX_1D,
   index, 1,
   VGPU10_NAME_INSTANCE_ID,
   VGPU10_OPERAND_4_COMPONENT,
         case TGSI_SEMANTIC_VERTEXID:
      emit->vs.vertex_id_sys_index = index;
   index = alloc_system_value_index(emit, index);
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT_SIV,
                        VGPU10_OPERAND_TYPE_INPUT,
   VGPU10_OPERAND_INDEX_1D,
   index, 1,
   VGPU10_NAME_VERTEX_ID,
   VGPU10_OPERAND_4_COMPONENT,
         case TGSI_SEMANTIC_SAMPLEID:
      assert(emit->unit == PIPE_SHADER_FRAGMENT);
   emit->fs.sample_id_sys_index = index;
   index = alloc_system_value_index(emit, index);
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT_PS_SIV,
                        VGPU10_OPERAND_TYPE_INPUT,
   VGPU10_OPERAND_INDEX_1D,
   index, 1,
   VGPU10_NAME_SAMPLE_INDEX,
   VGPU10_OPERAND_4_COMPONENT,
         case TGSI_SEMANTIC_SAMPLEPOS:
      /* This system value contains the position of the current sample
   * when using per-sample shading.  We implement this by calling
   * the VGPU10_OPCODE_SAMPLE_POS instruction with the current sample
   * index as the argument.  See emit_sample_position_instructions().
   */
   assert(emit->version >= 41);
   emit->fs.sample_pos_sys_index = index;
   index = alloc_system_value_index(emit, index);
      case TGSI_SEMANTIC_INVOCATIONID:
      /* Note: invocation id input is mapped to different register depending
   * on the shader type. In GS, it will be mapped to vGSInstanceID#.
   * In TCS, it will be mapped to vOutputControlPointID#.
   * Since in both cases, the mapped name is unique rather than
   * just a generic input name ("v#"), so there is no need to remap
   * the index value.
   */
   assert(emit->unit == PIPE_SHADER_GEOMETRY ||
                  if (emit->unit == PIPE_SHADER_GEOMETRY) {
      emit->gs.invocation_id_sys_index = index;
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_GS_INSTANCE_ID,
   VGPU10_OPERAND_INDEX_0D,
   index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_0_COMPONENT,
   } else if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      /* The emission of the control point id will be done
   * in the control point phase in emit_hull_shader_control_point_phase().
   */
      }
      case TGSI_SEMANTIC_SAMPLEMASK:
      /* Note: the PS sample mask input has a unique name ("vCoverage#")
   * rather than just a generic input name ("v#") so no need to remap the
   * index value.
   */
   assert(emit->unit == PIPE_SHADER_FRAGMENT);
   assert(emit->version >= 50);
   emit->fs.sample_mask_in_sys_index = index;
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_COVERAGE_MASK,
   VGPU10_OPERAND_INDEX_0D,
   index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_1_COMPONENT,
         case TGSI_SEMANTIC_TESSCOORD:
                        if (emit->tes.prim_mode == MESA_PRIM_TRIANGLES) {
         }
   else if (emit->tes.prim_mode == MESA_PRIM_LINES ||
                        emit->tes.tesscoord_sys_index = index;
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_DOMAIN_POINT,
   VGPU10_OPERAND_INDEX_0D,
   index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_4_COMPONENT,
         case TGSI_SEMANTIC_TESSINNER:
      assert(emit->version >= 50);
   emit->tes.inner.tgsi_index = index;
      case TGSI_SEMANTIC_TESSOUTER:
      assert(emit->version >= 50);
   emit->tes.outer.tgsi_index = index;
      case TGSI_SEMANTIC_VERTICESIN:
      assert(emit->unit == PIPE_SHADER_TESS_CTRL);
            /* save the system value index */
   emit->tcs.vertices_per_patch_index = index;
      case TGSI_SEMANTIC_PRIMID:
      assert(emit->version >= 50);
   if (emit->unit == PIPE_SHADER_TESS_CTRL) {
         }
   else if (emit->unit == PIPE_SHADER_TESS_EVAL) {
      emit->tes.prim_id_index = index;
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID,
   VGPU10_OPERAND_INDEX_0D,
   index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_0_COMPONENT,
   }
      case TGSI_SEMANTIC_THREAD_ID:
      assert(emit->unit >= PIPE_SHADER_COMPUTE);
   assert(emit->version >= 50);
   emit->cs.thread_id_index = index;
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP,
   VGPU10_OPERAND_INDEX_0D,
   index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_4_COMPONENT,
         case TGSI_SEMANTIC_BLOCK_ID:
      assert(emit->unit >= PIPE_SHADER_COMPUTE);
   assert(emit->version >= 50);
   emit->cs.block_id_index = index;
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT_THREAD_GROUP_ID,
   VGPU10_OPERAND_INDEX_0D,
   index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_4_COMPONENT,
         case TGSI_SEMANTIC_GRID_SIZE:
      assert(emit->unit == PIPE_SHADER_COMPUTE);
   assert(emit->version >= 50);
   emit->cs.grid_size.tgsi_index = index;
      default:
      debug_printf("unexpected system value semantic index %u / %s\n",
         }
      /**
   * Translate a TGSI declaration to VGPU10.
   */
   static bool
   emit_vgpu10_declaration(struct svga_shader_emitter_v10 *emit,
         {
      switch (decl->Declaration.File) {
   case TGSI_FILE_INPUT:
      /* do nothing - see emit_input_declarations() */
         case TGSI_FILE_OUTPUT:
      assert(decl->Range.First == decl->Range.Last);
   emit->output_usage_mask[decl->Range.First] = decl->Declaration.UsageMask;
         case TGSI_FILE_TEMPORARY:
      /* Don't declare the temps here.  Just keep track of how many
   * and emit the declaration later.
   */
   if (decl->Declaration.Array) {
      /* Indexed temporary array.  Save the start index of the array
   * and the size of the array.
   */
                  /* Save this array so we can emit the declaration for it later */
   create_temp_array(emit, arrayID, decl->Range.First,
                     /* for all temps, indexed or not, keep track of highest index */
   emit->num_shader_temps = MAX2(emit->num_shader_temps,
               case TGSI_FILE_CONSTANT:
      /* Don't declare constants here.  Just keep track and emit later. */
   {
      unsigned constbuf = 0, num_consts;
   if (decl->Declaration.Dimension) {
         }
   /* We throw an assertion here when, in fact, the shader should never
   * have linked due to constbuf index out of bounds, so we shouldn't
   * have reached here.
                                 if (num_consts > VGPU10_MAX_CONSTANT_BUFFER_ELEMENT_COUNT) {
      debug_printf("Warning: constant buffer is declared to size [%u]"
               " but [%u] is the limit.\n",
      }
   /* The linker doesn't enforce the max UBO size so we clamp here */
   emit->num_shader_consts[constbuf] =
      }
         case TGSI_FILE_IMMEDIATE:
      assert(!"TGSI_FILE_IMMEDIATE not handled yet!");
         case TGSI_FILE_SYSTEM_VALUE:
      emit_system_value_declaration(emit, decl->Semantic.Name,
               case TGSI_FILE_SAMPLER:
      /* Don't declare samplers here.  Just keep track and emit later. */
   emit->num_samplers = MAX2(emit->num_samplers, decl->Range.Last + 1);
      #if 0
      case TGSI_FILE_RESOURCE:
      /*opcode0.opcodeType = VGPU10_OPCODE_DCL_RESOURCE;*/
   /* XXX more, VGPU10_RETURN_TYPE_FLOAT */
   assert(!"TGSI_FILE_RESOURCE not handled yet");
   #endif
         case TGSI_FILE_ADDRESS:
      emit->num_address_regs = MAX2(emit->num_address_regs,
               case TGSI_FILE_SAMPLER_VIEW:
      {
      unsigned unit = decl->Range.First;
                  /* Note: we can ignore YZW return types for now */
   emit->sampler_return_type[unit] = decl->SamplerView.ReturnTypeX;
      }
         case TGSI_FILE_IMAGE:
      {
      unsigned unit = decl->Range.First;
   assert(decl->Range.First == decl->Range.Last);
   assert(unit < PIPE_MAX_SHADER_IMAGES);
   emit->image[unit] = decl->Image;
   emit->image_mask |= 1 << unit;
      }
         case TGSI_FILE_HW_ATOMIC:
      /* Declare the atomic buffer if it is not already declared. */
   if (!(emit->atomic_bufs_mask & (1 << decl->Dim.Index2D))) {
      emit->num_atomic_bufs++;
               /* Remember the maximum atomic counter index encountered */
   emit->max_atomic_counter_index =
               case TGSI_FILE_MEMORY:
      /* Record memory has been used. */
   if (emit->unit == PIPE_SHADER_COMPUTE &&
      decl->Declaration.MemType == TGSI_MEMORY_TYPE_SHARED) {
                     case TGSI_FILE_BUFFER:
      assert(emit->version >= 50);
   emit->num_shader_bufs++;
         default:
      assert(!"Unexpected type of declaration");
         }
         /**
   * Emit input declarations for fragment shader.
   */
   static void
   emit_fs_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
               for (i = 0; i < emit->linkage.num_inputs; i++) {
      enum tgsi_semantic semantic_name = emit->info.input_semantic_name[i];
   unsigned usage_mask = emit->info.input_usage_mask[i];
   unsigned index = emit->linkage.input_map[i];
   unsigned type, interpolationMode, name;
            if (usage_mask == 0)
            if (semantic_name == TGSI_SEMANTIC_POSITION) {
      /* fragment position input */
   type = VGPU10_OPCODE_DCL_INPUT_PS_SGV;
   interpolationMode = VGPU10_INTERPOLATION_LINEAR;
   name = VGPU10_NAME_POSITION;
   if (usage_mask & TGSI_WRITEMASK_W) {
      /* we need to replace use of 'w' with '1/w' */
         }
   else if (semantic_name == TGSI_SEMANTIC_FACE) {
      /* fragment front-facing input */
   type = VGPU10_OPCODE_DCL_INPUT_PS_SGV;
   interpolationMode = VGPU10_INTERPOLATION_CONSTANT;
   name = VGPU10_NAME_IS_FRONT_FACE;
      }
   else if (semantic_name == TGSI_SEMANTIC_PRIMID) {
      /* primitive ID */
   type = VGPU10_OPCODE_DCL_INPUT_PS_SGV;
   interpolationMode = VGPU10_INTERPOLATION_CONSTANT;
      }
   else if (semantic_name == TGSI_SEMANTIC_SAMPLEID) {
      /* sample index / ID */
   type = VGPU10_OPCODE_DCL_INPUT_PS_SGV;
   interpolationMode = VGPU10_INTERPOLATION_CONSTANT;
      }
   else if (semantic_name == TGSI_SEMANTIC_LAYER) {
      /* render target array index */
   if (emit->key.fs.layer_to_zero) {
      /**
   * The shader from the previous stage does not write to layer,
   * so reading the layer index in fragment shader should return 0.
   */
   emit->fs.layer_input_index = i;
      } else {
      type = VGPU10_OPCODE_DCL_INPUT_PS_SGV;
   interpolationMode = VGPU10_INTERPOLATION_CONSTANT;
   name = VGPU10_NAME_RENDER_TARGET_ARRAY_INDEX;
         }
   else if (semantic_name == TGSI_SEMANTIC_VIEWPORT_INDEX) {
      /* viewport index */
   type = VGPU10_OPCODE_DCL_INPUT_PS_SGV;
   interpolationMode = VGPU10_INTERPOLATION_CONSTANT;
   name = VGPU10_NAME_VIEWPORT_ARRAY_INDEX;
      }
   else {
      /* general fragment input */
   type = VGPU10_OPCODE_DCL_INPUT_PS;
   interpolationMode =
                        /* keeps track if flat interpolation mode is being used */
                              emit_input_declaration(emit, type,
                        VGPU10_OPERAND_TYPE_INPUT,
   VGPU10_OPERAND_INDEX_1D, index, 1,
   name,
   VGPU10_OPERAND_4_COMPONENT,
      }
         /**
   * Emit input declarations for vertex shader.
   */
   static void
   emit_vs_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
               for (i = 0; i < emit->info.file_max[TGSI_FILE_INPUT] + 1; i++) {
      unsigned usage_mask = emit->info.input_usage_mask[i];
            if (usage_mask == 0)
            emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        VGPU10_OPERAND_TYPE_INPUT,
   VGPU10_OPERAND_INDEX_1D, index, 1,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_4_COMPONENT,
      }
         /**
   * Emit input declarations for geometry shader.
   */
   static void
   emit_gs_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
               for (i = 0; i < emit->info.num_inputs; i++) {
      enum tgsi_semantic semantic_name = emit->info.input_semantic_name[i];
   unsigned usage_mask = emit->info.input_usage_mask[i];
   unsigned index = emit->linkage.input_map[i];
   unsigned opcodeType, operandType;
   unsigned numComp, selMode;
   unsigned name;
            if (usage_mask == 0)
            opcodeType = VGPU10_OPCODE_DCL_INPUT;
   operandType = VGPU10_OPERAND_TYPE_INPUT;
   numComp = VGPU10_OPERAND_4_COMPONENT;
   selMode = VGPU10_OPERAND_4_COMPONENT_MASK_MODE;
            /* all geometry shader inputs are two dimensional except
   * gl_PrimitiveID
   */
            if (semantic_name == TGSI_SEMANTIC_PRIMID) {
      /* Primitive ID */
   operandType = VGPU10_OPERAND_TYPE_INPUT_PRIMITIVEID;
   dim = VGPU10_OPERAND_INDEX_0D;
                  /* also save the register index so we can check for
   * primitive id when emit src register. We need to modify the
   * operand type, index dimension when emit primitive id src reg.
   */
      }
   else if (semantic_name == TGSI_SEMANTIC_POSITION) {
      /* vertex position input */
   opcodeType = VGPU10_OPCODE_DCL_INPUT_SIV;
               emit_input_declaration(emit, opcodeType, operandType,
                        dim, index,
   emit->gs.input_size,
   name,
      }
         /**
   * Emit input declarations for tessellation control shader.
   */
   static void
   emit_tcs_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned i;
   unsigned size = emit->key.tcs.vertices_per_patch;
            if (!emit->tcs.control_point_phase)
            for (i = 0; i < emit->info.num_inputs; i++) {
      unsigned usage_mask = emit->info.input_usage_mask[i];
   unsigned index = emit->linkage.input_map[i];
   enum tgsi_semantic semantic_name = emit->info.input_semantic_name[i];
   VGPU10_SYSTEM_NAME name = VGPU10_NAME_UNDEFINED;
   VGPU10_OPERAND_TYPE operandType = VGPU10_OPERAND_TYPE_INPUT;
   SVGA3dDXSignatureSemanticName sgn_name =
            if (semantic_name == TGSI_SEMANTIC_POSITION ||
      index == emit->linkage.position_index) {
   /* save the input control point index for later use */
      }
   else if (usage_mask == 0) {
         }
   else if (semantic_name == TGSI_SEMANTIC_CLIPDIST) {
      /* The shadow copy is being used here. So set the signature name
   * to UNDEFINED.
   */
               /* input control points in the patch constant phase are emitted in the
   * vicp register rather than the v register.
   */
   if (!emit->tcs.control_point_phase) {
                  /* Tessellation control shader inputs are two dimensional.
   * The array size is determined by the patch vertex count.
   */
   emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                        operandType,
   VGPU10_OPERAND_INDEX_2D,
   index, size, name,
   VGPU10_OPERAND_4_COMPONENT,
                        /* Also add an address register for the indirection to the
   * input control points
   */
         }
         static void
   emit_tessfactor_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
         /* In tcs, tess factors are emitted as extra outputs.
   * The starting register index for the tess factors is captured
   * in the compile key.
   */
            if (emit->tes.prim_mode == MESA_PRIM_QUADS) {
      if (emit->key.tes.need_tessouter) {
      emit->tes.outer.in_index = inputIndex;
   for (int i = 0; i < 4; i++) {
      emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
   VGPU10_NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR + i,
               if (emit->key.tes.need_tessinner) {
      emit->tes.inner.in_index = inputIndex;
   emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
               emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
   VGPU10_NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR,
      }
   else if (emit->tes.prim_mode == MESA_PRIM_TRIANGLES) {
      if (emit->key.tes.need_tessouter) {
      emit->tes.outer.in_index = inputIndex;
   for (int i = 0; i < 3; i++) {
      emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
   VGPU10_NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR + i,
               if (emit->key.tes.need_tessinner) {
      emit->tes.inner.in_index = inputIndex;
   emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
   VGPU10_NAME_FINAL_TRI_INSIDE_TESSFACTOR,
      }
   else if (emit->tes.prim_mode == MESA_PRIM_LINES) {
      if (emit->key.tes.need_tessouter) {
      emit->tes.outer.in_index = inputIndex;
   emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
               emit_tesslevel_declaration(emit, inputIndex++,
      VGPU10_OPCODE_DCL_INPUT_SIV,
   VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
   VGPU10_NAME_FINAL_LINE_DENSITY_TESSFACTOR,
         }
         /**
   * Emit input declarations for tessellation evaluation shader.
   */
   static void
   emit_tes_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
               for (i = 0; i < emit->info.num_inputs; i++) {
      unsigned usage_mask = emit->info.input_usage_mask[i];
   unsigned index = emit->linkage.input_map[i];
   unsigned size;
   const enum tgsi_semantic semantic_name =
         SVGA3dDXSignatureSemanticName sgn_name;
   VGPU10_OPERAND_TYPE operandType;
            if (usage_mask == 0)
            if (semantic_name == TGSI_SEMANTIC_PATCH) {
      operandType = VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT;
   dim = VGPU10_OPERAND_INDEX_1D;
   size = 1;
      }
   else {
      operandType = VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT;
   dim = VGPU10_OPERAND_INDEX_2D;
   size = emit->key.tes.vertices_per_patch;
               emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT, operandType,
                        dim, index, size, VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_4_COMPONENT,
                     /* DX spec requires DS input controlpoint/patch-constant signatures to match
   * the HS output controlpoint/patch-constant signatures exactly.
   * Add missing input declarations even if they are not used in the shader.
   */
   if (emit->linkage.num_inputs < emit->linkage.prevShader.num_outputs) {
      struct tgsi_shader_info *prevInfo = emit->prevShaderInfo;
               /* If a tcs output does not have a corresponding input register in
   * tes, add one.
   */
   if (emit->linkage.prevShader.output_map[i] >
                     if (sem_name == TGSI_SEMANTIC_PATCH) {
      emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                           VGPU10_OPERAND_TYPE_INPUT_PATCH_CONSTANT,
   VGPU10_OPERAND_INDEX_1D,
                     } else if (sem_name != TGSI_SEMANTIC_TESSINNER &&
            emit_input_declaration(emit, VGPU10_OPCODE_DCL_INPUT,
                           VGPU10_OPERAND_TYPE_INPUT_CONTROL_POINT,
   VGPU10_OPERAND_INDEX_2D,
   i, emit->key.tes.vertices_per_patch,
   VGPU10_NAME_UNDEFINED,
   VGPU10_OPERAND_4_COMPONENT,
      }
   /* tessellation factors are taken care of in
   * emit_tessfactor_input_declarations().
               }
         /**
   * Emit all input declarations.
   */
   static bool
   emit_input_declarations(struct svga_shader_emitter_v10 *emit)
   {
      emit->index_range.required =
            switch (emit->unit) {
   case PIPE_SHADER_FRAGMENT:
      emit_fs_input_declarations(emit);
      case PIPE_SHADER_GEOMETRY:
      emit_gs_input_declarations(emit);
      case PIPE_SHADER_VERTEX:
      emit_vs_input_declarations(emit);
      case PIPE_SHADER_TESS_CTRL:
      emit_tcs_input_declarations(emit);
      case PIPE_SHADER_TESS_EVAL:
      emit_tes_input_declarations(emit);
      case PIPE_SHADER_COMPUTE:
      //XXX emit_cs_input_declarations(emit);
      default:
                  if (emit->index_range.start_index != INVALID_INDEX) {
         }
   emit->index_range.required = false;
      }
         /**
   * Emit all output declarations.
   */
   static bool
   emit_output_declarations(struct svga_shader_emitter_v10 *emit)
   {
      emit->index_range.required =
            switch (emit->unit) {
   case PIPE_SHADER_FRAGMENT:
      emit_fs_output_declarations(emit);
      case PIPE_SHADER_GEOMETRY:
      emit_gs_output_declarations(emit);
      case PIPE_SHADER_VERTEX:
      emit_vs_output_declarations(emit);
      case PIPE_SHADER_TESS_CTRL:
      emit_tcs_output_declarations(emit);
      case PIPE_SHADER_TESS_EVAL:
      emit_tes_output_declarations(emit);
      case PIPE_SHADER_COMPUTE:
      //XXX emit_cs_output_declarations(emit);
      default:
                  if (emit->vposition.so_index != INVALID_INDEX &&
                        /* Emit the declaration for the non-adjusted vertex position
   * for stream output purpose
   */
   emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT,
                                       if (emit->clip_dist_so_index != INVALID_INDEX &&
                        /* Emit the declaration for the clip distance shadow copy which
   * will be used for stream output purpose and for clip distance
   * varying variable. Note all clip distances
   * will be written regardless of the enabled clipping planes.
   */
   emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT,
                                    if (emit->info.num_written_clipdistance > 4) {
      /* for the second clip distance register, each handles 4 planes */
   emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT,
                                          if (emit->index_range.start_index != INVALID_INDEX) {
         }
   emit->index_range.required = false;
      }
         /**
   * A helper function to create a temporary indexable array
   * and initialize the corresponding entries in the temp_map array.
   */
   static void
   create_temp_array(struct svga_shader_emitter_v10 *emit,
               {
               emit->num_temp_arrays = MAX2(emit->num_temp_arrays, arrayID + 1);
   assert(emit->num_temp_arrays <= MAX_TEMP_ARRAYS);
            emit->temp_arrays[arrayID].start = first;
            /* Fill in the temp_map entries for this temp array */
   for (i = 0; i < count; i++, tempIndex++) {
      emit->temp_map[tempIndex].arrayId = arrayID;
         }
         /**
   * Emit the declaration for the temporary registers.
   */
   static bool
   emit_temporaries_declaration(struct svga_shader_emitter_v10 *emit)
   {
                        /* If there is indirect access to non-indexable temps in the shader,
   * convert those temps to indexable temps. This works around a bug
   * in the GLSL->TGSI translator exposed in piglit test
   * glsl-1.20/execution/fs-const-array-of-struct-of-array.shader_test.
   * Internal temps added by the driver remain as non-indexable temps.
   */
   if ((emit->info.indirect_files & (1 << TGSI_FILE_TEMPORARY)) &&
      emit->num_temp_arrays == 0) {
               /* Allocate extra temps for specially-implemented instructions,
   * such as LIT.
   */
            /* Allocate extra temps for clip distance or clip vertex.
   */
   if (emit->clip_mode == CLIP_DISTANCE) {
      /* We need to write the clip distance to a temporary register
   * first. Then it will be copied to the shadow copy for
   * the clip distance varying variable and stream output purpose.
   * It will also be copied to the actual CLIPDIST register
   * according to the enabled clip planes
   */
   emit->clip_dist_tmp_index = total_temps++;
   if (emit->info.num_written_clipdistance > 4)
      }
   else if (emit->clip_mode == CLIP_VERTEX && emit->key.last_vertex_stage) {
      /* If the current shader is in the last vertex processing stage,
   * We need to convert the TGSI CLIPVERTEX output to one or more
   * clip distances.  Allocate a temp reg for the clipvertex here.
   */
   assert(emit->info.writes_clipvertex > 0);
   emit->clip_vertex_tmp_index = total_temps;
               if (emit->info.uses_vertexid) {
      assert(emit->unit == PIPE_SHADER_VERTEX);
               if (emit->unit == PIPE_SHADER_VERTEX || emit->unit == PIPE_SHADER_GEOMETRY) {
      if (emit->vposition.need_prescale || emit->key.vs.undo_viewport ||
      emit->key.clip_plane_enable ||
   emit->vposition.so_index != INVALID_INDEX) {
   emit->vposition.tmp_index = total_temps;
               if (emit->vposition.need_prescale) {
      emit->vposition.prescale_scale_index = total_temps++;
               if (emit->unit == PIPE_SHADER_VERTEX) {
      unsigned attrib_mask = (emit->key.vs.adjust_attrib_w_1 |
                           emit->key.vs.adjust_attrib_itof |
   emit->key.vs.adjust_attrib_utof |
   while (attrib_mask) {
      unsigned index = u_bit_scan(&attrib_mask);
         }
   else if (emit->unit == PIPE_SHADER_GEOMETRY) {
      if (emit->key.gs.writes_viewport_index)
         }
   else if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (emit->key.fs.alpha_func != SVGA3D_CMP_ALWAYS ||
      emit->key.fs.write_color0_to_n_cbufs > 1) {
   /* Allocate a temp to hold the output color */
   emit->fs.color_tmp_index = total_temps;
               if (emit->fs.face_input_index != INVALID_INDEX) {
      /* Allocate a temp for the +/-1 face register */
   emit->fs.face_tmp_index = total_temps;
               if (emit->fs.fragcoord_input_index != INVALID_INDEX) {
      /* Allocate a temp for modified fragment position register */
   emit->fs.fragcoord_tmp_index = total_temps;
               if (emit->fs.sample_pos_sys_index != INVALID_INDEX) {
      /* Allocate a temp for the sample position */
         }
   else if (emit->unit == PIPE_SHADER_TESS_EVAL) {
      if (emit->vposition.need_prescale) {
      emit->vposition.tmp_index = total_temps++;
   emit->vposition.prescale_scale_index = total_temps++;
               if (emit->tes.inner.tgsi_index) {
      emit->tes.inner.temp_index = total_temps;
               if (emit->tes.outer.tgsi_index) {
      emit->tes.outer.temp_index = total_temps;
         }
   else if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      if (emit->tcs.inner.tgsi_index != INVALID_INDEX) {
      if (!emit->tcs.control_point_phase) {
      emit->tcs.inner.temp_index = total_temps;
         }
   if (emit->tcs.outer.tgsi_index != INVALID_INDEX) {
      if (!emit->tcs.control_point_phase) {
      emit->tcs.outer.temp_index = total_temps;
                  if (emit->tcs.control_point_phase &&
      emit->info.reads_pervertex_outputs) {
   emit->tcs.control_point_tmp_index = total_temps;
      }
   else if (!emit->tcs.control_point_phase &&
               /* If there is indirect access to the patch constant outputs
   * in the control point phase, then an indexable temporary array
   * will be created for these patch constant outputs.
   * Note, indirect access can only be applicable to
   * patch constant outputs in the control point phase.
   */
   if (emit->info.indirect_files & (1 << TGSI_FILE_OUTPUT)) {
      unsigned arrayID =
         create_temp_array(emit, arrayID, 0,
      }
   emit->tcs.patch_generic_tmp_index = total_temps;
                           if (emit->raw_bufs) {
      /**
   * Add 3 more temporaries if we need to translate constant buffer
   * to srv raw buffer. Since we need to load the value to a temporary
   * before it can be used as a source. There could be three source
   * register in an instruction.
   */
   emit->raw_buf_tmp_index = total_temps;
               for (i = 0; i < emit->num_address_regs; i++) {
                  /* Initialize the temp_map array which maps TGSI temp indexes to VGPU10
   * temp indexes.  Basically, we compact all the non-array temp register
   * indexes into a consecutive series.
   *
   * Before, we may have some TGSI declarations like:
   *   DCL TEMP[0..1], LOCAL
   *   DCL TEMP[2..4], ARRAY(1), LOCAL
   *   DCL TEMP[5..7], ARRAY(2), LOCAL
   *   plus, some extra temps, like TEMP[8], TEMP[9] for misc things
   *
   * After, we'll have a map like this:
   *   temp_map[0] = { array 0, index 0 }
   *   temp_map[1] = { array 0, index 1 }
   *   temp_map[2] = { array 1, index 0 }
   *   temp_map[3] = { array 1, index 1 }
   *   temp_map[4] = { array 1, index 2 }
   *   temp_map[5] = { array 2, index 0 }
   *   temp_map[6] = { array 2, index 1 }
   *   temp_map[7] = { array 2, index 2 }
   *   temp_map[8] = { array 0, index 2 }
   *   temp_map[9] = { array 0, index 3 }
   *
   * We'll declare two arrays of 3 elements, plus a set of four non-indexed
   * temps numbered 0..3
   *
   * Any time we emit a temporary register index, we'll have to use the
   * temp_map[] table to convert the TGSI index to the VGPU10 index.
   *
   * Finally, we recompute the total_temps value here.
   */
   reg = 0;
   for (i = 0; i < total_temps; i++) {
      if (emit->temp_map[i].arrayId == 0) {
                     if (0) {
      debug_printf("total_temps %u\n", total_temps);
   for (i = 0; i < total_temps; i++) {
      debug_printf("temp %u ->  array %u  index %u\n",
                           /* Emit declaration of ordinary temp registers */
   if (total_temps > 0) {
               opcode0.value = 0;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, total_temps);
               /* Emit declarations for indexable temp arrays.  Skip 0th entry since
   * it's unused.
   */
   for (i = 1; i < emit->num_temp_arrays; i++) {
               if (num_temps > 0) {
                              begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, i); /* which array */
   emit_dword(emit, num_temps);
                                 /* Check that the grand total of all regular and indexed temps is
   * under the limit.
   */
               }
         static bool
   emit_rawbuf_declaration(struct svga_shader_emitter_v10 *emit,
         {
      VGPU10OpcodeToken0 opcode1;
            opcode1.value = 0;
   opcode1.opcodeType = VGPU10_OPCODE_DCL_RESOURCE_RAW;
            operand1.value = 0;
   operand1.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand1.operandType = VGPU10_OPERAND_TYPE_RESOURCE;
   operand1.indexDimension = VGPU10_OPERAND_INDEX_1D;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode1.value);
   emit_dword(emit, operand1.value);
   emit_dword(emit, index);
               }
         static bool
   emit_constant_declaration(struct svga_shader_emitter_v10 *emit)
   {
      VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_CONSTANT_BUFFER;
   opcode0.accessPattern = VGPU10_CB_IMMEDIATE_INDEXED;
            operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_4_COMPONENT;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_2D;
   operand0.index0Representation = VGPU10_OPERAND_INDEX_IMMEDIATE32;
   operand0.index1Representation = VGPU10_OPERAND_INDEX_IMMEDIATE32;
   operand0.operandType = VGPU10_OPERAND_TYPE_CONSTANT_BUFFER;
   operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SWIZZLE_MODE;
   operand0.swizzleX = 0;
   operand0.swizzleY = 1;
   operand0.swizzleZ = 2;
            /**
   * Emit declaration for constant buffer [0].  We also allocate
   * room for the extra constants here.
   */
            /* Now, allocate constant slots for the "extra" constants.
   * Note: it's critical that these extra constant locations
   * exactly match what's emitted by the "extra" constants code
   * in svga_state_constants.c
            /* Vertex position scale/translation */
   if (emit->vposition.need_prescale) {
      emit->vposition.prescale_cbuf_index = total_consts;
               if (emit->unit == PIPE_SHADER_VERTEX) {
      if (emit->key.vs.undo_viewport) {
         }
   if (emit->key.vs.need_vertex_id_bias) {
                     /* user-defined clip planes */
   if (emit->key.clip_plane_enable) {
      unsigned n = util_bitcount(emit->key.clip_plane_enable);
   assert(emit->unit != PIPE_SHADER_FRAGMENT &&
         for (i = 0; i < n; i++) {
                                 if (emit->key.tex[i].sampler_view) {
      /* Texcoord scale factors for RECT textures */
   if (emit->key.tex[i].unnormalized) {
                  /* Texture buffer sizes */
   if (emit->key.tex[i].target == PIPE_BUFFER) {
               }
   if (emit->key.image_size_used) {
      emit->image_size_index = total_consts;
               if (total_consts > 0) {
      if (total_consts > VGPU10_MAX_CONSTANT_BUFFER_ELEMENT_COUNT) {
      debug_printf("Warning: Too many constants [%u] declared in constant"
               " buffer 0. %u is the limit.\n",
   total_consts = VGPU10_MAX_CONSTANT_BUFFER_ELEMENT_COUNT;
      }
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, 0);  /* which const buffer slot */
   emit_dword(emit, total_consts);
                        for (i = 1; i < ARRAY_SIZE(emit->num_shader_consts); i++) {
      if (emit->num_shader_consts[i] > 0) {
      if (emit->raw_bufs & (1 << i)) {
      /* UBO declared as srv raw buffer */
                        /* UBO declared as const buffer */
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, i);  /* which const buffer slot */
   emit_dword(emit, emit->num_shader_consts[i]);
                        }
         /**
   * Emit declarations for samplers.
   */
   static bool
   emit_sampler_declarations(struct svga_shader_emitter_v10 *emit)
   {
                           VGPU10OpcodeToken0 opcode0;
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_SAMPLER;
            operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_SAMPLER;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, i);
                  }
         /**
   * Translate PIPE_TEXTURE_x to VGPU10_RESOURCE_DIMENSION_x.
   */
   static unsigned
   pipe_texture_to_resource_dimension(enum tgsi_texture_type target,
                     {
      switch (target) {
   case PIPE_BUFFER:
         case PIPE_TEXTURE_1D:
         case PIPE_TEXTURE_2D:
      return num_samples > 2 ? VGPU10_RESOURCE_DIMENSION_TEXTURE2DMS :
      case PIPE_TEXTURE_RECT:
         case PIPE_TEXTURE_3D:
         case PIPE_TEXTURE_CUBE:
         case PIPE_TEXTURE_1D_ARRAY:
      return is_array ? VGPU10_RESOURCE_DIMENSION_TEXTURE1DARRAY
      case PIPE_TEXTURE_2D_ARRAY:
      if (num_samples > 2 && is_array)
         else if (is_array)
         else
      case PIPE_TEXTURE_CUBE_ARRAY:
      return is_uav ? VGPU10_RESOURCE_DIMENSION_TEXTURE2DARRAY :
            default:
      assert(!"Unexpected resource type");
         }
         /**
   * Translate TGSI_TEXTURE_x to VGPU10_RESOURCE_DIMENSION_x.
   */
   static unsigned
   tgsi_texture_to_resource_dimension(enum tgsi_texture_type target,
                     {
      if (target == TGSI_TEXTURE_2D_MSAA && num_samples < 2) {
         }
   else if (target == TGSI_TEXTURE_2D_ARRAY_MSAA && num_samples < 2) {
                  switch (target) {
   case TGSI_TEXTURE_BUFFER:
         case TGSI_TEXTURE_1D:
         case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
         case TGSI_TEXTURE_3D:
         case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_SHADOWCUBE:
      return is_uav ? VGPU10_RESOURCE_DIMENSION_TEXTURE2DARRAY :
      case TGSI_TEXTURE_SHADOW1D:
         case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
         case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      return is_array ? VGPU10_RESOURCE_DIMENSION_TEXTURE1DARRAY
      case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
      return is_array ? VGPU10_RESOURCE_DIMENSION_TEXTURE2DARRAY
      case TGSI_TEXTURE_2D_MSAA:
         case TGSI_TEXTURE_2D_ARRAY_MSAA:
      return is_array ? VGPU10_RESOURCE_DIMENSION_TEXTURE2DMSARRAY
      case TGSI_TEXTURE_CUBE_ARRAY:
      return is_uav ? VGPU10_RESOURCE_DIMENSION_TEXTURE2DARRAY :
            case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      return is_array ? VGPU10_RESOURCE_DIMENSION_TEXTURECUBEARRAY
      default:
      assert(!"Unexpected resource type");
         }
         /**
   * Given a tgsi_return_type, return true iff it is an integer type.
   */
   static bool
   is_integer_type(enum tgsi_return_type type)
   {
      switch (type) {
      case TGSI_RETURN_TYPE_SINT:
   case TGSI_RETURN_TYPE_UINT:
         case TGSI_RETURN_TYPE_FLOAT:
   case TGSI_RETURN_TYPE_UNORM:
   case TGSI_RETURN_TYPE_SNORM:
         case TGSI_RETURN_TYPE_COUNT:
   default:
      assert(!"is_integer_type: Unknown tgsi_return_type");
      }
         /**
   * Emit declarations for resources.
   * XXX When we're sure that all TGSI shaders will be generated with
   * sampler view declarations (Ex: DCL SVIEW[n], 2D, UINT) we may
   * rework this code.
   */
   static bool
   emit_resource_declarations(struct svga_shader_emitter_v10 *emit)
   {
               /* Emit resource decl for each sampler */
   for (i = 0; i < emit->num_samplers; i++) {
      if (!(emit->info.samplers_declared & (1 << i)))
            VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
   VGPU10ResourceReturnTypeToken return_type;
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_RESOURCE;
   if (emit->sampler_view[i] || !emit->key.tex[i].sampler_view) {
      opcode0.resourceDimension =
      tgsi_texture_to_resource_dimension(emit->sampler_target[i],
               }
   else {
      opcode0.resourceDimension =
      pipe_texture_to_resource_dimension(emit->key.tex[i].target,
               }
   opcode0.sampleCount = emit->key.tex[i].num_samples;
   operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_RESOURCE;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
      #if 1
         /* convert TGSI_RETURN_TYPE_x to VGPU10_RETURN_TYPE_x */
   STATIC_ASSERT(VGPU10_RETURN_TYPE_UNORM == TGSI_RETURN_TYPE_UNORM + 1);
   STATIC_ASSERT(VGPU10_RETURN_TYPE_SNORM == TGSI_RETURN_TYPE_SNORM + 1);
   STATIC_ASSERT(VGPU10_RETURN_TYPE_SINT == TGSI_RETURN_TYPE_SINT + 1);
   STATIC_ASSERT(VGPU10_RETURN_TYPE_UINT == TGSI_RETURN_TYPE_UINT + 1);
   STATIC_ASSERT(VGPU10_RETURN_TYPE_FLOAT == TGSI_RETURN_TYPE_FLOAT + 1);
   assert(emit->sampler_return_type[i] <= TGSI_RETURN_TYPE_FLOAT);
   if (emit->sampler_view[i] || !emit->key.tex[i].sampler_view) {
         }
   else {
         #else
         switch (emit->sampler_return_type[i]) {
      case TGSI_RETURN_TYPE_UNORM: rt = VGPU10_RETURN_TYPE_UNORM; break;
   case TGSI_RETURN_TYPE_SNORM: rt = VGPU10_RETURN_TYPE_SNORM; break;
   case TGSI_RETURN_TYPE_SINT:  rt = VGPU10_RETURN_TYPE_SINT;  break;
   case TGSI_RETURN_TYPE_UINT:  rt = VGPU10_RETURN_TYPE_UINT;  break;
   case TGSI_RETURN_TYPE_FLOAT: rt = VGPU10_RETURN_TYPE_FLOAT; break;
   case TGSI_RETURN_TYPE_COUNT:
   default:
      rt = VGPU10_RETURN_TYPE_FLOAT;
   #endif
            return_type.value = 0;
   return_type.component0 = rt;
   return_type.component1 = rt;
   return_type.component2 = rt;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, i);
   emit_dword(emit, return_type.value);
                  }
         /**
   * Emit instruction to declare uav for the shader image
   */
   static void
   emit_image_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned i = 0;
   unsigned unit = 0;
            /* Emit uav decl for each image */
               /* Find the unit index of the next declared image.
   */
   while (!(emit->image_mask & (1 << unit))) {
                  VGPU10OpcodeToken0 opcode0;
   VGPU10OperandToken0 operand0;
            /* If the corresponding uav for the image is already declared,
   * skip this image declaration.
   */
   if (uav_mask & (1 << emit->key.images[unit].uav_index))
            opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_UAV_TYPED;
   opcode0.uavResourceDimension =
      tgsi_texture_to_resource_dimension(emit->image[unit].Resource,
               if (emit->key.images[unit].is_single_layer &&
      emit->key.images[unit].resource_target == PIPE_TEXTURE_3D) {
               /* Declare the uav as global coherent if the shader includes memory
   * barrier instructions.
   */
   opcode0.globallyCoherent =
            operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_UAV;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            return_type.value = 0;
   return_type.component0 =
      return_type.component1 =
               assert(emit->key.images[unit].uav_index != SVGA3D_INVALID_ID);
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, emit->key.images[unit].uav_index);
   emit_dword(emit, return_type.value);
            /* Mark the uav is already declared */
                  }
         /**
   * Emit instruction to declare uav for the shader buffer
   */
   static void
   emit_shader_buf_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned i;
            /* Emit uav decl for each shader buffer */
   for (i = 0; i < emit->num_shader_bufs; i++) {
      VGPU10OpcodeToken0 opcode0;
            if (emit->raw_shaderbufs & (1 << i)) {
      emit_rawbuf_declaration(emit, i + emit->raw_shaderbuf_srv_start_index);
               /* If the corresponding uav for the shader buf is already declared,
   * skip this shader buffer declaration.
   */
   if (uav_mask & (1 << emit->key.shader_buf_uav_index[i]))
            opcode0.value = 0;
            /* Declare the uav as global coherent if the shader includes memory
   * barrier instructions.
   */
   opcode0.globallyCoherent =
            operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_UAV;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            assert(emit->key.shader_buf_uav_index[i] != SVGA3D_INVALID_ID);
   begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, emit->key.shader_buf_uav_index[i]);
            /* Mark the uav is already declared */
                  }
         /**
   * Emit instruction to declare thread group shared memory(tgsm) for shared memory
   */
   static void
   emit_memory_declarations(struct svga_shader_emitter_v10 *emit)
   {
      if (emit->cs.shared_memory_declared) {
      VGPU10OpcodeToken0 opcode0;
            opcode0.value = 0;
            /* Declare the uav as global coherent if the shader includes memory
   * barrier instructions.
   */
   opcode0.globallyCoherent =
            operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
            /* Current state tracker only declares one shared memory for GLSL.
   * Use index 0 for this shared memory.
   */
   emit_dword(emit, 0);
   emit_dword(emit, emit->key.cs.mem_size); /* byte Count */
         }
         /**
   * Emit instruction to declare uav for atomic buffers
   */
   static void
   emit_atomic_buf_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned atomic_bufs_mask = emit->atomic_bufs_mask;
            /* Emit uav decl for each atomic buffer */
   while (atomic_bufs_mask) {
      unsigned buf_index = u_bit_scan(&atomic_bufs_mask);
            /* If the corresponding uav for the shader buf is already declared,
   * skip this shader buffer declaration.
   */
   if (uav_mask & (1 << uav_index))
            VGPU10OpcodeToken0 opcode0;
                     opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_DCL_UAV_RAW;
            /* Declare the uav as global coherent if the shader includes memory
   * barrier instructions.
   */
   opcode0.globallyCoherent =
                  operand0.value = 0;
   operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   operand0.operandType = VGPU10_OPERAND_TYPE_UAV;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dword(emit, operand0.value);
   emit_dword(emit, uav_index);
            /* Mark the uav is already declared */
                        /* Allocate immediates to be used for index to the atomic buffers */
   unsigned j = 0;
   for (unsigned i = 0; i <= emit->num_atomic_bufs / 4; i++, j+=4) {
                  /* Allocate immediates for the atomic counter index */
   for (; j <= emit->max_atomic_counter_index; j+=4) {
            }
         /**
   * Emit instruction with n=1, 2 or 3 source registers.
   */
   static void
   emit_instruction_opn(struct svga_shader_emitter_v10 *emit,
                        unsigned opcode,
   const struct tgsi_full_dst_register *dst,
      {
      begin_emit_instruction(emit);
   emit_opcode_precise(emit, opcode, saturate, precise);
   emit_dst_register(emit, dst);
   emit_src_register(emit, src1);
   if (src2) {
         }
   if (src3) {
         }
      }
      static void
   emit_instruction_op1(struct svga_shader_emitter_v10 *emit,
                     {
         }
      static void
   emit_instruction_op2(struct svga_shader_emitter_v10 *emit,
                           {
         }
      static void
   emit_instruction_op3(struct svga_shader_emitter_v10 *emit,
                        VGPU10_OPCODE_TYPE opcode,
      {
         }
      static void
   emit_instruction_op0(struct svga_shader_emitter_v10 *emit,
         {
      begin_emit_instruction(emit);
   emit_opcode(emit, opcode, false);
      }
      /**
   * Tessellation inner/outer levels needs to be store into its
   * appropriate registers depending on prim_mode.
   */
   static void
   store_tesslevels(struct svga_shader_emitter_v10 *emit)
   {
               /* tessellation levels are required input/out in hull shader.
   * emitting the inner/outer tessellation levels, either from
   * values provided in tcs or fallback default values which is 1.0
   */
   if (emit->key.tcs.prim_mode == MESA_PRIM_QUADS) {
               if (emit->tcs.inner.tgsi_index != INVALID_INDEX)
         else
            for (i = 0; i < 2; i++) {
      struct tgsi_full_src_register src =
         struct tgsi_full_dst_register dst =
         dst = writemask_dst(&dst, TGSI_WRITEMASK_X);
               if (emit->tcs.outer.tgsi_index != INVALID_INDEX)
         else
            for (i = 0; i < 4; i++) {
      struct tgsi_full_src_register src =
         struct tgsi_full_dst_register dst =
         dst = writemask_dst(&dst, TGSI_WRITEMASK_X);
         }
   else if (emit->key.tcs.prim_mode == MESA_PRIM_TRIANGLES) {
               if (emit->tcs.inner.tgsi_index != INVALID_INDEX)
         else
            struct tgsi_full_src_register src =
         struct tgsi_full_dst_register dst =
         dst = writemask_dst(&dst, TGSI_WRITEMASK_X);
            if (emit->tcs.outer.tgsi_index != INVALID_INDEX)
         else
            for (i = 0; i < 3; i++) {
      struct tgsi_full_src_register src =
         struct tgsi_full_dst_register dst =
         dst = writemask_dst(&dst, TGSI_WRITEMASK_X);
         }
   else if (emit->key.tcs.prim_mode ==  MESA_PRIM_LINES) {
      if (emit->tcs.outer.tgsi_index != INVALID_INDEX) {
      struct tgsi_full_src_register temp_src =
         for (i = 0; i < 2; i++) {
      struct tgsi_full_src_register src =
         struct tgsi_full_dst_register dst =
      make_dst_reg(TGSI_FILE_OUTPUT,
      dst = writemask_dst(&dst, TGSI_WRITEMASK_X);
            }
   else {
            }
         /**
   * Emit the actual clip distance instructions to be used for clipping
   * by copying the clip distance from the temporary registers to the
   * CLIPDIST registers written with the enabled planes mask.
   * Also copy the clip distance from the temporary to the clip distance
   * shadow copy register which will be referenced by the input shader
   */
   static void
   emit_clip_distance_instructions(struct svga_shader_emitter_v10 *emit)
   {
      struct tgsi_full_src_register tmp_clip_dist_src;
            unsigned i;
   unsigned clip_plane_enable = emit->key.clip_plane_enable;
   unsigned clip_dist_tmp_index = emit->clip_dist_tmp_index;
            assert(emit->clip_dist_out_index != INVALID_INDEX);
            /**
   * Temporary reset the temporary clip dist register index so
   * that the copy to the real clip dist register will not
   * attempt to copy to the temporary register again
   */
                                 /**
   * copy to the shadow copy for use by varying variable and
   * stream output. All clip distances
   * will be written regardless of the enabled clipping planes.
   */
   clip_dist_dst = make_dst_reg(TGSI_FILE_OUTPUT,
            /* MOV clip_dist_so, tmp_clip_dist */
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &clip_dist_dst,
            /**
   * copy those clip distances to enabled clipping planes
   * to CLIPDIST registers for clipping
   */
   if (clip_plane_enable & 0xf) {
      clip_dist_dst = make_dst_reg(TGSI_FILE_OUTPUT,
                  /* MOV CLIPDIST, tmp_clip_dist */
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &clip_dist_dst,
      }
   /* four clip planes per clip register */
      }
   /**
   * set the temporary clip dist register index back to the
   * temporary index for the next vertex
   */
      }
      /* Declare clip distance output registers for user-defined clip planes
   * or the TGSI_CLIPVERTEX output.
   */
   static void
   emit_clip_distance_declarations(struct svga_shader_emitter_v10 *emit)
   {
      unsigned num_clip_planes = util_bitcount(emit->key.clip_plane_enable);
   unsigned index = emit->num_outputs;
            assert(emit->unit != PIPE_SHADER_FRAGMENT);
            if (emit->clip_mode != CLIP_LEGACY &&
      emit->clip_mode != CLIP_VERTEX) {
               if (num_clip_planes == 0)
            /* Convert clip vertex to clip distances only in the last vertex stage */
   if (!emit->key.last_vertex_stage)
            /* Declare one or two clip output registers.  The number of components
   * in the mask reflects the number of clip planes.  For example, if 5
   * clip planes are needed, we'll declare outputs similar to:
   * dcl_output_siv o2.xyzw, clip_distance
   * dcl_output_siv o3.x, clip_distance
   */
            plane_mask = (1 << num_clip_planes) - 1;
   if (plane_mask & 0xf) {
      unsigned cmask = plane_mask & VGPU10_OPERAND_4_COMPONENT_MASK_ALL;
   emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT_SIV, index,
                  }
   if (plane_mask & 0xf0) {
      unsigned cmask = (plane_mask >> 4) & VGPU10_OPERAND_4_COMPONENT_MASK_ALL;
   emit_output_declaration(emit, VGPU10_OPCODE_DCL_OUTPUT_SIV, index + 1,
                     }
         /**
   * Emit the instructions for writing to the clip distance registers
   * to handle legacy/automatic clip planes.
   * For each clip plane, the distance is the dot product of the vertex
   * position (found in TEMP[vpos_tmp_index]) and the clip plane coefficients.
   * This is not used when the shader has an explicit CLIPVERTEX or CLIPDISTANCE
   * output registers already declared.
   */
   static void
   emit_clip_distance_from_vpos(struct svga_shader_emitter_v10 *emit,
         {
               assert(emit->clip_mode == CLIP_LEGACY);
            assert(emit->unit == PIPE_SHADER_VERTEX ||
                  for (i = 0; i < num_clip_planes; i++) {
      struct tgsi_full_dst_register dst;
   struct tgsi_full_src_register plane_src, vpos_src;
   unsigned reg_index = emit->clip_dist_out_index + i / 4;
   unsigned comp = i % 4;
            /* create dst, src regs */
   dst = make_dst_reg(TGSI_FILE_OUTPUT, reg_index);
            plane_src = make_src_const_reg(emit->clip_plane_const[i]);
            /* DP4 clip_dist, plane, vpos */
   emit_instruction_op2(emit, VGPU10_OPCODE_DP4, &dst,
         }
         /**
   * Emit the instructions for computing the clip distance results from
   * the clip vertex temporary.
   * For each clip plane, the distance is the dot product of the clip vertex
   * position (found in a temp reg) and the clip plane coefficients.
   */
   static void
   emit_clip_vertex_instructions(struct svga_shader_emitter_v10 *emit)
   {
      const unsigned num_clip = util_bitcount(emit->key.clip_plane_enable);
   unsigned i;
   struct tgsi_full_dst_register dst;
   struct tgsi_full_src_register clipvert_src;
            assert(emit->unit == PIPE_SHADER_VERTEX ||
                                    for (i = 0; i < num_clip; i++) {
      struct tgsi_full_src_register plane_src;
   unsigned reg_index = emit->clip_dist_out_index + i / 4;
   unsigned comp = i % 4;
            /* create dst, src regs */
   dst = make_dst_reg(TGSI_FILE_OUTPUT, reg_index);
                     /* DP4 clip_dist, plane, vpos */
   emit_instruction_op2(emit, VGPU10_OPCODE_DP4, &dst,
                                 /**
   * temporary reset the temporary clip vertex register index so
   * that copy to the clip vertex register will not attempt
   * to copy to the temporary register again
   */
            /* MOV clip_vertex, clip_vertex_tmp */
   dst = make_dst_reg(TGSI_FILE_OUTPUT, emit->clip_vertex_out_index);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV,
            /**
   * set the temporary clip vertex register index back to the
   * temporary index for the next vertex
   */
      }
      /**
   * Emit code to convert RGBA to BGRA
   */
   static void
   emit_swap_r_b(struct svga_shader_emitter_v10 *emit,
               {
      struct tgsi_full_src_register bgra_src =
            begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_MOV, false);
   emit_dst_register(emit, dst);
   emit_src_register(emit, &bgra_src);
      }
         /** Convert from 10_10_10_2 normalized to 10_10_10_2_snorm */
   static void
   emit_puint_to_snorm(struct svga_shader_emitter_v10 *emit,
               {
      struct tgsi_full_src_register half = make_immediate_reg_float(emit, 0.5f);
   struct tgsi_full_src_register two =
         struct tgsi_full_src_register neg_two =
            unsigned val_tmp = get_temp_index(emit);
   struct tgsi_full_dst_register val_dst = make_dst_temp_reg(val_tmp);
            unsigned bias_tmp = get_temp_index(emit);
   struct tgsi_full_dst_register bias_dst = make_dst_temp_reg(bias_tmp);
            /* val = src * 2.0 */
            /* bias = src > 0.5 */
            /* bias = bias & -2.0 */
   emit_instruction_op2(emit, VGPU10_OPCODE_AND, &bias_dst,
            /* dst = val + bias */
   emit_instruction_op2(emit, VGPU10_OPCODE_ADD, dst,
               }
         /** Convert from 10_10_10_2_unorm to 10_10_10_2_uscaled */
   static void
   emit_puint_to_uscaled(struct svga_shader_emitter_v10 *emit,
               {
      struct tgsi_full_src_register scale =
            /* dst = src * scale */
      }
         /** Convert from R32_UINT to 10_10_10_2_sscaled */
   static void
   emit_puint_to_sscaled(struct svga_shader_emitter_v10 *emit,
               {
      struct tgsi_full_src_register lshift =
         struct tgsi_full_src_register rshift =
                     unsigned tmp = get_temp_index(emit);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
            /*
   * r = (pixel << 22) >> 22;   # signed int in [511, -512]
   * g = (pixel << 12) >> 22;   # signed int in [511, -512]
   * b = (pixel <<  2) >> 22;   # signed int in [511, -512]
   * a = (pixel <<  0) >> 30;   # signed int in [1, -2]
   * dst = i_to_f(r,g,b,a);     # convert to float
   */
   emit_instruction_op2(emit, VGPU10_OPCODE_ISHL, &tmp_dst,
         emit_instruction_op2(emit, VGPU10_OPCODE_ISHR, &tmp_dst,
                     }
         /**
   * Emit code for TGSI_OPCODE_ARL or TGSI_OPCODE_UARL instruction.
   */
   static bool
   emit_arl_uarl(struct svga_shader_emitter_v10 *emit,
         {
      unsigned index = inst->Dst[0].Register.Index;
   struct tgsi_full_dst_register dst;
            assert(index < MAX_VGPU10_ADDR_REGS);
   dst = make_dst_temp_reg(emit->address_reg_index[index]);
            /* ARL dst, s0
   * Translates into:
   * FTOI address_tmp, s0
   *
   * UARL dst, s0
   * Translates into:
   * MOV address_tmp, s0
   */
   if (inst->Instruction.Opcode == TGSI_OPCODE_ARL)
         else
                        }
         /**
   * Emit code for TGSI_OPCODE_CAL instruction.
   */
   static bool
   emit_cal(struct svga_shader_emitter_v10 *emit,
         {
      unsigned label = inst->Label.Label;
   VGPU10OperandToken0 operand;
   operand.value = 0;
            begin_emit_instruction(emit);
   emit_dword(emit, operand.value);
   emit_dword(emit, label);
               }
         /**
   * Emit code for TGSI_OPCODE_IABS instruction.
   */
   static bool
   emit_iabs(struct svga_shader_emitter_v10 *emit,
         {
      /* dst.x = (src0.x < 0) ? -src0.x : src0.x
   * dst.y = (src0.y < 0) ? -src0.y : src0.y
   * dst.z = (src0.z < 0) ? -src0.z : src0.z
   * dst.w = (src0.w < 0) ? -src0.w : src0.w
   *
   * Translates into
   *   IMAX dst, src, neg(src)
   */
   struct tgsi_full_src_register neg_src = negate_src(&inst->Src[0]);
   emit_instruction_op2(emit, VGPU10_OPCODE_IMAX, &inst->Dst[0],
               }
         /**
   * Emit code for TGSI_OPCODE_CMP instruction.
   */
   static bool
   emit_cmp(struct svga_shader_emitter_v10 *emit,
         {
      /* dst.x = (src0.x < 0) ? src1.x : src2.x
   * dst.y = (src0.y < 0) ? src1.y : src2.y
   * dst.z = (src0.z < 0) ? src1.z : src2.z
   * dst.w = (src0.w < 0) ? src1.w : src2.w
   *
   * Translates into
   *   LT tmp, src0, 0.0
   *   MOVC dst, tmp, src1, src2
   */
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
            emit_instruction_opn(emit, VGPU10_OPCODE_LT, &tmp_dst,
               emit_instruction_opn(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0],
                              }
         /**
   * Emit code for TGSI_OPCODE_DST instruction.
   */
   static bool
   emit_dst(struct svga_shader_emitter_v10 *emit,
         {
      /*
   * dst.x = 1
   * dst.y = src0.y * src1.y
   * dst.z = src0.z
   * dst.w = src1.w
            struct tgsi_full_src_register s0_yyyy =
         struct tgsi_full_src_register s0_zzzz =
         struct tgsi_full_src_register s1_yyyy =
         struct tgsi_full_src_register s1_wwww =
            /*
   * If dst and either src0 and src1 are the same we need
   * to create a temporary for it and insert a extra move.
   */
   unsigned tmp_move = get_temp_index(emit);
   struct tgsi_full_src_register move_src = make_src_temp_reg(tmp_move);
            /* MOV dst.x, 1.0 */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      struct tgsi_full_dst_register dst_x =
                              /* MUL dst.y, s0.y, s1.y */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      struct tgsi_full_dst_register dst_y =
            emit_instruction_opn(emit, VGPU10_OPCODE_MUL, &dst_y, &s0_yyyy,
                     /* MOV dst.z, s0.z */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      struct tgsi_full_dst_register dst_z =
            emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
               }
         /* MOV dst.w, s1.w */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      struct tgsi_full_dst_register dst_w =
            emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
                           emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &move_src);
               }
         /**
   * A helper function to return the stream index as specified in
   * the immediate register
   */
   static inline unsigned
   find_stream_index(struct svga_shader_emitter_v10 *emit,
         {
         }
         /**
   * Emit code for TGSI_OPCODE_ENDPRIM (GS only)
   */
   static bool
   emit_endprim(struct svga_shader_emitter_v10 *emit,
         {
               begin_emit_instruction(emit);
   if (emit->version >= 50) {
               if (emit->info.num_stream_output_components[streamIndex] == 0) {
      /**
   * If there is no output for this stream, discard this instruction.
   */
      }
   else {
      emit_opcode(emit, VGPU10_OPCODE_CUT_STREAM, false);
   assert(inst->Src[0].Register.File == TGSI_FILE_IMMEDIATE);
         }
   else {
         }
   end_emit_instruction(emit);
      }
         /**
   * Emit code for TGSI_OPCODE_EX2 (2^x) instruction.
   */
   static bool
   emit_ex2(struct svga_shader_emitter_v10 *emit,
         {
      /* Note that TGSI_OPCODE_EX2 computes only one value from src.x
   * while VGPU10 computes four values.
   *
   * dst = EX2(src):
   *   dst.xyzw = 2.0 ^ src.x
            struct tgsi_full_src_register src_xxxx =
      swizzle_src(&inst->Src[0], TGSI_SWIZZLE_X, TGSI_SWIZZLE_X,
         /* EXP tmp, s0.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_EXP, &inst->Dst[0], &src_xxxx,
                           }
         /**
   * Emit code for TGSI_OPCODE_EXP instruction.
   */
   static bool
   emit_exp(struct svga_shader_emitter_v10 *emit,
         {
      /*
   * dst.x = 2 ^ floor(s0.x)
   * dst.y = s0.x - floor(s0.x)
   * dst.z = 2 ^ s0.x
   * dst.w = 1.0
            struct tgsi_full_src_register src_xxxx =
         unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
            /*
   * If dst and src are the same we need to create
   * a temporary for it and insert a extra move.
   */
   unsigned tmp_move = get_temp_index(emit);
   struct tgsi_full_src_register move_src = make_src_temp_reg(tmp_move);
            /* only use X component of temp reg */
   tmp_dst = writemask_dst(&tmp_dst, TGSI_WRITEMASK_X);
            /* ROUND_NI tmp.x, s0.x */
   emit_instruction_op1(emit, VGPU10_OPCODE_ROUND_NI, &tmp_dst,
            /* EXP dst.x, tmp.x */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      struct tgsi_full_dst_register dst_x =
            emit_instruction_opn(emit, VGPU10_OPCODE_EXP, &dst_x, &tmp_src,
                           /* ADD dst.y, s0.x, -tmp */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      struct tgsi_full_dst_register dst_y =
                  emit_instruction_opn(emit, VGPU10_OPCODE_ADD, &dst_y, &src_xxxx,
                           /* EXP dst.z, s0.x */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      struct tgsi_full_dst_register dst_z =
            emit_instruction_opn(emit, VGPU10_OPCODE_EXP, &dst_z, &src_xxxx,
                           /* MOV dst.w, 1.0 */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      struct tgsi_full_dst_register dst_w =
                                                   }
         /**
   * Emit code for TGSI_OPCODE_IF instruction.
   */
   static bool
   emit_if(struct svga_shader_emitter_v10 *emit,
         {
               /* The src register should be a scalar */
   assert(src->Register.SwizzleX == src->Register.SwizzleY &&
                  /* The only special thing here is that we need to set the
   * VGPU10_INSTRUCTION_TEST_NONZERO flag since we want to test if
   * src.x is non-zero.
   */
   opcode0.value = 0;
   opcode0.opcodeType = VGPU10_OPCODE_IF;
            begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_src_register(emit, src);
               }
         /**
   * Emit code for conditional discard instruction (discard fragment if any of
   * the register components are negative).
   */
   static bool
   emit_cond_discard(struct svga_shader_emitter_v10 *emit,
         {
      unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
                     struct tgsi_full_dst_register tmp_dst_x =
         struct tgsi_full_src_register tmp_src_xxxx =
            /* tmp = src[0] < 0.0 */
            if (!same_swizzle_terms(&inst->Src[0])) {
      /* If the swizzle is not XXXX, YYYY, ZZZZ or WWWW we need to
   * logically OR the swizzle terms.  Most uses of this conditional
   * discard instruction only test one channel so it's good to
   * avoid these extra steps.
   */
   struct tgsi_full_src_register tmp_src_yyyy =
         struct tgsi_full_src_register tmp_src_zzzz =
         struct tgsi_full_src_register tmp_src_wwww =
            emit_instruction_op2(emit, VGPU10_OPCODE_OR, &tmp_dst_x, &tmp_src_xxxx,
         emit_instruction_op2(emit, VGPU10_OPCODE_OR, &tmp_dst_x, &tmp_src_xxxx,
         emit_instruction_op2(emit, VGPU10_OPCODE_OR, &tmp_dst_x, &tmp_src_xxxx,
               begin_emit_instruction(emit);
   emit_discard_opcode(emit, true); /* discard if src0.x is non-zero */
   emit_src_register(emit, &tmp_src_xxxx);
                        }
         /**
   * Emit code for the unconditional discard instruction.
   */
   static bool
   emit_discard(struct svga_shader_emitter_v10 *emit,
         {
               /* DISCARD if 0.0 is zero */
   begin_emit_instruction(emit);
   emit_discard_opcode(emit, false);
   emit_src_register(emit, &zero);
               }
         /**
   * Emit code for TGSI_OPCODE_LG2 instruction.
   */
   static bool
   emit_lg2(struct svga_shader_emitter_v10 *emit,
         {
      /* Note that TGSI_OPCODE_LG2 computes only one value from src.x
   * while VGPU10 computes four values.
   *
   * dst = LG2(src):
   *   dst.xyzw = log2(src.x)
            struct tgsi_full_src_register src_xxxx =
      swizzle_src(&inst->Src[0], TGSI_SWIZZLE_X, TGSI_SWIZZLE_X,
         /* LOG tmp, s0.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_LOG,
                           }
         /**
   * Emit code for TGSI_OPCODE_LIT instruction.
   */
   static bool
   emit_lit(struct svga_shader_emitter_v10 *emit,
         {
               /*
   * If dst and src are the same we need to create
   * a temporary for it and insert a extra move.
   */
   unsigned tmp_move = get_temp_index(emit);
   struct tgsi_full_src_register move_src = make_src_temp_reg(tmp_move);
            /*
   * dst.x = 1
   * dst.y = max(src.x, 0)
   * dst.z = (src.x > 0) ? max(src.y, 0)^{clamp(src.w, -128, 128))} : 0
   * dst.w = 1
            /* MOV dst.x, 1.0 */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      struct tgsi_full_dst_register dst_x =
                     /* MOV dst.w, 1.0 */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      struct tgsi_full_dst_register dst_w =
                     /* MAX dst.y, src.x, 0.0 */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      struct tgsi_full_dst_register dst_y =
         struct tgsi_full_src_register zero =
         struct tgsi_full_src_register src_xxxx =
                  emit_instruction_opn(emit, VGPU10_OPCODE_MAX, &dst_y, &src_xxxx,
               /*
   * tmp1 = clamp(src.w, -128, 128);
   *   MAX tmp1, src.w, -128
   *   MIN tmp1, tmp1, 128
   *
   * tmp2 = max(tmp2, 0);
   *   MAX tmp2, src.y, 0
   *
   * tmp1 = pow(tmp2, tmp1);
   *   LOG tmp2, tmp2
   *   MUL tmp1, tmp2, tmp1
   *   EXP tmp1, tmp1
   *
   * tmp1 = (src.w == 0) ? 1 : tmp1;
   *   EQ tmp2, 0, src.w
   *   MOVC tmp1, tmp2, 1.0, tmp1
   *
   * dst.z = (0 < src.x) ? tmp1 : 0;
   *   LT tmp2, 0, src.x
   *   MOVC dst.z, tmp2, tmp1, 0.0
   */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      struct tgsi_full_dst_register dst_z =
            unsigned tmp1 = get_temp_index(emit);
   struct tgsi_full_src_register tmp1_src = make_src_temp_reg(tmp1);
   struct tgsi_full_dst_register tmp1_dst = make_dst_temp_reg(tmp1);
   unsigned tmp2 = get_temp_index(emit);
   struct tgsi_full_src_register tmp2_src = make_src_temp_reg(tmp2);
            struct tgsi_full_src_register src_xxxx =
         struct tgsi_full_src_register src_yyyy =
         struct tgsi_full_src_register src_wwww =
            struct tgsi_full_src_register zero =
         struct tgsi_full_src_register lowerbound =
         struct tgsi_full_src_register upperbound =
            emit_instruction_op2(emit, VGPU10_OPCODE_MAX, &tmp1_dst, &src_wwww,
         emit_instruction_op2(emit, VGPU10_OPCODE_MIN, &tmp1_dst, &tmp1_src,
         emit_instruction_op2(emit, VGPU10_OPCODE_MAX, &tmp2_dst, &src_yyyy,
            /* POW tmp1, tmp2, tmp1 */
   /* LOG tmp2, tmp2 */
            /* MUL tmp1, tmp2, tmp1 */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL, &tmp1_dst, &tmp2_src,
            /* EXP tmp1, tmp1 */
            /* EQ tmp2, 0, src.w */
   emit_instruction_op2(emit, VGPU10_OPCODE_EQ, &tmp2_dst, &zero, &src_wwww);
   /* MOVC tmp1.z, tmp2, tmp1, 1.0 */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &tmp1_dst,
            /* LT tmp2, 0, src.x */
   emit_instruction_op2(emit, VGPU10_OPCODE_LT, &tmp2_dst, &zero, &src_xxxx);
   /* MOVC dst.z, tmp2, tmp1, 0.0 */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &dst_z,
               emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &move_src);
               }
         /**
   * Emit Level Of Detail Query (LODQ) instruction.
   */
   static bool
   emit_lodq(struct svga_shader_emitter_v10 *emit,
         {
                        /* LOD dst, coord, resource, sampler */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_LOD, false);
   emit_dst_register(emit, &inst->Dst[0]);
   emit_src_register(emit, &inst->Src[0]); /* coord */
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
               }
         /**
   * Emit code for TGSI_OPCODE_LOG instruction.
   */
   static bool
   emit_log(struct svga_shader_emitter_v10 *emit,
         {
      /*
   * dst.x = floor(lg2(abs(s0.x)))
   * dst.y = abs(s0.x) / (2 ^ floor(lg2(abs(s0.x))))
   * dst.z = lg2(abs(s0.x))
   * dst.w = 1.0
            struct tgsi_full_src_register src_xxxx =
         unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
            /* only use X component of temp reg */
   tmp_dst = writemask_dst(&tmp_dst, TGSI_WRITEMASK_X);
            /* LOG tmp.x, abs(s0.x) */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XYZ) {
                  /* MOV dst.z, tmp.x */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      struct tgsi_full_dst_register dst_z =
            emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
                     /* FLR tmp.x, tmp.x */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XY) {
                  /* MOV dst.x, tmp.x */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      struct tgsi_full_dst_register dst_x =
            emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
                     /* EXP tmp.x, tmp.x */
   /* DIV dst.y, abs(s0.x), tmp.x */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      struct tgsi_full_dst_register dst_y =
            emit_instruction_op1(emit, VGPU10_OPCODE_EXP, &tmp_dst, &tmp_src);
   emit_instruction_opn(emit, VGPU10_OPCODE_DIV, &dst_y, &abs_src_xxxx,
               /* MOV dst.w, 1.0 */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      struct tgsi_full_dst_register dst_w =
         struct tgsi_full_src_register one =
                                    }
         /**
   * Emit code for TGSI_OPCODE_LRP instruction.
   */
   static bool
   emit_lrp(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = LRP(s0, s1, s2):
   *   dst = s0 * (s1 - s2) + s2
   * Translates into:
   *   SUB tmp, s1, s2;        tmp = s1 - s2
   *   MAD dst, s0, tmp, s2;   dst = s0 * t1 + s2
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register src_tmp = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register dst_tmp = make_dst_temp_reg(tmp);
            /* ADD tmp, s1, -s2 */
   emit_instruction_opn(emit, VGPU10_OPCODE_ADD, &dst_tmp,
                  /* MAD dst, s1, tmp, s3 */
   emit_instruction_opn(emit, VGPU10_OPCODE_MAD, &inst->Dst[0],
                                    }
         /**
   * Emit code for TGSI_OPCODE_POW instruction.
   */
   static bool
   emit_pow(struct svga_shader_emitter_v10 *emit,
         {
      /* Note that TGSI_OPCODE_POW computes only one value from src0.x and
   * src1.x while VGPU10 computes four values.
   *
   * dst = POW(src0, src1):
   *   dst.xyzw = src0.x ^ src1.x
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register src0_xxxx =
      swizzle_src(&inst->Src[0], TGSI_SWIZZLE_X, TGSI_SWIZZLE_X,
      struct tgsi_full_src_register src1_xxxx =
      swizzle_src(&inst->Src[1], TGSI_SWIZZLE_X, TGSI_SWIZZLE_X,
         /* LOG tmp, s0.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_LOG,
                  /* MUL tmp, tmp, s1.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_MUL,
                  /* EXP tmp, s0.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_EXP,
                        /* free tmp */
               }
         /**
   * Emit code for TGSI_OPCODE_RCP (reciprocal) instruction.
   */
   static bool
   emit_rcp(struct svga_shader_emitter_v10 *emit,
         {
      if (emit->version >= 50) {
      /* use new RCP instruction.  But VGPU10_OPCODE_RCP is component-wise
   * while TGSI_OPCODE_RCP computes dst.xyzw = 1.0 / src.xxxx so we need
   * to manipulate the src register's swizzle.
   */
   struct tgsi_full_src_register src = inst->Src[0];
   src.Register.SwizzleY =
   src.Register.SwizzleZ =
            begin_emit_instruction(emit);
   emit_opcode_precise(emit, VGPU10_OPCODE_RCP,
               emit_dst_register(emit, &inst->Dst[0]);
   emit_src_register(emit, &src);
      }
   else {
               unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
            struct tgsi_full_dst_register tmp_dst_x =
         struct tgsi_full_src_register tmp_src_xxxx =
            /* DIV tmp.x, 1.0, s0 */
   emit_instruction_opn(emit, VGPU10_OPCODE_DIV,
                  /* MOV dst, tmp.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
                                       }
         /**
   * Emit code for TGSI_OPCODE_RSQ instruction.
   */
   static bool
   emit_rsq(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = RSQ(src):
   *   dst.xyzw = 1 / sqrt(src.x)
   * Translates into:
   *   RSQ tmp, src.x
   *   MOV dst, tmp.xxxx
            unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
            struct tgsi_full_dst_register tmp_dst_x =
         struct tgsi_full_src_register tmp_src_xxxx =
            /* RSQ tmp, src.x */
   emit_instruction_opn(emit, VGPU10_OPCODE_RSQ,
                  /* MOV dst, tmp.xxxx */
   emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
                        /* free tmp */
               }
         /**
   * Emit code for TGSI_OPCODE_SEQ (Set Equal) instruction.
   */
   static bool
   emit_seq(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = SEQ(s0, s1):
   *   dst = s0 == s1 ? 1.0 : 0.0  (per component)
   * Translates into:
   *   EQ tmp, s0, s1;           tmp = s0 == s1 : 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp, 1.0, 0.0;  dst = tmp ? 1.0 : 0.0 (per component)
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
            /* EQ tmp, s0, s1 */
   emit_instruction_op2(emit, VGPU10_OPCODE_EQ, &tmp_dst, &inst->Src[0],
            /* MOVC dst, tmp, one, zero */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_SGE (Set Greater than or Equal) instruction.
   */
   static bool
   emit_sge(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = SGE(s0, s1):
   *   dst = s0 >= s1 ? 1.0 : 0.0  (per component)
   * Translates into:
   *   GE tmp, s0, s1;           tmp = s0 >= s1 : 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp, 1.0, 0.0;  dst = tmp ? 1.0 : 0.0 (per component)
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
            /* GE tmp, s0, s1 */
   emit_instruction_op2(emit, VGPU10_OPCODE_GE, &tmp_dst, &inst->Src[0],
            /* MOVC dst, tmp, one, zero */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_SGT (Set Greater than) instruction.
   */
   static bool
   emit_sgt(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = SGT(s0, s1):
   *   dst = s0 > s1 ? 1.0 : 0.0  (per component)
   * Translates into:
   *   LT tmp, s1, s0;           tmp = s1 < s0 ? 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp, 1.0, 0.0;  dst = tmp ? 1.0 : 0.0 (per component)
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
            /* LT tmp, s1, s0 */
   emit_instruction_op2(emit, VGPU10_OPCODE_LT, &tmp_dst, &inst->Src[1],
            /* MOVC dst, tmp, one, zero */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_SIN and TGSI_OPCODE_COS instructions.
   */
   static bool
   emit_sincos(struct svga_shader_emitter_v10 *emit,
         {
      unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
            struct tgsi_full_src_register tmp_src_xxxx =
         struct tgsi_full_dst_register tmp_dst_x =
            begin_emit_instruction(emit);
            if(inst->Instruction.Opcode == TGSI_OPCODE_SIN)
   {
      emit_dst_register(emit, &tmp_dst_x);  /* first destination register */
      }
   else {
      emit_null_dst_register(emit);
               emit_src_register(emit, &inst->Src[0]);
            emit_instruction_opn(emit, VGPU10_OPCODE_MOV,
                                    }
         /**
   * Emit code for TGSI_OPCODE_SLE (Set Less than or Equal) instruction.
   */
   static bool
   emit_sle(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = SLE(s0, s1):
   *   dst = s0 <= s1 ? 1.0 : 0.0  (per component)
   * Translates into:
   *   GE tmp, s1, s0;           tmp = s1 >= s0 : 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp, 1.0, 0.0;  dst = tmp ? 1.0 : 0.0 (per component)
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
            /* GE tmp, s1, s0 */
   emit_instruction_op2(emit, VGPU10_OPCODE_GE, &tmp_dst, &inst->Src[1],
            /* MOVC dst, tmp, one, zero */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_SLT (Set Less than) instruction.
   */
   static bool
   emit_slt(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = SLT(s0, s1):
   *   dst = s0 < s1 ? 1.0 : 0.0  (per component)
   * Translates into:
   *   LT tmp, s0, s1;           tmp = s0 < s1 ? 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp, 1.0, 0.0;  dst = tmp ? 1.0 : 0.0 (per component)
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
            /* LT tmp, s0, s1 */
   emit_instruction_op2(emit, VGPU10_OPCODE_LT, &tmp_dst, &inst->Src[0],
            /* MOVC dst, tmp, one, zero */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_SNE (Set Not Equal) instruction.
   */
   static bool
   emit_sne(struct svga_shader_emitter_v10 *emit,
         {
      /* dst = SNE(s0, s1):
   *   dst = s0 != s1 ? 1.0 : 0.0  (per component)
   * Translates into:
   *   EQ tmp, s0, s1;           tmp = s0 == s1 : 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp, 1.0, 0.0;  dst = tmp ? 1.0 : 0.0 (per component)
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
            /* NE tmp, s0, s1 */
   emit_instruction_op2(emit, VGPU10_OPCODE_NE, &tmp_dst, &inst->Src[0],
            /* MOVC dst, tmp, one, zero */
   emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_SSG (Set Sign) instruction.
   */
   static bool
   emit_ssg(struct svga_shader_emitter_v10 *emit,
         {
      /* dst.x = (src.x > 0.0) ? 1.0 : (src.x < 0.0) ? -1.0 : 0.0
   * dst.y = (src.y > 0.0) ? 1.0 : (src.y < 0.0) ? -1.0 : 0.0
   * dst.z = (src.z > 0.0) ? 1.0 : (src.z < 0.0) ? -1.0 : 0.0
   * dst.w = (src.w > 0.0) ? 1.0 : (src.w < 0.0) ? -1.0 : 0.0
   * Translates into:
   *   LT tmp1, src, zero;           tmp1 = src < zero ? 0xffffffff : 0 (per comp)
   *   MOVC tmp2, tmp1, -1.0, 0.0;   tmp2 = tmp1 ? -1.0 : 0.0 (per component)
   *   LT tmp1, zero, src;           tmp1 = zero < src ? 0xffffffff : 0 (per comp)
   *   MOVC dst, tmp1, 1.0, tmp2;    dst = tmp1 ? 1.0 : tmp2 (per component)
   */
   struct tgsi_full_src_register zero =
         struct tgsi_full_src_register one =
         struct tgsi_full_src_register neg_one =
            unsigned tmp1 = get_temp_index(emit);
   struct tgsi_full_src_register tmp1_src = make_src_temp_reg(tmp1);
            unsigned tmp2 = get_temp_index(emit);
   struct tgsi_full_src_register tmp2_src = make_src_temp_reg(tmp2);
            emit_instruction_op2(emit, VGPU10_OPCODE_LT, &tmp1_dst, &inst->Src[0],
         emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &tmp2_dst, &tmp1_src,
         emit_instruction_op2(emit, VGPU10_OPCODE_LT, &tmp1_dst, &zero,
         emit_instruction_op3(emit, VGPU10_OPCODE_MOVC, &inst->Dst[0], &tmp1_src,
                        }
         /**
   * Emit code for TGSI_OPCODE_ISSG (Integer Set Sign) instruction.
   */
   static bool
   emit_issg(struct svga_shader_emitter_v10 *emit,
         {
      /* dst.x = (src.x > 0) ? 1 : (src.x < 0) ? -1 : 0
   * dst.y = (src.y > 0) ? 1 : (src.y < 0) ? -1 : 0
   * dst.z = (src.z > 0) ? 1 : (src.z < 0) ? -1 : 0
   * dst.w = (src.w > 0) ? 1 : (src.w < 0) ? -1 : 0
   * Translates into:
   *   ILT tmp1, src, 0              tmp1 = src < 0 ? -1 : 0 (per component)
   *   ILT tmp2, 0, src              tmp2 = 0 < src ? -1 : 0 (per component)
   *   IADD dst, tmp1, neg(tmp2)     dst  = tmp1 - tmp2      (per component)
   */
            unsigned tmp1 = get_temp_index(emit);
   struct tgsi_full_src_register tmp1_src = make_src_temp_reg(tmp1);
            unsigned tmp2 = get_temp_index(emit);
   struct tgsi_full_src_register tmp2_src = make_src_temp_reg(tmp2);
                     emit_instruction_op2(emit, VGPU10_OPCODE_ILT, &tmp1_dst,
         emit_instruction_op2(emit, VGPU10_OPCODE_ILT, &tmp2_dst,
         emit_instruction_op2(emit, VGPU10_OPCODE_IADD, &inst->Dst[0],
                        }
         /**
   * Emit a comparison instruction.  The dest register will get
   * 0 or ~0 values depending on the outcome of comparing src0 to src1.
   */
   static void
   emit_comparison(struct svga_shader_emitter_v10 *emit,
                  SVGA3dCmpFunc func,
      {
      struct tgsi_full_src_register immediate;
   VGPU10OpcodeToken0 opcode0;
            /* Sanity checks for svga vs. gallium enums */
   STATIC_ASSERT(SVGA3D_CMP_LESS == (PIPE_FUNC_LESS + 1));
                     switch (func) {
   case SVGA3D_CMP_NEVER:
      immediate = make_immediate_reg_int(emit, 0);
   /* MOV dst, {0} */
   begin_emit_instruction(emit);
   emit_dword(emit, VGPU10_OPCODE_MOV);
   emit_dst_register(emit, dst);
   emit_src_register(emit, &immediate);
   end_emit_instruction(emit);
      case SVGA3D_CMP_ALWAYS:
      immediate = make_immediate_reg_int(emit, -1);
   /* MOV dst, {-1} */
   begin_emit_instruction(emit);
   emit_dword(emit, VGPU10_OPCODE_MOV);
   emit_dst_register(emit, dst);
   emit_src_register(emit, &immediate);
   end_emit_instruction(emit);
      case SVGA3D_CMP_LESS:
      opcode0.opcodeType = VGPU10_OPCODE_LT;
      case SVGA3D_CMP_EQUAL:
      opcode0.opcodeType = VGPU10_OPCODE_EQ;
      case SVGA3D_CMP_LESSEQUAL:
      opcode0.opcodeType = VGPU10_OPCODE_GE;
   swapSrc = true;
      case SVGA3D_CMP_GREATER:
      opcode0.opcodeType = VGPU10_OPCODE_LT;
   swapSrc = true;
      case SVGA3D_CMP_NOTEQUAL:
      opcode0.opcodeType = VGPU10_OPCODE_NE;
      case SVGA3D_CMP_GREATEREQUAL:
      opcode0.opcodeType = VGPU10_OPCODE_GE;
      default:
      assert(!"Unexpected comparison mode");
               begin_emit_instruction(emit);
   emit_dword(emit, opcode0.value);
   emit_dst_register(emit, dst);
   if (swapSrc) {
      emit_src_register(emit, src1);
      }
   else {
      emit_src_register(emit, src0);
      }
      }
         /**
   * Get texel/address offsets for a texture instruction.
   */
   static void
   get_texel_offsets(const struct svga_shader_emitter_v10 *emit,
         {
      if (inst->Texture.NumOffsets == 1) {
      /* According to OpenGL Shader Language spec the offsets are only
   * fetched from a previously-declared immediate/literal.
   */
   const struct tgsi_texture_offset *off = inst->TexOffsets;
   const unsigned index = off[0].Index;
   const unsigned swizzleX = off[0].SwizzleX;
   const unsigned swizzleY = off[0].SwizzleY;
   const unsigned swizzleZ = off[0].SwizzleZ;
                     offsets[0] = imm[swizzleX].Int;
   offsets[1] = imm[swizzleY].Int;
      }
   else {
            }
         /**
   * Set up the coordinate register for texture sampling.
   * When we're sampling from a RECT texture we have to scale the
   * unnormalized coordinate to a normalized coordinate.
   * We do that by multiplying the coordinate by an "extra" constant.
   * An alternative would be to use the RESINFO instruction to query the
   * texture's size.
   */
   static struct tgsi_full_src_register
   setup_texcoord(struct svga_shader_emitter_v10 *emit,
               {
      if (emit->key.tex[unit].sampler_view && emit->key.tex[unit].unnormalized) {
      unsigned scale_index = emit->texcoord_scale_index[unit];
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
            if (emit->key.tex[unit].texel_bias) {
      /* to fix texture coordinate rounding issue, 0.0001 offset is
   * been added. This fixes piglit test fbo-blit-scaled-linear. */
                  /* ADD tmp, coord, offset */
   emit_instruction_op2(emit, VGPU10_OPCODE_ADD, &tmp_dst,
         /* MUL tmp, tmp, scale */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL, &tmp_dst,
      }
   else {
      /* MUL tmp, coord, const[] */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL, &tmp_dst,
      }
      }
   else {
      /* use texcoord as-is */
         }
         /**
   * For SAMPLE_C instructions, emit the extra src register which indicates
   * the reference/comparision value.
   */
   static void
   emit_tex_compare_refcoord(struct svga_shader_emitter_v10 *emit,
               {
      struct tgsi_full_src_register coord_src_ref;
                     component = tgsi_util_get_shadow_ref_src_index(target) % 4;
                        }
         /**
   * Info for implementing texture swizzles.
   * The begin_tex_swizzle(), get_tex_swizzle_dst() and end_tex_swizzle()
   * functions use this to encapsulate the extra steps needed to perform
   * a texture swizzle, or shadow/depth comparisons.
   * The shadow/depth comparison is only done here if for the cases where
   * there's no VGPU10 opcode (like texture bias lookup w/ shadow compare).
   */
   struct tex_swizzle_info
   {
      bool swizzled;
   bool shadow_compare;
   unsigned unit;
   enum tgsi_texture_type texture_target;  /**< TGSI_TEXTURE_x */
   struct tgsi_full_src_register tmp_src;
   struct tgsi_full_dst_register tmp_dst;
   const struct tgsi_full_dst_register *inst_dst;
      };
         /**
   * Do setup for handling texture swizzles or shadow compares.
   * \param unit  the texture unit
   * \param inst  the TGSI texture instruction
   * \param shadow_compare  do shadow/depth comparison?
   * \param swz  returns the swizzle info
   */
   static void
   begin_tex_swizzle(struct svga_shader_emitter_v10 *emit,
                     unsigned unit,
   {
      swz->swizzled = (emit->key.tex[unit].swizzle_r != TGSI_SWIZZLE_X ||
                        swz->shadow_compare = shadow_compare;
            if (swz->swizzled || shadow_compare) {
      /* Allocate temp register for the result of the SAMPLE instruction
   * and the source of the MOV/compare/swizzle instructions.
   */
   unsigned tmp = get_temp_index(emit);
   swz->tmp_src = make_src_temp_reg(tmp);
               }
   swz->inst_dst = &inst->Dst[0];
               }
         /**
   * Returns the register to put the SAMPLE instruction results into.
   * This will either be the original instruction dst reg (if no swizzle
   * and no shadow comparison) or a temporary reg if there is a swizzle.
   */
   static const struct tgsi_full_dst_register *
   get_tex_swizzle_dst(const struct tex_swizzle_info *swz)
   {
      return (swz->swizzled || swz->shadow_compare)
      }
         /**
   * This emits the MOV instruction that actually implements a texture swizzle
   * and/or shadow comparison.
   */
   static void
   end_tex_swizzle(struct svga_shader_emitter_v10 *emit,
         {
      if (swz->shadow_compare) {
      /* Emit extra instructions to compare the fetched texel value against
   * a texture coordinate component.  The result of the comparison
   * is 0.0 or 1.0.
   */
   struct tgsi_full_src_register coord_src;
   struct tgsi_full_src_register texel_src =
         struct tgsi_full_src_register one =
         /* convert gallium comparison func to SVGA comparison func */
            int component =
         assert(component >= 0);
            /* COMPARE tmp, coord, texel */
   emit_comparison(emit, compare_func,
            /* AND dest, tmp, {1.0} */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_AND, false);
   if (swz->swizzled) {
         }
   else {
         }
   emit_src_register(emit, &swz->tmp_src);
   emit_src_register(emit, &one);
               if (swz->swizzled) {
      unsigned swz_r = emit->key.tex[swz->unit].swizzle_r;
   unsigned swz_g = emit->key.tex[swz->unit].swizzle_g;
   unsigned swz_b = emit->key.tex[swz->unit].swizzle_b;
   unsigned swz_a = emit->key.tex[swz->unit].swizzle_a;
   unsigned writemask_0 = 0, writemask_1 = 0;
            /* Swizzle w/out zero/one terms */
   struct tgsi_full_src_register src_swizzled =
      swizzle_src(&swz->tmp_src,
                           /* MOV dst, color(tmp).<swizzle> */
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV,
            /* handle swizzle zero terms */
   writemask_0 = (((swz_r == PIPE_SWIZZLE_0) << 0) |
                              if (writemask_0) {
      struct tgsi_full_src_register zero = int_tex ?
      make_immediate_reg_int(emit, 0) :
                     /* MOV dst.writemask_0, {0,0,0,0} */
               /* handle swizzle one terms */
   writemask_1 = (((swz_r == PIPE_SWIZZLE_1) << 0) |
                              if (writemask_1) {
      struct tgsi_full_src_register one = int_tex ?
      make_immediate_reg_int(emit, 1) :
                     /* MOV dst.writemask_1, {1,1,1,1} */
            }
         /**
   * Emit code for TGSI_OPCODE_SAMPLE instruction.
   */
   static bool
   emit_sample(struct svga_shader_emitter_v10 *emit,
         {
      const unsigned resource_unit = inst->Src[1].Register.Index;
   const unsigned sampler_unit = inst->Src[2].Register.Index;
   struct tgsi_full_src_register coord;
   int offsets[3];
                                       /* SAMPLE dst, coord(s0), resource, sampler */
            /* NOTE: for non-fragment shaders, we should use VGPU10_OPCODE_SAMPLE_L
   * with LOD=0.  But our virtual GPU accepts this as-is.
   */
   emit_sample_opcode(emit, VGPU10_OPCODE_SAMPLE,
         emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &coord);
   emit_resource_register(emit, resource_unit);
   emit_sampler_register(emit, sampler_unit);
                                 }
         /**
   * Check if a texture instruction is valid.
   * An example of an invalid texture instruction is doing shadow comparison
   * with an integer-valued texture.
   * If we detect an invalid texture instruction, we replace it with:
   *   MOV dst, {1,1,1,1};
   * \return TRUE if valid, FALSE if invalid.
   */
   static bool
   is_valid_tex_instruction(struct svga_shader_emitter_v10 *emit,
         {
      const unsigned unit = inst->Src[1].Register.Index;
   const enum tgsi_texture_type target = inst->Texture.Texture;
            if (tgsi_is_shadow_target(target) &&
      is_integer_type(emit->sampler_return_type[unit])) {
   debug_printf("Invalid SAMPLE_C with an integer texture!\n");
      }
            if (!valid) {
      /* emit a MOV dst, {1,1,1,1} instruction. */
   struct tgsi_full_src_register one = make_immediate_reg_float(emit, 1.0f);
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_MOV, false);
   emit_dst_register(emit, &inst->Dst[0]);
   emit_src_register(emit, &one);
                  }
         /**
   * Emit code for TGSI_OPCODE_TEX (simple texture lookup)
   */
   static bool
   emit_tex(struct svga_shader_emitter_v10 *emit,
         {
      const uint unit = inst->Src[1].Register.Index;
   const enum tgsi_texture_type target = inst->Texture.Texture;
   VGPU10_OPCODE_TYPE opcode;
   struct tgsi_full_src_register coord;
   int offsets[3];
   struct tex_swizzle_info swz_info;
            /* check that the sampler returns a float */
   if (!is_valid_tex_instruction(emit, inst))
            compare_in_shader = tgsi_is_shadow_target(target) &&
                                       /* SAMPLE dst, coord(s0), resource, sampler */
            if (tgsi_is_shadow_target(target) && !compare_in_shader)
         else
            emit_sample_opcode(emit, opcode, inst->Instruction.Saturate, offsets);
   emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &coord);
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
   if (opcode == VGPU10_OPCODE_SAMPLE_C) {
         }
                                 }
      /**
   * Emit code for TGSI_OPCODE_TG4 (texture lookup for texture gather)
   */
   static bool
   emit_tg4(struct svga_shader_emitter_v10 *emit,
         {
      const uint unit = inst->Src[2].Register.Index;
   struct tgsi_full_src_register src;
   struct tgsi_full_src_register offset_src, sampler, ref;
            /* check that the sampler returns a float */
   if (!is_valid_tex_instruction(emit, inst))
            if (emit->version >= 50) {
      unsigned target = inst->Texture.Texture;
   int index = inst->Src[1].Register.Index;
   const union tgsi_immediate_data *imm = emit->immediates[index];
   int select_comp  = imm[inst->Src[1].Register.SwizzleX].Int;
            if (!tgsi_is_shadow_target(target)) {
      switch (select_comp) {
   case 0:
      select_swizzle = emit->key.tex[unit].swizzle_r;
      case 1:
      select_swizzle = emit->key.tex[unit].swizzle_g;
      case 2:
      select_swizzle = emit->key.tex[unit].swizzle_b;
      case 3:
      select_swizzle = emit->key.tex[unit].swizzle_a;
      default:
            }
   else {
                  if (select_swizzle == PIPE_SWIZZLE_1) {
      src = make_immediate_reg_float(emit, 1.0);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &src);
      }
   else if (select_swizzle == PIPE_SWIZZLE_0) {
      src = make_immediate_reg_float(emit, 0.0);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &src);
                        /* GATHER4 dst, coord, resource, sampler */
   /* GATHER4_C dst, coord, resource, sampler ref */
   /* GATHER4_PO dst, coord, offset resource, sampler */
   /* GATHER4_PO_C dst, coord, offset resource, sampler, ref */
   begin_emit_instruction(emit);
   if (inst->Texture.NumOffsets == 1) {
      if (tgsi_is_shadow_target(target)) {
      emit_opcode(emit, VGPU10_OPCODE_GATHER4_PO_C,
      }
   else {
      emit_opcode(emit, VGPU10_OPCODE_GATHER4_PO,
         }
   else {
      if (tgsi_is_shadow_target(target)) {
      emit_opcode(emit, VGPU10_OPCODE_GATHER4_C,
      }
   else {
      emit_opcode(emit, VGPU10_OPCODE_GATHER4,
                  emit_dst_register(emit, &inst->Dst[0]);
   emit_src_register(emit, &src);
   if (inst->Texture.NumOffsets == 1) {
      /* offset */
   offset_src = make_src_reg(inst->TexOffsets[0].File,
         offset_src = swizzle_src(&offset_src, inst->TexOffsets[0].SwizzleX,
                                 /* resource */
            /* sampler */
   sampler = make_src_reg(TGSI_FILE_SAMPLER,
         sampler.Register.SwizzleX =
   sampler.Register.SwizzleY =
   sampler.Register.SwizzleZ =
   sampler.Register.SwizzleW = select_swizzle;
            if (tgsi_is_shadow_target(target)) {
      /* ref */
   if (target == TGSI_TEXTURE_SHADOWCUBE_ARRAY) {
      ref = scalar_src(&inst->Src[1], TGSI_SWIZZLE_X);
      }
   else {
                     end_emit_instruction(emit);
      }
   else {
      /* Only a single channel is supported in SM4_1 and we report
   * PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS = 1.
   * Only the 0th component will be gathered.
   */
   switch (emit->key.tex[unit].swizzle_r) {
   case PIPE_SWIZZLE_X:
                     /* Gather dst, coord, resource, sampler */
   begin_emit_instruction(emit);
   emit_sample_opcode(emit, VGPU10_OPCODE_GATHER4,
         emit_dst_register(emit, &inst->Dst[0]);
                  /* sampler */
   sampler = make_src_reg(TGSI_FILE_SAMPLER,
         sampler.Register.SwizzleX =
   sampler.Register.SwizzleY =
   sampler.Register.SwizzleZ =
                  end_emit_instruction(emit);
      case PIPE_SWIZZLE_W:
   case PIPE_SWIZZLE_1:
      src = make_immediate_reg_float(emit, 1.0);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &src);
      case PIPE_SWIZZLE_Y:
   case PIPE_SWIZZLE_Z:
   case PIPE_SWIZZLE_0:
   default:
      src = make_immediate_reg_float(emit, 0.0);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &src);
                     }
            /**
   * Emit code for TGSI_OPCODE_TEX2 (texture lookup for shadow cube map arrays)
   */
   static bool
   emit_tex2(struct svga_shader_emitter_v10 *emit,
         {
      const uint unit = inst->Src[2].Register.Index;
   unsigned target = inst->Texture.Texture;
   struct tgsi_full_src_register coord, ref;
   int offsets[3];
   struct tex_swizzle_info swz_info;
   VGPU10_OPCODE_TYPE opcode;
            /* check that the sampler returns a float */
   if (!is_valid_tex_instruction(emit, inst))
            compare_in_shader = emit->key.tex[unit].compare_in_shader;
   if (compare_in_shader)
         else
                              coord = setup_texcoord(emit, unit, &inst->Src[0]);
            /* SAMPLE_C dst, coord, resource, sampler, ref */
   begin_emit_instruction(emit);
   emit_sample_opcode(emit, opcode,
         emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &coord);
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
   if (opcode == VGPU10_OPCODE_SAMPLE_C) {
         }
                                 }
         /**
   * Emit code for TGSI_OPCODE_TXP (projective texture)
   */
   static bool
   emit_txp(struct svga_shader_emitter_v10 *emit,
         {
      const uint unit = inst->Src[1].Register.Index;
   const enum tgsi_texture_type target = inst->Texture.Texture;
   VGPU10_OPCODE_TYPE opcode;
   int offsets[3];
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register src0_wwww =
         struct tgsi_full_src_register coord;
   struct tex_swizzle_info swz_info;
            /* check that the sampler returns a float */
   if (!is_valid_tex_instruction(emit, inst))
            compare_in_shader = tgsi_is_shadow_target(target) &&
                                       /* DIV tmp, coord, coord.wwww */
   emit_instruction_op2(emit, VGPU10_OPCODE_DIV, &tmp_dst,
            /* SAMPLE dst, coord(tmp), resource, sampler */
            if (tgsi_is_shadow_target(target) && !compare_in_shader)
      /* NOTE: for non-fragment shaders, we should use
   * VGPU10_OPCODE_SAMPLE_C_LZ, but our virtual GPU accepts this as-is.
   */
      else
            emit_sample_opcode(emit, opcode, inst->Instruction.Saturate, offsets);
   emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &tmp_src);  /* projected coord */
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
   if (opcode == VGPU10_OPCODE_SAMPLE_C) {
         }
                                 }
         /**
   * Emit code for TGSI_OPCODE_TXD (explicit derivatives)
   */
   static bool
   emit_txd(struct svga_shader_emitter_v10 *emit,
         {
      const uint unit = inst->Src[3].Register.Index;
   const enum tgsi_texture_type target = inst->Texture.Texture;
   int offsets[3];
   struct tgsi_full_src_register coord;
            begin_tex_swizzle(emit, unit, inst, tgsi_is_shadow_target(target),
                              /* SAMPLE_D dst, coord(s0), resource, sampler, Xderiv(s1), Yderiv(s2) */
   begin_emit_instruction(emit);
   emit_sample_opcode(emit, VGPU10_OPCODE_SAMPLE_D,
         emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &coord);
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
   emit_src_register(emit, &inst->Src[1]);  /* Xderiv */
   emit_src_register(emit, &inst->Src[2]);  /* Yderiv */
                                 }
         /**
   * Emit code for TGSI_OPCODE_TXF (texel fetch)
   */
   static bool
   emit_txf(struct svga_shader_emitter_v10 *emit,
         {
      const uint unit = inst->Src[1].Register.Index;
   const bool msaa = tgsi_is_msaa_target(inst->Texture.Texture)
         int offsets[3];
                              if (msaa) {
               /* Fetch one sample from an MSAA texture */
   struct tgsi_full_src_register sampleIndex =
         /* LD_MS dst, coord(s0), resource, sampleIndex */
   begin_emit_instruction(emit);
   emit_sample_opcode(emit, VGPU10_OPCODE_LD_MS,
         emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &inst->Src[0]);
   emit_resource_register(emit, unit);
   emit_src_register(emit, &sampleIndex);
      }
   else {
      /* Fetch one texel specified by integer coordinate */
   /* LD dst, coord(s0), resource */
   begin_emit_instruction(emit);
   emit_sample_opcode(emit, VGPU10_OPCODE_LD,
         emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &inst->Src[0]);
   emit_resource_register(emit, unit);
                                    }
         /**
   * Emit code for TGSI_OPCODE_TXL (explicit LOD) or TGSI_OPCODE_TXB (LOD bias)
   * or TGSI_OPCODE_TXB2 (for cube shadow maps).
   */
   static bool
   emit_txl_txb(struct svga_shader_emitter_v10 *emit,
         {
      const enum tgsi_texture_type target = inst->Texture.Texture;
   VGPU10_OPCODE_TYPE opcode;
   unsigned unit;
   int offsets[3];
   struct tgsi_full_src_register coord, lod_bias;
            assert(inst->Instruction.Opcode == TGSI_OPCODE_TXL ||
                  if (inst->Instruction.Opcode == TGSI_OPCODE_TXB2) {
      lod_bias = scalar_src(&inst->Src[1], TGSI_SWIZZLE_X);
      }
   else {
      lod_bias = scalar_src(&inst->Src[0], TGSI_SWIZZLE_W);
               begin_tex_swizzle(emit, unit, inst, tgsi_is_shadow_target(target),
                              /* SAMPLE_L/B dst, coord(s0), resource, sampler, lod(s3) */
   begin_emit_instruction(emit);
   if (inst->Instruction.Opcode == TGSI_OPCODE_TXL) {
         }
   else {
         }
   emit_sample_opcode(emit, opcode, inst->Instruction.Saturate, offsets);
   emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &coord);
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
   emit_src_register(emit, &lod_bias);
                                 }
         /**
   * Emit code for TGSI_OPCODE_TXL2 (explicit LOD) for cubemap array.
   */
   static bool
   emit_txl2(struct svga_shader_emitter_v10 *emit,
         {
      unsigned target = inst->Texture.Texture;
   unsigned opcode, unit;
   int offsets[3];
   struct tgsi_full_src_register coord, lod;
                     lod = scalar_src(&inst->Src[1], TGSI_SWIZZLE_X);
            begin_tex_swizzle(emit, unit, inst, tgsi_is_shadow_target(target),
                              /* SAMPLE_L dst, coord(s0), resource, sampler, lod(s3) */
   begin_emit_instruction(emit);
   opcode = VGPU10_OPCODE_SAMPLE_L;
   emit_sample_opcode(emit, opcode, inst->Instruction.Saturate, offsets);
   emit_dst_register(emit, get_tex_swizzle_dst(&swz_info));
   emit_src_register(emit, &coord);
   emit_resource_register(emit, unit);
   emit_sampler_register(emit, unit);
   emit_src_register(emit, &lod);
                                 }
         /**
   * Emit code for TGSI_OPCODE_TXQ (texture query) instruction.
   */
   static bool
   emit_txq(struct svga_shader_emitter_v10 *emit,
         {
               if (emit->key.tex[unit].target == PIPE_BUFFER) {
      /* RESINFO does not support querying texture buffers, so we instead
   * store texture buffer sizes in shader constants, then copy them to
   * implement TXQ instead of emitting RESINFO.
   * MOV dst, const[texture_buffer_size_index[unit]]
   */
   struct tgsi_full_src_register size_src =
            } else {
      /* RESINFO dst, srcMipLevel, resource */
   begin_emit_instruction(emit);
   emit_opcode_resinfo(emit, VGPU10_RESINFO_RETURN_UINT);
   emit_dst_register(emit, &inst->Dst[0]);
   emit_src_register(emit, &inst->Src[0]);
   emit_resource_register(emit, unit);
                           }
         /**
   * Does this opcode produce a double-precision result?
   * XXX perhaps move this to a TGSI utility.
   */
   static bool
   opcode_has_dbl_dst(unsigned opcode)
   {
      switch (opcode) {
   case TGSI_OPCODE_F2D:
   case TGSI_OPCODE_DABS:
   case TGSI_OPCODE_DADD:
   case TGSI_OPCODE_DFRAC:
   case TGSI_OPCODE_DMAX:
   case TGSI_OPCODE_DMIN:
   case TGSI_OPCODE_DMUL:
   case TGSI_OPCODE_DNEG:
   case TGSI_OPCODE_I2D:
   case TGSI_OPCODE_U2D:
   case TGSI_OPCODE_DFMA:
      // XXX more TBD
      default:
            }
         /**
   * Does this opcode use double-precision source registers?
   */
   static bool
   opcode_has_dbl_src(unsigned opcode)
   {
      switch (opcode) {
   case TGSI_OPCODE_D2F:
   case TGSI_OPCODE_DABS:
   case TGSI_OPCODE_DADD:
   case TGSI_OPCODE_DFRAC:
   case TGSI_OPCODE_DMAX:
   case TGSI_OPCODE_DMIN:
   case TGSI_OPCODE_DMUL:
   case TGSI_OPCODE_DNEG:
   case TGSI_OPCODE_D2I:
   case TGSI_OPCODE_D2U:
   case TGSI_OPCODE_DFMA:
   case TGSI_OPCODE_DSLT:
   case TGSI_OPCODE_DSGE:
   case TGSI_OPCODE_DSEQ:
   case TGSI_OPCODE_DSNE:
   case TGSI_OPCODE_DRCP:
   case TGSI_OPCODE_DSQRT:
   case TGSI_OPCODE_DMAD:
   case TGSI_OPCODE_DLDEXP:
   case TGSI_OPCODE_DRSQ:
   case TGSI_OPCODE_DTRUNC:
   case TGSI_OPCODE_DCEIL:
   case TGSI_OPCODE_DFLR:
   case TGSI_OPCODE_DROUND:
   case TGSI_OPCODE_DSSG:
         default:
            }
         /**
   * Check that the swizzle for reading from a double-precision register
   * is valid. If not valid, move the source to a temporary register first.
   */
   static struct tgsi_full_src_register
   check_double_src(struct svga_shader_emitter_v10 *emit,
         {
               if (((reg->Register.SwizzleX == PIPE_SWIZZLE_X &&
            (reg->Register.SwizzleX == PIPE_SWIZZLE_Z &&
         ((reg->Register.SwizzleZ == PIPE_SWIZZLE_X &&
         (reg->Register.SwizzleZ == PIPE_SWIZZLE_Z &&
            } else {
      /* move the src to a temporary to fix the swizzle */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &tmp_dst, reg);
               }
      }
      /**
   * Check that the writemask for a double-precision instruction is valid.
   */
   static void
   check_double_dst_writemask(const struct tgsi_full_instruction *inst)
   {
               switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_DABS:
   case TGSI_OPCODE_DADD:
   case TGSI_OPCODE_DFRAC:
   case TGSI_OPCODE_DNEG:
   case TGSI_OPCODE_DMAD:
   case TGSI_OPCODE_DMAX:
   case TGSI_OPCODE_DMIN:
   case TGSI_OPCODE_DMUL:
   case TGSI_OPCODE_DRCP:
   case TGSI_OPCODE_DSQRT:
   case TGSI_OPCODE_F2D:
   case TGSI_OPCODE_DFMA:
      assert(writemask == TGSI_WRITEMASK_XYZW ||
         writemask == TGSI_WRITEMASK_XY ||
      case TGSI_OPCODE_DSEQ:
   case TGSI_OPCODE_DSGE:
   case TGSI_OPCODE_DSNE:
   case TGSI_OPCODE_DSLT:
   case TGSI_OPCODE_D2I:
   case TGSI_OPCODE_D2U:
      /* Write to 1 or 2 components only */
   assert(util_bitcount(writemask) <= 2);
      default:
      /* XXX this list may be incomplete */
         }
         /**
   * Double-precision absolute value.
   */
   static bool
   emit_dabs(struct svga_shader_emitter_v10 *emit,
         {
               struct tgsi_full_src_register src = check_double_src(emit, &inst->Src[0]);
                     /* DMOV dst, |src| */
            free_temp_indexes(emit);
      }
         /**
   * Double-precision negation
   */
   static bool
   emit_dneg(struct svga_shader_emitter_v10 *emit,
         {
      assert(emit->version >= 50);
   struct tgsi_full_src_register src = check_double_src(emit, &inst->Src[0]);
                     /* DMOV dst, -src */
            free_temp_indexes(emit);
      }
         /**
   * SM5 has no DMAD opcode.  Implement negation with DMUL/DADD.
   */
   static bool
   emit_dmad(struct svga_shader_emitter_v10 *emit,
         {
      assert(emit->version >= 50);
   struct tgsi_full_src_register src0 = check_double_src(emit, &inst->Src[0]);
   struct tgsi_full_src_register src1 = check_double_src(emit, &inst->Src[1]);
   struct tgsi_full_src_register src2 = check_double_src(emit, &inst->Src[2]);
            unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
            /* DMUL tmp, src[0], src[1] */
   emit_instruction_opn(emit, VGPU10_OPCODE_DMUL,
                  /* DADD dst, tmp, src[2] */
   emit_instruction_opn(emit, VGPU10_OPCODE_DADD,
                           }
         /**
   * Double precision reciprocal square root
   */
   static bool
   emit_drsq(struct svga_shader_emitter_v10 *emit,
               {
               VGPU10OpcodeToken0 token0;
                     token0.value = 0;
   token0.opcodeType = VGPU10_OPCODE_VMWARE;
   token0.vmwareOpcodeType = VGPU10_VMWARE_OPCODE_DRSQ;
   emit_dword(emit, token0.value);
   emit_dst_register(emit, dst);
   emit_src_register(emit, &dsrc);
                        }
         /**
   * There is no SM5 opcode for double precision square root.
   * It will be implemented with DRSQ.
   * dst = src * DRSQ(src)
   */
   static bool
   emit_dsqrt(struct svga_shader_emitter_v10 *emit,
         {
                        /* temporary register to hold the source */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
            /* temporary register to hold the DEQ result */
   unsigned tmp_cond = get_temp_index(emit);
   struct tgsi_full_dst_register tmp_cond_dst = make_dst_temp_reg(tmp_cond);
   struct tgsi_full_dst_register tmp_cond_dst_xy =
         struct tgsi_full_src_register tmp_cond_src = make_src_temp_reg(tmp_cond);
   struct tgsi_full_src_register tmp_cond_src_xy =
         swizzle_src(&tmp_cond_src,
            /* The reciprocal square root of zero yields INF.
   * So if the source is 0, we replace it with 1 in the tmp register.
   * The later multiplication of zero in the original source will yield 0
   * in the result.
            /* tmp1 = (src == 0) ? 1 : src;
   *   EQ tmp1, 0, src
   *   MOVC tmp, tmp1, 1.0, src
   */
   struct tgsi_full_src_register zero =
            struct tgsi_full_src_register one =
            emit_instruction_op2(emit, VGPU10_OPCODE_DEQ, &tmp_cond_dst_xy,
         emit_instruction_op3(emit, VGPU10_OPCODE_DMOVC, &tmp_dst,
            struct tgsi_full_dst_register tmp_rsq_dst = make_dst_temp_reg(tmp);
            /* DRSQ tmp_rsq, tmp */
            /* DMUL dst, tmp_rsq, src[0] */
   emit_instruction_op2(emit, VGPU10_OPCODE_DMUL, &inst->Dst[0],
                        }
         /**
   * glsl-nir path does not lower DTRUNC, so we need to
   * add the translation here.
   *
   * frac = DFRAC(src)
   * tmp = src - frac
   * dst = src >= 0 ? tmp : (tmp + (frac==0 ? 0 : 1))
   */
   static bool
   emit_dtrunc(struct svga_shader_emitter_v10 *emit,
         {
                        /* frac = DFRAC(src) */
   unsigned frac_index = get_temp_index(emit);
   struct tgsi_full_dst_register frac_dst = make_dst_temp_reg(frac_index);
            VGPU10OpcodeToken0 token0;
   begin_emit_instruction(emit);
   token0.value = 0;
   token0.opcodeType = VGPU10_OPCODE_VMWARE;
   token0.vmwareOpcodeType = VGPU10_VMWARE_OPCODE_DFRC;
   emit_dword(emit, token0.value);
   emit_dst_register(emit, &frac_dst);
   emit_src_register(emit, &src);
            /* tmp = src - frac */
   unsigned tmp_index = get_temp_index(emit);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp_index);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp_index);
   struct tgsi_full_src_register negate_frac_src = negate_src(&frac_src);
   emit_instruction_opn(emit, VGPU10_OPCODE_DADD,
                  /* cond = frac==0 */
   unsigned cond_index = get_temp_index(emit);
   struct tgsi_full_dst_register cond_dst = make_dst_temp_reg(cond_index);
   struct tgsi_full_src_register cond_src = make_src_temp_reg(cond_index);
   struct tgsi_full_src_register zero =
            /* Only use one or two components for double opcode */
            emit_instruction_opn(emit, VGPU10_OPCODE_DEQ,
                  /* tmp2 = cond ? 0 : 1 */
   unsigned tmp2_index = get_temp_index(emit);
   struct tgsi_full_dst_register tmp2_dst = make_dst_temp_reg(tmp2_index);
   struct tgsi_full_src_register tmp2_src = make_src_temp_reg(tmp2_index);
   struct tgsi_full_src_register cond_src_xy =
      swizzle_src(&cond_src, PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y,
      struct tgsi_full_src_register one =
            emit_instruction_opn(emit, VGPU10_OPCODE_DMOVC,
                  /* tmp2 = tmp + tmp2 */
   emit_instruction_opn(emit, VGPU10_OPCODE_DADD,
                  /* cond = src>=0 */
   emit_instruction_opn(emit, VGPU10_OPCODE_DGE,
                  /* dst = cond ? tmp : tmp2 */
   emit_instruction_opn(emit, VGPU10_OPCODE_DMOVC,
                  free_temp_indexes(emit);
      }
         static bool
   emit_interp_offset(struct svga_shader_emitter_v10 *emit,
         {
               /* The src1.xy offset is a float with values in the range [-0.5, 0.5]
   * where (0,0) is the center of the pixel.  We need to translate that
   * into an integer offset on a 16x16 grid in the range [-8/16, 7/16].
   * Also need to flip the Y axis (I think).
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_dst_register tmp_dst_xy =
         struct tgsi_full_src_register const16 =
            /* MUL tmp.xy, src1, {16, -16, 0, 0} */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL,
            /* FTOI tmp.xy, tmp */
            /* EVAL_SNAPPED dst, src0, tmp */
   emit_instruction_op2(emit, VGPU10_OPCODE_EVAL_SNAPPED,
                        }
         /**
   * Emit a simple instruction (like ADD, MUL, MIN, etc).
   */
   static bool
   emit_simple(struct svga_shader_emitter_v10 *emit,
         {
      const enum tgsi_opcode opcode = inst->Instruction.Opcode;
   const struct tgsi_opcode_info *op = tgsi_get_opcode_info(opcode);
   const bool dbl_dst = opcode_has_dbl_dst(inst->Instruction.Opcode);
   const bool dbl_src = opcode_has_dbl_src(inst->Instruction.Opcode);
                     if (inst->Instruction.Opcode == TGSI_OPCODE_BGNLOOP) {
         }
   else if (inst->Instruction.Opcode == TGSI_OPCODE_ENDLOOP) {
                  for (i = 0; i < op->num_src; i++) {
      if (dbl_src)
         else
               begin_emit_instruction(emit);
   emit_opcode_precise(emit, translate_opcode(inst->Instruction.Opcode),
               for (i = 0; i < op->num_dst; i++) {
      if (dbl_dst) {
         }
      }
   for (i = 0; i < op->num_src; i++) {
         }
            free_temp_indexes(emit);
      }
         /**
   * Emit MSB instruction (like IMSB, UMSB).
   *
   * GLSL returns the index starting from the LSB;
   * whereas in SM5, firstbit_hi/shi returns the index starting from the MSB.
   * To get correct location as per glsl from SM5 device, we should
   * return (31 - index) if returned index is not -1.
   */
   static bool
   emit_msb(struct svga_shader_emitter_v10 *emit,
         {
                        struct tgsi_full_src_register index_src =
         struct tgsi_full_src_register imm31 =
         imm31 = scalar_src(&imm31, TGSI_SWIZZLE_X);
   struct tgsi_full_src_register neg_one =
         neg_one = scalar_src(&neg_one, TGSI_SWIZZLE_X);
   unsigned tmp = get_temp_index(emit);
   const struct tgsi_full_dst_register tmp_dst =
         const struct tgsi_full_dst_register tmp_dst_x =
         const struct tgsi_full_src_register tmp_src_x =
         int writemask = TGSI_WRITEMASK_X;
   int src_swizzle = TGSI_SWIZZLE_X;
                     /* index conversion from SM5 to GLSL */
   while (writemask & dst_writemask) {
      struct tgsi_full_src_register index_src_comp =
         struct tgsi_full_dst_register index_dst_comp =
            /* check if index_src_comp != -1 */
   emit_instruction_op2(emit, VGPU10_OPCODE_INE,
            /* if */
            index_src_comp = negate_src(&index_src_comp);
   /* SUB DST, IMM{31}, DST */
   emit_instruction_op2(emit, VGPU10_OPCODE_IADD,
            /* endif */
            writemask = writemask << 1;
      }
   free_temp_indexes(emit);
      }
         /**
   * Emit a BFE instruction (like UBFE, IBFE).
   * tgsi representation:
   * U/IBFE dst, value, offset, width
   * SM5 representation:
   * U/IBFE dst, width, offset, value
   * Note: SM5 has width & offset range (0-31);
   *      whereas GLSL has width & offset range (0-32)
   */
   static bool
   emit_bfe(struct svga_shader_emitter_v10 *emit,
         {
      const enum tgsi_opcode opcode = inst->Instruction.Opcode;
   struct tgsi_full_src_register imm32 = make_immediate_reg_int(emit, 32);
   imm32 = scalar_src(&imm32, TGSI_SWIZZLE_X);
   struct tgsi_full_src_register zero = make_immediate_reg_int(emit, 0);
            unsigned tmp1 = get_temp_index(emit);
   const struct tgsi_full_dst_register cond1_dst = make_dst_temp_reg(tmp1);
   const struct tgsi_full_dst_register cond1_dst_x =
         const struct tgsi_full_src_register cond1_src_x =
            unsigned tmp2 = get_temp_index(emit);
   const struct tgsi_full_dst_register cond2_dst = make_dst_temp_reg(tmp2);
   const struct tgsi_full_dst_register cond2_dst_x =
         const struct tgsi_full_src_register cond2_src_x =
            /**
   * In SM5, when width = 32  and offset = 0, it returns 0.
   * On the other hand GLSL, expects value to be copied as it is, to dst.
            /* cond1 = width ! = 32 */
   emit_instruction_op2(emit, VGPU10_OPCODE_IEQ,
            /* cond2 = offset ! = 0 */
   emit_instruction_op2(emit, VGPU10_OPCODE_IEQ,
            /* cond 2 = cond1 & cond 2 */
   emit_instruction_op2(emit, VGPU10_OPCODE_AND, &cond2_dst_x,
               /* IF */
            emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0],
            /* ELSE */
            /* U/IBFE dst, width, offset, value */
   emit_instruction_op3(emit, translate_opcode(opcode), &inst->Dst[0],
            /* ENDIF */
            free_temp_indexes(emit);
      }
         /**
   * Emit BFI  instruction
   * tgsi representation:
   * BFI dst, base, insert, offset, width
   * SM5 representation:
   * BFI dst, width, offset, insert, base
   * Note: SM5 has width & offset range (0-31);
   *      whereas GLSL has width & offset range (0-32)
   */
   static bool
   emit_bfi(struct svga_shader_emitter_v10 *emit,
         {
      const enum tgsi_opcode opcode = inst->Instruction.Opcode;
   struct tgsi_full_src_register imm32 = make_immediate_reg_int(emit, 32);
            struct tgsi_full_src_register zero = make_immediate_reg_int(emit, 0);
            unsigned tmp1 = get_temp_index(emit);
   const struct tgsi_full_dst_register cond1_dst = make_dst_temp_reg(tmp1);
   const struct tgsi_full_dst_register cond1_dst_x =
         const struct tgsi_full_src_register cond1_src_x =
            unsigned tmp2 = get_temp_index(emit);
   const struct tgsi_full_dst_register cond2_dst = make_dst_temp_reg(tmp2);
   const struct tgsi_full_dst_register cond2_dst_x =
         const struct tgsi_full_src_register cond2_src_x =
            /**
   * In SM5, when width = 32  and offset = 0, it returns 0.
   * On the other hand GLSL, expects insert to be copied as it is, to dst.
            /* cond1 = width == 32 */
   emit_instruction_op2(emit, VGPU10_OPCODE_IEQ,
            /* cond1 = offset == 0 */
   emit_instruction_op2(emit, VGPU10_OPCODE_IEQ,
            /* cond2 = cond1 & cond2 */
   emit_instruction_op2(emit, VGPU10_OPCODE_AND,
            /* if */
            emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0],
            /* else */
            /* BFI dst, width, offset, insert, base */
   begin_emit_instruction(emit);
   emit_opcode(emit, translate_opcode(opcode), inst->Instruction.Saturate);
   emit_dst_register(emit, &inst->Dst[0]);
   emit_src_register(emit, &inst->Src[3]);
   emit_src_register(emit, &inst->Src[2]);
   emit_src_register(emit, &inst->Src[1]);
   emit_src_register(emit, &inst->Src[0]);
            /* endif */
            free_temp_indexes(emit);
      }
         /**
   * We only special case the MOV instruction to try to detect constant
   * color writes in the fragment shader.
   */
   static bool
   emit_mov(struct svga_shader_emitter_v10 *emit,
         {
      const struct tgsi_full_src_register *src = &inst->Src[0];
            if (emit->unit == PIPE_SHADER_FRAGMENT &&
      dst->Register.File == TGSI_FILE_OUTPUT &&
   dst->Register.Index == 0 &&
   src->Register.File == TGSI_FILE_CONSTANT &&
   !src->Register.Indirect) {
                  }
         /**
   * Emit a simple VGPU10 instruction which writes to multiple dest registers,
   * where TGSI only uses one dest register.
   */
   static bool
   emit_simple_1dst(struct svga_shader_emitter_v10 *emit,
                     {
      const enum tgsi_opcode opcode = inst->Instruction.Opcode;
   const struct tgsi_opcode_info *op = tgsi_get_opcode_info(opcode);
            begin_emit_instruction(emit);
            for (i = 0; i < dst_count; i++) {
      if (i == dst_index) {
         } else {
                     for (i = 0; i < op->num_src; i++) {
         }
               }
         /**
   * Emit a vmware specific VGPU10 instruction.
   */
   static bool
   emit_vmware(struct svga_shader_emitter_v10 *emit,
               {
      VGPU10OpcodeToken0 token0;
   const enum tgsi_opcode opcode = inst->Instruction.Opcode;
   const struct tgsi_opcode_info *op = tgsi_get_opcode_info(opcode);
   const bool dbl_dst = opcode_has_dbl_dst(inst->Instruction.Opcode);
   const bool dbl_src = opcode_has_dbl_src(inst->Instruction.Opcode);
   unsigned i;
            for (i = 0; i < op->num_src; i++) {
      if (dbl_src)
         else
                                 token0.value = 0;
   token0.opcodeType = VGPU10_OPCODE_VMWARE;
   token0.vmwareOpcodeType = subopcode;
            if (subopcode == VGPU10_VMWARE_OPCODE_IDIV) {
      /* IDIV only uses the first dest register. */
   emit_dst_register(emit, &inst->Dst[0]);
      } else {
      for (i = 0; i < op->num_dst; i++) {
      if (dbl_dst) {
         }
                  for (i = 0; i < op->num_src; i++) {
         }
            free_temp_indexes(emit);
      }
      /**
   * Emit a memory register
   */
      typedef enum {
      MEM_STORE = 0,
   MEM_LOAD = 1,
      } memory_op;
      static void
   emit_memory_register(struct svga_shader_emitter_v10 *emit,
                     {
      VGPU10OperandToken0 operand0;
            operand0.value = 0;
   operand0.operandType = VGPU10_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            switch (mem_op) {
   case MEM_ATOMIC_COUNTER:
   {
      operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
   resIndex = inst->Src[regIndex].Register.Index;
      }
   case MEM_STORE:
   {
               operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_MASK_MODE;
   operand0.mask = writemask;
   resIndex = reg->Register.Index;
      }
   case MEM_LOAD:
   {
               operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SWIZZLE_MODE;
   operand0.swizzleX = reg->Register.SwizzleX;
   operand0.swizzleY = reg->Register.SwizzleY;
   operand0.swizzleZ = reg->Register.SwizzleZ;
   operand0.swizzleW = reg->Register.SwizzleW;
   resIndex = reg->Register.Index;
      }
   default:
      assert(!"Unexpected memory opcode");
               emit_dword(emit, operand0.value);
      }
         typedef enum {
      UAV_STORE = 0,
   UAV_LOAD = 1,
   UAV_ATOMIC = 2,
      } UAV_OP;
         /**
   * Emit a uav register
   * \param uav_index     index of resource register
   * \param uav_op        UAV_STORE/ UAV_LOAD/ UAV_ATOMIC depending on opcode
   * \param resourceType  resource file type
   * \param writemask     resource writemask
   */
      static void
   emit_uav_register(struct svga_shader_emitter_v10 *emit,
               {
      VGPU10OperandToken0 operand0;
            operand0.value = 0;
   operand0.operandType = VGPU10_OPERAND_TYPE_UAV;
   operand0.indexDimension = VGPU10_OPERAND_INDEX_1D;
            switch (resourceType) {
   case TGSI_FILE_IMAGE:
      uav_index = emit->key.images[res_index].uav_index;
      case TGSI_FILE_BUFFER:
      uav_index = emit->key.shader_buf_uav_index[res_index];
      case TGSI_FILE_HW_ATOMIC:
      uav_index = emit->key.atomic_buf_uav_index[res_index];
      default:
                  switch (uav_op) {
   case UAV_ATOMIC:
      operand0.numComponents = VGPU10_OPERAND_0_COMPONENT;
         case UAV_STORE:
      operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_MASK_MODE;
   operand0.mask = writemask;
         case UAV_LOAD:
   case UAV_RESQ:
      operand0.selectionMode = VGPU10_OPERAND_4_COMPONENT_SWIZZLE_MODE;
   operand0.swizzleX = VGPU10_COMPONENT_X;
   operand0.swizzleY = VGPU10_COMPONENT_Y;
   operand0.swizzleZ = VGPU10_COMPONENT_Z;
   operand0.swizzleW = VGPU10_COMPONENT_W;
         default:
                  emit_dword(emit, operand0.value);
      }
         /**
   * A helper function to emit the uav address.
   * For memory, buffer, and image resource, it is set to the specified address.
   * For HW atomic counter, the address is the sum of the address offset and the
   * offset into the HW atomic buffer as specified by the register index.
   * It is also possible to specify the counter index as an indirect address.
   * And in this case, the uav address will be the sum of the address offset and the
   * counter index specified in the indirect address.
   */
   static
   struct tgsi_full_src_register
   emit_uav_addr_offset(struct svga_shader_emitter_v10 *emit,
                        enum tgsi_file_type resourceType,
      {
      unsigned addr_tmp;
   struct tgsi_full_dst_register addr_dst;
   struct tgsi_full_src_register addr_src;
   struct tgsi_full_src_register two = make_immediate_reg_int(emit, 2);
            addr_tmp = get_temp_index(emit);
   addr_dst = make_dst_temp_reg(addr_tmp);
            /* specified address offset */
   if (addr_reg)
         else
            /* For HW atomic counter, we need to find the index to the
   * HW atomic buffer.
   */
   if (resourceType == TGSI_FILE_HW_ATOMIC) {
                  /**
   * uav addr offset  = counter layout offset +
                  /* counter layout offset */
   struct tgsi_full_src_register layout_offset;
                  /* counter layout offset + address offset */
                  /* counter indirect index address */
                                          /* counter layout offset + address offset + counter indirect address */
               } else {
                        /* uav addr offset  = counter index address + address offset */
   emit_instruction_op2(emit, VGPU10_OPCODE_ADD, &addr_dst,
               /* HW atomic buffer is declared as raw buffer, so the buffer address is
   * the byte offset, so we need to multiple the counter addr offset by 4.
   */
   emit_instruction_op2(emit, VGPU10_OPCODE_ISHL, &addr_dst,
      }
   else if (resourceType == TGSI_FILE_IMAGE) {
      if ((emit->key.images[resourceIndex].resource_target == PIPE_TEXTURE_3D)
                              /* For non-layered 3D texture image view, we have to make sure the z
   * component of the address offset is set to 0.
   */
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &addr_dst_z,
                     }
            /**
   * A helper function to expand indirect indexing to uav resource
   * by looping through the resource array, compare the indirect index and
   * emit the instruction for each resource in the array.
   */
   static void
   loop_instruction(unsigned index, unsigned count,
                  struct tgsi_full_src_register *addr_index,
   void (*fb)(struct svga_shader_emitter_v10 *,
      {
      if (count == 0)
            if (index > 0) {
      /* ELSE */
               struct tgsi_full_src_register index_src =
            unsigned tmp_index = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp_index);
   struct tgsi_full_src_register tmp_src_x =
                  /* IEQ tmp, addr_tmp_index, index */
   emit_instruction_op2(emit, VGPU10_OPCODE_IEQ, &tmp_dst,
            /* IF tmp */
                                       /* ENDIF */
      }
         /**
   * A helper function to emit the load instruction.
   */
   static void
   emit_load_instruction(struct svga_shader_emitter_v10 *emit,
               {
      VGPU10OpcodeToken0 token0;
   struct tgsi_full_src_register addr_src;
            /* Resolve the resource address for this resource first */
   addr_src = emit_uav_addr_offset(emit, resourceType, resourceIndex,
                        /* LOAD resource, address, src */
                     if (resourceType == TGSI_FILE_MEMORY ||
      resourceType == TGSI_FILE_BUFFER ||
   resourceType == TGSI_FILE_HW_ATOMIC) {
   token0.opcodeType = VGPU10_OPCODE_LD_RAW;
      }
   else {
                  token0.saturate = inst->Instruction.Saturate,
            emit_dst_register(emit, &inst->Dst[0]);
            if (resourceType == TGSI_FILE_MEMORY) {
         } else if (resourceType == TGSI_FILE_HW_ATOMIC) {
      emit_uav_register(emit, inst->Src[0].Dimension.Index,
      } else if (resourceType == TGSI_FILE_BUFFER) {
      if (emit->raw_shaderbufs & (1 << resourceIndex))
      emit_resource_register(emit, resourceIndex +
      else
      emit_uav_register(emit, resourceIndex,
   } else {
      emit_uav_register(emit, resourceIndex,
                           }
         /**
   * Emit uav / memory load instruction
   */
   static bool
   emit_load(struct svga_shader_emitter_v10 *emit,
         {
      enum tgsi_file_type resourceType = inst->Src[0].Register.File;
            /* If the resource register has indirect index, we will need
   * to expand it since SM5 device does not support indirect indexing
   * for uav.
   */
   if (inst->Src[0].Register.Indirect &&
               unsigned indirect_index = inst->Src[0].Indirect.Index;
   unsigned num_resources =
                  /* indirect index tmp register */
   unsigned indirect_addr = emit->address_reg_index[indirect_index];
   struct tgsi_full_src_register indirect_addr_src =
                  /* Add offset to the indirect index */
   if (inst->Src[0].Register.Index != 0) {
      struct tgsi_full_src_register offset =
         struct tgsi_full_dst_register indirect_addr_dst =
         emit_instruction_op2(emit, VGPU10_OPCODE_IADD, &indirect_addr_dst,
               /* Loop through the resource array to find which resource to use.
   */
   loop_instruction(0, num_resources, &indirect_addr_src,
      }
   else {
                              }
         /**
   * A helper function to emit a store instruction.
   */
   static void
   emit_store_instruction(struct svga_shader_emitter_v10 *emit,
               {
      VGPU10OpcodeToken0 token0;
   enum tgsi_file_type resourceType = inst->Dst[0].Register.File;
   unsigned writemask = inst->Dst[0].Register.WriteMask;
            unsigned tmp_index = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp_index);
   struct tgsi_full_dst_register tmp_dst_xyzw = make_dst_temp_reg(tmp_index);
            struct tgsi_full_src_register src = inst->Src[1];
            bool needLoad = false;
   bool needPerComponentStore = false;
            /* Resolve the resource address for this resource first */
   addr_src = emit_uav_addr_offset(emit, resourceType,
                              /* First check the writemask to see if it can be supported
   * by the store instruction.
   * store_raw only allows .x, .xy, .xyz, .xyzw. For the typeless memory,
   * we can adjust the address offset, and do a per-component store.
   * store_uav_typed only allows .xyzw. In this case, we need to
   * do a load first, update the temporary and then issue the
   * store. This does have a small risk that if different threads
   * update different components of the same address, data might not be
   * in sync.
   */
   if (resourceType == TGSI_FILE_IMAGE) {
         }
   else if (resourceType == TGSI_FILE_BUFFER ||
            if (!(writemask == TGSI_WRITEMASK_X || writemask == TGSI_WRITEMASK_XY ||
         writemask == TGSI_WRITEMASK_XYZ ||
                     if (needLoad) {
               /* LOAD resource, address, src */
            token0.value = 0;
   token0.opcodeType = VGPU10_OPCODE_LD_UAV_TYPED;
   token0.saturate = inst->Instruction.Saturate,
            emit_dst_register(emit, &tmp_dst_xyzw);
   emit_src_register(emit, &addr_src);
                     /* MOV tmp(writemask) src */
   tmp_dst = writemask_dst(&tmp_dst_xyzw, writemask);
            /* Now set the writemask to xyzw for the store_uav_typed instruction */
      }
   else if (needPerComponentStore) {
      /* Save the src swizzles */
   swizzles = src.Register.SwizzleX |
            src.Register.SwizzleY << 2 |
            bool storeDone = false;
   unsigned perComponentWritemask = writemask;
   unsigned shift = 0;
                        if (needPerComponentStore) {
      assert(perComponentWritemask);
   while (!(perComponentWritemask & TGSI_WRITEMASK_X)) {
      shift++;
               /* First adjust the addr_src to the next component */
   if (shift != 0) {
      struct tgsi_full_dst_register addr_dst =
         shift_src = make_immediate_reg_int(emit, shift);
                  /* Adjust the src swizzle as well */
               /* Now the address offset is set to the next component,
   * we can set the writemask to .x and make sure to set
   * the src swizzle as well.
   */
                  /* Shift for the next component check */
   perComponentWritemask >>= 1;
               /* STORE resource, address, src */
            token0.value = 0;
            if (resourceType == TGSI_FILE_MEMORY) {
      token0.opcodeType = VGPU10_OPCODE_STORE_RAW;
   addr_src = scalar_src(&addr_src, TGSI_SWIZZLE_X);
   emit_dword(emit, token0.value);
      }
   else if (resourceType == TGSI_FILE_BUFFER ||
            token0.opcodeType = VGPU10_OPCODE_STORE_RAW;
   addr_src = scalar_src(&addr_src, TGSI_SWIZZLE_X);
   emit_dword(emit, token0.value);
   emit_uav_register(emit, resourceIndex, UAV_STORE,
      }
   else {
      token0.opcodeType = VGPU10_OPCODE_STORE_UAV_TYPED;
   emit_dword(emit, token0.value);
   emit_uav_register(emit, resourceIndex, UAV_STORE,
                        if (needLoad)
         else
                     if (!needPerComponentStore || !perComponentWritemask)
                  }
         /**
   * Emit uav / memory store instruction
   */
   static bool
   emit_store(struct svga_shader_emitter_v10 *emit,
         {
      enum tgsi_file_type resourceType = inst->Dst[0].Register.File;
            /* If the resource register has indirect index, we will need
   * to expand it since SM5 device does not support indirect indexing
   * for uav.
   */
   if (inst->Dst[0].Register.Indirect &&
               unsigned indirect_index = inst->Dst[0].Indirect.Index;
   unsigned num_resources =
                  /* Indirect index tmp register */
   unsigned indirect_addr = emit->address_reg_index[indirect_index];
   struct tgsi_full_src_register indirect_addr_src =
                  /* Add offset to the indirect index */
   if (inst->Dst[0].Register.Index != 0) {
      struct tgsi_full_src_register offset =
         struct tgsi_full_dst_register indirect_addr_dst =
         emit_instruction_op2(emit, VGPU10_OPCODE_IADD, &indirect_addr_dst,
               /* Loop through the resource array to find which resource to use.
   */
   loop_instruction(0, num_resources, &indirect_addr_src,
      }
   else {
                              }
         /**
   * A helper function to emit an atomic instruction.
   */
      static void
   emit_atomic_instruction(struct svga_shader_emitter_v10 *emit,
               {
      VGPU10OpcodeToken0 token0;
   enum tgsi_file_type resourceType = inst->Src[0].Register.File;
   struct tgsi_full_src_register addr_src;
   VGPU10_OPCODE_TYPE opcode = emit->cur_atomic_opcode;
            /* ntt does not specify offset for HWATOMIC. So just set offset to NULL. */
            /* Resolve the resource address */
   addr_src = emit_uav_addr_offset(emit, resourceType,
                              /* Emit the atomic operation */
            token0.value = 0;
   token0.opcodeType = opcode;
   token0.saturate = inst->Instruction.Saturate,
                     if (inst->Src[0].Register.File == TGSI_FILE_MEMORY) {
         } else if (inst->Src[0].Register.File == TGSI_FILE_HW_ATOMIC) {
      assert(inst->Src[0].Register.Dimension == 1);
   emit_uav_register(emit, inst->Src[0].Dimension.Index,
      } else {
      emit_uav_register(emit, resourceIndex,
               /* resource address offset */
            struct tgsi_full_src_register src0_x =
         swizzle_src(&inst->Src[2], TGSI_SWIZZLE_X, TGSI_SWIZZLE_X,
            if (opcode == VGPU10_OPCODE_IMM_ATOMIC_CMP_EXCH) {
      struct tgsi_full_src_register src1_x =
                                          }
         /**
   * Emit atomic instruction
   */
   static bool
   emit_atomic(struct svga_shader_emitter_v10 *emit,
               {
      enum tgsi_file_type resourceType = inst->Src[0].Register.File;
                     /* If the resource register has indirect index, we will need
   * to expand it since SM5 device does not support indirect indexing
   * for uav.
   */
   if (inst->Dst[0].Register.Indirect &&
               unsigned indirect_index = inst->Dst[0].Indirect.Index;
   unsigned num_resources =
                  /* indirect index tmp register */
   unsigned indirect_addr = emit->address_reg_index[indirect_index];
   struct tgsi_full_src_register indirect_addr_src =
                  /* Loop through the resource array to find which resource to use.
   */
   loop_instruction(0, num_resources, &indirect_addr_src,
      }
   else {
                              }
         /**
   * Emit barrier instruction
   */
   static bool
   emit_barrier(struct svga_shader_emitter_v10 *emit,
         {
                        token0.value = 0;
            if (emit->unit == PIPE_SHADER_TESS_CTRL && emit->version == 50) {
      /* SM5 device doesn't support BARRIER in tcs . If barrier is used
   * in shader, don't do anything for this opcode and continue rest
   * of shader translation
   */
   util_debug_message(&emit->svga_debug_callback, INFO,
            }
   else if (emit->unit == PIPE_SHADER_COMPUTE) {
      if (emit->cs.shared_memory_declared)
            if (emit->uav_declared)
               } else {
                  assert(token0.syncUAVMemoryGlobal || token0.syncUAVMemoryGroup ||
            begin_emit_instruction(emit);
   emit_dword(emit, token0.value);
               }
      /**
   * Emit memory barrier instruction
   */
   static bool
   emit_memory_barrier(struct svga_shader_emitter_v10 *emit,
         {
      unsigned index = inst->Src[0].Register.Index;
   unsigned swizzle = inst->Src[0].Register.SwizzleX;
   unsigned bartype = emit->immediates[index][swizzle].Int;
            token0.value = 0;
                        /* For compute shader, issue sync opcode with different options
   * depending on the memory barrier type.
   *
   * Bit 0: Shader storage buffers
   * Bit 1: Atomic buffers
   * Bit 2: Images
   * Bit 3: Shared memory
   * Bit 4: Thread group
            if (bartype & (TGSI_MEMBAR_SHADER_BUFFER | TGSI_MEMBAR_ATOMIC_BUFFER |
               else if (bartype & TGSI_MEMBAR_THREAD_GROUP)
            if (bartype & TGSI_MEMBAR_SHARED)
      }
   else {
      /**
   * For graphics stages, only sync_uglobal is available.
   */
   if (bartype & (TGSI_MEMBAR_SHADER_BUFFER | TGSI_MEMBAR_ATOMIC_BUFFER |
                     assert(token0.syncUAVMemoryGlobal || token0.syncUAVMemoryGroup ||
            begin_emit_instruction(emit);
   emit_dword(emit, token0.value);
               }
         /**
   * Emit code for TGSI_OPCODE_RESQ (image size) instruction.
   */
   static bool
   emit_resq(struct svga_shader_emitter_v10 *emit,
         {
      struct tgsi_full_src_register zero =
                     if (uav_resource == TGSI_TEXTURE_CUBE_ARRAY) {
                        emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &inst->Dst[0], &image_src);
               begin_emit_instruction(emit);
   if (uav_resource == TGSI_TEXTURE_BUFFER) {
      emit_opcode(emit, VGPU10_OPCODE_BUFINFO, false);
      }
   else {
      emit_opcode_resinfo(emit, VGPU10_RESINFO_RETURN_UINT);
   emit_dst_register(emit, &inst->Dst[0]);
      }
   emit_uav_register(emit, inst->Src[0].Register.Index,
                     }
         static bool
   emit_instruction(struct svga_shader_emitter_v10 *emit,
               {
               switch (opcode) {
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_BGNLOOP:
   case TGSI_OPCODE_BRK:
   case TGSI_OPCODE_CEIL:
   case TGSI_OPCODE_CONT:
   case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
   case TGSI_OPCODE_DIV:
   case TGSI_OPCODE_DP2:
   case TGSI_OPCODE_DP3:
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_ENDIF:
   case TGSI_OPCODE_ENDLOOP:
   case TGSI_OPCODE_ENDSUB:
   case TGSI_OPCODE_F2I:
   case TGSI_OPCODE_F2U:
   case TGSI_OPCODE_FLR:
   case TGSI_OPCODE_FRC:
   case TGSI_OPCODE_FSEQ:
   case TGSI_OPCODE_FSGE:
   case TGSI_OPCODE_FSLT:
   case TGSI_OPCODE_FSNE:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_NOP:
   case TGSI_OPCODE_NOT:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_ROUND:
   case TGSI_OPCODE_SQRT:
   case TGSI_OPCODE_SHL:
   case TGSI_OPCODE_TRUNC:
   case TGSI_OPCODE_U2F:
   case TGSI_OPCODE_UCMP:
   case TGSI_OPCODE_USHR:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_XOR:
   /* Begin SM5 opcodes */
   case TGSI_OPCODE_F2D:
   case TGSI_OPCODE_D2F:
   case TGSI_OPCODE_DADD:
   case TGSI_OPCODE_DMUL:
   case TGSI_OPCODE_DMAX:
   case TGSI_OPCODE_DMIN:
   case TGSI_OPCODE_DSGE:
   case TGSI_OPCODE_DSLT:
   case TGSI_OPCODE_DSEQ:
   case TGSI_OPCODE_DSNE:
   case TGSI_OPCODE_BREV:
   case TGSI_OPCODE_POPC:
   case TGSI_OPCODE_LSB:
   case TGSI_OPCODE_INTERP_CENTROID:
   case TGSI_OPCODE_INTERP_SAMPLE:
      /* simple instructions */
      case TGSI_OPCODE_RET:
      if (emit->unit == PIPE_SHADER_TESS_CTRL &&
               /* store the tessellation levels in the patch constant phase only */
      }
         case TGSI_OPCODE_IMSB:
   case TGSI_OPCODE_UMSB:
         case TGSI_OPCODE_IBFE:
   case TGSI_OPCODE_UBFE:
         case TGSI_OPCODE_BFI:
         case TGSI_OPCODE_MOV:
         case TGSI_OPCODE_EMIT:
         case TGSI_OPCODE_ENDPRIM:
         case TGSI_OPCODE_IABS:
         case TGSI_OPCODE_ARL:
         case TGSI_OPCODE_UARL:
         case TGSI_OPCODE_BGNSUB:
      /* no-op */
      case TGSI_OPCODE_CAL:
         case TGSI_OPCODE_CMP:
         case TGSI_OPCODE_COS:
         case TGSI_OPCODE_DST:
         case TGSI_OPCODE_EX2:
         case TGSI_OPCODE_EXP:
         case TGSI_OPCODE_IF:
         case TGSI_OPCODE_KILL:
         case TGSI_OPCODE_KILL_IF:
         case TGSI_OPCODE_LG2:
         case TGSI_OPCODE_LIT:
         case TGSI_OPCODE_LODQ:
         case TGSI_OPCODE_LOG:
         case TGSI_OPCODE_LRP:
         case TGSI_OPCODE_POW:
         case TGSI_OPCODE_RCP:
         case TGSI_OPCODE_RSQ:
         case TGSI_OPCODE_SAMPLE:
         case TGSI_OPCODE_SEQ:
         case TGSI_OPCODE_SGE:
         case TGSI_OPCODE_SGT:
         case TGSI_OPCODE_SIN:
         case TGSI_OPCODE_SLE:
         case TGSI_OPCODE_SLT:
         case TGSI_OPCODE_SNE:
         case TGSI_OPCODE_SSG:
         case TGSI_OPCODE_ISSG:
         case TGSI_OPCODE_TEX:
         case TGSI_OPCODE_TG4:
         case TGSI_OPCODE_TEX2:
         case TGSI_OPCODE_TXP:
         case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXB2:
   case TGSI_OPCODE_TXL:
         case TGSI_OPCODE_TXD:
         case TGSI_OPCODE_TXF:
         case TGSI_OPCODE_TXL2:
         case TGSI_OPCODE_TXQ:
         case TGSI_OPCODE_UIF:
         case TGSI_OPCODE_UMUL_HI:
   case TGSI_OPCODE_IMUL_HI:
   case TGSI_OPCODE_UDIV:
      /* These cases use only the FIRST of two destination registers */
      case TGSI_OPCODE_IDIV:
         case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_MOD:
      /* These cases use only the SECOND of two destination registers */
         /* Begin SM5 opcodes */
   case TGSI_OPCODE_DABS:
         case TGSI_OPCODE_DNEG:
         case TGSI_OPCODE_DRCP:
         case TGSI_OPCODE_DSQRT:
         case TGSI_OPCODE_DMAD:
         case TGSI_OPCODE_DFRAC:
         case TGSI_OPCODE_D2I:
   case TGSI_OPCODE_D2U:
         case TGSI_OPCODE_I2D:
   case TGSI_OPCODE_U2D:
         case TGSI_OPCODE_DRSQ:
         case TGSI_OPCODE_DDIV:
         case TGSI_OPCODE_INTERP_OFFSET:
         case TGSI_OPCODE_FMA:
   case TGSI_OPCODE_DFMA:
            case TGSI_OPCODE_DTRUNC:
            /* The following opcodes should never be seen here.  We return zero
   * for PIPE_CAP_TGSI_DROUND_SUPPORTED.
   */
   case TGSI_OPCODE_LDEXP:
   case TGSI_OPCODE_DSSG:
   case TGSI_OPCODE_DLDEXP:
   case TGSI_OPCODE_DCEIL:
   case TGSI_OPCODE_DFLR:
      debug_printf("Unexpected TGSI opcode %s.  "
                     case TGSI_OPCODE_LOAD:
            case TGSI_OPCODE_STORE:
            case TGSI_OPCODE_ATOMAND:
            case TGSI_OPCODE_ATOMCAS:
            case TGSI_OPCODE_ATOMIMAX:
            case TGSI_OPCODE_ATOMIMIN:
            case TGSI_OPCODE_ATOMOR:
            case TGSI_OPCODE_ATOMUADD:
            case TGSI_OPCODE_ATOMUMAX:
            case TGSI_OPCODE_ATOMUMIN:
            case TGSI_OPCODE_ATOMXCHG:
            case TGSI_OPCODE_ATOMXOR:
            case TGSI_OPCODE_BARRIER:
            case TGSI_OPCODE_MEMBAR:
            case TGSI_OPCODE_RESQ:
            case TGSI_OPCODE_END:
      if (!emit_post_helpers(emit))
               default:
      debug_printf("Unimplemented tgsi instruction %s\n",
                        }
         /**
   * Translate a single TGSI instruction to VGPU10.
   */
   static bool
   emit_vgpu10_instruction(struct svga_shader_emitter_v10 *emit,
               {
      if (emit->skip_instruction)
            bool ret = true;
                              if (emit->reemit_tgsi_instruction) {
      /**
   * Reset emit->ptr to where the translation of this tgsi instruction
   * started.
   */
   VGPU10OpcodeToken0 *tokens = (VGPU10OpcodeToken0 *) emit->buf;
               }
      }
         /**
   * Emit the extra instructions to adjust the vertex position.
   * There are two possible adjustments:
   * 1. Converting from Gallium to VGPU10 coordinate space by applying the
   *    "prescale" and "pretranslate" values.
   * 2. Undoing the viewport transformation when we use the swtnl/draw path.
   * \param vs_pos_tmp_index  which temporary register contains the vertex pos.
   */
   static void
   emit_vpos_instructions(struct svga_shader_emitter_v10 *emit)
   {
      struct tgsi_full_src_register tmp_pos_src;
   struct tgsi_full_dst_register pos_dst;
            /* Don't bother to emit any extra vertex instructions if vertex position is
   * not written out
   */
   if (emit->vposition.out_index == INVALID_INDEX)
            /**
   * Reset the temporary vertex position register index
   * so that emit_dst_register() will use the real vertex position output
   */
            tmp_pos_src = make_src_temp_reg(vs_pos_tmp_index);
            /* If non-adjusted vertex position register index
   * is valid, copy the vertex position from the temporary
   * vertex position register before it is modified by the
   * prescale computation.
   */
   if (emit->vposition.so_index != INVALID_INDEX) {
      struct tgsi_full_dst_register pos_so_dst =
            /* MOV pos_so, tmp_pos */
               if (emit->vposition.need_prescale) {
      /* This code adjusts the vertex position to match the VGPU10 convention.
   * If p is the position computed by the shader (usually by applying the
   * modelview and projection matrices), the new position q is computed by:
   *
   * q.x = p.w * trans.x + p.x * scale.x
   * q.y = p.w * trans.y + p.y * scale.y
   * q.z = p.w * trans.z + p.z * scale.z;
   * q.w = p.w * trans.w + p.w;
   */
   struct tgsi_full_src_register tmp_pos_src_w =
         struct tgsi_full_dst_register tmp_pos_dst =
         struct tgsi_full_dst_register tmp_pos_dst_xyz =
            struct tgsi_full_src_register prescale_scale =
         struct tgsi_full_src_register prescale_trans =
            /* MUL tmp_pos.xyz, tmp_pos, prescale.scale */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL, &tmp_pos_dst_xyz,
            /* MAD pos, tmp_pos.wwww, prescale.trans, tmp_pos */
   emit_instruction_op3(emit, VGPU10_OPCODE_MAD, &pos_dst, &tmp_pos_src_w,
      }
   else if (emit->key.vs.undo_viewport) {
      /* This code computes the final vertex position from the temporary
   * vertex position by undoing the viewport transformation and the
   * divide-by-W operation (we convert window coords back to clip coords).
   * This is needed when we use the 'draw' module for fallbacks.
   * If p is the temp pos in window coords, then the NDC coord q is:
   *   q.x = (p.x - vp.x_trans) / vp.x_scale * p.w
   *   q.y = (p.y - vp.y_trans) / vp.y_scale * p.w
   *   q.z = p.z * p.w
   *   q.w = p.w
   * CONST[vs_viewport_index] contains:
   *   { 1/vp.x_scale, 1/vp.y_scale, -vp.x_trans, -vp.y_trans }
   */
   struct tgsi_full_dst_register tmp_pos_dst =
         struct tgsi_full_dst_register tmp_pos_dst_xy =
         struct tgsi_full_src_register tmp_pos_src_wwww =
            struct tgsi_full_dst_register pos_dst_xyz =
         struct tgsi_full_dst_register pos_dst_w =
            struct tgsi_full_src_register vp_xyzw =
         struct tgsi_full_src_register vp_zwww =
                  /* ADD tmp_pos.xy, tmp_pos.xy, viewport.zwww */
   emit_instruction_op2(emit, VGPU10_OPCODE_ADD, &tmp_pos_dst_xy,
            /* MUL tmp_pos.xy, tmp_pos.xyzw, viewport.xyzy */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL, &tmp_pos_dst_xy,
            /* MUL pos.xyz, tmp_pos.xyz, tmp_pos.www */
   emit_instruction_op2(emit, VGPU10_OPCODE_MUL, &pos_dst_xyz,
            /* MOV pos.w, tmp_pos.w */
      }
   else if (vs_pos_tmp_index != INVALID_INDEX) {
      /* This code is to handle the case where the temporary vertex
   * position register is created when the vertex shader has stream
   * output and prescale is disabled because rasterization is to be
   * discarded.
   */
   struct tgsi_full_dst_register pos_dst =
            /* MOV pos, tmp_pos */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_MOV, false);
   emit_dst_register(emit, &pos_dst);
   emit_src_register(emit, &tmp_pos_src);
               /* Restore original vposition.tmp_index value for the next GS vertex.
   * It doesn't matter for VS.
   */
      }
      static void
   emit_clipping_instructions(struct svga_shader_emitter_v10 *emit)
   {
      if (emit->clip_mode == CLIP_DISTANCE) {
      /* Copy from copy distance temporary to CLIPDIST & the shadow copy */
         } else if (emit->clip_mode == CLIP_VERTEX &&
            /* Convert TGSI CLIPVERTEX to CLIPDIST */
               /**
   * Emit vertex position and take care of legacy user planes only if
   * there is a valid vertex position register index.
   * This is to take care of the case
   * where the shader doesn't output vertex position. Then in
   * this case, don't bother to emit more vertex instructions.
   */
   if (emit->vposition.out_index == INVALID_INDEX)
            /**
   * Emit per-vertex clipping instructions for legacy user defined clip planes.
   * NOTE: we must emit the clip distance instructions before the
   * emit_vpos_instructions() call since the later function will change
   * the TEMP[vs_pos_tmp_index] value.
   */
   if (emit->clip_mode == CLIP_LEGACY && emit->key.last_vertex_stage) {
      /* Emit CLIPDIST for legacy user defined clip planes */
         }
         /**
   * Emit extra per-vertex instructions.  This includes clip-coordinate
   * space conversion and computing clip distances.  This is called for
   * each GS emit-vertex instruction and at the end of VS translation.
   */
   static void
   emit_vertex_instructions(struct svga_shader_emitter_v10 *emit)
   {
      /* Emit clipping instructions based on clipping mode */
            /* Emit vertex position instructions */
      }
         /**
   * Translate the TGSI_OPCODE_EMIT GS instruction.
   */
   static bool
   emit_vertex(struct svga_shader_emitter_v10 *emit,
         {
                        /**
   * Emit the viewport array index for the first vertex.
   */
   if (emit->gs.viewport_index_out_index != INVALID_INDEX) {
      struct tgsi_full_dst_register viewport_index_out =
         struct tgsi_full_dst_register viewport_index_out_x =
         struct tgsi_full_src_register viewport_index_tmp =
            /* Set the out index to INVALID_INDEX, so it will not
   * be assigned to a temp again in emit_dst_register, and
   * the viewport index will not be assigned again in the
   * subsequent vertices.
   */
   emit->gs.viewport_index_out_index = INVALID_INDEX;
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV,
               /**
   * Find the stream index associated with this emit vertex instruction.
   */
   assert(inst->Src[0].Register.File == TGSI_FILE_IMMEDIATE);
            /**
   * According to the ARB_gpu_shader5 spec, the built-in geometry shader
   * outputs are always associated with vertex stream zero.
   * So emit the extra vertex instructions for position or clip distance
   * for stream zero only.
   */
   if (streamIndex == 0) {
      /**
   * Before emitting vertex instructions, emit the temporaries for
   * the prescale constants based on the viewport index if needed.
   */
   if (emit->vposition.need_prescale && !emit->vposition.have_prescale)
                        begin_emit_instruction(emit);
   if (emit->version >= 50) {
      if (emit->info.num_stream_output_components[streamIndex] == 0) {
      /**
   * If there is no output for this stream, discard this instruction.
   */
      }
   else {
      emit_opcode(emit, VGPU10_OPCODE_EMIT_STREAM, false);
         }
   else {
         }
               }
         /**
   * Emit the extra code to convert from VGPU10's boolean front-face
   * register to TGSI's signed front-face register.
   *
   * TODO: Make temporary front-face register a scalar.
   */
   static void
   emit_frontface_instructions(struct svga_shader_emitter_v10 *emit)
   {
               if (emit->fs.face_input_index != INVALID_INDEX) {
      /* convert vgpu10 boolean face register to gallium +/-1 value */
   struct tgsi_full_dst_register tmp_dst =
         struct tgsi_full_src_register one =
         struct tgsi_full_src_register neg_one =
            /* MOVC face_tmp, IS_FRONT_FACE.x, 1.0, -1.0 */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_MOVC, false);
   emit_dst_register(emit, &tmp_dst);
   emit_face_register(emit);
   emit_src_register(emit, &one);
   emit_src_register(emit, &neg_one);
         }
         /**
   * Emit the extra code to convert from VGPU10's fragcoord.w value to 1/w.
   */
   static void
   emit_fragcoord_instructions(struct svga_shader_emitter_v10 *emit)
   {
               if (emit->fs.fragcoord_input_index != INVALID_INDEX) {
      struct tgsi_full_dst_register tmp_dst =
         struct tgsi_full_dst_register tmp_dst_xyz =
         struct tgsi_full_dst_register tmp_dst_w =
         struct tgsi_full_src_register one =
         struct tgsi_full_src_register fragcoord =
            /* save the input index */
   unsigned fragcoord_input_index = emit->fs.fragcoord_input_index;
   /* set to invalid to prevent substitution in emit_src_register() */
            /* MOV fragcoord_tmp.xyz, fragcoord.xyz */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_MOV, false);
   emit_dst_register(emit, &tmp_dst_xyz);
   emit_src_register(emit, &fragcoord);
            /* DIV fragcoord_tmp.w, 1.0, fragcoord.w */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_DIV, false);
   emit_dst_register(emit, &tmp_dst_w);
   emit_src_register(emit, &one);
   emit_src_register(emit, &fragcoord);
            /* restore saved value */
         }
         /**
   * Emit the extra code to get the current sample position value and
   * put it into a temp register.
   */
   static void
   emit_sample_position_instructions(struct svga_shader_emitter_v10 *emit)
   {
               if (emit->fs.sample_pos_sys_index != INVALID_INDEX) {
               struct tgsi_full_dst_register tmp_dst =
         struct tgsi_full_src_register half =
            struct tgsi_full_src_register tmp_src =
         struct tgsi_full_src_register sample_index_reg =
                  /* The first src register is a shader resource (if we want a
   * multisampled resource sample position) or the rasterizer register
   * (if we want the current sample position in the color buffer).  We
   * want the later.
            /* SAMPLE_POS dst, RASTERIZER, sampleIndex */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_SAMPLE_POS, false);
   emit_dst_register(emit, &tmp_dst);
   emit_rasterizer_register(emit);
   emit_src_register(emit, &sample_index_reg);
            /* Convert from D3D coords to GL coords by adding 0.5 bias */
   /* ADD dst, dst, half */
   begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_ADD, false);
   emit_dst_register(emit, &tmp_dst);
   emit_src_register(emit, &tmp_src);
   emit_src_register(emit, &half);
         }
         /**
   * Emit extra instructions to adjust VS inputs/attributes.  This can
   * mean casting a vertex attribute from int to float or setting the
   * W component to 1, or both.
   */
   static void
   emit_vertex_attrib_instructions(struct svga_shader_emitter_v10 *emit)
   {
      const unsigned save_w_1_mask = emit->key.vs.adjust_attrib_w_1;
   const unsigned save_itof_mask = emit->key.vs.adjust_attrib_itof;
   const unsigned save_utof_mask = emit->key.vs.adjust_attrib_utof;
   const unsigned save_is_bgra_mask = emit->key.vs.attrib_is_bgra;
   const unsigned save_puint_to_snorm_mask = emit->key.vs.attrib_puint_to_snorm;
   const unsigned save_puint_to_uscaled_mask = emit->key.vs.attrib_puint_to_uscaled;
            unsigned adjust_mask = (save_w_1_mask |
                           save_itof_mask |
                     if (adjust_mask) {
      struct tgsi_full_src_register one =
            struct tgsi_full_src_register one_int =
            /* We need to turn off these bitmasks while emitting the
   * instructions below, then restore them afterward.
   */
   emit->key.vs.adjust_attrib_w_1 = 0;
   emit->key.vs.adjust_attrib_itof = 0;
   emit->key.vs.adjust_attrib_utof = 0;
   emit->key.vs.attrib_is_bgra = 0;
   emit->key.vs.attrib_puint_to_snorm = 0;
   emit->key.vs.attrib_puint_to_uscaled = 0;
            while (adjust_mask) {
               /* skip the instruction if this vertex attribute is not being used */
                  unsigned tmp = emit->vs.adjusted_input[index];
                  struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
                  /* ITOF/UTOF/MOV tmp, input[index] */
   if (save_itof_mask & (1 << index)) {
      emit_instruction_op1(emit, VGPU10_OPCODE_ITOF,
      }
   else if (save_utof_mask & (1 << index)) {
      emit_instruction_op1(emit, VGPU10_OPCODE_UTOF,
      }
   else if (save_puint_to_snorm_mask & (1 << index)) {
         }
   else if (save_puint_to_uscaled_mask & (1 << index)) {
         }
   else if (save_puint_to_sscaled_mask & (1 << index)) {
         }
   else {
      assert((save_w_1_mask | save_is_bgra_mask) & (1 << index));
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV,
               if (save_is_bgra_mask & (1 << index)) {
                  if (save_w_1_mask & (1 << index)) {
      /* MOV tmp.w, 1.0 */
   if (emit->key.vs.attrib_is_pure_int & (1 << index)) {
      emit_instruction_op1(emit, VGPU10_OPCODE_MOV,
      }
   else {
      emit_instruction_op1(emit, VGPU10_OPCODE_MOV,
                     emit->key.vs.adjust_attrib_w_1 = save_w_1_mask;
   emit->key.vs.adjust_attrib_itof = save_itof_mask;
   emit->key.vs.adjust_attrib_utof = save_utof_mask;
   emit->key.vs.attrib_is_bgra = save_is_bgra_mask;
   emit->key.vs.attrib_puint_to_snorm = save_puint_to_snorm_mask;
   emit->key.vs.attrib_puint_to_uscaled = save_puint_to_uscaled_mask;
         }
         /* Find zero-value immedate for default layer index */
   static void
   emit_default_layer_instructions(struct svga_shader_emitter_v10 *emit)
   {
               /* immediate for default layer index 0 */
   if (emit->fs.layer_input_index != INVALID_INDEX) {
      union tgsi_immediate_data imm;
   imm.Int = 0;
         }
         static void
   emit_temp_prescale_from_cbuf(struct svga_shader_emitter_v10 *emit,
                     {
      struct tgsi_full_src_register scale_cbuf = make_src_const_reg(cbuf_index);
            emit_instruction_op1(emit, VGPU10_OPCODE_MOV, scale, &scale_cbuf);
      }
         /**
   * A recursive helper function to find the prescale from the constant buffer
   */
   static void
   find_prescale_from_cbuf(struct svga_shader_emitter_v10 *emit,
                           unsigned index, unsigned num_prescale,
   struct tgsi_full_src_register *vp_index,
   {
      if (num_prescale == 0)
            if (index > 0) {
      /* ELSE */
               struct tgsi_full_src_register index_src =
            if (index == 0) {
      /* GE tmp, vp_index, index */
   emit_instruction_op2(emit, VGPU10_OPCODE_GE, tmp_dst,
      } else {
      /* EQ tmp, vp_index, index */
   emit_instruction_op2(emit, VGPU10_OPCODE_EQ, tmp_dst,
               /* IF tmp */
   emit_if(emit, tmp_src);
   emit_temp_prescale_from_cbuf(emit,
                  find_prescale_from_cbuf(emit, index+1, num_prescale-1,
                  /* ENDIF */
      }
         /**
   * This helper function emits instructions to set the prescale
   * and translate temporaries to the correct constants from the
   * constant buffer according to the designated viewport.
   */
   static void
   emit_temp_prescale_instructions(struct svga_shader_emitter_v10 *emit)
   {
      struct tgsi_full_dst_register prescale_scale =
         struct tgsi_full_dst_register prescale_translate =
                     if (emit->vposition.num_prescale == 1) {
      emit_temp_prescale_from_cbuf(emit,
            } else {
      /**
   * Since SM5 device does not support dynamic indexing, we need
   * to do the if-else to find the prescale constants for the
   * specified viewport.
   */
   struct tgsi_full_src_register vp_index_src =
            struct tgsi_full_src_register vp_index_src_x =
            unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_src_register tmp_src_x =
                  find_prescale_from_cbuf(emit, 0, emit->vposition.num_prescale,
                           /* Mark prescale temporaries are emitted */
      }
         /**
   * A helper function to emit an instruction in a vertex shader to add a bias
   * to the VertexID system value. This patches the VertexID in the SVGA vertex
   * shader to include the base vertex of an indexed primitive or the start index
   * of a non-indexed primitive.
   */
   static void
   emit_vertex_id_nobase_instruction(struct svga_shader_emitter_v10 *emit)
   {
      struct tgsi_full_src_register vertex_id_bias_index =
         struct tgsi_full_src_register vertex_id_sys_src =
         struct tgsi_full_src_register vertex_id_sys_src_x =
         struct tgsi_full_dst_register vertex_id_tmp_dst =
            /* IADD vertex_id_tmp, vertex_id_sys, vertex_id_bias */
   unsigned vertex_id_tmp_index = emit->vs.vertex_id_tmp_index;
   emit->vs.vertex_id_tmp_index = INVALID_INDEX;
   emit_instruction_opn(emit, VGPU10_OPCODE_IADD, &vertex_id_tmp_dst,
                  }
      /**
   * Hull Shader must have control point outputs. But tessellation
   * control shader can return without writing to control point output.
   * In this case, the control point output is assumed to be passthrough
   * from the control point input.
   * This helper function is to write out a control point output first in case
   * the tessellation control shader returns before writing a
   * control point output.
   */
   static void
   emit_tcs_default_control_point_output(struct svga_shader_emitter_v10 *emit)
   {
      assert(emit->unit == PIPE_SHADER_TESS_CTRL);
   assert(emit->tcs.control_point_phase);
   assert(emit->tcs.control_point_out_index != INVALID_INDEX);
            struct tgsi_full_dst_register output_control_point;
   output_control_point =
            if (emit->tcs.control_point_input_index == INVALID_INDEX) {
      /* MOV OUTPUT 0.0f */
   struct tgsi_full_src_register zero = make_immediate_reg_float(emit, 0.0f);
   begin_emit_instruction(emit);
   emit_opcode_precise(emit, VGPU10_OPCODE_MOV, false, false);
   emit_dst_register(emit, &output_control_point);
   emit_src_register(emit, &zero);
      }
   else {
               struct tgsi_full_src_register invocation_src;
   struct tgsi_full_dst_register addr_dst;
   struct tgsi_full_dst_register addr_dst_x;
            addr_tmp = emit->address_reg_index[emit->tcs.control_point_addr_index];
   addr_dst = make_dst_temp_reg(addr_tmp);
            invocation_src = make_src_reg(TGSI_FILE_SYSTEM_VALUE,
            begin_emit_instruction(emit);
   emit_opcode_precise(emit, VGPU10_OPCODE_MOV, false, false);
   emit_dst_register(emit, &addr_dst_x);
   emit_src_register(emit, &invocation_src);
                        struct tgsi_full_src_register input_control_point;
   input_control_point = make_src_reg(TGSI_FILE_INPUT,
         input_control_point.Register.Dimension = 1;
   input_control_point.Dimension.Indirect = 1;
   input_control_point.DimIndirect.File = TGSI_FILE_ADDRESS;
   input_control_point.DimIndirect.Index =
            begin_emit_instruction(emit);
   emit_opcode_precise(emit, VGPU10_OPCODE_MOV, false, false);
   emit_dst_register(emit, &output_control_point);
   emit_src_register(emit, &input_control_point);
         }
      /**
   * This functions constructs temporary tessfactor from VGPU10*_TESSFACTOR
   * values in domain shader. SM5 has tessfactors as floating point values where
   * as tgsi emit them as vector. This function allows to construct temp
   * tessfactor vector similar to TGSI_SEMANTIC_TESSINNER/OUTER filled with
   * values from VGPU10*_TESSFACTOR. Use this constructed vector whenever
   * TGSI_SEMANTIC_TESSINNER/OUTER is used in shader.
   */
   static void
   emit_temp_tessfactor_instructions(struct svga_shader_emitter_v10 *emit)
   {
      struct tgsi_full_src_register src;
            if (emit->tes.inner.tgsi_index != INVALID_INDEX) {
               switch (emit->tes.prim_mode) {
   case MESA_PRIM_QUADS:
      src = make_src_scalar_reg(TGSI_FILE_INPUT,
         dst = writemask_dst(&dst, TGSI_WRITEMASK_Y);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &dst, &src);
      case MESA_PRIM_TRIANGLES:
      src = make_src_scalar_reg(TGSI_FILE_INPUT,
         dst = writemask_dst(&dst, TGSI_WRITEMASK_X);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &dst, &src);
      case MESA_PRIM_LINES:
      /**
   * As per SM5 spec, InsideTessFactor for isolines are unused.
   * In fact glsl tessInnerLevel for isolines doesn't mean anything but if
   * any application try to read tessInnerLevel in TES when primitive type
   * is isolines, then instead of driver throwing segfault for accesing it,
   * return atleast vec(1.0f)
   */
   src = make_immediate_reg_float(emit, 1.0f);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &dst, &src);
      default:
                     if (emit->tes.outer.tgsi_index != INVALID_INDEX) {
               switch (emit->tes.prim_mode) {
   case MESA_PRIM_QUADS:
      src = make_src_scalar_reg(TGSI_FILE_INPUT,
         dst = writemask_dst(&dst, TGSI_WRITEMASK_W);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &dst, &src);
      case MESA_PRIM_TRIANGLES:
      src = make_src_scalar_reg(TGSI_FILE_INPUT,
         dst = writemask_dst(&dst, TGSI_WRITEMASK_Z);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &dst, &src);
      case MESA_PRIM_LINES:
      src = make_src_scalar_reg(TGSI_FILE_INPUT,
                        src = make_src_scalar_reg(TGSI_FILE_INPUT,
                           default:
               }
         static void
   emit_initialize_temp_instruction(struct svga_shader_emitter_v10 *emit)
   {
      struct tgsi_full_src_register src;
   struct tgsi_full_dst_register dst;
   unsigned vgpu10_temp_index = remap_temp_index(emit, TGSI_FILE_TEMPORARY,
         src = make_immediate_reg_float(emit, 0.0f);
   dst = make_dst_temp_reg(vgpu10_temp_index);
   emit_instruction_op1(emit, VGPU10_OPCODE_MOV, &dst, &src);
   emit->temp_map[emit->initialize_temp_index].initialized = true;
      }
         /**
   * Emit any extra/helper declarations/code that we might need between
   * the declaration section and code section.
   */
   static bool
   emit_pre_helpers(struct svga_shader_emitter_v10 *emit)
   {
      /* Properties */
   if (emit->unit == PIPE_SHADER_GEOMETRY)
         else if (emit->unit == PIPE_SHADER_TESS_CTRL) {
               /* Save the position of the first instruction token so that we can
   * do a second pass of the instructions for the patch constant phase.
   */
   emit->tcs.instruction_token_pos = emit->cur_tgsi_token;
            if (!emit_hull_shader_control_point_phase(emit)) {
      emit->skip_instruction = true;
               /* Set the current tcs phase to control point phase */
      }
   else if (emit->unit == PIPE_SHADER_TESS_EVAL) {
         }
   else if (emit->unit == PIPE_SHADER_COMPUTE) {
                  /* Declare inputs */
   if (!emit_input_declarations(emit))
            /* Declare outputs */
   if (!emit_output_declarations(emit))
            /* Declare temporary registers */
            /* For PIPE_SHADER_TESS_CTRL, constants, samplers, resources and immediates
   * will already be declared in hs_decls (emit_hull_shader_declarations)
   */
                        /* Declare constant registers */
            /* Declare samplers and resources */
   emit_sampler_declarations(emit);
            /* Declare images */
            /* Declare shader buffers */
            /* Declare atomic buffers */
               if (emit->unit != PIPE_SHADER_FRAGMENT &&
      emit->unit != PIPE_SHADER_COMPUTE) {
   /*
   * Declare clip distance output registers for ClipVertex or
   * user defined planes
   */
               if (emit->unit == PIPE_SHADER_COMPUTE) {
               if (emit->cs.grid_size.tgsi_index != INVALID_INDEX) {
      emit->cs.grid_size.imm_index =
      alloc_immediate_int4(emit,
                           if (emit->unit == PIPE_SHADER_FRAGMENT &&
      emit->key.fs.alpha_func != SVGA3D_CMP_ALWAYS) {
   float alpha = emit->key.fs.alpha_ref;
   emit->fs.alpha_ref_index =
               if (emit->unit != PIPE_SHADER_TESS_CTRL) {
      /**
   * For PIPE_SHADER_TESS_CTRL, immediates are already declared in
   * hs_decls
   */
      }
   else {
                  if (emit->unit == PIPE_SHADER_FRAGMENT) {
      emit_frontface_instructions(emit);
   emit_fragcoord_instructions(emit);
   emit_sample_position_instructions(emit);
      }
   else if (emit->unit == PIPE_SHADER_VERTEX) {
               if (emit->info.uses_vertexid)
      }
   else if (emit->unit == PIPE_SHADER_TESS_EVAL) {
                  /**
   * For geometry shader that writes to viewport index, the prescale
   * temporaries will be done at the first vertex emission.
   */
   if (emit->vposition.need_prescale && emit->vposition.num_prescale == 1)
               }
         /**
   * The device has no direct support for the pipe_blend_state::alpha_to_one
   * option so we implement it here with shader code.
   *
   * Note that this is kind of pointless, actually.  Here we're clobbering
   * the alpha value with 1.0.  So if alpha-to-coverage is enabled, we'll wind
   * up with 100% coverage.  That's almost certainly not what the user wants.
   * The work-around is to add extra shader code to compute coverage from alpha
   * and write it to the coverage output register (if the user's shader doesn't
   * do so already).  We'll probably do that in the future.
   */
   static void
   emit_alpha_to_one_instructions(struct svga_shader_emitter_v10 *emit,
         {
      struct tgsi_full_src_register one = make_immediate_reg_float(emit, 1.0f);
            /* Note: it's not 100% clear from the spec if we're supposed to clobber
   * the alpha for all render targets.  But that's what NVIDIA does and
   * that's what Piglit tests.
   */
   for (i = 0; i < emit->fs.num_color_outputs; i++) {
               if (fs_color_tmp_index != INVALID_INDEX && i == 0) {
      /* write to the temp color register */
      }
   else {
      /* write directly to the color[i] output */
                              }
         /**
   * Emit alpha test code.  This compares TEMP[fs_color_tmp_index].w
   * against the alpha reference value and discards the fragment if the
   * comparison fails.
   */
   static void
   emit_alpha_test_instructions(struct svga_shader_emitter_v10 *emit,
         {
      /* compare output color's alpha to alpha ref and discard if comparison
   * fails.
   */
   unsigned tmp = get_temp_index(emit);
   struct tgsi_full_src_register tmp_src = make_src_temp_reg(tmp);
   struct tgsi_full_src_register tmp_src_x =
         struct tgsi_full_dst_register tmp_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register color_src =
         struct tgsi_full_src_register color_src_w =
         struct tgsi_full_src_register ref_src =
         struct tgsi_full_dst_register color_dst =
                     /* dst = src0 'alpha_func' src1 */
   emit_comparison(emit, emit->key.fs.alpha_func, &tmp_dst,
            /* DISCARD if dst.x == 0 */
   begin_emit_instruction(emit);
   emit_discard_opcode(emit, false);  /* discard if src0.x is zero */
   emit_src_register(emit, &tmp_src_x);
            /* If we don't need to broadcast the color below, emit the final color here.
   */
   if (emit->key.fs.write_color0_to_n_cbufs <= 1) {
      /* MOV output.color, tempcolor */
                  }
         /**
   * Emit instructions for writing a single color output to multiple
   * color buffers.
   * This is used when the TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS (or
   * when key.fs.white_fragments is true).
   * property is set and the number of render targets is greater than one.
   * \param fs_color_tmp_index  index of the temp register that holds the
   *                            color to broadcast.
   */
   static void
   emit_broadcast_color_instructions(struct svga_shader_emitter_v10 *emit,
         {
      const unsigned n = emit->key.fs.write_color0_to_n_cbufs;
   unsigned i;
            if (emit->key.fs.white_fragments) {
      /* set all color outputs to white */
      }
   else {
      /* set all color outputs to TEMP[fs_color_tmp_index] */
   assert(fs_color_tmp_index != INVALID_INDEX);
                        for (i = 0; i < n; i++) {
      unsigned output_reg = emit->fs.color_out_index[i];
   struct tgsi_full_dst_register color_dst =
            /* Fill in this semantic here since we'll use it later in
   * emit_dst_register().
   */
            /* MOV output.color[i], tempcolor */
         }
         /**
   * Emit extra helper code after the original shader code, but before the
   * last END/RET instruction.
   * For vertex shaders this means emitting the extra code to apply the
   * prescale scale/translation.
   */
   static bool
   emit_post_helpers(struct svga_shader_emitter_v10 *emit)
   {
      if (emit->unit == PIPE_SHADER_VERTEX) {
         }
   else if (emit->unit == PIPE_SHADER_FRAGMENT) {
               assert(!(emit->key.fs.white_fragments &&
            /* We no longer want emit_dst_register() to substitute the
   * temporary fragment color register for the real color output.
   */
            if (emit->key.fs.alpha_to_one) {
         }
   if (emit->key.fs.alpha_func != SVGA3D_CMP_ALWAYS) {
         }
   if (emit->key.fs.write_color0_to_n_cbufs > 1 ||
      emit->key.fs.white_fragments) {
         }
   else if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      if (!emit->tcs.control_point_phase) {
      /* store the tessellation levels in the patch constant phase only */
      }
   else {
            }
   else if (emit->unit == PIPE_SHADER_TESS_EVAL) {
                     }
         /**
   * Reemit rawbuf instruction
   */
   static bool
   emit_rawbuf_instruction(struct svga_shader_emitter_v10 *emit,
               {
               /* For all the rawbuf references in this instruction,
   * load the rawbuf reference and assign to the designated temporary.
   * Then reeemit the instruction.
   */
            unsigned offset_tmp = get_temp_index(emit);
   struct tgsi_full_dst_register offset_dst = make_dst_temp_reg(offset_tmp);
   struct tgsi_full_src_register offset_src = make_src_temp_reg(offset_tmp);
            for (unsigned i = 0; i < emit->raw_buf_cur_tmp_index; i++) {
                        if (emit->raw_buf_tmp[i].indirect) {
      unsigned tmp = get_temp_index(emit);
   struct tgsi_full_dst_register element_dst = make_dst_temp_reg(tmp);
   struct tgsi_full_src_register element_index =
                        element_src = make_src_temp_reg(tmp);
                  /* element index from the indirect register */
                  /* IADD element_src element_index element_index_relative */
   emit_instruction_op2(emit, VGPU10_OPCODE_IADD, &element_dst,
      }
   else {
      unsigned element_index = emit->raw_buf_tmp[i].element_index;
   union tgsi_immediate_data imm;
   imm.Int = element_index;
   int immpos = find_immediate(emit, imm, 0);
   if (immpos < 0) {
      UNUSED unsigned element_index_imm =
      }
               /* byte offset = element index << 4 */
   emit_instruction_op2(emit, VGPU10_OPCODE_ISHL, &offset_dst,
            struct tgsi_full_dst_register dst_tmp =
                     begin_emit_instruction(emit);
   emit_opcode(emit, VGPU10_OPCODE_LD_RAW, false);
            struct tgsi_full_src_register offset_x =
                  emit_resource_register(emit,
                                       /* reset raw buf state */
   emit->raw_buf_cur_tmp_index = 0;
                        }
         /**
   * Translate the TGSI tokens into VGPU10 tokens.
   */
   static bool
   emit_vgpu10_instructions(struct svga_shader_emitter_v10 *emit,
         {
      struct tgsi_parse_context parse;
   bool ret = true;
   bool pre_helpers_emitted = false;
                                 /* Save the current tgsi token starting position */
                     switch (parse.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_IMMEDIATE:
      ret = emit_vgpu10_immediate(emit, &parse.FullToken.FullImmediate);
   if (!ret)
               case TGSI_TOKEN_TYPE_DECLARATION:
      ret = emit_vgpu10_declaration(emit, &parse.FullToken.FullDeclaration);
   if (!ret)
               case TGSI_TOKEN_TYPE_INSTRUCTION:
      if (!pre_helpers_emitted) {
      ret = emit_pre_helpers(emit);
   if (!ret)
            }
                  /* Usually this applies to TCS only. If shader is reading control
   * point outputs in control point phase, we should reemit all
   * instructions which are writting into control point output in
   * control phase to store results into temporaries.
   */
   if (emit->reemit_instruction) {
      assert(emit->unit == PIPE_SHADER_TESS_CTRL);
   ret = emit_vgpu10_instruction(emit, inst_number,
      }
   else if (emit->initialize_temp_index != INVALID_INDEX) {
      emit_initialize_temp_instruction(emit);
   emit->initialize_temp_index = INVALID_INDEX;
   ret = emit_vgpu10_instruction(emit, inst_number - 1,
      }
   else if (emit->reemit_rawbuf_instruction) {
      ret = emit_rawbuf_instruction(emit, inst_number - 1,
               if (!ret)
               case TGSI_TOKEN_TYPE_PROPERTY:
      ret = emit_vgpu10_property(emit, &parse.FullToken.FullProperty);
   if (!ret)
               default:
                     if (emit->unit == PIPE_SHADER_TESS_CTRL) {
               done:
      tgsi_parse_free(&parse);
      }
         /**
   * Emit the first VGPU10 shader tokens.
   */
   static bool
   emit_vgpu10_header(struct svga_shader_emitter_v10 *emit)
   {
                        /* Maximum supported shader version is 50 */
            ptoken.value = 0; /* init whole token to zero */
   ptoken.majorVersion = version / 10;
   ptoken.minorVersion = version % 10;
   ptoken.programType = translate_shader_type(emit->unit);
   if (!emit_dword(emit, ptoken.value))
            /* Second token: total length of shader, in tokens.  We can't fill this
   * in until we're all done.  Emit zero for now.
   */
   if (!emit_dword(emit, 0))
            if (emit->version >= 50) {
               if (emit->unit == PIPE_SHADER_TESS_CTRL) {
      /* For hull shader, we need to start the declarations phase first before
   * emitting any declarations including the global flags.
   */
   token.value = 0;
   token.opcodeType = VGPU10_OPCODE_HS_DECLS;
   begin_emit_instruction(emit);
   emit_dword(emit, token.value);
               /* Emit global flags */
   token.value = 0;    /* init whole token to zero */
   token.opcodeType = VGPU10_OPCODE_DCL_GLOBAL_FLAGS;
   token.enableDoublePrecisionFloatOps = 1;  /* set bit */
   token.instructionLength = 1;
   if (!emit_dword(emit, token.value))
               if (emit->version >= 40) {
               /* Reserved for global flag such as refactoringAllowed.
   * If the shader does not use the precise qualifier, we will set the
   * refactoringAllowed global flag; otherwise, we will leave the reserved
   * token to NOP.
   */
   emit->reserved_token = (emit->ptr - emit->buf) / sizeof(VGPU10OpcodeToken0);
   token.value = 0;
   token.opcodeType = VGPU10_OPCODE_NOP;
   token.instructionLength = 1;
   if (!emit_dword(emit, token.value))
                  }
         static bool
   emit_vgpu10_tail(struct svga_shader_emitter_v10 *emit)
   {
               /* Replace the second token with total shader length */
   tokens = (VGPU10ProgramToken *) emit->buf;
            if (emit->version >= 40 && !emit->uses_precise_qualifier) {
      /* Replace the reserved token with the RefactoringAllowed global flag */
            ptoken = (VGPU10OpcodeToken0 *)&tokens[emit->reserved_token];
   assert(ptoken->opcodeType == VGPU10_OPCODE_NOP);
   ptoken->opcodeType = VGPU10_OPCODE_DCL_GLOBAL_FLAGS;
               if (emit->version >= 50 && emit->fs.forceEarlyDepthStencil) {
      /* Replace the reserved token with the forceEarlyDepthStencil  global flag */
            ptoken = (VGPU10OpcodeToken0 *)&tokens[emit->reserved_token];
   ptoken->opcodeType = VGPU10_OPCODE_DCL_GLOBAL_FLAGS;
                  }
         /**
   * Modify the FS to read the BCOLORs and use the FACE register
   * to choose between the front/back colors.
   */
   static const struct tgsi_token *
   transform_fs_twoside(const struct tgsi_token *tokens)
   {
      if (0) {
      debug_printf("Before tgsi_add_two_side ------------------\n");
      }
   tokens = tgsi_add_two_side(tokens);
   if (0) {
      debug_printf("After tgsi_add_two_side ------------------\n");
      }
      }
         /**
   * Modify the FS to do polygon stipple.
   */
   static const struct tgsi_token *
   transform_fs_pstipple(struct svga_shader_emitter_v10 *emit,
         {
      const struct tgsi_token *new_tokens;
            if (0) {
      debug_printf("Before pstipple ------------------\n");
               new_tokens = util_pstipple_create_fragment_shader(tokens, &unit, 0,
                     /* The new sampler state is appended to the end of the samplers list */
            /* Setup texture state for stipple */
   emit->sampler_target[unit] = TGSI_TEXTURE_2D;
   emit->key.tex[unit].swizzle_r = TGSI_SWIZZLE_X;
   emit->key.tex[unit].swizzle_g = TGSI_SWIZZLE_Y;
   emit->key.tex[unit].swizzle_b = TGSI_SWIZZLE_Z;
   emit->key.tex[unit].swizzle_a = TGSI_SWIZZLE_W;
   emit->key.tex[unit].target = PIPE_TEXTURE_2D;
            if (0) {
      debug_printf("After pstipple ------------------\n");
                  }
      /**
   * Modify the FS to support anti-aliasing point.
   */
   static const struct tgsi_token *
   transform_fs_aapoint(struct svga_context *svga,
         const struct tgsi_token *tokens,
   {
      bool need_texcoord_semantic =
            if (0) {
      debug_printf("Before tgsi_add_aa_point ------------------\n");
      }
   tokens = tgsi_add_aa_point(tokens, aa_coord_index, need_texcoord_semantic);
   if (0) {
      debug_printf("After tgsi_add_aa_point ------------------\n");
      }
      }
         /**
   * A helper function to determine the shader in the previous stage and
   * then call the linker function to determine the input mapping for this
   * shader to match the output indices from the shader in the previous stage.
   */
   static void
   compute_input_mapping(struct svga_context *svga,
               {
               if (unit == PIPE_SHADER_FRAGMENT) {
      prevShader = svga->curr.gs ?
      &svga->curr.gs->base : (svga->curr.tes ?
   } else if (unit == PIPE_SHADER_GEOMETRY) {
         } else if (unit == PIPE_SHADER_TESS_EVAL) {
      assert(svga->curr.tcs);
      } else if (unit == PIPE_SHADER_TESS_CTRL) {
      assert(svga->curr.vs);
               if (prevShader != NULL) {
      svga_link_shaders(&prevShader->tgsi_info, &emit->info, &emit->linkage);
      } 
   else {
      /**
   * Since vertex shader does not need to go through the linker to
   * establish the input map, we need to make sure the highest index
   * of input registers is set properly here.
   */
   emit->linkage.input_map_max = MAX2((int)emit->linkage.input_map_max,
         }
         /**
   * Copies the shader signature info to the shader variant
   */
   static void
   copy_shader_signature(struct svga_shader_signature *sgn,
         {
               /* Calculate the signature length */
   variant->signatureLen = sizeof(SVGA3dDXShaderSignatureHeader) +
                              /* Allocate buffer for the signature info */
   variant->signature =
            char *sgnBuf = (char *)variant->signature;
            /* Copy the signature info to the shader variant structure */
   memcpy(sgnBuf, &sgn->header, sizeof(SVGA3dDXShaderSignatureHeader));
            if (header->numInputSignatures) {
      sgnLen =
         memcpy(sgnBuf, &sgn->inputs[0], sgnLen);
               if (header->numOutputSignatures) {
      sgnLen =
         memcpy(sgnBuf, &sgn->outputs[0], sgnLen);
               if (header->numPatchConstantSignatures) {
      sgnLen =
               }
         /**
   * This is the main entrypoint for the TGSI -> VPGU10 translator.
   */
   struct svga_shader_variant *
   svga_tgsi_vgpu10_translate(struct svga_context *svga,
                     {
      struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   struct svga_shader_variant *variant = NULL;
   struct svga_shader_emitter_v10 *emit;
                     assert(unit == PIPE_SHADER_VERTEX ||
         unit == PIPE_SHADER_GEOMETRY ||
   unit == PIPE_SHADER_FRAGMENT ||
   unit == PIPE_SHADER_TESS_CTRL ||
            /* These two flags cannot be used together */
            SVGA_STATS_TIME_PUSH(svga_sws(svga), SVGA_STATS_TIME_TGSIVGPU10TRANSLATE);
   /*
   * Setup the code emitter
   */
   emit = alloc_emitter();
   if (!emit)
            emit->unit = unit;
   if (svga_have_gl43(svga)) {
         } else if (svga_have_sm5(svga)) {
         } else if (svga_have_sm4_1(svga)) {
         } else {
                                             emit->vposition.need_prescale = (emit->key.vs.need_prescale ||
                  /* Determine how many prescale factors in the constant buffer */
   emit->vposition.num_prescale = 1;
   if (emit->vposition.need_prescale && emit->key.gs.writes_viewport_index) {
      assert(emit->unit == PIPE_SHADER_GEOMETRY);
               emit->vposition.tmp_index = INVALID_INDEX;
   emit->vposition.so_index = INVALID_INDEX;
            emit->vs.vertex_id_sys_index = INVALID_INDEX;
   emit->vs.vertex_id_tmp_index = INVALID_INDEX;
            emit->fs.color_tmp_index = INVALID_INDEX;
   emit->fs.face_input_index = INVALID_INDEX;
   emit->fs.fragcoord_input_index = INVALID_INDEX;
   emit->fs.sample_id_sys_index = INVALID_INDEX;
   emit->fs.sample_pos_sys_index = INVALID_INDEX;
   emit->fs.sample_mask_in_sys_index = INVALID_INDEX;
   emit->fs.layer_input_index = INVALID_INDEX;
            emit->gs.prim_id_index = INVALID_INDEX;
   emit->gs.invocation_id_sys_index = INVALID_INDEX;
   emit->gs.viewport_index_out_index = INVALID_INDEX;
            emit->tcs.vertices_per_patch_index = INVALID_INDEX;
   emit->tcs.invocation_id_sys_index = INVALID_INDEX;
   emit->tcs.control_point_input_index = INVALID_INDEX;
   emit->tcs.control_point_addr_index = INVALID_INDEX;
   emit->tcs.control_point_out_index = INVALID_INDEX;
   emit->tcs.control_point_tmp_index = INVALID_INDEX;
   emit->tcs.control_point_out_count = 0;
   emit->tcs.inner.out_index = INVALID_INDEX;
   emit->tcs.inner.temp_index = INVALID_INDEX;
   emit->tcs.inner.tgsi_index = INVALID_INDEX;
   emit->tcs.outer.out_index = INVALID_INDEX;
   emit->tcs.outer.temp_index = INVALID_INDEX;
   emit->tcs.outer.tgsi_index = INVALID_INDEX;
   emit->tcs.patch_generic_out_count = 0;
   emit->tcs.patch_generic_out_index = INVALID_INDEX;
   emit->tcs.patch_generic_tmp_index = INVALID_INDEX;
            emit->tes.tesscoord_sys_index = INVALID_INDEX;
   emit->tes.inner.in_index = INVALID_INDEX;
   emit->tes.inner.temp_index = INVALID_INDEX;
   emit->tes.inner.tgsi_index = INVALID_INDEX;
   emit->tes.outer.in_index = INVALID_INDEX;
   emit->tes.outer.temp_index = INVALID_INDEX;
   emit->tes.outer.tgsi_index = INVALID_INDEX;
            emit->cs.thread_id_index = INVALID_INDEX;
   emit->cs.block_id_index = INVALID_INDEX;
   emit->cs.grid_size.tgsi_index = INVALID_INDEX;
   emit->cs.grid_size.imm_index = INVALID_INDEX;
   emit->cs.block_width = 1;
   emit->cs.block_height = 1;
            emit->clip_dist_out_index = INVALID_INDEX;
   emit->clip_dist_tmp_index = INVALID_INDEX;
   emit->clip_dist_so_index = INVALID_INDEX;
   emit->clip_vertex_out_index = INVALID_INDEX;
   emit->clip_vertex_tmp_index = INVALID_INDEX;
            emit->index_range.start_index = INVALID_INDEX;
   emit->index_range.count = 0;
   emit->index_range.required = false;
   emit->index_range.operandType = VGPU10_NUM_OPERANDS;
   emit->index_range.dim = 0;
                     emit->initialize_temp_index = INVALID_INDEX;
            emit->max_vs_inputs  = svgascreen->max_vs_inputs;
   emit->max_vs_outputs = svgascreen->max_vs_outputs;
            if (emit->key.fs.alpha_func == SVGA3D_CMP_INVALID) {
                  if (unit == PIPE_SHADER_FRAGMENT) {
      if (key->fs.light_twoside) {
         }
   if (key->fs.pstipple) {
      const struct tgsi_token *new_tokens =
         if (tokens != shader->tokens) {
      /* free the two-sided shader tokens */
      }
      }
   if (key->fs.aa_point) {
      tokens = transform_fs_aapoint(svga, tokens,
                  if (SVGA_DEBUG & DEBUG_TGSI) {
      debug_printf("#####################################\n");
   debug_printf("### TGSI Shader %u\n", shader->id);
               /**
   * Rescan the header if the token string is different from the one
   * included in the shader; otherwise, the header info is already up-to-date
   */
   if (tokens != shader->tokens) {
         } else {
                           /**
   * Compute input mapping to match the outputs from shader
   * in the previous stage
   */
                     if (unit == PIPE_SHADER_GEOMETRY || unit == PIPE_SHADER_VERTEX ||
      unit == PIPE_SHADER_TESS_CTRL || unit == PIPE_SHADER_TESS_EVAL) {
   if (shader->stream_output != NULL || emit->clip_mode == CLIP_DISTANCE) {
      /* if there is stream output declarations associated
   * with this shader or the shader writes to ClipDistance
   * then reserve extra registers for the non-adjusted vertex position
   * and the ClipDistance shadow copy.
                  if (emit->clip_mode == CLIP_DISTANCE) {
      emit->clip_dist_so_index = emit->num_outputs++;
   if (emit->info.num_written_clipdistance > 4)
                     /* Determine if constbuf to rawbuf translation is needed */
   emit->raw_buf_srv_start_index = emit->key.srv_raw_constbuf_index;
   if (emit->info.const_buffers_declared)
            emit->raw_shaderbuf_srv_start_index = emit->key.srv_raw_shaderbuf_index;
   if (emit->info.shader_buffers_declared)
            /*
   * Do actual shader translation.
   */
   if (!emit_vgpu10_header(emit)) {
      debug_printf("svga: emit VGPU10 header failed\n");
               if (!emit_vgpu10_instructions(emit, tokens)) {
      debug_printf("svga: emit VGPU10 instructions failed\n");
               if (emit->num_new_immediates > 0) {
                  if (!emit_vgpu10_tail(emit)) {
      debug_printf("svga: emit VGPU10 tail failed\n");
               if (emit->register_overflow) {
                  /*
   * Create, initialize the 'variant' object.
   */
   variant = svga_new_shader_variant(svga, unit);
   if (!variant)
            variant->shader = shader;
   variant->nr_tokens = emit_get_num_tokens(emit);
            /* Copy shader signature info to the shader variant */
   if (svga_have_sm5(svga)) {
                  emit->buf = NULL;  /* buffer is no longer owed by emitter context */
   memcpy(&variant->key, key, sizeof(*key));
            /* The extra constant starting offset starts with the number of
   * shader constants declared in the shader.
   */
   variant->extra_const_start = emit->num_shader_consts[0];
   if (key->gs.wide_point) {
      /**
   * The extra constant added in the transformed shader
   * for inverse viewport scale is to be supplied by the driver.
   * So the extra constant starting offset needs to be reduced by 1.
   */
   assert(variant->extra_const_start > 0);
               if (unit == PIPE_SHADER_FRAGMENT) {
               fs_variant->pstipple_sampler_unit = emit->fs.pstipple_sampler_unit;
   fs_variant->pstipple_sampler_state_index =
            /* If there was exactly one write to a fragment shader output register
   * and it came from a constant buffer, we know all fragments will have
   * the same color (except for blending).
   */
   fs_variant->constant_color_output =
            /** keep track in the variant if flat interpolation is used
   *  for any of the varyings.
   */
               }
   else if (unit == PIPE_SHADER_TESS_EVAL) {
               /* Keep track in the tes variant some of the layout parameters.
   * These parameters will be referenced by the tcs to emit
   * the necessary declarations for the hull shader.
   */
   tes_variant->prim_mode = emit->tes.prim_mode;
   tes_variant->spacing = emit->tes.spacing;
   tes_variant->vertices_order_cw = emit->tes.vertices_order_cw;
                  if (tokens != shader->tokens) {
               cleanup:
            done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
