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
      #include "core/queue.hpp"
   #include "core/event.hpp"
   #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "util/u_debug.h"
      using namespace clover;
      namespace {
      void
   debug_notify_callback(void *data,
                        unsigned *id,
   const command_queue *queue = (const command_queue *)data;
   char buffer[1024];
   vsnprintf(buffer, sizeof(buffer), fmt, args);
         }
      command_queue::command_queue(clover::context &ctx, clover::device &dev,
            context(ctx), device(dev), _props(props) {
   pipe = dev.pipe->context_create(dev.pipe, NULL, PIPE_CONTEXT_COMPUTE_ONLY);
   if (!pipe)
            if (ctx.notify) {
      struct util_debug_callback cb;
   memset(&cb, 0, sizeof(cb));
   cb.debug_message = &debug_notify_callback;
   cb.data = this;
   if (pipe->set_debug_callback)
         }
   command_queue::command_queue(clover::context &ctx, clover::device &dev,
                     for(std::vector<cl_queue_properties>::size_type i = 0; i != properties.size(); i += 2) {
      if (properties[i] == 0)
         if (properties[i] == CL_QUEUE_PROPERTIES)
         else if (properties[i] != CL_QUEUE_SIZE)
               pipe = dev.pipe->context_create(dev.pipe, NULL, PIPE_CONTEXT_COMPUTE_ONLY);
   if (!pipe)
            if (ctx.notify) {
      struct util_debug_callback cb;
   memset(&cb, 0, sizeof(cb));
   cb.debug_message = &debug_notify_callback;
   cb.data = this;
   if (pipe->set_debug_callback)
         }
      command_queue::~command_queue() {
         }
      void
   command_queue::flush() {
      std::lock_guard<std::mutex> lock(queued_events_mutex);
      }
      void
   command_queue::flush_unlocked() {
      pipe_screen *screen = device().pipe;
            if (!queued_events.empty()) {
               while (!queued_events.empty() &&
            queued_events.front()().fence(fence);
                     }
      void
   command_queue::svm_migrate(const std::vector<void const*> &svm_pointers,
                  if (!pipe->svm_migrate)
            bool to_device = !(flags & CL_MIGRATE_MEM_OBJECT_HOST);
   bool mem_undefined = flags & CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED;
   pipe->svm_migrate(pipe, svm_pointers.size(), svm_pointers.data(),
      }
      cl_command_queue_properties
   command_queue::props() const {
         }
      std::vector<cl_queue_properties>
   command_queue::properties() const {
         }
      bool
   command_queue::profiling_enabled() const {
         }
      void
   command_queue::sequence(hard_event &ev) {
      std::lock_guard<std::mutex> lock(queued_events_mutex);
   if (!queued_events.empty())
                     // Arbitrary threshold.
   // The CTS tends to run a lot of subtests without flushing with the image
   // tests, so flush regularly to prevent stack overflows.
   if (queued_events.size() > 1000)
      }
