   #!/usr/bin/env python3
      # (C) Copyright 2015, NVIDIA CORPORATION.
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
      import collections
   import re
   import sys
   import xml.etree.ElementTree as etree
      import os
   GLAPI = os.path.join(os.path.dirname(__file__), "..", "glapi", "gen")
   sys.path.insert(0, GLAPI)
   import static_data
      MAPI_TABLE_NUM_DYNAMIC = 4096
      _LIBRARY_FEATURE_NAMES = {
      # libGL and libGLdiapatch both include every function.
   "gl" : None,
   "gldispatch" : None,
   "opengl" : frozenset(( "GL_VERSION_1_0", "GL_VERSION_1_1",
      "GL_VERSION_1_2", "GL_VERSION_1_3", "GL_VERSION_1_4", "GL_VERSION_1_5",
   "GL_VERSION_2_0", "GL_VERSION_2_1", "GL_VERSION_3_0", "GL_VERSION_3_1",
   "GL_VERSION_3_2", "GL_VERSION_3_3", "GL_VERSION_4_0", "GL_VERSION_4_1",
      )),
   "glesv1" : frozenset(("GL_VERSION_ES_CM_1_0", "GL_OES_point_size_array")),
   "glesv2" : frozenset(("GL_ES_VERSION_2_0", "GL_ES_VERSION_3_0",
            }
      def getFunctions(xmlFiles):
      """
            xmlFile should be the path to Khronos's gl.xml file. The return value is a
   sequence of FunctionDesc objects, ordered by slot number.
   """
   roots = [ etree.parse(xmlFile).getroot() for xmlFile in xmlFiles ]
         def getFunctionsFromRoots(roots):
      functions = {}
   for root in roots:
      for func in _getFunctionList(root):
               # Sort the function list by name.
            # Lookup for fixed offset/slot functions and use it if available.
   # Assign a slot number to each function. This isn't strictly necessary,
   # since you can just look at the index in the list, but it makes it easier
            next_slot = 0
   for i in range(len(functions)):
               if name in static_data.offsets:
         elif not name.endswith("ARB") and name + "ARB" in static_data.offsets:
         elif not name.endswith("EXT") and name + "EXT" in static_data.offsets:
         else:
                     def getExportNamesFromRoots(target, roots):
      """
   Goes through the <feature> tags from gl.xml and returns a set of OpenGL
            target should be one of "gl", "gldispatch", "opengl", "glesv1", or
   "glesv2".
   """
   featureNames = _LIBRARY_FEATURE_NAMES[target]
   if featureNames is None:
            names = set()
   for root in roots:
      features = []
   for featElem in root.findall("feature"):
         if featElem.get("name") in featureNames:
   for featElem in root.findall("extensions/extension"):
         if featElem.get("name") in featureNames:
   for featElem in features:
                  class FunctionArg(collections.namedtuple("FunctionArg", "type name")):
      @property
   def dec(self):
      """
   Returns a "TYPE NAME" string, suitable for a function prototype.
   """
   rv = str(self.type)
   if not rv.endswith("*"):
         rv += self.name
      class FunctionDesc(collections.namedtuple("FunctionDesc", "name rt args slot")):
      def hasReturn(self):
      """
   Returns true if the function returns a value.
   """
         @property
   def decArgs(self):
      """
   Returns a string with the types and names of the arguments, as you
   would use in a function declaration.
   """
   if not self.args:
         else:
         @property
   def callArgs(self):
      """
   Returns a string with the names of the arguments, as you would use in a
   function call.
   """
         @property
   def basename(self):
      assert self.name.startswith("gl")
      def _getFunctionList(root):
      for elem in root.findall("commands/command"):
         def _parseCommandElem(elem):
      protoElem = elem.find("proto")
            args = []
   for ch in elem.findall("param"):
      # <param> tags have the same format as a <proto> tag.
                     def _parseProtoElem(elem):
      # If I just remove the tags and string the text together, I'll get valid C code.
   text = _flattenText(elem)
   text = text.strip()
   m = re.match(r"^(.+)\b(\w+)(?:\s*\[\s*(\d*)\s*\])?$", text, re.S)
   if m:
      typename = _fixupTypeName(m.group(1))
   name = m.group(2)
   if m.group(3):
         # HACK: glPathGlyphIndexRangeNV defines an argument like this:
   # GLuint baseAndCount[2]
   # Convert it to a pointer and hope for the best.
      else:
         def _flattenText(elem):
      """
   Returns the text in an element and all child elements, with the tags
   removed.
   """
   text = ""
   if elem.text is not None:
         for ch in elem:
      text += _flattenText(ch)
   if ch.tail is not None:
            def _fixupTypeName(typeName):
      """
   Converts a typename into a more consistent format.
                     # Replace "GLvoid" with just plain "void".
            # Remove the vendor suffixes from types that have a suffix-less version.
                     # Clear out any leading and trailing whitespace.
            # Remove any whitespace before a '*'
            # Change "foo*" to "foo *"
            # Condense all whitespace into a single space.
               