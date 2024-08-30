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
      #include "vertexdeclaration9.h"
   #include "vertexbuffer9.h"
   #include "device9.h"
   #include "nine_helpers.h"
   #include "nine_shader.h"
      #include "util/format/u_formats.h"
   #include "pipe/p_context.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "translate/translate.h"
      #define DBG_CHANNEL DBG_VERTEXDECLARATION
      static inline enum pipe_format decltype_format(BYTE type)
   {
      switch (type) {
   case D3DDECLTYPE_FLOAT1:    return PIPE_FORMAT_R32_FLOAT;
   case D3DDECLTYPE_FLOAT2:    return PIPE_FORMAT_R32G32_FLOAT;
   case D3DDECLTYPE_FLOAT3:    return PIPE_FORMAT_R32G32B32_FLOAT;
   case D3DDECLTYPE_FLOAT4:    return PIPE_FORMAT_R32G32B32A32_FLOAT;
   case D3DDECLTYPE_D3DCOLOR:  return PIPE_FORMAT_B8G8R8A8_UNORM;
   case D3DDECLTYPE_UBYTE4:    return PIPE_FORMAT_R8G8B8A8_USCALED;
   case D3DDECLTYPE_SHORT2:    return PIPE_FORMAT_R16G16_SSCALED;
   case D3DDECLTYPE_SHORT4:    return PIPE_FORMAT_R16G16B16A16_SSCALED;
   case D3DDECLTYPE_UBYTE4N:   return PIPE_FORMAT_R8G8B8A8_UNORM;
   case D3DDECLTYPE_SHORT2N:   return PIPE_FORMAT_R16G16_SNORM;
   case D3DDECLTYPE_SHORT4N:   return PIPE_FORMAT_R16G16B16A16_SNORM;
   case D3DDECLTYPE_USHORT2N:  return PIPE_FORMAT_R16G16_UNORM;
   case D3DDECLTYPE_USHORT4N:  return PIPE_FORMAT_R16G16B16A16_UNORM;
   case D3DDECLTYPE_UDEC3:     return PIPE_FORMAT_R10G10B10X2_USCALED;
   case D3DDECLTYPE_DEC3N:     return PIPE_FORMAT_R10G10B10X2_SNORM;
   case D3DDECLTYPE_FLOAT16_2: return PIPE_FORMAT_R16G16_FLOAT;
   case D3DDECLTYPE_FLOAT16_4: return PIPE_FORMAT_R16G16B16A16_FLOAT;
   default:
         }
      }
      static inline unsigned decltype_size(BYTE type)
   {
      switch (type) {
   case D3DDECLTYPE_FLOAT1: return 1 * sizeof(float);
   case D3DDECLTYPE_FLOAT2: return 2 * sizeof(float);
   case D3DDECLTYPE_FLOAT3: return 3 * sizeof(float);
   case D3DDECLTYPE_FLOAT4: return 4 * sizeof(float);
   case D3DDECLTYPE_D3DCOLOR: return 1 * sizeof(DWORD);
   case D3DDECLTYPE_UBYTE4: return 4 * sizeof(BYTE);
   case D3DDECLTYPE_SHORT2: return 2 * sizeof(short);
   case D3DDECLTYPE_SHORT4: return 4 * sizeof(short);
   case D3DDECLTYPE_UBYTE4N: return 4 * sizeof(BYTE);
   case D3DDECLTYPE_SHORT2N: return 2 * sizeof(short);
   case D3DDECLTYPE_SHORT4N: return 4 * sizeof(short);
   case D3DDECLTYPE_USHORT2N: return 2 * sizeof(short);
   case D3DDECLTYPE_USHORT4N: return 4 * sizeof(short);
   case D3DDECLTYPE_UDEC3: return 4;
   case D3DDECLTYPE_DEC3N: return 4;
   case D3DDECLTYPE_FLOAT16_2: return 2 * 2;
   case D3DDECLTYPE_FLOAT16_4: return 4 * 2;
   default:
         }
      }
      /* Actually, arbitrary usage index values are permitted, but a
   * simple lookup table won't work in that case. Let's just wait
   * with making this more generic until we need it.
   */
   static inline bool
   nine_d3ddeclusage_check(unsigned usage, unsigned usage_idx)
   {
      switch (usage) {
   case D3DDECLUSAGE_POSITIONT:
   case D3DDECLUSAGE_TESSFACTOR:
   case D3DDECLUSAGE_DEPTH:
   case D3DDECLUSAGE_NORMAL:
   case D3DDECLUSAGE_TANGENT:
   case D3DDECLUSAGE_BINORMAL:
   case D3DDECLUSAGE_POSITION:
   case D3DDECLUSAGE_BLENDWEIGHT:
   case D3DDECLUSAGE_BLENDINDICES:
   case D3DDECLUSAGE_COLOR:
         case D3DDECLUSAGE_PSIZE:
   case D3DDECLUSAGE_FOG:
   case D3DDECLUSAGE_SAMPLE:
         case D3DDECLUSAGE_TEXCOORD:
         default:
            }
      #define NINE_DECLUSAGE_CASE0(n) case D3DDECLUSAGE_##n: return NINE_DECLUSAGE_##n
   #define NINE_DECLUSAGE_CASEi(n) case D3DDECLUSAGE_##n: return NINE_DECLUSAGE_i(n, usage_idx)
   uint16_t
   nine_d3d9_to_nine_declusage(unsigned usage, unsigned usage_idx)
   {
      if (!nine_d3ddeclusage_check(usage, usage_idx))
         assert(nine_d3ddeclusage_check(usage, usage_idx));
   switch (usage) {
   NINE_DECLUSAGE_CASEi(POSITION);
   NINE_DECLUSAGE_CASEi(BLENDWEIGHT);
   NINE_DECLUSAGE_CASEi(BLENDINDICES);
   NINE_DECLUSAGE_CASEi(NORMAL);
   NINE_DECLUSAGE_CASE0(PSIZE);
   NINE_DECLUSAGE_CASEi(TEXCOORD);
   NINE_DECLUSAGE_CASEi(TANGENT);
   NINE_DECLUSAGE_CASEi(BINORMAL);
   NINE_DECLUSAGE_CASE0(TESSFACTOR);
   NINE_DECLUSAGE_CASEi(POSITIONT);
   NINE_DECLUSAGE_CASEi(COLOR);
   NINE_DECLUSAGE_CASE0(DEPTH);
   NINE_DECLUSAGE_CASE0(FOG);
   NINE_DECLUSAGE_CASE0(SAMPLE);
   default:
      assert(!"Invalid DECLUSAGE.");
         }
      static const char *nine_declusage_names[] =
   {
      [NINE_DECLUSAGE_POSITION]        = "POSITION",
   [NINE_DECLUSAGE_BLENDWEIGHT]     = "BLENDWEIGHT",
   [NINE_DECLUSAGE_BLENDINDICES]    = "BLENDINDICES",
   [NINE_DECLUSAGE_NORMAL]          = "NORMAL",
   [NINE_DECLUSAGE_PSIZE]           = "PSIZE",
   [NINE_DECLUSAGE_TEXCOORD]        = "TEXCOORD",
   [NINE_DECLUSAGE_TANGENT]         = "TANGENT",
   [NINE_DECLUSAGE_BINORMAL]        = "BINORMAL",
   [NINE_DECLUSAGE_TESSFACTOR]      = "TESSFACTOR",
   [NINE_DECLUSAGE_POSITIONT]       = "POSITIONT",
   [NINE_DECLUSAGE_COLOR]           = "DIFFUSE",
   [NINE_DECLUSAGE_DEPTH]           = "DEPTH",
   [NINE_DECLUSAGE_FOG]             = "FOG",
      };
   static inline const char *
   nine_declusage_name(unsigned ndcl)
   {
         }
      HRESULT
   NineVertexDeclaration9_ctor( struct NineVertexDeclaration9 *This,
               {
      const D3DCAPS9 *caps;
   unsigned i, nelems;
            /* wine */
   for (nelems = 0;
         pElements[nelems].Stream != 0xFF;
      user_assert(pElements[nelems].Type != D3DDECLTYPE_UNUSED, E_FAIL);
               caps = NineDevice9_GetCaps(pParams->device);
            HRESULT hr = NineUnknown_ctor(&This->base, pParams);
            This->nelems = nelems;
   This->decls = CALLOC(This->nelems+1, sizeof(D3DVERTEXELEMENT9));
   This->elems = CALLOC(This->nelems, sizeof(struct pipe_vertex_element));
   This->usage_map = CALLOC(This->nelems, sizeof(uint16_t));
   if (!This->decls || !This->elems || !This->usage_map) { return E_OUTOFMEMORY; }
            for (i = 0; i < This->nelems; ++i) {
      uint16_t usage = nine_d3d9_to_nine_declusage(This->decls[i].Usage,
                  if (This->decls[i].Usage == D3DDECLUSAGE_POSITIONT)
            This->elems[i].src_offset = This->decls[i].Offset;
   This->elems[i].instance_divisor = 0;
   This->elems[i].vertex_buffer_index = This->decls[i].Stream;
   This->elems[i].src_format = decltype_format(This->decls[i].Type);
   This->elems[i].dual_slot = false;
            DBG("VERTEXELEMENT[%u]: Stream=%u Offset=%u Type=%s DeclUsage=%s%d\n", i,
         This->decls[i].Stream,
   This->decls[i].Offset,
   util_format_name(This->elems[i].src_format),
                  }
      void
   NineVertexDeclaration9_dtor( struct NineVertexDeclaration9 *This )
   {
               FREE(This->decls);
   FREE(This->elems);
               }
      HRESULT NINE_WINAPI
   NineVertexDeclaration9_GetDeclaration( struct NineVertexDeclaration9 *This,
               {
      if (!pElement) {
      user_assert(pNumElements, D3DERR_INVALIDCALL);
   *pNumElements = This->nelems+1;
      }
   if (pNumElements) { *pNumElements = This->nelems+1; }
   memcpy(pElement, This->decls, sizeof(D3DVERTEXELEMENT9)*(This->nelems+1));
      }
      IDirect3DVertexDeclaration9Vtbl NineVertexDeclaration9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of VertexDecl9 iface */
      };
      static const GUID *NineVertexDeclaration9_IIDs[] = {
      &IID_IDirect3DVertexDeclaration9,
   &IID_IUnknown,
      };
      HRESULT
   NineVertexDeclaration9_new( struct NineDevice9 *pDevice,
               {
         }
      HRESULT
   NineVertexDeclaration9_new_from_fvf( struct NineDevice9 *pDevice,
               {
      D3DVERTEXELEMENT9 elems[16], decl_end = D3DDECL_END();
   unsigned texcount, i, betas, nelems = 0;
            switch (FVF & D3DFVF_POSITION_MASK) {
      case D3DFVF_XYZ: /* simple XYZ */
   case D3DFVF_XYZB1:
   case D3DFVF_XYZB2:
   case D3DFVF_XYZB3:
   case D3DFVF_XYZB4:
   case D3DFVF_XYZB5: /* XYZ with beta values */
         elems[nelems].Type = D3DDECLTYPE_FLOAT3;
   elems[nelems].Usage = D3DDECLUSAGE_POSITION;
   elems[nelems].UsageIndex = 0;
   ++nelems;
                  betas = (((FVF & D3DFVF_XYZB5)-D3DFVF_XYZB1)>>1)+1;
   if (FVF & D3DFVF_LASTBETA_D3DCOLOR) {
         } else if (FVF & D3DFVF_LASTBETA_UBYTE4) {
         } else if ((FVF & D3DFVF_XYZB5) == D3DFVF_XYZB5) {
                        if (betas > 0) {
      switch (betas) {
      case 1: elems[nelems].Type = D3DDECLTYPE_FLOAT1; break;
   case 2: elems[nelems].Type = D3DDECLTYPE_FLOAT2; break;
   case 3: elems[nelems].Type = D3DDECLTYPE_FLOAT3; break;
   case 4: elems[nelems].Type = D3DDECLTYPE_FLOAT4; break;
   default:
      }
   elems[nelems].Usage = D3DDECLUSAGE_BLENDWEIGHT;
                     if (beta_index != 0xFF) {
      elems[nelems].Type = beta_index;
   elems[nelems].Usage = D3DDECLUSAGE_BLENDINDICES;
   elems[nelems].UsageIndex = 0;
               case D3DFVF_XYZW: /* simple XYZW */
   case D3DFVF_XYZRHW: /* pretransformed XYZW */
         elems[nelems].Type = D3DDECLTYPE_FLOAT4;
   elems[nelems].Usage =
      ((FVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZW) ?
      elems[nelems].UsageIndex = 0;
            default:
               /* normals, psize and colors */
   if (FVF & D3DFVF_NORMAL) {
      elems[nelems].Type = D3DDECLTYPE_FLOAT3;
   elems[nelems].Usage = D3DDECLUSAGE_NORMAL;
   elems[nelems].UsageIndex = 0;
      }
   if (FVF & D3DFVF_PSIZE) {
      elems[nelems].Type = D3DDECLTYPE_FLOAT1;
   elems[nelems].Usage = D3DDECLUSAGE_PSIZE;
   elems[nelems].UsageIndex = 0;
      }
   if (FVF & D3DFVF_DIFFUSE) {
      elems[nelems].Type = D3DDECLTYPE_D3DCOLOR;
   elems[nelems].Usage = D3DDECLUSAGE_COLOR;
   elems[nelems].UsageIndex = 0;
      }
   if (FVF & D3DFVF_SPECULAR) {
      elems[nelems].Type = D3DDECLTYPE_D3DCOLOR;
   elems[nelems].Usage = D3DDECLUSAGE_COLOR;
   elems[nelems].UsageIndex = 1;
               /* textures */
   texcount = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
            for (i = 0; i < texcount; ++i) {
      switch ((FVF >> (16+i*2)) & 0x3) {
         case D3DFVF_TEXTUREFORMAT1:
                  case D3DFVF_TEXTUREFORMAT2:
                  case D3DFVF_TEXTUREFORMAT3:
                  case D3DFVF_TEXTUREFORMAT4:
                  default:
   }
   elems[nelems].Usage = D3DDECLUSAGE_TEXCOORD;
   elems[nelems].UsageIndex = i;
               /* fill out remaining data */
   for (i = 0; i < nelems; ++i) {
      elems[i].Stream = 0;
   elems[i].Offset = (i == 0) ? 0 : (elems[i-1].Offset +
            }
               }
      void
   NineVertexDeclaration9_FillStreamOutputInfo(
      struct NineVertexDeclaration9 *This,
   struct nine_vs_output_info *ShaderOutputsInfo,
   unsigned numOutputs,
      {
      unsigned so_outputs = 0;
                     for (i = 0; i < numOutputs; i++) {
      BYTE output_semantic = ShaderOutputsInfo[i].output_semantic;
            for (j = 0; j < This->nelems; j++) {
         if ((This->decls[j].Usage == output_semantic ||
      (output_semantic == D3DDECLUSAGE_POSITION &&
         This->decls[j].UsageIndex == output_semantic_index) {
   DBG("Matching %s %d: o%d -> %d\n",
      nine_declusage_name(nine_d3d9_to_nine_declusage(This->decls[j].Usage, 0)),
      so->output[so_outputs].register_index = ShaderOutputsInfo[i].output_index;
   so->output[so_outputs].start_component = 0;
   if (ShaderOutputsInfo[i].mask & 8)
         else if (ShaderOutputsInfo[i].mask & 4)
         else if (ShaderOutputsInfo[i].mask & 2)
         else
         so->output[so_outputs].output_buffer = 0;
   so->output[so_outputs].dst_offset = so_outputs * sizeof(float[4])/4;
   so->output[so_outputs].stream = 0;
   so_outputs++;
                  so->num_outputs = so_outputs;
      }
      /* ProcessVertices runs stream output into a temporary buffer to capture
   * all outputs.
   * Now we have to convert them to the format and order set by the vertex
   * declaration, for which we use u_translate.
   * This is necessary if the vertex declaration contains elements using a
   * non float32 format, because stream output only supports f32/u32/s32.
   */
   HRESULT
   NineVertexDeclaration9_ConvertStreamOutput(
      struct NineVertexDeclaration9 *This,
   struct NineVertexBuffer9 *pDstBuf,
   UINT DestIndex,
   UINT VertexCount,
   void *pSrcBuf,
      {
      struct translate *translate;
   struct translate_key transkey;
   HRESULT hr;
   unsigned i;
            DBG("This=%p pDstBuf=%p DestIndex=%u VertexCount=%u pSrcBuf=%p so=%p\n",
            transkey.output_stride = 0;
   for (i = 0; i < This->nelems; ++i) {
               switch (so->output[i].num_components) {
   case 1: format = PIPE_FORMAT_R32_FLOAT; break;
   case 2: format = PIPE_FORMAT_R32G32_FLOAT; break;
   case 3: format = PIPE_FORMAT_R32G32B32_FLOAT; break;
   default:
         assert(so->output[i].num_components == 4);
   format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }
   transkey.element[i].type = TRANSLATE_ELEMENT_NORMAL;
   transkey.element[i].input_format = format;
   transkey.element[i].input_buffer = 0;
   transkey.element[i].input_offset = so->output[i].dst_offset * 4;
            transkey.element[i].output_format = This->elems[i].src_format;
   transkey.element[i].output_offset = This->elems[i].src_offset;
   transkey.output_stride +=
               }
            translate = translate_create(&transkey);
   if (!translate)
            hr = NineVertexBuffer9_Lock(pDstBuf,
                     if (FAILED(hr))
                                 out:
      translate->release(translate); /* TODO: cache these */
      }
