   #include <stdlib.h>
   #include <stdio.h>
   #include <float.h>
      #include "util/u_math.h"
   #include "util/half_float.h"
   #include "util/u_cpu_detect.h"
      static void
   test(void)
   {
      unsigned i;
            for(i = 0; i < 1 << 16; ++i)
   {
      uint16_t h = (uint16_t) i;
   union fi f;
            f.f = _mesa_half_to_float(h);
            if (h != rh && !(util_is_half_nan(h) && util_is_half_nan(rh))) {
      printf("Roundtrip failed: %x -> %x = %f -> %x\n", h, f.ui, f.f, rh);
                  if(roundtrip_fails) {
      printf("Failure! %u/65536 half floats failed a conversion to float and back.\n", roundtrip_fails);
         }
      int
   main(int argc, char **argv)
   {
               /* Test non-f16c. */
   if (util_get_cpu_caps()->has_f16c) {
      ((struct util_cpu_caps_t *)util_get_cpu_caps())->has_f16c = false;
               printf("Success!\n");
      }
