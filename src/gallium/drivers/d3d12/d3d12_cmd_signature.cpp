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
      #include "d3d12_cmd_signature.h"
   #include "d3d12_compiler.h"
   #include "d3d12_screen.h"
      #include "util/u_memory.h"
      #include <dxguids/dxguids.h>
      struct d3d12_cmd_signature {
      struct d3d12_cmd_signature_key key;
      };
      static ID3D12CommandSignature *
   create_cmd_signature(struct d3d12_context *ctx, const struct d3d12_cmd_signature_key *key)
   {
      D3D12_COMMAND_SIGNATURE_DESC cmd_sig_desc = {};
            unsigned num_args = 0;
   if (key->draw_or_dispatch_params) {
      indirect_args[num_args].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
   indirect_args[num_args].Constant.RootParameterIndex = key->params_root_const_param;
   indirect_args[num_args].Constant.DestOffsetIn32BitValues = key->params_root_const_offset;
               indirect_args[num_args++].Type = key->compute ? D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH :
      key->indexed ? D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED :
      cmd_sig_desc.ByteStride = key->multi_draw_stride;
   cmd_sig_desc.NumArgumentDescs = num_args;
            ID3D12CommandSignature *ret = nullptr;
   d3d12_screen(ctx->base.screen)->dev->CreateCommandSignature(&cmd_sig_desc, key->root_sig,
            }
      ID3D12CommandSignature *
   d3d12_get_cmd_signature(struct d3d12_context *ctx,
         {
      struct hash_entry *entry = _mesa_hash_table_search(ctx->cmd_signature_cache, &key);
      if (!entry) {
      struct d3d12_cmd_signature *data =
         if (!data)
            memcpy(&data->key, key, sizeof(*key));
   data->sig = create_cmd_signature(ctx, key);
   if (!data->sig) {
      FREE(data);
               entry = _mesa_hash_table_insert(ctx->cmd_signature_cache, &data->key, data);
                  }
      static uint32_t
   hash_cmd_signature_key(const void *key)
   {
         }
      static bool
   equals_cmd_signature_key(const void *a, const void *b)
   {
         }
      void
   d3d12_cmd_signature_cache_init(struct d3d12_context *ctx)
   {
      ctx->cmd_signature_cache = _mesa_hash_table_create(NULL,
            }
      static void
   delete_entry(struct hash_entry *entry)
   {
      struct d3d12_cmd_signature *data = (struct d3d12_cmd_signature *)entry->data;
   data->sig->Release();
      }
      void
   d3d12_cmd_signature_cache_destroy(struct d3d12_context *ctx)
   {
         }
