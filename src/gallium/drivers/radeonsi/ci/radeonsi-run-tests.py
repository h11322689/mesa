   #!/usr/bin/python3
   #
   # Copyright 2021 Advanced Micro Devices, Inc.
   #
   # SPDX-License-Identifier: MIT
   #
      import os
   import sys
   import argparse
   import subprocess
   import shutil
   from datetime import datetime
   import tempfile
   import itertools
   import filecmp
   import multiprocessing
   import csv
         def print_red(txt, end_line=True, prefix=None):
      if prefix:
                  def print_yellow(txt, end_line=True, prefix=None):
      if prefix:
                  def print_green(txt, end_line=True, prefix=None):
      if prefix:
                  parser = argparse.ArgumentParser(
      description="radeonsi tester",
      )
   parser.add_argument(
      "--jobs",
   "-j",
   type=int,
   help="Number of processes/threads to use.",
      )
      # The path to above the mesa directory, i.e. ../../../../../..
   path_above_mesa = os.path.realpath(os.path.join(os.path.dirname(__file__), *['..'] * 6))
      parser.add_argument("--piglit-path", type=str, help="Path to piglit source folder.")
   parser.add_argument("--glcts-path", type=str, help="Path to GLCTS source folder.")
   parser.add_argument("--escts-path", type=str, help="Path to GLES CTS source folder.")
   parser.add_argument("--deqp-path", type=str, help="Path to dEQP source folder.")
   parser.add_argument(
      "--parent-path",
   type=str,
   help="Path to folder containing piglit/GLCTS and dEQP source folders.",
      )
   parser.add_argument("--verbose", "-v", action="count", default=0)
   parser.add_argument(
      "--include-tests",
   "-t",
   action="append",
   dest="include_tests",
   default=[],
      )
   parser.add_argument(
      "--baseline",
   dest="baseline",
   help="Folder containing expected results files",
      )
   parser.add_argument(
         )
   parser.add_argument(
         )
   parser.add_argument(
         )
   parser.add_argument(
         )
   parser.add_argument(
         )
   parser.add_argument(
      "--no-deqp-egl",
   dest="deqp_egl",
   help="Disable dEQP-EGL tests",
      )
   parser.add_argument(
      "--no-deqp-gles2",
   dest="deqp_gles2",
   help="Disable dEQP-gles2 tests",
      )
   parser.add_argument(
      "--no-deqp-gles3",
   dest="deqp_gles3",
   help="Disable dEQP-gles3 tests",
      )
   parser.add_argument(
      "--no-deqp-gles31",
   dest="deqp_gles31",
   help="Disable dEQP-gles31 tests",
      )
   parser.set_defaults(piglit=True)
   parser.set_defaults(glcts=True)
   parser.set_defaults(escts=True)
   parser.set_defaults(deqp=True)
   parser.set_defaults(deqp_egl=True)
   parser.set_defaults(deqp_gles2=True)
   parser.set_defaults(deqp_gles3=True)
   parser.set_defaults(deqp_gles31=True)
   parser.set_defaults(slow=False)
      parser.add_argument(
      "output_folder",
   nargs="?",
   help="Output folder (logs, etc)",
   default=os.path.join(
      # Default is ../../../../../../test-results/datetime
   os.path.join(path_above_mesa, 'test-results',
         )
      available_gpus = []
   for f in os.listdir("/dev/dri/by-path"):
      idx = f.find("-render")
   if idx < 0:
         # gbm name is the full path, but DRI_PRIME expects a different
   # format
   available_gpus += [
      (
         os.path.join("/dev/dri/by-path", f),
            parser.add_argument(
      "--gpu",
   type=int,
   dest="gpu",
   default=0,
      )
      args = parser.parse_args(sys.argv[1:])
   piglit_path = args.piglit_path
   glcts_path = args.glcts_path
   escts_path = args.escts_path
   deqp_path = args.deqp_path
      if args.parent_path:
      if args.piglit_path or args.glcts_path or args.deqp_path:
      parser.print_help()
      piglit_path = os.path.join(args.parent_path, "piglit")
   glcts_path = os.path.join(args.parent_path, "glcts")
   escts_path = os.path.join(args.parent_path, "escts")
      else:
      if not args.piglit_path or not args.glcts_path or not args.escts_path or not args.deqp_path:
      parser.print_help()
      base = args.baseline
   skips = os.path.join(os.path.dirname(__file__), "skips.csv")
      env = os.environ.copy()
      if "DISPLAY" not in env:
      print_red("DISPLAY environment variable missing.")
      p = subprocess.run(
         )
   for line in p.stdout.decode().split("\n"):
      if line.find("deqp-runner") >= 0:
      s = line.split(" ")[1].split(".")
   if args.verbose > 1:
         # We want at least 0.9.0
   if not (int(s[0]) > 0 or int(s[1]) >= 9):
            env["PIGLIT_PLATFORM"] = "gbm"
      if "DRI_PRIME" in env:
      print("Don't use DRI_PRIME. Instead use --gpu N")
         assert "gpu" in args, "--gpu defaults to 0"
      gpu_device = available_gpus[args.gpu][1]
   env["DRI_PRIME"] = gpu_device
   env["WAFFLE_GBM_DEVICE"] = available_gpus[args.gpu][0]
      # Use piglit's glinfo to determine the GPU name
   gpu_name = "unknown"
   gpu_name_full = ""
   gfx_level = -1
      env["AMD_DEBUG"] = "info"
   p = subprocess.run(
      ["./glinfo"],
   capture_output="True",
   cwd=os.path.join(piglit_path, "bin"),
   check=True,
      )
   del env["AMD_DEBUG"]
   for line in p.stdout.decode().split("\n"):
      if "GL_RENDER" in line:
      line = line.split("=")[1]
   gpu_name_full = "(".join(line.split("(")[:-1]).strip()
   gpu_name = line.replace("(TM)", "").split("(")[1].split(",")[0].lower()
      elif "gfx_level" in line:
         output_folder = args.output_folder
   print_green("Tested GPU: '{}' ({}) {}".format(gpu_name_full, gpu_name, gpu_device))
   print_green("Output folder: '{}'".format(output_folder))
      count = 1
   while os.path.exists(output_folder):
      output_folder = "{}.{}".format(os.path.abspath(args.output_folder), count)
         os.makedirs(output_folder, exist_ok=True)
      logfile = open(os.path.join(output_folder, "{}-run-tests.log".format(gpu_name)), "w")
      spin = itertools.cycle("-\\|/")
      shutil.copy(skips, output_folder)
   skips = os.path.join(output_folder, "skips.csv")
   if not args.slow:
      # Exclude these 4 tests slow tests
   with open(skips, "a") as f:
      print("KHR-GL46.copy_image.functional", file=f)
   print("KHR-GL46.texture_swizzle.smoke", file=f)
   print(
         "KHR-GL46.tessellation_shader.tessellation_control_to_tessellation_evaluation.gl_MaxPatchVertices_Position_PointSize",
   )
         def gfx_level_to_str(cl):
      supported = ["gfx6", "gfx7", "gfx8", "gfx9", "gfx10", "gfx10_3", "gfx11"]
   if 8 <= cl and cl < 8 + len(supported):
                  def run_cmd(args, verbosity):
      if verbosity > 1:
      print_yellow(
         "| Command line argument '"
   + " ".join(['"{}"'.format(a) for a in args])
      start = datetime.now()
   proc = subprocess.Popen(
         )
   while True:
      line = proc.stdout.readline().decode()
   if verbosity > 0:
         if "ERROR" in line:
         else:
   else:
         sys.stdout.write(next(spin))
                     if proc.poll() is not None:
      proc.wait()
            if verbosity == 0:
            print_yellow(
      "Completed in {} seconds".format(int((end - start).total_seconds())),
               def verify_results(results):
      with open(results) as file:
      if len(file.readlines()) == 0:
      print_red("New results (fails or pass). Check {}".format(results))
            def parse_test_filters(include_tests):
      cmd = []
   for t in include_tests:
      if os.path.exists(t):
         with open(t, "r") as file:
      for row in csv.reader(file, delimiter=","):
      if not row or row[0][0] == "#":
   else:
               def select_baseline(basepath, gfx_level, gpu_name):
               # select the best baseline we can find
   # 1. exact match
   exact = os.path.join(base, "{}-{}-fail.csv".format(gfx_level_str, gpu_name))
   if os.path.exists(exact):
         # 2. any baseline with the same gfx_level
   while gfx_level >= 8:
      for subdir, dirs, files in os.walk(basepath):
         for file in files:
         # No match. Try an earlier class
   gfx_level = gfx_level - 1
                  success = True
   filters_args = parse_test_filters(args.include_tests)
   baseline = select_baseline(base, gfx_level, gpu_name)
   flakes = [
      f
   for f in (
      os.path.join(base, g)
   for g in [
         "radeonsi-flakes.csv",
      )
      ]
   flakes_args = []
   for f in flakes:
            if os.path.exists(baseline):
         if flakes_args:
            # piglit test
   if args.piglit:
      out = os.path.join(output_folder, "piglit")
   print_yellow("Running piglit tests", args.verbose > 0)
   cmd = [
      "piglit-runner",
   "run",
   "--piglit-folder",
   piglit_path,
   "--profile",
   "quick",
   "--output",
   out,
   "--process-isolation",
   "--timeout",
   "300",
   "--jobs",
   str(args.jobs),
   "--skips",
   skips,
   "--skips",
               if os.path.exists(baseline):
                     if not verify_results(os.path.join(out, "failures.csv")):
         deqp_args = "-- --deqp-surface-width=256 --deqp-surface-height=256 --deqp-gl-config-name=rgba8888d24s8ms0 --deqp-visibility=hidden".split(
         )
      # glcts test
   if args.glcts:
      out = os.path.join(output_folder, "glcts")
   print_yellow("Running  GLCTS tests", args.verbose > 0)
            cmd = [
      "deqp-runner",
   "run",
   "--tests-per-group",
   "100",
   "--deqp",
   "{}/build/external/openglcts/modules/glcts".format(glcts_path),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gl/khronos_mustpass/4.6.1.x/gl46-master.txt".format(
         ),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gl/khronos_mustpass_single/4.6.1.x/gl46-khr-single.txt".format(
         ),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gl/khronos_mustpass/4.6.1.x/gl46-gtf-master.txt".format(
         ),
   "--output",
   out,
   "--skips",
   skips,
   "--jobs",
   str(args.jobs),
   "--timeout",
               if os.path.exists(baseline):
                           if not verify_results(os.path.join(out, "failures.csv")):
         # escts test
   if args.escts:
      out = os.path.join(output_folder, "escts")
   print_yellow("Running  ESCTS tests", args.verbose > 0)
            cmd = [
      "deqp-runner",
   "run",
   "--tests-per-group",
   "100",
   "--deqp",
   "{}/build/external/openglcts/modules/glcts".format(escts_path),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gles/khronos_mustpass/3.2.6.x/gles2-khr-master.txt".format(
         ),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gles/khronos_mustpass/3.2.6.x/gles3-khr-master.txt".format(
         ),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gles/khronos_mustpass/3.2.6.x/gles31-khr-master.txt".format(
         ),
   "--caselist",
   "{}/build/external/openglcts/modules/gl_cts/data/mustpass/gles/khronos_mustpass/3.2.6.x/gles32-khr-master.txt".format(
         ),
   "--output",
   out,
   "--skips",
   skips,
   "--jobs",
   str(args.jobs),
   "--timeout",
               if os.path.exists(baseline):
                           if not verify_results(os.path.join(out, "failures.csv")):
         if args.deqp:
               # Generate a test-suite file
   out = os.path.join(output_folder, "deqp")
   suite_filename = os.path.join(output_folder, "deqp-suite.toml")
   suite = open(suite_filename, "w")
            deqp_tests = {
      "egl": args.deqp_egl,
   "gles2": args.deqp_gles2,
   "gles3": args.deqp_gles3,
               for k in deqp_tests:
      if not deqp_tests[k]:
            suite.write("[[deqp]]\n")
   suite.write(
         'deqp = "{}"\n'.format(
         )
   suite.write(
         'caselists = ["{}"]\n'.format(
         )
   if os.path.exists(baseline):
         suite.write('skips = ["{}"]\n'.format(skips))
   suite.write("deqp_args = [\n")
   for a in deqp_args[1:-1]:
         suite.write('    "{}"\n'.format(deqp_args[-1]))
                  cmd = [
      "deqp-runner",
   "suite",
   "--jobs",
   str(args.jobs),
   "--output",
   os.path.join(output_folder, "deqp"),
   "--suite",
                        if not verify_results(os.path.join(out, "failures.csv")):
         sys.exit(0 if success else 1)
