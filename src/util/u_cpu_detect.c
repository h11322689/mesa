   /**************************************************************************
   *
   * Copyright 2008 Dennis Smit
   * All Rights Reserved.
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * @file
   * CPU feature detection.
   *
   * @author Dennis Smit
   * @author Based on the work of Eric Anholt <anholt@FreeBSD.org>
   */
      #include "util/detect.h"
   #include "util/compiler.h"
      #include "util/u_debug.h"
   #include "u_cpu_detect.h"
   #include "u_math.h"
   #include "os_file.h"
   #include "c11/threads.h"
      #include <stdio.h>
   #include <inttypes.h>
      #if DETECT_ARCH_PPC
   #if DETECT_OS_APPLE
   #include <sys/sysctl.h>
   #else
   #include <signal.h>
   #include <setjmp.h>
   #endif
   #endif
      #if DETECT_OS_BSD
   #include <sys/param.h>
   #include <sys/sysctl.h>
   #include <machine/cpu.h>
   #endif
      #if DETECT_OS_FREEBSD
   #if __has_include(<sys/auxv.h>)
   #include <sys/auxv.h>
   #define HAVE_ELF_AUX_INFO
   #endif
   #endif
      #if DETECT_OS_LINUX
   #include <signal.h>
   #include <fcntl.h>
   #include <elf.h>
   #endif
      #if DETECT_OS_UNIX
   #include <unistd.h>
   #endif
      #if defined(HAS_ANDROID_CPUFEATURES)
   #include <cpu-features.h>
   #endif
      #if DETECT_OS_WINDOWS
   #include <windows.h>
   #if DETECT_CC_MSVC
   #include <intrin.h>
   #endif
   #endif
      #if defined(HAS_SCHED_H)
   #include <sched.h>
   #endif
      // prevent inadvert infinite recursion
   #define util_get_cpu_caps() util_get_cpu_caps_DO_NOT_USE()
      DEBUG_GET_ONCE_BOOL_OPTION(dump_cpu, "GALLIUM_DUMP_CPU", false)
      static
   struct util_cpu_caps_t util_cpu_caps;
      /* Do not try to access _util_cpu_caps_state directly, call to util_get_cpu_caps instead */
   struct _util_cpu_caps_state_t _util_cpu_caps_state = {
      .once_flag = ONCE_FLAG_INIT,
      };
      #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
   static int has_cpuid(void);
   #endif
         #if DETECT_ARCH_PPC && !DETECT_OS_APPLE && !DETECT_OS_BSD && !DETECT_OS_LINUX
   static jmp_buf  __lv_powerpc_jmpbuf;
   static volatile sig_atomic_t __lv_powerpc_canjump = 0;
      static void
   sigill_handler(int sig)
   {
      if (!__lv_powerpc_canjump) {
      signal (sig, SIG_DFL);
               __lv_powerpc_canjump = 0;
      }
   #endif
      #if DETECT_ARCH_PPC
   static void
   check_os_altivec_support(void)
   {
   #if defined(__ALTIVEC__)
         #endif
   #if defined(__VSX__)
         #endif
   #if defined(__ALTIVEC__) && defined(__VSX__)
   /* Do nothing */
   #elif DETECT_OS_APPLE || DETECT_OS_NETBSD || DETECT_OS_OPENBSD
   #ifdef HW_VECTORUNIT
         #else
         #endif
      int has_vu = 0;
   size_t len = sizeof (has_vu);
                     if (err == 0) {
      if (has_vu != 0) {
               #elif DETECT_OS_FREEBSD /* !DETECT_OS_APPLE && !DETECT_OS_NETBSD && !DETECT_OS_OPENBSD */
         #ifdef HAVE_ELF_AUX_INFO
         #else
      size_t len = sizeof(hwcap);
      #endif
      if (hwcap & PPC_FEATURE_HAS_ALTIVEC)
         if (hwcap & PPC_FEATURE_HAS_VSX)
      #elif DETECT_OS_LINUX /* !DETECT_OS_FREEBSD */
   #if DETECT_ARCH_PPC_64
         #else
         #endif
      int fd = open("/proc/self/auxv", O_RDONLY | O_CLOEXEC);
   if (fd >= 0) {
      while (read(fd, &aux, sizeof(aux)) == sizeof(aux)) {
      if (aux.a_type == AT_HWCAP) {
      char *env_vsx = getenv("GALLIVM_VSX");
   uint64_t hwcap = aux.a_un.a_val;
   util_cpu_caps.has_altivec = (hwcap >> 28) & 1;
   if (!env_vsx || env_vsx[0] != '0') {
         }
         }
         #else /* !DETECT_OS_APPLE && !DETECT_OS_BSD && !DETECT_OS_LINUX */
      /* not on Apple/Darwin or Linux, do it the brute-force way */
   /* this is borrowed from the libmpeg2 library */
   signal(SIGILL, sigill_handler);
   if (setjmp(__lv_powerpc_jmpbuf)) {
         } else {
      bool enable_altivec = true;    /* Default: enable  if available, and if not overridden */
   #ifdef DEBUG
         /* Disabling Altivec code generation is not the same as disabling VSX code generation,
   * which can be done simply by passing -mattr=-vsx to the LLVM compiler; cf.
   * lp_build_create_jit_compiler_for_module().
   * If you want to disable Altivec code generation, the best place to do it is here.
   */
   char *env_control = getenv("GALLIVM_ALTIVEC");    /* 1=enable (default); 0=disable */
   if (env_control && env_control[0] == '0') {
         #endif
         /* VSX instructions can be explicitly enabled/disabled via GALLIVM_VSX=1 or 0 */
   char *env_vsx = getenv("GALLIVM_VSX");
   if (env_vsx && env_vsx[0] == '0') {
         }
   if (enable_altivec) {
               __asm __volatile
      ("mtspr 256, %0\n\t"
   "vand %%v0, %%v0, %%v0"
                        if (enable_vsx) {
      __asm __volatile("xxland %vs0, %vs0, %vs0");
      }
      } else {
               #endif /* !DETECT_OS_APPLE && !DETECT_OS_LINUX */
   }
   #endif /* DETECT_ARCH_PPC */
         #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
   static int has_cpuid(void)
   {
   #if DETECT_ARCH_X86
   #if DETECT_OS_GCC
               __asm __volatile
      ("pushf\n"
   "popl %0\n"
   "movl %0, %1\n"
   "xorl $0x200000, %0\n"
   "push %0\n"
   "popf\n"
   "pushf\n"
   "popl %0\n"
   : "=a" (a), "=c" (c)
   :
            #else
      /* FIXME */
      #endif
   #elif DETECT_ARCH_X86_64
         #else
         #endif
   }
         /**
   * @sa cpuid.h included in gcc-4.3 onwards.
   * @sa http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
   */
   static inline void
   cpuid(uint32_t ax, uint32_t *p)
   {
   #if DETECT_CC_GCC && DETECT_ARCH_X86
      __asm __volatile (
   "xchgl %%ebx, %1\n\t"
   "cpuid\n\t"
   "xchgl %%ebx, %1"
   : "=a" (p[0]),
      "=S" (p[1]),
   "=c" (p[2]),
      : "0" (ax)
      #elif DETECT_CC_GCC && DETECT_ARCH_X86_64
      __asm __volatile (
   "cpuid\n\t"
   : "=a" (p[0]),
      "=b" (p[1]),
   "=c" (p[2]),
      : "0" (ax)
      #elif DETECT_CC_MSVC
         #else
      p[0] = 0;
   p[1] = 0;
   p[2] = 0;
      #endif
   }
      /**
   * @sa cpuid.h included in gcc-4.4 onwards.
   * @sa http://msdn.microsoft.com/en-us/library/hskdteyh%28v=vs.90%29.aspx
   */
   static inline void
   cpuid_count(uint32_t ax, uint32_t cx, uint32_t *p)
   {
   #if DETECT_CC_GCC && DETECT_ARCH_X86
      __asm __volatile (
   "xchgl %%ebx, %1\n\t"
   "cpuid\n\t"
   "xchgl %%ebx, %1"
   : "=a" (p[0]),
      "=S" (p[1]),
   "=c" (p[2]),
      : "0" (ax), "2" (cx)
      #elif DETECT_CC_GCC && DETECT_ARCH_X86_64
      __asm __volatile (
   "cpuid\n\t"
   : "=a" (p[0]),
      "=b" (p[1]),
   "=c" (p[2]),
      : "0" (ax), "2" (cx)
      #elif DETECT_CC_MSVC
         #else
      p[0] = 0;
   p[1] = 0;
   p[2] = 0;
      #endif
   }
         static inline uint64_t xgetbv(void)
   {
   #if DETECT_CC_GCC
               __asm __volatile (
   ".byte 0x0f, 0x01, 0xd0" // xgetbv isn't supported on gcc < 4.4
   : "=a"(eax),
         : "c"(0)
               #elif DETECT_CC_MSVC && defined(_MSC_FULL_VER) && defined(_XCR_XFEATURE_ENABLED_MASK)
         #else
         #endif
   }
         #if DETECT_ARCH_X86
   UTIL_ALIGN_STACK
   static inline bool
   sse2_has_daz(void)
   {
      alignas(16) struct {
      uint32_t pad1[7];
   uint32_t mxcsr_mask;
                  #if DETECT_CC_GCC
         #elif DETECT_CC_MSVC || DETECT_CC_ICL
         #else
         #endif
         }
   #endif
      #endif /* X86 or X86_64 */
      #if DETECT_ARCH_ARM
   static void
   check_os_arm_support(void)
   {
      /*
   * On Android, the cpufeatures library is preferred way of checking
   * CPU capabilities. However, it is not available for standalone Mesa
   * builds, i.e. when Android build system (Android.mk-based) is not
   * used. Because of this we cannot use DETECT_OS_ANDROID here, but rather
   * have a separate macro that only gets enabled from respective Android.mk.
      #if defined(__ARM_NEON) || defined(__ARM_NEON__)
         #elif DETECT_OS_FREEBSD && defined(HAVE_ELF_AUX_INFO)
      unsigned long hwcap = 0;
   elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));
   if (hwcap & HWCAP_NEON)
      #elif defined(HAS_ANDROID_CPUFEATURES)
      AndroidCpuFamily cpu_family = android_getCpuFamily();
            if (cpu_family == ANDROID_CPU_FAMILY_ARM) {
      if (cpu_features & ANDROID_CPU_ARM_FEATURE_NEON)
         #elif DETECT_OS_LINUX
      Elf32_auxv_t aux;
            fd = open("/proc/self/auxv", O_RDONLY | O_CLOEXEC);
   if (fd >= 0) {
      while (read(fd, &aux, sizeof(Elf32_auxv_t)) == sizeof(Elf32_auxv_t)) {
                        util_cpu_caps.has_neon = (hwcap >> 12) & 1;
         }
         #endif /* DETECT_OS_LINUX */
   }
      #elif DETECT_ARCH_AARCH64
   static void
   check_os_arm_support(void)
   {
         }
   #endif /* DETECT_ARCH_ARM || DETECT_ARCH_AARCH64 */
      #if DETECT_ARCH_MIPS64
   static void
   check_os_mips64_support(void)
   {
   #if DETECT_OS_LINUX
      Elf64_auxv_t aux;
            fd = open("/proc/self/auxv", O_RDONLY | O_CLOEXEC);
   if (fd >= 0) {
      while (read(fd, &aux, sizeof(Elf64_auxv_t)) == sizeof(Elf64_auxv_t)) {
                        util_cpu_caps.has_msa = (hwcap >> 1) & 1;
         }
         #endif /* DETECT_OS_LINUX */
   }
   #endif /* DETECT_ARCH_MIPS64 */
         static void
   get_cpu_topology(void)
   {
      /* Default. This is OK if L3 is not present or there is only one. */
                  #if DETECT_OS_LINUX
      uint64_t big_cap = 0;
   unsigned num_big_cpus = 0;
   uint64_t *caps = malloc(sizeof(uint64_t) * util_cpu_caps.max_cpus);
   bool fail = false;
   for (unsigned i = 0; caps && i < util_cpu_caps.max_cpus; i++) {
      char name[PATH_MAX];
   snprintf(name, sizeof(name), "/sys/devices/system/cpu/cpu%u/cpu_capacity", i);
   size_t size = 0;
   char *cap = os_read_file(name, &size);
   if (!cap) {
      num_big_cpus = 0;
   fail = true;
      }
   errno = 0;
   caps[i] = strtoull(cap, NULL, 10);
   free(cap);
   if (errno) {
      fail = true;
      }
      }
   if (!fail) {
      for (unsigned i = 0; caps && i < util_cpu_caps.max_cpus; i++) {
      if (caps[i] >= big_cap / 2)
         }
   free(caps);
      #endif
      #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      /* AMD Zen */
   if (util_cpu_caps.family >= CPU_AMD_ZEN1_ZEN2 &&
      util_cpu_caps.family < CPU_AMD_LAST) {
            uint32_t saved_mask[UTIL_MAX_CPUS / 32] = {0};
   uint32_t mask[UTIL_MAX_CPUS / 32] = {0};
            uint32_t L3_found[UTIL_MAX_CPUS] = {0};
   uint32_t num_L3_caches = 0;
            /* Query APIC IDs from each CPU core.
   *
   * An APIC ID is a logical ID of the CPU with respect to the cache
   * hierarchy, meaning that consecutive APIC IDs are neighbours in
   * the hierarchy, e.g. sharing the same cache.
   *
   * For example, CPU 0 can have APIC ID 0 and CPU 12 can have APIC ID 1,
   * which means that both CPU 0 and 12 are next to each other.
   * (e.g. they are 2 threads belonging to 1 SMT2 core)
   *
   * We need to find out which CPUs share the same L3 cache and they can
   * be all over the place.
   *
   * Querying the APIC ID can only be done by pinning the current thread
   * to each core. The original affinity mask is saved.
   *
   * Loop over all possible CPUs even though some may be offline.
   */
   for (int16_t i = 0; i < util_cpu_caps.max_cpus && i < UTIL_MAX_CPUS; i++) {
                        /* The assumption is that trying to bind the thread to a CPU that is
   * offline will fail.
   */
   if (util_set_current_thread_affinity(mask,
                           /* Query the APIC ID of the current core. */
                  /* Query the total core count for the CPU */
   uint32_t core_count = 1;
                           /* Query the L3 cache count. */
   cpuid_count(0x8000001D, 3, regs);
                                 unsigned local_core_id = apic_id & (core_count - 1);
   #define L3_ID(p, i) (p << 16 | i << 1 | 1);
                  unsigned l3_id = L3_ID(phys_id, local_l3_cache_index);
   int idx = -1;
   for (unsigned c = 0; c < num_L3_caches; c++) {
      if (L3_found[c] == l3_id) {
      idx = c;
         }
   if (idx == -1) {
      idx = num_L3_caches;
   L3_found[num_L3_caches++] = l3_id;
   L3_affinity_masks = realloc(L3_affinity_masks, sizeof(util_affinity_mask) * num_L3_caches);
   if (!L3_affinity_masks)
            }
               }
               util_cpu_caps.num_L3_caches = num_L3_caches;
            if (saved) {
      if (debug_get_option_dump_cpu()) {
      fprintf(stderr, "CPU <-> L3 cache mapping:\n");
   for (unsigned i = 0; i < util_cpu_caps.num_L3_caches; i++) {
      fprintf(stderr, "  - L3 %u mask = ", i);
   for (int j = util_cpu_caps.max_cpus - 1; j >= 0; j -= 32)
                        /* Restore the original affinity mask. */
   util_set_current_thread_affinity(saved_mask, NULL,
      } else {
      if (debug_get_option_dump_cpu())
            #endif
   }
      static
   void check_cpu_caps_override(void)
   {
         #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      if (debug_get_bool_option("GALLIUM_NOSSE", false)) {
            #ifdef DEBUG
      /* For simulating less capable machines */
   if (debug_get_bool_option("LP_FORCE_SSE2", false)) {
            #endif
   #endif /* DETECT_ARCH_X86 || DETECT_ARCH_X86_64 */
            #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
         if (!strcmp(override_cpu_caps, "nosse")) {
         } else if (!strcmp(override_cpu_caps, "sse")) {
         } else if (!strcmp(override_cpu_caps, "sse2")) {
         } else if (!strcmp(override_cpu_caps, "sse3")) {
         } else if (!strcmp(override_cpu_caps, "ssse3")) {
         } else if (!strcmp(override_cpu_caps, "sse4.1")) {
         } else if (!strcmp(override_cpu_caps, "avx")) {
         #endif /* DETECT_ARCH_X86 || DETECT_ARCH_X86_64 */
            #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      if (!util_cpu_caps.has_sse) {
         }
   if (!util_cpu_caps.has_sse2) {
         }
   if (!util_cpu_caps.has_sse3) {
         }
   if (!util_cpu_caps.has_ssse3) {
         }
   if (!util_cpu_caps.has_sse4_1) {
      util_cpu_caps.has_sse4_2 = 0;
      }
   if (!util_cpu_caps.has_avx) {
      util_cpu_caps.has_avx2 = 0;
   util_cpu_caps.has_f16c = 0;
   util_cpu_caps.has_fma = 0;
      }
   if (!util_cpu_caps.has_avx512f) {
      /* avx512 are cleared */
   util_cpu_caps.has_avx512dq   = 0;
   util_cpu_caps.has_avx512ifma = 0;
   util_cpu_caps.has_avx512pf   = 0;
   util_cpu_caps.has_avx512er   = 0;
   util_cpu_caps.has_avx512cd   = 0;
   util_cpu_caps.has_avx512bw   = 0;
   util_cpu_caps.has_avx512vl   = 0;
         #endif /* DETECT_ARCH_X86 || DETECT_ARCH_X86_64 */
   }
      static
   void check_max_vector_bits(void)
   {
      /* Leave it at 128, even when no SIMD extensions are available.
   * Really needs to be a multiple of 128 so can fit 4 floats.
   */
      #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      if (util_cpu_caps.has_avx512f) {
         } else if (util_cpu_caps.has_avx) {
            #endif
   }
      void _util_cpu_detect_once(void);
      void
   _util_cpu_detect_once(void)
   {
      int available_cpus = 0;
                        #if DETECT_OS_WINDOWS
      {
      SYSTEM_INFO system_info;
   GetSystemInfo(&system_info);
         #elif DETECT_OS_UNIX
   #  if defined(HAS_SCHED_GETAFFINITY)
      {
      /* sched_setaffinity() can be used to further restrict the number of
   * CPUs on which the process can run.  Use sched_getaffinity() to
   * determine the true number of available CPUs.
   *
   * FIXME: The Linux manual page for sched_getaffinity describes how this
   * simple implementation will fail with > 1024 CPUs, and we'll fall back
   * to the _SC_NPROCESSORS_ONLN path.  Support for > 1024 CPUs can be
   * added to this path once someone has such a system for testing.
   */
   cpu_set_t affin;
   if (sched_getaffinity(getpid(), sizeof(affin), &affin) == 0)
         #  endif
         /* Linux, FreeBSD, DragonFly, and Mac OS X should have
   * _SC_NOPROCESSORS_ONLN.  NetBSD and OpenBSD should have HW_NCPUONLINE.
   * This is what FFmpeg uses on those platforms.
      #  if DETECT_OS_BSD && defined(HW_NCPUONLINE)
      if (available_cpus == 0) {
      const int mib[] = { CTL_HW, HW_NCPUONLINE };
   int ncpu;
            sysctl(mib, 2, &ncpu, &len, NULL, 0);
         #  elif defined(_SC_NPROCESSORS_ONLN)
      if (available_cpus == 0) {
      available_cpus = sysconf(_SC_NPROCESSORS_ONLN);
   if (available_cpus == ~0)
         #  elif DETECT_OS_BSD
      if (available_cpus == 0) {
      const int mib[] = { CTL_HW, HW_NCPU };
   int ncpu;
            sysctl(mib, 2, &ncpu, &len, NULL, 0);
         #  endif /* DETECT_OS_BSD */
         /* Determine the maximum number of CPUs configured in the system.  This is
   * used to properly set num_cpu_mask_bits below.  On BSDs that don't have
   * HW_NCPUONLINE, it was not clear whether HW_NCPU is the number of
   * configured or the number of online CPUs.  For that reason, prefer the
   * _SC_NPROCESSORS_CONF path on all BSDs.
      #  if defined(_SC_NPROCESSORS_CONF)
      total_cpus = sysconf(_SC_NPROCESSORS_CONF);
   if (total_cpus == ~0)
      #  elif DETECT_OS_BSD
      {
      const int mib[] = { CTL_HW, HW_NCPU };
   int ncpu;
            sysctl(mib, 2, &ncpu, &len, NULL, 0);
         #  endif /* DETECT_OS_BSD */
   #endif /* DETECT_OS_UNIX */
         util_cpu_caps.nr_cpus = MAX2(1, available_cpus);
            util_cpu_caps.max_cpus = total_cpus;
            /* Make the fallback cacheline size nonzero so that it can be
   * safely passed to align().
   */
         #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      if (has_cpuid()) {
      uint32_t regs[4];
                     /* Get max cpuid level */
            if (regs[0] >= 0x00000001) {
                        util_cpu_caps.x86_cpu_type = (regs2[0] >> 8) & 0xf;
   /* Add "extended family". */
                  switch (util_cpu_caps.x86_cpu_type) {
   case 0x17:
      util_cpu_caps.family = CPU_AMD_ZEN1_ZEN2;
      case 0x18:
      util_cpu_caps.family = CPU_AMD_ZEN_HYGON;
      case 0x19:
      util_cpu_caps.family = CPU_AMD_ZEN3;
      default:
      if (util_cpu_caps.x86_cpu_type > 0x19)
               /* general feature flags */
   util_cpu_caps.has_mmx    = (regs2[3] >> 23) & 1; /* 0x0800000 */
   util_cpu_caps.has_sse    = (regs2[3] >> 25) & 1; /* 0x2000000 */
   util_cpu_caps.has_sse2   = (regs2[3] >> 26) & 1; /* 0x4000000 */
   util_cpu_caps.has_sse3   = (regs2[2] >>  0) & 1; /* 0x0000001 */
   util_cpu_caps.has_ssse3  = (regs2[2] >>  9) & 1; /* 0x0000020 */
   util_cpu_caps.has_sse4_1 = (regs2[2] >> 19) & 1;
   util_cpu_caps.has_sse4_2 = (regs2[2] >> 20) & 1;
   util_cpu_caps.has_popcnt = (regs2[2] >> 23) & 1;
   util_cpu_caps.has_avx    = ((regs2[2] >> 28) & 1) && // AVX
               util_cpu_caps.has_f16c   = ((regs2[2] >> 29) & 1) && util_cpu_caps.has_avx;
      #if DETECT_ARCH_X86_64
         #else
               #endif
               cacheline = ((regs2[1] >> 8) & 0xFF) * 8;
   if (cacheline > 0)
      }
   if (regs[0] >= 0x00000007) {
      uint32_t regs7[4];
   cpuid_count(0x00000007, 0x00000000, regs7);
   util_cpu_caps.has_clflushopt = (regs7[1] >> 23) & 1;
                     // check for avx512
   if (xgetbv() & (0x7 << 5)) { // OPMASK: upper-256 enabled by OS
      util_cpu_caps.has_avx512f    = (regs7[1] >> 16) & 1;
   util_cpu_caps.has_avx512dq   = (regs7[1] >> 17) & 1;
   util_cpu_caps.has_avx512ifma = (regs7[1] >> 21) & 1;
   util_cpu_caps.has_avx512pf   = (regs7[1] >> 26) & 1;
   util_cpu_caps.has_avx512er   = (regs7[1] >> 27) & 1;
   util_cpu_caps.has_avx512cd   = (regs7[1] >> 28) & 1;
   util_cpu_caps.has_avx512bw   = (regs7[1] >> 30) & 1;
   util_cpu_caps.has_avx512vl   = (regs7[1] >> 31) & 1;
                     if (regs[1] == 0x756e6547 && regs[2] == 0x6c65746e && regs[3] == 0x49656e69) {
      /* GenuineIntel */
                                             util_cpu_caps.has_mmx  |= (regs2[3] >> 23) & 1;
   util_cpu_caps.has_mmx2 |= (regs2[3] >> 22) & 1;
                  util_cpu_caps.has_xop = util_cpu_caps.has_avx &&
               if (regs[0] >= 0x80000006) {
      /* should we really do this if the clflush size above worked? */
   unsigned int cacheline;
   cpuid(0x80000006, regs2);
   cacheline = regs2[2] & 0xFF;
   if (cacheline > 0)
            #endif /* DETECT_ARCH_X86 || DETECT_ARCH_X86_64 */
      #if DETECT_ARCH_ARM || DETECT_ARCH_AARCH64
         #endif
      #if DETECT_ARCH_PPC
         #endif /* DETECT_ARCH_PPC */
      #if DETECT_ARCH_MIPS64
         #endif /* DETECT_ARCH_MIPS64 */
      #if DETECT_ARCH_S390
         #endif
                  /* max_vector_bits should be checked after cpu caps override */
                     if (debug_get_option_dump_cpu()) {
               printf("util_cpu_caps.x86_cpu_type = %u\n", util_cpu_caps.x86_cpu_type);
            printf("util_cpu_caps.has_mmx = %u\n", util_cpu_caps.has_mmx);
   printf("util_cpu_caps.has_mmx2 = %u\n", util_cpu_caps.has_mmx2);
   printf("util_cpu_caps.has_sse = %u\n", util_cpu_caps.has_sse);
   printf("util_cpu_caps.has_sse2 = %u\n", util_cpu_caps.has_sse2);
   printf("util_cpu_caps.has_sse3 = %u\n", util_cpu_caps.has_sse3);
   printf("util_cpu_caps.has_ssse3 = %u\n", util_cpu_caps.has_ssse3);
   printf("util_cpu_caps.has_sse4_1 = %u\n", util_cpu_caps.has_sse4_1);
   printf("util_cpu_caps.has_sse4_2 = %u\n", util_cpu_caps.has_sse4_2);
   printf("util_cpu_caps.has_avx = %u\n", util_cpu_caps.has_avx);
   printf("util_cpu_caps.has_avx2 = %u\n", util_cpu_caps.has_avx2);
   printf("util_cpu_caps.has_f16c = %u\n", util_cpu_caps.has_f16c);
   printf("util_cpu_caps.has_popcnt = %u\n", util_cpu_caps.has_popcnt);
   printf("util_cpu_caps.has_3dnow = %u\n", util_cpu_caps.has_3dnow);
   printf("util_cpu_caps.has_3dnow_ext = %u\n", util_cpu_caps.has_3dnow_ext);
   printf("util_cpu_caps.has_xop = %u\n", util_cpu_caps.has_xop);
   printf("util_cpu_caps.has_altivec = %u\n", util_cpu_caps.has_altivec);
   printf("util_cpu_caps.has_vsx = %u\n", util_cpu_caps.has_vsx);
   printf("util_cpu_caps.has_neon = %u\n", util_cpu_caps.has_neon);
   printf("util_cpu_caps.has_msa = %u\n", util_cpu_caps.has_msa);
   printf("util_cpu_caps.has_daz = %u\n", util_cpu_caps.has_daz);
   printf("util_cpu_caps.has_avx512f = %u\n", util_cpu_caps.has_avx512f);
   printf("util_cpu_caps.has_avx512dq = %u\n", util_cpu_caps.has_avx512dq);
   printf("util_cpu_caps.has_avx512ifma = %u\n", util_cpu_caps.has_avx512ifma);
   printf("util_cpu_caps.has_avx512pf = %u\n", util_cpu_caps.has_avx512pf);
   printf("util_cpu_caps.has_avx512er = %u\n", util_cpu_caps.has_avx512er);
   printf("util_cpu_caps.has_avx512cd = %u\n", util_cpu_caps.has_avx512cd);
   printf("util_cpu_caps.has_avx512bw = %u\n", util_cpu_caps.has_avx512bw);
   printf("util_cpu_caps.has_avx512vl = %u\n", util_cpu_caps.has_avx512vl);
   printf("util_cpu_caps.has_avx512vbmi = %u\n", util_cpu_caps.has_avx512vbmi);
   printf("util_cpu_caps.has_clflushopt = %u\n", util_cpu_caps.has_clflushopt);
   printf("util_cpu_caps.num_L3_caches = %u\n", util_cpu_caps.num_L3_caches);
      }
            /* This must happen at the end as it's used to guard everything else */
      }
