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
      #include "core/resource.hpp"
   #include "core/memory.hpp"
   #include "pipe/p_screen.h"
   #include "util/u_sampler.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_resource.h"
   #include "util/u_surface.h"
      using namespace clover;
      namespace {
      class box {
   public:
      box(const resource::vector &origin, const resource::vector &size) :
   pipe({ (int)origin[0], (int16_t)origin[1],
                        operator const pipe_box *() {
               protected:
            }
      resource::resource(clover::device &dev, memory_obj &obj) :
         }
      resource::~resource() {
   }
      void
   resource::copy(command_queue &q, const vector &origin, const vector &region,
                     q.pipe->resource_copy_region(q.pipe, pipe, 0, p[0], p[1], p[2],
            }
      void
   resource::clear(command_queue &q, const vector &origin, const vector &region,
                     if (pipe->target == PIPE_BUFFER) {
         } else {
      std::string texture_data;
   texture_data.reserve(util_format_get_blocksize(pipe->format));
   util_format_pack_rgba(pipe->format, &texture_data[0], data.data(), 1);
   if (q.pipe->clear_texture) {
         } else {
               }
      mapping *
   resource::add_map(command_queue &q, cl_map_flags flags, bool blocking,
            maps.emplace_back(q, *this, flags, blocking, origin, region);
      }
      void
   resource::del_map(void *p) {
      erase_if([&](const mapping &b) {
            }
      unsigned
   resource::map_count() const {
         }
      pipe_sampler_view *
   resource::bind_sampler_view(command_queue &q) {
               u_sampler_view_default_template(&info, pipe, pipe->format);
      }
      void
   resource::unbind_sampler_view(command_queue &q,
               }
      pipe_image_view
   resource::create_image_view(command_queue &q) {
      pipe_image_view view;
   view.resource = pipe;
   view.format = pipe->format;
   view.access = 0;
            if (pipe->target == PIPE_BUFFER) {
      view.u.buf.offset = 0;
      } else {
      view.u.tex.first_layer = 0;
   if (util_texture_is_array(pipe->target))
         else
                        }
      pipe_surface *
   resource::bind_surface(command_queue &q, bool rw) {
               info.format = pipe->format;
            if (pipe->target == PIPE_BUFFER)
               }
      void
   resource::unbind_surface(command_queue &q, pipe_surface *st) {
         }
      root_resource::root_resource(clover::device &dev, memory_obj &obj,
            resource(dev, obj) {
            if (image *img = dynamic_cast<image *>(&obj)) {
      info.format = translate_format(img->format());
   info.width0 = img->width();
   info.height0 = img->height();
   info.depth0 = img->depth();
      } else {
      info.width0 = obj.size();
   info.height0 = 1;
   info.depth0 = 1;
               info.target = translate_target(obj.type());
   info.bind = (PIPE_BIND_SAMPLER_VIEW |
                  if (obj.flags() & CL_MEM_USE_HOST_PTR && dev.allows_user_pointers()) {
      // Page alignment is normally required for this, just try, hope for the
   // best and fall back if it fails.
   pipe = dev.pipe->resource_from_user_memory(dev.pipe, &info, obj.host_ptr());
   if (pipe)
               if (obj.flags() & (CL_MEM_ALLOC_HOST_PTR | CL_MEM_USE_HOST_PTR)) {
                  pipe = dev.pipe->resource_create(dev.pipe, &info);
   if (!pipe)
            if (data_ptr) {
      box rect { {{ 0, 0, 0 }}, {{ info.width0, info.height0, info.depth0 }} };
            if (pipe->target == PIPE_BUFFER)
      q.pipe->buffer_subdata(q.pipe, pipe, PIPE_MAP_WRITE,
      else
      q.pipe->texture_subdata(q.pipe, pipe, 0, PIPE_MAP_WRITE,
            }
      root_resource::root_resource(clover::device &dev, memory_obj &obj,
            resource(dev, obj) {
      }
      root_resource::~root_resource() {
         }
      sub_resource::sub_resource(resource &r, const vector &offset) :
      resource(r.device(), r.obj) {
   this->pipe = r.pipe;
      }
      mapping::mapping(command_queue &q, resource &r,
                  cl_map_flags flags, bool blocking,
   pctx(q.pipe), pres(NULL) {
   unsigned usage = ((flags & CL_MAP_WRITE ? PIPE_MAP_WRITE : 0 ) |
                              p = pctx->buffer_map(pctx, r.pipe, 0, usage,
         if (!p) {
      pxfer = NULL;
      }
      }
      mapping::mapping(mapping &&m) :
      pctx(m.pctx), pxfer(m.pxfer), pres(m.pres), p(m.p) {
   m.pctx = NULL;
   m.pxfer = NULL;
   m.pres = NULL;
      }
      mapping::~mapping() {
      if (pxfer) {
         }
      }
      mapping &
   mapping::operator=(mapping m) {
      std::swap(pctx, m.pctx);
   std::swap(pxfer, m.pxfer);
   std::swap(pres, m.pres);
   std::swap(p, m.p);
      }
      resource::vector
   mapping::pitch() const
   {
      return {
      util_format_get_blocksize(pres->format),
   pxfer->stride,
         }
