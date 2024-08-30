   /**********************************************************
   * Copyright 2009-2023 VMware, Inc.  All rights reserved.
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
         #include "svga_cmd.h"
      #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_debug_stack.h"
   #include "util/u_debug_flush.h"
   #include "util/u_hash_table.h"
   #include "pipebuffer/pb_buffer.h"
   #include "pipebuffer/pb_validate.h"
      #include "svga_winsys.h"
   #include "vmw_context.h"
   #include "vmw_screen.h"
   #include "vmw_buffer.h"
   #include "vmw_surface.h"
   #include "vmw_fence.h"
   #include "vmw_shader.h"
   #include "vmw_query.h"
      #define VMW_COMMAND_SIZE (64*1024)
   #define VMW_SURFACE_RELOCS (1024)
   #define VMW_SHADER_RELOCS (1024)
   #define VMW_REGION_RELOCS (512)
      #define VMW_MUST_FLUSH_STACK 8
      /*
   * A factor applied to the maximum mob memory size to determine
   * the optimial time to preemptively flush the command buffer.
   * The constant is based on some performance trials with SpecViewperf.
   */
   #define VMW_MAX_MOB_MEM_FACTOR  2
      /*
   * A factor applied to the maximum surface memory size to determine
   * the optimial time to preemptively flush the command buffer.
   * The constant is based on some performance trials with SpecViewperf.
   */
   #define VMW_MAX_SURF_MEM_FACTOR 2
            struct vmw_buffer_relocation
   {
      struct pb_buffer *buffer;
   bool is_mob;
            union {
      struct SVGAGuestPtr *where;
         } region;
   SVGAMobId *id;
   uint32 *offset_into_mob;
               };
      struct vmw_ctx_validate_item {
      union {
      struct vmw_svga_winsys_surface *vsurf;
      };
      };
      struct vmw_svga_winsys_context
   {
               struct vmw_winsys_screen *vws;
         #ifdef DEBUG
      bool must_flush;
   struct debug_stack_frame must_flush_stack[VMW_MUST_FLUSH_STACK];
      #endif
         struct {
      uint8_t buffer[VMW_COMMAND_SIZE];
   uint32_t size;
   uint32_t used;
               struct {
      struct vmw_ctx_validate_item items[VMW_SURFACE_RELOCS];
   uint32_t size;
   uint32_t used;
   uint32_t staged;
      } surface;
      struct {
      struct vmw_buffer_relocation relocs[VMW_REGION_RELOCS];
   uint32_t size;
   uint32_t used;
   uint32_t staged;
               struct {
      struct vmw_ctx_validate_item items[VMW_SHADER_RELOCS];
   uint32_t size;
   uint32_t used;
   uint32_t staged;
                        /**
   * The amount of surface, GMR or MOB memory that is referred by the commands
   * currently batched in the context command buffer.
   */
   uint64_t seen_surfaces;
   uint64_t seen_regions;
            /**
   * Whether this context should fail to reserve more commands, not because it
   * ran out of command space, but because a substantial ammount of GMR was
   * referred.
   */
      };
         static inline struct vmw_svga_winsys_context *
   vmw_svga_winsys_context(struct svga_winsys_context *swc)
   {
      assert(swc);
      }
         static inline enum pb_usage_flags
   vmw_translate_to_pb_flags(unsigned flags)
   {
      enum pb_usage_flags f = 0;
   if (flags & SVGA_RELOC_READ)
            if (flags & SVGA_RELOC_WRITE)
               }
      static enum pipe_error
   vmw_swc_flush(struct svga_winsys_context *swc,
         {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_winsys_screen *vws = vswc->vws;
   struct pipe_fence_handle *fence = NULL;
   unsigned i;
            /*
   * If we hit a retry, lock the mutex and retry immediately.
   * If we then still hit a retry, sleep until another thread
   * wakes us up after it has released its buffers from the
   * validate list.
   *
   * If we hit another error condition, we still need to broadcast since
   * pb_validate_validate releases validated buffers in its error path.
            ret = pb_validate_validate(vswc->validate);
   if (ret != PIPE_OK) {
      mtx_lock(&vws->cs_mutex);
   while (ret == PIPE_ERROR_RETRY) {
      ret = pb_validate_validate(vswc->validate);
   if (ret == PIPE_ERROR_RETRY) {
            }
   if (ret != PIPE_OK) {
         }
               assert(ret == PIPE_OK);
   if(ret == PIPE_OK) {
         /* Apply relocations */
   for(i = 0; i < vswc->region.used; ++i) {
                                    if (reloc->is_mob) {
      if (reloc->mob.id)
         if (reloc->mob.offset_into_mob)
         else {
            } else
      *reloc->region.where = ptr;
               if (vswc->command.used || pfence != NULL)
      vmw_ioctl_command(vws,
                     vswc->base.cid,
   0,
               pb_validate_fence(vswc->validate, fence);
   mtx_lock(&vws->cs_mutex);
   cnd_broadcast(&vws->cs_cond);
               vswc->command.used = 0;
            for(i = 0; i < vswc->surface.used + vswc->surface.staged; ++i) {
      struct vmw_ctx_validate_item *isurf = &vswc->surface.items[i];
   if (isurf->referenced)
                     _mesa_hash_table_clear(vswc->hash, NULL);
   vswc->surface.used = 0;
            for(i = 0; i < vswc->shader.used + vswc->shader.staged; ++i) {
      struct vmw_ctx_validate_item *ishader = &vswc->shader.items[i];
   if (ishader->referenced)
                     vswc->shader.used = 0;
            vswc->region.used = 0;
         #ifdef DEBUG
      vswc->must_flush = false;
      #endif
      swc->hints &= ~SVGA_HINT_FLAG_CAN_PRE_FLUSH;
   swc->hints &= ~SVGA_HINT_FLAG_EXPORT_FENCE_FD;
   vswc->preemptive_flush = false;
   vswc->seen_surfaces = 0;
   vswc->seen_regions = 0;
            if (vswc->base.imported_fence_fd != -1) {
      close(vswc->base.imported_fence_fd);
               if(pfence)
                        }
         static void *
   vmw_swc_reserve(struct svga_winsys_context *swc,
         {
            #ifdef DEBUG
      /* Check if somebody forgot to check the previous failure */
   if(vswc->must_flush) {
      debug_printf("Forgot to flush:\n");
   debug_backtrace_dump(vswc->must_flush_stack, VMW_MUST_FLUSH_STACK);
      }
      #endif
         assert(nr_bytes <= vswc->command.size);
   if(nr_bytes > vswc->command.size)
            if(vswc->preemptive_flush ||
      vswc->command.used + nr_bytes > vswc->command.size ||
   vswc->surface.used + nr_relocs > vswc->surface.size ||
   vswc->shader.used + nr_relocs > vswc->shader.size ||
   #ifdef DEBUG
         vswc->must_flush = true;
   debug_backtrace_capture(vswc->must_flush_stack, 1,
   #endif
                     assert(vswc->command.used + nr_bytes <= vswc->command.size);
   assert(vswc->surface.used + nr_relocs <= vswc->surface.size);
   assert(vswc->shader.used + nr_relocs <= vswc->shader.size);
   assert(vswc->region.used + nr_relocs <= vswc->region.size);
      vswc->command.reserved = nr_bytes;
   vswc->surface.reserved = nr_relocs;
   vswc->surface.staged = 0;
   vswc->shader.reserved = nr_relocs;
   vswc->shader.staged = 0;
   vswc->region.reserved = nr_relocs;
   vswc->region.staged = 0;
         }
      static unsigned
   vmw_swc_get_command_buffer_size(struct svga_winsys_context *swc)
   {
      const struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
      }
      static void
   vmw_swc_context_relocation(struct svga_winsys_context *swc,
         {
         }
      static bool
   vmw_swc_add_validate_buffer(struct vmw_svga_winsys_context *vswc,
         struct pb_buffer *pb_buf,
   {
      ASSERTED enum pipe_error ret;
   unsigned translated_flags;
            translated_flags = vmw_translate_to_pb_flags(flags);
   ret = pb_validate_add_buffer(vswc->validate, pb_buf, translated_flags,
         assert(ret == PIPE_OK);
      }
      static void
   vmw_swc_region_relocation(struct svga_winsys_context *swc,
                           {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
                     reloc = &vswc->region.relocs[vswc->region.used + vswc->region.staged];
            /*
   * pb_validate holds a refcount to the buffer, so no need to
   * refcount it again in the relocation.
   */
   reloc->buffer = vmw_pb_buffer(buffer);
   reloc->offset = offset;
   reloc->is_mob = false;
            if (vmw_swc_add_validate_buffer(vswc, reloc->buffer, flags)) {
      vswc->seen_regions += reloc->buffer->size;
   if ((swc->hints & SVGA_HINT_FLAG_CAN_PRE_FLUSH) &&
      vswc->seen_regions >= VMW_GMR_POOL_SIZE/5)
         #ifdef DEBUG
      if (!(flags & SVGA_RELOC_INTERNAL))
      #endif
   }
      static void
   vmw_swc_mob_relocation(struct svga_winsys_context *swc,
            SVGAMobId *id,
   uint32 *offset_into_mob,
   struct svga_winsys_buffer *buffer,
      {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_buffer_relocation *reloc;
            if (id) {
               reloc = &vswc->region.relocs[vswc->region.used + vswc->region.staged];
   reloc->mob.id = id;
            /*
   * pb_validate holds a refcount to the buffer, so no need to
   * refcount it again in the relocation.
   */
   reloc->buffer = pb_buffer;
   reloc->offset = offset;
   reloc->is_mob = true;
               if (vmw_swc_add_validate_buffer(vswc, pb_buffer, flags)) {
               if ((swc->hints & SVGA_HINT_FLAG_CAN_PRE_FLUSH) &&
      vswc->seen_mobs >=
               #ifdef DEBUG
      if (!(flags & SVGA_RELOC_INTERNAL))
      #endif
   }
         /**
   * vmw_swc_surface_clear_reference - Clear referenced info for a surface
   *
   * @swc:   Pointer to an svga_winsys_context
   * @vsurf: Pointer to a vmw_svga_winsys_surface, the referenced info of which
   *         we want to clear
   *
   * This is primarily used by a discard surface map to indicate that the
   * surface data is no longer referenced by a draw call, and mapping it
   * should therefore no longer cause a flush.
   */
   void
   vmw_swc_surface_clear_reference(struct svga_winsys_context *swc,
         {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_ctx_validate_item *isrf =
            if (isrf && isrf->referenced) {
      isrf->referenced = false;
         }
      static void
   vmw_swc_surface_only_relocation(struct svga_winsys_context *swc,
      uint32 *where,
   struct vmw_svga_winsys_surface *vsurf,
      {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
            assert(vswc->surface.staged < vswc->surface.reserved);
            if (isrf == NULL) {
      isrf = &vswc->surface.items[vswc->surface.used + vswc->surface.staged];
   vmw_svga_winsys_surface_reference(&isrf->vsurf, vsurf);
            _mesa_hash_table_insert(vswc->hash, vsurf, isrf);
            vswc->seen_surfaces += vsurf->size;
   if ((swc->hints & SVGA_HINT_FLAG_CAN_PRE_FLUSH) &&
      vswc->seen_surfaces >=
                  if (!(flags & SVGA_RELOC_INTERNAL) && !isrf->referenced) {
      isrf->referenced = true;
               if (where)
      }
      static void
   vmw_swc_surface_relocation(struct svga_winsys_context *swc,
                           {
                        if (!surface) {
      *where = SVGA3D_INVALID_ID;
   if (mobid)
                     vsurf = vmw_svga_winsys_surface(surface);
                        /*
   * Make sure backup buffer ends up fenced.
            mtx_lock(&vsurf->mutex);
            /*
   * An internal reloc means that the surface transfer direction
   * is opposite to the MOB transfer direction...
   */
   if ((flags & SVGA_RELOC_INTERNAL) &&
      (flags & (SVGA_RELOC_READ | SVGA_RELOC_WRITE)) !=
   (SVGA_RELOC_READ | SVGA_RELOC_WRITE))
      vmw_swc_mob_relocation(swc, mobid, NULL, (struct svga_winsys_buffer *)
               }
      static void
   vmw_swc_shader_relocation(struct svga_winsys_context *swc,
      uint32 *shid,
   uint32 *mobid,
   uint32 *offset,
   struct svga_winsys_gb_shader *shader,
      {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_winsys_screen *vws = vswc->vws;
   struct vmw_svga_winsys_shader *vshader;
            if(!shader) {
      *shid = SVGA3D_INVALID_ID;
                        if (!vws->base.have_vgpu10) {
      assert(vswc->shader.staged < vswc->shader.reserved);
            if (ishader == NULL) {
      ishader = &vswc->shader.items[vswc->shader.used + vswc->shader.staged];
                  _mesa_hash_table_insert(vswc->hash, vshader, ishader);
               if (!ishader->referenced) {
      ishader->referenced = true;
                  if (shid)
            if (vshader->buf)
      vmw_swc_mob_relocation(swc, mobid, offset, vshader->buf,
   }
      static void
   vmw_swc_query_relocation(struct svga_winsys_context *swc,
               {
      /* Queries are backed by one big MOB */
   vmw_swc_mob_relocation(swc, id, NULL, query->buf, 0,
      }
      static void
   vmw_swc_commit(struct svga_winsys_context *swc)
   {
               assert(vswc->command.used + vswc->command.reserved <= vswc->command.size);
   vswc->command.used += vswc->command.reserved;
            assert(vswc->surface.staged <= vswc->surface.reserved);
   assert(vswc->surface.used + vswc->surface.staged <= vswc->surface.size);
   vswc->surface.used += vswc->surface.staged;
   vswc->surface.staged = 0;
            assert(vswc->shader.staged <= vswc->shader.reserved);
   assert(vswc->shader.used + vswc->shader.staged <= vswc->shader.size);
   vswc->shader.used += vswc->shader.staged;
   vswc->shader.staged = 0;
            assert(vswc->region.staged <= vswc->region.reserved);
   assert(vswc->region.used + vswc->region.staged <= vswc->region.size);
   vswc->region.used += vswc->region.staged;
   vswc->region.staged = 0;
      }
         static void
   vmw_swc_destroy(struct svga_winsys_context *swc)
   {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
            for(i = 0; i < vswc->surface.used; ++i) {
      struct vmw_ctx_validate_item *isurf = &vswc->surface.items[i];
   if (isurf->referenced)
                     for(i = 0; i < vswc->shader.used; ++i) {
      struct vmw_ctx_validate_item *ishader = &vswc->shader.items[i];
   if (ishader->referenced)
                     _mesa_hash_table_destroy(vswc->hash, NULL);
   pb_validate_destroy(vswc->validate);
      #ifdef DEBUG
         #endif
         }
      /**
   * vmw_svga_winsys_vgpu10_shader_screate - The winsys shader_crate callback
   *
   * @swc: The winsys context.
   * @shaderId: Previously allocated shader id.
   * @shaderType: The shader type.
   * @bytecode: The shader bytecode
   * @bytecodelen: The length of the bytecode.
   *
   * Creates an svga_winsys_gb_shader structure and allocates a buffer for the
   * shader code and copies the shader code into the buffer. Shader
   * resource creation is not done.
   */
   static struct svga_winsys_gb_shader *
   vmw_svga_winsys_vgpu10_shader_create(struct svga_winsys_context *swc,
                                       {
      struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_svga_winsys_shader *shader;
   shader = vmw_svga_shader_create(&vswc->vws->base, shaderType, bytecode,
         if (!shader)
            shader->shid = shaderId;
      }
      /**
   * vmw_svga_winsys_vgpu10_shader_destroy - The winsys shader_destroy callback.
   *
   * @swc: The winsys context.
   * @shader: A shader structure previously allocated by shader_create.
   *
   * Frees the shader structure and the buffer holding the shader code.
   */
   static void
   vmw_svga_winsys_vgpu10_shader_destroy(struct svga_winsys_context *swc,
         {
                  }
      /**
   * vmw_svga_winsys_resource_rebind - The winsys resource_rebind callback
   *
   * @swc: The winsys context.
   * @surface: The surface to be referenced.
   * @shader: The shader to be referenced.
   * @flags: Relocation flags.
   *
   * This callback is needed because shader backing buffers are sub-allocated, and
   * hence the kernel fencing is not sufficient. The buffers need to be put on
   * the context's validation list and fenced after command submission to avoid
   * reuse of busy shader buffers. In addition, surfaces need to be put on the
   * validation list in order for the driver to regard them as referenced
   * by the command stream.
   */
   static enum pipe_error
   vmw_svga_winsys_resource_rebind(struct svga_winsys_context *swc,
                     {
      /**
   * Need to reserve one validation item for either the surface or
   * the shader.
   */
   if (!vmw_swc_reserve(swc, 0, 1))
            if (surface)
         else if (shader)
                        }
      struct svga_winsys_context *
   vmw_svga_winsys_context_create(struct svga_winsys_screen *sws)
   {
      struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
            vswc = CALLOC_STRUCT(vmw_svga_winsys_context);
   if(!vswc)
            vswc->base.destroy = vmw_swc_destroy;
   vswc->base.reserve = vmw_swc_reserve;
   vswc->base.get_command_buffer_size = vmw_swc_get_command_buffer_size;
   vswc->base.surface_relocation = vmw_swc_surface_relocation;
   vswc->base.region_relocation = vmw_swc_region_relocation;
   vswc->base.mob_relocation = vmw_swc_mob_relocation;
   vswc->base.query_relocation = vmw_swc_query_relocation;
   vswc->base.query_bind = vmw_swc_query_bind;
   vswc->base.context_relocation = vmw_swc_context_relocation;
   vswc->base.shader_relocation = vmw_swc_shader_relocation;
   vswc->base.commit = vmw_swc_commit;
   vswc->base.flush = vmw_swc_flush;
   vswc->base.surface_map = vmw_svga_winsys_surface_map;
            vswc->base.shader_create = vmw_svga_winsys_vgpu10_shader_create;
                     if (sws->have_vgpu10)
         else
            if (vswc->base.cid == -1)
                                       vswc->command.size = VMW_COMMAND_SIZE;
   vswc->surface.size = VMW_SURFACE_RELOCS;
   vswc->shader.size = VMW_SHADER_RELOCS;
            vswc->validate = pb_validate_create();
   if(!vswc->validate)
            vswc->hash = util_hash_table_create_ptr_keys();
   if (!vswc->hash)
         #ifdef DEBUG
         #endif
         vswc->base.force_coherent = vws->force_coherent;
         out_no_hash:
         out_no_validate:
         out_no_context:
      FREE(vswc);
      }
