   #! /usr/bin/env python3
      # script to parse nvidia CL headers and generate inlines to be used in pushbuffer encoding.
   # probably needs python3.9
      import argparse
   import os.path
   import sys
      from mako.template import Template
      METHOD_ARRAY_SIZES = {
      'BIND_GROUP_CONSTANT_BUFFER'        : 16,
   'CALL_MME_DATA'                     : 256,
   'CALL_MME_MACRO'                    : 256,
   'LOAD_CONSTANT_BUFFER'              : 16,
   'SET_ANTI_ALIAS_SAMPLE_POSITIONS'   : 4,
   'SET_BLEND'                         : 8,
   'SET_BLEND_PER_TARGET_*'            : 8,
   'SET_COLOR_TARGET_*'                : 8,
   'SET_COLOR_COMPRESSION'             : 8,
   'SET_COLOR_CLEAR_VALUE'             : 4,
   'SET_CT_WRITE'                      : 8,
   'SET_MME_SHADOW_SCRATCH'            : 256,
   'SET_PIPELINE_*'                    : 6,
   'SET_SCISSOR_*'                     : 16,
   'SET_STREAM_OUT_BUFFER_*'           : 4,
   'SET_VIEWPORT_*'                    : 16,
   'SET_VERTEX_ATTRIBUTE_*'            : 16,
      }
      METHOD_IS_FLOAT = [
      'SET_BLEND_CONST_*',
   'SET_DEPTH_BIAS',
   'SET_SLOPE_SCALE_DEPTH_BIAS',
   'SET_DEPTH_BIAS_CLAMP',
   'SET_DEPTH_BOUNDS_M*',
   'SET_LINE_WIDTH_FLOAT',
   'SET_ALIASED_LINE_WIDTH_FLOAT',
   'SET_VIEWPORT_SCALE_*',
   'SET_VIEWPORT_OFFSET_*',
   'SET_VIEWPORT_CLIP_MIN_Z',
   'SET_VIEWPORT_CLIP_MAX_Z',
      ]
      TEMPLATE_H = Template("""\
   /* parsed class ${nvcl} */
      #include "nvtypes.h"
   #include "${clheader}"
      #include <assert.h>
   #include <stdio.h>
   #include "util/u_math.h"
      %for mthd in mthddict:
   struct nv_${nvcl.lower()}_${mthd} {
   %for field_name in mthddict[mthd].field_name_start:
         %endfor
   };
      static inline void
   __${nvcl}_${mthd}(uint32_t *val_out, struct nv_${nvcl.lower()}_${mthd} st)
   {
         %for field_name in mthddict[mthd].field_name_start:
      <%
      field_start = int(mthddict[mthd].field_name_start[field_name])
   field_end = int(mthddict[mthd].field_name_end[field_name])
      %>
   %if field_width == 32:
   val |= st.${field_name.lower()};
   %else:
   assert(st.${field_name.lower()} < (1ULL << ${field_width}));
   val |= st.${field_name.lower()} << ${field_start};
      %endfor
         }
      #define V_${nvcl}_${mthd}(val, args...) { ${bs}
   %for field_name in mthddict[mthd].field_name_start:
      %for d in mthddict[mthd].field_defs[field_name]:
   UNUSED uint32_t ${field_name}_${d} = ${nvcl}_${mthd}_${field_name}_${d}; ${bs}
      %endfor
   %if len(mthddict[mthd].field_name_start) > 1:
         %else:
   <% field_name = next(iter(mthddict[mthd].field_name_start)).lower() %>\
         %endif
         }
      %if mthddict[mthd].is_array:
   #define VA_${nvcl}_${mthd}(i) V_${nvcl}_${mthd}
   %else:
   #define VA_${nvcl}_${mthd} V_${nvcl}_${mthd}
   %endif
      %if mthddict[mthd].is_array:
   #define P_${nvcl}_${mthd}(push, idx, args...) do { ${bs}
   %else:
   #define P_${nvcl}_${mthd}(push, args...) do { ${bs}
   %endif
   %for field_name in mthddict[mthd].field_name_start:
      %for d in mthddict[mthd].field_defs[field_name]:
   UNUSED uint32_t ${field_name}_${d} = ${nvcl}_${mthd}_${field_name}_${d}; ${bs}
      %endfor
      uint32_t nvk_p_ret; ${bs}
   V_${nvcl}_${mthd}(nvk_p_ret, args); ${bs}
   %if mthddict[mthd].is_array:
   nv_push_val(push, ${nvcl}_${mthd}(idx), nvk_p_ret); ${bs}
   %else:
   nv_push_val(push, ${nvcl}_${mthd}, nvk_p_ret); ${bs}
      } while(0)
      %endfor
      const char *P_PARSE_${nvcl}_MTHD(uint16_t idx);
   void P_DUMP_${nvcl}_MTHD_DATA(FILE *fp, uint16_t idx, uint32_t data,
         """)
      TEMPLATE_C = Template("""\
   #include "${header}"
      #include <stdio.h>
      const char*
   P_PARSE_${nvcl}_MTHD(uint16_t idx)
   {
         %for mthd in mthddict:
   %if mthddict[mthd].is_array and mthddict[mthd].array_size == 0:
         %endif
   %if mthddict[mthd].is_array:
      %for i in range(mthddict[mthd].array_size):
   case ${nvcl}_${mthd}(${i}):
            % else:
      case ${nvcl}_${mthd}:
      %endif
   %endfor
      default:
            }
      void
   P_DUMP_${nvcl}_MTHD_DATA(FILE *fp, uint16_t idx, uint32_t data,
         {
      uint32_t parsed;
      %for mthd in mthddict:
   %if mthddict[mthd].is_array and mthddict[mthd].array_size == 0:
         %endif
   %if mthddict[mthd].is_array:
      %for i in range(mthddict[mthd].array_size):
   case ${nvcl}_${mthd}(${i}):
      % else:
         %endif
   %for field_name in mthddict[mthd].field_name_start:
      <%
      field_start = int(mthddict[mthd].field_name_start[field_name])
   field_end = int(mthddict[mthd].field_name_end[field_name])
      %>
   %if field_width == 32:
         %else:
         %endif
         %if len(mthddict[mthd].field_defs[field_name]):
      switch (parsed) {
   %for d in mthddict[mthd].field_defs[field_name]:
   case ${nvcl}_${mthd}_${field_name}_${d}:
         fprintf(fp, "${d}${bs}n");
   %endfor
   default:
         fprintf(fp, "0x%x${bs}n", parsed);
      %else:
      %if mthddict[mthd].is_float:
   fprintf(fp, "%ff (0x%x)${bs}n", uif(parsed), parsed);
   %else:
   fprintf(fp, "(0x%x)${bs}n", parsed);
         %endfor
         %endfor
      default:
      fprintf(fp, "%s.VALUE = 0x%x${bs}n", prefix, data);
         }
   """)
      def glob_match(glob, name):
      if glob.endswith('*'):
         else:
      assert '*' not in glob
      class method(object):
      @property
   def array_size(self):
      for (glob, value) in METHOD_ARRAY_SIZES.items():
         if glob_match(glob, self.name):
         @property
   def is_float(self):
      for glob in METHOD_IS_FLOAT:
         if glob_match(glob, self.name):
            def parse_header(nvcl, f):
      # Simple state machine
   # state 0 looking for a new method define
   # state 1 looking for new fields in a method
   # state 2 looking for enums for a fields in a method
            state = 0
   mthddict = {}
   curmthd = {}
               if line.strip() == "":
         state = 0
   if (curmthd):
      if not len(curmthd.field_name_start):
               if line.startswith("#define"):
         list = line.split();
                                                if state == 2:
      teststr = nvcl + "_" + curmthd.name + "_" + curfield + "_"
   if ":" in list[2]:
         elif teststr in list[1]:
                     if state == 1:
      teststr = nvcl + "_" + curmthd.name + "_"
   if teststr in list[1]:
      if ("0x" in list[2]):
         else:
         field = list[1].removeprefix(teststr)
   bitfield = list[2].split(":")
   curmthd.field_name_start[field] = bitfield[1]
   curmthd.field_name_end[field] = bitfield[0]
   curmthd.field_defs[field] = {}
      else:
      if not len(curmthd.field_name_start):
                  if state == 0:
      if (curmthd):
      if not len(curmthd.field_name_start):
      teststr = nvcl + "_"
   is_array = 0
   if (':' in list[2]):
         name = list[1].removeprefix(teststr)
   if name.endswith("(i)"):
      is_array = 1
      if name.endswith("(j)"):
      is_array = 1
      x = method()
   x.name = name
   x.addr = list[2]
   x.is_array = is_array
   x.field_name_start = {}
                                 def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out_h', required=True, help='Output C header.')
   parser.add_argument('--out_c', required=True, help='Output C file.')
   parser.add_argument('--in_h',
                        clheader = os.path.basename(args.in_h)
   nvcl = clheader
   nvcl = nvcl.removeprefix("cl")
   nvcl = nvcl.removesuffix(".h")
   nvcl = nvcl.upper()
            with open(args.in_h, 'r', encoding='utf-8') as f:
            environment = {
      'clheader': clheader,
   'header': os.path.basename(args.out_h),
   'nvcl': nvcl,
   'mthddict': mthddict,
               try:
      with open(args.out_h, 'w', encoding='utf-8') as f:
         with open(args.out_c, 'w', encoding='utf-8') as f:
         except Exception:
      # In the event there's an error, this imports some helpers from mako
   # to print a useful stack trace and prints it, then exits with
   # status 1, if python is run with debug; otherwise it just raises
   # the exception
   import sys
   from mako import exceptions
   print(exceptions.text_error_template().render(), file=sys.stderr)
      if __name__ == '__main__':
      main()
