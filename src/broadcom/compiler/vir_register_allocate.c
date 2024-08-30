   /*
   * Copyright © 2014 Broadcom
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
      #include "util/ralloc.h"
   #include "util/register_allocate.h"
   #include "common/v3d_device_info.h"
   #include "v3d_compiler.h"
      #define ACC_INDEX     0
   #define ACC_COUNT     6
      /* RA nodes used to track RF registers with implicit writes */
   #define IMPLICIT_RF_COUNT 1
      #define PHYS_COUNT 64
      static uint8_t
   get_phys_index(const struct v3d_device_info *devinfo)
   {
         if (devinfo->has_accumulators)
         else
   }
      /* ACC as accumulator */
   #define CLASS_BITS_PHYS   (1 << 0)
   #define CLASS_BITS_ACC    (1 << 1)
   #define CLASS_BITS_R5     (1 << 4)
      static uint8_t
   get_class_bit_any(const struct v3d_device_info *devinfo)
   {
         if (devinfo->has_accumulators)
         else
   }
      static uint8_t
   filter_class_bits(const struct v3d_device_info *devinfo, uint8_t class_bits)
   {
      if (!devinfo->has_accumulators) {
      assert(class_bits & CLASS_BITS_PHYS);
      }
      }
      static inline uint32_t
   temp_to_node(struct v3d_compile *c, uint32_t temp)
   {
         return temp + (c->devinfo->has_accumulators ? ACC_COUNT :
   }
      static inline uint32_t
   node_to_temp(struct v3d_compile *c, uint32_t node)
   {
         assert((c->devinfo->has_accumulators && node >= ACC_COUNT) ||
         return node - (c->devinfo->has_accumulators ? ACC_COUNT :
   }
      static inline uint8_t
   get_temp_class_bits(struct v3d_compile *c,
         {
         }
      static inline void
   set_temp_class_bits(struct v3d_compile *c,
         {
         }
      static struct ra_class *
   choose_reg_class(struct v3d_compile *c, uint8_t class_bits)
   {
         if (class_bits == CLASS_BITS_PHYS) {
         } else if (class_bits == (CLASS_BITS_R5)) {
               } else if (class_bits == (CLASS_BITS_PHYS | CLASS_BITS_ACC)) {
               } else {
               }
      static inline struct ra_class *
   choose_reg_class_for_temp(struct v3d_compile *c, uint32_t temp)
   {
         assert(temp < c->num_temps && temp < c->nodes.alloc_count);
   }
      static inline bool
   qinst_writes_tmu(const struct v3d_device_info *devinfo,
         {
         return (inst->dst.file == QFILE_MAGIC &&
         }
      static bool
   is_end_of_tmu_sequence(const struct v3d_device_info *devinfo,
         {
         /* Only tmuwt and ldtmu can finish TMU sequences */
   bool is_tmuwt = inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
         bool is_ldtmu = inst->qpu.sig.ldtmu;
   if (!is_tmuwt && !is_ldtmu)
            /* Check if this is the last tmuwt or ldtmu in the sequence */
   list_for_each_entry_from(struct qinst, scan_inst, inst->link.next,
                                                                  }
      static bool
   vir_is_mov_uniform(struct v3d_compile *c, int temp)
   {
                  }
      static bool
   can_reconstruct_inst(struct qinst *inst)
   {
                  if (vir_is_add(inst)) {
            switch (inst->qpu.alu.add.op) {
   case V3D_QPU_A_FXCD:
   case V3D_QPU_A_FYCD:
   case V3D_QPU_A_XCD:
   case V3D_QPU_A_YCD:
   case V3D_QPU_A_IID:
   case V3D_QPU_A_EIDX:
   case V3D_QPU_A_TIDX:
   case V3D_QPU_A_SAMPID:
            /* No need to check input unpacks because none of these
   * opcodes read sources. FXCD,FYCD have pack variants.
   */
   return inst->qpu.flags.ac == V3D_QPU_COND_NONE &&
            default:
               }
      static bool
   can_reconstruct_temp(struct v3d_compile *c, int temp)
   {
         struct qinst *def = c->defs[temp];
   }
      static struct qreg
   reconstruct_temp(struct v3d_compile *c, enum v3d_qpu_add_op op)
   {
         struct qreg dest;
   switch (op) {
   case V3D_QPU_A_FXCD:
               case V3D_QPU_A_FYCD:
               case V3D_QPU_A_XCD:
               case V3D_QPU_A_YCD:
               case V3D_QPU_A_IID:
               case V3D_QPU_A_EIDX:
               case V3D_QPU_A_TIDX:
               case V3D_QPU_A_SAMPID:
               default:
                  }
      enum temp_spill_type {
         SPILL_TYPE_UNIFORM,
   SPILL_TYPE_RECONSTRUCT,
   };
      static enum temp_spill_type
   get_spill_type_for_temp(struct v3d_compile *c, int temp)
   {
      if (vir_is_mov_uniform(c, temp))
            if (can_reconstruct_temp(c, temp))
               }
      static int
   v3d_choose_spill_node(struct v3d_compile *c)
   {
         const float tmu_scale = 10;
   float block_scale = 1.0;
   float spill_costs[c->num_temps];
   bool in_tmu_operation = false;
            for (unsigned i = 0; i < c->num_temps; i++)
            /* XXX: Scale the cost up when inside of a loop. */
   vir_for_each_block(block, c) {
            vir_for_each_inst(inst, block) {
            /* We can't insert new thread switches after
   * starting output writes.
                                                                                    if (spill_type != SPILL_TYPE_TMU) {
         } else if (!no_spilling) {
            float tmu_op_scale = in_tmu_operation ?
                                                                        if (spill_type != SPILL_TYPE_TMU) {
         } else if (!no_spilling) {
                                    /* Refuse to spill a ldvary's dst, because that means
   * that ldvary's r5 would end up being used across a
   * thrsw.
   */
                                             /* Track when we're in between a TMU setup and the
   * final LDTMU or TMUWT from that TMU setup.  We
                                       /* We always emit a "last thrsw" to ensure all our spilling occurs
      * before the last thread section. See vir_emit_last_thrsw.
               for (unsigned i = 0; i < c->num_temps; i++) {
            if (BITSET_TEST(c->spillable, i)) {
                     }
      static void
   ensure_nodes(struct v3d_compile *c)
   {
         if (c->num_temps < c->nodes.alloc_count)
            c->nodes.alloc_count *= 2;
   c->nodes.info = reralloc_array_size(c,
                     }
      /* Creates the interference node for a new temp. We use this to keep the node
   * list updated during the spilling process, which generates new temps/nodes.
   */
   static void
   add_node(struct v3d_compile *c, uint32_t temp, uint8_t class_bits)
   {
                  int node = ra_add_node(c->g, choose_reg_class(c, class_bits));
   assert(c->devinfo->has_accumulators ? node == temp + ACC_COUNT :
            /* We fill the node priority after we are done inserting spills */
   c->nodes.info[node].class_bits = class_bits;
   c->nodes.info[node].priority = 0;
   c->nodes.info[node].is_ldunif_dst = false;
   c->nodes.info[node].is_program_end = false;
   }
      /* The spill offset for this thread takes a bit of setup, so do it once at
   * program start.
   */
   void
   v3d_setup_spill_base(struct v3d_compile *c)
   {
         /* Setting up the spill base is done in the entry block; so change
      * both the current block to emit and the cursor.
      struct qblock *current_block = c->cur_block;
   c->cur_block = vir_entry_block(c);
                     /* Each thread wants to be in a separate region of the scratch space
      * so that the QPUs aren't fighting over cache lines.  We have the
   * driver keep a single global spill BO rather than
   * per-spilling-program BOs, so we need a uniform from the driver for
   * what the per-thread scale is.
      struct qreg thread_offset =
                        /* Each channel in a reg is 4 bytes, so scale them up by that. */
   struct qreg element_offset = vir_SHL(c, vir_EIDX(c),
            c->spill_base = vir_ADD(c,
                  /* Make sure that we don't spill the spilling setup instructions. */
   for (int i = start_num_temps; i < c->num_temps; i++) {
                     /* If we are spilling, update the RA map with the temps added
   * by the spill setup. Our spill_base register can never be an
   * accumulator because it is used for TMU spill/fill and thus
   * needs to persist across thread switches.
   */
   if (c->spilling) {
            int temp_class = CLASS_BITS_PHYS;
   if (c->devinfo->has_accumulators &&
      i != c->spill_base.index) {
               /* Restore the current block. */
   c->cur_block = current_block;
   }
      /**
   * Computes the address for a spill/fill sequence and completes the spill/fill
   * sequence by emitting the following code:
   *
   * ldunif.spill_offset
   * add tmua spill_base spill_offset
   * thrsw
   *
   * If the sequence is for a spill, then it will emit a tmuwt after the thrsw,
   * otherwise it will emit an ldtmu to load the fill result into 'fill_dst'.
   *
   * The parameter 'ip' represents the ip at which the spill/fill is happening.
   * This is used to disallow accumulators on temps that cross this ip boundary
   * due to the new thrsw itroduced in the sequence above.
   */
   static void
   v3d_emit_spill_tmua(struct v3d_compile *c,
                     uint32_t spill_offset,
   {
                  /* Load a uniform with the spill offset and add it to the spill base
      * to obtain the TMUA address. It can be of class ANY because we know
   * we are consuming it immediately without thrsw in between.
      assert(c->disable_ldunif_opt);
   struct qreg offset = vir_uniform_ui(c, spill_offset);
            /* We always enable per-quad on spills/fills to ensure we spill
      * any channels involved with helper invocations.
      struct qreg tmua = vir_reg(QFILE_MAGIC, V3D_QPU_WADDR_TMUAU);
   struct qinst *inst = vir_ADD_dest(c, tmua, c->spill_base, offset);
   inst->qpu.flags.ac = cond;
   inst->ldtmu_count = 1;
   inst->uniform = vir_get_uniform_index(c, QUNIFORM_CONSTANT,
                     /* If this is for a spill, emit a TMUWT otherwise a LDTMU to load the
      * result of the fill. The TMUWT temp is not really read, the ldtmu
   * temp will be used immediately so just like the uniform above we
   * can allow accumulators.
      int temp_class =
         if (!fill_dst) {
            struct qreg dst = vir_TMUWT(c);
      } else {
            *fill_dst = vir_LDTMU(c);
               /* Temps across the thread switch we injected can't be assigned to
      * accumulators.
   *
   * Fills inject code before ip, so anything that starts at ip or later
   * is not affected by the thrsw. Something that ends at ip will be
   * affected though.
   *
   * Spills inject code after ip, so anything that starts strictly later
   * than ip is not affected (the temp starting at ip is usually the
   * spilled temp except for postponed spills). Something that ends at ip
   * won't be affected either.
      for (int i = 0; i < c->spill_start_num_temps; i++) {
            bool thrsw_cross = fill_dst ?
               if (thrsw_cross) {
            }
      static void
   v3d_emit_tmu_spill(struct v3d_compile *c,
                     struct qinst *inst,
   struct qreg spill_temp,
   {
         assert(inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU);
                              /* If inst and position don't match, this is a postponed spill,
      * in which case we have already allocated the temp for the spill
   * and we should use that, otherwise create a new temp with the
   * same register class bits as the original.
      if (inst == position) {
            uint8_t class_bits = get_temp_class_bits(c, inst->dst.index);
      } else {
                     /* If this is a postponed spill the register being spilled may
   * have been written more than once including conditional
   * writes, so ignore predication on the spill instruction and
   * always spill the full register.
               struct qinst *tmp =
                                 c->spills++;
   }
      static inline bool
   interferes(int32_t t0_start, int32_t t0_end, int32_t t1_start, int32_t t1_end)
   {
         }
      static void
   v3d_spill_reg(struct v3d_compile *c, int *acc_nodes, int *implicit_rf_nodes,
         {
         c->spill_start_num_temps = c->num_temps;
                     uint32_t spill_offset = 0;
   if (spill_type == SPILL_TYPE_TMU) {
                                                   /* Don't allocate our spill base to rf0 to avoid
   * conflicts with instructions doing implicit writes
   * to that register.
   */
   if (!c->devinfo->has_accumulators) {
         ra_add_node_interference(
                  struct qinst *last_thrsw = c->last_thrsw;
            int uniform_index = ~0;
   if (spill_type == SPILL_TYPE_UNIFORM) {
                        enum v3d_qpu_add_op reconstruct_op = V3D_QPU_A_NOP;
   if (spill_type == SPILL_TYPE_RECONSTRUCT) {
            struct qinst *orig_def = c->defs[spill_temp];
                        /* We must disable the ldunif optimization if we are spilling uniforms */
   bool had_disable_ldunif_opt = c->disable_ldunif_opt;
            struct qinst *start_of_tmu_sequence = NULL;
   struct qinst *postponed_spill = NULL;
   struct qreg postponed_spill_temp = { 0 };
   vir_for_each_block(block, c) {
                                    /* Track when we're in between a TMU setup and the final
   * LDTMU or TMUWT from that TMU setup. We can't spill/fill any
   * temps during that time, because that involves inserting a
   * new TMU setup/LDTMU sequence, so we postpone the spill or
   * move the fill up to not intrude in the middle of the TMU
   * sequence.
   */
   if (is_end_of_tmu_sequence(c->devinfo, inst, block)) {
                                                                                    /* fills */
   int filled_src = -1;
   for (int i = 0; i < vir_get_nsrc(inst); i++) {
                                                                        if (spill_type == SPILL_TYPE_UNIFORM) {
            struct qreg unif =
            vir_uniform(c,
      inst->src[i] = unif;
   /* We are using the uniform in the
   * instruction immediately after, so
   * we can use any register class for it.
   */
      } else if (spill_type == SPILL_TYPE_RECONSTRUCT) {
            struct qreg temp =
         inst->src[i] = temp;
   /* We are using the temp in the
   * instruction immediately after so we
   * can use ACC.
   */
   int temp_class =
            } else {
            /* If we have a postponed spill, we
   * don't need a fill as the temp would
   * not have been spilled yet, however,
   * we need to update the temp index.
   */
   if (postponed_spill) {
                                                                                             /* spills */
   if (inst->dst.file == QFILE_TEMP &&
      inst->dst.index == spill_temp) {
      if (spill_type != SPILL_TYPE_TMU) {
               } else {
            /* If we are in the middle of a TMU
   * sequence, we postpone the actual
   * spill until we have finished it. We,
   * still need to replace the spill temp
   * with a new temp though.
   */
   if (start_of_tmu_sequence) {
            if (postponed_spill) {
         postponed_spill->dst =
   }
   if (!postponed_spill ||
      vir_get_cond(inst) == V3D_QPU_COND_NONE) {
      postponed_spill_temp =
         add_node(c,
            } else {
            v3d_emit_tmu_spill(c, inst,
            /* Make sure c->last_thrsw is the actual last thrsw, not just one we
      * inserted in our most recent unspill.
               /* Don't allow spilling of our spilling instructions.  There's no way
      * they can help get things colored.
      for (int i = c->spill_start_num_temps; i < c->num_temps; i++)
            /* Reset interference for spilled node */
   ra_set_node_spill_cost(c->g, spill_node, 0);
   ra_reset_node_interference(c->g, spill_node);
            /* Rebuild program ips */
   int32_t ip = 0;
   vir_for_each_inst_inorder(inst, c)
            /* Rebuild liveness */
            /* Add interferences for the new spilled temps and update interferences
      * for c->spill_base (since we may have modified its liveness). Also,
   * update node priorities based one new liveness data.
      uint32_t sb_temp =c->spill_base.index;
   uint32_t sb_node = temp_to_node(c, sb_temp);
   for (uint32_t i = 0; i < c->num_temps; i++) {
                                                for (uint32_t j = MAX2(i + 1, c->spill_start_num_temps);
         j < c->num_temps; j++) {
      if (interferes(c->temp_start[i], c->temp_end[i],
                           if (spill_type == SPILL_TYPE_TMU) {
            if (i != sb_temp &&
      interferes(c->temp_start[i], c->temp_end[i],
               c->disable_ldunif_opt = had_disable_ldunif_opt;
   }
      struct v3d_ra_select_callback_data {
         uint32_t phys_index;
   uint32_t next_acc;
   uint32_t next_phys;
   struct v3d_ra_node_info *nodes;
   };
      /* Choosing accumulators improves chances of merging QPU instructions
   * due to these merges requiring that at most 2 rf registers are used
   * by the add and mul instructions.
   */
   static bool
   v3d_ra_favor_accum(struct v3d_ra_select_callback_data *v3d_ra,
               {
         if (!v3d_ra->devinfo->has_accumulators)
            /* Favor accumulators if we have less that this number of physical
      * registers. Accumulators have more restrictions (like being
   * invalidated through thrsw), so running out of physical registers
   * even if we have accumulators available can lead to register
   * allocation failures.
      static const int available_rf_threshold = 5;
   int available_rf = 0 ;
   for (int i = 0; i < PHYS_COUNT; i++) {
            if (BITSET_TEST(regs, v3d_ra->phys_index + i))
            }
   if (available_rf < available_rf_threshold)
            /* Favor accumulators for short-lived temps (our priority represents
      * liveness), to prevent long-lived temps from grabbing accumulators
   * and preventing follow-up instructions from using them, potentially
   * leading to large portions of the shader being unable to use
   * accumulators and therefore merge instructions successfully.
      static const int priority_threshold = 20;
   if (priority <= priority_threshold)
            }
      static bool
   v3d_ra_select_accum(struct v3d_ra_select_callback_data *v3d_ra,
               {
         if (!v3d_ra->devinfo->has_accumulators)
            /* Choose r5 for our ldunifs if possible (nobody else can load to that
      * reg, and it keeps the QPU cond field free from being occupied by
   * ldunifrf).
      int r5 = ACC_INDEX + 5;
   if (BITSET_TEST(regs, r5)) {
                        /* Round-robin through our accumulators to give post-RA instruction
      * selection more options.
      for (int i = 0; i < ACC_COUNT; i++) {
                           if (BITSET_TEST(regs, acc)) {
            v3d_ra->next_acc = acc_off + 1;
            }
      static bool
   v3d_ra_select_rf(struct v3d_ra_select_callback_data *v3d_ra,
                     {
         /* If this node is for an unused temp, ignore. */
   if (v3d_ra->nodes->info[node].unused) {
                        /* In V3D 7.x, try to assign rf0 to temps used as ldunif's dst
      * so we can avoid turning them into ldunifrf (which uses the
   * cond field to encode the dst and would prevent merge with
   * instructions that use cond flags).
      if (v3d_ra->nodes->info[node].is_ldunif_dst &&
         BITSET_TEST(regs, v3d_ra->phys_index)) {
      assert(v3d_ra->devinfo->ver >= 71);
               /* The last 3 instructions in a shader can't use some specific registers
      * (usually early rf registers, depends on v3d version) so try to
   * avoid allocating these to registers used by the last instructions
   * in the shader.
      const uint32_t safe_rf_start = v3d_ra->devinfo->ver <= 42 ? 3 : 4;
   if (v3d_ra->nodes->info[node].is_program_end &&
         v3d_ra->next_phys < safe_rf_start) {
            for (int i = 0; i < PHYS_COUNT; i++) {
                                                   if (BITSET_TEST(regs, phys)) {
            v3d_ra->next_phys = phys_off + 1;
            /* If we couldn't allocate, do try to assign rf0 if it is available. */
   if (v3d_ra->devinfo->ver >= 71 &&
         BITSET_TEST(regs, v3d_ra->phys_index)) {
      v3d_ra->next_phys = 1;
               }
      static unsigned int
   v3d_ra_select_callback(unsigned int n, BITSET_WORD *regs, void *data)
   {
                  unsigned int reg;
   if (v3d_ra_favor_accum(v3d_ra, regs, v3d_ra->nodes->info[n].priority) &&
         v3d_ra_select_accum(v3d_ra, regs, &reg)) {
            if (v3d_ra_select_rf(v3d_ra, n, regs, &reg))
            /* If we ran out of physical registers try to assign an accumulator
      * if we didn't favor that option earlier.
      if (v3d_ra_select_accum(v3d_ra, regs, &reg))
            }
      bool
   vir_init_reg_sets(struct v3d_compiler *compiler)
   {
         /* Allocate up to 3 regfile classes, for the ways the physical
      * register file can be divided up for fragment shader threading.
      int max_thread_index = (compiler->devinfo->ver >= 40 ? 2 : 3);
            compiler->regs = ra_alloc_reg_set(compiler, phys_index + PHYS_COUNT,
         if (!compiler->regs)
            for (int threads = 0; threads < max_thread_index; threads++) {
            compiler->reg_class_any[threads] =
         if (compiler->devinfo->has_accumulators) {
            compiler->reg_class_r5[threads] =
                                 /* Init physical regs */
   for (int i = phys_index;
         i < phys_index + (PHYS_COUNT >> threads); i++) {
      if (compiler->devinfo->has_accumulators)
                     /* Init accumulator regs */
   if (compiler->devinfo->has_accumulators) {
            for (int i = ACC_INDEX + 0; i < ACC_INDEX + ACC_COUNT - 1; i++) {
         ra_class_add_reg(compiler->reg_class_phys_or_acc[threads], i);
   }
   /* r5 can only store a single 32-bit value, so not much can
   * use it.
   */
   ra_class_add_reg(compiler->reg_class_r5[threads],
                           }
      static inline bool
   tmu_spilling_allowed(struct v3d_compile *c)
   {
         }
      static void
   update_graph_and_reg_classes_for_inst(struct v3d_compile *c,
                           {
         int32_t ip = inst->ip;
            /* If the instruction writes r3/r4 (and optionally moves its
      * result to a temp), nothing else can be stored in r3/r4 across
   * it.
      if (vir_writes_r3_implicitly(c->devinfo, inst)) {
            for (int i = 0; i < c->num_temps; i++) {
            if (c->temp_start[i] < ip && c->temp_end[i] > ip) {
         ra_add_node_interference(c->g,
            if (vir_writes_r4_implicitly(c->devinfo, inst)) {
            for (int i = 0; i < c->num_temps; i++) {
            if (c->temp_start[i] < ip && c->temp_end[i] > ip) {
         ra_add_node_interference(c->g,
            /* If any instruction writes to a physical register implicitly
      * nothing else can write the same register across it.
      if (v3d_qpu_writes_rf0_implicitly(c->devinfo, &inst->qpu)) {
            for (int i = 0; i < c->num_temps; i++) {
            if (c->temp_start[i] < ip && c->temp_end[i] > ip) {
         ra_add_node_interference(c->g,
            if (inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU) {
            switch (inst->qpu.alu.add.op) {
   case V3D_QPU_A_LDVPMV_IN:
   case V3D_QPU_A_LDVPMV_OUT:
   case V3D_QPU_A_LDVPMD_IN:
   case V3D_QPU_A_LDVPMD_OUT:
   case V3D_QPU_A_LDVPMP:
   case V3D_QPU_A_LDVPMG_IN:
   case V3D_QPU_A_LDVPMG_OUT: {
            /* LDVPMs only store to temps (the MA flag
   * decides whether the LDVPM is in or out)
   */
   assert(inst->dst.file == QFILE_TEMP);
                     case V3D_QPU_A_RECIP:
   case V3D_QPU_A_RSQRT:
   case V3D_QPU_A_EXP:
   case V3D_QPU_A_LOG:
   case V3D_QPU_A_SIN:
   case V3D_QPU_A_RSQRT2: {
            /* The SFU instructions write directly to the
   * phys regfile.
   */
   assert(inst->dst.file == QFILE_TEMP);
                     default:
               if (inst->src[0].file == QFILE_REG) {
            switch (inst->src[0].index) {
   case 0:
            /* V3D 7.x doesn't use rf0 for thread payload */
   if (c->devinfo->ver >= 71)
            case 1:
   case 2:
   case 3: {
            /* Payload setup instructions: Force allocate
   * the dst to the given register (so the MOV
   * will disappear).
   */
   assert(inst->qpu.alu.mul.op == V3D_QPU_M_MOV);
   assert(inst->dst.file == QFILE_TEMP);
   uint32_t node = temp_to_node(c, inst->dst.index);
   ra_set_node_reg(c->g, node,
                        /* Don't allocate rf0 to temps that cross ranges where we have
      * live implicit rf0 writes from ldvary. We can identify these
   * by tracking the last ldvary instruction and explicit reads
   * of rf0.
      if (c->devinfo->ver >= 71 &&
         ((inst->src[0].file == QFILE_REG && inst->src[0].index == 0) ||
   (vir_get_nsrc(inst) > 1 &&
      inst->src[1].file == QFILE_REG && inst->src[1].index == 0))) {
   for (int i = 0; i < c->num_temps; i++) {
            if (c->temp_start[i] < ip &&
      c->temp_end[i] > last_ldvary_ip) {
                     if (inst->dst.file == QFILE_TEMP) {
            /* Only a ldunif gets to write to R5, which only has a
   * single 32-bit channel of storage.
   *
   * NOTE: ldunifa is subject to the same, however, going by
   * shader-db it is best to keep r5 exclusive to ldunif, probably
   * because ldunif has usually a shorter lifespan, allowing for
   * more accumulator reuse and QPU merges.
   */
   if (c->devinfo->has_accumulators) {
            if (!inst->qpu.sig.ldunif) {
                                    } else {
         /* Until V3D 4.x, we could only load a uniform
      * to r5, so we'll need to spill if uniform
   * loads interfere with each other.
      if (c->devinfo->ver < 40) {
            } else {
            /* Make sure we don't allocate the ldvary's
   * destination to rf0, since it would clash
   * with its implicit write to that register.
   */
   if (inst->qpu.sig.ldvary) {
         ra_add_node_interference(c->g,
         }
   /* Flag dst temps from ldunif(a) instructions
   * so we can try to assign rf0 to them and avoid
   * converting these to ldunif(a)rf.
   */
   if (inst->qpu.sig.ldunif || inst->qpu.sig.ldunifa) {
         const uint32_t dst_n =
            /* All accumulators are invalidated across a thread switch. */
   if (inst->qpu.sig.thrsw && c->devinfo->has_accumulators) {
            for (int i = 0; i < c->num_temps; i++) {
            if (c->temp_start[i] < ip && c->temp_end[i] > ip) {
         }
      static void
   flag_program_end_nodes(struct v3d_compile *c)
   {
         /* Only look for registers used in this many instructions */
            struct qblock *last_block = vir_exit_block(c);
   list_for_each_entry_rev(struct qinst, inst, &last_block->instructions, link) {
                           int num_src = v3d_qpu_add_op_num_src(inst->qpu.alu.add.op);
   for (int i = 0; i < num_src; i++) {
            if (inst->src[i].file == QFILE_TEMP) {
                     num_src = v3d_qpu_mul_op_num_src(inst->qpu.alu.mul.op);
   for (int i = 0; i < num_src; i++) {
                                       if (inst->dst.file == QFILE_TEMP) {
                           }
      /**
   * Returns a mapping from QFILE_TEMP indices to struct qpu_regs.
   *
   * The return value should be freed by the caller.
   */
   struct qpu_reg *
   v3d_register_allocate(struct v3d_compile *c)
   {
         int acc_nodes[ACC_COUNT];
            unsigned num_ra_nodes = c->num_temps;
   if (c->devinfo->has_accumulators)
         else
            c->nodes = (struct v3d_ra_node_info) {
            .alloc_count = c->num_temps,
                        struct v3d_ra_select_callback_data callback_data = {
            .phys_index = phys_index,
   .next_acc = 0,
   /* Start at RF3, to try to keep the TLB writes from using
   * RF0-2. Start at RF4 in 7.x to prevent TLB writes from
   * using RF2-3.
   */
   .next_phys = c->devinfo->ver <= 42 ? 3 : 4,
                        /* Convert 1, 2, 4 threads to 0, 1, 2 index.
      *
   * V3D 4.x has double the physical register space, so 64 physical regs
   * are available at both 1x and 2x threading, and 4x has 32.
      c->thread_index = ffs(c->threads) - 1;
   if (c->devinfo->ver >= 40) {
                        c->g = ra_alloc_interference_graph(c->compiler->regs, num_ra_nodes);
            /* Make some fixed nodes for the accumulators, which we will need to
      * interfere with when ops have implied r3/r4 writes or for the thread
   * switches.  We could represent these as classes for the nodes to
   * live in, but the classes take up a lot of memory to set up, so we
   * don't want to make too many. We use the same mechanism on platforms
   * without accumulators that can have implicit writes to phys regs.
      for (uint32_t i = 0; i < num_ra_nodes; i++) {
            c->nodes.info[i].is_ldunif_dst = false;
   c->nodes.info[i].is_program_end = false;
   c->nodes.info[i].unused = false;
   c->nodes.info[i].priority = 0;
   c->nodes.info[i].class_bits = 0;
   if (c->devinfo->has_accumulators && i < ACC_COUNT) {
               } else if (!c->devinfo->has_accumulators &&
                     } else {
            uint32_t t = node_to_temp(c, i);
   c->nodes.info[i].priority =
                  /* Walk the instructions adding register class restrictions and
      * interferences.
      int ip = 0;
   int last_ldvary_ip = -1;
   vir_for_each_inst_inorder(inst, c) {
                     /* ldunif(a) always write to a temporary, so we have
   * liveness info available to decide if rf0 is
   * available for them, however, ldvary is different:
   * it always writes to rf0 directly so we don't have
   * liveness information for its implicit rf0 write.
   *
   * That means the allocator may assign rf0 to a temp
   * that is defined while an implicit rf0 write from
   * ldvary is still live. We fix that by manually
   * tracking rf0 live ranges from ldvary instructions.
                        update_graph_and_reg_classes_for_inst(c, acc_nodes,
               /* Flag the nodes that are used in the last instructions of the program
      * (there are some registers that cannot be used in the last 3
   * instructions). We only do this for fragment shaders, because the idea
   * is that by avoiding this conflict we may be able to emit the last
   * thread switch earlier in some cases, however, in non-fragment shaders
   * this won't happen because the last instructions are always VPM stores
   * with a small immediate, which conflicts with other signals,
   * preventing us from ever moving the thrsw earlier.
      if (c->s->info.stage == MESA_SHADER_FRAGMENT)
            /* Set the register classes for all our temporaries in the graph */
   for (uint32_t i = 0; i < c->num_temps; i++) {
                        /* Add register interferences based on liveness data */
   for (uint32_t i = 0; i < c->num_temps; i++) {
            /* And while we are here, let's also flag nodes for
   * unused temps.
                        for (uint32_t j = i + 1; j < c->num_temps; j++) {
            if (interferes(c->temp_start[i], c->temp_end[i],
               ra_add_node_interference(c->g,
            /* Debug option to force a bit of TMU spilling, for running
      * across conformance tests to make sure that spilling works.
      const int force_register_spills = 0;
   if (force_register_spills > 0)
            struct qpu_reg *temp_registers = NULL;
   while (true) {
            if (c->spill_size <
      V3D_CHANNELS * sizeof(uint32_t) * force_register_spills) {
         int node = v3d_choose_spill_node(c);
   uint32_t temp = node_to_temp(c, node);
   if (node != -1) {
                                    /* Failed allocation, try to spill */
                        uint32_t temp = node_to_temp(c, node);
   enum temp_spill_type spill_type =
         if (spill_type != SPILL_TYPE_TMU || tmu_spilling_allowed(c)) {
            v3d_spill_reg(c, acc_nodes, implicit_rf_nodes, temp);
      } else {
               /* Allocation was successful, build the 'temp -> reg' map */
   temp_registers = calloc(c->num_temps, sizeof(*temp_registers));
   for (uint32_t i = 0; i < c->num_temps; i++) {
            int ra_reg = ra_get_node_reg(c->g, temp_to_node(c, i));
   if (ra_reg < phys_index) {
            temp_registers[i].magic = true;
      } else {
               spill_fail:
         ralloc_free(c->nodes.info);
   c->nodes.info = NULL;
   c->nodes.alloc_count = 0;
   ralloc_free(c->g);
   c->g = NULL;
   }
