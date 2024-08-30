   #include "../sfn_liverangeevaluator.h"
   #include "../sfn_shader.h"
   #include "sfn_test_shaders.h"
      #include "gtest/gtest.h"
   #include <sstream>
      namespace r600 {
      using std::ostringstream;
      class LiveRangeTests : public TestShader {
      protected:
         };
      using SimpleTest = testing::Test;
      TEST_F(SimpleTest, SimpleLiveRangeMapTest)
   {
      LiveRangeMap a;
                     Register r1x(1, 0, pin_none);
   a.append_register(&r1x);
   r1x.set_index(0);
                     b.append_register(&r1x);
   b.set_life_range(r1x, 0, 1);
            Register r2x(2, 0, pin_none);
   a.append_register(&r2x);
   r2x.set_index(0);
                     b.append_register(&r2x);
   b.set_life_range(r2x, 0, 2);
            a.set_life_range(r2x, 1, 2);
            b.set_life_range(r2x, 1, 2);
            a.set_life_range(r2x, 0, 1);
      }
      TEST_F(LiveRangeTests, SimpleAssignments)
   {
                        Register *r1x = vf.dest_from_string("S1.x@free");
                     expect.set_life_range(*r1x, 2, 3);
   for (int i = 0; i < 4; ++i)
               }
      TEST_F(LiveRangeTests, SimpleAdd)
   {
               ValueFactory vf;
   Register *r0x = vf.dest_from_string("S0.x@free");
   Register *r1x = vf.dest_from_string("S1.x@free");
   RegisterVec4 r2 = vf.dest_vec4_from_string("S2.xyzw", dummy, pin_none);
   Register *r3x = vf.dest_from_string("S3.x@free");
                     expect.set_life_range(*r0x, 1, 4);
   expect.set_life_range(*r1x, 2, 3);
            expect.set_life_range(*r2[0], 3, 4);
   for (int i = 1; i < 4; ++i)
            for (int i = 0; i < 4; ++i)
               }
      TEST_F(LiveRangeTests, SimpleAInterpolation)
   {
               ValueFactory vf;
   Register *r0x = vf.dest_from_string("R0.x@fully");
   r0x->set_flag(Register::pin_start);
   Register *r0y = vf.dest_from_string("R0.y@fully");
            Register *r1x = vf.dest_from_string("S1.x@free");
            Register *r3x = vf.dest_from_string("S3.x");
   Register *r3y = vf.dest_from_string("S3.y");
            Register *r4x = vf.dest_from_string("S4.x");
            RegisterVec4 r5 = vf.dest_vec4_from_string("S5.xy_w", dummy, pin_group);
                     expect.set_life_range(*r0x, 0, 3);
                     expect.set_life_range(*r2[0], 3, 4);
   expect.set_life_range(*r2[1], 3, 4);
   expect.set_life_range(*r2[2], 2, 3);
            expect.set_life_range(*r3x, 4, 5);
   expect.set_life_range(*r3y, 4, 5);
            expect.set_life_range(*r4x, 5, 6);
            expect.set_life_range(*r5[0], 6, 7);
   expect.set_life_range(*r5[1], 6, 7);
            expect.set_life_range(*r6[0], 7, 8);
   expect.set_life_range(*r6[1], 7, 8);
   expect.set_life_range(*r6[2], 7, 8);
               }
      TEST_F(LiveRangeTests, SimpleArrayAccess)
   {
                                 auto s1 = vf.dest_from_string("S1.x");
   auto s2x = vf.dest_from_string("S2.x");
                              expect.set_life_range(*array->element(0, nullptr, 0), 1, 5);
   expect.set_life_range(*array->element(0, nullptr, 1), 1, 5);
                              expect.set_life_range(*s2x, 5, 6);
            expect.set_life_range(*s3[0], 6, 7);
               }
      void
   LiveRangeTests::check(const char *shader, LiveRangeMap& expect)
   {
      auto sh = from_string(shader);
                              ostringstream eval_str;
            ostringstream expect_str;
               }
      } // namespace r600
