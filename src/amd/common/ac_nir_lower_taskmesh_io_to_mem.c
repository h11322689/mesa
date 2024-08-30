   /*
   * Copyright © 2022 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_nir.h"
   #include "nir_builder.h"
   #include "amdgfxregs.h"
   #include "util/u_math.h"
      /*
   * These NIR passes are used to lower NIR cross-stage I/O intrinsics
   * between task and mesh shader stages into the memory accesses
   * that actually happen on the HW.
   *
   */
      typedef struct {
      unsigned payload_entry_bytes;
   unsigned draw_entry_bytes;
            /* True if the lowering needs to insert shader query. */
      } lower_tsms_io_state;
      static nir_def *
   task_workgroup_index(nir_builder *b,
         {
               nir_def *x = nir_channel(b, id, 0);
   nir_def *y = nir_channel(b, id, 1);
            nir_def *grid_size = nir_load_num_workgroups(b);
   nir_def *grid_size_x = nir_channel(b, grid_size, 0);
            return nir_iadd(b, nir_imul(b, nir_imul(b, grid_size_x, grid_size_y), z),
      }
      static nir_def *
   task_ring_entry_index(nir_builder *b,
         {
      /* Task shader ring_entry shader argument:
   *
   * - It's a copy of write_ptr[31:0] from the task control buffer.
   * - The same value (which is the initial value at dispatch)
   *   seems to be copied to all workgroups in the same dispatch,
   *   therefore a workgroup index needs to be added.
   * - write_ptr must be initialized to num_entries so ring_entry needs
   *   AND with num_entries - 1 to get the correct meaning.
   *   Note that num_entries must be a power of two.
   */
   nir_def *ring_entry = nir_load_task_ring_entry_amd(b);
   nir_def *idx = nir_iadd_nuw(b, ring_entry, task_workgroup_index(b, s));
      }
      static nir_def *
   task_draw_ready_bit(nir_builder *b,
         {
      /* Value of the ready bit is 1 for odd and 0 for even passes through the draw ring.
   *
   * The ring_entry is a copy of the write_ptr. We use that to determine whether
   * the current pass through the draw ring is odd or even, so we can write the
   * correct value to the draw ready bit.
   *
   * This tells the firmware that it can now start launching mesh shader workgroups.
   * The encoding of the last dword of the draw ring entry is:
   * - bit 0: Draw ready bit.
   *          Its meaning flips on every pass through the entry.
   * - bit 1: Packet end bit.
   *          The firmware uses this to mark the entry after the last one
   *          used by the current task dispatch.
   * - bits [2:31] unused.
   *
   * Task shaders MUST write the draw ready bit to the draw ring
   * before they finish. The firmware waits for the shader to write
   * this bit before it reads the mesh dispatch size to launch the
   * mesh shader workgroups.
   *
   * If the task shader doesn't write this bit, the HW hangs.
            nir_def *ring_entry = nir_load_task_ring_entry_amd(b);
            nir_def *idx = nir_iadd_nuw(b, ring_entry, workgroup_index);
      }
      static nir_def *
   mesh_ring_entry_index(nir_builder *b,
         {
      /* Mesh shader ring_entry shader argument:
   *
   * - It's a copy of the read_ptr[31:0] from the task control buffer.
   * - All workgroups in the same task->mesh dispatch get the same value,
   *   which is fine because they need to read the same entry.
   * - read_ptr must be initialized to num_entries so ring_entry needs
   *   AND with num_entries - 1 to get the correct meaning.
   *   Note that num_entries must be a power of two.
   */
      }
      static void
   task_write_draw_ring(nir_builder *b,
                     {
      nir_def *ptr = task_ring_entry_index(b, s);
   nir_def *ring = nir_load_ring_task_draw_amd(b);
   nir_def *scalar_off = nir_imul_imm(b, ptr, s->draw_entry_bytes);
   nir_def *vector_off = nir_imm_int(b, 0);
            nir_store_buffer_amd(b, store_val, ring, vector_off, scalar_off, zero,
            }
      static bool
   filter_task_intrinsics(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   return intrin->intrinsic == nir_intrinsic_launch_mesh_workgroups ||
            }
      static void
   task_invocation_query(nir_builder *b, lower_tsms_io_state *s)
   {
      if (!s->has_query)
            const unsigned invocations = b->shader->info.workgroup_size[0] *
                  nir_if *if_pipeline_query = nir_push_if(b, nir_load_pipeline_stat_query_enabled_amd(b));
   {
         }
      }
      static nir_def *
   lower_task_launch_mesh_workgroups(nir_builder *b,
               {
      /* This intrinsic must be always in uniform control flow,
   * so we assume that all invocations are active here.
            /* Wait for all necessary stores to finish.
   * Device memory scope is necessary because we need to ensure there is
   * always a waitcnt_vscnt instruction in order to avoid a race condition
   * between payload stores and their loads after mesh shaders launch.
   */
   nir_barrier(b, .execution_scope = SCOPE_WORKGROUP,
                              /* On the first invocation, write the full draw ring entry. */
   nir_def *invocation_index = nir_load_local_invocation_index(b);
   nir_if *if_invocation_index_zero = nir_push_if(b, nir_ieq_imm(b, invocation_index, 0));
   {
      nir_def *dimensions = intrin->src[0].ssa;
   nir_def *x = nir_channel(b, dimensions, 0);
   nir_def *y = nir_channel(b, dimensions, 1);
            /* When either Y or Z are 0, also set X to 0.
   * Not necessary, but speeds up the job of the CP.
   */
            /* Dispatch dimensions of mesh shader workgroups. */
   task_write_draw_ring(b, nir_vec3(b, x, y, z), 0, s);
   /* Prevent the two stores from being reordered. */
   nir_scoped_memory_barrier(b, SCOPE_INVOCATION, NIR_MEMORY_RELEASE, nir_var_shader_out);
   /* Ready bit, only write the low 8 bits. */
               }
               }
      static nir_def *
   lower_task_payload_store(nir_builder *b,
               {
      unsigned write_mask = nir_intrinsic_write_mask(intrin);
            nir_def *store_val = intrin->src[0].ssa;
   nir_def *addr = intrin->src[1].ssa;
   nir_def *ring = nir_load_ring_task_payload_amd(b);
   nir_def *ptr = task_ring_entry_index(b, s);
   nir_def *ring_off = nir_imul_imm(b, ptr, s->payload_entry_bytes);
            nir_store_buffer_amd(b, store_val, ring, addr, ring_off, zero, .base = base,
                           }
      static nir_def *
   lower_taskmesh_payload_load(nir_builder *b,
               {
      unsigned base = nir_intrinsic_base(intrin);
   unsigned num_components = intrin->def.num_components;
            nir_def *ptr =
      b->shader->info.stage == MESA_SHADER_TASK ?
   task_ring_entry_index(b, s) :
         nir_def *addr = intrin->src[0].ssa;
   nir_def *ring = nir_load_ring_task_payload_amd(b);
   nir_def *ring_off = nir_imul_imm(b, ptr, s->payload_entry_bytes);
            return nir_load_buffer_amd(b, num_components, bit_size, ring, addr, ring_off, zero, .base = base,
            }
      static nir_def *
   lower_task_intrinsics(nir_builder *b,
               {
      assert(instr->type == nir_instr_type_intrinsic);
   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
            switch (intrin->intrinsic) {
      case nir_intrinsic_store_task_payload:
         case nir_intrinsic_load_task_payload:
         case nir_intrinsic_launch_mesh_workgroups:
         default:
         }
      void
   ac_nir_lower_task_outputs_to_mem(nir_shader *shader,
                     {
               nir_lower_task_shader_options lower_ts_opt = {
         };
            lower_tsms_io_state state = {
      .draw_entry_bytes = 16,
   .payload_entry_bytes = task_payload_entry_bytes,
   .num_entries = task_num_entries,
                        nir_shader_lower_instructions(shader,
                        nir_metadata_preserve(impl, nir_metadata_none);
      }
      static bool
   filter_mesh_input_load(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      }
      static nir_def *
   lower_mesh_intrinsics(nir_builder *b,
               {
      assert(instr->type == nir_instr_type_intrinsic);
   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
            if (intrin->intrinsic == nir_intrinsic_load_task_payload)
         else
      }
      void
   ac_nir_lower_mesh_inputs_to_mem(nir_shader *shader,
               {
               lower_tsms_io_state state = {
      .draw_entry_bytes = 16,
   .payload_entry_bytes = task_payload_entry_bytes,
               nir_shader_lower_instructions(shader,
                  }
