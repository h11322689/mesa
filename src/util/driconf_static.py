   #
   # Copyright (C) 2021 Google, Inc.
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
      from mako.template import Template
   from xml.etree import ElementTree
   import argparse
   import os
      def dbg(str):
      if False:
         cnt = 0
   def cname(name):
      global cnt
   cnt = cnt + 1
         class Option(object):
      def __init__(self, xml):
      self.cname = cname('option')
   self.name = xml.attrib['name']
      class Application(object):
      def __init__(self, xml):
      self.cname = cname('application')
   self.name = xml.attrib['name']
   self.executable = xml.attrib.get('executable', None)
   self.executable_regexp = xml.attrib.get('executable_regexp', None)
   self.sha1 = xml.attrib.get('sha1', None)
   self.application_name_match = xml.attrib.get('application_name_match', None)
   self.application_versions = xml.attrib.get('application_versions', None)
            for option in xml.findall('option'):
      class Engine(object):
      def __init__(self, xml):
      self.cname = cname('engine')
   self.engine_name_match = xml.attrib['engine_name_match']
   self.engine_versions = xml.attrib.get('engine_versions', None)
            for option in xml.findall('option'):
      class Device(object):
      def __init__(self, xml):
      self.cname = cname('device')
   self.driver = xml.attrib.get('driver', None)
   self.device = xml.attrib.get('device', None)
   self.applications = []
            for application in xml.findall('application'):
            for engine in xml.findall('engine'):
      class DriConf(object):
      def __init__(self, xmlpaths):
      self.devices = []
   for xmlpath in xmlpaths:
                        template = """\
   /* Copyright (C) 2021 Google, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      struct driconf_option {
      const char *name;
      };
      struct driconf_application {
      const char *name;
   const char *executable;
   const char *executable_regexp;
   const char *sha1;
   const char *application_name_match;
   const char *application_versions;
   unsigned num_options;
      };
      struct driconf_engine {
      const char *engine_name_match;
   const char *engine_versions;
   unsigned num_options;
      };
      struct driconf_device {
      const char *driver;
   const char *device;
   unsigned num_engines;
   const struct driconf_engine *engines;
   unsigned num_applications;
      };
      <%def name="render_options(cname, options)">
   static const struct driconf_option ${cname}[] = {
   %    for option in options:
         %    endfor
   };
   </%def>
      %for device in driconf.devices:
   %    for engine in device.engines:
         %    endfor
      %if len(device.engines) > 0:
   static const struct driconf_engine ${device.cname}_engines[] = {
   %    for engine in device.engines:
         %        if engine.engine_versions:
         %        endif
         .num_options = ${len(engine.options)},
         %    endfor
   };
   %endif
      %    for application in device.applications:
         %    endfor
      %if len(device.applications) > 0:
   static const struct driconf_application ${device.cname}_applications[] = {
   %    for application in device.applications:
         %        if application.executable:
         %        endif
   %        if application.executable_regexp:
         %        endif
   %        if application.sha1:
         %        endif
   %        if application.application_name_match:
         %        endif
   %        if application.application_versions:
         %        endif
         .num_options = ${len(application.options)},
         %    endfor
   };
   %endif
      static const struct driconf_device ${device.cname} = {
   %    if device.driver:
         %    endif
   %    if device.device:
         %    endif
         %    if len(device.engines) > 0:
         %    endif
         %    if len(device.applications) > 0:
         %    endif
   };
   %endfor
      static const struct driconf_device *driconf[] = {
   %for device in driconf.devices:
         %endfor
   };
   """
      parser = argparse.ArgumentParser()
   parser.add_argument('drirc',
               parser.add_argument('header',
         args = parser.parse_args()
      xml = args.drirc
   dst = args.header
      with open(dst, 'wb') as f:
         