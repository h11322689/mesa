   #!/usr/bin/env python3
      import argparse
   from generate.eglFunctionList import EGL_FUNCTIONS as GLVND_ENTRYPOINTS
         PREFIX1 = 'EGL_ENTRYPOINT('
   PREFIX2 = 'EGL_ENTRYPOINT2('
   SUFFIX = ')'
         # These entrypoints should *not* be in the GLVND entrypoints
   GLVND_EXCLUDED_ENTRYPOINTS = [
         # EGL_KHR_debug
   'eglDebugMessageControlKHR',
   'eglQueryDebugKHR',
               def check_entrypoint_sorted(entrypoints):
               for i, _ in enumerate(entrypoints):
      # Can't compare the first one with the previous
   if i == 0:
         if entrypoints[i - 1] > entrypoints[i]:
                        def check_glvnd_entrypoints(egl_entrypoints, glvnd_entrypoints):
      print('Checking the GLVND entrypoints against the plain EGL ones...')
            for egl_entrypoint in egl_entrypoints:
      if egl_entrypoint in GLVND_EXCLUDED_ENTRYPOINTS:
         if egl_entrypoint not in glvnd_entrypoints:
               for glvnd_entrypoint in glvnd_entrypoints:
      if glvnd_entrypoint not in egl_entrypoints:
               for glvnd_entrypoint in GLVND_EXCLUDED_ENTRYPOINTS:
      if glvnd_entrypoint in glvnd_entrypoints:
               if success:
         else:
            def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('header')
            with open(args.header) as header:
            entrypoints = []
   for line in lines:
      line = line.strip()
   if line.startswith(PREFIX1):
         assert line.endswith(SUFFIX)
   if line.startswith(PREFIX2):
         assert line.endswith(SUFFIX)
   entrypoint = line[len(PREFIX2):-len(SUFFIX)]
                                 if __name__ == '__main__':
      main()
