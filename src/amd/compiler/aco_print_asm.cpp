   /*
   * Copyright © 2018 Valve Corporation
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
   *
   */
      #include "aco_ir.h"
      #include "util/u_debug.h"
      #ifdef LLVM_AVAILABLE
   #if defined(_MSC_VER) && defined(restrict)
   #undef restrict
   #endif
   #include "llvm/ac_llvm_util.h"
      #include "llvm-c/Disassembler.h"
   #include <llvm/ADT/StringRef.h>
   #include <llvm/MC/MCDisassembler/MCDisassembler.h>
   #endif
      #include <array>
   #include <iomanip>
   #include <vector>
      namespace aco {
   namespace {
      std::vector<bool>
   get_referenced_blocks(Program* program)
   {
      std::vector<bool> referenced_blocks(program->blocks.size());
   referenced_blocks[0] = true;
   for (Block& block : program->blocks) {
      for (unsigned succ : block.linear_succs)
      }
      }
      void
   print_block_markers(FILE* output, Program* program, const std::vector<bool>& referenced_blocks,
         {
      while (*next_block < program->blocks.size() && pos == program->blocks[*next_block].offset) {
      if (referenced_blocks[*next_block])
               }
      void
   print_instr(FILE* output, const std::vector<uint32_t>& binary, char* instr, unsigned size,
         {
               for (unsigned i = 0; i < size; i++)
            }
      void
   print_constant_data(FILE* output, Program* program)
   {
      if (program->constant_data.empty())
            fputs("\n/* constant data */\n", output);
   for (unsigned i = 0; i < program->constant_data.size(); i += 32) {
      fprintf(output, "[%.6u]", i);
   unsigned line_size = std::min<size_t>(program->constant_data.size() - i, 32);
   for (unsigned j = 0; j < line_size; j += 4) {
      unsigned size = std::min<size_t>(program->constant_data.size() - (i + j), 4);
   uint32_t v = 0;
   memcpy(&v, &program->constant_data[i + j], size);
      }
         }
      /**
   * Determines the GPU type to use for CLRXdisasm
   */
   const char*
   to_clrx_device_name(amd_gfx_level gfx_level, radeon_family family)
   {
      switch (gfx_level) {
   case GFX6:
      switch (family) {
   case CHIP_TAHITI: return "tahiti";
   case CHIP_PITCAIRN: return "pitcairn";
   case CHIP_VERDE: return "capeverde";
   case CHIP_OLAND: return "oland";
   case CHIP_HAINAN: return "hainan";
   default: return nullptr;
      case GFX7:
      switch (family) {
   case CHIP_BONAIRE: return "bonaire";
   case CHIP_KAVERI: return "gfx700";
   case CHIP_HAWAII: return "hawaii";
   default: return nullptr;
      case GFX8:
      switch (family) {
   case CHIP_TONGA: return "tonga";
   case CHIP_ICELAND: return "iceland";
   case CHIP_CARRIZO: return "carrizo";
   case CHIP_FIJI: return "fiji";
   case CHIP_STONEY: return "stoney";
   case CHIP_POLARIS10: return "polaris10";
   case CHIP_POLARIS11: return "polaris11";
   case CHIP_POLARIS12: return "polaris12";
   case CHIP_VEGAM: return "polaris11";
   default: return nullptr;
      case GFX9:
      switch (family) {
   case CHIP_VEGA10: return "vega10";
   case CHIP_VEGA12: return "vega12";
   case CHIP_VEGA20: return "vega20";
   case CHIP_RAVEN: return "raven";
   default: return nullptr;
      case GFX10:
      switch (family) {
   case CHIP_NAVI10: return "gfx1010";
   case CHIP_NAVI12: return "gfx1011";
   default: return nullptr;
      default: return nullptr;
      }
      bool
   get_branch_target(char** output, Program* program, const std::vector<bool>& referenced_blocks,
         {
      unsigned pos;
   if (sscanf(*line_start, ".L%d_0", &pos) != 1)
         pos /= 4;
            for (Block& block : program->blocks) {
      if (referenced_blocks[block.index] && block.offset == pos) {
      *output += sprintf(*output, "BB%u", block.index);
         }
      }
      bool
   print_asm_clrx(Program* program, std::vector<uint32_t>& binary, unsigned exec_size, FILE* output)
   {
   #ifdef _WIN32
         #else
      char path[] = "/tmp/fileXXXXXX";
   char line[2048], command[128];
   FILE* p;
                     /* Dump the binary into a temporary file. */
   fd = mkstemp(path);
   if (fd < 0)
            for (unsigned i = 0; i < exec_size; i++) {
      if (write(fd, &binary[i], 4) == -1)
                        p = popen(command, "r");
   if (p) {
      if (!fgets(line, sizeof(line), p)) {
      fprintf(output, "clrxdisasm not found\n");
   pclose(p);
               std::vector<bool> referenced_blocks = get_referenced_blocks(program);
            char prev_instr[2048];
   unsigned prev_pos = 0;
   do {
      char* line_start = line;
                  unsigned pos;
   if (sscanf(line_start, "/*%x*/", &pos) != 1)
                  while (strncmp(line_start, "*/", 2))
                  while (line_start[0] == ' ')
                                 if (pos != prev_pos) {
      /* Print the previous instruction, now that we know the encoding size. */
   print_instr(output, binary, prev_instr, pos - prev_pos, prev_pos);
                        char* dest = prev_instr;
   *(dest++) = '\t';
   while (*line_start) {
      if (!strncmp(line_start, ".L", 2) &&
      get_branch_target(&dest, program, referenced_blocks, &line_start))
         }
               if (prev_pos != exec_size)
                                       fail:
      close(fd);
   unlink(path);
      #endif
   }
      #ifdef LLVM_AVAILABLE
   std::pair<bool, size_t>
   disasm_instr(amd_gfx_level gfx_level, LLVMDisasmContextRef disasm, uint32_t* binary,
         {
      size_t l =
      LLVMDisasmInstruction(disasm, (uint8_t*)&binary[pos], (exec_size - pos) * sizeof(uint32_t),
         if (gfx_level >= GFX10 && l == 8 && ((binary[pos] & 0xffff0000) == 0xd7610000) &&
      ((binary[pos + 1] & 0x1ff) == 0xff)) {
   /* v_writelane with literal uses 3 dwords but llvm consumes only 2 */
               bool invalid = false;
   size_t size;
   if (!l &&
      ((gfx_level >= GFX9 &&
         (gfx_level >= GFX10 &&
         (gfx_level <= GFX9 &&
         (gfx_level >= GFX10 && (binary[pos] & 0xffff8000) == 0xd76d8000) || /* v_add3_u32 + clamp */
   (gfx_level == GFX9 && (binary[pos] & 0xffff8000) == 0xd1ff8000)) /* v_add3_u32 + clamp */) {
   strcpy(outline, "\tinteger addition + clamp");
   bool has_literal = gfx_level >= GFX10 && (((binary[pos + 1] & 0x1ff) == 0xff) ||
            } else if (gfx_level >= GFX10 && l == 4 && ((binary[pos] & 0xfe0001ff) == 0x020000f9)) {
      strcpy(outline, "\tv_cndmask_b32 + sdwa");
      } else if (!l) {
      strcpy(outline, "(invalid instruction)");
   size = 1;
      } else {
      assert(l % 4 == 0);
                  }
      bool
   print_asm_llvm(Program* program, std::vector<uint32_t>& binary, unsigned exec_size, FILE* output)
   {
               std::vector<llvm::SymbolInfoTy> symbols;
   std::vector<std::array<char, 16>> block_names;
   block_names.reserve(program->blocks.size());
   for (Block& block : program->blocks) {
      if (!referenced_blocks[block.index])
         std::array<char, 16> name;
   sprintf(name.data(), "BB%u", block.index);
   block_names.push_back(name);
   symbols.emplace_back(block.offset * 4,
               const char* features = "";
   if (program->gfx_level >= GFX10 && program->wave_size == 64) {
                  LLVMDisasmContextRef disasm =
      LLVMCreateDisasmCPUFeatures("amdgcn-mesa-mesa3d", ac_get_llvm_processor_name(program->family),
         size_t pos = 0;
   bool invalid = false;
            unsigned prev_size = 0;
   unsigned prev_pos = 0;
   unsigned repeat_count = 0;
   while (pos <= exec_size) {
      bool new_block =
         if (pos + prev_size <= exec_size && prev_pos != pos && !new_block &&
      memcmp(&binary[prev_pos], &binary[pos], prev_size * 4) == 0) {
   repeat_count++;
   pos += prev_size;
      } else {
      if (repeat_count)
                              /* For empty last block, only print block marker. */
   if (pos == exec_size)
            char outline[1024];
   std::pair<bool, size_t> res = disasm_instr(program->gfx_level, disasm, binary.data(),
                           prev_size = res.second;
   prev_pos = pos;
      }
                                 }
   #endif /* LLVM_AVAILABLE */
      } /* end namespace */
      bool
   check_print_asm_support(Program* program)
   {
   #ifdef LLVM_AVAILABLE
      if (program->gfx_level >= GFX8) {
      /* LLVM disassembler only supports GFX8+ */
   const char* name = ac_get_llvm_processor_name(program->family);
   const char* triple = "amdgcn--";
            LLVMTargetMachineRef tm = LLVMCreateTargetMachine(
            bool supported = ac_is_llvm_processor_supported(tm, name);
            if (supported)
         #endif
      #ifndef _WIN32
      /* Check if CLRX disassembler binary is available and can disassemble the program */
   return to_clrx_device_name(program->gfx_level, program->family) &&
      #else
         #endif
   }
      /* Returns true on failure */
   bool
   print_asm(Program* program, std::vector<uint32_t>& binary, unsigned exec_size, FILE* output)
   {
   #ifdef LLVM_AVAILABLE
      if (program->gfx_level >= GFX8) {
            #endif
            }
      } // namespace aco
