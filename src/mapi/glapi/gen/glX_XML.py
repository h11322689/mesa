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
      import gl_XML
   import license
   import sys, getopt, string
         class glx_item_factory(gl_XML.gl_item_factory):
               def create_function(self, element, context):
            def create_enum(self, element, context, category):
            def create_api(self):
            class glx_enum(gl_XML.gl_enum):
      def __init__(self, element, context, category):
                        for child in element:
         if child.tag == "size":
                           if not c:
                        if m == "get":
                                    class glx_function(gl_XML.gl_function):
      def __init__(self, element, context):
      self.glx_rop = 0
   self.glx_sop = 0
                     # If this is set to true, it means that GLdouble parameters should be
   # written to the GLX protocol packet in the order they appear in the
   # prototype.  This is different from the "classic" ordering.  In the
   # classic ordering GLdoubles are written to the protocol packet first,
   # followed by non-doubles.  NV_vertex_program was the first extension
                     self.vectorequiv = None
   self.output = None
   self.can_be_large = 0
   self.reply_always_array = 0
   self.dimensions_in_reply = 0
            self.server_handcode = 0
   self.client_handcode = 0
            self.count_parameter_list = []
   self.counter_list = []
   self.parameters_by_name = {}
            gl_XML.gl_function.__init__(self, element, context)
            def process_element(self, element):
               # If the function already has a vector equivalent set, don't
   # set it again.  This can happen if an alias to a function
            if not self.vectorequiv:
               name = element.get("name")
   if name == self.name:
                                                   for child in element:
         if child.tag == "glx":
                                                                                                      handcode = child.get( 'handcode', 'false' )
   if handcode == "false":
      self.server_handcode = 0
      elif handcode == "true":
      self.server_handcode = 1
      elif handcode == "client":
      self.server_handcode = 0
      elif handcode == "server":
      self.server_handcode = 1
                     self.ignore               = gl_XML.is_attr_true( child, 'ignore' )
   self.can_be_large         = gl_XML.is_attr_true( child, 'large' )
                  # Do some validation of the GLX protocol information.  As
            for param in self.parameters:
                           def has_variable_size_request(self):
               The GLX request packet is variable sized in several common
            1. The function has a non-output parameter that is counted
                  2. The function has a non-output parameter whose count is
                  3. The function has a non-output parameter that is an
            4. The function must be hand-coded on the server.
            if self.glx_rop == 0:
            if self.server_handcode or self.images:
            for param in self.parameters:
         if not param.is_output:
                     def variable_length_parameter(self):
      for param in self.parameters:
         if not param.is_output:
                     def calculate_offsets(self):
      if not self.offsets_calculated:
         # Calculate the offset of the first function parameter
   # in the GLX command packet.  This byte offset is
   # measured from the end of the Render / RenderLarge
   # header.  The offset for all non-pixel commends is
                                                         if dim <=  2:
         elif dim <= 4:
         else:
                     for param in self.parameterIterateGlxSend():
                     if param.name != self.img_reset:
                                                def offset_of(self, param_name):
      self.calculate_offsets()
            def parameterIterateGlxSend(self, include_variable_parameters = 1):
               # The parameter lists are usually quite short, so it's easier
   # (i.e., less code) to just generate a new list with the
            temp = [ [],  [], [] ]
   for param in self.parameters:
                  if param.is_variable_length():
         elif not self.glx_doubles_in_order and param.is_64_bit():
                  parameters = temp[0]
   parameters.extend( temp[1] )
   if include_variable_parameters:
                  def parameterIterateCounters(self):
      temp = []
   for name in self.counter_list:
                     def parameterIterateOutputs(self):
      temp = []
   for p in self.parameters:
                           def command_fixed_length(self):
      """Return the length, in bytes as an integer, of the
            if len(self.parameters) == 0:
                     size = 0
   for param in self.parameterIterateGlxSend(0):
         if param.name != self.img_reset and not param.is_client_only:
      if size == 0:
                              for param in self.images:
                           def command_variable_length(self):
      """Return the length, as a string, of the variable-sized
            size_string = ""
   for p in self.parameterIterateGlxSend():
         if (not p.is_output) and (p.is_variable_length() or p.is_image()):
      # FIXME Replace the 1 in the size_string call
   # FIXME w/0 to eliminate some un-needed parnes
                                 def command_length(self):
               if self.glx_rop != 0:
            size = ((size + 3) & ~3)
            def opcode_real_value(self):
               Behaves similarly to opcode_value, except for
   X_GLXVendorPrivate and X_GLXVendorPrivateWithReply commands.
   In these cases the value for the GLX opcode field (i.e.,
   16 for X_GLXVendorPrivate or 17 for
   X_GLXVendorPrivateWithReply) is returned.  For other 'single'
   commands, the opcode for the command (e.g., 101 for
            if self.glx_vendorpriv != 0:
         if self.needs_reply():
         else:
   else:
            def opcode_value(self):
               if (self.glx_rop == 0) and self.vectorequiv:
                     if self.glx_rop != 0:
         elif self.glx_sop != 0:
         elif self.glx_vendorpriv != 0:
         else:
            def opcode_rop_basename(self):
               Returns either the name of the function or the name of the
   name of the equivalent vector (e.g., glVertex3fv for
            if self.vectorequiv == None:
         else:
            def opcode_name(self):
               if (self.glx_rop == 0) and self.vectorequiv:
         equiv = self.context.functions_by_name[ self.vectorequiv ]
               if self.glx_rop != 0:
         elif self.glx_sop != 0:
         elif self.glx_vendorpriv != 0:
         else:
            def opcode_vendor_name(self, name):
      if name in self.glx_vendorpriv_names:
         else:
            def opcode_real_name(self):
               Behaves similarly to opcode_name, except for
   X_GLXVendorPrivate and X_GLXVendorPrivateWithReply commands.
   In these cases the string 'X_GLXVendorPrivate' or
   'X_GLXVendorPrivateWithReply' is returned.  For other
   single or render commands 'X_GLsop' or 'X_GLrop' plus the
            if self.glx_vendorpriv != 0:
         if self.needs_reply():
         else:
   else:
            def needs_reply(self):
      try:
         except Exception:
         x = 0
                  for param in self.parameters:
                                    def pad_after(self, p):
      """Returns the name of the field inserted after the
            for image in self.images:
         if image.img_pad_dimensions:
      if not image.height:
      if p.name == image.width:
         elif p.name == image.img_xoff:
      elif not image.extent:
      if p.name == image.depth:
                           def has_different_protocol(self, name):
               Some functions, such as glDeleteTextures and
   glDeleteTexturesEXT are functionally identical, but have
   different protocol.  This function returns true if the
   named function is an alias name and that named version uses
   different protocol from the function that is aliased.
                     def static_glx_name(self, name):
      if self.has_different_protocol(name):
         for n in self.glx_vendorpriv_names:
                     def client_supported_for_indirect(self):
      """Returns true if the function is supported on the client
                  class glx_function_iterator(object):
               def __init__(self, context):
      self.iterator = context.functionIterateByOffset()
            def __iter__(self):
               def __next__(self):
      while True:
                                 class glx_api(gl_XML.gl_api):
      def functionIterateGlx(self):
      