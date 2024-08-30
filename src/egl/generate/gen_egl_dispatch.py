   #!/usr/bin/env python3
      # (C) Copyright 2016, NVIDIA CORPORATION.
   # All Rights Reserved.
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # on the rights to use, copy, modify, merge, publish, distribute, sub
   # license, and/or sell copies of the Software, and to permit persons to whom
   # the Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   # IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
   #
   # Authors:
   #    Kyle Brenneman <kbrenneman@nvidia.com>
      """
   Generates dispatch functions for EGL.
      The list of functions and arguments is read from the Khronos's XML files, with
   additional information defined in the module eglFunctionList.
   """
      import argparse
   import collections
   import eglFunctionList
   import sys
   import textwrap
      import os
   NEWAPI = os.path.join(os.path.dirname(__file__), "..", "..", "mapi", "new")
   sys.path.insert(0, NEWAPI)
   import genCommon
      def main():
      parser = argparse.ArgumentParser()
   parser.add_argument("target", choices=("header", "source"),
                           xmlFunctions = genCommon.getFunctions(args.xml_files)
   xmlByName = dict((f.name, f) for f in xmlFunctions)
   functions = []
   for (name, eglFunc) in eglFunctionList.EGL_FUNCTIONS:
      func = xmlByName[name]
   eglFunc = fixupEglFunc(func, eglFunc)
         # Sort the function list by name.
            if args.target == "header":
         elif args.target == "source":
               def fixupEglFunc(func, eglFunc):
      result = dict(eglFunc)
   if result.get("prefix") is None:
            if result.get("extension") is not None:
      text = "defined(" + result["extension"] + ")"
         if result["method"] in ("none", "custom"):
            if result["method"] not in ("display", "device", "current"):
            if func.hasReturn():
      if result.get("retval") is None:
               def generateHeader(functions):
      text = textwrap.dedent(r"""
   #ifndef G_EGLDISPATCH_STUBS_H
            #ifdef __cplusplus
   extern "C" {
            #include <EGL/egl.h>
   #include <EGL/eglext.h>
   #include <EGL/eglmesaext.h>
   #include <EGL/eglext_angle.h>
   #include <GL/mesa_glinterop.h>
                     text += "enum {\n"
   for (func, eglFunc) in functions:
      text += generateGuardBegin(func, eglFunc)
   text += "    __EGL_DISPATCH_" + func.name + ",\n"
      text += "    __EGL_DISPATCH_COUNT\n"
            for (func, eglFunc) in functions:
      if eglFunc["inheader"]:
         text += generateGuardBegin(func, eglFunc)
         text += textwrap.dedent(r"""
   #ifdef __cplusplus
   }
   #endif
   #endif // G_EGLDISPATCH_STUBS_H
   """)
         def generateSource(functions):
      # First, sort the function list by name.
   text = ""
   text += '#include "egldispatchstubs.h"\n'
   text += '#include "g_egldispatchstubs.h"\n'
   text += '#include <stddef.h>\n'
            for (func, eglFunc) in functions:
      if eglFunc["method"] not in ("custom", "none"):
         text += generateGuardBegin(func, eglFunc)
         text += "\n"
   text += "const char * const __EGL_DISPATCH_FUNC_NAMES[__EGL_DISPATCH_COUNT + 1] = {\n"
   for (func, eglFunc) in functions:
      text += generateGuardBegin(func, eglFunc)
   text += '    "' + func.name + '",\n'
      text += "    NULL\n"
            text += "const __eglMustCastToProperFunctionPointerType __EGL_DISPATCH_FUNCS[__EGL_DISPATCH_COUNT + 1] = {\n"
   for (func, eglFunc) in functions:
      text += generateGuardBegin(func, eglFunc)
   if eglFunc["method"] != "none":
         else:
            text += "    NULL\n"
                  def generateGuardBegin(func, eglFunc):
      ext = eglFunc.get("extension")
   if ext is not None:
         else:
         def generateGuardEnd(func, eglFunc):
      if eglFunc.get("extension") is not None:
         else:
         def generateDispatchFunc(func, eglFunc):
               if eglFunc.get("static"):
         elif eglFunc.get("public"):
         text += textwrap.dedent(
   r"""
   {f.rt} EGLAPIENTRY {ef[prefix]}{f.name}({f.decArgs})
   {{
                  if func.hasReturn():
            text += "    _pfn_{f.name} _ptr_{f.name} = (_pfn_{f.name}) ".format(f=func)
   if eglFunc["method"] == "current":
            elif eglFunc["method"] in ("display", "device"):
      if eglFunc["method"] == "display":
         lookupFunc = "__eglDispatchFetchByDisplay"
   else:
         assert eglFunc["method"] == "device"
            lookupArg = None
   for arg in func.args:
         if arg.type == lookupType:
         if lookupArg is None:
            text += "{lookupFunc}({lookupArg}, __EGL_DISPATCH_{f.name});\n".format(
      else:
            text += "    if(_ptr_{f.name} != NULL) {{\n".format(f=func)
   text += "        "
   if func.hasReturn():
         text += "_ptr_{f.name}({f.callArgs});\n".format(f=func)
            if func.hasReturn():
         text += "}\n"
         def getDefaultReturnValue(typename):
      if typename.endswith("*"):
         elif typename == "EGLDisplay":
         elif typename == "EGLContext":
         elif typename == "EGLSurface":
         elif typename == "EGLBoolean":
                  if __name__ == "__main__":
         