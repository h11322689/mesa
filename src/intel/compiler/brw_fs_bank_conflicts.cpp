   /*
   * Copyright © 2017 Intel Corporation
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
      /** @file brw_fs_bank_conflicts.cpp
   *
   * This file contains a GRF bank conflict mitigation pass.  The pass is
   * intended to be run after register allocation and works by rearranging the
   * layout of the GRF space (without altering the semantics of the program) in
   * a way that minimizes the number of GRF bank conflicts incurred by ternary
   * instructions.
   *
   * Unfortunately there is close to no information about bank conflicts in the
   * hardware spec, but experimentally on Gfx7-Gfx9 ternary instructions seem to
   * incur an average bank conflict penalty of one cycle per SIMD8 op whenever
   * the second and third source are stored in the same GRF bank (\sa bank_of()
   * for the exact bank layout) which cannot be fetched during the same cycle by
   * the EU, unless the EU logic manages to optimize out the read cycle of a
   * duplicate source register (\sa is_conflict_optimized_out()).
   *
   * The asymptotic run-time of the algorithm is dominated by the
   * shader_conflict_weight_matrix() computation below, which is O(n) on the
   * number of instructions in the program, however for small and medium-sized
   * programs the run-time is likely to be dominated by
   * optimize_reg_permutation() which is O(m^3) on the number of GRF atoms of
   * the program (\sa partitioning), which is bounded (since the program uses a
   * bounded number of registers post-regalloc) and of the order of 100.  For
   * that reason optimize_reg_permutation() is vectorized in order to keep the
   * cubic term within reasonable bounds for m close to its theoretical maximum.
   */
      #include "brw_fs.h"
   #include "brw_cfg.h"
      #ifdef __SSE2__
      #include <emmintrin.h>
      /**
   * Thin layer around vector intrinsics so they can be easily replaced with
   * e.g. the fall-back scalar path, an implementation with different vector
   * width or using different SIMD architectures (AVX-512?!).
   *
   * This implementation operates on pairs of independent SSE2 integer vectors à
   * la SIMD16 for somewhat improved throughput.  SSE2 is supported by virtually
   * all platforms that care about bank conflicts, so this path should almost
   * always be available in practice.
   */
   namespace {
      /**
   * SIMD integer vector data type.
   */
   struct vector_type {
                  /**
   * Scalar data type matching the representation of a single component of \p
   * vector_type.
   */
            /**
   * Maximum integer value representable as a \p scalar_type.
   */
            /**
   * Number of components of a \p vector_type.
   */
            /**
   * Set the i-th component of vector \p v to \p x.
   */
   void
   set(vector_type &v, unsigned i, scalar_type x)
   {
      assert(i < vector_width);
               /**
   * Get the i-th component of vector \p v.
   */
   scalar_type
   get(const vector_type &v, unsigned i)
   {
      assert(i < vector_width);
   scalar_type x;
   memcpy(&x, (char *)v.v + i * sizeof(x), sizeof(x));
               /**
   * Add two vectors with saturation.
   */
   vector_type
   adds(const vector_type &v, const vector_type &w)
   {
      const vector_type u = {{
         _mm_adds_epi16(v.v[0], w.v[0]),
                     /**
   * Subtract two vectors with saturation.
   */
   vector_type
   subs(const vector_type &v, const vector_type &w)
   {
      const vector_type u = {{
         _mm_subs_epi16(v.v[0], w.v[0]),
                     /**
   * Compute the bitwise conjunction of two vectors.
   */
   vector_type
   mask(const vector_type &v, const vector_type &w)
   {
      const vector_type u = {{
         _mm_and_si128(v.v[0], w.v[0]),
                     /**
   * Reduce the components of a vector using saturating addition.
   */
   scalar_type
   sums(const vector_type &v)
   {
      const __m128i v8 = _mm_adds_epi16(v.v[0], v.v[1]);
   const __m128i v4 = _mm_adds_epi16(v8, _mm_shuffle_epi32(v8, 0x4e));
   const __m128i v2 = _mm_adds_epi16(v4, _mm_shuffle_epi32(v4, 0xb1));
   const __m128i v1 = _mm_adds_epi16(v2, _mm_shufflelo_epi16(v2, 0xb1));
         }
      #else
      /**
   * Thin layer around vector intrinsics so they can be easily replaced with
   * e.g. the fall-back scalar path, an implementation with different vector
   * width or using different SIMD architectures (AVX-512?!).
   *
   * This implementation operates on scalar values and doesn't rely on
   * any vector extensions.  This is mainly intended for debugging and
   * to keep this file building on exotic platforms.
   */
   namespace {
      /**
   * SIMD integer vector data type.
   */
            /**
   * Scalar data type matching the representation of a single component of \p
   * vector_type.
   */
            /**
   * Maximum integer value representable as a \p scalar_type.
   */
            /**
   * Number of components of a \p vector_type.
   */
            /**
   * Set the i-th component of vector \p v to \p x.
   */
   void
   set(vector_type &v, unsigned i, scalar_type x)
   {
      assert(i < vector_width);
               /**
   * Get the i-th component of vector \p v.
   */
   scalar_type
   get(const vector_type &v, unsigned i)
   {
      assert(i < vector_width);
               /**
   * Add two vectors with saturation.
   */
   vector_type
   adds(vector_type v, vector_type w)
   {
                  /**
   * Subtract two vectors with saturation.
   */
   vector_type
   subs(vector_type v, vector_type w)
   {
                  /**
   * Compute the bitwise conjunction of two vectors.
   */
   vector_type
   mask(vector_type v, vector_type w)
   {
                  /**
   * Reduce the components of a vector using saturating addition.
   */
   scalar_type
   sums(vector_type v)
   {
            }
      #endif
      /**
   * Swap \p x and \p y.
   */
   #define SWAP(x, y) do {                          \
         __typeof(y) _swap_tmp = y;                 \
   y = x;                                     \
            namespace {
      /**
   * Variable-length vector type intended to represent cycle-count costs for
   * arbitrary atom-to-bank assignments.  It's indexed by a pair of integers
   * (i, p), where i is an atom index and p in {0, 1} indicates the parity of
   * the conflict (respectively, whether the cost is incurred whenever the
   * atoms are assigned the same bank b or opposite-parity banks b and b^1).
   * \sa shader_conflict_weight_matrix()
   */
   struct weight_vector_type {
                        weight_vector_type(const weight_vector_type &u) :
         {
      memcpy(v, u.v,
               ~weight_vector_type()
   {
                  weight_vector_type &
   operator=(weight_vector_type u)
   {
      SWAP(v, u.v);
   SWAP(size, u.size);
               vector_type *v;
         private:
      static vector_type *
   alloc(unsigned n)
   {
      const unsigned align = MAX2(sizeof(void *), __alignof__(vector_type));
   const unsigned size = DIV_ROUND_UP(n, vector_width) * sizeof(vector_type);
   void *p;
   if (posix_memalign(&p, align, size))
         memset(p, 0, size);
                  /**
   * Set the (i, p)-th component of weight vector \p v to \p x.
   */
   void
   set(weight_vector_type &v, unsigned i, unsigned p, scalar_type x)
   {
                  /**
   * Get the (i, p)-th component of weight vector \p v.
   */
   scalar_type
   get(const weight_vector_type &v, unsigned i, unsigned p)
   {
                  /**
   * Swap the (i, p)-th and (j, q)-th components of weight vector \p v.
   */
   void
   swap(weight_vector_type &v,
      unsigned i, unsigned p,
      {
      const scalar_type tmp = get(v, i, p);
   set(v, i, p, get(v, j, q));
         }
      namespace {
      /**
   * Object that represents the partitioning of an arbitrary register space
   * into indivisible units (referred to as atoms below) that can potentially
   * be rearranged independently from other registers.  The partitioning is
   * inferred from a number of contiguity requirements specified using
   * require_contiguous().  This allows efficient look-up of the atom index a
   * given register address belongs to, or conversely the range of register
   * addresses that belong to a given atom.
   */
   struct partitioning {
      /**
   * Create a (for the moment unrestricted) partitioning of a register
   * file of size \p n.  The units are arbitrary.
   */
   partitioning(unsigned n) :
      max_reg(n),
   offsets(new unsigned[n + num_terminator_atoms]),
      {
      for (unsigned i = 0; i < n + num_terminator_atoms; i++) {
      offsets[i] = i;
                  partitioning(const partitioning &p) :
      max_reg(p.max_reg),
   offsets(new unsigned[p.num_atoms() + num_terminator_atoms]),
      {
      memcpy(offsets, p.offsets,
         memcpy(atoms, p.atoms,
               ~partitioning()
   {
      delete[] offsets;
               partitioning &
   operator=(partitioning p)
   {
      SWAP(max_reg, p.max_reg);
   SWAP(offsets, p.offsets);
   SWAP(atoms, p.atoms);
               /**
   * Require register range [reg, reg + n[ to be considered part of the
   * same atom.
   */
   void
   require_contiguous(unsigned reg, unsigned n)
   {
               /* Renumber atoms[reg...] = { r... } and their offsets[r...] for the
   * case that the specified contiguity requirement leads to the fusion
   * (yay) of one or more existing atoms.
   */
   for (unsigned reg1 = reg + 1; reg1 <= max_reg; reg1++) {
      if (offsets[atoms[reg1]] < reg + n) {
         } else {
                     offsets[r] = offsets[atoms[reg1]];
                     /**
   * Get the atom index register address \p reg belongs to.
   */
   unsigned
   atom_of_reg(unsigned reg) const
   {
                  /**
   * Get the base register address that belongs to atom \p r.
   */
   unsigned
   reg_of_atom(unsigned r) const
   {
                  /**
   * Get the size of atom \p r in register address units.
   */
   unsigned
   size_of_atom(unsigned r) const
   {
      assert(r < num_atoms());
               /**
   * Get the number of atoms the whole register space is partitioned into.
   */
   unsigned
   num_atoms() const
   {
               private:
      /**
   * Number of trailing atoms inserted for convenience so among other
   * things we don't need to special-case the last element in
   * size_of_atom().
   */
   static const unsigned num_terminator_atoms = 1;
   unsigned max_reg;
   unsigned *offsets;
               /**
   * Only GRF sources (whether they have been register-allocated or not) can
   * possibly incur bank conflicts.
   */
   bool
   is_grf(const fs_reg &r)
   {
                  /**
   * Register offset of \p r in GRF units.  Useful because the representation
   * of GRFs post-register allocation is somewhat inconsistent and depends on
   * whether the register already had a fixed GRF offset prior to register
   * allocation or whether it was part of a VGRF allocation.
   */
   unsigned
   reg_of(const fs_reg &r)
   {
      assert(is_grf(r));
   if (r.file == VGRF)
         else
               /**
   * Calculate the finest partitioning of the GRF space compatible with the
   * register contiguity requirements derived from all instructions part of
   * the program.
   */
   partitioning
   shader_reg_partitioning(const fs_visitor *v)
   {
               foreach_block_and_inst(block, fs_inst, inst, v->cfg) {
                     for (int i = 0; i < inst->sources; i++) {
      if (is_grf(inst->src[i]))
                              /**
   * Return the set of GRF atoms that should be left untouched at their
   * original location to avoid violating hardware or software assumptions.
   */
   bool *
   shader_reg_constraints(const fs_visitor *v, const partitioning &p)
   {
               /* These are read implicitly by some send-message instructions without
   * any indication at the IR level.  Assume they are unsafe to move
   * around.
   */
   for (unsigned reg = 0; reg < 2; reg++)
            /* At Intel Broadwell PRM, vol 07, section "Instruction Set Reference",
   * subsection "EUISA Instructions", Send Message (page 990):
   *
   * "r127 must not be used for return address when there is a src and
   * dest overlap in send instruction."
   *
   * Register allocation ensures that, so don't move 127 around to avoid
   * breaking that property.
   */
   if (v->devinfo->ver >= 8)
            foreach_block_and_inst(block, fs_inst, inst, v->cfg) {
      /* Assume that anything referenced via fixed GRFs is baked into the
   * hardware's fixed-function logic and may be unsafe to move around.
   * Also take into account the source GRF restrictions of EOT
   * send-message instructions.
   */
                  for (int i = 0; i < inst->sources; i++) {
      if (inst->src[i].file == FIXED_GRF ||
                     /* Preserve the original allocation of VGRFs used by the barycentric
   * source of the LINTERP instruction on Gfx6, since pair-aligned
   * barycentrics allow the PLN instruction to be used.
   */
   if (v->devinfo->has_pln && v->devinfo->ver <= 6 &&
                  /* The location of the Gfx7 MRF hack registers is hard-coded in the
   * rest of the compiler back-end.  Don't attempt to move them around.
   */
                     for (unsigned i = 0; i < inst->implied_mrf_writes(); i++) {
      const unsigned reg = GFX7_MRF_HACK_START + inst->base_mrf + i;
                                 /**
   * Return whether the hardware will be able to prevent a bank conflict by
   * optimizing out the read cycle of a source register.  The formula was
   * found experimentally.
   */
   bool
   is_conflict_optimized_out(const intel_device_info *devinfo,
         {
      return devinfo->ver >= 9 &&
      ((is_grf(inst->src[0]) && (reg_of(inst->src[0]) == reg_of(inst->src[1]) ||
                  /**
   * Return a matrix that allows reasonably efficient computation of the
   * cycle-count cost of bank conflicts incurred throughout the whole program
   * for any given atom-to-bank assignment.
   *
   * More precisely, if C_r_s_p is the result of this function, the total
   * cost of all bank conflicts involving any given atom r can be readily
   * recovered as follows:
   *
   *  S(B) = Sum_s_p(d_(p^B_r)_(B_s) * C_r_s_p)
   *
   * where d_i_j is the Kronecker delta, and B_r indicates the bank
   * assignment of r.  \sa delta_conflicts() for a vectorized implementation
   * of the expression above.
   *
   * FINISHME: Teach this about the Gfx10+ bank conflict rules, which are
   *           somewhat more relaxed than on previous generations.  In the
   *           meantime optimizing based on Gfx9 weights is likely to be more
   *           helpful than not optimizing at all.
   */
   weight_vector_type *
   shader_conflict_weight_matrix(const fs_visitor *v, const partitioning &p)
   {
      weight_vector_type *conflicts = new weight_vector_type[p.num_atoms()];
   for (unsigned r = 0; r < p.num_atoms(); r++)
            /* Crude approximation of the number of times the current basic block
   * will be executed at run-time.
   */
            foreach_block_and_inst(block, fs_inst, inst, v->cfg) {
                                    } else if (inst->is_3src(v->compiler) &&
                           /* Estimate of the cycle-count cost of incurring a bank conflict
   * for this instruction.  This is only true on the average, for a
   * sequence of back-to-back ternary instructions, since the EU
   * front-end only seems to be able to issue a new instruction at
   * an even cycle.  The cost of a bank conflict incurred by an
   * isolated ternary instruction may be higher.
   */
   const unsigned exec_size = inst->dst.component_size(inst->exec_size);
                  /* Neglect same-atom conflicts (since they're either trivial or
   * impossible to avoid without splitting the atom), and conflicts
   * known to be optimized out by the hardware.
   */
   if (r != s && !is_conflict_optimized_out(v->devinfo, inst)) {
      /* Calculate the parity of the sources relative to the start of
   * their respective atoms.  If their parity is the same (and
   * none of the atoms straddle the 2KB mark), the instruction
   * will incur a conflict iff both atoms are assigned the same
   * bank b.  If their parity is opposite, the instruction will
   * incur a conflict iff they are assigned opposite banks (b and
   * b^1).
   */
                        /* Calculate the updated cost of a hypothetical conflict
   * between atoms r and s.  Note that the weight matrix is
   * symmetric with respect to indices r and s by construction.
   */
   const scalar_type w = MIN2(unsigned(max_scalar),
         set(conflicts[r], s, p, w);
                                 /**
   * Return the set of GRF atoms that could potentially lead to bank
   * conflicts if laid out unfavorably in the GRF space according to
   * the specified \p conflicts matrix (\sa
   * shader_conflict_weight_matrix()).
   */
   bool *
   have_any_conflicts(const partitioning &p,
         {
               for (unsigned r = 0; r < p.num_atoms(); r++) {
      const unsigned m = DIV_ROUND_UP(conflicts[r].size, vector_width);
   for (unsigned s = 0; s < m; s++)
                           /**
   * Calculate the difference between two S(B) cost estimates as defined
   * above (\sa shader_conflict_weight_matrix()).  This represents the
   * (partial) cycle-count benefit from moving an atom r from bank p to n.
   * The respective bank assignments Bp and Bn are encoded as the \p
   * bank_mask_p and \p bank_mask_n bitmasks for efficient computation,
   * according to the formula:
   *
   *  bank_mask(B)_s_p = -d_(p^B_r)_(B_s)
   *
   * Notice the similarity with the delta function in the S(B) expression
   * above, and how bank_mask(B) can be precomputed for every possible
   * selection of r since bank_mask(B) only depends on it via B_r that may
   * only assume one of four different values, so the caller can keep every
   * possible bank_mask(B) vector in memory without much hassle (\sa
   * bank_characteristics()).
   */
   int
   delta_conflicts(const weight_vector_type &bank_mask_p,
               {
      const unsigned m = DIV_ROUND_UP(conflicts.size, vector_width);
            for (unsigned r = 0; r < m; r++) {
      s_p = adds(s_p, mask(bank_mask_p.v[r], conflicts.v[r]));
                           /**
   * Register atom permutation, represented as the start GRF offset each atom
   * is mapped into.
   */
   struct permutation {
               permutation(unsigned n) :
            permutation(const permutation &p) :
         {
                  ~permutation()
   {
                  permutation &
   operator=(permutation p)
   {
      SWAP(v, p.v);
   SWAP(size, p.size);
               unsigned *v;
               /**
   * Return an identity permutation of GRF atoms.
   */
   permutation
   identity_reg_permutation(const partitioning &p)
   {
               for (unsigned r = 0; r < map.size; r++)
                        /**
   * Return the bank index of GRF address \p reg, numbered according to the
   * table:
   *        Even Odd
   *    Lo    0   1
   *    Hi    2   3
   */
   unsigned
   bank_of(unsigned reg)
   {
                  /**
   * Return bitmasks suitable for use as bank mask arguments for the
   * delta_conflicts() computation.  Note that this is just the (negative)
   * characteristic function of each bank, if you regard it as a set
   * containing all atoms assigned to it according to the \p map array.
   */
   weight_vector_type *
   bank_characteristics(const permutation &map)
   {
               for (unsigned b = 0; b < 4; b++) {
               for (unsigned j = 0; j < map.size; j++) {
      for (unsigned p = 0; p < 2; p++)
      set(banks[b], j, p,
                           /**
   * Return an improved permutation of GRF atoms based on \p map attempting
   * to reduce the total cycle-count cost of bank conflicts greedily.
   *
   * Note that this doesn't attempt to merge multiple atoms into one, which
   * may allow it to do a better job in some cases -- It simply reorders
   * existing atoms in the GRF space without affecting their identity.
   */
   permutation
   optimize_reg_permutation(const partitioning &p,
                     {
      const bool *any_conflicts = have_any_conflicts(p, conflicts);
            for (unsigned r = 0; r < map.size; r++) {
               if (!constrained[r]) {
                                       if (bank_r != bank_s && !constrained[s] &&
      p.size_of_atom(r) == p.size_of_atom(s) &&
   (any_conflicts[r] || any_conflicts[s])) {
                        if (benefit > best_benefit) {
      best_s = s;
                     if (best_s != r) {
      for (unsigned b = 0; b < 4; b++) {
                                          delete[] banks;
   delete[] any_conflicts;
               /**
   * Apply the GRF atom permutation given by \p map to register \p r and
   * return the result.
   */
   fs_reg
   transform(const partitioning &p, const permutation &map, fs_reg r)
   {
      if (r.file == VGRF) {
      const unsigned reg = reg_of(r);
   const unsigned s = p.atom_of_reg(reg);
   r.nr = map.v[s] + reg - p.reg_of_atom(s);
                     }
      bool
   fs_visitor::opt_bank_conflicts()
   {
               /* No ternary instructions -- No bank conflicts. */
   if (devinfo->ver < 6)
            const partitioning p = shader_reg_partitioning(this);
   const bool *constrained = shader_reg_constraints(this, p);
   const weight_vector_type *conflicts =
         const permutation map =
      optimize_reg_permutation(p, constrained, conflicts,
         foreach_block_and_inst(block, fs_inst, inst, cfg) {
               for (int i = 0; i < inst->sources; i++)
               delete[] conflicts;
   delete[] constrained;
      }
      /**
   * Return whether the instruction incurs GRF bank conflict cycles.
   *
   * Note that this is only accurate after register allocation because otherwise
   * we don't know which bank each VGRF is going to end up aligned to.
   */
   bool
   has_bank_conflict(const struct brw_isa_info *isa, const fs_inst *inst)
   {
      return is_3src(isa, inst->opcode) &&
         is_grf(inst->src[1]) && is_grf(inst->src[2]) &&
      }
