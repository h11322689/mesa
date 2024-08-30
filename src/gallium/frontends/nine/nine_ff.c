      /* FF is big and ugly so feel free to write lines as long as you like.
   * Aieeeeeeeee !
   *
   * Let me make that clearer:
   * Aieeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee ! !! !!!
   */
      #include "device9.h"
   #include "basetexture9.h"
   #include "vertexdeclaration9.h"
   #include "vertexshader9.h"
   #include "pixelshader9.h"
   #include "nine_ff.h"
   #include "nine_defines.h"
   #include "nine_helpers.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
      #include "pipe/p_context.h"
   #include "tgsi/tgsi_ureg.h"
   #include "tgsi/tgsi_dump.h"
   #include "util/bitscan.h"
   #include "util/u_box.h"
   #include "util/u_hash_table.h"
   #include "util/u_upload_mgr.h"
      #define DBG_CHANNEL DBG_FF
      #define NINE_FF_NUM_VS_CONST 204
   #define NINE_FF_NUM_PS_CONST 24
      struct fvec4
   {
         };
      struct nine_ff_vs_key
   {
      union {
      struct {
         uint32_t position_t : 1;
   uint32_t lighting   : 1;
   uint32_t darkness   : 1; /* lighting enabled but no active lights */
   uint32_t localviewer : 1;
   uint32_t vertexpointsize : 1;
   uint32_t pointscale : 1;
   uint32_t vertexblend : 3;
   uint32_t vertexblend_indexed : 1;
   uint32_t vertextween : 1;
   uint32_t mtl_diffuse : 2; /* 0 = material, 1 = color1, 2 = color2 */
   uint32_t mtl_ambient : 2;
   uint32_t mtl_specular : 2;
   uint32_t mtl_emissive : 2;
   uint32_t fog_mode : 2;
   uint32_t fog_range : 1;
   uint32_t color0in_one : 1;
   uint32_t color1in_zero : 1;
   uint32_t has_normal : 1;
   uint32_t fog : 1;
   uint32_t normalizenormals : 1;
   uint32_t ucp : 1;
   uint32_t pad1 : 4;
   uint32_t tc_dim_input: 16; /* 8 * 2 bits */
   uint32_t pad2 : 16;
   uint32_t tc_dim_output: 24; /* 8 * 3 bits */
   uint32_t pad3 : 8;
   uint32_t tc_gen : 24; /* 8 * 3 bits */
   uint32_t pad4 : 8;
   uint32_t tc_idx : 24;
   uint32_t clipplane_emulate : 8;
   };
   uint64_t value64[3]; /* don't forget to resize VertexShader9.ff_key */
         };
      /* Texture stage state:
   *
   * COLOROP       D3DTOP 5 bit
   * ALPHAOP       D3DTOP 5 bit
   * COLORARG0     D3DTA  3 bit
   * COLORARG1     D3DTA  3 bit
   * COLORARG2     D3DTA  3 bit
   * ALPHAARG0     D3DTA  3 bit
   * ALPHAARG1     D3DTA  3 bit
   * ALPHAARG2     D3DTA  3 bit
   * RESULTARG     D3DTA  1 bit (CURRENT:0 or TEMP:1)
   * TEXCOORDINDEX 0 - 7  3 bit
   * ===========================
   *                     32 bit per stage
   */
   struct nine_ff_ps_key
   {
      union {
      struct {
         struct {
      uint32_t colorop   : 5;
   uint32_t alphaop   : 5;
   uint32_t colorarg0 : 3;
   uint32_t colorarg1 : 3;
   uint32_t colorarg2 : 3;
   uint32_t alphaarg0 : 3;
   uint32_t alphaarg1 : 3;
   uint32_t alphaarg2 : 3;
   uint32_t resultarg : 1; /* CURRENT:0 or TEMP:1 */
   uint32_t textarget : 2; /* 1D/2D/3D/CUBE */
   uint32_t pad       : 1;
      } ts[8];
   uint32_t projected : 16;
   uint32_t fog : 1; /* for vFog coming from VS */
   uint32_t fog_mode : 2;
   uint32_t fog_source : 1; /* 0: Z, 1: W */
   uint32_t specular : 1;
   uint32_t alpha_test_emulation : 3;
   uint32_t flatshade : 1;
   uint32_t pad1 : 7; /* 9 32-bit words with this */
   uint8_t colorarg_b4[3];
   uint8_t colorarg_b5[3];
   uint8_t alphaarg_b4[3]; /* 11 32-bit words plus a byte */
   };
   uint64_t value64[6]; /* don't forget to resize PixelShader9.ff_key */
         };
      static uint32_t nine_ff_vs_key_hash(const void *key)
   {
      const struct nine_ff_vs_key *vs = key;
   unsigned i;
   uint32_t hash = vs->value32[0];
   for (i = 1; i < ARRAY_SIZE(vs->value32); ++i)
            }
   static bool nine_ff_vs_key_comp(const void *key1, const void *key2)
   {
      struct nine_ff_vs_key *a = (struct nine_ff_vs_key *)key1;
               }
   static uint32_t nine_ff_ps_key_hash(const void *key)
   {
      const struct nine_ff_ps_key *ps = key;
   unsigned i;
   uint32_t hash = ps->value32[0];
   for (i = 1; i < ARRAY_SIZE(ps->value32); ++i)
            }
   static bool nine_ff_ps_key_comp(const void *key1, const void *key2)
   {
      struct nine_ff_ps_key *a = (struct nine_ff_ps_key *)key1;
               }
   static uint32_t nine_ff_fvf_key_hash(const void *key)
   {
         }
   static bool nine_ff_fvf_key_comp(const void *key1, const void *key2)
   {
         }
      static void nine_ff_prune_vs(struct NineDevice9 *);
   static void nine_ff_prune_ps(struct NineDevice9 *);
      static void nine_ureg_tgsi_dump(struct ureg_program *ureg, bool override)
   {
      if (debug_get_bool_option("NINE_FF_DUMP", false) || override) {
      const struct tgsi_token *toks = ureg_get_tokens(ureg, NULL);
   tgsi_dump(toks, 0);
         }
      #define _X(r) ureg_scalar(ureg_src(r), TGSI_SWIZZLE_X)
   #define _Y(r) ureg_scalar(ureg_src(r), TGSI_SWIZZLE_Y)
   #define _Z(r) ureg_scalar(ureg_src(r), TGSI_SWIZZLE_Z)
   #define _W(r) ureg_scalar(ureg_src(r), TGSI_SWIZZLE_W)
      #define _XXXX(r) ureg_scalar(r, TGSI_SWIZZLE_X)
   #define _YYYY(r) ureg_scalar(r, TGSI_SWIZZLE_Y)
   #define _ZZZZ(r) ureg_scalar(r, TGSI_SWIZZLE_Z)
   #define _WWWW(r) ureg_scalar(r, TGSI_SWIZZLE_W)
      #define _XYZW(r) (r)
      /* AL should contain base address of lights table. */
   #define LIGHT_CONST(i)                                                \
            #define MATERIAL_CONST(i) \
            #define _CONST(n) ureg_DECL_constant(ureg, n)
      /* VS FF constants layout:
   *
   * CONST[ 0.. 3] D3DTS_WORLD * D3DTS_VIEW * D3DTS_PROJECTION
   * CONST[ 4.. 7] D3DTS_WORLD * D3DTS_VIEW
   * CONST[ 8..11] D3DTS_PROJECTION
   * CONST[12..15] D3DTS_VIEW^(-1)
   * CONST[16..18] Normal matrix
   *
   * CONST[19].xyz  MATERIAL.Emissive + Material.Ambient * RS.Ambient
   * CONST[20]      MATERIAL.Diffuse
   * CONST[21]      MATERIAL.Ambient
   * CONST[22]      MATERIAL.Specular
   * CONST[23].x___ MATERIAL.Power
   * CONST[24]      MATERIAL.Emissive
   * CONST[25]      RS.Ambient
   *
   * CONST[26].x___ RS.PointSizeMin
   * CONST[26]._y__ RS.PointSizeMax
   * CONST[26].__z_ RS.PointSize
   * CONST[26].___w RS.PointScaleA
   * CONST[27].x___ RS.PointScaleB
   * CONST[27]._y__ RS.PointScaleC
   *
   * CONST[28].x___ RS.FogEnd
   * CONST[28]._y__ 1.0f / (RS.FogEnd - RS.FogStart)
   * CONST[28].__z_ RS.FogDensity
      * CONST[30].x___ TWEENFACTOR
   *
   * CONST[32].x___ LIGHT[0].Type
   * CONST[32]._yzw LIGHT[0].Attenuation0,1,2
   * CONST[33]      LIGHT[0].Diffuse
   * CONST[34]      LIGHT[0].Specular
   * CONST[35]      LIGHT[0].Ambient
   * CONST[36].xyz_ LIGHT[0].Position
   * CONST[36].___w LIGHT[0].Range
   * CONST[37].xyz_ LIGHT[0].Direction
   * CONST[37].___w LIGHT[0].Falloff
   * CONST[38].x___ cos(LIGHT[0].Theta / 2)
   * CONST[38]._y__ cos(LIGHT[0].Phi / 2)
   * CONST[38].__z_ 1.0f / (cos(LIGHT[0].Theta / 2) - cos(Light[0].Phi / 2))
   * CONST[39].xyz_ LIGHT[0].HalfVector (for directional lights)
   * CONST[39].___w 1 if this is the last active light, 0 if not
   * CONST[40]      LIGHT[1]
   * CONST[48]      LIGHT[2]
   * CONST[56]      LIGHT[3]
   * CONST[64]      LIGHT[4]
   * CONST[72]      LIGHT[5]
   * CONST[80]      LIGHT[6]
   * CONST[88]      LIGHT[7]
   * NOTE: no lighting code is generated if there are no active lights
   *
   * CONST[100].x___ Viewport 2/width
   * CONST[100]._y__ Viewport 2/height
   * CONST[100].__z_ Viewport 1/(zmax - zmin)
   * CONST[100].___w Viewport width
   * CONST[101].x___ Viewport x0
   * CONST[101]._y__ Viewport y0
   * CONST[101].__z_ Viewport z0
   *
   * CONST[128..131] D3DTS_TEXTURE0
   * CONST[132..135] D3DTS_TEXTURE1
   * CONST[136..139] D3DTS_TEXTURE2
   * CONST[140..143] D3DTS_TEXTURE3
   * CONST[144..147] D3DTS_TEXTURE4
   * CONST[148..151] D3DTS_TEXTURE5
   * CONST[152..155] D3DTS_TEXTURE6
   * CONST[156..159] D3DTS_TEXTURE7
   *
   * CONST[160] D3DTS_WORLDMATRIX[0] * D3DTS_VIEW
   * CONST[164] D3DTS_WORLDMATRIX[1] * D3DTS_VIEW
   * ...
   * CONST[192] D3DTS_WORLDMATRIX[8] * D3DTS_VIEW
   * CONST[196] UCP0
   ...
   * CONST[203] UCP7
   */
   struct vs_build_ctx
   {
      struct ureg_program *ureg;
            uint16_t input[PIPE_MAX_ATTRIBS];
            struct ureg_src aVtx;
   struct ureg_src aNrm;
   struct ureg_src aCol[2];
   struct ureg_src aTex[8];
   struct ureg_src aPsz;
   struct ureg_src aInd;
            struct ureg_src aVtx1; /* tweening */
            struct ureg_src mtlA;
   struct ureg_src mtlD;
   struct ureg_src mtlS;
      };
      static inline unsigned
   get_texcoord_sn(struct pipe_screen *screen)
   {
      if (screen->get_param(screen, PIPE_CAP_TGSI_TEXCOORD))
            }
      static inline struct ureg_src
   build_vs_add_input(struct vs_build_ctx *vs, uint16_t ndecl)
   {
      const unsigned i = vs->num_inputs++;
   assert(i < PIPE_MAX_ATTRIBS);
   vs->input[i] = ndecl;
      }
      /* NOTE: dst may alias src */
   static inline void
   ureg_normalize3(struct ureg_program *ureg,
         {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
            ureg_DP3(ureg, tmp_x, src, src);
   ureg_RSQ(ureg, tmp_x, _X(tmp));
   ureg_MUL(ureg, dst, src, _X(tmp));
      }
      static void *
   nine_ff_build_vs(struct NineDevice9 *device, struct vs_build_ctx *vs)
   {
      const struct nine_ff_vs_key *key = vs->key;
   struct ureg_program *ureg = ureg_create(PIPE_SHADER_VERTEX);
   struct ureg_dst oPos, oCol[2], oPsz, oFog;
   struct ureg_dst AR;
   unsigned i, c;
   unsigned label[32], l = 0;
   bool need_aNrm = key->lighting || key->passthrough & (1 << NINE_DECLUSAGE_NORMAL);
   bool has_aNrm;
   bool need_aVtx = key->lighting || key->fog_mode || key->pointscale || key->ucp;
                     /* Check which inputs we should transform. */
   for (i = 0; i < 8 * 3; i += 3) {
      switch ((key->tc_gen >> i) & 0x7) {
   case NINED3DTSS_TCI_CAMERASPACENORMAL:
         need_aNrm = true;
   case NINED3DTSS_TCI_CAMERASPACEPOSITION:
         need_aVtx = true;
   case NINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
         need_aVtx = need_aNrm = true;
   case NINED3DTSS_TCI_SPHEREMAP:
         need_aVtx = need_aNrm = true;
   default:
                              /* Declare and record used inputs (needed for linkage with vertex format):
   * (texture coordinates handled later)
   */
   vs->aVtx = build_vs_add_input(vs,
            vs->aNrm = ureg_imm1f(ureg, 0.0f);
   if (has_aNrm)
            vs->aCol[0] = ureg_imm1f(ureg, 1.0f);
            if (key->lighting || key->darkness) {
      const unsigned mask = key->mtl_diffuse | key->mtl_specular |
         if ((mask & 0x1) && !key->color0in_one)
         if ((mask & 0x2) && !key->color1in_zero)
            vs->mtlD = MATERIAL_CONST(1);
   vs->mtlA = MATERIAL_CONST(2);
   vs->mtlS = MATERIAL_CONST(3);
   vs->mtlE = MATERIAL_CONST(5);
   if (key->mtl_diffuse  == 1) vs->mtlD = vs->aCol[0]; else
   if (key->mtl_diffuse  == 2) vs->mtlD = vs->aCol[1];
   if (key->mtl_ambient  == 1) vs->mtlA = vs->aCol[0]; else
   if (key->mtl_ambient  == 2) vs->mtlA = vs->aCol[1];
   if (key->mtl_specular == 1) vs->mtlS = vs->aCol[0]; else
   if (key->mtl_specular == 2) vs->mtlS = vs->aCol[1];
   if (key->mtl_emissive == 1) vs->mtlE = vs->aCol[0]; else
      } else {
      if (!key->color0in_one) vs->aCol[0] = build_vs_add_input(vs, NINE_DECLUSAGE_i(COLOR, 0));
               if (key->vertexpointsize)
            if (key->vertexblend_indexed || key->passthrough & (1 << NINE_DECLUSAGE_BLENDINDICES))
         if (key->vertexblend || key->passthrough & (1 << NINE_DECLUSAGE_BLENDWEIGHT))
         if (key->vertextween) {
      vs->aVtx1 = build_vs_add_input(vs, NINE_DECLUSAGE_i(POSITION,1));
               /* Declare outputs:
   */
   oPos = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0); /* HPOS */
   oCol[0] = ureg_saturate(ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0));
   oCol[1] = ureg_saturate(ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 1));
   if (key->fog || key->passthrough & (1 << NINE_DECLUSAGE_FOG)) {
      oFog = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 16);
               if (key->vertexpointsize || key->pointscale || device->driver_caps.always_output_pointsize) {
      oPsz = ureg_DECL_output_masked(ureg, TGSI_SEMANTIC_PSIZE, 0,
                     if (key->lighting || key->vertexblend)
            /* === Vertex transformation / vertex blending:
            if (key->position_t) {
      if (device->driver_caps.window_space_position_support) {
         } else {
         struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   /* vs->aVtx contains the coordinates buffer wise.
   * later in the pipeline, clipping, viewport and division
   * by w (rhw = 1/w) are going to be applied, so do the reverse
   * of these transformations (except clipping) to have the good
   * position at the end.*/
   ureg_MOV(ureg, tmp, vs->aVtx);
   /* X from [X_min, X_min + width] to [-1, 1], same for Y. Z to [0, 1] */
   ureg_ADD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ), ureg_src(tmp), ureg_negate(_CONST(101)));
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ), ureg_src(tmp), _CONST(100));
   ureg_ADD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XY), ureg_src(tmp), ureg_imm1f(ureg, -1.0f));
   /* Y needs to be reversed */
   ureg_MOV(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_negate(ureg_src(tmp)));
   /* Replace w by 1 if it equals to 0 */
   ureg_CMP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_W), ureg_negate(ureg_abs(ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_W))),
         /* inverse rhw */
   ureg_RCP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_W), _W(tmp));
   /* multiply X, Y, Z by w */
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ), ureg_src(tmp), _W(tmp));
   ureg_MOV(ureg, oPos, ureg_src(tmp));
      } else if (key->vertexblend) {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp2 = ureg_DECL_temporary(ureg);
   struct ureg_dst aVtx_dst = ureg_DECL_temporary(ureg);
   struct ureg_dst aNrm_dst = ureg_DECL_temporary(ureg);
   struct ureg_dst sum_blendweights = ureg_DECL_temporary(ureg);
            for (i = 160; i <= 195; ++i)
            /* translate world matrix index to constant file index */
   if (key->vertexblend_indexed) {
         ureg_MAD(ureg, tmp, vs->aInd, ureg_imm1f(ureg, 4.0f), ureg_imm1f(ureg, 160.0f));
            ureg_MOV(ureg, aVtx_dst, ureg_imm4f(ureg, 0.0f, 0.0f, 0.0f, 0.0f));
   ureg_MOV(ureg, aNrm_dst, ureg_imm4f(ureg, 0.0f, 0.0f, 0.0f, 0.0f));
            for (i = 0; i < key->vertexblend; ++i) {
         for (c = 0; c < 4; ++c) {
      cWM[c] = ureg_src_dimension(ureg_src_register(TGSI_FILE_CONSTANT, (160 + i * 4) * !key->vertexblend_indexed + c), 0);
                     /* multiply by WORLD(index) */
   ureg_MUL(ureg, tmp, _XXXX(vs->aVtx), cWM[0]);
   ureg_MAD(ureg, tmp, _YYYY(vs->aVtx), cWM[1], ureg_src(tmp));
                  if (has_aNrm) {
      /* Note: the spec says the transpose of the inverse of the
   * WorldView matrices should be used, but all tests show
   * otherwise.
   * Only case unknown: D3DVBF_0WEIGHTS */
   ureg_MUL(ureg, tmp2, _XXXX(vs->aNrm), cWM[0]);
                     if (i < (key->vertexblend - 1)) {
      /* accumulate weighted position value */
   ureg_MAD(ureg, aVtx_dst, ureg_src(tmp), ureg_scalar(vs->aWgt, i), ureg_src(aVtx_dst));
   if (has_aNrm)
         /* subtract weighted position value for last value */
               /* the last weighted position is always 1 - sum_of_previous_weights */
   ureg_MAD(ureg, aVtx_dst, ureg_src(tmp), ureg_scalar(ureg_src(sum_blendweights), key->vertexblend - 1), ureg_src(aVtx_dst));
   if (has_aNrm)
            /* multiply by VIEW_PROJ */
   ureg_MUL(ureg, tmp, _X(aVtx_dst), _CONST(8));
   ureg_MAD(ureg, tmp, _Y(aVtx_dst), _CONST(9),  ureg_src(tmp));
   ureg_MAD(ureg, tmp, _Z(aVtx_dst), _CONST(10), ureg_src(tmp));
            if (need_aVtx)
            ureg_release_temporary(ureg, tmp);
   ureg_release_temporary(ureg, tmp2);
   ureg_release_temporary(ureg, sum_blendweights);
   if (!need_aVtx)
            if (has_aNrm) {
         if (key->normalizenormals)
         } else
      } else {
               if (key->vertextween) {
         struct ureg_dst aVtx_dst = ureg_DECL_temporary(ureg);
   ureg_LRP(ureg, aVtx_dst, _XXXX(_CONST(30)), vs->aVtx1, vs->aVtx);
   vs->aVtx = ureg_src(aVtx_dst);
   if (has_aNrm) {
      struct ureg_dst aNrm_dst = ureg_DECL_temporary(ureg);
   ureg_LRP(ureg, aNrm_dst, _XXXX(_CONST(30)), vs->aNrm1, vs->aNrm);
               /* position = vertex * WORLD_VIEW_PROJ */
   ureg_MUL(ureg, tmp, _XXXX(vs->aVtx), _CONST(0));
   ureg_MAD(ureg, tmp, _YYYY(vs->aVtx), _CONST(1), ureg_src(tmp));
   ureg_MAD(ureg, tmp, _ZZZZ(vs->aVtx), _CONST(2), ureg_src(tmp));
   ureg_MAD(ureg, oPos, _WWWW(vs->aVtx), _CONST(3), ureg_src(tmp));
            if (need_aVtx) {
         struct ureg_dst aVtx_dst = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_XYZ);
   ureg_MUL(ureg, aVtx_dst, _XXXX(vs->aVtx), _CONST(4));
   ureg_MAD(ureg, aVtx_dst, _YYYY(vs->aVtx), _CONST(5), ureg_src(aVtx_dst));
   ureg_MAD(ureg, aVtx_dst, _ZZZZ(vs->aVtx), _CONST(6), ureg_src(aVtx_dst));
   ureg_MAD(ureg, aVtx_dst, _WWWW(vs->aVtx), _CONST(7), ureg_src(aVtx_dst));
   }
   if (has_aNrm) {
         struct ureg_dst aNrm_dst = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_XYZ);
   ureg_MUL(ureg, aNrm_dst, _XXXX(vs->aNrm), _CONST(16));
   ureg_MAD(ureg, aNrm_dst, _YYYY(vs->aNrm), _CONST(17), ureg_src(aNrm_dst));
   ureg_MAD(ureg, aNrm_dst, _ZZZZ(vs->aNrm), _CONST(18), ureg_src(aNrm_dst));
   if (key->normalizenormals)
                     /* === Process point size:
   */
   if (key->vertexpointsize || key->pointscale || device->driver_caps.always_output_pointsize) {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp_x = ureg_writemask(tmp, TGSI_WRITEMASK_X);
   struct ureg_dst tmp_y = ureg_writemask(tmp, TGSI_WRITEMASK_Y);
   struct ureg_dst tmp_z = ureg_writemask(tmp, TGSI_WRITEMASK_Z);
   if (key->vertexpointsize) {
         struct ureg_src cPsz1 = ureg_DECL_constant(ureg, 26);
   ureg_MAX(ureg, tmp_z, _XXXX(vs->aPsz), _XXXX(cPsz1));
   } else {
         struct ureg_src cPsz1 = ureg_DECL_constant(ureg, 26);
            if (key->pointscale) {
                        ureg_DP3(ureg, tmp_x, vs->aVtx, vs->aVtx);
   ureg_RSQ(ureg, tmp_y, _X(tmp));
   ureg_MUL(ureg, tmp_y, _Y(tmp), _X(tmp));
   ureg_CMP(ureg, tmp_y, ureg_negate(_Y(tmp)), _Y(tmp), ureg_imm1f(ureg, 0.0f));
   ureg_MAD(ureg, tmp_x, _Y(tmp), _YYYY(cPsz2), _XXXX(cPsz2));
   ureg_MAD(ureg, tmp_x, _Y(tmp), _X(tmp), _WWWW(cPsz1));
   ureg_RSQ(ureg, tmp_x, _X(tmp));
   ureg_MUL(ureg, tmp_x, _X(tmp), _Z(tmp));
   ureg_MUL(ureg, tmp_x, _X(tmp), _WWWW(_CONST(100)));
   ureg_MAX(ureg, tmp_x, _X(tmp), _XXXX(cPsz1));
            ureg_MOV(ureg, oPsz, _Z(tmp));
               for (i = 0; i < 8; ++i) {
      struct ureg_dst tmp, tmp_x, tmp2;
   struct ureg_dst oTex, input_coord, transformed, t, aVtx_normed;
   unsigned c, writemask;
   const unsigned tci = (key->tc_gen >> (i * 3)) & 0x7;
   const unsigned idx = (key->tc_idx >> (i * 3)) & 0x7;
   unsigned dim_input = 1 + ((key->tc_dim_input >> (i * 2)) & 0x3);
            /* No texture output of index s */
   if (tci == NINED3DTSS_TCI_DISABLE)
         oTex = ureg_DECL_output(ureg, texcoord_sn, i);
   tmp = ureg_DECL_temporary(ureg);
   tmp_x = ureg_writemask(tmp, TGSI_WRITEMASK_X);
   input_coord = ureg_DECL_temporary(ureg);
            /* Get the coordinate */
   switch (tci) {
   case NINED3DTSS_TCI_PASSTHRU:
         /* NINED3DTSS_TCI_PASSTHRU => Use texcoord coming from index idx *
   * Else the idx is used only to determine wrapping mode. */
   vs->aTex[idx] = build_vs_add_input(vs, NINE_DECLUSAGE_i(TEXCOORD,idx));
   ureg_MOV(ureg, input_coord, vs->aTex[idx]);
   case NINED3DTSS_TCI_CAMERASPACENORMAL:
         ureg_MOV(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_XYZ), vs->aNrm);
   ureg_MOV(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_W), ureg_imm1f(ureg, 1.0f));
   dim_input = 4;
   case NINED3DTSS_TCI_CAMERASPACEPOSITION:
         ureg_MOV(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_XYZ), vs->aVtx);
   ureg_MOV(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_W), ureg_imm1f(ureg, 1.0f));
   dim_input = 4;
   case NINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
         tmp.WriteMask = TGSI_WRITEMASK_XYZ;
   aVtx_normed = ureg_DECL_temporary(ureg);
   ureg_normalize3(ureg, aVtx_normed, vs->aVtx);
   ureg_DP3(ureg, tmp_x, ureg_src(aVtx_normed), vs->aNrm);
   ureg_MUL(ureg, tmp, vs->aNrm, _X(tmp));
   ureg_ADD(ureg, tmp, ureg_src(tmp), ureg_src(tmp));
   ureg_ADD(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_XYZ), ureg_src(aVtx_normed), ureg_negate(ureg_src(tmp)));
   ureg_MOV(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_W), ureg_imm1f(ureg, 1.0f));
   ureg_release_temporary(ureg, aVtx_normed);
   dim_input = 4;
   tmp.WriteMask = TGSI_WRITEMASK_XYZW;
   case NINED3DTSS_TCI_SPHEREMAP:
         /* Implement the formula of GL_SPHERE_MAP */
   tmp.WriteMask = TGSI_WRITEMASK_XYZ;
   aVtx_normed = ureg_DECL_temporary(ureg);
   tmp2 = ureg_DECL_temporary(ureg);
   ureg_normalize3(ureg, aVtx_normed, vs->aVtx);
   ureg_DP3(ureg, tmp_x, ureg_src(aVtx_normed), vs->aNrm);
   ureg_MUL(ureg, tmp, vs->aNrm, _X(tmp));
   ureg_ADD(ureg, tmp, ureg_src(tmp), ureg_src(tmp));
   ureg_ADD(ureg, tmp, ureg_src(aVtx_normed), ureg_negate(ureg_src(tmp)));
   /* now tmp = normed(Vtx) - 2 dot3(normed(Vtx), Nrm) Nrm */
   ureg_MOV(ureg, ureg_writemask(tmp2, TGSI_WRITEMASK_XYZ), ureg_src(tmp));
   ureg_MUL(ureg, tmp2, ureg_src(tmp2), ureg_src(tmp2));
   ureg_DP3(ureg, ureg_writemask(tmp2, TGSI_WRITEMASK_X), ureg_src(tmp2), ureg_src(tmp2));
   ureg_RSQ(ureg, ureg_writemask(tmp2, TGSI_WRITEMASK_X), ureg_src(tmp2));
   ureg_MUL(ureg, ureg_writemask(tmp2, TGSI_WRITEMASK_X), ureg_src(tmp2), ureg_imm1f(ureg, 0.5f));
   /* tmp2 = 0.5 / sqrt(tmp.x^2 + tmp.y^2 + (tmp.z+1)^2)
   * TODO: z coordinates are a bit different gl vs d3d, should the formula be adapted ? */
   ureg_MUL(ureg, tmp, ureg_src(tmp), _X(tmp2));
   ureg_ADD(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_XY), ureg_src(tmp), ureg_imm1f(ureg, 0.5f));
   ureg_MOV(ureg, ureg_writemask(input_coord, TGSI_WRITEMASK_ZW), ureg_imm4f(ureg, 0.0f, 0.0f, 0.0f, 1.0f));
   ureg_release_temporary(ureg, aVtx_normed);
   ureg_release_temporary(ureg, tmp2);
   dim_input = 4;
   tmp.WriteMask = TGSI_WRITEMASK_XYZW;
   default:
         assert(0);
            /* Apply the transformation */
   /* dim_output == 0 => do not transform the components.
         if (!dim_output || key->position_t) {
         ureg_release_temporary(ureg, transformed);
   transformed = input_coord;
   } else {
         for (c = 0; c < dim_output; c++) {
      t = ureg_writemask(transformed, 1 << c);
   switch (dim_input) {
   /* dim_input = 1 2 3: -> we add trailing 1 to input*/
   case 1: ureg_MAD(ureg, t, _X(input_coord), _XXXX(_CONST(128 + i * 4 + c)), _YYYY(_CONST(128 + i * 4 + c)));
         case 2: ureg_DP2(ureg, t, ureg_src(input_coord), _CONST(128 + i * 4 + c));
               case 3: ureg_DP3(ureg, t, ureg_src(input_coord), _CONST(128 + i * 4 + c));
               case 4: ureg_DP4(ureg, t, ureg_src(input_coord), _CONST(128 + i * 4 + c)); break;
   default:
            }
   writemask = (1 << dim_output) - 1;
            ureg_MOV(ureg, ureg_writemask(oTex, writemask), ureg_src(transformed));
   ureg_release_temporary(ureg, transformed);
               /* === Lighting:
   *
   * DIRECTIONAL:  Light at infinite distance, parallel rays, no attenuation.
   * POINT: Finite distance to scene, divergent rays, isotropic, attenuation.
   * SPOT: Finite distance, divergent rays, angular dependence, attenuation.
   *
   * vec3 normal = normalize(in.Normal * NormalMatrix);
   * vec3 hitDir = light.direction;
   * float atten = 1.0;
   *
   * if (light.type != DIRECTIONAL)
   * {
   *     vec3 hitVec = light.position - eyeVertex;
   *     float d = length(hitVec);
   *     hitDir = hitVec / d;
   *     atten = 1 / ((light.atten2 * d + light.atten1) * d + light.atten0);
   * }
   *
   * if (light.type == SPOTLIGHT)
   * {
   *     float rho = dp3(-hitVec, light.direction);
   *     if (rho < cos(light.phi / 2))
   *         atten = 0;
   *     if (rho < cos(light.theta / 2))
   *         atten *= pow(some_func(rho), light.falloff);
   * }
   *
   * float nDotHit = dp3_sat(normal, hitVec);
   * float powFact = 0.0;
   *
   * if (nDotHit > 0.0)
   * {
   *     vec3 midVec = normalize(hitDir + eye);
   *     float nDotMid = dp3_sat(normal, midVec);
   *     pFact = pow(nDotMid, material.power);
   * }
   *
   * ambient += light.ambient * atten;
   * diffuse += light.diffuse * atten * nDotHit;
   * specular += light.specular * atten * powFact;
   */
   if (key->lighting) {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp_x = ureg_writemask(tmp, TGSI_WRITEMASK_X);
   struct ureg_dst tmp_y = ureg_writemask(tmp, TGSI_WRITEMASK_Y);
   struct ureg_dst tmp_z = ureg_writemask(tmp, TGSI_WRITEMASK_Z);
   struct ureg_dst rAtt = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_W);
   struct ureg_dst rHit = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_XYZ);
                              /* Light.*.Alpha is not used. */
   struct ureg_dst rD = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_XYZ);
   struct ureg_dst rA = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_XYZ);
                     struct ureg_src cLKind = _XXXX(LIGHT_CONST(0));
   struct ureg_src cLAtt0 = _YYYY(LIGHT_CONST(0));
   struct ureg_src cLAtt1 = _ZZZZ(LIGHT_CONST(0));
   struct ureg_src cLAtt2 = _WWWW(LIGHT_CONST(0));
   struct ureg_src cLColD = _XYZW(LIGHT_CONST(1));
   struct ureg_src cLColS = _XYZW(LIGHT_CONST(2));
   struct ureg_src cLColA = _XYZW(LIGHT_CONST(3));
   struct ureg_src cLPos  = _XYZW(LIGHT_CONST(4));
   struct ureg_src cLRng  = _WWWW(LIGHT_CONST(4));
   struct ureg_src cLDir  = _XYZW(LIGHT_CONST(5));
   struct ureg_src cLFOff = _WWWW(LIGHT_CONST(5));
   struct ureg_src cLTht  = _XXXX(LIGHT_CONST(6));
   struct ureg_src cLPhi  = _YYYY(LIGHT_CONST(6));
   struct ureg_src cLSDiv = _ZZZZ(LIGHT_CONST(6));
                     /* Declare all light constants to allow indirect adressing */
   for (i = 32; i < 96; i++)
            ureg_MOV(ureg, rCtr, ureg_imm1f(ureg, 32.0f)); /* &lightconst(0) */
   ureg_MOV(ureg, rD, ureg_imm1f(ureg, 0.0f));
   ureg_MOV(ureg, rA, ureg_imm1f(ureg, 0.0f));
            /* loop management */
   ureg_BGNLOOP(ureg, &label[loop_label]);
            /* if (not DIRECTIONAL light): */
   ureg_SNE(ureg, tmp_x, cLKind, ureg_imm1f(ureg, D3DLIGHT_DIRECTIONAL));
   ureg_MOV(ureg, rHit, ureg_negate(cLDir));
   ureg_MOV(ureg, rAtt, ureg_imm1f(ureg, 1.0f));
   ureg_IF(ureg, _X(tmp), &label[l++]);
   {
         /* hitDir = light.position - eyeVtx
   * d = length(hitDir)
   */
   ureg_ADD(ureg, rHit, cLPos, ureg_negate(vs->aVtx));
   ureg_DP3(ureg, tmp_x, ureg_src(rHit), ureg_src(rHit));
                  /* att = 1.0 / (light.att0 + (light.att1 + light.att2 * d) * d) */
   ureg_MAD(ureg, rAtt, _X(tmp), cLAtt2, cLAtt1);
   ureg_MAD(ureg, rAtt, _X(tmp), _W(rAtt), cLAtt0);
   ureg_RCP(ureg, rAtt, _W(rAtt));
   /* cut-off if distance exceeds Light.Range */
   ureg_SLT(ureg, tmp_x, _X(tmp), cLRng);
   }
   ureg_fixup_label(ureg, label[l-1], ureg_get_instruction_number(ureg));
            /* normalize hitDir */
            /* if (SPOT light) */
   ureg_SEQ(ureg, tmp_x, cLKind, ureg_imm1f(ureg, D3DLIGHT_SPOT));
   ureg_IF(ureg, _X(tmp), &label[l++]);
   {
         /* rho = dp3(-hitDir, light.spotDir)
   *
   * if (rho  > light.ctht2) NOTE: 0 <= phi <= pi, 0 <= theta <= phi
   *     spotAtt = 1
   * else
   * if (rho <= light.cphi2)
   *     spotAtt = 0
   * else
   *     spotAtt = (rho - light.cphi2) / (light.ctht2 - light.cphi2) ^ light.falloff
   */
   ureg_DP3(ureg, tmp_y, ureg_negate(ureg_src(rHit)), cLDir); /* rho */
   ureg_ADD(ureg, tmp_x, _Y(tmp), ureg_negate(cLPhi));
   ureg_MUL(ureg, tmp_x, _X(tmp), cLSDiv);
   ureg_POW(ureg, tmp_x, _X(tmp), cLFOff); /* spotAtten */
   ureg_SGE(ureg, tmp_z, _Y(tmp), cLTht); /* if inside theta && phi */
   ureg_SGE(ureg, tmp_y, _Y(tmp), cLPhi); /* if inside phi */
   ureg_MAD(ureg, ureg_saturate(tmp_x), _X(tmp), _Y(tmp), _Z(tmp));
   }
   ureg_fixup_label(ureg, label[l-1], ureg_get_instruction_number(ureg));
                     if (has_aNrm) {
         if (key->localviewer) {
      ureg_normalize3(ureg, rMid, vs->aVtx);
      } else {
         }
   ureg_normalize3(ureg, rMid, ureg_src(rMid));
   ureg_DP3(ureg, ureg_saturate(tmp_x), vs->aNrm, ureg_src(rHit));
   ureg_DP3(ureg, ureg_saturate(tmp_y), vs->aNrm, ureg_src(rMid));
   ureg_MUL(ureg, tmp_z, _X(tmp), _Y(tmp));
   /* Tests show that specular is computed only if (dp3(normal,hitDir) > 0).
   * For front facing, it is more restrictive than test (dp3(normal,mid) > 0).
   * No tests were made for backfacing, so add the two conditions */
   ureg_IF(ureg, _Z(tmp), &label[l++]);
   {
      ureg_DP3(ureg, ureg_saturate(tmp_y), vs->aNrm, ureg_src(rMid));
   ureg_POW(ureg, tmp_y, _Y(tmp), mtlP);
   ureg_MUL(ureg, tmp_y, _W(rAtt), _Y(tmp)); /* power factor * att */
      }
                  ureg_MUL(ureg, tmp_x, _W(rAtt), _X(tmp)); /* dp3(normal,hitDir) * att */
                     /* break if this was the last light */
   ureg_IF(ureg, cLLast, &label[l++]);
   ureg_BRK(ureg);
   ureg_ENDIF(ureg);
            ureg_ADD(ureg, rCtr, _W(rCtr), ureg_imm1f(ureg, 8.0f));
   ureg_fixup_label(ureg, label[loop_label], ureg_get_instruction_number(ureg));
            /* Apply to material:
      *
   * oCol[0] = (material.emissive + material.ambient * rs.ambient) +
   *           material.ambient * ambient +
   *           material.diffuse * diffuse +
   * oCol[1] = material.specular * specular;
      if (key->mtl_emissive == 0 && key->mtl_ambient == 0)
         else {
         ureg_ADD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ), ureg_src(rA), _CONST(25));
            ureg_MAD(ureg, ureg_writemask(oCol[0], TGSI_WRITEMASK_XYZ), ureg_src(rD), vs->mtlD, ureg_src(tmp));
   ureg_MOV(ureg, ureg_writemask(oCol[0], TGSI_WRITEMASK_W), vs->mtlD);
   ureg_MUL(ureg, oCol[1], ureg_src(rS), vs->mtlS);
   ureg_release_temporary(ureg, rAtt);
   ureg_release_temporary(ureg, rHit);
   ureg_release_temporary(ureg, rMid);
   ureg_release_temporary(ureg, rCtr);
   ureg_release_temporary(ureg, rD);
   ureg_release_temporary(ureg, rA);
   ureg_release_temporary(ureg, rS);
   ureg_release_temporary(ureg, rAtt);
      } else
   /* COLOR */
   if (key->darkness) {
      if (key->mtl_emissive == 0 && key->mtl_ambient == 0)
         else
         ureg_MOV(ureg, ureg_writemask(oCol[0], TGSI_WRITEMASK_W), vs->mtlD);
      } else {
      ureg_MOV(ureg, oCol[0], vs->aCol[0]);
               /* === Process fog.
   *
   * exp(x) = ex2(log2(e) * x)
   */
   if (key->fog_mode) {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp_x = ureg_writemask(tmp, TGSI_WRITEMASK_X);
   struct ureg_dst tmp_z = ureg_writemask(tmp, TGSI_WRITEMASK_Z);
   if (key->fog_range) {
         ureg_DP3(ureg, tmp_x, vs->aVtx, vs->aVtx);
   ureg_RSQ(ureg, tmp_z, _X(tmp));
   } else {
                  if (key->fog_mode == D3DFOG_EXP) {
         ureg_MUL(ureg, tmp_x, _Z(tmp), _ZZZZ(_CONST(28)));
   ureg_MUL(ureg, tmp_x, _X(tmp), ureg_imm1f(ureg, -1.442695f));
   } else
   if (key->fog_mode == D3DFOG_EXP2) {
         ureg_MUL(ureg, tmp_x, _Z(tmp), _ZZZZ(_CONST(28)));
   ureg_MUL(ureg, tmp_x, _X(tmp), _X(tmp));
   ureg_MUL(ureg, tmp_x, _X(tmp), ureg_imm1f(ureg, -1.442695f));
   } else
   if (key->fog_mode == D3DFOG_LINEAR) {
         ureg_ADD(ureg, tmp_x, _XXXX(_CONST(28)), ureg_negate(_Z(tmp)));
   }
   ureg_MOV(ureg, oFog, _X(tmp));
      } else if (key->fog && !(key->passthrough & (1 << NINE_DECLUSAGE_FOG))) {
                  if (key->passthrough & (1 << NINE_DECLUSAGE_BLENDWEIGHT)) {
      struct ureg_src input;
   struct ureg_dst output;
   input = vs->aWgt;
   output = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 19);
      }
   if (key->passthrough & (1 << NINE_DECLUSAGE_BLENDINDICES)) {
      struct ureg_src input;
   struct ureg_dst output;
   input = vs->aInd;
   output = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 20);
      }
   if (key->passthrough & (1 << NINE_DECLUSAGE_NORMAL)) {
      struct ureg_src input;
   struct ureg_dst output;
   input = vs->aNrm;
   output = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 21);
      }
   if (key->passthrough & (1 << NINE_DECLUSAGE_TANGENT)) {
      struct ureg_src input;
   struct ureg_dst output;
   input = build_vs_add_input(vs, NINE_DECLUSAGE_TANGENT);
   output = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 22);
      }
   if (key->passthrough & (1 << NINE_DECLUSAGE_BINORMAL)) {
      struct ureg_src input;
   struct ureg_dst output;
   input = build_vs_add_input(vs, NINE_DECLUSAGE_BINORMAL);
   output = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 23);
      }
   if (key->passthrough & (1 << NINE_DECLUSAGE_FOG)) {
      struct ureg_src input;
   struct ureg_dst output;
   input = build_vs_add_input(vs, NINE_DECLUSAGE_FOG);
   input = ureg_scalar(input, TGSI_SWIZZLE_X);
   output = oFog;
      }
   if (key->passthrough & (1 << NINE_DECLUSAGE_DEPTH)) {
                  /* ucp for ff applies on world coordinates.
   * aVtx is in worldview coordinates. */
   if (key->ucp) {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   ureg_MUL(ureg, tmp, _XXXX(vs->aVtx), _CONST(12));
   ureg_MAD(ureg, tmp, _YYYY(vs->aVtx), _CONST(13),  ureg_src(tmp));
   ureg_MAD(ureg, tmp, _ZZZZ(vs->aVtx), _CONST(14), ureg_src(tmp));
   if (!key->clipplane_emulate) {
         struct ureg_dst clipVect = ureg_DECL_output(ureg, TGSI_SEMANTIC_CLIPVERTEX, 0);
   } else {
         struct ureg_dst clipdist[2] = {ureg_dst_undef(), ureg_dst_undef()};
   int num_clipdist = ffs(key->clipplane_emulate);
   ureg_ADD(ureg, tmp, _CONST(15), ureg_src(tmp));
   clipdist[0] = ureg_DECL_output_masked(ureg, TGSI_SEMANTIC_CLIPDIST, 0,
         if (num_clipdist >= 5)
      clipdist[1] = ureg_DECL_output_masked(ureg, TGSI_SEMANTIC_CLIPDIST, 1,
      ureg_property(ureg, TGSI_PROPERTY_NUM_CLIPDIST_ENABLED, num_clipdist);
   for (i = 0; i < num_clipdist; i++) {
      assert(!ureg_dst_is_undef(clipdist[i>>2]));
   if (!(key->clipplane_emulate & (1 << i)))
         else
      ureg_DP4(ureg, ureg_writemask(clipdist[i>>2], 1 << (i & 0x2)),
   }
               if (key->position_t && device->driver_caps.window_space_position_support)
            ureg_END(ureg);
   nine_ureg_tgsi_dump(ureg, false);
      }
      /* PS FF constants layout:
   *
   * CONST[ 0.. 7]      stage[i].D3DTSS_CONSTANT
   * CONST[ 8..15].x___ stage[i].D3DTSS_BUMPENVMAT00
   * CONST[ 8..15]._y__ stage[i].D3DTSS_BUMPENVMAT01
   * CONST[ 8..15].__z_ stage[i].D3DTSS_BUMPENVMAT10
   * CONST[ 8..15].___w stage[i].D3DTSS_BUMPENVMAT11
   * CONST[16..19].x_z_ stage[i].D3DTSS_BUMPENVLSCALE
   * CONST[17..19]._y_w stage[i].D3DTSS_BUMPENVLOFFSET
   *
   * CONST[20] D3DRS_TEXTUREFACTOR
   * CONST[21] D3DRS_FOGCOLOR
   * CONST[22].x___ RS.FogEnd
   * CONST[22]._y__ 1.0f / (RS.FogEnd - RS.FogStart)
   * CONST[22].__z_ RS.FogDensity
   * CONST[22].___w Alpha ref
   */
   struct ps_build_ctx
   {
      struct ureg_program *ureg;
            struct ureg_src vC[2]; /* DIFFUSE, SPECULAR */
   struct ureg_src vT[8]; /* TEXCOORD[i] */
   struct ureg_dst rCur; /* D3DTA_CURRENT */
   struct ureg_dst rMod;
   struct ureg_src rCurSrc;
   struct ureg_dst rTmp; /* D3DTA_TEMP */
   struct ureg_src rTmpSrc;
   struct ureg_dst rTex;
   struct ureg_src rTexSrc;
   struct ureg_src cBEM[8];
            struct {
      unsigned index;
         };
      static struct ureg_src
   ps_get_ts_arg(struct ps_build_ctx *ps, unsigned ta)
   {
               switch (ta & D3DTA_SELECTMASK) {
   case D3DTA_CONSTANT:
      reg = ureg_DECL_constant(ps->ureg, ps->stage.index);
      case D3DTA_CURRENT:
      reg = (ps->stage.index == ps->stage.index_pre_mod) ? ureg_src(ps->rMod) : ps->rCurSrc;
      case D3DTA_DIFFUSE:
      reg = ureg_DECL_fs_input(ps->ureg, TGSI_SEMANTIC_COLOR, 0, ps->color_interpolate_flag);
      case D3DTA_SPECULAR:
      reg = ureg_DECL_fs_input(ps->ureg, TGSI_SEMANTIC_COLOR, 1, ps->color_interpolate_flag);
      case D3DTA_TEMP:
      reg = ps->rTmpSrc;
      case D3DTA_TEXTURE:
      reg = ps->rTexSrc;
      case D3DTA_TFACTOR:
      reg = ureg_DECL_constant(ps->ureg, 20);
      default:
      assert(0);
   reg = ureg_src_undef();
      }
   if (ta & D3DTA_COMPLEMENT) {
      struct ureg_dst dst = ureg_DECL_temporary(ps->ureg);
   ureg_ADD(ps->ureg, dst, ureg_imm1f(ps->ureg, 1.0f), ureg_negate(reg));
      }
   if (ta & D3DTA_ALPHAREPLICATE)
            }
      static struct ureg_dst
   ps_get_ts_dst(struct ps_build_ctx *ps, unsigned ta)
   {
               switch (ta & D3DTA_SELECTMASK) {
   case D3DTA_CURRENT:
         case D3DTA_TEMP:
         default:
      assert(0);
         }
      static uint8_t ps_d3dtop_args_mask(D3DTEXTUREOP top)
   {
      switch (top) {
   case D3DTOP_DISABLE:
         case D3DTOP_SELECTARG1:
   case D3DTOP_PREMODULATE:
         case D3DTOP_SELECTARG2:
         case D3DTOP_MULTIPLYADD:
   case D3DTOP_LERP:
         default:
            }
      static inline bool
   is_MOV_no_op(struct ureg_dst dst, struct ureg_src src)
   {
      return !dst.WriteMask ||
      (dst.File == src.File &&
      dst.Index == src.Index &&
   !dst.Indirect &&
   !dst.Saturate &&
   !src.Indirect &&
   !src.Negate &&
   !src.Absolute &&
   (!(dst.WriteMask & TGSI_WRITEMASK_X) || (src.SwizzleX == TGSI_SWIZZLE_X)) &&
   (!(dst.WriteMask & TGSI_WRITEMASK_Y) || (src.SwizzleY == TGSI_SWIZZLE_Y)) &&
         }
      static void
   ps_do_ts_op(struct ps_build_ctx *ps, unsigned top, struct ureg_dst dst, struct ureg_src *arg)
   {
      struct ureg_program *ureg = ps->ureg;
   struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp2 = ureg_DECL_temporary(ureg);
                     if (top != D3DTOP_SELECTARG1 && top != D3DTOP_SELECTARG2 &&
      top != D3DTOP_MODULATE && top != D3DTOP_PREMODULATE &&
   top != D3DTOP_BLENDDIFFUSEALPHA && top != D3DTOP_BLENDTEXTUREALPHA &&
   top != D3DTOP_BLENDFACTORALPHA && top != D3DTOP_BLENDCURRENTALPHA &&
   top != D3DTOP_BUMPENVMAP && top != D3DTOP_BUMPENVMAPLUMINANCE &&
   top != D3DTOP_LERP)
         switch (top) {
   case D3DTOP_SELECTARG1:
      if (!is_MOV_no_op(dst, arg[1]))
            case D3DTOP_SELECTARG2:
      if (!is_MOV_no_op(dst, arg[2]))
            case D3DTOP_MODULATE:
      ureg_MUL(ureg, dst, arg[1], arg[2]);
      case D3DTOP_MODULATE2X:
      ureg_MUL(ureg, tmp, arg[1], arg[2]);
   ureg_ADD(ureg, dst, ureg_src(tmp), ureg_src(tmp));
      case D3DTOP_MODULATE4X:
      ureg_MUL(ureg, tmp, arg[1], arg[2]);
   ureg_MUL(ureg, dst, ureg_src(tmp), ureg_imm1f(ureg, 4.0f));
      case D3DTOP_ADD:
      ureg_ADD(ureg, dst, arg[1], arg[2]);
      case D3DTOP_ADDSIGNED:
      ureg_ADD(ureg, tmp, arg[1], arg[2]);
   ureg_ADD(ureg, dst, ureg_src(tmp), ureg_imm1f(ureg, -0.5f));
      case D3DTOP_ADDSIGNED2X:
      ureg_ADD(ureg, tmp, arg[1], arg[2]);
   ureg_MAD(ureg, dst, ureg_src(tmp), ureg_imm1f(ureg, 2.0f), ureg_imm1f(ureg, -1.0f));
      case D3DTOP_SUBTRACT:
      ureg_ADD(ureg, dst, arg[1], ureg_negate(arg[2]));
      case D3DTOP_ADDSMOOTH:
      ureg_ADD(ureg, tmp, ureg_imm1f(ureg, 1.0f), ureg_negate(arg[1]));
   ureg_MAD(ureg, dst, ureg_src(tmp), arg[2], arg[1]);
      case D3DTOP_BLENDDIFFUSEALPHA:
      ureg_LRP(ureg, dst, _WWWW(ps->vC[0]), arg[1], arg[2]);
      case D3DTOP_BLENDTEXTUREALPHA:
      /* XXX: alpha taken from previous stage, texture or result ? */
   ureg_LRP(ureg, dst, _W(ps->rTex), arg[1], arg[2]);
      case D3DTOP_BLENDFACTORALPHA:
      ureg_LRP(ureg, dst, _WWWW(_CONST(20)), arg[1], arg[2]);
      case D3DTOP_BLENDTEXTUREALPHAPM:
      ureg_ADD(ureg, tmp_x, ureg_imm1f(ureg, 1.0f), ureg_negate(_W(ps->rTex)));
   ureg_MAD(ureg, dst, arg[2], _X(tmp), arg[1]);
      case D3DTOP_BLENDCURRENTALPHA:
      ureg_LRP(ureg, dst, _WWWW(ps->rCurSrc), arg[1], arg[2]);
      case D3DTOP_PREMODULATE:
      ureg_MOV(ureg, dst, arg[1]);
   ps->stage.index_pre_mod = ps->stage.index + 1;
      case D3DTOP_MODULATEALPHA_ADDCOLOR:
      ureg_MAD(ureg, dst, _WWWW(arg[1]), arg[2], arg[1]);
      case D3DTOP_MODULATECOLOR_ADDALPHA:
      ureg_MAD(ureg, dst, arg[1], arg[2], _WWWW(arg[1]));
      case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
      ureg_ADD(ureg, tmp_x, ureg_imm1f(ureg, 1.0f), ureg_negate(_WWWW(arg[1])));
   ureg_MAD(ureg, dst, _X(tmp), arg[2], arg[1]);
      case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
      ureg_ADD(ureg, tmp, ureg_imm1f(ureg, 1.0f), ureg_negate(arg[1]));
   ureg_MAD(ureg, dst, ureg_src(tmp), arg[2], _WWWW(arg[1]));
      case D3DTOP_BUMPENVMAP:
         case D3DTOP_BUMPENVMAPLUMINANCE:
         case D3DTOP_DOTPRODUCT3:
      ureg_ADD(ureg, tmp, arg[1], ureg_imm4f(ureg,-0.5,-0.5,-0.5,-0.5));
   ureg_ADD(ureg, tmp2, arg[2] , ureg_imm4f(ureg,-0.5,-0.5,-0.5,-0.5));
   ureg_DP3(ureg, tmp, ureg_src(tmp), ureg_src(tmp2));
   ureg_MUL(ureg, ureg_saturate(dst), ureg_src(tmp), ureg_imm4f(ureg,4.0,4.0,4.0,4.0));
      case D3DTOP_MULTIPLYADD:
      ureg_MAD(ureg, dst, arg[1], arg[2], arg[0]);
      case D3DTOP_LERP:
      ureg_LRP(ureg, dst, arg[0], arg[1], arg[2]);
      case D3DTOP_DISABLE:
      /* no-op ? */
      default:
      assert(!"invalid D3DTOP");
      }
   ureg_release_temporary(ureg, tmp);
      }
      static void *
   nine_ff_build_ps(struct NineDevice9 *device, struct nine_ff_ps_key *key)
   {
      struct ps_build_ctx ps;
   struct ureg_program *ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   struct ureg_dst oCol;
   unsigned s;
            memset(&ps, 0, sizeof(ps));
   ps.ureg = ureg;
   ps.color_interpolate_flag = key->flatshade ? TGSI_INTERPOLATE_CONSTANT : TGSI_INTERPOLATE_PERSPECTIVE;
                     ps.rCur = ureg_DECL_temporary(ureg);
   ps.rTmp = ureg_DECL_temporary(ureg);
   ps.rTex = ureg_DECL_temporary(ureg);
   ps.rCurSrc = ureg_src(ps.rCur);
   ps.rTmpSrc = ureg_src(ps.rTmp);
            /* Initial values */
   ureg_MOV(ureg, ps.rCur, ps.vC[0]);
   ureg_MOV(ureg, ps.rTmp, ureg_imm1f(ureg, 0.0f));
            for (s = 0; s < 8; ++s) {
               if (key->ts[s].colorop != D3DTOP_DISABLE) {
         if (key->ts[s].colorarg0 == D3DTA_SPECULAR ||
                        if (key->ts[s].colorarg0 == D3DTA_TEXTURE ||
      key->ts[s].colorarg1 == D3DTA_TEXTURE ||
   key->ts[s].colorarg2 == D3DTA_TEXTURE ||
   key->ts[s].colorop == D3DTOP_BLENDTEXTUREALPHA ||
   key->ts[s].colorop == D3DTOP_BLENDTEXTUREALPHAPM) {
   ps.s[s] = ureg_DECL_sampler(ureg, s);
      }
   if (s && (key->ts[s - 1].colorop == D3DTOP_PREMODULATE ||
                  if (key->ts[s].alphaop != D3DTOP_DISABLE) {
         if (key->ts[s].alphaarg0 == D3DTA_SPECULAR ||
                        if (key->ts[s].alphaarg0 == D3DTA_TEXTURE ||
      key->ts[s].alphaarg1 == D3DTA_TEXTURE ||
   key->ts[s].alphaarg2 == D3DTA_TEXTURE ||
   key->ts[s].colorop == D3DTOP_BLENDTEXTUREALPHA ||
   key->ts[s].colorop == D3DTOP_BLENDTEXTUREALPHAPM) {
   ps.s[s] = ureg_DECL_sampler(ureg, s);
         }
   if (key->specular)
                     /* Run stages.
   */
   for (s = 0; s < 8; ++s) {
      unsigned colorarg[3];
   unsigned alphaarg[3];
   const uint8_t used_c = ps_d3dtop_args_mask(key->ts[s].colorop);
   const uint8_t used_a = ps_d3dtop_args_mask(key->ts[s].alphaop);
   struct ureg_dst dst;
            if (key->ts[s].colorop == D3DTOP_DISABLE) {
         assert (key->ts[s].alphaop == D3DTOP_DISABLE);
   }
            DBG("STAGE[%u]: colorop=%s alphaop=%s\n", s,
                  if (!ureg_src_is_undef(ps.s[s])) {
         unsigned target;
   struct ureg_src texture_coord = ps.vT[s];
   struct ureg_dst delta;
   switch (key->ts[s].textarget) {
   case 0: target = TGSI_TEXTURE_1D; break;
   case 1: target = TGSI_TEXTURE_2D; break;
   case 2: target = TGSI_TEXTURE_3D; break;
   case 3: target = TGSI_TEXTURE_CUBE; break;
                  /* Modify coordinates */
   if (s >= 1 &&
      (key->ts[s-1].colorop == D3DTOP_BUMPENVMAP ||
   key->ts[s-1].colorop == D3DTOP_BUMPENVMAPLUMINANCE)) {
   delta = ureg_DECL_temporary(ureg);
   /* Du' = D3DTSS_BUMPENVMAT00(stage s-1)*t(s-1)R + D3DTSS_BUMPENVMAT10(stage s-1)*t(s-1)G */
   ureg_MUL(ureg, ureg_writemask(delta, TGSI_WRITEMASK_X), _X(ps.rTex), _XXXX(_CONST(8 + s - 1)));
   ureg_MAD(ureg, ureg_writemask(delta, TGSI_WRITEMASK_X), _Y(ps.rTex), _ZZZZ(_CONST(8 + s - 1)), ureg_src(delta));
   /* Dv' = D3DTSS_BUMPENVMAT01(stage s-1)*t(s-1)R + D3DTSS_BUMPENVMAT11(stage s-1)*t(s-1)G */
   ureg_MUL(ureg, ureg_writemask(delta, TGSI_WRITEMASK_Y), _X(ps.rTex), _YYYY(_CONST(8 + s - 1)));
   ureg_MAD(ureg, ureg_writemask(delta, TGSI_WRITEMASK_Y), _Y(ps.rTex), _WWWW(_CONST(8 + s - 1)), ureg_src(delta));
   texture_coord = ureg_src(ureg_DECL_temporary(ureg));
   ureg_MOV(ureg, ureg_writemask(ureg_dst(texture_coord), ureg_dst(ps.vT[s]).WriteMask), ps.vT[s]);
   ureg_ADD(ureg, ureg_writemask(ureg_dst(texture_coord), TGSI_WRITEMASK_XY), texture_coord, ureg_src(delta));
   /* Prepare luminance multiplier
   * t(s)RGBA = t(s)RGBA * clamp[(t(s-1)B * D3DTSS_BUMPENVLSCALE(stage s-1)) + D3DTSS_BUMPENVLOFFSET(stage s-1)] */
                                 }
   if (key->projected & (3 << (s *2))) {
      unsigned dim = 1 + ((key->projected >> (2 * s)) & 3);
   if (dim == 4)
         else {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   ureg_RCP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(texture_coord, dim-1));
   ureg_MUL(ureg, ps.rTmp, _X(tmp), texture_coord);
   ureg_TEX(ureg, ps.rTex, target, ps.rTmpSrc, ps.s[s]);
         } else {
         }
   if (s >= 1 && key->ts[s-1].colorop == D3DTOP_BUMPENVMAPLUMINANCE)
            if (key->ts[s].colorop == D3DTOP_BUMPENVMAP ||
                           if (ps.stage.index_pre_mod == ps.stage.index) {
         ps.rMod = ureg_DECL_temporary(ureg);
            colorarg[0] = (key->ts[s].colorarg0 | (((key->colorarg_b4[0] >> s) & 0x1) << 4) | ((key->colorarg_b5[0] >> s) << 5)) & 0x3f;
   colorarg[1] = (key->ts[s].colorarg1 | (((key->colorarg_b4[1] >> s) & 0x1) << 4) | ((key->colorarg_b5[1] >> s) << 5)) & 0x3f;
   colorarg[2] = (key->ts[s].colorarg2 | (((key->colorarg_b4[2] >> s) & 0x1) << 4) | ((key->colorarg_b5[2] >> s) << 5)) & 0x3f;
   alphaarg[0] = (key->ts[s].alphaarg0 | ((key->alphaarg_b4[0] >> s) << 4)) & 0x1f;
   alphaarg[1] = (key->ts[s].alphaarg1 | ((key->alphaarg_b4[1] >> s) << 4)) & 0x1f;
            if (key->ts[s].colorop != key->ts[s].alphaop ||
         colorarg[0] != alphaarg[0] ||
   colorarg[1] != alphaarg[1] ||
            /* Special DOTPRODUCT behaviour (see wine tests) */
   if (key->ts[s].colorop == D3DTOP_DOTPRODUCT3)
            if (used_c & 0x1) arg[0] = ps_get_ts_arg(&ps, colorarg[0]);
   if (used_c & 0x2) arg[1] = ps_get_ts_arg(&ps, colorarg[1]);
   if (used_c & 0x4) arg[2] = ps_get_ts_arg(&ps, colorarg[2]);
            if (dst.WriteMask != TGSI_WRITEMASK_XYZW) {
                  if (used_a & 0x1) arg[0] = ps_get_ts_arg(&ps, alphaarg[0]);
   if (used_a & 0x2) arg[1] = ps_get_ts_arg(&ps, alphaarg[1]);
   if (used_a & 0x4) arg[2] = ps_get_ts_arg(&ps, alphaarg[2]);
               if (key->specular)
            if (key->alpha_test_emulation == PIPE_FUNC_NEVER) {
         } else if (key->alpha_test_emulation != PIPE_FUNC_ALWAYS) {
      unsigned cmp_op;
   struct ureg_src src[2];
   struct ureg_dst tmp = ps.rTmp;
   cmp_op = pipe_comp_to_tgsi_opposite(key->alpha_test_emulation);
   src[0] = ureg_scalar(ps.rCurSrc, TGSI_SWIZZLE_W); /* Read color alpha channel */
   src[1] = _WWWW(_CONST(22)); /* Read alpha ref */
   ureg_insn(ureg, cmp_op, &tmp, 1, src, 2, 0);
               /* Fog.
   */
   if (key->fog_mode) {
      struct ureg_dst rFog = ureg_writemask(ps.rTmp, TGSI_WRITEMASK_X);
   struct ureg_src vPos;
   if (device->screen->get_param(device->screen,
               } else {
         vPos = ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_POSITION, 0,
            /* Source is either W or Z.
      * Z is when an orthogonal projection matrix is detected,
   * W (WFOG) else.
      if (!key->fog_source)
         else
                  if (key->fog_mode == D3DFOG_EXP) {
         ureg_MUL(ureg, rFog, _X(rFog), _ZZZZ(_CONST(22)));
   ureg_MUL(ureg, rFog, _X(rFog), ureg_imm1f(ureg, -1.442695f));
   } else
   if (key->fog_mode == D3DFOG_EXP2) {
         ureg_MUL(ureg, rFog, _X(rFog), _ZZZZ(_CONST(22)));
   ureg_MUL(ureg, rFog, _X(rFog), _X(rFog));
   ureg_MUL(ureg, rFog, _X(rFog), ureg_imm1f(ureg, -1.442695f));
   } else
   if (key->fog_mode == D3DFOG_LINEAR) {
         ureg_ADD(ureg, rFog, _XXXX(_CONST(22)), ureg_negate(_X(rFog)));
   }
   ureg_LRP(ureg, ureg_writemask(oCol, TGSI_WRITEMASK_XYZ), _X(rFog), ps.rCurSrc, _CONST(21));
      } else
   if (key->fog) {
      struct ureg_src vFog = ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_GENERIC, 16, TGSI_INTERPOLATE_PERSPECTIVE);
   ureg_LRP(ureg, ureg_writemask(oCol, TGSI_WRITEMASK_XYZ), _XXXX(vFog), ps.rCurSrc, _CONST(21));
      } else {
                  ureg_END(ureg);
   nine_ureg_tgsi_dump(ureg, false);
      }
      static struct NineVertexShader9 *
   nine_ff_get_vs(struct NineDevice9 *device)
   {
      const struct nine_context *context = &device->context;
   struct NineVertexShader9 *vs;
   struct vs_build_ctx bld;
   struct nine_ff_vs_key key;
   unsigned s, i;
   bool has_indexes = false;
   bool has_weights = false;
                     memset(&key, 0, sizeof(key));
   memset(&bld, 0, sizeof(bld));
                     /* FIXME: this shouldn't be NULL, but it is on init */
   if (context->vdecl) {
      key.color0in_one = 1;
   key.color1in_zero = 1;
   for (i = 0; i < context->vdecl->nelems; i++) {
         uint16_t usage = context->vdecl->usage_map[i];
   if (usage == NINE_DECLUSAGE_POSITIONT)
         else if (usage == NINE_DECLUSAGE_i(COLOR, 0))
         else if (usage == NINE_DECLUSAGE_i(COLOR, 1))
         else if (usage == NINE_DECLUSAGE_i(BLENDINDICES, 0)) {
      has_indexes = true;
      } else if (usage == NINE_DECLUSAGE_i(BLENDWEIGHT, 0)) {
      has_weights = true;
      } else if (usage == NINE_DECLUSAGE_i(NORMAL, 0)) {
      key.has_normal = 1;
      } else if (usage == NINE_DECLUSAGE_PSIZE)
         else if (usage % NINE_DECLUSAGE_COUNT == NINE_DECLUSAGE_TEXCOORD) {
      s = usage / NINE_DECLUSAGE_COUNT;
   if (s < 8)
         else
      } else if (usage < NINE_DECLUSAGE_NONE)
      }
   /* ff vs + ps 3.0: some elements are passed to the ps (wine test).
   * We do restrict to indices 0 */
   key.passthrough &= ~((1 << NINE_DECLUSAGE_POSITION) | (1 << NINE_DECLUSAGE_PSIZE) |
               if (!key.position_t)
                  key.lighting = !!context->rs[D3DRS_LIGHTING] &&  context->ff.num_lights_active;
   key.darkness = !!context->rs[D3DRS_LIGHTING] && !context->ff.num_lights_active;
   if (key.position_t) {
      key.darkness = 0; /* |= key.lighting; */ /* XXX ? */
      }
   if ((key.lighting | key.darkness) && context->rs[D3DRS_COLORVERTEX]) {
      uint32_t mask = (key.color0in_one ? 0 : 1) | (key.color1in_zero ? 0 : 2);
   key.mtl_diffuse = context->rs[D3DRS_DIFFUSEMATERIALSOURCE] & mask;
   key.mtl_ambient = context->rs[D3DRS_AMBIENTMATERIALSOURCE] & mask;
   key.mtl_specular = context->rs[D3DRS_SPECULARMATERIALSOURCE] & mask;
      }
   key.fog = !!context->rs[D3DRS_FOGENABLE];
   key.fog_mode = (!key.position_t && context->rs[D3DRS_FOGENABLE]) ? context->rs[D3DRS_FOGVERTEXMODE] : 0;
   if (key.fog_mode)
            key.localviewer = !!context->rs[D3DRS_LOCALVIEWER];
   key.normalizenormals = !!context->rs[D3DRS_NORMALIZENORMALS];
   key.ucp = !!context->rs[D3DRS_CLIPPLANEENABLE];
            if (context->rs[D3DRS_VERTEXBLEND] != D3DVBF_DISABLE) {
               switch (context->rs[D3DRS_VERTEXBLEND]) {
   case D3DVBF_0WEIGHTS: key.vertexblend = key.vertexblend_indexed; break;
   case D3DVBF_1WEIGHTS: key.vertexblend = 2; break;
   case D3DVBF_2WEIGHTS: key.vertexblend = 3; break;
   case D3DVBF_3WEIGHTS: key.vertexblend = 4; break;
   case D3DVBF_TWEENING: key.vertextween = 1; break;
   default:
         assert(!"invalid D3DVBF");
   }
   if (!has_weights && context->rs[D3DRS_VERTEXBLEND] != D3DVBF_0WEIGHTS)
               for (s = 0; s < 8; ++s) {
      unsigned gen = (context->ff.tex_stage[s][D3DTSS_TEXCOORDINDEX] >> 16) + 1;
   unsigned idx = context->ff.tex_stage[s][D3DTSS_TEXCOORDINDEX] & 7;
            if (key.position_t && gen > NINED3DTSS_TCI_PASSTHRU)
            if (!input_texture_coord[idx] && gen == NINED3DTSS_TCI_PASSTHRU)
            key.tc_gen |= gen << (s * 3);
   key.tc_idx |= idx << (s * 3);
            dim = context->ff.tex_stage[s][D3DTSS_TEXTURETRANSFORMFLAGS] & 0x7;
   if (dim > 4)
         if (dim == 1) /* NV behaviour */
                     DBG("VS ff key hash: %x\n", nine_ff_vs_key_hash(&key));
   vs = util_hash_table_get(device->ff.ht_vs, &key);
   if (vs)
                  nine_ff_prune_vs(device);
   if (vs) {
                        _mesa_hash_table_insert(device->ff.ht_vs, &vs->ff_key, vs);
            vs->num_inputs = bld.num_inputs;
   for (n = 0; n < bld.num_inputs; ++n)
            vs->position_t = key.position_t;
      }
      }
      #define GET_D3DTS(n) nine_state_access_transform(&context->ff, D3DTS_##n, FALSE)
   #define IS_D3DTS_DIRTY(s,n) ((s)->ff.changed.transform[(D3DTS_##n) / 32] & (1 << ((D3DTS_##n) % 32)))
      static struct NinePixelShader9 *
   nine_ff_get_ps(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
   struct NinePixelShader9 *ps;
   struct nine_ff_ps_key key;
   unsigned s;
                     memset(&key, 0, sizeof(key));
   for (s = 0; s < 8; ++s) {
      key.ts[s].colorop = context->ff.tex_stage[s][D3DTSS_COLOROP];
   key.ts[s].alphaop = context->ff.tex_stage[s][D3DTSS_ALPHAOP];
   const uint8_t used_c = ps_d3dtop_args_mask(key.ts[s].colorop);
   const uint8_t used_a = ps_d3dtop_args_mask(key.ts[s].alphaop);
   /* MSDN says D3DTOP_DISABLE disables this and all subsequent stages.
      * ALPHAOP cannot be enabled if COLOROP is disabled.
      if (key.ts[s].colorop == D3DTOP_DISABLE) {
         key.ts[s].alphaop = D3DTOP_DISABLE; /* DISABLE == 1, avoid degenerate keys */
            if (!context->texture[s].enabled &&
         ((context->ff.tex_stage[s][D3DTSS_COLORARG0] == D3DTA_TEXTURE &&
   used_c & 0x1) ||
   (context->ff.tex_stage[s][D3DTSS_COLORARG1] == D3DTA_TEXTURE &&
   used_c & 0x2) ||
   (context->ff.tex_stage[s][D3DTSS_COLORARG2] == D3DTA_TEXTURE &&
   used_c & 0x4))) {
   /* Tested on Windows: Invalid texture read disables the stage
   * and the subsequent ones, but only for colorop. For alpha,
   * it's as if the texture had alpha of 1.0, which is what
   * has our dummy texture in that case. Invalid color also
   * disabled the following alpha stages. */
   key.ts[s].colorop = key.ts[s].alphaop = D3DTOP_DISABLE;
            if (context->ff.tex_stage[s][D3DTSS_COLORARG0] == D3DTA_TEXTURE ||
         context->ff.tex_stage[s][D3DTSS_COLORARG1] == D3DTA_TEXTURE ||
   context->ff.tex_stage[s][D3DTSS_COLORARG2] == D3DTA_TEXTURE ||
   context->ff.tex_stage[s][D3DTSS_ALPHAARG0] == D3DTA_TEXTURE ||
   context->ff.tex_stage[s][D3DTSS_ALPHAARG1] == D3DTA_TEXTURE ||
            if (key.ts[s].colorop != D3DTOP_DISABLE) {
         if (used_c & 0x1) key.ts[s].colorarg0 = context->ff.tex_stage[s][D3DTSS_COLORARG0] & 0x7;
   if (used_c & 0x2) key.ts[s].colorarg1 = context->ff.tex_stage[s][D3DTSS_COLORARG1] & 0x7;
   if (used_c & 0x4) key.ts[s].colorarg2 = context->ff.tex_stage[s][D3DTSS_COLORARG2] & 0x7;
   if (used_c & 0x1) key.colorarg_b4[0] |= ((context->ff.tex_stage[s][D3DTSS_COLORARG0] >> 4) & 0x1) << s;
   if (used_c & 0x1) key.colorarg_b5[0] |= ((context->ff.tex_stage[s][D3DTSS_COLORARG0] >> 5) & 0x1) << s;
   if (used_c & 0x2) key.colorarg_b4[1] |= ((context->ff.tex_stage[s][D3DTSS_COLORARG1] >> 4) & 0x1) << s;
   if (used_c & 0x2) key.colorarg_b5[1] |= ((context->ff.tex_stage[s][D3DTSS_COLORARG1] >> 5) & 0x1) << s;
   if (used_c & 0x4) key.colorarg_b4[2] |= ((context->ff.tex_stage[s][D3DTSS_COLORARG2] >> 4) & 0x1) << s;
   }
   if (key.ts[s].alphaop != D3DTOP_DISABLE) {
         if (used_a & 0x1) key.ts[s].alphaarg0 = context->ff.tex_stage[s][D3DTSS_ALPHAARG0] & 0x7;
   if (used_a & 0x2) key.ts[s].alphaarg1 = context->ff.tex_stage[s][D3DTSS_ALPHAARG1] & 0x7;
   if (used_a & 0x4) key.ts[s].alphaarg2 = context->ff.tex_stage[s][D3DTSS_ALPHAARG2] & 0x7;
   if (used_a & 0x1) key.alphaarg_b4[0] |= ((context->ff.tex_stage[s][D3DTSS_ALPHAARG0] >> 4) & 0x1) << s;
   if (used_a & 0x2) key.alphaarg_b4[1] |= ((context->ff.tex_stage[s][D3DTSS_ALPHAARG1] >> 4) & 0x1) << s;
   }
            if (context->texture[s].enabled) {
         switch (context->texture[s].type) {
   case D3DRTYPE_TEXTURE:       key.ts[s].textarget = 1; break;
   case D3DRTYPE_VOLUMETEXTURE: key.ts[s].textarget = 2; break;
   case D3DRTYPE_CUBETEXTURE:   key.ts[s].textarget = 3; break;
   default:
      assert(!"unexpected texture type");
      } else {
                     /* Note: If colorop is D3DTOP_DISABLE for the first stage
   * (which implies alphaop is too), nothing particular happens,
   * that is, current is equal to diffuse (which is the case anyway,
   * because it is how it is initialized).
   * Special case seems if alphaop is D3DTOP_DISABLE and not colorop,
   * because then if the resultarg is TEMP, then diffuse alpha is written
   * to it. */
   if (key.ts[0].colorop != D3DTOP_DISABLE &&
      key.ts[0].alphaop == D3DTOP_DISABLE &&
   key.ts[0].resultarg != 0) {
   key.ts[0].alphaop = D3DTOP_SELECTARG1;
      }
   /* When no alpha stage writes to current, diffuse alpha is taken.
            /* Last stage always writes to Current */
   if (s >= 1)
            key.projected = nine_ff_get_projected_key_ff(context);
   key.specular = !!context->rs[D3DRS_SPECULARENABLE];
            for (; s < 8; ++s)
         if (context->rs[D3DRS_FOGENABLE])
         key.fog = !!context->rs[D3DRS_FOGENABLE];
   if (key.fog_mode && key.fog)
                  DBG("PS ff key hash: %x\n", nine_ff_ps_key_hash(&key));
   ps = util_hash_table_get(device->ff.ht_ps, &key);
   if (ps)
                  nine_ff_prune_ps(device);
   if (ps) {
               _mesa_hash_table_insert(device->ff.ht_ps, &ps->ff_key, ps);
            ps->rt_mask = 0x1;
      }
      }
      static void
   nine_ff_load_vs_transforms(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
   D3DMATRIX T;
   D3DMATRIX *M = (D3DMATRIX *)device->ff.vs_const;
            /* TODO: make this nicer, and only upload the ones we need */
            if (IS_D3DTS_DIRTY(context, WORLD) ||
      IS_D3DTS_DIRTY(context, VIEW) ||
   IS_D3DTS_DIRTY(context, PROJECTION)) {
   /* WVP, WV matrices */
   nine_d3d_matrix_matrix_mul(&M[1], GET_D3DTS(WORLD), GET_D3DTS(VIEW));
            /* normal matrix == transpose(inverse(WV)) */
   nine_d3d_matrix_inverse(&T, &M[1]);
            /* P matrix */
            /* V and W matrix */
   nine_d3d_matrix_inverse(&M[3], GET_D3DTS(VIEW));
               if (context->rs[D3DRS_VERTEXBLEND] != D3DVBF_DISABLE) {
      /* load other world matrices */
   for (i = 1; i <= 8; ++i) {
                        }
      static void
   nine_ff_load_lights(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
   struct fvec4 *dst = (struct fvec4 *)device->ff.vs_const;
            if (context->changed.group & NINE_STATE_FF_MATERIAL) {
               memcpy(&dst[20], &mtl->Diffuse, 4 * sizeof(float));
   memcpy(&dst[21], &mtl->Ambient, 4 * sizeof(float));
   memcpy(&dst[22], &mtl->Specular, 4 * sizeof(float));
   dst[23].x = mtl->Power;
   memcpy(&dst[24], &mtl->Emissive, 4 * sizeof(float));
   d3dcolor_to_rgba(&dst[25].x, context->rs[D3DRS_AMBIENT]);
   dst[19].x = dst[25].x * mtl->Ambient.r + mtl->Emissive.r;
   dst[19].y = dst[25].y * mtl->Ambient.g + mtl->Emissive.g;
               if (!(context->changed.group & NINE_STATE_FF_LIGHTING))
            for (l = 0; l < context->ff.num_lights_active; ++l) {
               dst[32 + l * 8].x = light->Type;
   dst[32 + l * 8].y = light->Attenuation0;
   dst[32 + l * 8].z = light->Attenuation1;
   dst[32 + l * 8].w = light->Attenuation2;
   memcpy(&dst[33 + l * 8].x, &light->Diffuse, sizeof(light->Diffuse));
   memcpy(&dst[34 + l * 8].x, &light->Specular, sizeof(light->Specular));
   memcpy(&dst[35 + l * 8].x, &light->Ambient, sizeof(light->Ambient));
   nine_d3d_vector4_matrix_mul((D3DVECTOR *)&dst[36 + l * 8].x, &light->Position, GET_D3DTS(VIEW));
   nine_d3d_vector3_matrix_mul((D3DVECTOR *)&dst[37 + l * 8].x, &light->Direction, GET_D3DTS(VIEW));
   dst[36 + l * 8].w = light->Type == D3DLIGHT_DIRECTIONAL ? 1e9f : light->Range;
   dst[37 + l * 8].w = light->Falloff;
   dst[38 + l * 8].x = cosf(light->Theta * 0.5f);
   dst[38 + l * 8].y = cosf(light->Phi * 0.5f);
   dst[38 + l * 8].z = 1.0f / (dst[38 + l * 8].x - dst[38 + l * 8].y);
         }
      static void
   nine_ff_load_point_and_fog_params(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
            if (!(context->changed.group & NINE_STATE_FF_VS_OTHER))
         dst[26].x = asfloat(context->rs[D3DRS_POINTSIZE_MIN]);
   dst[26].y = asfloat(context->rs[D3DRS_POINTSIZE_MAX]);
   dst[26].z = CLAMP(asfloat(context->rs[D3DRS_POINTSIZE]),
               dst[26].w = asfloat(context->rs[D3DRS_POINTSCALE_A]);
   dst[27].x = asfloat(context->rs[D3DRS_POINTSCALE_B]);
   dst[27].y = asfloat(context->rs[D3DRS_POINTSCALE_C]);
   dst[28].x = asfloat(context->rs[D3DRS_FOGEND]);
   dst[28].y = 1.0f / (asfloat(context->rs[D3DRS_FOGEND]) - asfloat(context->rs[D3DRS_FOGSTART]));
   if (isinf(dst[28].y))
         dst[28].z = asfloat(context->rs[D3DRS_FOGDENSITY]);
   if (device->driver_caps.emulate_ucp)
      }
      static void
   nine_ff_load_tex_matrices(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
   D3DMATRIX *M = (D3DMATRIX *)device->ff.vs_const;
            if (!(context->ff.changed.transform[0] & 0xff0000))
         for (s = 0; s < 8; ++s) {
      if (IS_D3DTS_DIRTY(context, TEXTURE0 + s))
         }
      static void
   nine_ff_load_ps_params(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
   struct fvec4 *dst = (struct fvec4 *)device->ff.ps_const;
            if (!(context->changed.group & NINE_STATE_FF_PS_CONSTS))
            for (s = 0; s < 8; ++s)
            for (s = 0; s < 8; ++s) {
      dst[8 + s].x = asfloat(context->ff.tex_stage[s][D3DTSS_BUMPENVMAT00]);
   dst[8 + s].y = asfloat(context->ff.tex_stage[s][D3DTSS_BUMPENVMAT01]);
   dst[8 + s].z = asfloat(context->ff.tex_stage[s][D3DTSS_BUMPENVMAT10]);
   dst[8 + s].w = asfloat(context->ff.tex_stage[s][D3DTSS_BUMPENVMAT11]);
   if (s & 1) {
         dst[16 + s / 2].z = asfloat(context->ff.tex_stage[s][D3DTSS_BUMPENVLSCALE]);
   } else {
         dst[16 + s / 2].x = asfloat(context->ff.tex_stage[s][D3DTSS_BUMPENVLSCALE]);
               d3dcolor_to_rgba(&dst[20].x, context->rs[D3DRS_TEXTUREFACTOR]);
   d3dcolor_to_rgba(&dst[21].x, context->rs[D3DRS_FOGCOLOR]);
   dst[22].x = asfloat(context->rs[D3DRS_FOGEND]);
   dst[22].y = 1.0f / (asfloat(context->rs[D3DRS_FOGEND]) - asfloat(context->rs[D3DRS_FOGSTART]));
   dst[22].z = asfloat(context->rs[D3DRS_FOGDENSITY]);
      }
      static void
   nine_ff_load_viewport_info(struct NineDevice9 *device)
   {
      D3DVIEWPORT9 *viewport = &device->context.viewport;
   struct fvec4 *dst = (struct fvec4 *)device->ff.vs_const;
            /* Note: the other functions avoids to fill the const again if nothing changed.
   * But we don't have much to fill, and adding code to allow that may be complex
   * so just fill it always */
   dst[100].x = 2.0f / (float)(viewport->Width);
   dst[100].y = 2.0f / (float)(viewport->Height);
   dst[100].z = (diffZ == 0.0f) ? 0.0f : (1.0f / diffZ);
   dst[100].w = (float)(viewport->Width);
   dst[101].x = (float)(viewport->X);
   dst[101].y = (float)(viewport->Y);
      }
      void
   nine_ff_update(struct NineDevice9 *device)
   {
      struct nine_context *context = &device->context;
                     /* NOTE: the only reference belongs to the hash table */
   if (!context->programmable_vs) {
      device->ff.vs = nine_ff_get_vs(device);
      }
   if (!context->ps) {
      device->ff.ps = nine_ff_get_ps(device);
               if (!context->programmable_vs) {
      nine_ff_load_vs_transforms(device);
   nine_ff_load_tex_matrices(device);
   nine_ff_load_lights(device);
   nine_ff_load_point_and_fog_params(device);
                     cb.buffer_offset = 0;
   cb.buffer = NULL;
   cb.user_buffer = device->ff.vs_const;
            context->pipe_data.cb_vs_ff = cb;
                        if (!context->ps) {
               cb.buffer_offset = 0;
   cb.buffer = NULL;
   cb.user_buffer = device->ff.ps_const;
            context->pipe_data.cb_ps_ff = cb;
                  }
         bool
   nine_ff_init(struct NineDevice9 *device)
   {
      device->ff.ht_vs = _mesa_hash_table_create(NULL, nine_ff_vs_key_hash,
         device->ff.ht_ps = _mesa_hash_table_create(NULL, nine_ff_ps_key_hash,
            device->ff.ht_fvf = _mesa_hash_table_create(NULL, nine_ff_fvf_key_hash,
            device->ff.vs_const = CALLOC(NINE_FF_NUM_VS_CONST, 4 * sizeof(float));
            return device->ff.ht_vs && device->ff.ht_ps &&
      device->ff.ht_fvf &&
   }
      static enum pipe_error nine_ff_ht_delete_cb(void *key, void *value, void *data)
   {
      NineUnknown_Unbind(NineUnknown(value));
      }
      void
   nine_ff_fini(struct NineDevice9 *device)
   {
      if (device->ff.ht_vs) {
      util_hash_table_foreach(device->ff.ht_vs, nine_ff_ht_delete_cb, NULL);
      }
   if (device->ff.ht_ps) {
      util_hash_table_foreach(device->ff.ht_ps, nine_ff_ht_delete_cb, NULL);
      }
   if (device->ff.ht_fvf) {
      util_hash_table_foreach(device->ff.ht_fvf, nine_ff_ht_delete_cb, NULL);
      }
   device->ff.vs = NULL; /* destroyed by unbinding from hash table */
            FREE(device->ff.vs_const);
      }
      static void
   nine_ff_prune_vs(struct NineDevice9 *device)
   {
               if (device->ff.num_vs > 1024) {
      /* could destroy the bound one here, so unbind */
   context->pipe->bind_vs_state(context->pipe, NULL);
   util_hash_table_foreach(device->ff.ht_vs, nine_ff_ht_delete_cb, NULL);
   _mesa_hash_table_clear(device->ff.ht_vs, NULL);
   device->ff.num_vs = 0;
         }
   static void
   nine_ff_prune_ps(struct NineDevice9 *device)
   {
               if (device->ff.num_ps > 1024) {
      /* could destroy the bound one here, so unbind */
   context->pipe->bind_fs_state(context->pipe, NULL);
   util_hash_table_foreach(device->ff.ht_ps, nine_ff_ht_delete_cb, NULL);
   _mesa_hash_table_clear(device->ff.ht_ps, NULL);
   device->ff.num_ps = 0;
         }
      /* ========================================================================== */
      /* Matrix multiplication:
   *
   * in memory: 0 1 2 3 (row major)
   *            4 5 6 7
   *            8 9 a b
   *            c d e f
   *
   *    cA cB cC cD
   * r0             = (r0 * cA) (r0 * cB) . .
   * r1             = (r1 * cA) (r1 * cB)
   * r2             = (r2 * cA) .
   * r3             = (r3 * cA) .
   *
   *               r: (11) (12) (13) (14)
   *                  (21) (22) (23) (24)
   *                  (31) (32) (33) (34)
   *                  (41) (42) (43) (44)
   * l: (11 12 13 14)
   *    (21 22 23 24)
   *    (31 32 33 34)
   *    (41 42 43 44)
   *
   * v: (x  y  z  1 )
   *
   * t.xyzw = MUL(v.xxxx, r[0]);
   * t.xyzw = MAD(v.yyyy, r[1], t.xyzw);
   * t.xyzw = MAD(v.zzzz, r[2], t.xyzw);
   * v.xyzw = MAD(v.wwww, r[3], t.xyzw);
   *
   * v.x = DP4(v, c[0]);
   * v.y = DP4(v, c[1]);
   * v.z = DP4(v, c[2]);
   * v.w = DP4(v, c[3]) = 1
   */
      /*
   static void
   nine_D3DMATRIX_print(const D3DMATRIX *M)
   {
      DBG("\n(%f %f %f %f)\n"
      "(%f %f %f %f)\n"
   "(%f %f %f %f)\n"
   "(%f %f %f %f)\n",
   M->m[0][0], M->m[0][1], M->m[0][2], M->m[0][3],
   M->m[1][0], M->m[1][1], M->m[1][2], M->m[1][3],
   M->m[2][0], M->m[2][1], M->m[2][2], M->m[2][3],
   }
   */
      static inline float
   nine_DP4_row_col(const D3DMATRIX *A, int r, const D3DMATRIX *B, int c)
   {
      return A->m[r][0] * B->m[0][c] +
         A->m[r][1] * B->m[1][c] +
      }
      static inline float
   nine_DP4_vec_col(const D3DVECTOR *v, const D3DMATRIX *M, int c)
   {
      return v->x * M->m[0][c] +
         v->y * M->m[1][c] +
      }
      static inline float
   nine_DP3_vec_col(const D3DVECTOR *v, const D3DMATRIX *M, int c)
   {
      return v->x * M->m[0][c] +
            }
      void
   nine_d3d_matrix_matrix_mul(D3DMATRIX *D, const D3DMATRIX *L, const D3DMATRIX *R)
   {
      D->_11 = nine_DP4_row_col(L, 0, R, 0);
   D->_12 = nine_DP4_row_col(L, 0, R, 1);
   D->_13 = nine_DP4_row_col(L, 0, R, 2);
            D->_21 = nine_DP4_row_col(L, 1, R, 0);
   D->_22 = nine_DP4_row_col(L, 1, R, 1);
   D->_23 = nine_DP4_row_col(L, 1, R, 2);
            D->_31 = nine_DP4_row_col(L, 2, R, 0);
   D->_32 = nine_DP4_row_col(L, 2, R, 1);
   D->_33 = nine_DP4_row_col(L, 2, R, 2);
            D->_41 = nine_DP4_row_col(L, 3, R, 0);
   D->_42 = nine_DP4_row_col(L, 3, R, 1);
   D->_43 = nine_DP4_row_col(L, 3, R, 2);
      }
      void
   nine_d3d_vector4_matrix_mul(D3DVECTOR *d, const D3DVECTOR *v, const D3DMATRIX *M)
   {
      d->x = nine_DP4_vec_col(v, M, 0);
   d->y = nine_DP4_vec_col(v, M, 1);
      }
      void
   nine_d3d_vector3_matrix_mul(D3DVECTOR *d, const D3DVECTOR *v, const D3DMATRIX *M)
   {
      d->x = nine_DP3_vec_col(v, M, 0);
   d->y = nine_DP3_vec_col(v, M, 1);
      }
      void
   nine_d3d_matrix_transpose(D3DMATRIX *D, const D3DMATRIX *M)
   {
      unsigned i, j;
   for (i = 0; i < 4; ++i)
   for (j = 0; j < 4; ++j)
      }
      #define _M_ADD_PROD_1i_2j_3k_4l(i,j,k,l) do {            \
      float t = M->_1##i * M->_2##j * M->_3##k * M->_4##l; \
         #define _M_SUB_PROD_1i_2j_3k_4l(i,j,k,l) do {            \
      float t = M->_1##i * M->_2##j * M->_3##k * M->_4##l; \
      float
   nine_d3d_matrix_det(const D3DMATRIX *M)
   {
      float pos = 0.0f;
            _M_ADD_PROD_1i_2j_3k_4l(1, 2, 3, 4);
   _M_ADD_PROD_1i_2j_3k_4l(1, 3, 4, 2);
            _M_ADD_PROD_1i_2j_3k_4l(2, 1, 4, 3);
   _M_ADD_PROD_1i_2j_3k_4l(2, 3, 1, 4);
            _M_ADD_PROD_1i_2j_3k_4l(3, 1, 2, 4);
   _M_ADD_PROD_1i_2j_3k_4l(3, 2, 4, 1);
            _M_ADD_PROD_1i_2j_3k_4l(4, 1, 3, 2);
   _M_ADD_PROD_1i_2j_3k_4l(4, 2, 1, 3);
            _M_SUB_PROD_1i_2j_3k_4l(1, 2, 4, 3);
   _M_SUB_PROD_1i_2j_3k_4l(1, 3, 2, 4);
            _M_SUB_PROD_1i_2j_3k_4l(2, 1, 3, 4);
   _M_SUB_PROD_1i_2j_3k_4l(2, 3, 4, 1);
            _M_SUB_PROD_1i_2j_3k_4l(3, 1, 4, 2);
   _M_SUB_PROD_1i_2j_3k_4l(3, 2, 1, 4);
            _M_SUB_PROD_1i_2j_3k_4l(4, 1, 2, 3);
   _M_SUB_PROD_1i_2j_3k_4l(4, 2, 3, 1);
               }
      /* XXX: Probably better to just use src/mesa/math/m_matrix.c because
   * I have no idea where this code came from.
   */
   void
   nine_d3d_matrix_inverse(D3DMATRIX *D, const D3DMATRIX *M)
   {
      int i, k;
            D->m[0][0] =
      M->m[1][1] * M->m[2][2] * M->m[3][3] -
   M->m[1][1] * M->m[3][2] * M->m[2][3] -
   M->m[1][2] * M->m[2][1] * M->m[3][3] +
   M->m[1][2] * M->m[3][1] * M->m[2][3] +
   M->m[1][3] * M->m[2][1] * M->m[3][2] -
         D->m[0][1] =
      -M->m[0][1] * M->m[2][2] * M->m[3][3] +
   M->m[0][1] * M->m[3][2] * M->m[2][3] +
   M->m[0][2] * M->m[2][1] * M->m[3][3] -
   M->m[0][2] * M->m[3][1] * M->m[2][3] -
   M->m[0][3] * M->m[2][1] * M->m[3][2] +
         D->m[0][2] =
      M->m[0][1] * M->m[1][2] * M->m[3][3] -
   M->m[0][1] * M->m[3][2] * M->m[1][3] -
   M->m[0][2] * M->m[1][1] * M->m[3][3] +
   M->m[0][2] * M->m[3][1] * M->m[1][3] +
   M->m[0][3] * M->m[1][1] * M->m[3][2] -
         D->m[0][3] =
      -M->m[0][1] * M->m[1][2] * M->m[2][3] +
   M->m[0][1] * M->m[2][2] * M->m[1][3] +
   M->m[0][2] * M->m[1][1] * M->m[2][3] -
   M->m[0][2] * M->m[2][1] * M->m[1][3] -
   M->m[0][3] * M->m[1][1] * M->m[2][2] +
         D->m[1][0] =
      -M->m[1][0] * M->m[2][2] * M->m[3][3] +
   M->m[1][0] * M->m[3][2] * M->m[2][3] +
   M->m[1][2] * M->m[2][0] * M->m[3][3] -
   M->m[1][2] * M->m[3][0] * M->m[2][3] -
   M->m[1][3] * M->m[2][0] * M->m[3][2] +
         D->m[1][1] =
      M->m[0][0] * M->m[2][2] * M->m[3][3] -
   M->m[0][0] * M->m[3][2] * M->m[2][3] -
   M->m[0][2] * M->m[2][0] * M->m[3][3] +
   M->m[0][2] * M->m[3][0] * M->m[2][3] +
   M->m[0][3] * M->m[2][0] * M->m[3][2] -
         D->m[1][2] =
      -M->m[0][0] * M->m[1][2] * M->m[3][3] +
   M->m[0][0] * M->m[3][2] * M->m[1][3] +
   M->m[0][2] * M->m[1][0] * M->m[3][3] -
   M->m[0][2] * M->m[3][0] * M->m[1][3] -
   M->m[0][3] * M->m[1][0] * M->m[3][2] +
         D->m[1][3] =
      M->m[0][0] * M->m[1][2] * M->m[2][3] -
   M->m[0][0] * M->m[2][2] * M->m[1][3] -
   M->m[0][2] * M->m[1][0] * M->m[2][3] +
   M->m[0][2] * M->m[2][0] * M->m[1][3] +
   M->m[0][3] * M->m[1][0] * M->m[2][2] -
         D->m[2][0] =
      M->m[1][0] * M->m[2][1] * M->m[3][3] -
   M->m[1][0] * M->m[3][1] * M->m[2][3] -
   M->m[1][1] * M->m[2][0] * M->m[3][3] +
   M->m[1][1] * M->m[3][0] * M->m[2][3] +
   M->m[1][3] * M->m[2][0] * M->m[3][1] -
         D->m[2][1] =
      -M->m[0][0] * M->m[2][1] * M->m[3][3] +
   M->m[0][0] * M->m[3][1] * M->m[2][3] +
   M->m[0][1] * M->m[2][0] * M->m[3][3] -
   M->m[0][1] * M->m[3][0] * M->m[2][3] -
   M->m[0][3] * M->m[2][0] * M->m[3][1] +
         D->m[2][2] =
      M->m[0][0] * M->m[1][1] * M->m[3][3] -
   M->m[0][0] * M->m[3][1] * M->m[1][3] -
   M->m[0][1] * M->m[1][0] * M->m[3][3] +
   M->m[0][1] * M->m[3][0] * M->m[1][3] +
   M->m[0][3] * M->m[1][0] * M->m[3][1] -
         D->m[2][3] =
      -M->m[0][0] * M->m[1][1] * M->m[2][3] +
   M->m[0][0] * M->m[2][1] * M->m[1][3] +
   M->m[0][1] * M->m[1][0] * M->m[2][3] -
   M->m[0][1] * M->m[2][0] * M->m[1][3] -
   M->m[0][3] * M->m[1][0] * M->m[2][1] +
         D->m[3][0] =
      -M->m[1][0] * M->m[2][1] * M->m[3][2] +
   M->m[1][0] * M->m[3][1] * M->m[2][2] +
   M->m[1][1] * M->m[2][0] * M->m[3][2] -
   M->m[1][1] * M->m[3][0] * M->m[2][2] -
   M->m[1][2] * M->m[2][0] * M->m[3][1] +
         D->m[3][1] =
      M->m[0][0] * M->m[2][1] * M->m[3][2] -
   M->m[0][0] * M->m[3][1] * M->m[2][2] -
   M->m[0][1] * M->m[2][0] * M->m[3][2] +
   M->m[0][1] * M->m[3][0] * M->m[2][2] +
   M->m[0][2] * M->m[2][0] * M->m[3][1] -
         D->m[3][2] =
      -M->m[0][0] * M->m[1][1] * M->m[3][2] +
   M->m[0][0] * M->m[3][1] * M->m[1][2] +
   M->m[0][1] * M->m[1][0] * M->m[3][2] -
   M->m[0][1] * M->m[3][0] * M->m[1][2] -
   M->m[0][2] * M->m[1][0] * M->m[3][1] +
         D->m[3][3] =
      M->m[0][0] * M->m[1][1] * M->m[2][2] -
   M->m[0][0] * M->m[2][1] * M->m[1][2] -
   M->m[0][1] * M->m[1][0] * M->m[2][2] +
   M->m[0][1] * M->m[2][0] * M->m[1][2] +
   M->m[0][2] * M->m[1][0] * M->m[2][1] -
         det =
      M->m[0][0] * D->m[0][0] +
   M->m[1][0] * D->m[0][1] +
   M->m[2][0] * D->m[0][2] +
         if (fabsf(det) < 1e-30) {/* non inversible */
      *D = *M; /* wine tests */
                        for (i = 0; i < 4; i++)
   for (k = 0; k < 4; k++)
         #if defined(DEBUG) || !defined(NDEBUG)
      {
                        for (i = 0; i < 4; ++i)
   for (k = 0; k < 4; ++k)
               #endif
   }
