      #include "../sfn_instr_alu.h"
   #include "../sfn_instr_export.h"
   #include "../sfn_instr_tex.h"
   #include "../sfn_instrfactory.h"
      #include "gtest/gtest.h"
   #include <sstream>
      using namespace r600;
      using std::istringstream;
   using std::string;
   using std::vector;
      class TestShaderFromString : public ::testing::Test {
   public:
      void SetUp() override
   {
      init_pool();
                                       protected:
      void check(const vector<PInst>& eval,
         private:
         };
      TEST_F(TestShaderFromString, test_simple_fs)
   {
      auto init_str =
         # load constant color
   ALU MOV R2000.x@group : L[0x38000000] {W}
   ALU MOV R2000.y@group : L[0x0] {W}
   ALU MOV R2000.z@group : L[0x0] {W}
   ALU MOV R2000.w@group : L[0x38F00000] {WL}
      # write output
   EXPORT_DONE PIXEL 0 R2000.xyzw
   )";
                           expect.push_back(new AluInstr(op1_mov,
                        expect.push_back(new AluInstr(
            expect.push_back(new AluInstr(
            expect.push_back(new AluInstr(op1_mov,
                        auto exp = new ExportInstr(ExportInstr::pixel, 0, RegisterVec4(2000, false));
   exp->set_is_last_export(true);
               }
      TestShaderFromString::TestShaderFromString() {}
      std::vector<PInst>
   TestShaderFromString::from_string(const std::string& s)
   {
      istringstream is(s);
                     while (std::getline(is, line)) {
      if (line.find_first_not_of(" \t") == std::string::npos)
         if (line[0] == '#')
                           }
      void
   TestShaderFromString::check(const vector<PInst>& eval,
         {
               for (unsigned i = 0; i < eval.size(); ++i) {
            }
