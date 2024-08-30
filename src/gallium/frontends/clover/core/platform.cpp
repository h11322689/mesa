   //
   // Copyright 2012 Francisco Jerez
   //
   // Permission is hereby granted, free of charge, to any person obtaining a
   // copy of this software and associated documentation files (the "Software"),
   // to deal in the Software without restriction, including without limitation
   // the rights to use, copy, modify, merge, publish, distribute, sublicense,
   // and/or sell copies of the Software, and to permit persons to whom the
   // Software is furnished to do so, subject to the following conditions:
   //
   // The above copyright notice and this permission notice shall be included in
   // all copies or substantial portions of the Software.
   //
   // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   // THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   // OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   // ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   // OTHER DEALINGS IN THE SOFTWARE.
   //
      #include "core/platform.hpp"
   #include "util/u_debug.h"
      using namespace clover;
      platform::platform() : adaptor_range(evals(), devs) {
      int n = pipe_loader_probe(NULL, 0, false);
            unsigned major = 1, minor = 1;
   debug_get_version_option("CLOVER_PLATFORM_VERSION_OVERRIDE", &major, &minor);
                     for (pipe_loader_device *ldev : ldevs) {
      try {
      if (ldev)
      } catch (error &) {
               }
      std::vector<cl_name_version>
   platform::supported_extensions() const {
               vec.push_back( cl_name_version{ CL_MAKE_VERSION(1, 0, 0), "cl_khr_icd" } );
      }
      std::string
   platform::supported_extensions_as_string() const {
               if (!extensions_string.empty())
            const auto extension_list = supported_extensions();
   for (const auto &extension : extension_list) {
      if (!extensions_string.empty())
            }
      }
      std::string
   platform::platform_version_as_string() const {
      static const std::string version_string =
      std::to_string(CL_VERSION_MAJOR(version)) + "." +
         }
      cl_version
   platform::platform_version() const {
         }
