   # Copyright © 2023 Intel Corporation
      # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
      # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
      # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      import argparse
   import collections
   import json
   import os
   import sys
   from mako.template import Template
      HEADER_TEMPLATE = Template("""\
   /*
   * Copyright © 2023 Intel Corporation
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
      #ifndef INTEL_WA_H
   #define INTEL_WA_H
      #include "util/macros.h"
      #ifdef __cplusplus
   extern "C" {
   #endif
      struct intel_device_info;
   void intel_device_info_init_was(struct intel_device_info *devinfo);
      enum intel_wa_steppings {
   % for a in stepping_enum:
         % endfor
         };
      enum intel_workaround_id {
   % for a in wa_def:
         % endfor
         };
      /* These defines are used to identify when a workaround potentially applies
   * in genxml code.  They should not be used directly. intel_needs_workaround()
   * checks these definitions to eliminate bitset tests at compile time.
   */
   % for a in wa_def:
   #define INTEL_WA_${a}_GFX_VER ${wa_macro[a]}
   % endfor
      /* These defines are suitable for use to compile out genxml code using #if
   * guards.  Workarounds that apply to part of a generation must use a
   * combination of run time checks and INTEL_WA_{NUM}_GFX_VER macros.  Those
   * workarounds are 'poisoned' below.
   */
   % for a in partial_gens:
         PRAGMA_POISON(INTEL_NEEDS_WA_${a})
         #define INTEL_NEEDS_WA_${a} INTEL_WA_${a}_GFX_VER
         % endfor
      #define INTEL_ALL_WA ${"\\\\"}
   % for wa_id in wa_def:
   INTEL_WA(${wa_id}), ${"\\\\"}
   % endfor
      #ifdef __cplusplus
   }
   #endif
      #endif /* INTEL_WA_H */
   """)
      IMPL_TEMPLATE = Template("""\
   /*
   * Copyright © 2023 Intel Corporation
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
      #include "dev/intel_wa.h"
   #include "dev/intel_device_info.h"
   #include "util/bitset.h"
      void intel_device_info_init_was(struct intel_device_info *devinfo)
   {
         % for platform in platform_bugs:
         % if platform in stepping_bugs:
         % for stepping, ids in stepping_bugs[platform].items():
         % for id in ids:
         % endfor
         % endfor
               default:
   % endif
   % for id in platform_bugs[platform]:
         % endfor
         % endfor
         default:
      /* unsupported platform */
      }
   """)
      def stepping_enums(wa_def):
      """provide a sorted list of all known steppings"""
   stepping_enum = []
   for bug in wa_def.values():
      for platform_info in bug["mesa_platforms"].values():
         steppings = platform_info["steppings"]
   if steppings == "all":
         steppings = steppings.split("..")
   for stepping in steppings:
      stepping = stepping.upper()
         _PLATFORM_GFXVERS = {"INTEL_PLATFORM_BDW" : 80,
                        "INTEL_PLATFORM_CHV" : 80,
   "INTEL_PLATFORM_SKL" : 90,
   "INTEL_PLATFORM_BXT" : 90,
   "INTEL_PLATFORM_KBL" : 90,
   "INTEL_PLATFORM_GLK" : 90,
   "INTEL_PLATFORM_CFL" : 90,
   "INTEL_PLATFORM_ICL" : 110,
   "INTEL_PLATFORM_EHL" : 110,
   "INTEL_PLATFORM_TGL" : 120,
   "INTEL_PLATFORM_RKL" : 120,
   "INTEL_PLATFORM_DG1" : 120,
   "INTEL_PLATFORM_ADL" : 120,
   "INTEL_PLATFORM_RPL" : 120,
   "INTEL_PLATFORM_DG2_G10" : 125,
   "INTEL_PLATFORM_DG2_G11" : 125,
         def macro_versions(wa_def):
      """provide a map of workaround id -> GFX_VERx10 macro test"""
   wa_macro = {}
   for bug_id, bug in wa_def.items():
      platforms = set()
   for platform in bug["mesa_platforms"]:
         gfxver = _PLATFORM_GFXVERS[platform]
   if gfxver not in platforms:
   if not platforms:
         ver_cmps = [f"(GFX_VERx10 == {platform})" for platform in sorted(platforms)]
   wa_macro[bug_id] = ver_cmps[0]
   if len(ver_cmps) > 1:
            def partial_gens(wa_def):
      """provide a map of workaround id -> true/false, indicating whether the wa
   applies to a subset of platforms in a generation"""
            # map of gfxver -> set(all platforms for gfxver)
   generations = collections.defaultdict(set)
   for platform, gfxver in _PLATFORM_GFXVERS.items():
            # map of platform -> set(all required platforms for gen completeness)
   required_platforms = collections.defaultdict(set)
   for gen_set in generations.values():
      for platform in gen_set:
         for bug_id, bug in wa_def.items():
      # for the given wa, create a set which includes all platforms that
   # match any of the affected gfxver.
   wa_required_for_completeness = set()
   for platform in bug["mesa_platforms"]:
            # eliminate each platform specifically indicated by the WA, to see if
   # are left over.
   for platform in bug["mesa_platforms"]:
            # if any platform remains in the required set, then this wa *partially*
   # applies to one of the gfxvers.
            def platform_was(wa_def):
      """provide a map of platform -> list of workarounds"""
   platform_bugs = collections.defaultdict(list)
   for workaround, bug in wa_def.items():
      for platform, desc in bug["mesa_platforms"].items():
         if desc["steppings"] != "all":
      # stepping-specific workaround, not platform-wide
         def stepping_was(wa_def, all_steppings):
      """provide a map of wa[platform][stepping] -> [ids]"""
   stepping_bugs = collections.defaultdict(lambda: collections.defaultdict(list))
   for workaround, bug in wa_def.items():
      for platform, desc in bug["mesa_platforms"].items():
         if desc["steppings"] == "all":
         first_stepping, fixed_stepping = desc["steppings"].split("..")
   first_stepping = first_stepping.upper()
   fixed_stepping = fixed_stepping.upper()
   steppings = []
   for step in all_steppings:
      if step <first_stepping:
         if step >= fixed_stepping:
            for step in steppings:
      u_step = step.upper()
         def main():
      """writes c/h generated files to outdir"""
   parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
   parser.add_argument("wa_file", type=str,
         parser.add_argument("header_file", help="include file to generate")
   parser.add_argument("impl_file", help="implementation file to generate")
   args = parser.parse_args()
   if not os.path.exists(args.wa_file):
      print(f"Error: workaround definition not found: {args.wa_file}")
         # json dictionary of workaround definitions
   wa_def = {}
   with open(args.wa_file, encoding='utf8') as wa_fh:
            steppings = stepping_enums(wa_def)
   with open(args.header_file, 'w', encoding='utf8') as header:
      header.write(HEADER_TEMPLATE.render(wa_def=wa_def,
                  with open(args.impl_file, 'w', encoding='utf8') as impl:
      impl.write(IMPL_TEMPLATE.render(platform_bugs=platform_was(wa_def),
      main()
