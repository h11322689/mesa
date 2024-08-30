   /*
   * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "nine_helpers.h"
   #include "nine_shader.h"
      #include "pixelshader9.h"
      #include "device9.h"
   #include "pipe/p_context.h"
      #define DBG_CHANNEL DBG_PIXELSHADER
      HRESULT
   NinePixelShader9_ctor( struct NinePixelShader9 *This,
               {
      struct NineDevice9 *device;
   struct nine_shader_info info;
   struct pipe_context *pipe;
                     hr = NineUnknown_ctor(&This->base, pParams);
   if (FAILED(hr))
            if (cso) {
      This->ff_cso = cso;
      }
            info.type = PIPE_SHADER_FRAGMENT;
   info.byte_code = pFunction;
   info.const_i_base = NINE_CONST_I_BASE(NINE_MAX_CONST_F_PS3) / 16;
   info.const_b_base = NINE_CONST_B_BASE(NINE_MAX_CONST_F_PS3) / 16;
   info.sampler_mask_shadow = 0x0;
   info.fetch4 = 0x0;
   info.force_color_in_centroid = 0;
   info.sampler_ps1xtypes = 0x0;
   info.fog_enable = 0;
   info.projected = 0;
   info.alpha_test_emulation = 0;
   info.color_flatshade = 0;
   info.add_constants_defs.c_combination = NULL;
   info.add_constants_defs.int_const_added = NULL;
   info.add_constants_defs.bool_const_added = NULL;
   info.process_vertices = false;
            pipe = nine_context_get_pipe_acquire(device);
   hr = nine_translate_shader(device, &info, pipe);
   nine_context_get_pipe_release(device);
   if (FAILED(hr))
                  This->byte_code.tokens = mem_dup(pFunction, info.byte_size);
   if (!This->byte_code.tokens)
                  This->variant.cso = info.cso;
   This->variant.const_ranges = info.const_ranges;
   This->variant.const_used_size = info.const_used_size;
   This->last_cso = info.cso;
   This->last_const_ranges = info.const_ranges;
   This->last_const_used_size = info.const_used_size;
            This->sampler_mask = info.sampler_mask;
   This->rt_mask = info.rt_mask;
            memcpy(This->int_slots_used, info.int_slots_used, sizeof(This->int_slots_used));
            This->const_int_slots = info.const_int_slots;
                     /* no constant relative addressing for ps */
   assert(info.lconstf.data == NULL);
               }
      void
   NinePixelShader9_dtor( struct NinePixelShader9 *This )
   {
               if (This->base.device) {
      struct pipe_context *pipe = nine_context_get_pipe_multithread(This->base.device);
            do {
         if (var->cso) {
      if (This->base.device->context.cso_shader.ps == var->cso) {
      /* unbind because it is illegal to delete something bound */
   pipe->bind_fs_state(pipe, NULL);
   /* This will rebind cso_shader.ps in case somehow actually
            }
   pipe->delete_fs_state(pipe, var->cso);
      }
            if (This->ff_cso) {
         if (This->ff_cso == This->base.device->context.cso_shader.ps) {
      pipe->bind_fs_state(pipe, NULL);
      }
      }
                                 }
      HRESULT NINE_WINAPI
   NinePixelShader9_GetFunction( struct NinePixelShader9 *This,
               {
                        if (!pData) {
      *pSizeOfData = This->byte_code.size;
      }
                        }
      void *
   NinePixelShader9_GetVariant( struct NinePixelShader9 *This,
               {
      /* GetVariant is called from nine_context, thus we can
   * get pipe directly */
   struct pipe_context *pipe = This->base.device->context.pipe;
   void *cso;
            key = This->next_key;
   if (key == This->last_key) {
      *const_ranges = This->last_const_ranges;
   *const_used_size = This->last_const_used_size;
               cso = nine_shader_variant_get(&This->variant, const_ranges, const_used_size, key);
   if (!cso) {
      struct NineDevice9 *device = This->base.device;
   struct nine_shader_info info;
            info.type = PIPE_SHADER_FRAGMENT;
   info.const_i_base = NINE_CONST_I_BASE(NINE_MAX_CONST_F_PS3) / 16;
   info.const_b_base = NINE_CONST_B_BASE(NINE_MAX_CONST_F_PS3) / 16;
   info.byte_code = This->byte_code.tokens;
   info.sampler_mask_shadow = key & 0xffff;
   /* intended overlap with sampler_mask_shadow */
   if (unlikely(This->byte_code.version < 0x20)) {
         if (This->byte_code.version < 0x14) {
      info.sampler_ps1xtypes = (key >> 4) & 0xff;
      } else {
      info.sampler_ps1xtypes = (key >> 6) & 0xfff;
      } else {
         info.sampler_ps1xtypes = 0;
   }
   info.fog_enable = device->context.rs[D3DRS_FOGENABLE];
   info.fog_mode = device->context.rs[D3DRS_FOGTABLEMODE];
   info.zfog = device->context.zfog;
   info.add_constants_defs.c_combination =
         info.add_constants_defs.int_const_added = &This->int_slots_used;
   info.add_constants_defs.bool_const_added = &This->bool_slots_used;
   info.fetch4 = (key >> 32) & 0xffff;
   info.force_color_in_centroid = (key >> 48) & 1;
   info.alpha_test_emulation = (key >> 49) & 0x7;
   info.color_flatshade = (key >> 52) & 1;
   info.force_color_in_centroid &= !info.color_flatshade; /* centroid doesn't make sense with flatshade */
   info.process_vertices = false;
            hr = nine_translate_shader(This->base.device, &info, pipe);
   if (FAILED(hr))
         nine_shader_variant_add(&This->variant, key, info.cso,
         cso = info.cso;
   *const_ranges = info.const_ranges;
               This->last_key = key;
   This->last_cso = cso;
   This->last_const_ranges = *const_ranges;
               }
      IDirect3DPixelShader9Vtbl NinePixelShader9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice,
      };
      static const GUID *NinePixelShader9_IIDs[] = {
      &IID_IDirect3DPixelShader9,
   &IID_IUnknown,
      };
      HRESULT
   NinePixelShader9_new( struct NineDevice9 *pDevice,
               {
      if (cso) { /* ff shader. Needs to start with bind count */
         } else {
            }
