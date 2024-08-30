   #include <cstdint>
   #include <cstdio>
   #include <cstdlib>
   #include <array>
   #include <memory>
      #include <gtest/gtest.h>
      #include "GL/osmesa.h"
   #include "util/macros.h"
   #include "util/u_endian.h"
   #include "util/u_math.h"
      typedef struct {
      unsigned format;
   GLenum type;
   int bpp;
      } Params;
      class OSMesaRenderTestFixture : public testing::TestWithParam<Params> {};
      std::string
   name_params(const testing::TestParamInfo<Params> params) {
      auto p = params.param;
   std::string first, second;
   switch (p.format) {
   case OSMESA_RGBA:
      first = "rgba";
      case OSMESA_BGRA:
      first = "bgra";
      case OSMESA_RGB:
      first = "rgb";
      case OSMESA_RGB_565:
      first = "rgb_565";
      case OSMESA_ARGB:
      first = "argb";
               switch (p.type) {
   case GL_UNSIGNED_SHORT:
      second = "unsigned_short";
      case GL_UNSIGNED_BYTE:
      second = "unsigned_byte";
      case GL_FLOAT:
      second = "float";
      case GL_UNSIGNED_SHORT_5_6_5:
      second = "unsigned_short_565";
                  };
      TEST_P(OSMesaRenderTestFixture, Render)
   {
      auto p = GetParam();
   const int w = 2, h = 2;
            std::unique_ptr<osmesa_context, decltype(&OSMesaDestroyContext)> ctx{
                  auto ret = OSMesaMakeCurrent(ctx.get(), &pixels, p.type, w, h);
                              /* All the formats other than 565 and RGB/byte are array formats, but our
   * expected values are packed, so byte swap appropriately.
   */
   if (UTIL_ARCH_BIG_ENDIAN) {
      switch (p.bpp) {
   case 8:
                  case 4:
                  case 3:
   case 2:
                     glClear(GL_COLOR_BUFFER_BIT);
         #if 0 /* XXX */
      for (unsigned i = 0; i < ARRAY_SIZE(pixels); i += 4) {
      fprintf(stderr, "pixel %d: %02x %02x %02x %02x\n",
         i / 4,
   pixels[i + 0],
   pixels[i + 1],
         #endif
         for (unsigned i = 0; i < w * h; i++) {
      switch (p.bpp) {
   case 2: {
      uint16_t color = 0;
   memcpy(&color, &pixels[i * p.bpp], p.bpp);
   ASSERT_EQ(expected, color);
               case 3: {
      uint32_t color = ((pixels[i * p.bpp + 0] << 0) |
               ASSERT_EQ(expected, color);
               case 4: {
      uint32_t color = 0;
   memcpy(&color, &pixels[i * p.bpp], p.bpp);
   ASSERT_EQ(expected, color);
               case 8: {
      uint64_t color = 0;
   memcpy(&color, &pixels[i * p.bpp], p.bpp);
   ASSERT_EQ(expected, color);
               default:
               }
      INSTANTIATE_TEST_SUITE_P(
      OSMesaRenderTest,
   OSMesaRenderTestFixture,
   testing::Values(
      Params{ OSMESA_RGBA, GL_UNSIGNED_BYTE,  4, 0xbf80ff40 },
   Params{ OSMESA_BGRA, GL_UNSIGNED_BYTE,  4, 0xbf40ff80 },
   Params{ OSMESA_ARGB, GL_UNSIGNED_BYTE,  4, 0x80ff40bf},
   Params{ OSMESA_RGB,  GL_UNSIGNED_BYTE,  3, 0x80ff40 },
   Params{ OSMESA_RGBA, GL_UNSIGNED_SHORT, 8, 0xbfff8000ffff4000ull },
   Params{ OSMESA_RGB_565, GL_UNSIGNED_SHORT_5_6_5, 2, ((0x10 << 0) |
            ),
      );
      TEST(OSMesaRenderTest, depth)
   {
      std::unique_ptr<osmesa_context, decltype(&OSMesaDestroyContext)> ctx{
                  const int w = 3, h = 2;
   uint8_t pixels[4096 * h * 2] = {0}; /* different cpp from our depth! */
   auto ret = OSMesaMakeCurrent(ctx.get(), &pixels, GL_UNSIGNED_SHORT_5_6_5, w, h);
            /* Expand the row length for the color buffer so we can see that it doesn't affect depth. */
            uint32_t *depth;
   GLint dw, dh, depth_cpp;
            ASSERT_EQ(dw, w);
   ASSERT_EQ(dh, h);
            glClearDepth(1.0);
   glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
   glFinish();
   EXPECT_EQ(depth[w * 0 + 0], 0x00ffffff);
   EXPECT_EQ(depth[w * 0 + 1], 0x00ffffff);
   EXPECT_EQ(depth[w * 1 + 0], 0x00ffffff);
            /* Scissor to the top half and clear */
   glEnable(GL_SCISSOR_TEST);
   glScissor(0, 1, 2, 1);
   glClearDepth(0.0);
   glClear(GL_DEPTH_BUFFER_BIT);
   glFinish();
   EXPECT_EQ(depth[w * 0 + 0], 0x00ffffff);
   EXPECT_EQ(depth[w * 0 + 1], 0x00ffffff);
   EXPECT_EQ(depth[w * 1 + 0], 0x00000000);
            /* Y_UP didn't affect depth buffer orientation in classic osmesa. */
   OSMesaPixelStore(OSMESA_Y_UP, false);
   glScissor(0, 1, 1, 1);
   glClearDepth(1.0);
   glClear(GL_DEPTH_BUFFER_BIT);
   glFinish();
   EXPECT_EQ(depth[w * 0 + 0], 0x00ffffff);
   EXPECT_EQ(depth[w * 0 + 1], 0x00ffffff);
   EXPECT_EQ(depth[w * 1 + 0], 0x00ffffff);
      }
      TEST(OSMesaRenderTest, depth_get_no_attachment)
   {
      std::unique_ptr<osmesa_context, decltype(&OSMesaDestroyContext)> ctx{
                  uint32_t pixel;
   auto ret = OSMesaMakeCurrent(ctx.get(), &pixel, GL_UNSIGNED_BYTE, 1, 1);
            uint32_t *depth;
   GLint dw = 1, dh = 1, depth_cpp = 1;
   ASSERT_EQ(false, OSMesaGetDepthBuffer(ctx.get(), &dw, &dh, &depth_cpp, (void **)&depth));
   ASSERT_EQ(depth_cpp, NULL);
   ASSERT_EQ(dw, 0);
   ASSERT_EQ(dh, 0);
      }
      static uint32_t be_bswap32(uint32_t x)
   {
      if (UTIL_ARCH_BIG_ENDIAN)
         else
      }
      TEST(OSMesaRenderTest, separate_buffers_per_context)
   {
      std::unique_ptr<osmesa_context, decltype(&OSMesaDestroyContext)> ctx1{
         std::unique_ptr<osmesa_context, decltype(&OSMesaDestroyContext)> ctx2{
         ASSERT_TRUE(ctx1);
                     ASSERT_EQ(OSMesaMakeCurrent(ctx1.get(), &pixel1, GL_UNSIGNED_BYTE, 1, 1), GL_TRUE);
   glClearColor(1.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);
   glFinish();
            ASSERT_EQ(OSMesaMakeCurrent(ctx2.get(), &pixel2, GL_UNSIGNED_BYTE, 1, 1), GL_TRUE);
   glClearColor(0.0, 1.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);
   glFinish();
   EXPECT_EQ(pixel1, be_bswap32(0x000000ff));
            /* Leave a dangling render to pixel2 as we switch contexts (there should be
   */
   glClearColor(0.0, 0.0, 1.0, 0.0);
            ASSERT_EQ(OSMesaMakeCurrent(ctx1.get(), &pixel1, GL_UNSIGNED_BYTE, 1, 1), GL_TRUE);
   /* Draw something off screen to trigger a real flush.  We should have the
   * same contents in pixel1 as before
   */
   glBegin(GL_TRIANGLES);
   glVertex2f(-2, -2);
   glVertex2f(-2, -2);
   glVertex2f(-2, -2);
   glEnd();
   glFinish();
   EXPECT_EQ(pixel1, be_bswap32(0x000000ff));
      }
      TEST(OSMesaRenderTest, resize)
   {
      std::unique_ptr<osmesa_context, decltype(&OSMesaDestroyContext)> ctx{
                           ASSERT_EQ(OSMesaMakeCurrent(ctx.get(), draw1, GL_UNSIGNED_BYTE, 1, 1), GL_TRUE);
   glClearColor(1.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);
   glFinish();
            ASSERT_EQ(OSMesaMakeCurrent(ctx.get(), draw2, GL_UNSIGNED_BYTE, 2, 2), GL_TRUE);
   glClearColor(0.0, 1.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);
   glFinish();
   for (unsigned i = 0; i < ARRAY_SIZE(draw2); i++)
            }
