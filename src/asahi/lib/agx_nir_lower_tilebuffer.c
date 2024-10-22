   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/agx_internal_formats.h"
   #include "compiler/glsl_types.h"
   #include "util/macros.h"
   #include "agx_nir_format_helpers.h"
   #include "agx_pack.h"
   #include "agx_tilebuffer.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_builder_opcodes.h"
      #define AGX_NUM_TEXTURE_STATE_REGS 16
   #define ALL_SAMPLES                0xFF
      struct ctx {
      struct agx_tilebuffer_layout *tib;
   uint8_t *colormasks;
   bool *translucent;
   unsigned bindless_base;
   bool any_memory_stores;
   bool layer_id_sr;
      };
      static bool
   tib_filter(const nir_instr *instr, UNUSED const void *_)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_output &&
      intr->intrinsic != nir_intrinsic_load_output)
         nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   assert(sem.dual_source_blend_index == 0 && "dual source blending lowered");
      }
      static void
   store_tilebuffer(nir_builder *b, struct agx_tilebuffer_layout *tib,
               {
      /* The hardware cannot extend for a 32-bit format. Extend ourselves. */
   if (format == PIPE_FORMAT_R32_UINT && value->bit_size == 16) {
      if (util_format_is_pure_sint(logical_format))
         else if (util_format_is_pure_uint(logical_format))
         else
               uint8_t offset_B = agx_tilebuffer_offset_B(tib, rt);
   nir_store_local_pixel_agx(b, value, nir_imm_intN_t(b, ALL_SAMPLES, 16),
            }
      static nir_def *
   load_tilebuffer(nir_builder *b, struct agx_tilebuffer_layout *tib,
               {
      unsigned comps = util_format_get_nr_components(logical_format);
            /* Don't load with F16 */
   if (f16)
            uint8_t offset_B = agx_tilebuffer_offset_B(tib, rt);
   nir_def *res = nir_load_local_pixel_agx(
      b, MIN2(load_comps, comps), f16 ? 16 : bit_size,
         /* Extend floats */
   if (f16 && bit_size != 16) {
      assert(bit_size == 32);
               res = nir_sign_extend_if_sint(b, res, logical_format);
      }
      /*
   * As a simple implementation, we use image load/store instructions to access
   * spilled render targets. The driver will supply corresponding texture and PBE
   * descriptors for each render target, accessed bindlessly
   *
   * Note that this lower happens after driver bindings are lowered, so the
   * bindless handle is in the AGX-specific format.
   *
   * Assumes that texture states are mapped to a bindless table is in u0_u1 and
   * texture/PBE descriptors are alternated for each render target. This is
   * ABI. If we need to make this more flexible for Vulkan later, we can.
   */
   static nir_def *
   handle_for_rt(nir_builder *b, unsigned base, unsigned rt, bool pbe,
         {
      unsigned index = base + (2 * rt) + (pbe ? 1 : 0);
            if (*bindless) {
      unsigned table = 0 * 2;
   unsigned offset_B = index * AGX_TEXTURE_LENGTH;
      } else {
            }
      static enum glsl_sampler_dim
   dim_for_rt(nir_builder *b, unsigned nr_samples, nir_def **sample)
   {
      if (nr_samples == 1) {
      *sample = nir_imm_intN_t(b, 0, 16);
      } else {
      *sample = nir_load_sample_id(b);
   b->shader->info.fs.uses_sample_shading = true;
         }
      static nir_def *
   image_coords(nir_builder *b, nir_def *layer_id)
   {
      nir_def *xy = nir_u2u32(b, nir_load_pixel_coord(b));
            if (layer_id)
               }
      static void
   store_memory(nir_builder *b, unsigned bindless_base, unsigned nr_samples,
               {
      /* Force bindless for multisampled image writes since they will be lowered
   * with a descriptor crawl later.
   */
   bool bindless = (nr_samples > 1);
   nir_def *image = handle_for_rt(b, bindless_base, rt, true, &bindless);
   nir_def *zero = nir_imm_intN_t(b, 0, 16);
            nir_def *sample;
   enum glsl_sampler_dim dim = dim_for_rt(b, nr_samples, &sample);
                     if (nr_samples > 1) {
      nir_def *coverage = nir_load_sample_mask(b);
   nir_def *covered = nir_ubitfield_extract(
                        if (bindless) {
      nir_bindless_image_store(b, image, coords, sample, value, lod,
            } else {
      nir_image_store(b, image, coords, sample, value, lod, .image_dim = dim,
               if (nr_samples > 1)
               }
      static nir_def *
   load_memory(nir_builder *b, unsigned bindless_base, unsigned nr_samples,
               {
      bool bindless = false;
   nir_def *image = handle_for_rt(b, bindless_base, rt, false, &bindless);
   nir_def *zero = nir_imm_intN_t(b, 0, 16);
            nir_def *sample;
   enum glsl_sampler_dim dim = dim_for_rt(b, nr_samples, &sample);
            /* Ensure pixels below this one have written out their results */
            if (bindless) {
      return nir_bindless_image_load(
      b, comps, bit_size, image, coords, sample, lod, .image_dim = dim,
   } else {
      return nir_image_load(b, comps, bit_size, image, coords, sample, lod,
               }
      nir_def *
   agx_internal_layer_id(nir_builder *b)
   {
      /* In the background and end-of-tile programs, the layer ID is available as
   * sr2, the Z component of the workgroup index.
   */
      }
      static nir_def *
   tib_layer_id(nir_builder *b, struct ctx *ctx)
   {
      if (!ctx->tib->layered) {
      /* If we're not layered, there's no explicit layer ID */
      } else if (ctx->layer_id_sr) {
         } else {
      /* Otherwise, the layer ID is loaded as a flat varying. */
            return nir_load_input(b, 1, 32, nir_imm_int(b, 0),
         }
      static nir_def *
   tib_impl(nir_builder *b, nir_instr *instr, void *data)
   {
      struct ctx *ctx = data;
   struct agx_tilebuffer_layout *tib = ctx->tib;
            nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   unsigned rt = sem.location - FRAG_RESULT_DATA0;
            enum pipe_format logical_format = tib->logical_format[rt];
   enum pipe_format format = agx_tilebuffer_physical_format(tib, rt);
            if (intr->intrinsic == nir_intrinsic_store_output) {
               /* Only write components that actually exist */
            /* Delete stores to nonexistent render targets */
   if (logical_format == PIPE_FORMAT_NONE)
            /* Only write colours masked by the blend state */
   if (ctx->colormasks)
            /* Masked stores require a translucent pass type */
   if (write_mask != BITFIELD_MASK(comps)) {
                     assert(agx_tilebuffer_supports_mask(tib, rt));
               /* But we ignore the NIR write mask for that, since it's basically an
   * optimization hint.
   */
   if (agx_tilebuffer_supports_mask(tib, rt))
            /* Delete stores that are entirely masked out */
   if (!write_mask)
                     /* Trim to format as required by hardware */
            if (tib->spilled[rt]) {
      store_memory(b, ctx->bindless_base, tib->nr_samples,
            } else {
      store_tilebuffer(b, tib, format, logical_format, rt, value,
                  } else {
               /* Loads from non-existent render targets are undefined in NIR but not
   * possible to encode in the hardware, delete them.
   */
   if (logical_format == PIPE_FORMAT_NONE) {
         } else if (tib->spilled[rt]) {
               return load_memory(b, ctx->bindless_base, tib->nr_samples,
            } else {
      return load_tilebuffer(b, tib, intr->num_components, bit_size, rt,
            }
      bool
   agx_nir_lower_tilebuffer(nir_shader *shader, struct agx_tilebuffer_layout *tib,
               {
               struct ctx ctx = {
      .tib = tib,
   .colormasks = colormasks,
   .translucent = translucent,
               /* Allocate 1 texture + 1 PBE descriptor for each spilled descriptor */
   if (agx_tilebuffer_spills(tib)) {
      assert(bindless_base != NULL && "must be specified if spilling");
   ctx.bindless_base = *bindless_base;
               bool progress =
            /* Flush at end */
   if (ctx.any_memory_stores) {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_builder b = nir_builder_at(nir_after_impl(impl));
               /* If there are any render targets bound to the framebuffer that aren't
   * statically written by the fragment shader, that acts as an implicit mask
   * and requires translucency.
   *
   * XXX: Could be optimized.
   */
   for (unsigned i = 0; i < ARRAY_SIZE(tib->logical_format); ++i) {
      bool exists = tib->logical_format[i] != PIPE_FORMAT_NONE;
            if (translucent)
                  }
