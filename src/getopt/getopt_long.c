   /*	$OpenBSD: getopt_long.c,v 1.24 2010/07/22 19:31:53 blambert Exp $	*/
   /*	$NetBSD: getopt_long.c,v 1.15 2002/01/31 22:43:40 tv Exp $	*/
      /*
   * Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
   *
   * Permission to use, copy, modify, and distribute this software for any
   * purpose with or without fee is hereby granted, provided that the above
   * copyright notice and this permission notice appear in all copies.
   *
   * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
   *
   * Sponsored in part by the Defense Advanced Research Projects
   * Agency (DARPA) and Air Force Research Laboratory, Air Force
   * Materiel Command, USAF, under agreement number F39502-99-1-0512.
   */
   /*-
   * Copyright (c) 2000 The NetBSD Foundation, Inc.
   * All rights reserved.
   *
   * This code is derived from software contributed to The NetBSD Foundation
   * by Dieter Baron and Thomas Klausner.
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions
   * are met:
   * 1. Redistributions of source code must retain the above copyright
   *    notice, this list of conditions and the following disclaimer.
   * 2. Redistributions in binary form must reproduce the above copyright
   *    notice, this list of conditions and the following disclaimer in the
   *    documentation and/or other materials provided with the distribution.
   *
   * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
   * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
   * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   * POSSIBILITY OF SUCH DAMAGE.
   */
      #include <errno.h>
   #include <getopt.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
      int	opterr = 1;		/* if error message should be printed */
   int	optind = 1;		/* index into parent argv vector */
   int	optopt = '?';		/* character checked for validity */
   int	optreset;		/* reset getopt */
   char    *optarg;		/* argument associated with option */
      #define PRINT_ERROR	((opterr) && (*options != ':'))
      #define FLAG_PERMUTE	0x01	/* permute non-options to the end of argv */
   #define FLAG_ALLARGS	0x02	/* treat non-options as args to option "-1" */
   #define FLAG_LONGONLY	0x04	/* operate as getopt_long_only */
      /* return values */
   #define	BADCH		(int)'?'
   #define	BADARG		((*options == ':') ? (int)':' : (int)'?')
   #define	INORDER 	(int)1
      #define	EMSG		""
      static int getopt_internal(int, char * const *, const char *,
         static int parse_long_options(char * const *, const char *,
         static int gcd(int, int);
   static void permute_args(int, int, int, char * const *);
      static char *place = EMSG; /* option letter processing */
      /* XXX: set optreset to 1 rather than these two */
   static int nonopt_start = -1; /* first non option argument (for permute) */
   static int nonopt_end = -1;   /* first option after non options (for permute) */
      /* Error messages */
   static const char recargchar[] = "option requires an argument -- %c";
   static const char recargstring[] = "option requires an argument -- %s";
   static const char ambig[] = "ambiguous option -- %.*s";
   static const char noarg[] = "option doesn't take an argument -- %.*s";
   static const char illoptchar[] = "unknown option -- %c";
   static const char illoptstring[] = "unknown option -- %s";
      /*
   * Compute the greatest common divisor of a and b.
   */
   static int
   gcd(int a, int b)
   {
   int c;
      c = a % b;
   while (c != 0) {
   a = b;
   b = c;
   c = a % b;
   }
      return (b);
   }
      /*
   * Exchange the block from nonopt_start to nonopt_end with the block
   * from nonopt_end to opt_end (keeping the same order of arguments
   * in each block).
   */
   static void
   permute_args(int panonopt_start, int panonopt_end, int opt_end,
   char * const *nargv)
   {
   int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
   char *swap;
      /*
   * compute lengths of blocks and number and size of cycles
   */
   nnonopts = panonopt_end - panonopt_start;
   nopts = opt_end - panonopt_end;
   ncycle = gcd(nnonopts, nopts);
   cyclelen = (opt_end - panonopt_start) / ncycle;
      for (i = 0; i < ncycle; i++) {
   cstart = panonopt_end+i;
   pos = cstart;
   for (j = 0; j < cyclelen; j++) {
      if (pos >= panonopt_end)
   pos -= nnonopts;
   else
   pos += nopts;
   swap = nargv[pos];
   /* LINTED const cast */
   ((char **) nargv)[pos] = nargv[cstart];
   /* LINTED const cast */
      }
   }
   }
      /*
   * parse_long_options --
   *	Parse long options in argc/argv argument vector.
   * Returns -1 if short_too is set and the option does not match long_options.
   */
   static int
   parse_long_options(char * const *nargv, const char *options,
   const struct option *long_options, int *idx, int short_too)
   {
   char *current_argv, *has_equal;
   size_t current_argv_len;
   int i, match;
      current_argv = place;
   match = -1;
      optind++;
      if ((has_equal = strchr(current_argv, '=')) != NULL) {
   /* argument found (--option=arg) */
   current_argv_len = has_equal - current_argv;
   has_equal++;
   } else
   current_argv_len = strlen(current_argv);
      for (i = 0; long_options[i].name; i++) {
   /* find matching long option */
   if (strncmp(current_argv, long_options[i].name,
                  if (strlen(long_options[i].name) == current_argv_len) {
      /* exact match */
   match = i;
      }
   /*
      * If this is a known short option, don't allow
   * a partial match of a single character.
      if (short_too && current_argv_len == 1)
            if (match == -1)	/* partial match */
         else {
      /* ambiguous abbreviation */
   if (PRINT_ERROR)
   fprintf(stderr, ambig, (int)current_argv_len,
         optopt = 0;
      }
   }
   if (match != -1) {		/* option found */
   if (long_options[match].has_arg == no_argument
            if (PRINT_ERROR)
   fprintf(stderr, noarg, (int)current_argv_len,
         /*
   * XXX: GNU sets optopt to val regardless of flag
   */
   if (long_options[match].flag == NULL)
   optopt = long_options[match].val;
   else
   optopt = 0;
      }
   if (long_options[match].has_arg == required_argument ||
            if (has_equal)
   optarg = has_equal;
   else if (long_options[match].has_arg ==
         /*
   * optional argument doesn't use next nargv
   */
   optarg = nargv[optind++];
      }
   if ((long_options[match].has_arg == required_argument)
            /*
   * Missing argument; leading ':' indicates no error
   * should be generated.
   */
   if (PRINT_ERROR)
   fprintf(stderr, recargstring,
         /*
   * XXX: GNU sets optopt to val regardless of flag
   */
   if (long_options[match].flag == NULL)
   optopt = long_options[match].val;
   else
   optopt = 0;
   --optind;
      }
   } else {			/* unknown option */
   if (short_too) {
      --optind;
      }
   if (PRINT_ERROR)
         optopt = 0;
   return (BADCH);
   }
   if (idx)
   *idx = match;
   if (long_options[match].flag) {
   *long_options[match].flag = long_options[match].val;
   return (0);
   } else
   return (long_options[match].val);
   }
      /*
   * getopt_internal --
   *	Parse argc/argv argument vector.  Called by user level routines.
   */
   static int
   getopt_internal(int nargc, char * const *nargv, const char *options,
   const struct option *long_options, int *idx, int flags)
   {
   char *oli;				/* option letter list index */
   int optchar, short_too;
   static int posixly_correct = -1;
      if (options == NULL)
   return (-1);
      /*
   * Disable GNU extensions if POSIXLY_CORRECT is set or options
   * string begins with a '+'.
   */
   if (posixly_correct == -1)
   posixly_correct = (getenv("POSIXLY_CORRECT") != NULL);
   if (posixly_correct || *options == '+')
   flags &= ~FLAG_PERMUTE;
   else if (*options == '-')
   flags |= FLAG_ALLARGS;
   if (*options == '+' || *options == '-')
   options++;
      /*
   * XXX Some GNU programs (like cvs) set optind to 0 instead of
   * XXX using optreset.  Work around this braindamage.
   */
   if (optind == 0)
   optind = optreset = 1;
      optarg = NULL;
   if (optreset)
   nonopt_start = nonopt_end = -1;
   start:
   if (optreset || !*place) {		/* update scanning pointer */
   optreset = 0;
   if (optind >= nargc) {          /* end of argument vector */
      place = EMSG;
   if (nonopt_end != -1) {
   /* do permutation, if we have to */
   permute_args(nonopt_start, nonopt_end,
         optind -= nonopt_end - nonopt_start;
   }
   else if (nonopt_start != -1) {
   /*
   * If we skipped non-options, set optind
   * to the first of them.
   */
   optind = nonopt_start;
   }
   nonopt_start = nonopt_end = -1;
      }
   if (*(place = nargv[optind]) != '-' ||
            place = EMSG;		/* found non-option */
   if (flags & FLAG_ALLARGS) {
   /*
   * GNU extension:
   * return non-option as argument to option 1
   */
   optarg = nargv[optind++];
   return (INORDER);
   }
   if (!(flags & FLAG_PERMUTE)) {
   /*
   * If no permutation wanted, stop parsing
   * at first non-option.
   */
   return (-1);
   }
   /* do permutation */
   if (nonopt_start == -1)
   nonopt_start = optind;
   else if (nonopt_end != -1) {
   permute_args(nonopt_start, nonopt_end,
         nonopt_start = optind -
         nonopt_end = -1;
   }
   optind++;
   /* process next argument */
      }
   if (nonopt_start != -1 && nonopt_end == -1)
            /*
      * If we have "-" do nothing, if "--" we are done.
      if (place[1] != '\0' && *++place == '-' && place[1] == '\0') {
      optind++;
   place = EMSG;
   /*
   * We found an option (--), so if we skipped
   * non-options, we have to permute.
   */
   if (nonopt_end != -1) {
   permute_args(nonopt_start, nonopt_end,
         optind -= nonopt_end - nonopt_start;
   }
   nonopt_start = nonopt_end = -1;
      }
   }
      /*
   * Check long options if:
   *  1) we were passed some
   *  2) the arg is not just "-"
   *  3) either the arg starts with -- we are getopt_long_only()
   */
   if (long_options != NULL && place != nargv[optind] &&
         short_too = 0;
   if (*place == '-')
         else if (*place != ':' && strchr(options, *place) != NULL)
            optchar = parse_long_options(nargv, options, long_options,
         if (optchar != -1) {
      place = EMSG;
      }
   }
      if ((optchar = (int)*place++) == (int)':' ||
      (optchar == (int)'-' && *place != '\0') ||
      /*
      * If the user specified "-" and  '-' isn't listed in
   * options, return -1 (non-option) as per POSIX.
   * Otherwise, it is an unknown option character (or ':').
      if (optchar == (int)'-' && *place == '\0')
         if (!*place)
         if (PRINT_ERROR)
         optopt = optchar;
   return (BADCH);
   }
   if (long_options != NULL && optchar == 'W' && oli[1] == ';') {
   /* -W long-option */
   if (*place)			/* no space */
         else if (++optind >= nargc) {	/* no arg */
      place = EMSG;
   if (PRINT_ERROR)
   fprintf(stderr, recargchar, optchar);
   optopt = optchar;
      } else				/* white space */
         optchar = parse_long_options(nargv, options, long_options,
         place = EMSG;
   return (optchar);
   }
   if (*++oli != ':') {			/* doesn't take argument */
   if (!*place)
         } else {				/* takes (optional) argument */
   optarg = NULL;
   if (*place)			/* no white space */
         else if (oli[1] != ':') {	/* arg not optional */
      if (++optind >= nargc) {	/* no arg */
   place = EMSG;
   if (PRINT_ERROR)
   fprintf(stderr, recargchar, optchar);
   optopt = optchar;
   return (BADARG);
   } else
      }
   place = EMSG;
   ++optind;
   }
   /* dump back option letter */
   return (optchar);
   }
      /*
   * getopt --
   *	Parse argc/argv argument vector.
   *
   * [eventually this will replace the BSD getopt]
   */
   int
   getopt(int nargc, char * const *nargv, const char *options)
   {
      /*
   * We don't pass FLAG_PERMUTE to getopt_internal() since
   * the BSD getopt(3) (unlike GNU) has never done this.
   *
   * Furthermore, since many privileged programs call getopt()
   * before dropping privileges it makes sense to keep things
   * as simple (and bug-free) as possible.
   */
   return (getopt_internal(nargc, nargv, options, NULL, NULL, 0));
   }
      /*
   * getopt_long --
   *	Parse argc/argv argument vector.
   */
   int
   getopt_long(int nargc, char * const *nargv, const char *options,
         {
      return (getopt_internal(nargc, nargv, options, long_options, idx,
         }
      /*
   * getopt_long_only --
   *	Parse argc/argv argument vector.
   */
   int
   getopt_long_only(int nargc, char * const *nargv, const char *options,
         {
      return (getopt_internal(nargc, nargv, options, long_options, idx,
         }
