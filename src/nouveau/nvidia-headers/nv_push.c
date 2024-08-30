   #include "nv_push.h"
      #include "nv_device_info.h"
      #include <inttypes.h>
      #include "nvk_cl906f.h"
   #include "nvk_cl9097.h"
   #include "nvk_cl902d.h"
   #include "nvk_cl90b5.h"
   #include "nvk_cla097.h"
   #include "nvk_cla0b5.h"
   #include "nvk_cla0c0.h"
   #include "nvk_clb197.h"
   #include "nvk_clc0c0.h"
   #include "nvk_clc1b5.h"
   #include "nvk_clc397.h"
   #include "nvk_clc3c0.h"
   #include "nvk_clc597.h"
      #ifndef NDEBUG
   void
   nv_push_validate(struct nv_push *push)
   {
               /* submitting empty push buffers is probably a bug */
            /* make sure we don't overrun the bo */
            /* parse all the headers to see if we get to buf->map */
   while (cur < push->end) {
      uint32_t hdr = *cur;
            switch (mthd) {
   /* immd */
   case 4:
         case 1:
   case 3:
   case 5: {
      uint32_t count = (hdr >> 16) & 0x1fff;
   assert(count);
   cur += count;
      }
   default:
                  cur++;
         }
   #endif
      void
   vk_push_print(FILE *fp, const struct nv_push *push,
         {
               while (cur < push->end) {
      uint32_t hdr = *cur;
   uint32_t type = hdr >> 29;
   uint32_t inc = 0;
   uint32_t count = (hdr >> 16) & 0x1fff;
   uint32_t subchan = (hdr >> 13) & 0x7;
   uint32_t mthd = (hdr & 0xfff) << 2;
   uint32_t value = 0;
            fprintf(fp, "[0x%08" PRIxPTR "] HDR %x subch %i",
                  switch (type) {
   case 4:
      fprintf(fp, " IMMD\n");
   inc = 0;
   is_immd = true;
   value = count;
   count = 1;
      case 1:
      fprintf(fp, " NINC\n");
   inc = count;
      case 3:
      fprintf(fp, " 0INC\n");
   inc = 0;
      case 5:
      fprintf(fp, " 1INC\n");
   inc = 1;
               while (count--) {
      const char *mthd_name = "";
   if (mthd < 0x100) {
         } else {
      switch (subchan) {
   case 0:
      if (devinfo->cls_eng3d >= 0xc597)
         else if (devinfo->cls_eng3d >= 0xc397)
         else if (devinfo->cls_eng3d >= 0xb197)
         else if (devinfo->cls_eng3d >= 0xa097)
         else
            case 1:
      if (devinfo->cls_compute >= 0xc3c0)
         else if (devinfo->cls_compute >= 0xc0c0)
         else
            case 3:
      mthd_name = P_PARSE_NV902D_MTHD(mthd);
      case 4:
      if (devinfo->cls_copy >= 0xc1b5)
         else if (devinfo->cls_copy >= 0xa0b5)
         else
            default:
      mthd_name = "";
                                 fprintf(fp, "\tmthd %04x %s\n", mthd, mthd_name);
   if (mthd < 0x100) {
         } else {
      switch (subchan) {
   case 0:
      if (devinfo->cls_eng3d >= 0xc597)
         else if (devinfo->cls_eng3d >= 0xc397)
         else if (devinfo->cls_eng3d >= 0xb197)
         else if (devinfo->cls_eng3d >= 0xa097)
         else
            case 1:
      if (devinfo->cls_compute >= 0xc3c0)
         else if (devinfo->cls_compute >= 0xc0c0)
         else
            case 3:
      P_DUMP_NV902D_MTHD_DATA(fp, mthd, value, "\t\t");
      case 4:
      if (devinfo->cls_copy >= 0xc1b5)
         else if (devinfo->cls_copy >= 0xa0b5)
         else
            default:
      mthd_name = "";
                                 if (inc) {
      inc--;
                        }
