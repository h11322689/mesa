   /*
   * Copyright © 2010 Intel Corporation
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
   * Authors:
   *    Eric Anholt <eric@anholt.net>
   *
   */
      #include "brw_eu.h"
   #include "brw_fs.h"
   #include "brw_cfg.h"
   #include "util/set.h"
   #include "util/register_allocate.h"
      using namespace brw;
      #define REG_CLASS_COUNT 20
      static void
   assign_reg(const struct intel_device_info *devinfo,
         {
      if (reg->file == VGRF) {
      reg->nr = reg_unit(devinfo) * reg_hw_locations[reg->nr] + reg->offset / REG_SIZE;
         }
      void
   fs_visitor::assign_regs_trivial()
   {
      unsigned hw_reg_mapping[this->alloc.count + 1];
   unsigned i;
            /* Note that compressed instructions require alignment to 2 registers. */
   hw_reg_mapping[0] = ALIGN(this->first_non_payload_grf, reg_width);
   for (i = 1; i <= this->alloc.count; i++) {
      hw_reg_mapping[i] = (hw_reg_mapping[i - 1] +
            }
            foreach_block_and_inst(block, fs_inst, inst, cfg) {
      assign_reg(devinfo, hw_reg_mapping, &inst->dst);
   for (i = 0; i < inst->sources; i++) {
                     if (this->grf_used >= max_grf) {
         this->grf_used, max_grf);
   } else {
               }
      /**
   * Size of a register from the aligned_bary_class register class.
   */
   static unsigned
   aligned_bary_size(unsigned dispatch_width)
   {
         }
      static void
   brw_alloc_reg_set(struct brw_compiler *compiler, int dispatch_width)
   {
      const struct intel_device_info *devinfo = compiler->devinfo;
   int base_reg_count = BRW_MAX_GRF;
            if (dispatch_width > 8 && devinfo->ver >= 7) {
      /* For IVB+, we don't need the PLN hacks or the even-reg alignment in
   * SIMD16.  Therefore, we can use the exact same register sets for
   * SIMD16 as we do for SIMD8 and we don't need to recalculate them.
   */
   compiler->fs_reg_sets[index] = compiler->fs_reg_sets[0];
               /* The registers used to make up almost all values handled in the compiler
   * are a scalar value occupying a single register (or 2 registers in the
   * case of SIMD16, which is handled by dividing base_reg_count by 2 and
   * multiplying allocated register numbers by 2).  Things that were
   * aggregates of scalar values at the GLSL level were split to scalar
   * values by split_virtual_grfs().
   *
   * However, texture SEND messages return a series of contiguous registers
   * to write into.  We currently always ask for 4 registers, but we may
   * convert that to use less some day.
   *
   * Additionally, on gfx5 we need aligned pairs of registers for the PLN
   * instruction, and on gfx4 we need 8 contiguous regs for workaround simd16
   * texturing.
   */
   assert(REG_CLASS_COUNT == MAX_VGRF_SIZE(devinfo) / reg_unit(devinfo));
   int class_sizes[REG_CLASS_COUNT];
   for (unsigned i = 0; i < REG_CLASS_COUNT; i++)
            struct ra_regs *regs = ra_alloc_reg_set(compiler, BRW_MAX_GRF, false);
   if (devinfo->ver >= 6)
         struct ra_class **classes = ralloc_array(compiler, struct ra_class *,
                  /* Now, make the register classes for each size of contiguous register
   * allocation we might need to make.
   */
   for (int i = 0; i < REG_CLASS_COUNT; i++) {
               if (devinfo->ver <= 5 && dispatch_width >= 16) {
      /* From the G45 PRM:
   *
   * In order to reduce the hardware complexity, the following
   * rules and restrictions apply to the compressed instruction:
   * ...
   * * Operand Alignment Rule: With the exceptions listed below, a
   *   source/destination operand in general should be aligned to
   *   even 256-bit physical register with a region size equal to
   *   two 256-bit physical register
   */
   for (int reg = 0; reg <= base_reg_count - class_sizes[i]; reg += 2)
      } else {
      for (int reg = 0; reg <= base_reg_count - class_sizes[i]; reg++)
                  /* Add a special class for aligned barycentrics, which we'll put the
   * first source of LINTERP on so that we can do PLN on Gen <= 6.
   */
   if (devinfo->has_pln && (devinfo->ver == 6 ||
            int contig_len = aligned_bary_size(dispatch_width);
            for (int i = 0; i <= base_reg_count - contig_len; i += 2)
                        compiler->fs_reg_sets[index].regs = regs;
   for (unsigned i = 0; i < ARRAY_SIZE(compiler->fs_reg_sets[index].classes); i++)
         for (int i = 0; i < REG_CLASS_COUNT; i++)
            }
      void
   brw_fs_alloc_reg_sets(struct brw_compiler *compiler)
   {
      brw_alloc_reg_set(compiler, 8);
   brw_alloc_reg_set(compiler, 16);
      }
      static int
   count_to_loop_end(const bblock_t *block)
   {
      if (block->end()->opcode == BRW_OPCODE_WHILE)
            int depth = 1;
   /* Skip the first block, since we don't want to count the do the calling
   * function found.
   */
   for (block = block->next();
      depth > 0;
   block = block->next()) {
   if (block->start()->opcode == BRW_OPCODE_DO)
         if (block->end()->opcode == BRW_OPCODE_WHILE) {
      depth--;
   if (depth == 0)
         }
      }
      void fs_visitor::calculate_payload_ranges(unsigned payload_node_count,
         {
      int loop_depth = 0;
            for (unsigned i = 0; i < payload_node_count; i++)
            int ip = 0;
   foreach_block_and_inst(block, fs_inst, inst, cfg) {
      switch (inst->opcode) {
   case BRW_OPCODE_DO:
               /* Since payload regs are deffed only at the start of the shader
   * execution, any uses of the payload within a loop mean the live
   * interval extends to the end of the outermost loop.  Find the ip of
   * the end now.
   */
   if (loop_depth == 1)
            case BRW_OPCODE_WHILE:
      loop_depth--;
      default:
                  int use_ip;
   if (loop_depth > 0)
         else
            /* Note that UNIFORM args have been turned into FIXED_GRF by
   * assign_curbe_setup(), and interpolation uses fixed hardware regs from
   * the start (see interp_reg()).
   */
   for (int i = 0; i < inst->sources; i++) {
      if (inst->src[i].file == FIXED_GRF) {
      unsigned reg_nr = inst->src[i].nr;
                  for (unsigned j = reg_nr / reg_unit(devinfo);
      j < DIV_ROUND_UP(reg_nr + regs_read(inst, i),
         j++) {
   payload_last_use_ip[j] = use_ip;
                     if (inst->dst.file == FIXED_GRF) {
      unsigned reg_nr = inst->dst.nr;
   if (reg_nr / reg_unit(devinfo) < payload_node_count) {
      for (unsigned j = reg_nr / reg_unit(devinfo);
      j < DIV_ROUND_UP(reg_nr + regs_written(inst),
         j++) {
   payload_last_use_ip[j] = use_ip;
                     /* Special case instructions which have extra implied registers used. */
   switch (inst->opcode) {
   case CS_OPCODE_CS_TERMINATE:
                  default:
      if (inst->eot) {
      /* We could omit this for the !inst->header_present case, except
   * that the simulator apparently incorrectly reads from g0/g1
   * instead of sideband.  It also really freaks out driver
   * developers to see g0 used in unusual places, so just always
   * reserve it.
   */
   payload_last_use_ip[0] = use_ip;
      }
                     }
      class fs_reg_alloc {
   public:
      fs_reg_alloc(fs_visitor *fs):
      fs(fs), devinfo(fs->devinfo), compiler(fs->compiler),
   live(fs->live_analysis.require()), g(NULL),
      {
               /* Stash the number of instructions so we can sanity check that our
   * counts still match liveness.
   */
                     /* Most of this allocation was written for a reg_width of 1
   * (dispatch_width == 8).  In extending to SIMD16, the code was
   * left in place and it was converted to have the hardware
   * registers it's allocating be contiguous physical pairs of regs
   * for reg_width == 2.
   */
   int reg_width = fs->dispatch_width / 8;
   rsi = util_logbase2(reg_width);
            /* Get payload IP information */
            node_count = 0;
   first_payload_node = 0;
   first_mrf_hack_node = 0;
   scratch_header_node = 0;
   grf127_send_hack_node = 0;
   first_vgrf_node = 0;
   last_vgrf_node = 0;
            spill_vgrf_ip = NULL;
   spill_vgrf_ip_alloc = 0;
               ~fs_reg_alloc()
   {
                        private:
      void setup_live_interference(unsigned node,
                  void build_interference_graph(bool allow_spilling);
            fs_reg build_lane_offsets(const fs_builder &bld,
         fs_reg build_single_offset(const fs_builder &bld,
            void emit_unspill(const fs_builder &bld, struct shader_stats *stats,
         void emit_spill(const fs_builder &bld, struct shader_stats *stats,
            void set_spill_costs();
   int choose_spill_reg();
   fs_reg alloc_scratch_header();
   fs_reg alloc_spill_reg(unsigned size, int ip);
            void *mem_ctx;
   fs_visitor *fs;
   const intel_device_info *devinfo;
   const brw_compiler *compiler;
   const fs_live_variables &live;
                     /* Which compiler->fs_reg_sets[] to use */
            ra_graph *g;
            int payload_node_count;
            int node_count;
   int first_payload_node;
   int first_mrf_hack_node;
   int scratch_header_node;
   int grf127_send_hack_node;
   int first_vgrf_node;
   int last_vgrf_node;
            int *spill_vgrf_ip;
   int spill_vgrf_ip_alloc;
               };
      /**
   * Sets the mrf_used array to indicate which MRFs are used by the shader IR
   *
   * This is used in assign_regs() to decide which of the GRFs that we use as
   * MRFs on gfx7 get normally register allocated, and in register spilling to
   * see if we can actually use MRFs to do spills without overwriting normal MRF
   * contents.
   */
   static void
   get_used_mrfs(const fs_visitor *v, bool *mrf_used)
   {
                        foreach_block_and_inst(block, fs_inst, inst, v->cfg) {
      if (inst->dst.file == MRF) {
      int reg = inst->dst.nr & ~BRW_MRF_COMPR4;
   mrf_used[reg] = true;
   if (reg_width == 2) {
      if (inst->dst.nr & BRW_MRF_COMPR4) {
         } else {
                        for (unsigned i = 0; i < inst->implied_mrf_writes(); i++) {
                           }
      namespace {
      /**
   * Maximum spill block size we expect to encounter in 32B units.
   *
   * This is somewhat arbitrary and doesn't necessarily limit the maximum
   * variable size that can be spilled -- A higher value will allow a
   * variable of a given size to be spilled more efficiently with a smaller
   * number of scratch messages, but will increase the likelihood of a
   * collision between the MRFs reserved for spilling and other MRFs used by
   * the program (and possibly increase GRF register pressure on platforms
   * without hardware MRFs), what could cause register allocation to fail.
   *
   * For the moment reserve just enough space so a register of 32 bit
   * component type and natural region width can be spilled without splitting
   * into multiple (force_writemask_all) scratch messages.
   */
   unsigned
   spill_max_size(const backend_shader *s)
   {
      /* LSC is limited to SIMD16 sends */
   if (s->devinfo->has_lsc)
            /* FINISHME - On Gfx7+ it should be possible to avoid this limit
   *            altogether by spilling directly from the temporary GRF
   *            allocated to hold the result of the instruction (and the
   *            scratch write header).
   */
   /* FINISHME - The shader's dispatch width probably belongs in
   *            backend_shader (or some nonexistent fs_shader class?)
   *            rather than in the visitor class.
   */
               /**
   * First MRF register available for spilling.
   */
   unsigned
   spill_base_mrf(const backend_shader *s)
   {
      /* We don't use the MRF hack on Gfx9+ */
   assert(s->devinfo->ver < 9);
         }
      void
   fs_reg_alloc::setup_live_interference(unsigned node,
         {
      /* Mark any virtual grf that is live between the start of the program and
   * the last use of a payload node interfering with that payload node.
   */
   for (int i = 0; i < payload_node_count; i++) {
      if (payload_last_use_ip[i] == -1)
            /* Note that we use a <= comparison, unlike vgrfs_interfere(),
   * in order to not have to worry about the uniform issue described in
   * calculate_live_intervals().
   */
   if (node_start_ip <= payload_last_use_ip[i])
               /* If we have the MRF hack enabled, mark this node as interfering with all
   * MRF registers.
   */
   if (first_mrf_hack_node >= 0) {
      for (int i = spill_base_mrf(fs); i < BRW_MAX_MRF(devinfo->ver); i++)
               /* Everything interferes with the scratch header */
   if (scratch_header_node >= 0)
            /* Add interference with every vgrf whose live range intersects this
   * node's.  We only need to look at nodes below this one as the reflexivity
   * of interference will take care of the rest.
   */
   for (unsigned n2 = first_vgrf_node;
      n2 <= (unsigned)last_vgrf_node && n2 < node; n2++) {
   unsigned vgrf = n2 - first_vgrf_node;
   if (!(node_end_ip <= live.vgrf_start[vgrf] ||
               }
      void
   fs_reg_alloc::setup_inst_interference(const fs_inst *inst)
   {
      /* Certain instructions can't safely use the same register for their
   * sources and destination.  Add interference.
   */
   if (inst->dst.file == VGRF && inst->has_source_and_destination_hazard()) {
      for (unsigned i = 0; i < inst->sources; i++) {
      if (inst->src[i].file == VGRF) {
      ra_add_node_interference(g, first_vgrf_node + inst->dst.nr,
                     /* A compressed instruction is actually two instructions executed
   * simultaneously.  On most platforms, it ok to have the source and
   * destination registers be the same.  In this case, each instruction
   * over-writes its own source and there's no problem.  The real problem
   * here is if the source and destination registers are off by one.  Then
   * you can end up in a scenario where the first instruction over-writes the
   * source of the second instruction.  Since the compiler doesn't know about
   * this level of granularity, we simply make the source and destination
   * interfere.
   */
   if (inst->dst.component_size(inst->exec_size) > REG_SIZE &&
      inst->dst.file == VGRF) {
   for (int i = 0; i < inst->sources; ++i) {
      if (inst->src[i].file == VGRF) {
      ra_add_node_interference(g, first_vgrf_node + inst->dst.nr,
                     if (grf127_send_hack_node >= 0) {
      /* At Intel Broadwell PRM, vol 07, section "Instruction Set Reference",
   * subsection "EUISA Instructions", Send Message (page 990):
   *
   * "r127 must not be used for return address when there is a src and
   * dest overlap in send instruction."
   *
   * We are avoiding using grf127 as part of the destination of send
   * messages adding a node interference to the grf127_send_hack_node.
   * This node has a fixed assignment to grf127.
   *
   * We don't apply it to SIMD16 instructions because previous code avoids
   * any register overlap between sources and destination.
   */
   if (inst->exec_size < 16 && inst->is_send_from_grf() &&
      inst->dst.file == VGRF)
               /* Spilling instruction are generated as SEND messages from MRF but as
   * Gfx7+ supports sending from GRF the driver will maps assingn these
   * MRF registers to a GRF. Implementations reuses the dest of the send
   * message as source. So as we will have an overlap for sure, we create
   * an interference between destination and grf127.
   */
   if ((inst->opcode == SHADER_OPCODE_GFX7_SCRATCH_READ ||
      inst->opcode == SHADER_OPCODE_GFX4_SCRATCH_READ) &&
   inst->dst.file == VGRF)
   ra_add_node_interference(g, first_vgrf_node + inst->dst.nr,
            /* From the Skylake PRM Vol. 2a docs for sends:
   *
   *    "It is required that the second block of GRFs does not overlap with
   *    the first block."
   *
   * Normally, this is taken care of by fixup_sends_duplicate_payload() but
   * in the case where one of the registers is an undefined value, the
   * register allocator may decide that they don't interfere even though
   * they're used as sources in the same instruction.  We also need to add
   * interference here.
   */
   if (devinfo->ver >= 9) {
      if (inst->opcode == SHADER_OPCODE_SEND && inst->ex_mlen > 0 &&
      inst->src[2].file == VGRF && inst->src[3].file == VGRF &&
   inst->src[2].nr != inst->src[3].nr)
   ra_add_node_interference(g, first_vgrf_node + inst->src[2].nr,
            /* When we do send-from-GRF for FB writes, we need to ensure that the last
   * write instruction sends from a high register.  This is because the
   * vertex fetcher wants to start filling the low payload registers while
   * the pixel data port is still working on writing out the memory.  If we
   * don't do this, we get rendering artifacts.
   *
   * We could just do "something high".  Instead, we just pick the highest
   * register that works.
   */
   if (inst->eot) {
      const int vgrf = inst->opcode == SHADER_OPCODE_SEND ?
         const int size = DIV_ROUND_UP(fs->alloc.sizes[vgrf], reg_unit(devinfo));
            if (first_mrf_hack_node >= 0) {
      /* If something happened to spill, we want to push the EOT send
   * register early enough in the register file that we don't
   * conflict with any used MRF hack registers.
   */
      } else if (grf127_send_hack_node >= 0) {
      /* Avoid r127 which might be unusable if the node was previously
   * written by a SIMD8 SEND message with source/destination overlap.
   */
                        if (inst->ex_mlen > 0) {
      const int vgrf = inst->src[3].nr;
   reg -= DIV_ROUND_UP(fs->alloc.sizes[vgrf], reg_unit(devinfo));
            }
      void
   fs_reg_alloc::build_interference_graph(bool allow_spilling)
   {
      /* Compute the RA node layout */
   node_count = 0;
   first_payload_node = node_count;
   node_count += payload_node_count;
   if (devinfo->ver >= 7 && devinfo->ver < 9 && allow_spilling) {
      first_mrf_hack_node = node_count;
      } else {
         }
   if (devinfo->ver >= 8) {
      grf127_send_hack_node = node_count;
      } else {
         }
   first_vgrf_node = node_count;
   node_count += fs->alloc.count;
   last_vgrf_node = node_count - 1;
   if ((devinfo->ver >= 9 && devinfo->verx10 < 125) && allow_spilling) {
         } else {
         }
            fs->calculate_payload_ranges(payload_node_count,
            assert(g == NULL);
   g = ra_alloc_interference_graph(compiler->fs_reg_sets[rsi].regs, node_count);
            /* Set up the payload nodes */
   for (int i = 0; i < payload_node_count; i++)
            if (first_mrf_hack_node >= 0) {
      /* Mark each MRF reg node as being allocated to its physical
   * register.
   *
   * The alternative would be to have per-physical-register classes,
   * which would just be silly.
   */
   for (int i = 0; i < BRW_MAX_MRF(devinfo->ver); i++) {
      ra_set_node_reg(g, first_mrf_hack_node + i,
                  if (grf127_send_hack_node >= 0)
            /* Specify the classes of each virtual register. */
   for (unsigned i = 0; i < fs->alloc.count; i++) {
               assert(size <= ARRAY_SIZE(compiler->fs_reg_sets[rsi].classes) &&
            ra_set_node_class(g, first_vgrf_node + i,
               /* Special case: on pre-Gfx7 hardware that supports PLN, the second operand
   * of a PLN instruction needs to be an even-numbered register, so we have a
   * special register class aligned_bary_class to handle this case.
   */
   if (compiler->fs_reg_sets[rsi].aligned_bary_class) {
      foreach_block_and_inst(block, fs_inst, inst, fs->cfg) {
      if (inst->opcode == FS_OPCODE_LINTERP && inst->src[0].file == VGRF &&
      fs->alloc.sizes[inst->src[0].nr] ==
         ra_set_node_class(g, first_vgrf_node + inst->src[0].nr,
                     /* Add interference based on the live range of the register */
   for (unsigned i = 0; i < fs->alloc.count; i++) {
      setup_live_interference(first_vgrf_node + i,
                     /* Add interference based on the instructions in which a register is used.
   */
   foreach_block_and_inst(block, fs_inst, inst, fs->cfg)
      }
      void
   fs_reg_alloc::discard_interference_graph()
   {
      ralloc_free(g);
   g = NULL;
      }
      fs_reg
   fs_reg_alloc::build_single_offset(const fs_builder &bld, uint32_t spill_offset, int ip)
   {
      fs_reg offset = retype(alloc_spill_reg(1, ip), BRW_REGISTER_TYPE_UD);
   fs_inst *inst = bld.MOV(offset, brw_imm_ud(spill_offset));
   _mesa_set_add(spill_insts, inst);
      }
      fs_reg
   fs_reg_alloc::build_lane_offsets(const fs_builder &bld, uint32_t spill_offset, int ip)
   {
      /* LSC messages are limited to SIMD16 */
            const fs_builder ubld = bld.exec_all();
            fs_reg offset = retype(alloc_spill_reg(reg_count, ip), BRW_REGISTER_TYPE_UD);
            /* Build an offset per lane in SIMD8 */
   inst = ubld.group(8, 0).MOV(retype(offset, BRW_REGISTER_TYPE_UW),
         _mesa_set_add(spill_insts, inst);
   inst = ubld.group(8, 0).MOV(offset, retype(offset, BRW_REGISTER_TYPE_UW));
            /* Build offsets in the upper 8 lanes of SIMD16 */
   if (ubld.dispatch_width() > 8) {
      inst = ubld.group(8, 0).ADD(
      byte_offset(offset, REG_SIZE),
   byte_offset(offset, 0),
                  /* Make the offset a dword */
   inst = ubld.SHL(offset, offset, brw_imm_ud(2));
            /* Add the base offset */
   inst = ubld.ADD(offset, offset, brw_imm_ud(spill_offset));
               }
      void
   fs_reg_alloc::emit_unspill(const fs_builder &bld,
                     {
      const intel_device_info *devinfo = bld.shader->devinfo;
   const unsigned reg_size = dst.component_size(bld.dispatch_width()) /
                  for (unsigned i = 0; i < count / reg_size; i++) {
               fs_inst *unspill_inst;
   if (devinfo->verx10 >= 125) {
      /* LSC is limited to SIMD16 load/store but we can load more using
   * transpose messages.
   */
   const bool use_transpose = bld.dispatch_width() > 16;
   const fs_builder ubld = use_transpose ? bld.exec_all().group(1, 0) : bld;
   fs_reg offset;
   if (use_transpose) {
         } else {
         }
   /* We leave the extended descriptor empty and flag the instruction to
   * ask the generated to insert the extended descriptor in the address
   * register. That way we don't need to burn an additional register
   * for register allocation spill/fill.
   */
   fs_reg srcs[] = {
      brw_imm_ud(0), /* desc */
   brw_imm_ud(0), /* ex_desc */
   offset,        /* payload */
               unspill_inst = ubld.emit(SHADER_OPCODE_SEND, dst,
         unspill_inst->sfid = GFX12_SFID_UGM;
   unspill_inst->desc = lsc_msg_desc(devinfo, LSC_OP_LOAD,
                                    unspill_inst->exec_size,
   LSC_ADDR_SURFTYPE_SS,
   LSC_ADDR_SIZE_A32,
      unspill_inst->header_size = 0;
   unspill_inst->mlen =
         unspill_inst->ex_mlen = 0;
   unspill_inst->size_written =
         unspill_inst->send_has_side_effects = false;
   unspill_inst->send_is_volatile = true;
      } else if (devinfo->ver >= 9) {
      fs_reg header = this->scratch_header;
   fs_builder ubld = bld.exec_all().group(1, 0);
   assert(spill_offset % 16 == 0);
   unspill_inst = ubld.MOV(component(header, 2),
                                 fs_reg srcs[] = { brw_imm_ud(0), ex_desc, header };
   unspill_inst = bld.emit(SHADER_OPCODE_SEND, dst,
         unspill_inst->mlen = 1;
   unspill_inst->header_size = 1;
   unspill_inst->size_written = reg_size * REG_SIZE;
   unspill_inst->send_has_side_effects = false;
   unspill_inst->send_is_volatile = true;
   unspill_inst->sfid = GFX7_SFID_DATAPORT_DATA_CACHE;
   unspill_inst->desc =
      brw_dp_desc(devinfo, bti,
         } else if (devinfo->ver >= 7 && spill_offset < (1 << 12) * REG_SIZE) {
      /* The Gfx7 descriptor-based offset is 12 bits of HWORD units.
   * Because the Gfx7-style scratch block read is hardwired to BTI 255,
   * on Gfx9+ it would cause the DC to do an IA-coherent read, what
   * largely outweighs the slight advantage from not having to provide
   * the address as part of the message header, so we're better off
   * using plain old oword block reads.
   */
   unspill_inst = bld.emit(SHADER_OPCODE_GFX7_SCRATCH_READ, dst);
      } else {
      unspill_inst = bld.emit(SHADER_OPCODE_GFX4_SCRATCH_READ, dst);
   unspill_inst->offset = spill_offset;
   unspill_inst->base_mrf = spill_base_mrf(bld.shader);
      }
            dst.offset += reg_size * REG_SIZE;
         }
      void
   fs_reg_alloc::emit_spill(const fs_builder &bld,
                     {
      const intel_device_info *devinfo = bld.shader->devinfo;
   const unsigned reg_size = src.component_size(bld.dispatch_width()) /
                  for (unsigned i = 0; i < count / reg_size; i++) {
               fs_inst *spill_inst;
   if (devinfo->verx10 >= 125) {
      fs_reg offset = build_lane_offsets(bld, spill_offset, ip);
   /* We leave the extended descriptor empty and flag the instruction
   * relocate the extended descriptor. That way the surface offset is
   * directly put into the instruction and we don't need to use a
   * register to hold it.
   */
   fs_reg srcs[] = {
      brw_imm_ud(0),        /* desc */
   brw_imm_ud(0),        /* ex_desc */
   offset,               /* payload */
      };
   spill_inst = bld.emit(SHADER_OPCODE_SEND, bld.null_reg_f(),
         spill_inst->sfid = GFX12_SFID_UGM;
   spill_inst->desc = lsc_msg_desc(devinfo, LSC_OP_STORE,
                                 bld.dispatch_width(),
   LSC_ADDR_SURFTYPE_SS,
   LSC_ADDR_SIZE_A32,
   1 /* num_coordinates */,
   spill_inst->header_size = 0;
   spill_inst->mlen = lsc_msg_desc_src0_len(devinfo, spill_inst->desc);
   spill_inst->ex_mlen = reg_size;
   spill_inst->size_written = 0;
   spill_inst->send_has_side_effects = true;
   spill_inst->send_is_volatile = false;
      } else if (devinfo->ver >= 9) {
      fs_reg header = this->scratch_header;
   fs_builder ubld = bld.exec_all().group(1, 0);
   assert(spill_offset % 16 == 0);
   spill_inst = ubld.MOV(component(header, 2),
                                 fs_reg srcs[] = { brw_imm_ud(0), ex_desc, header, src };
   spill_inst = bld.emit(SHADER_OPCODE_SEND, bld.null_reg_f(),
         spill_inst->mlen = 1;
   spill_inst->ex_mlen = reg_size;
   spill_inst->size_written = 0;
   spill_inst->header_size = 1;
   spill_inst->send_has_side_effects = true;
   spill_inst->send_is_volatile = false;
   spill_inst->sfid = GFX7_SFID_DATAPORT_DATA_CACHE;
   spill_inst->desc =
      brw_dp_desc(devinfo, bti,
         } else {
      spill_inst = bld.emit(SHADER_OPCODE_GFX4_SCRATCH_WRITE,
         spill_inst->offset = spill_offset;
   spill_inst->mlen = 1 + reg_size; /* header, value */
      }
            src.offset += reg_size * REG_SIZE;
         }
      void
   fs_reg_alloc::set_spill_costs()
   {
      float block_scale = 1.0;
   float spill_costs[fs->alloc.count];
            for (unsigned i = 0; i < fs->alloc.count; i++) {
      spill_costs[i] = 0.0;
               /* Calculate costs for spilling nodes.  Call it a cost of 1 per
   * spill/unspill we'll have to do, and guess that the insides of
   * loops run 10 times.
   */
   foreach_block_and_inst(block, fs_inst, inst, fs->cfg) {
      if (inst->src[i].file == VGRF)
                        if (inst->dst.file == VGRF)
            /* Don't spill anything we generated while spilling */
   if (_mesa_set_search(spill_insts, inst)) {
      if (inst->src[i].file == VGRF)
            if (inst->dst.file == VGRF)
                                 block_scale *= 10;
   break;
            block_scale /= 10;
   break;
            case BRW_OPCODE_IF:
   case BRW_OPCODE_IFF:
                  case BRW_OPCODE_ENDIF:
                  break;
                     for (unsigned i = 0; i < fs->alloc.count; i++) {
      /* Do the no_spill check first.  Registers that are used as spill
   * temporaries may have been allocated after we calculated liveness so
   * we shouldn't look their liveness up.  Fortunately, they're always
   * used in SCRATCH_READ/WRITE instructions so they'll always be flagged
   * no_spill.
   */
   if (no_spill[i])
            int live_length = live.vgrf_end[i] - live.vgrf_start[i];
   if (live_length <= 0)
            /* Divide the cost (in number of spills/fills) by the log of the length
   * of the live range of the register.  This will encourage spill logic
   * to spill long-living things before spilling short-lived things where
   * spilling is less likely to actually do us any good.  We use the log
   * of the length because it will fall off very quickly and not cause us
   * to spill medium length registers with more uses.
   */
   float adjusted_cost = spill_costs[i] / logf(live_length);
                  }
      int
   fs_reg_alloc::choose_spill_reg()
   {
      if (!have_spill_costs)
            int node = ra_get_best_spill_node(g);
   if (node < 0)
            assert(node >= first_vgrf_node);
      }
      fs_reg
   fs_reg_alloc::alloc_scratch_header()
   {
      int vgrf = fs->alloc.allocate(1);
   assert(first_vgrf_node + vgrf == scratch_header_node);
   ra_set_node_class(g, scratch_header_node,
                        }
      fs_reg
   fs_reg_alloc::alloc_spill_reg(unsigned size, int ip)
   {
      int vgrf = fs->alloc.allocate(ALIGN(size, reg_unit(devinfo)));
   int class_idx = DIV_ROUND_UP(size, reg_unit(devinfo)) - 1;
   int n = ra_add_node(g, compiler->fs_reg_sets[rsi].classes[class_idx]);
   assert(n == first_vgrf_node + vgrf);
                     /* Add interference between this spill node and any other spill nodes for
   * the same instruction.
   */
   for (int s = 0; s < spill_node_count; s++) {
      if (spill_vgrf_ip[s] == ip)
               /* Add this spill node to the list for next time */
   if (spill_node_count >= spill_vgrf_ip_alloc) {
      if (spill_vgrf_ip_alloc == 0)
         else
         spill_vgrf_ip = reralloc(mem_ctx, spill_vgrf_ip, int,
      }
               }
      void
   fs_reg_alloc::spill_reg(unsigned spill_reg)
   {
      int size = fs->alloc.sizes[spill_reg];
   unsigned int spill_offset = fs->last_scratch;
            /* Spills may use MRFs 13-15 in the SIMD16 case.  Our texturing is done
   * using up to 11 MRFs starting from either m1 or m2, and fb writes can use
   * up to m13 (gfx6+ simd16: 2 header + 8 color + 2 src0alpha + 2 omask) or
   * m15 (gfx4-5 simd16: 2 header + 8 color + 1 aads + 2 src depth + 2 dst
   * depth), starting from m1.  In summary: We may not be able to spill in
   * SIMD16 mode, because we'd stomp the FB writes.
   */
   if (!fs->spilled_any_registers) {
      if (devinfo->verx10 >= 125) {
         } else if (devinfo->ver >= 9) {
      this->scratch_header = alloc_scratch_header();
                  fs_inst *inst = ubld.emit(SHADER_OPCODE_SCRATCH_HEADER,
            } else {
                     for (int i = spill_base_mrf(fs); i < BRW_MAX_MRF(devinfo->ver); i++) {
      if (mrf_used[i]) {
         return;
                                       /* We're about to replace all uses of this register.  It no longer
   * conflicts with anything so we can get rid of its interference.
   */
   ra_set_node_spill_cost(g, first_vgrf_node + spill_reg, 0);
            /* Generate spill/unspill instructions for the objects being
   * spilled.  Right now, we spill or unspill the whole thing to a
   * virtual grf of the same size.  For most instructions, though, we
   * could just spill/unspill the GRF being accessed.
   */
   int ip = 0;
   foreach_block_and_inst (block, fs_inst, inst, fs->cfg) {
      const fs_builder ibld = fs_builder(fs, block, inst);
   exec_node *before = inst->prev;
            if (inst->src[i].file == VGRF &&
               inst->src[i].nr == spill_reg) {
   int count = regs_read(inst, i);
   int subset_spill_offset = spill_offset +
                                 /* We read the largest power-of-two divisor of the register count
   * (because only POT scratch read blocks are allowed by the
   * hardware) up to the maximum supported block size.
   */
                  /* Set exec_all() on unspill messages under the (rather
   * pessimistic) assumption that there is no one-to-one
   * correspondence between channels of the spilled variable in
   * scratch space and the scratch read message, which operates on
   * 32 bit channels.  It shouldn't hurt in any case because the
   * unspill destination is a block-local temporary.
   */
   }
                  if (inst->dst.file == VGRF &&
      inst->dst.nr == spill_reg &&
   inst->opcode != SHADER_OPCODE_UNDEF) {
   int subset_spill_offset = spill_offset +
                                 /* If we're immediately spilling the register, we should not use
   * destination dependency hints.  Doing so will cause the GPU do
   * try to read and write the register at the same time and may
   * hang the GPU.
   */
                  /* Calculate the execution width of the scratch messages (which work
   * in terms of 32 bit components so we have a fixed number of eight
   * channels per spilled register).  We attempt to write one
   * exec_size-wide component of the variable at a time without
   * exceeding the maximum number of (fake) MRF registers reserved for
   * spills.
   */
   const unsigned width = 8 * reg_unit(devinfo) *
      DIV_ROUND_UP(MIN2(inst->dst.component_size(inst->exec_size),
               /* Spills should only write data initialized by the instruction for
   * whichever channels are enabled in the execution mask.  If that's
   * not possible we'll have to emit a matching unspill before the
   * instruction and set force_writemask_all on the spill.
   */
   const bool per_channel =
                        /* If our write is going to affect just part of the
            * regs_written(inst), then we need to unspill the destination since
   * we write back out all of the regs_written().  If the original
   * instruction had force_writemask_all set and is not a partial
   * write, there should be no need for the unspill since the
   */
         if (inst->is_partial_write() ||
      (!inst->force_writemask_all && !per_channel))
               emit_spill(ubld.at(block, inst->next), &fs->shader_stats, spill_src,
               for (fs_inst *inst = (fs_inst *)before->next;
                  /* We don't advance the ip for scratch read/write instructions
   * because we consider them to have the same ip as instruction we're
   * spilling around for the purposes of interference.  Also, we're
   * inserting spill instructions without re-running liveness analysis
   * and we don't want to mess up our IPs.
   */
   if (!_mesa_set_search(spill_insts, inst))
                  }
      bool
   fs_reg_alloc::assign_regs(bool allow_spilling, bool spill_all)
   {
               unsigned spilled = 0;
   while (1) {
      /* Debug of register spilling: Go spill everything. */
   if (unlikely(spill_all)) {
      int reg = choose_spill_reg();
   if (reg != -1) {
      spill_reg(reg);
                  if (ra_allocate(g))
            if (!allow_spilling)
            /* Failed to allocate registers.  Spill some regs, and the caller will
   * loop back into here to try again.
   */
   unsigned nr_spills = 1;
   if (compiler->spilling_rate)
            for (unsigned j = 0; j < nr_spills; j++) {
      int reg = choose_spill_reg();
   if (reg == -1) {
      if (j == 0)
                     /* If we're going to spill but we've never spilled before, we need
   * to re-build the interference graph with MRFs enabled to allow
   * spilling.
   */
   if (!fs->spilled_any_registers) {
      discard_interference_graph();
               spill_reg(reg);
                  if (spilled)
            /* Get the chosen virtual registers for each node, and map virtual
   * regs in the register classes back down to real hardware reg
   * numbers.
   */
   unsigned hw_reg_mapping[fs->alloc.count];
   fs->grf_used = fs->first_non_payload_grf;
   for (unsigned i = 0; i < fs->alloc.count; i++) {
               hw_reg_mapping[i] = reg;
      hw_reg_mapping[i] + DIV_ROUND_UP(fs->alloc.sizes[i],
                  foreach_block_and_inst(block, fs_inst, inst, fs->cfg) {
      assign_reg(devinfo, hw_reg_mapping, &inst->dst);
   for (int i = 0; i < inst->sources; i++) {
                                 }
      bool
   fs_visitor::assign_regs(bool allow_spilling, bool spill_all)
   {
      fs_reg_alloc alloc(this);
   bool success = alloc.assign_regs(allow_spilling, spill_all);
   if (!success && allow_spilling) {
      fail("no register to spill:\n");
      }
      }
