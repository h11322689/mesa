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
      #include "core/event.hpp"
   #include "pipe/p_screen.h"
      using namespace clover;
      event::event(clover::context &ctx, const ref_vector<event> &deps,
            context(ctx), _wait_count(1), _status(0),
   action_ok(action_ok), action_fail(action_fail) {
   for (auto &ev : deps)
      }
      event::~event() {
   }
      std::vector<intrusive_ref<event>>
   event::trigger_self() {
      std::lock_guard<std::mutex> lock(mutex);
            if (_wait_count && !--_wait_count)
            cv.notify_all();
      }
      void
   event::trigger() try {
      if (wait_count() == 1)
            for (event &ev : trigger_self())
      } catch (error &e) {
         }
      std::vector<intrusive_ref<event>>
   event::abort_self(cl_int status) {
      std::lock_guard<std::mutex> lock(mutex);
            _status = status;
   _wait_count = 0;
            cv.notify_all();
      }
      void
   event::abort(cl_int status) {
               for (event &ev : abort_self(status))
      }
      unsigned
   event::wait_count() const {
      std::lock_guard<std::mutex> lock(mutex);
      }
      bool
   event::signalled() const {
         }
      cl_int
   event::status() const {
      std::lock_guard<std::mutex> lock(mutex);
      }
      void
   event::chain(event &ev) {
      std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
   std::unique_lock<std::mutex> lock_ev(ev.mutex, std::defer_lock);
            if (_wait_count) {
      ev._wait_count++;
      }
      }
      void
   event::wait_signalled() const {
      std::unique_lock<std::mutex> lock(mutex);
      }
      void
   event::wait() const {
      std::vector<intrusive_ref<event>> evs;
            for (event &ev : evs)
               }
      hard_event::hard_event(command_queue &q, cl_command_type command,
            event(q.context(), deps, profile(q, action), [](event &ev){}),
   _queue(q), _command(command), _fence(NULL) {
   if (q.profiling_enabled())
            q.sequence(*this);
      }
      hard_event::~hard_event() {
      pipe_screen *screen = queue()->device().pipe;
      }
      cl_int
   hard_event::status() const {
               if (event::status() < 0)
            else if (!_fence)
            else if (!screen->fence_finish(screen, NULL, _fence, 0))
            else
      }
      command_queue *
   hard_event::queue() const {
         }
      cl_command_type
   hard_event::command() const {
         }
      void
   hard_event::wait() const {
                        if (status() == CL_QUEUED)
            if (!_fence ||
      !screen->fence_finish(screen, NULL, _fence, OS_TIMEOUT_INFINITE))
   }
      const lazy<cl_ulong> &
   hard_event::time_queued() const {
         }
      const lazy<cl_ulong> &
   hard_event::time_submit() const {
         }
      const lazy<cl_ulong> &
   hard_event::time_start() const {
         }
      const lazy<cl_ulong> &
   hard_event::time_end() const {
         }
      void
   hard_event::fence(pipe_fence_handle *fence) {
      assert(fence);
   pipe_screen *screen = queue()->device().pipe;
   screen->fence_reference(screen, &_fence, fence);
      }
      event::action
   hard_event::profile(command_queue &q, const action &action) const {
      if (q.profiling_enabled()) {
      return [&q, action] (event &ev) {
                                                } else {
            }
      soft_event::soft_event(clover::context &ctx, const ref_vector<event> &deps,
            event(ctx, deps, action, action) {
   if (_trigger)
      }
      cl_int
   soft_event::status() const {
      if (event::status() < 0)
            else if (!signalled() ||
            any_of([](const event &ev) {
               else
      }
      command_queue *
   soft_event::queue() const {
         }
      cl_command_type
   soft_event::command() const {
         }
      void
   soft_event::wait() const {
               if (status() != CL_COMPLETE)
      }
