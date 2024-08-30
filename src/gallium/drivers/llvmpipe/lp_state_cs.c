   /**************************************************************************
   *
   * Copyright 2019 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/u_memory.h"
   #include "util/os_time.h"
   #include "util/u_dump.h"
   #include "util/u_prim.h"
   #include "util/u_string.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_intr.h"
   #include "gallivm/lp_bld_flow.h"
   #include "gallivm/lp_bld_pack.h"
   #include "gallivm/lp_bld_gather.h"
   #include "gallivm/lp_bld_coro.h"
   #include "gallivm/lp_bld_nir.h"
   #include "gallivm/lp_bld_jit_sample.h"
   #include "lp_state_cs.h"
   #include "lp_context.h"
   #include "lp_setup_context.h"
   #include "lp_debug.h"
   #include "lp_state.h"
   #include "lp_perf.h"
   #include "lp_screen.h"
   #include "lp_memory.h"
   #include "lp_query.h"
   #include "lp_cs_tpool.h"
   #include "frontend/sw_winsys.h"
   #include "nir/nir_to_tgsi_info.h"
   #include "nir/tgsi_to_nir.h"
   #include "util/mesa-sha1.h"
   #include "nir_serialize.h"
      #include "draw/draw_context.h"
   #include "draw/draw_llvm.h"
   #include "draw/draw_mesh_prim.h"
      /** Fragment shader number (for debugging) */
   static unsigned cs_no = 0;
   static unsigned task_no = 0;
   static unsigned mesh_no = 0;
      struct lp_cs_job_info {
      unsigned grid_size[3];
   unsigned iter_size[3];
   unsigned grid_base[3];
   unsigned block_size[3];
   unsigned req_local_mem;
   unsigned work_dim;
   unsigned draw_id;
   bool zero_initialize_shared_memory;
   bool use_iters;
   struct lp_cs_exec *current;
   struct vertex_header *io;
   size_t io_stride;
   void *payload;
      };
      enum {
      CS_ARG_CONTEXT,
   CS_ARG_RESOURCES,
   CS_ARG_BLOCK_X_SIZE,
   CS_ARG_BLOCK_Y_SIZE,
   CS_ARG_BLOCK_Z_SIZE,
   CS_ARG_GRID_X,
   CS_ARG_GRID_Y,
   CS_ARG_GRID_Z,
   CS_ARG_GRID_SIZE_X,
   CS_ARG_GRID_SIZE_Y,
   CS_ARG_GRID_SIZE_Z,
   CS_ARG_WORK_DIM,
   CS_ARG_DRAW_ID,
   CS_ARG_VERTEX_DATA,
   CS_ARG_PER_THREAD_DATA,
   CS_ARG_OUTER_COUNT,
   CS_ARG_CORO_X_LOOPS = CS_ARG_OUTER_COUNT,
   CS_ARG_CORO_PARTIALS,
   CS_ARG_CORO_BLOCK_X_SIZE,
   CS_ARG_CORO_BLOCK_Y_SIZE,
   CS_ARG_CORO_BLOCK_Z_SIZE,
   CS_ARG_CORO_IDX,
   CS_ARG_CORO_MEM,
   CS_ARG_CORO_OUTPUTS,
      };
      struct lp_mesh_llvm_iface {
               LLVMValueRef vertex_count;
   LLVMValueRef prim_count;
      };
      static inline const struct lp_mesh_llvm_iface *
   lp_mesh_llvm_iface(const struct lp_build_mesh_iface *iface)
   {
         }
         static LLVMTypeRef
   create_mesh_jit_output_type_deref(struct gallivm_state *gallivm)
   {
      LLVMTypeRef float_type = LLVMFloatTypeInContext(gallivm->context);
            output_array = LLVMArrayType(float_type, TGSI_NUM_CHANNELS); /* num channels */
   output_array = LLVMArrayType(output_array, PIPE_MAX_SHADER_OUTPUTS); /* num attrs per vertex */
      }
      static void
   lp_mesh_llvm_emit_store_output(const struct lp_build_mesh_iface *mesh_iface,
                                 struct lp_build_context *bld,
   unsigned name,
   bool is_vindex_indirect,
   LLVMValueRef vertex_index,
   bool is_aindex_indirect,
   {
      const struct lp_mesh_llvm_iface *mesh = lp_mesh_llvm_iface(mesh_iface);
   struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef indices[3];
   LLVMValueRef res;
   struct lp_type type = bld->type;
            if (is_vindex_indirect || is_aindex_indirect || is_sindex_indirect) {
      for (int i = 0; i < type.length; ++i) {
      LLVMValueRef idx = lp_build_const_int32(gallivm, i);
   LLVMValueRef vert_chan_index = vertex_index ? vertex_index : lp_build_const_int32(gallivm, 0);
   LLVMValueRef attr_chan_index = attrib_index;
                  if (is_vindex_indirect) {
      vert_chan_index = LLVMBuildExtractElement(builder,
      }
   if (is_aindex_indirect) {
      attr_chan_index = LLVMBuildExtractElement(builder,
               if (is_sindex_indirect) {
      swiz_chan_index = LLVMBuildExtractElement(builder,
               indices[0] = vert_chan_index;
                                    struct lp_build_if_state ifthen;
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, mask_vec, lp_build_const_int_vec(gallivm, bld->type, 0), "");
   cond = LLVMBuildExtractElement(gallivm->builder, cond, idx, "");
   lp_build_if(&ifthen, gallivm, cond);
   LLVMBuildStore(builder, res, channel_vec);
         } else {
      indices[0] = vertex_index ? vertex_index : lp_build_const_int32(gallivm, 0);
   indices[1] = attrib_index;
            res = LLVMBuildGEP2(builder, output_type, mesh->outputs, indices, 3, "");
   for (unsigned i = 0; i < type.length; ++i) {
                     struct lp_build_if_state ifthen;
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, mask_vec, lp_build_const_int_vec(gallivm, bld->type, 0), "");
   cond = LLVMBuildExtractElement(gallivm->builder, cond, idx, "");
   lp_build_if(&ifthen, gallivm, cond);
   LLVMBuildStore(builder, val, res);
            }
      static void
   lp_mesh_emit_vertex_and_primitive_count(const struct lp_build_mesh_iface *mesh_iface,
                     {
      const struct lp_mesh_llvm_iface *mesh = lp_mesh_llvm_iface(mesh_iface);
            LLVMBuildStore(gallivm->builder, vertices_count, mesh->vertex_count);
      }
      static void
   mesh_convert_to_aos(struct gallivm_state *gallivm,
                     nir_shader *nir,
   bool vert_only,
   LLVMTypeRef io_type,
   LLVMValueRef io,
   LLVMValueRef outputs,
   LLVMValueRef clipmask,
   LLVMValueRef vertex_index,
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef inds[3];
      #if DEBUG_STORE
         #endif
      int first_per_prim_attrib = -1;
   nir_foreach_shader_out_variable(var, nir) {
      if (var->data.per_primitive) {
      first_per_prim_attrib = var->data.driver_location;
         }
               if (vert_only && var->data.per_primitive)
         if (!vert_only && !var->data.per_primitive)
         int attrib = var->data.driver_location;
            for (unsigned s = 0; s < slots; s++) {
      LLVMValueRef soa[TGSI_NUM_CHANNELS];
   LLVMValueRef aos[LP_MAX_VECTOR_WIDTH / 32];
   for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
      inds[0] = vertex_index;
                  LLVMValueRef res = LLVMBuildGEP2(builder, output_type, outputs, inds, 3, "");
   LLVMTypeRef single_type = (attrib == primid_slot) ? lp_build_int_elem_type(gallivm, soa_type) : lp_build_elem_type(gallivm, soa_type);
   #if DEBUG_STORE
               lp_build_printf(gallivm, "output %d : %d ",
                  LLVMConstInt(LLVMInt32TypeInContext(gallivm->context),
      lp_build_print_value(gallivm, "val = ", out);
   {
                  #endif
                  }
   LLVMTypeRef float_type = LLVMFloatTypeInContext(gallivm->context);
   aos[0] = LLVMGetUndef(LLVMVectorType(float_type, 4));
   for (unsigned i = 0; i <  4; i++)
         int aos_attrib = attrib;
   if (var->data.per_primitive)
         draw_store_aos_array(gallivm,
                        soa_type,
   io_type,
   io,
   NULL,
               #if DEBUG_STORE
         #endif
   }
      static void
   generate_compute(struct llvmpipe_context *lp,
               {
      struct gallivm_state *gallivm = variant->gallivm;
   struct nir_shader *nir = shader->base.ir.nir;
   const struct lp_compute_shader_variant_key *key = &variant->key;
   char func_name[64], func_name_coro[64];
   LLVMTypeRef arg_types[CS_ARG_MAX];
   LLVMTypeRef func_type, coro_func_type;
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef context_ptr, resources_ptr;
   LLVMValueRef block_x_size_arg, block_y_size_arg, block_z_size_arg;
   LLVMValueRef grid_x_arg, grid_y_arg, grid_z_arg;
   LLVMValueRef grid_size_x_arg, grid_size_y_arg, grid_size_z_arg;
   LLVMValueRef work_dim_arg, draw_id_arg, thread_data_ptr, io_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   struct lp_build_sampler_soa *sampler;
   struct lp_build_image_soa *image;
   LLVMValueRef function, coro;
   struct lp_type cs_type;
   struct lp_mesh_llvm_iface mesh_iface;
   bool is_mesh = nir->info.stage == MESA_SHADER_MESH;
                     /*
   * This function has two parts
   * a) setup the coroutine execution environment loop.
   * b) build the compute shader llvm for use inside the coroutine.
   */
            memset(&cs_type, 0, sizeof cs_type);
   cs_type.floating = true;      /* floating point values */
   cs_type.sign = true;          /* values are signed */
   cs_type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   cs_type.width = 32;           /* 32-bit float */
   cs_type.length = MIN2(lp_native_vector_width / 32, 16); /* n*4 elements per vector */
                     arg_types[CS_ARG_CONTEXT] = variant->jit_cs_context_ptr_type;       /* context */
   arg_types[CS_ARG_RESOURCES]=  variant->jit_resources_ptr_type;
   arg_types[CS_ARG_BLOCK_X_SIZE] = int32_type;                        /* block_x_size */
   arg_types[CS_ARG_BLOCK_Y_SIZE] = int32_type;                        /* block_y_size */
   arg_types[CS_ARG_BLOCK_Z_SIZE] = int32_type;                        /* block_z_size */
   arg_types[CS_ARG_GRID_X] = int32_type;                              /* grid_x */
   arg_types[CS_ARG_GRID_Y] = int32_type;                              /* grid_y */
   arg_types[CS_ARG_GRID_Z] = int32_type;                              /* grid_z */
   arg_types[CS_ARG_GRID_SIZE_X] = int32_type;                         /* grid_size_x */
   arg_types[CS_ARG_GRID_SIZE_Y] = int32_type;                         /* grid_size_y */
   arg_types[CS_ARG_GRID_SIZE_Z] = int32_type;                         /* grid_size_z */
   arg_types[CS_ARG_WORK_DIM] = int32_type;                            /* work dim */
   arg_types[CS_ARG_DRAW_ID] = int32_type;                             /* draw id */
   if (variant->jit_vertex_header_ptr_type)
         else
         arg_types[CS_ARG_PER_THREAD_DATA] = variant->jit_cs_thread_data_ptr_type;  /* per thread data */
   arg_types[CS_ARG_CORO_X_LOOPS] = int32_type;                        /* coro only - num X loops */
   arg_types[CS_ARG_CORO_PARTIALS] = int32_type;                       /* coro only - partials */
   arg_types[CS_ARG_CORO_BLOCK_X_SIZE] = int32_type;                   /* coro block_x_size */
   arg_types[CS_ARG_CORO_BLOCK_Y_SIZE] = int32_type;                   /* coro block_y_size */
   arg_types[CS_ARG_CORO_BLOCK_Z_SIZE] = int32_type;                   /* coro block_z_size */
   arg_types[CS_ARG_CORO_IDX] = int32_type;                            /* coro idx */
   arg_types[CS_ARG_CORO_MEM] = LLVMPointerType(LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0), 0);
            func_type = LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context),
            coro_func_type = LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0),
            function = LLVMAddFunction(gallivm->module, func_name, func_type);
            coro = LLVMAddFunction(gallivm->module, func_name_coro, coro_func_type);
   LLVMSetFunctionCallConv(coro, LLVMCCallConv);
                     for (i = 0; i < CS_ARG_MAX - !is_mesh; ++i) {
      if (LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind) {
      lp_add_function_attr(coro, i + 1, LP_FUNC_ATTR_NOALIAS);
   if (i < CS_ARG_OUTER_COUNT)
                  if (variant->gallivm->cache->data_size)
            context_ptr  = LLVMGetParam(function, CS_ARG_CONTEXT);
   resources_ptr  = LLVMGetParam(function, CS_ARG_RESOURCES);
   block_x_size_arg = LLVMGetParam(function, CS_ARG_BLOCK_X_SIZE);
   block_y_size_arg = LLVMGetParam(function, CS_ARG_BLOCK_Y_SIZE);
   block_z_size_arg = LLVMGetParam(function, CS_ARG_BLOCK_Z_SIZE);
   grid_x_arg = LLVMGetParam(function, CS_ARG_GRID_X);
   grid_y_arg = LLVMGetParam(function, CS_ARG_GRID_Y);
   grid_z_arg = LLVMGetParam(function, CS_ARG_GRID_Z);
   grid_size_x_arg = LLVMGetParam(function, CS_ARG_GRID_SIZE_X);
   grid_size_y_arg = LLVMGetParam(function, CS_ARG_GRID_SIZE_Y);
   grid_size_z_arg = LLVMGetParam(function, CS_ARG_GRID_SIZE_Z);
   work_dim_arg = LLVMGetParam(function, CS_ARG_WORK_DIM);
   draw_id_arg = LLVMGetParam(function, CS_ARG_DRAW_ID);
   io_ptr = LLVMGetParam(function, CS_ARG_VERTEX_DATA);
            lp_build_name(context_ptr, "context");
   lp_build_name(resources_ptr, "resources");
   lp_build_name(block_x_size_arg, "x_size");
   lp_build_name(block_y_size_arg, "y_size");
   lp_build_name(block_z_size_arg, "z_size");
   lp_build_name(grid_x_arg, "grid_x");
   lp_build_name(grid_y_arg, "grid_y");
   lp_build_name(grid_z_arg, "grid_z");
   lp_build_name(grid_size_x_arg, "grid_size_x");
   lp_build_name(grid_size_y_arg, "grid_size_y");
   lp_build_name(grid_size_z_arg, "grid_size_z");
   lp_build_name(work_dim_arg, "work_dim");
   lp_build_name(draw_id_arg, "draw_id");
   lp_build_name(thread_data_ptr, "thread_data");
            lp_build_nir_prepasses(nir);
            if (exec_list_length(&nir->functions) > 1) {
      LLVMTypeRef call_context_type = lp_build_cs_func_call_context(gallivm, cs_type.length,
               nir_foreach_function(func, nir) {
                                             args[0] = LLVMVectorType(LLVMInt32TypeInContext(gallivm->context), cs_type.length); /* mask */
   args[1] = LLVMPointerType(call_context_type, 0);
   for (int i = 0; i < func->num_params; i++) {
      args[i + LP_RESV_FUNC_ARGS] = LLVMVectorType(LLVMIntTypeInContext(gallivm->context, func->params[i].bit_size), cs_type.length);
   if (func->params[i].num_components > 1)
               LLVMTypeRef func_type = LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context),
                        struct lp_build_fn *new_fn = ralloc(fns, struct lp_build_fn);
   new_fn->fn_type = func_type;
   new_fn->fn = lfunc;
               nir_foreach_function(func, nir) {
                     struct hash_entry *entry = _mesa_hash_table_search(fns, func);
   assert(entry);
   struct lp_build_fn *new_fn = entry->data;
                  builder = gallivm->builder;
   LLVMPositionBuilderAtEnd(builder, block);
   LLVMValueRef mask_param = LLVMGetParam(lfunc, 0);
   LLVMValueRef call_context_ptr = LLVMGetParam(lfunc, 1);
   LLVMValueRef call_context = LLVMBuildLoad2(builder, call_context_type, call_context_ptr, "");
                                          struct lp_build_tgsi_params params;
   memset(&params, 0, sizeof(params));
   params.type = cs_type;
   params.mask = &mask;
   params.fns = fns;
   params.current_func = lfunc;
   params.context_type = variant->jit_cs_context_type;
   params.resources_type = variant->jit_resources_type;
   params.call_context_ptr = call_context_ptr;
   params.context_ptr = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_CONTEXT, "");
   params.resources_ptr = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_RESOURCES, "");
   params.shared_ptr = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_SHARED, "");
   params.scratch_ptr = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_SCRATCH, "");
   system_values.work_dim = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_WORK_DIM, "");
   system_values.thread_id[0] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_THREAD_ID_0, "");
   system_values.thread_id[1] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_THREAD_ID_1, "");
   system_values.thread_id[2] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_THREAD_ID_2, "");
   system_values.block_id[0] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_BLOCK_ID_0, "");
   system_values.block_id[1] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_BLOCK_ID_1, "");
   system_values.block_id[2] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_BLOCK_ID_2, "");
   system_values.grid_size[0] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_GRID_SIZE_0, "");
   system_values.grid_size[1] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_GRID_SIZE_1, "");
   system_values.grid_size[2] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_GRID_SIZE_2, "");
   system_values.block_size[0] = LLVMBuildExtractValue(builder, call_context, LP_NIR_CALL_CONTEXT_BLOCK_SIZE_0, "");
                           params.consts_ptr = lp_jit_resources_constants(gallivm,
               params.ssbo_ptr = lp_jit_resources_ssbos(gallivm,
               lp_build_nir_soa_func(gallivm, shader->base.ir.nir,
                                 LLVMBuildRetVoid(builder);
                  block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   builder = gallivm->builder;
   assert(builder);
   LLVMPositionBuilderAtEnd(builder, block);
   sampler = lp_llvm_sampler_soa_create(lp_cs_variant_key_samplers(key),
                        if (is_mesh) {
      LLVMTypeRef output_type = create_mesh_jit_output_type_deref(gallivm);
               struct lp_build_loop_state loop_state[4];
   LLVMValueRef num_x_loop;
   LLVMValueRef vec_length = lp_build_const_int32(gallivm, cs_type.length);
   num_x_loop = LLVMBuildAdd(gallivm->builder, block_x_size_arg, vec_length, "");
   num_x_loop = LLVMBuildSub(gallivm->builder, num_x_loop, lp_build_const_int32(gallivm, 1), "");
   num_x_loop = LLVMBuildUDiv(gallivm->builder, num_x_loop, vec_length, "");
            LLVMValueRef coro_num_hdls = LLVMBuildMul(gallivm->builder, num_x_loop, block_y_size_arg, "");
            /* build a ptr in memory to store all the frames in later. */
   LLVMTypeRef hdl_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0);
   LLVMValueRef coro_mem = LLVMBuildAlloca(gallivm->builder, hdl_ptr_type, "coro_mem");
                              /*
   * This is the main coroutine execution loop. It iterates over the dimensions
   * and calls the coroutine main entrypoint on the first pass, but in subsequent
   * passes it checks if the coroutine has completed and resumes it if not.
   */
   /* take x_width - round up to type.length width */
   lp_build_loop_begin(&loop_state[3], gallivm,
         lp_build_loop_begin(&loop_state[2], gallivm,
         lp_build_loop_begin(&loop_state[1], gallivm,
         lp_build_loop_begin(&loop_state[0], gallivm,
         {
      LLVMValueRef args[CS_ARG_MAX];
   args[CS_ARG_CONTEXT] = context_ptr;
   args[CS_ARG_RESOURCES] = resources_ptr;
   args[CS_ARG_BLOCK_X_SIZE] = loop_state[0].counter;
   args[CS_ARG_BLOCK_Y_SIZE] = loop_state[1].counter;
   args[CS_ARG_BLOCK_Z_SIZE] = loop_state[2].counter;
   args[CS_ARG_GRID_X] = grid_x_arg;
   args[CS_ARG_GRID_Y] = grid_y_arg;
   args[CS_ARG_GRID_Z] = grid_z_arg;
   args[CS_ARG_GRID_SIZE_X] = grid_size_x_arg;
   args[CS_ARG_GRID_SIZE_Y] = grid_size_y_arg;
   args[CS_ARG_GRID_SIZE_Z] = grid_size_z_arg;
   args[CS_ARG_WORK_DIM] = work_dim_arg;
   args[CS_ARG_DRAW_ID] = draw_id_arg;
   args[CS_ARG_VERTEX_DATA] = io_ptr;
   args[CS_ARG_PER_THREAD_DATA] = thread_data_ptr;
   args[CS_ARG_CORO_X_LOOPS] = num_x_loop;
   args[CS_ARG_CORO_PARTIALS] = partials;
   args[CS_ARG_CORO_BLOCK_X_SIZE] = block_x_size_arg;
   args[CS_ARG_CORO_BLOCK_Y_SIZE] = block_y_size_arg;
            /* idx = (z * (size_x * size_y) + y * size_x + x */
   LLVMValueRef coro_hdl_idx = LLVMBuildMul(gallivm->builder, loop_state[2].counter,
         coro_hdl_idx = LLVMBuildAdd(gallivm->builder, coro_hdl_idx,
               coro_hdl_idx = LLVMBuildAdd(gallivm->builder, coro_hdl_idx,
                              if (is_mesh)
                              struct lp_build_if_state ifstate;
   LLVMValueRef cmp = LLVMBuildICmp(gallivm->builder, LLVMIntEQ, loop_state[3].counter,
         /* first time here - call the coroutine function entry point */
   lp_build_if(&ifstate, gallivm, cmp);
   LLVMValueRef coro_ret = LLVMBuildCall2(gallivm->builder, coro_func_type, coro, args, CS_ARG_MAX - !is_mesh, "");
   LLVMBuildStore(gallivm->builder, coro_ret, coro_entry);
   lp_build_else(&ifstate);
   /* subsequent calls for this invocation - check if done. */
   LLVMValueRef coro_done = lp_build_coro_done(gallivm, coro_hdl);
   struct lp_build_if_state ifstate2;
   lp_build_if(&ifstate2, gallivm, coro_done);
   /* if done destroy and force loop exit */
   lp_build_coro_destroy(gallivm, coro_hdl);
   lp_build_loop_force_set_counter(&loop_state[3], lp_build_const_int32(gallivm, end_coroutine - 1));
   lp_build_else(&ifstate2);
   /* otherwise resume the coroutine */
   lp_build_coro_resume(gallivm, coro_hdl);
   lp_build_endif(&ifstate2);
   lp_build_endif(&ifstate);
      }
   lp_build_loop_end_cond(&loop_state[0],
               lp_build_loop_end_cond(&loop_state[1],
               lp_build_loop_end_cond(&loop_state[2],
               lp_build_loop_end_cond(&loop_state[3],
                  LLVMValueRef coro_mem_ptr = LLVMBuildLoad2(builder, hdl_ptr_type, coro_mem, "");
   LLVMTypeRef mem_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0);
   LLVMTypeRef free_type = LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context), &mem_ptr_type, 1, 0);
                     /* This is stage (b) - generate the compute shader code inside the coroutine. */
   LLVMValueRef x_size_arg, y_size_arg, z_size_arg;
   context_ptr  = LLVMGetParam(coro, CS_ARG_CONTEXT);
   resources_ptr = LLVMGetParam(coro, CS_ARG_RESOURCES);
   x_size_arg = LLVMGetParam(coro, CS_ARG_BLOCK_X_SIZE);
   y_size_arg = LLVMGetParam(coro, CS_ARG_BLOCK_Y_SIZE);
   z_size_arg = LLVMGetParam(coro, CS_ARG_BLOCK_Z_SIZE);
   grid_x_arg = LLVMGetParam(coro, CS_ARG_GRID_X);
   grid_y_arg = LLVMGetParam(coro, CS_ARG_GRID_Y);
   grid_z_arg = LLVMGetParam(coro, CS_ARG_GRID_Z);
   grid_size_x_arg = LLVMGetParam(coro, CS_ARG_GRID_SIZE_X);
   grid_size_y_arg = LLVMGetParam(coro, CS_ARG_GRID_SIZE_Y);
   grid_size_z_arg = LLVMGetParam(coro, CS_ARG_GRID_SIZE_Z);
   work_dim_arg = LLVMGetParam(coro, CS_ARG_WORK_DIM);
   draw_id_arg = LLVMGetParam(coro, CS_ARG_DRAW_ID);
   io_ptr = LLVMGetParam(coro, CS_ARG_VERTEX_DATA);
   thread_data_ptr  = LLVMGetParam(coro, CS_ARG_PER_THREAD_DATA);
   num_x_loop = LLVMGetParam(coro, CS_ARG_CORO_X_LOOPS);
   partials = LLVMGetParam(coro, CS_ARG_CORO_PARTIALS);
   block_x_size_arg = LLVMGetParam(coro, CS_ARG_CORO_BLOCK_X_SIZE);
   block_y_size_arg = LLVMGetParam(coro, CS_ARG_CORO_BLOCK_Y_SIZE);
   block_z_size_arg = LLVMGetParam(coro, CS_ARG_CORO_BLOCK_Z_SIZE);
   LLVMValueRef coro_idx = LLVMGetParam(coro, CS_ARG_CORO_IDX);
   coro_mem = LLVMGetParam(coro, CS_ARG_CORO_MEM);
   if (is_mesh)
         block = LLVMAppendBasicBlockInContext(gallivm->context, coro, "entry");
   LLVMPositionBuilderAtEnd(builder, block);
   {
      LLVMValueRef consts_ptr;
   LLVMValueRef ssbo_ptr;
   LLVMValueRef shared_ptr;
   LLVMValueRef payload_ptr;
   LLVMValueRef kernel_args_ptr;
   struct lp_build_mask_context mask;
            memset(&system_values, 0, sizeof(system_values));
   consts_ptr = lp_jit_resources_constants(gallivm, variant->jit_resources_type, resources_ptr);
   ssbo_ptr = lp_jit_resources_ssbos(gallivm, variant->jit_resources_type, resources_ptr);
   kernel_args_ptr = lp_jit_cs_context_kernel_args(gallivm,
                  shared_ptr = lp_jit_cs_thread_data_shared(gallivm,
               payload_ptr = lp_jit_cs_thread_data_payload(gallivm,
                  LLVMValueRef coro_num_hdls = LLVMBuildMul(gallivm->builder, num_x_loop, block_y_size_arg, "");
            /* these are coroutine entrypoint necessities */
   LLVMValueRef coro_id = lp_build_coro_id(gallivm);
   LLVMValueRef coro_entry = lp_build_coro_alloc_mem_array(gallivm, coro_mem, coro_idx, coro_num_hdls);
   LLVMTypeRef mem_ptr_type = LLVMInt8TypeInContext(gallivm->context);
   LLVMValueRef alloced_ptr = LLVMBuildLoad2(gallivm->builder, hdl_ptr_type, coro_mem, "");
   alloced_ptr = LLVMBuildGEP2(gallivm->builder, mem_ptr_type, alloced_ptr, &coro_entry, 1, "");
   LLVMValueRef coro_hdl = lp_build_coro_begin(gallivm, coro_id, alloced_ptr);
   LLVMValueRef has_partials = LLVMBuildICmp(gallivm->builder, LLVMIntNE, partials, lp_build_const_int32(gallivm, 0), "");
   LLVMValueRef tids_x[LP_MAX_VECTOR_LENGTH], tids_y[LP_MAX_VECTOR_LENGTH], tids_z[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef base_val = LLVMBuildMul(gallivm->builder, x_size_arg, vec_length, "");
   for (i = 0; i < cs_type.length; i++) {
      tids_x[i] = LLVMBuildAdd(gallivm->builder, base_val, lp_build_const_int32(gallivm, i), "");
   tids_y[i] = y_size_arg;
      }
   system_values.thread_id[0] = lp_build_gather_values(gallivm, tids_x, cs_type.length);
   system_values.thread_id[1] = lp_build_gather_values(gallivm, tids_y, cs_type.length);
            system_values.block_id[0] = grid_x_arg;
   system_values.block_id[1] = grid_y_arg;
            system_values.grid_size[0] = grid_size_x_arg;
   system_values.grid_size[1] = grid_size_y_arg;
            system_values.work_dim = work_dim_arg;
            /* subgroup_id = ((z * block_size_x * block_size_y) + (y * block_size_x) + x) / subgroup_size
   *
   * this breaks if z or y is zero, so distribute the division to preserve ids
   *
   * subgroup_id = ((z * block_size_x * block_size_y) / subgroup_size) + ((y * block_size_x) / subgroup_size) + (x / subgroup_size)
   *
   * except "x" is pre-divided here
   *
   * subgroup_id = ((z * block_size_x * block_size_y) / subgroup_size) + ((y * block_size_x) / subgroup_size) + x
   */
   LLVMValueRef subgroup_id = LLVMBuildUDiv(builder,
               subgroup_id = LLVMBuildAdd(gallivm->builder,
                     subgroup_id = LLVMBuildAdd(gallivm->builder, subgroup_id, x_size_arg, "");
   system_values.subgroup_id = subgroup_id;
   LLVMValueRef num_subgroups = LLVMBuildUDiv(builder,
                     LLVMValueRef subgroup_cmp = LLVMBuildICmp(gallivm->builder, LLVMIntEQ, num_subgroups, lp_build_const_int32(gallivm, 0), "");
            system_values.block_size[0] = block_x_size_arg;
   system_values.block_size[1] = block_y_size_arg;
            LLVMValueRef last_x_loop = LLVMBuildICmp(gallivm->builder, LLVMIntEQ, x_size_arg, LLVMBuildSub(gallivm->builder, num_x_loop, lp_build_const_int32(gallivm, 1), ""), "");
   LLVMValueRef use_partial_mask = LLVMBuildAnd(gallivm->builder, last_x_loop, has_partials, "");
   struct lp_build_if_state if_state;
   LLVMTypeRef mask_type = LLVMVectorType(int32_type, cs_type.length);
   LLVMValueRef mask_val = lp_build_alloca(gallivm, mask_type, "mask");
   LLVMValueRef full_mask_val = lp_build_const_int_vec(gallivm, cs_type, ~0);
            lp_build_if(&if_state, gallivm, use_partial_mask);
   struct lp_build_loop_state mask_loop_state;
   lp_build_loop_begin(&mask_loop_state, gallivm, partials);
   LLVMValueRef tmask_val = LLVMBuildLoad2(gallivm->builder, mask_type, mask_val, "");
   tmask_val = LLVMBuildInsertElement(gallivm->builder, tmask_val, lp_build_const_int32(gallivm, 0), mask_loop_state.counter, "");
   LLVMBuildStore(gallivm->builder, tmask_val, mask_val);
   lp_build_loop_end_cond(&mask_loop_state, vec_length, NULL, LLVMIntUGE);
            mask_val = LLVMBuildLoad2(gallivm->builder, mask_type, mask_val, "");
                     LLVMBasicBlockRef sus_block = LLVMAppendBasicBlockInContext(gallivm->context, coro, "suspend");
            coro_info.suspend = sus_block;
            if (is_mesh) {
      LLVMValueRef vertex_count = lp_build_alloca(gallivm, LLVMInt32TypeInContext(gallivm->context), "vertex_count");
   LLVMValueRef primitive_count = lp_build_alloca(gallivm, LLVMInt32TypeInContext(gallivm->context), "prim_count");
   mesh_iface.base.emit_store_output = lp_mesh_llvm_emit_store_output;
   mesh_iface.base.emit_vertex_and_primitive_count = lp_mesh_emit_vertex_and_primitive_count;
   mesh_iface.vertex_count = vertex_count;
   mesh_iface.prim_count = primitive_count;
               struct lp_build_tgsi_params params;
            params.type = cs_type;
   params.mask = &mask;
   params.consts_ptr = consts_ptr;
   params.system_values = &system_values;
   params.context_type = variant->jit_cs_context_type;
   params.context_ptr = context_ptr;
   params.resources_type = variant->jit_resources_type;
   params.resources_ptr = resources_ptr;
   params.sampler = sampler;
   params.ssbo_ptr = ssbo_ptr;
   params.image = image;
   params.shared_ptr = shared_ptr;
   params.payload_ptr = payload_ptr;
   params.coro = &coro_info;
   params.kernel_args = kernel_args_ptr;
   params.aniso_filter_table = lp_jit_resources_aniso_filter_table(gallivm,
                        params.current_func = NULL;
   params.fns = fns;
   lp_build_nir_soa_func(gallivm, nir,
                  if (is_mesh) {
      LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
                  struct lp_build_if_state iter0state;
   LLVMValueRef is_iter0 = LLVMBuildICmp(gallivm->builder, LLVMIntEQ, coro_idx,
                        LLVMValueRef vert_count_ptr, prim_count_ptr;
   LLVMValueRef indices = lp_build_const_int32(gallivm, 1);
   vert_count_ptr = LLVMBuildGEP2(gallivm->builder, i32t, io_ptr, &indices, 1, "");
                  lp_build_if(&iter0state, gallivm, is_iter0);
   LLVMBuildStore(gallivm->builder, vertex_count, vert_count_ptr);
                                                         int per_prim_count = util_bitcount64(nir->info.per_primitive_outputs);
   int out_count = util_bitcount64(nir->info.outputs_written);
   int per_vert_count = out_count - per_prim_count;
   int vsize = (sizeof(struct vertex_header) + per_vert_count * 4 * sizeof(float)) * 8;
                  lp_build_loop_begin(&vertex_loop_state, gallivm,
         LLVMValueRef io;
   io = LLVMBuildPtrToInt(gallivm->builder, io_ptr, LLVMInt64TypeInContext(gallivm->context),  "");
   io = LLVMBuildAdd(builder, io, LLVMBuildZExt(builder, LLVMBuildMul(builder, vertex_loop_state.counter, lp_build_const_int32(gallivm, vsize), ""), LLVMInt64TypeInContext(gallivm->context), ""), "");
   io = LLVMBuildIntToPtr(gallivm->builder, io, LLVMPointerType(LLVMVoidTypeInContext(gallivm->context), 0), "");
   mesh_convert_to_aos(gallivm, shader->base.ir.nir, true, variant->jit_vertex_header_type,
               lp_build_loop_end_cond(&vertex_loop_state,
                  struct lp_build_loop_state prim_loop_state;
   lp_build_loop_begin(&prim_loop_state, gallivm,
         io = LLVMBuildPtrToInt(gallivm->builder, io_ptr, LLVMInt64TypeInContext(gallivm->context),  "");
   LLVMValueRef prim_offset = LLVMBuildMul(builder, prim_loop_state.counter, lp_build_const_int32(gallivm, psize), "");
   prim_offset = LLVMBuildAdd(builder, prim_offset, lp_build_const_int32(gallivm, vsize * (nir->info.mesh.max_vertices_out + 8)), "");
   io = LLVMBuildAdd(builder, io, LLVMBuildZExt(builder, prim_offset, LLVMInt64TypeInContext(gallivm->context), ""), "");
   io = LLVMBuildIntToPtr(gallivm->builder, io, LLVMPointerType(LLVMVoidTypeInContext(gallivm->context), 0), "");
   mesh_convert_to_aos(gallivm, shader->base.ir.nir, false, variant->jit_prim_type,
               lp_build_loop_end_cond(&prim_loop_state,
                              lp_build_coro_suspend_switch(gallivm, &coro_info, NULL, true);
            LLVMBuildBr(builder, sus_block);
            lp_build_coro_end(gallivm, coro_hdl);
               lp_bld_llvm_sampler_soa_destroy(sampler);
   lp_bld_llvm_image_soa_destroy(image);
            gallivm_verify_function(gallivm, coro);
      }
         static void *
   llvmpipe_create_compute_state(struct pipe_context *pipe,
         {
      struct lp_compute_shader *shader = CALLOC_STRUCT(lp_compute_shader);
   struct nir_shader *nir = NULL;
   if (!shader)
                              if (templ->ir_type == PIPE_SHADER_IR_TGSI) {
         } else if (templ->ir_type == PIPE_SHADER_IR_NIR_SERIALIZED) {
      struct blob_reader reader;
            blob_reader_init(&reader, hdr->blob, hdr->num_bytes);
               } else if (templ->ir_type == PIPE_SHADER_IR_NIR) {
                  nir = (struct nir_shader *)shader->base.ir.nir;
   shader->req_local_mem += nir->info.shared_size;
                              int nr_samplers = BITSET_LAST_BIT(nir->info.samplers_used);
   int nr_sampler_views = BITSET_LAST_BIT(nir->info.textures_used);
   int nr_images = BITSET_LAST_BIT(nir->info.images_used);
               }
         static void
   llvmpipe_bind_compute_state(struct pipe_context *pipe,
         {
               if (llvmpipe->cs == cs)
            llvmpipe->cs = (struct lp_compute_shader *)cs;
      }
      static void
   llvmpipe_get_compute_state_info(struct pipe_context *pipe, void *cs,
         {
      struct lp_compute_shader* shader = cs;
            info->max_threads = 1024;
   info->simd_sizes = lp_native_vector_width / 32;
   info->preferred_simd_size = info->simd_sizes;
   // TODO: this is a bad estimate, but not much we can do without actually compiling the shaders
      }
         /**
   * Remove shader variant from two lists: the shader's variant list
   * and the context's variant list.
   */
   static void
   llvmpipe_remove_cs_shader_variant(struct llvmpipe_context *lp,
         {
      if ((LP_DEBUG & DEBUG_CS) || (gallivm_debug & GALLIVM_DEBUG_IR)) {
      debug_printf("llvmpipe: del cs #%u var %u v created %u v cached %u "
               "v total cached %u inst %u total inst %u\n",
   variant->shader->no, variant->no,
                        /* remove from shader's list */
   list_del(&variant->list_item_local.list);
            /* remove from context's list */
   list_del(&variant->list_item_global.list);
   lp->nr_cs_variants--;
               }
         static void
   llvmpipe_delete_compute_state(struct pipe_context *pipe,
         {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_compute_shader *shader = cs;
                     if (llvmpipe->cs == cs)
         for (unsigned i = 0; i < shader->max_global_buffers; i++)
                  /* Delete all the variants */
   LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
         }
   ralloc_free(shader->base.ir.nir);
      }
         static struct lp_compute_shader_variant_key *
   make_variant_key(struct llvmpipe_context *lp,
                     {
      struct lp_compute_shader_variant_key *key =
                  struct nir_shader *nir = (struct nir_shader *)shader->base.ir.nir;
   /* This value will be the same for all the variants of a given shader:
   */
   key->nr_samplers = BITSET_LAST_BIT(nir->info.samplers_used);
   key->nr_sampler_views = BITSET_LAST_BIT(nir->info.textures_used);
                     memset(cs_sampler, 0, MAX2(key->nr_samplers, key->nr_sampler_views) * sizeof *cs_sampler);
   for (unsigned i = 0; i < key->nr_samplers; ++i) {
      if (BITSET_TEST(nir->info.samplers_used, i)) {
      lp_sampler_static_sampler_state(&cs_sampler[i].sampler_state,
                  /*
   * XXX If TGSI_FILE_SAMPLER_VIEW exists assume all texture opcodes
   * are dx10-style? Can't really have mixed opcodes, at least not
   * if we want to skip the holes here (without rescanning tgsi).
   */
   if (!BITSET_IS_EMPTY(nir->info.textures_used)) {
      for (unsigned i = 0; i < key->nr_sampler_views; ++i) {
      /*
   * Note sview may exceed what's representable by file_mask.
   * This will still work, the only downside is that not actually
   * used views may be included in the shader key.
   */
   if (BITSET_TEST(nir->info.textures_used, i)) {
      lp_sampler_static_texture_state(&cs_sampler[i].texture_state,
            } else {
      key->nr_sampler_views = key->nr_samplers;
   for (unsigned i = 0; i < key->nr_sampler_views; ++i) {
      if (BITSET_TEST(nir->info.samplers_used, i)) {
      lp_sampler_static_texture_state(&cs_sampler[i].texture_state,
                     struct lp_image_static_state *lp_image;
   lp_image = lp_cs_variant_key_images(key);
            if (key->nr_images)
      memset(lp_image, 0,
      for (unsigned i = 0; i < key->nr_images; ++i) {
      if (BITSET_TEST(nir->info.images_used, i)) {
      lp_sampler_static_texture_state_image(&lp_image[i].image_state,
         }
      }
         static void
   dump_cs_variant_key(const struct lp_compute_shader_variant_key *key)
   {
      int i;
            for (i = 0; i < key->nr_samplers; ++i) {
      const struct lp_sampler_static_state *samplers = lp_cs_variant_key_samplers(key);
   const struct lp_static_sampler_state *sampler = &samplers[i].sampler_state;
   debug_printf("sampler[%u] = \n", i);
   debug_printf("  .wrap = %s %s %s\n",
               util_str_tex_wrap(sampler->wrap_s, true),
   debug_printf("  .min_img_filter = %s\n",
         debug_printf("  .min_mip_filter = %s\n",
         debug_printf("  .mag_img_filter = %s\n",
         if (sampler->compare_mode != PIPE_TEX_COMPARE_NONE)
         debug_printf("  .normalized_coords = %u\n", sampler->normalized_coords);
   debug_printf("  .min_max_lod_equal = %u\n", sampler->min_max_lod_equal);
   debug_printf("  .lod_bias_non_zero = %u\n", sampler->lod_bias_non_zero);
   debug_printf("  .apply_min_lod = %u\n", sampler->apply_min_lod);
   debug_printf("  .apply_max_lod = %u\n", sampler->apply_max_lod);
      }
   for (i = 0; i < key->nr_sampler_views; ++i) {
      const struct lp_sampler_static_state *samplers = lp_cs_variant_key_samplers(key);
   const struct lp_static_texture_state *texture = &samplers[i].texture_state;
   debug_printf("texture[%u] = \n", i);
   debug_printf("  .format = %s\n",
         debug_printf("  .target = %s\n",
         debug_printf("  .level_zero_only = %u\n",
         debug_printf("  .pot = %u %u %u\n",
                  }
   struct lp_image_static_state *images = lp_cs_variant_key_images(key);
   for (i = 0; i < key->nr_images; ++i) {
      const struct lp_static_texture_state *image = &images[i].image_state;
   debug_printf("image[%u] = \n", i);
   debug_printf("  .format = %s\n",
         debug_printf("  .target = %s\n",
         debug_printf("  .level_zero_only = %u\n",
         debug_printf("  .pot = %u %u %u\n",
                     }
         static void
   lp_debug_cs_variant(const struct lp_compute_shader_variant *variant)
   {
      debug_printf("llvmpipe: Compute shader #%u variant #%u:\n",
         nir_print_shader(variant->shader->base.ir.nir, stderr);
   dump_cs_variant_key(&variant->key);
      }
         static void
   lp_cs_get_ir_cache_key(struct lp_compute_shader_variant *variant,
         {
      struct blob blob = { 0 };
   unsigned ir_size;
            blob_init(&blob);
   nir_serialize(&blob, variant->shader->base.ir.nir, true);
   ir_binary = blob.data;
            struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, &variant->key, variant->shader->variant_key_size);
   _mesa_sha1_update(&ctx, ir_binary, ir_size);
               }
         static struct lp_compute_shader_variant *
   generate_variant(struct llvmpipe_context *lp,
                     {
               struct lp_compute_shader_variant *variant =
         if (!variant)
                     char module_name[64];
   const char *shname = sh_type == PIPE_SHADER_MESH ? "ms" :
         snprintf(module_name, sizeof(module_name), "%s%u_variant%u",
            variant->shader = shader;
            unsigned char ir_sha1_cache_key[20];
   struct lp_cached_code cached = { 0 };
                     lp_disk_cache_find_shader(screen, &cached, ir_sha1_cache_key);
   if (!cached.data_size)
            variant->gallivm = gallivm_create(module_name, lp->context, &cached);
   if (!variant->gallivm) {
      FREE(variant);
               variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
            if ((LP_DEBUG & DEBUG_CS) || (gallivm_debug & GALLIVM_DEBUG_IR)) {
                           if (sh_type == PIPE_SHADER_MESH) {
      struct nir_shader *nir = shader->base.ir.nir;
   int per_prim_count = util_bitcount64(nir->info.per_primitive_outputs);
   int out_count = util_bitcount64(nir->info.outputs_written);
   int per_vert_count = out_count - per_prim_count;
   variant->jit_vertex_header_type = lp_build_create_jit_vertex_header_type(variant->gallivm, per_vert_count);
   variant->jit_vertex_header_ptr_type = LLVMPointerType(variant->jit_vertex_header_type, 0);
                                          variant->jit_function = (lp_jit_cs_func)
            if (needs_caching) {
         }
   gallivm_free_ir(variant->gallivm);
      }
         static void
   lp_cs_ctx_set_cs_variant(struct lp_cs_context *csctx,
         {
         }
         static struct lp_compute_shader_variant *
   llvmpipe_update_cs_variant(struct llvmpipe_context *lp,
               {
      char store[LP_CS_MAX_VARIANT_KEY_SIZE];
   struct lp_compute_shader_variant_key *key =
         struct lp_compute_shader_variant *variant = NULL;
            /* Search the variants for one which matches the key */
   LIST_FOR_EACH_ENTRY(li, &shader->variants.list, list) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
      variant = li->base;
                  if (variant) {
      /* Move this variant to the head of the list to implement LRU
   * deletion of shader's when we have too many.
   */
   list_move_to(&variant->list_item_global.list,
      } else {
               if (LP_DEBUG & DEBUG_CS) {
      debug_printf("%u variants,\t%u instrs,\t%u instrs/variant\n",
               lp->nr_cs_variants,
               /* First, check if we've exceeded the max number of shader variants.
   * If so, free 6.25% of them (the least recently used ones).
   */
   unsigned variants_to_cull = lp->nr_cs_variants >= LP_MAX_SHADER_VARIANTS
            if (variants_to_cull ||
      lp->nr_cs_instrs >= LP_MAX_SHADER_INSTRUCTIONS) {
   if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("Evicting CS: %u cs variants,\t%u total variants,"
               "\t%u instrs,\t%u instrs/variant\n",
               /*
   * We need to re-check lp->nr_cs_variants because an arbitrarily large
   * number of shader variants (potentially all of them) could be
   * pending for destruction on flush.
   */
   for (unsigned i = 0;
      i < variants_to_cull ||
         struct lp_cs_variant_list_item *item;
   if (list_is_empty(&lp->cs_variants_list.list)) {
         }
   item = list_last_entry(&lp->cs_variants_list.list,
         assert(item);
   assert(item->base);
                  /*
   * Generate the new variant.
   */
   int64_t t0, t1, dt;
   t0 = os_time_get();
   variant = generate_variant(lp, shader, sh_type, key);
   t1 = os_time_get();
   dt = t1 - t0;
   LP_COUNT_ADD(llvm_compile_time, dt);
            /* Put the new variant into the list */
   if (variant) {
      list_add(&variant->list_item_local.list, &shader->variants.list);
   list_add(&variant->list_item_global.list, &lp->cs_variants_list.list);
   lp->nr_cs_variants++;
   lp->nr_cs_instrs += variant->nr_instrs;
         }
      }
      static void
   llvmpipe_update_cs(struct llvmpipe_context *lp)
   {
      struct lp_compute_shader_variant *variant;
   variant = llvmpipe_update_cs_variant(lp, PIPE_SHADER_COMPUTE, lp->cs);
   /* Bind this variant */
      }
         /**
   * Called during state validation when LP_CSNEW_SAMPLER_VIEW is set.
   */
   static void
   lp_csctx_set_sampler_views(struct lp_cs_context *csctx,
               {
                                 for (unsigned i = 0; i < max_tex_num; i++) {
               /* We are going to overwrite/unref the current texture further below. If
   * set, make sure to unmap its resource to avoid leaking previous
   * mapping.  */
   if (csctx->cs.current_tex[i])
            if (view) {
      struct pipe_resource *res = view->texture;
                  /* We're referencing the texture's internal data, so save a
   * reference to it.
                     } else {
            }
      }
         /**
   * Called during state validation when LP_NEW_SAMPLER is set.
   */
   static void
   lp_csctx_set_sampler_state(struct lp_cs_context *csctx,
               {
                        for (unsigned i = 0; i < PIPE_MAX_SAMPLERS; i++) {
               if (sampler) {
                     jit_sam->min_lod = sampler->min_lod;
   jit_sam->max_lod = sampler->max_lod;
   jit_sam->lod_bias = sampler->lod_bias;
   jit_sam->max_aniso = sampler->max_anisotropy;
            }
         static void
   lp_csctx_set_cs_constants(struct lp_cs_context *csctx,
               {
                                 for (i = 0; i < num; ++i) {
         }
   for (; i < ARRAY_SIZE(csctx->constants); i++) {
            }
         static void
   lp_csctx_set_cs_ssbos(struct lp_cs_context *csctx,
               {
      int i;
                     for (i = 0; i < num; ++i) {
         }
   for (; i < ARRAY_SIZE(csctx->ssbos); i++) {
            }
         static void
   lp_csctx_set_cs_images(struct lp_cs_context *csctx,
               {
                                 for (i = 0; i < num; ++i) {
      struct pipe_image_view *image = &images[i];
            struct pipe_resource *res = image->resource;
   struct llvmpipe_resource *lp_res = llvmpipe_resource(res);
            jit_image = &csctx->cs.current.jit_resources.images[i];
   if (!lp_res)
               }
   for (; i < ARRAY_SIZE(csctx->images); i++) {
            }
         static void
   update_csctx_consts(struct llvmpipe_context *llvmpipe,
         {
      for (int i = 0; i < ARRAY_SIZE(csctx->constants); ++i) {
      lp_jit_buffer_from_pipe_const(&csctx->cs.current.jit_resources.constants[i],
         }
         static void
   update_csctx_ssbo(struct llvmpipe_context *llvmpipe,
         {
      for (int i = 0; i < ARRAY_SIZE(csctx->ssbos); ++i) {
      struct pipe_resource *buffer = csctx->ssbos[i].current.buffer;
            /* resource buffer */
   if (buffer)
         if (current_data) {
               csctx->cs.current.jit_resources.ssbos[i].u = (const uint32_t *)current_data;
      } else {
      csctx->cs.current.jit_resources.ssbos[i].u = NULL;
            }
         static void
   llvmpipe_cs_update_derived(struct llvmpipe_context *llvmpipe, const void *input)
   {
      if (llvmpipe->cs_dirty & LP_CSNEW_CONSTANTS) {
      lp_csctx_set_cs_constants(llvmpipe->csctx,
                           if (llvmpipe->cs_dirty & LP_CSNEW_SSBOS) {
      lp_csctx_set_cs_ssbos(llvmpipe->csctx,
                           if (llvmpipe->cs_dirty & LP_CSNEW_SAMPLER_VIEW)
      lp_csctx_set_sampler_views(llvmpipe->csctx,
               if (llvmpipe->cs_dirty & LP_CSNEW_SAMPLER)
      lp_csctx_set_sampler_state(llvmpipe->csctx,
               if (llvmpipe->cs_dirty & LP_CSNEW_IMAGES)
      lp_csctx_set_cs_images(llvmpipe->csctx,
               struct lp_cs_context *csctx = llvmpipe->csctx;
   csctx->cs.current.jit_resources.aniso_filter_table = lp_build_sample_aniso_filter_table();
   if (input) {
      csctx->input = input;
               if (llvmpipe->cs_dirty & (LP_CSNEW_CS |
                                    }
         static void
   cs_exec_fn(void *init_data, int iter_idx, struct lp_cs_local_mem *lmem)
   {
      struct lp_cs_job_info *job_info = init_data;
                     if (lmem->local_size < job_info->req_local_mem) {
      lmem->local_mem_ptr = REALLOC(lmem->local_mem_ptr, lmem->local_size,
            }
   if (job_info->zero_initialize_shared_memory)
                                    if (job_info->use_iters) {
      grid_z = iter_idx / (job_info->iter_size[0] * job_info->iter_size[1]);
   grid_y = (iter_idx - (grid_z * (job_info->iter_size[0] * job_info->iter_size[1]))) / job_info->iter_size[0];
      } else {
      grid_z = iter_idx / (job_info->grid_size[0] * job_info->grid_size[1]);
   grid_y = (iter_idx - (grid_z * (job_info->grid_size[0] * job_info->grid_size[1]))) / job_info->grid_size[0];
               grid_z += job_info->grid_base[2];
   grid_y += job_info->grid_base[1];
   grid_x += job_info->grid_base[0];
            void *io_ptr = NULL;
   if (job_info->io) {
      size_t io_offset = job_info->io_stride * iter_idx;
      }
   if (thread_data.payload) {
      size_t payload_offset = job_info->payload_stride * iter_idx;
      }
   variant->jit_function(&job_info->current->jit_context,
                        &job_info->current->jit_resources,
   job_info->block_size[0], job_info->block_size[1], job_info->block_size[2],
   grid_x, grid_y, grid_z,
   }
         static void
   fill_grid_size(struct pipe_context *pipe,
                     {
      struct pipe_transfer *transfer;
   uint32_t *params;
   if (!info->indirect) {
      grid_size[0] = info->grid[0];
   grid_size[1] = info->grid[1];
   grid_size[2] = info->grid[2];
      }
   params = pipe_buffer_map_range(pipe, info->indirect,
                              if (!transfer)
            grid_size[0] = params[0];
   grid_size[1] = params[1];
   grid_size[2] = params[2];
      }
         static void
   llvmpipe_launch_grid(struct pipe_context *pipe,
         {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
            if (!llvmpipe_check_render_cond(llvmpipe))
                                       job_info.grid_base[0] = info->grid_base[0];
   job_info.grid_base[1] = info->grid_base[1];
   job_info.grid_base[2] = info->grid_base[2];
   job_info.block_size[0] = info->block[0];
   job_info.block_size[1] = info->block[1];
   job_info.block_size[2] = info->block[2];
   job_info.work_dim = info->work_dim;
   job_info.req_local_mem = llvmpipe->cs->req_local_mem + info->variable_shared_mem;
   job_info.zero_initialize_shared_memory = llvmpipe->cs->zero_initialize_shared_memory;
            int num_tasks = job_info.grid_size[2] * job_info.grid_size[1] * job_info.grid_size[0];
   if (num_tasks) {
      struct lp_cs_tpool_task *task;
   mtx_lock(&screen->cs_mutex);
   task = lp_cs_tpool_queue_task(screen->cs_tpool, cs_exec_fn, &job_info, num_tasks);
               }
   if (!llvmpipe->queries_disabled)
      }
         static void
   llvmpipe_set_compute_resources(struct pipe_context *pipe,
               {
   }
         static void
   llvmpipe_set_global_binding(struct pipe_context *pipe,
                     {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
            if (first + count > cs->max_global_buffers) {
      unsigned old_max = cs->max_global_buffers;
   cs->max_global_buffers = first + count;
   cs->global_buffers = realloc(cs->global_buffers,
         if (!cs->global_buffers) {
                              if (!resources) {
      for (unsigned i = 0; i < count; i++)
                     for (unsigned i = 0; i < count; i++) {
      uintptr_t va;
   uint32_t offset;
   pipe_resource_reference(&cs->global_buffers[first + i], resources[i]);
   struct llvmpipe_resource *lp_res = llvmpipe_resource(resources[i]);
   offset = *handles[i];
   va = (uintptr_t)((char *)lp_res->data + offset);
         }
         void
   llvmpipe_init_compute_funcs(struct llvmpipe_context *llvmpipe)
   {
      llvmpipe->pipe.create_compute_state = llvmpipe_create_compute_state;
   llvmpipe->pipe.bind_compute_state = llvmpipe_bind_compute_state;
   llvmpipe->pipe.get_compute_state_info = llvmpipe_get_compute_state_info;
   llvmpipe->pipe.delete_compute_state = llvmpipe_delete_compute_state;
   llvmpipe->pipe.set_compute_resources = llvmpipe_set_compute_resources;
   llvmpipe->pipe.set_global_binding = llvmpipe_set_global_binding;
      }
         void
   lp_csctx_destroy(struct lp_cs_context *csctx)
   {
      unsigned i;
   for (i = 0; i < ARRAY_SIZE(csctx->cs.current_tex); i++) {
      struct pipe_resource **res_ptr = &csctx->cs.current_tex[i];
   if (*res_ptr)
            }
   for (i = 0; i < ARRAY_SIZE(csctx->constants); i++) {
         }
   for (i = 0; i < ARRAY_SIZE(csctx->ssbos); i++) {
         }
   for (i = 0; i < ARRAY_SIZE(csctx->images); i++) {
         }
      }
         struct lp_cs_context *
   lp_csctx_create(struct pipe_context *pipe)
   {
      struct lp_cs_context *csctx = CALLOC_STRUCT(lp_cs_context);
   if (!csctx)
            csctx->pipe = pipe;
      }
      void
   llvmpipe_update_task_shader(struct llvmpipe_context *lp)
   {
      if (!lp->tss)
         struct lp_compute_shader_variant *variant = llvmpipe_update_cs_variant(lp, PIPE_SHADER_TASK, lp->tss);
      }
      static void *
   llvmpipe_create_ts_state(struct pipe_context *pipe,
         {
      struct lp_compute_shader *shader = CALLOC_STRUCT(lp_compute_shader);
   if (!shader)
                     shader->no = task_no++;
            shader->base.ir.nir = templ->ir.nir;
   shader->req_local_mem += ((struct nir_shader *)shader->base.ir.nir)->info.shared_size;
            struct nir_shader *nir = shader->base.ir.nir;
   int nr_samplers = BITSET_LAST_BIT(nir->info.samplers_used);
   int nr_sampler_views = BITSET_LAST_BIT(nir->info.textures_used);
   int nr_images = BITSET_LAST_BIT(nir->info.images_used);
   shader->variant_key_size = lp_cs_variant_key_size(MAX2(nr_samplers, nr_sampler_views), nr_images);
      }
         static void
   llvmpipe_bind_ts_state(struct pipe_context *pipe, void *_task)
   {
               if (llvmpipe->tss == _task)
            llvmpipe->tss = (struct lp_compute_shader *)_task;
      }
      static void
   llvmpipe_delete_ts_state(struct pipe_context *pipe, void *_task)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_compute_shader *shader = _task;
                     /* Delete all the variants */
   LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
         }
   ralloc_free(shader->base.ir.nir);
      }
      void
   llvmpipe_init_task_funcs(struct llvmpipe_context *llvmpipe)
   {
      llvmpipe->pipe.create_ts_state = llvmpipe_create_ts_state;
   llvmpipe->pipe.bind_ts_state   = llvmpipe_bind_ts_state;
      }
      void
   llvmpipe_update_mesh_shader(struct llvmpipe_context *lp)
   {
      if (!lp->mhs)
         struct lp_compute_shader_variant *variant = llvmpipe_update_cs_variant(lp, PIPE_SHADER_MESH, lp->mhs);
      }
      static void *
   llvmpipe_create_ms_state(struct pipe_context *pipe,
         {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_compute_shader *shader = CALLOC_STRUCT(lp_compute_shader);
   if (!shader)
                     shader->no = mesh_no++;
            shader->base.ir.nir = templ->ir.nir;
   shader->req_local_mem += ((struct nir_shader *)shader->base.ir.nir)->info.shared_size;
            shader->draw_mesh_data = draw_create_mesh_shader(llvmpipe->draw, templ);
   if (shader->draw_mesh_data == NULL) {
      FREE(shader);
   llvmpipe_register_shader(pipe, templ, true);
               struct nir_shader *nir = shader->base.ir.nir;
   int nr_samplers = BITSET_LAST_BIT(nir->info.samplers_used);
   int nr_sampler_views = BITSET_LAST_BIT(nir->info.textures_used);
   int nr_images = BITSET_LAST_BIT(nir->info.images_used);
   shader->variant_key_size = lp_cs_variant_key_size(MAX2(nr_samplers, nr_sampler_views), nr_images);
      }
         static void
   llvmpipe_bind_ms_state(struct pipe_context *pipe, void *_mesh)
   {
               if (llvmpipe->mhs == _mesh)
                     draw_bind_mesh_shader(llvmpipe->draw, _mesh ? llvmpipe->mhs->draw_mesh_data : NULL);
      }
         static void
   llvmpipe_delete_ms_state(struct pipe_context *pipe, void *_mesh)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_compute_shader *shader = _mesh;
                     /* Delete all the variants */
   LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
                  draw_delete_mesh_shader(llvmpipe->draw, shader->draw_mesh_data);
               }
      static void
   lp_mesh_call_draw(struct llvmpipe_context *lp,
                     enum mesa_prim prim,
   int prim_out_idx,
   int cull_prim_idx,
   int task_idx,
   {
      unsigned prim_len = u_vertices_per_prim(prim);
   uint32_t *ptr = (uint32_t *)((char *)vbuf + task_out_size * task_idx);
   uint32_t vertex_count = ptr[1];
            if (!vertex_count || !prim_count)
            struct draw_vertex_info vinfo;
   vinfo.verts = (struct vertex_header *)ptr;
   vinfo.vertex_size = vsize / 8;
   vinfo.stride = vsize;
            unsigned elts_size = prim_len * prim_count;
   unsigned short *elts = calloc(sizeof(uint16_t), elts_size);
   uint32_t *prim_lengths = calloc(prim_count, sizeof(uint32_t));
   int elts_idx = 0;
   char *prim_ptr = (char *)ptr + prim_offset;
   for (unsigned p = 0; p < prim_count; p++) {
      uint32_t *prim_idxs = (uint32_t *)(prim_ptr + p * psize + prim_out_idx * 4 * sizeof(float));
   for (unsigned elt = 0; elt < prim_len; elt++){
         }
               struct draw_prim_info prim_info = { 0 };
   prim_info.prim = prim;
   prim_info.linear = false;
   prim_info.elts = elts;
   prim_info.count = prim_count;
   prim_info.primitive_count = prim_count;
            struct draw_vertex_info vert_out = { 0 };
   struct draw_prim_info prim_out = { 0 };
   draw_mesh_prim_run(lp->draw,
                     per_prim_count,
   prim_ptr,
   cull_prim_idx,
   &prim_info,
   free(elts);
            draw_collect_primitives_generated(lp->draw,
                        free(vert_out.verts);
      }
      static void
   llvmpipe_draw_mesh_tasks(struct pipe_context *pipe,
         {
      struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
            if (!llvmpipe_check_render_cond(lp))
            memset(&job_info, 0, sizeof(job_info));
   if (lp->dirty)
            unsigned draw_count = info->draw_count;
   if (info->indirect && info->indirect_draw_count) {
      struct pipe_transfer *dc_transfer;
   uint32_t *dc_param = pipe_buffer_map_range(pipe,
                     if (!dc_transfer) {
      debug_printf("%s: failed to map indirect draw count buffer\n", __func__);
      }
   if (dc_param[0] < draw_count)
                     struct nir_shader *mhs_shader = lp->mhs->base.ir.nir;
   int prim_out_idx = -1;
   int first_per_prim_idx = -1;
   int cull_prim_idx = -1;
   nir_foreach_shader_out_variable(var, mhs_shader) {
      if (var->data.per_primitive) {
      first_per_prim_idx = var->data.driver_location;
         }
   nir_foreach_shader_out_variable(var, mhs_shader) {
      if (var->data.location == VARYING_SLOT_PRIMITIVE_INDICES) {
      prim_out_idx = var->data.driver_location;
         }
   nir_foreach_shader_out_variable(var, mhs_shader) {
      if (var->data.location == VARYING_SLOT_CULL_PRIMITIVE) {
      cull_prim_idx = var->data.driver_location - first_per_prim_idx;
         }
   int per_prim_count = util_bitcount64(mhs_shader->info.per_primitive_outputs);
   int out_count = util_bitcount64(mhs_shader->info.outputs_written);
   int per_vert_count = out_count - per_prim_count;
   int vsize = (sizeof(struct vertex_header) + per_vert_count * 4 * sizeof(float)) * 8;
   int psize = (per_prim_count * 4 * sizeof(float)) * 8;
   size_t prim_offset = vsize * (mhs_shader->info.mesh.max_vertices_out + 8);
            for (unsigned dr = 0; dr < draw_count; dr++) {
               job_info.grid_base[0] = info->grid_base[0];
   job_info.grid_base[1] = info->grid_base[1];
   job_info.grid_base[2] = info->grid_base[2];
   job_info.block_size[0] = info->block[0];
   job_info.block_size[1] = info->block[1];
            void *payload = NULL;
   size_t payload_stride = 0;
   int num_tasks = job_info.grid_size[2] * job_info.grid_size[1] * job_info.grid_size[0];
   int num_mesh_invocs = 1;
   if (lp->tss) {
                              job_info.use_iters = false;
   job_info.payload = payload;
   job_info.payload_stride = payload_stride;
   job_info.work_dim = info->work_dim;
   job_info.draw_id = dr;
                  if (num_tasks) {
      struct lp_cs_tpool_task *task;
   mtx_lock(&screen->cs_mutex);
                     }
   if (!lp->queries_disabled)
                     for (unsigned i = 0; i < num_mesh_invocs; i++) {
      if (payload) {
      void *this_payload = (char *)payload + (payload_stride * i);
   uint32_t *payload_grid = (uint32_t *)this_payload;
   assert(lp->tss);
   job_info.grid_size[0] = payload_grid[0];
   job_info.grid_size[1] = payload_grid[1];
   job_info.grid_size[2] = payload_grid[2];
   job_info.payload = this_payload;
   job_info.block_size[0] = mhs_shader->info.workgroup_size[0];
   job_info.block_size[1] = mhs_shader->info.workgroup_size[1];
               job_info.req_local_mem = lp->mhs->req_local_mem + info->variable_shared_mem;
   job_info.current = &lp->mesh_ctx->cs.current;
   job_info.payload_stride = 0;
                  uint32_t job_strides[3] = { job_info.grid_size[0], job_info.grid_size[1], job_info.grid_size[2] };
   uint32_t total_grid[3] = { job_info.grid_size[0], job_info.grid_size[1], job_info.grid_size[2] };
   const unsigned int max_tasks = 4096;
   /* limit how large memory allocation can get for vbuf */
   for (unsigned g = 0; g < 3; g++) {
      if (job_strides[g] > max_tasks) {
                     for (unsigned grid_z = 0; grid_z < total_grid[2]; grid_z += job_strides[2]) {
      int this_z = MIN2(total_grid[2] - grid_z, max_tasks);
   job_info.grid_base[2] = grid_z;
   for (unsigned grid_y = 0; grid_y < total_grid[1]; grid_y += job_strides[1]) {
      int this_y = MIN2(total_grid[1] - grid_y, max_tasks);
   job_info.grid_base[1] = grid_y;
   for (unsigned grid_x = 0; grid_x < total_grid[0]; grid_x += job_strides[0]) {
                           job_info.iter_size[0] = this_x;
                                             job_info.io = vbuf;
   if (num_tasks) {
                                                         for (unsigned t = 0; t < num_tasks; t++)
      lp_mesh_call_draw(lp,
                                    }
      }
      }
      void
   llvmpipe_init_mesh_funcs(struct llvmpipe_context *llvmpipe)
   {
      llvmpipe->pipe.create_ms_state = llvmpipe_create_ms_state;
   llvmpipe->pipe.bind_ms_state   = llvmpipe_bind_ms_state;
               }
      void
   llvmpipe_task_update_derived(struct llvmpipe_context *llvmpipe)
   {
      if (llvmpipe->dirty & LP_NEW_TASK_CONSTANTS) {
      lp_csctx_set_cs_constants(llvmpipe->task_ctx,
                           if (llvmpipe->dirty & LP_NEW_TASK_SSBOS) {
      lp_csctx_set_cs_ssbos(llvmpipe->task_ctx,
                           if (llvmpipe->dirty & LP_NEW_TASK_SAMPLER_VIEW)
      lp_csctx_set_sampler_views(llvmpipe->task_ctx,
               if (llvmpipe->dirty & LP_NEW_TASK_SAMPLER)
      lp_csctx_set_sampler_state(llvmpipe->task_ctx,
               if (llvmpipe->dirty & LP_NEW_TASK_IMAGES)
      lp_csctx_set_cs_images(llvmpipe->task_ctx,
               struct lp_cs_context *csctx = llvmpipe->task_ctx;
      }
      void
   llvmpipe_mesh_update_derived(struct llvmpipe_context *llvmpipe)
   {
      if (llvmpipe->dirty & LP_NEW_MESH_CONSTANTS) {
      lp_csctx_set_cs_constants(llvmpipe->mesh_ctx,
                           if (llvmpipe->dirty & LP_NEW_MESH_SSBOS) {
      lp_csctx_set_cs_ssbos(llvmpipe->mesh_ctx,
                           if (llvmpipe->dirty & LP_NEW_MESH_SAMPLER_VIEW)
      lp_csctx_set_sampler_views(llvmpipe->mesh_ctx,
               if (llvmpipe->dirty & LP_NEW_MESH_SAMPLER)
      lp_csctx_set_sampler_state(llvmpipe->mesh_ctx,
               if (llvmpipe->dirty & LP_NEW_MESH_IMAGES)
      lp_csctx_set_cs_images(llvmpipe->mesh_ctx,
               struct lp_cs_context *csctx = llvmpipe->mesh_ctx;
      }
