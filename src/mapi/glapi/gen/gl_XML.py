      # (C) Copyright IBM Corporation 2004, 2005
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
   #    Ian Romanick <idr@us.ibm.com>
      from collections import OrderedDict
   from decimal import Decimal
   import xml.etree.ElementTree as ET
   import re, sys
   import os.path
   import typeexpr
   import static_data
         def parse_GL_API( file_name, factory = None ):
         if not factory:
            api = factory.create_api()
            # After the XML has been processed, we need to go back and assign
   # dispatch offsets to the functions that request that their offsets
   # be assigned by the scripts.  Typically this means all functions
            for func in api.functionIterateByCategory():
      if func.assign_offset and func.offset < 0:
                        def is_attr_true( element, name, default = "false" ):
               The value read from the attribute list must be either 'true' or
   'false'.  If the value is 'false', zero will be returned.  If the
   value is 'true', non-zero will be returned.  An exception will be
            value = element.get( name, default )
   if value == "true":
         elif value == "false":
         else:
            class gl_print_base(object):
               In the model-view-controller pattern, this is the view.  Any derived
   class will want to over-ride the printBody, printRealHader, and
   printRealFooter methods.  Some derived classes may want to over-ride
   printHeader and printFooter, or even Print (though this is unlikely).
            def __init__(self):
      # Name of the script that is generating the output file.
   # Every derived class should set this to the name of its
                        # License on the *generated* source file.  This may differ
   # from the license on the script that is generating the file.
   # Every derived class should set this to some reasonable
   # value.
   #
                        # The header_tag is the name of the C preprocessor define
   # used to prevent multiple inclusion.  Typically only
   # generated C header files need this to be set.  Setting it
   # causes code to be generated automatically in printHeader
                        # List of file-private defines that must be undefined at the
   # end of the file.  This can be used in header files to define
   # names for use in the file, then undefine them at the end of
            self.undef_list = []
            def Print(self, api):
      self.printHeader()
   self.printBody(api)
   self.printFooter()
            def printHeader(self):
               print('/* DO NOT EDIT - This file generated automatically by %s script */' \
         print('')
   print('/*')
   print((' * ' + self.license.replace('\n', '\n * ')).replace(' \n', '\n'))
   print(' */')
   print('')
   if self.header_tag:
         print('#if !defined( %s )' % (self.header_tag))
   print('#  define %s' % (self.header_tag))
   self.printRealHeader();
            def printFooter(self):
                        if self.undef_list:
         print('')
            if self.header_tag:
                  def printRealHeader(self):
               In the base class, this function is empty.  All derived
   classes should over-ride this function."""
            def printRealFooter(self):
               In the base class, this function is empty.  All derived
   classes should over-ride this function."""
            def printPure(self):
               Conditionally defines a preprocessor macro `PURE' that wraps
   GCC's `pure' function attribute.  The conditional code can be
   easilly adapted to other compilers that support a similar
            The name is also added to the file's undef_list.
   """
   self.undef_list.append("PURE")
   #    define PURE __attribute__((pure))
   #  else
   #    define PURE
   #  endif""")
                  def printFastcall(self):
               Conditionally defines a preprocessor macro `FASTCALL' that
   wraps GCC's `fastcall' function attribute.  The conditional
   code can be easilly adapted to other compilers that support a
            The name is also added to the file's undef_list.
            self.undef_list.append("FASTCALL")
   #    define FASTCALL __attribute__((fastcall))
   #  else
   #    define FASTCALL
   #  endif""")
                  def printVisibility(self, S, s):
               Conditionally defines a preprocessor macro name S that wraps
   GCC's visibility function attribute.  The visibility used is
   the parameter s.  The conditional code can be easilly adapted
            The name is also added to the file's undef_list.
            self.undef_list.append(S)
   #    define %s  __attribute__((visibility("%s")))
   #  else
   #    define %s
   #  endif""" % (S, s, S))
                  def printNoinline(self):
               Conditionally defines a preprocessor macro `NOINLINE' that
   wraps GCC's `noinline' function attribute.  The conditional
   code can be easilly adapted to other compilers that support a
            The name is also added to the file's undef_list.
            self.undef_list.append("NOINLINE")
   #    define NOINLINE __attribute__((noinline))
   #  else
   #    define NOINLINE
   #  endif""")
               def real_function_name(element):
      name = element.get( "name" )
            if alias:
         else:
            def real_category_name(c):
      if re.compile("[1-9][0-9]*[.][0-9]+").match(c):
         else:
            def classify_category(name, number):
               Categories are divided into four classes numbered 0 through 3.  The
                     0. Core GL versions, sorted by version number.
   1. ARB extensions, sorted by extension number.
               try:
         except Exception:
            if core_version > 0.0:
      cat_type = 0
      elif name.startswith("GL_ARB_") or name.startswith("GLX_ARB_") or name.startswith("WGL_ARB_"):
      cat_type = 1
      else:
      if number != None:
         cat_type = 2
   else:
                           def create_parameter_string(parameters, include_names):
               list = []
   for p in parameters:
      if p.is_padding:
            if include_names:
         else:
                           class gl_item(object):
      def __init__(self, element, context, category):
      self.context = context
   self.name = element.get( "name" )
                  class gl_type( gl_item ):
      def __init__(self, element, context, category):
      gl_item.__init__(self, element, context, category)
            te = typeexpr.type_expression( None )
   tn = typeexpr.type_node()
   tn.size = int( element.get( "size" ), 0 )
   tn.integer = not is_attr_true( element, "float" )
   tn.unsigned = is_attr_true( element, "unsigned" )
   tn.pointer = is_attr_true( element, "pointer" )
   tn.name = "GL" + self.name
            self.type_expr = te
            def get_type_expression(self):
            class gl_enum( gl_item ):
      def __init__(self, element, context, category):
      gl_item.__init__(self, element, context, category)
            temp = element.get( "count" )
   if not temp or temp == "?":
         else:
         try:
                                    def priority(self):
               When an enum is looked up by number, there may be many
   possible names, but only one is the 'prefered' name.  The
            Highest precedence is given to core GL name.  ARB extension
   names have the next highest, followed by EXT extension names.
   Vendor extension names are the lowest.
            if self.name.endswith( "_BIT" ):
         else:
            if self.category.startswith( "GL_VERSION_" ):
         elif self.category.startswith( "GL_ARB_" ):
         elif self.category.startswith( "GL_EXT_" ):
         else:
                     class gl_parameter(object):
      def __init__(self, element, context):
               ts = element.get( "type" )
            temp = element.get( "variable_param" )
   if temp:
         else:
            # The count tag can be either a numeric string or the name of
   # a variable.  If it is the name of a variable, the int(c)
   # statement will throw an exception, and the except block will
            c = element.get( "count" )
   try: 
         count = int(c)
   self.count = count
   except Exception:
         count = 1
            self.marshal_count = element.get("marshal_count")
            elements = (count * self.count_scale)
   if elements == 1:
            #if ts == "GLdouble":
   #	print '/* stack size -> %s = %u (before)*/' % (self.name, self.type_expr.get_stack_size())
   #	print '/* # elements = %u */' % (elements)
   self.type_expr.set_elements( elements )
   #if ts == "GLdouble":
            self.is_client_only = is_attr_true( element, 'client_only' )
   self.is_counter     = is_attr_true( element, 'counter' )
                        self.width      = element.get('img_width')
   self.height     = element.get('img_height')
   self.depth      = element.get('img_depth')
            self.img_xoff   = element.get('img_xoff')
   self.img_yoff   = element.get('img_yoff')
   self.img_zoff   = element.get('img_zoff')
            self.img_format = element.get('img_format')
   self.img_type   = element.get('img_type')
            self.img_pad_dimensions = is_attr_true( element, 'img_pad_dimensions' )
   self.img_null_flag      = is_attr_true( element, 'img_null_flag' )
            self.is_padding = is_attr_true( element, 'padding' )
            def compatible(self, other):
               def is_array(self):
               def is_pointer(self):
               def is_image(self):
      if self.width:
         else:
            def is_variable_length(self):
               def is_64_bit(self):
      count = self.type_expr.get_element_count()
   if count:
         if (self.size() / count) == 8:
   else:
                           def string(self):
      if self.type_expr.original_string[-1] == '*':
         else:
            def type_string(self):
               def get_base_type_string(self):
               def get_dimensions(self):
      if not self.width:
            dim = 1
   w = self.width
   h = "1"
   d = "1"
            if self.height:
                  if self.depth:
                  if self.extent:
                           def get_stack_size(self):
               def size(self):
      if self.is_image():
         else:
            def get_element_count(self):
      c = self.type_expr.get_element_count()
   if c == 0:
                     def size_string(self, use_parens = 1, marshal = 0):
               count = self.get_element_count()
   if count:
                     if self.counter or self.count_parameter_list or (self.marshal_count and marshal):
                  if self.marshal_count and marshal:
         elif self.counter and self.count_parameter_list:
                                       if len(list) > 1 and use_parens :
                  elif self.is_image():
         else:
            def format_string(self):
      if self.type_expr.original_string == "GLenum":
         else:
         class gl_function( gl_item ):
      def __init__(self, element, context):
      self.context = context
            self.entry_points = []
   self.return_type = "void"
   self.parameters = []
   self.offset = -1
   self.initialized = 0
   self.images = []
   self.exec_flavor = 'mesa'
   self.has_hw_select_variant = False
   self.desktop = True
   self.deprecated = None
            # self.api_map[api] is a decimal value indicating the earliest
   # version of the given API in which ANY alias for the function
   # exists.  The map only lists APIs which contain the function
   # in at least one version.  For example, for the ClipPlanex
   # function, self.api_map == { 'es1':
   # Decimal('1.1') }.
                              # Track the parameter string (for the function prototype)
   # for each entry-point.  This is done because some functions
   # change their prototype slightly when promoted from extension
   # to ARB extension to core.  glTexImage3DEXT and glTexImage3D
   # are good examples of this.  Scripts that need to generate
   # code for these differing aliases need to real prototype
   # for each entry-point.  Otherwise, they may generate code
                                       def process_element(self, element):
      name = element.get( "name" )
            # marshal isn't allowed with alias
   assert not alias or not element.get('marshal')
   assert not alias or not element.get('marshal_count')
   assert not alias or not element.get('marshal_sync')
   assert not alias or not element.get('marshal_call_before')
            if name in static_data.functions:
                     for api in ('es1', 'es2'):
         version_str = element.get(api, 'none')
   assert version_str is not None
   if version_str != 'none':
      version_decimal = Decimal(version_str)
               exec_flavor = element.get('exec')
   if exec_flavor:
            deprecated = element.get('deprecated', 'none')
   if deprecated != 'none':
            if not is_attr_true(element, 'desktop', 'true'):
            if self.has_no_error_variant or is_attr_true(element, 'no_error'):
         else:
            if alias:
         else:
                                          if name in static_data.offsets and static_data.offsets[name] <= static_data.MAX_OFFSETS:
         elif name in static_data.offsets and static_data.offsets[name] > static_data.MAX_OFFSETS:
      self.offset = static_data.offsets[name]
      else:
                  if not self.name:
         elif self.name != true_name:
               # There are two possible cases.  The first time an entry-point
   # with data is seen, self.initialized will be 0.  On that
   # pass, we just fill in the data.  The next time an
   # entry-point with data is seen, self.initialized will be 1.
   # On that pass we have to make that the new values match the
            parameters = []
   return_type = "void"
   for child in element:
         if child.tag == "return":
         elif child.tag == "param":
               if self.initialized:
                                       for j in range(0, len(parameters)):
      p1 = parameters[j]
                  if true_name == name or not self.initialized:
                        for param in self.parameters:
            if list(element):
         self.initialized = 1
   else:
                  def filter_entry_points(self, entry_point_list):
      """Filter out entry points not in entry_point_list."""
   if not self.initialized:
            entry_points = []
   for ent in self.entry_points:
         if ent not in entry_point_list:
      if ent in self.static_entry_points:
                     if not entry_points:
            self.entry_points = entry_points
   if self.name not in entry_points:
         # use the first remaining entry point
         def get_images(self):
      """Return potentially empty list of input images."""
            def parameterIterator(self, name = None):
      if name is not None:
         else:
            def get_parameter_string(self, entrypoint = None):
      if entrypoint:
         else:
                  def get_called_parameter_string(self):
      p_string = ""
            for p in self.parameterIterator():
         if p.is_padding:
                           def is_abi(self):
            def is_static_entry_point(self, name):
            def dispatch_name(self):
      if self.name in self.static_entry_points:
         else:
         def static_name(self, name):
      if name in self.static_entry_points:
         else:
      class gl_item_factory(object):
               def create_function(self, element, context):
            def create_type(self, element, context, category):
            def create_enum(self, element, context, category):
            def create_parameter(self, element, context):
            def create_api(self):
            class gl_api(object):
      def __init__(self, factory):
      self.functions_by_name = OrderedDict()
   self.enums_by_name = {}
            self.category_dict = {}
                              typeexpr.create_initial_types()
         def parse_file(self, file_name):
      doc = ET.parse( file_name )
            def process_element(self, file_name, doc):
      element = doc.getroot()
   if element.tag == "OpenGLAPI":
                  def process_OpenGLAPI(self, file_name, element):
      for child in element:
         if child.tag == "category":
         elif child.tag == "OpenGLAPI":
         elif child.tag == '{http://www.w3.org/2001/XInclude}include':
                           def process_category(self, cat):
      cat_name = cat.get( "name" )
            [cat_type, key] = classify_category(cat_name, cat_number)
            for child in cat:
                                          if func_name in self.functions_by_name:
      func = self.functions_by_name[ func_name ]
                                          elif child.tag == "enum":
      enum = self.factory.create_enum( child, self, cat_name )
      elif child.tag == "type":
                     def functionIterateByCategory(self, cat = None):
               If cat is None, all known functions are iterated in category
   order.  See classify_category for details of the ordering.
   Within a category, functions are sorted by name.  If cat is
   not None, then only functions in that category are iterated.
   """
            for func in self.functionIterateAll():
                                                      functions = []
   for func_cat_type in range(0,4):
                                                   def functionIterateByOffset(self):
      max_offset = -1
   for func in self.functions_by_name.values():
                     temp = [None for i in range(0, max_offset + 1)]
   for func in self.functions_by_name.values():
                     list = []
   for i in range(0, max_offset + 1):
                           def functionIterateAll(self):
               def enumIterateByName(self):
               list = []
   for enum in keys:
                     def categoryIterate(self):
               Iterate over all known categories in the order specified by
   classify_category.  Each iterated value is a tuple of the
   name and number (which may be None) of the category.
            list = []
   for cat_type in range(0,4):
                                    def get_category_for_name( self, name ):
      if name in self.category_dict:
         else:
            def typeIterate(self):
               def find_type( self, type_name ):
      if type_name in self.types_by_name:
         else:
         print("Unable to find base type matching \"%s\"." % (type_name))
   return None
