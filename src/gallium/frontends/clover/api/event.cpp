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
      #include "api/util.hpp"
   #include "core/event.hpp"
      using namespace clover;
      CLOVER_API cl_event
   clCreateUserEvent(cl_context d_ctx, cl_int *r_errcode) try {
               ret_error(r_errcode, CL_SUCCESS);
         } catch (error &e) {
      ret_error(r_errcode, e);
      }
      CLOVER_API cl_int
   clSetUserEventStatus(cl_event d_ev, cl_int status) try {
               if (status > 0)
            if (sev.status() <= 0)
            if (status)
         else
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clWaitForEvents(cl_uint num_evs, const cl_event *d_evs) try {
               for (auto &ev : evs) {
      if (ev.context() != evs.front().context())
            if (ev.status() < 0)
               // Create a temporary soft event that depends on all the events in
   // the wait list
            // ...and wait on it.
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetEventInfo(cl_event d_ev, cl_event_info param,
            property_buffer buf { r_buf, size, r_size };
            switch (param) {
   case CL_EVENT_COMMAND_QUEUE:
      buf.as_scalar<cl_command_queue>() = desc(ev.queue());
         case CL_EVENT_CONTEXT:
      buf.as_scalar<cl_context>() = desc(ev.context());
         case CL_EVENT_COMMAND_TYPE:
      buf.as_scalar<cl_command_type>() = ev.command();
         case CL_EVENT_COMMAND_EXECUTION_STATUS:
      buf.as_scalar<cl_int>() = ev.status();
         case CL_EVENT_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = ev.ref_count();
         default:
                        } catch (error &e) {
         }
      CLOVER_API cl_int
   clSetEventCallback(cl_event d_ev, cl_int type,
                           if (!pfn_notify ||
      (type != CL_COMPLETE && type != CL_SUBMITTED && type != CL_RUNNING))
         // Create a temporary soft event that depends on ev, with
   // pfn_notify as completion action.
   create<soft_event>(ev.context(), ref_vector<event> { ev }, true,
                                    } catch (error &e) {
         }
      CLOVER_API cl_int
   clRetainEvent(cl_event d_ev) try {
      obj(d_ev).retain();
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clReleaseEvent(cl_event d_ev) try {
      if (obj(d_ev).release())
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueMarker(cl_command_queue d_q, cl_event *rd_ev) try {
               if (!rd_ev)
                           } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueMarkerWithWaitList(cl_command_queue d_q, cl_uint num_deps,
            auto &q = obj(d_q);
            for (auto &ev : deps) {
      if (ev.context() != q.context())
               // Create a hard event that depends on the events in the wait list:
   // previous commands in the same queue are implicitly serialized
   // with respect to it -- hard events always are.
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueBarrier(cl_command_queue d_q) try {
                              } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueBarrierWithWaitList(cl_command_queue d_q, cl_uint num_deps,
            auto &q = obj(d_q);
            for (auto &ev : deps) {
      if (ev.context() != q.context())
               // Create a hard event that depends on the events in the wait list:
   // subsequent commands in the same queue will be implicitly
   // serialized with respect to it -- hard events always are.
            ret_object(rd_ev, hev);
         } catch (error &e) {
         }
      CLOVER_API cl_int
   clEnqueueWaitForEvents(cl_command_queue d_q, cl_uint num_evs,
            // The wait list is mandatory for clEnqueueWaitForEvents().
                  } catch (error &e) {
         }
      CLOVER_API cl_int
   clGetEventProfilingInfo(cl_event d_ev, cl_profiling_info param,
            property_buffer buf { r_buf, size, r_size };
            if (hev.status() != CL_COMPLETE)
            switch (param) {
   case CL_PROFILING_COMMAND_QUEUED:
      buf.as_scalar<cl_ulong>() = hev.time_queued();
         case CL_PROFILING_COMMAND_SUBMIT:
      buf.as_scalar<cl_ulong>() = hev.time_submit();
         case CL_PROFILING_COMMAND_START:
      buf.as_scalar<cl_ulong>() = hev.time_start();
         case CL_PROFILING_COMMAND_END:
   case CL_PROFILING_COMMAND_COMPLETE:
      buf.as_scalar<cl_ulong>() = hev.time_end();
         default:
                        } catch (std::bad_cast &) {
            } catch (lazy<cl_ulong>::undefined_error &) {
            } catch (error &e) {
         }
      CLOVER_API cl_int
   clFinish(cl_command_queue d_q) try {
               // Create a temporary hard event -- it implicitly depends on all
   // the previously queued hard events.
            // And wait on it.
                  } catch (error &e) {
         }
