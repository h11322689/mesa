      # Copyright (C) 2012 Intel Corporation
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      # marshal_XML.py: factory for interpreting XML for the purpose of
   # building thread marshalling code.
      import gl_XML
         class marshal_item_factory(gl_XML.gl_item_factory):
      """Factory to create objects derived from gl_item containing
            def create_function(self, element, context):
            class marshal_function(gl_XML.gl_function):
      def process_element(self, element):
      # Do normal processing.
            # Only do further processing when we see the canonical
   # function name.
   if element.get('name') != self.name:
            # Classify fixed and variable parameters.
   self.fixed_params = []
   self.variable_params = []
   for p in self.parameters:
         if p.is_padding:
         if p.is_variable_length():
                  # Store the "marshal" attribute, if present.
   self.marshal = element.get('marshal')
   self.marshal_sync = element.get('marshal_sync')
   self.marshal_call_before = element.get('marshal_call_before')
         def marshal_flavor(self):
      """Find out how this function should be marshalled between
   client and server threads."""
   # If a "marshal" attribute was present, that overrides any
   # determination that would otherwise be made by this function.
   if self.marshal is not None:
            if self.exec_flavor == 'skip':
         # Functions marked exec="skip" are not yet implemented in
            if self.return_type != 'void':
         for p in self.parameters:
         if p.is_output:
         if (p.is_pointer() and not (p.count or p.counter or p.marshal_count)):
         if p.count_parameter_list and not p.marshal_count:
      # Parameter size is determined by enums; haven't
            def marshal_is_static(self):
      return (self.marshal_flavor() != 'custom' and
            self.name[0:8] != 'Internal' and
   self.exec_flavor != 'beginend')
