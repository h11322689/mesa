   /*
   * Copyright 2023 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
   #include <stdio.h>
   #include <stdarg.h>
   #include <string.h>
      #include "util/hash_table.h"
   #include "util/u_process.h"
   #include "util/hash_table.h"
      #include "si_pipe.h"
   #include "si_perfetto.h"
   #include "si_tracepoints.h"
      #ifdef HAVE_PERFETTO
      #include "util/perf/u_perfetto.h"
   #include "util/perf/u_perfetto_renderpass.h"
      #include "si_tracepoints_perfetto.h"
      /* Just naming stages */
   static const struct {
               /* The perfetto UI requires that there is a parent-child relationship
   * within a row of elements. Which means that all children elements must
   * end within the lifespan of their parent.
   *
   * Some elements like stalls and command buffers follow that relationship,
   * but not all. This tells us in which UI row the elements should live.
   */
      } si_queue_stage_desc[SI_DS_QUEUE_STAGE_N_STAGES] = {
      /* Order must match the enum! */
   {
      "queue",
      },
   {
      "compute",
      },
   {
      "draw",
         };
      struct SIRenderpassIncrementalState {
         };
      struct SIRenderpassTraits : public perfetto::DefaultDataSourceTraits {
         };
      class SIRenderpassDataSource : public MesaRenderpassDataSource<SIRenderpassDataSource, 
         };
      PERFETTO_DECLARE_DATA_SOURCE_STATIC_MEMBERS(SIRenderpassDataSource);
   PERFETTO_DEFINE_DATA_SOURCE_STATIC_MEMBERS(SIRenderpassDataSource);
      using perfetto::protos::pbzero::InternedGpuRenderStageSpecification_RenderStageCategory;
      static void sync_timestamp(SIRenderpassDataSource::TraceContext &ctx, struct si_ds_device *device)
   {
      uint64_t cpu_ts = perfetto::base::GetBootTimeNs().count();
            struct si_context *sctx = container_of(device, struct si_context, ds);   
                        if (cpu_ts < device->next_clock_sync_ns)
                     device->sync_gpu_ts = gpu_ts;
   device->next_clock_sync_ns = cpu_ts + 1000000000ull;
   MesaRenderpassDataSource<SIRenderpassDataSource, SIRenderpassTraits>::
      }
      static void send_descriptors(SIRenderpassDataSource::TraceContext &ctx, 
         {
               device->event_id = 0;
   list_for_each_entry_safe(struct si_ds_queue, queue, &device->queues, link) {
      for (uint32_t s = 0; s < ARRAY_SIZE(queue->stages); s++) {
                     {
               packet->set_timestamp(perfetto::base::GetBootTimeNs().count());
   packet->set_timestamp_clock_id(perfetto::protos::pbzero::BUILTIN_CLOCK_BOOTTIME);
                     {
      auto desc = interned_data->add_graphics_contexts();
   desc->set_iid(device->iid);
   desc->set_pid(getpid());
   switch (device->api) {
   case AMD_DS_API_OPENGL:
      desc->set_api(perfetto::protos::pbzero::InternedGraphicsContext_Api::OPEN_GL);
      case AMD_DS_API_VULKAN:
      desc->set_api(perfetto::protos::pbzero::InternedGraphicsContext_Api::VULKAN);
      default:
                     /* Emit all the IID picked at device/queue creation. */
   list_for_each_entry_safe(struct si_ds_queue, queue, &device->queues, link) {
      for (unsigned s = 0; s < SI_DS_QUEUE_STAGE_N_STAGES; s++) {
      {
      /* We put the stage number in there so that all rows are order
   * by si_ds_queue_stage.
   */
                        auto desc = interned_data->add_gpu_specifications();
   desc->set_iid(queue->stages[s].queue_iid);
      }
   {
      auto desc = interned_data->add_gpu_specifications();
   desc->set_iid(queue->stages[s].stage_iid);
                        device->next_clock_sync_ns = 0;
      }
      typedef void (*trace_payload_as_extra_func)(perfetto::protos::pbzero::GpuRenderStageEvent *, 
            static void begin_event(struct si_ds_queue *queue, uint64_t ts_ns, enum si_ds_queue_stage stage_id)
   {
      PERFETTO_LOG("begin event called - ts_ns=%lu", ts_ns);
   uint32_t level = queue->stages[stage_id].level;
   /* If we haven't managed to calibrate the alignment between GPU and CPU
   * timestamps yet, then skip this trace, otherwise perfetto won't know
   * what to do with it.
   */
   if (!queue->device->sync_gpu_ts) {
      queue->stages[stage_id].start_ns[level] = 0;
               if (level >= (ARRAY_SIZE(queue->stages[stage_id].start_ns) - 1))
            queue->stages[stage_id].start_ns[level] = ts_ns;
      }
      static void end_event(struct si_ds_queue *queue, uint64_t ts_ns, enum si_ds_queue_stage stage_id,
               {
      PERFETTO_LOG("end event called - ts_ns=%lu", ts_ns);
            /* If we haven't managed to calibrate the alignment between GPU and CPU
   * timestamps yet, then skip this trace, otherwise perfetto won't know
   * what to do with it.
   */
   if (!device->sync_gpu_ts)
            if (queue->stages[stage_id].level == 0)
            uint32_t level = --queue->stages[stage_id].level;
   struct si_ds_stage *stage = &queue->stages[stage_id];
   uint64_t start_ns = stage->start_ns[level];
   PERFETTO_LOG("end event called - start_ns=%lu ts_ns=%lu", start_ns, ts_ns);
   if (!start_ns || start_ns > ts_ns)
            SIRenderpassDataSource::Trace([=](SIRenderpassDataSource::TraceContext tctx) {
      if (auto state = tctx.GetIncrementalState(); state->was_cleared) {
      send_descriptors(tctx, queue->device);
                                 /* If this is an application event, we might need to generate a new
   * stage_iid if not already seen. Otherwise, it's a driver event and we
   * have use the internal stage_iid.
   */
   uint64_t stage_iid = app_event ? 
                           packet->set_timestamp(start_ns);
                     auto event = packet->set_gpu_render_stage_event();
            event->set_hw_queue_iid(stage->queue_iid);
   event->set_stage_iid(stage_iid);
   event->set_context(queue->device->iid);
   event->set_event_id(evt_id);
   event->set_duration(ts_ns - start_ns);
            if (payload && payload_as_extra) {
                        }
      #endif /* HAVE_PERFETTO */
      #ifdef __cplusplus
   extern "C" {
   #endif
      #ifdef HAVE_PERFETTO
      /*
   * Trace callbacks, called from u_trace once the timestamps from GPU have been
   * collected.
   */
      #define CREATE_DUAL_EVENT_CALLBACK(event_name, stage)                                             \
   void si_ds_begin_##event_name(struct si_ds_device *device, uint64_t ts_ns, uint16_t tp_idx,       \
               {                                                                                                 \
      const struct si_ds_flush_data *flush = (const struct si_ds_flush_data *) flush_data;           \
      }                                                                                                 \
         void si_ds_end_##event_name(struct si_ds_device *device, uint64_t ts_ns, uint16_t tp_idx,         \
               {                                                                                                 \
      const struct si_ds_flush_data *flush =  (const struct si_ds_flush_data *) flush_data;          \
   end_event(flush->queue, ts_ns, stage, flush->submission_id, NULL, payload,                     \
      }                                                                                                 \
      CREATE_DUAL_EVENT_CALLBACK(draw, SI_DS_QUEUE_STAGE_DRAW)
   CREATE_DUAL_EVENT_CALLBACK(compute, SI_DS_QUEUE_STAGE_COMPUTE)
      uint64_t si_ds_begin_submit(struct si_ds_queue *queue)
   {
         }
      void si_ds_end_submit(struct si_ds_queue *queue, uint64_t start_ts)
   {
      if (!u_trace_should_process(&queue->device->trace_context)) {
      queue->device->sync_gpu_ts = 0;
   queue->device->next_clock_sync_ns = 0;
               uint64_t end_ts = perfetto::base::GetBootTimeNs().count();
            SIRenderpassDataSource::Trace([=](SIRenderpassDataSource::TraceContext tctx) {
      if (auto state = tctx.GetIncrementalState(); state->was_cleared) {
      send_descriptors(tctx, queue->device);
                                          auto event = packet->set_vulkan_api_event();
            submit->set_duration_ns(end_ts - start_ts);
   submit->set_vk_queue((uintptr_t) queue);
         }
      #endif /* HAVE_PERFETTO */
      static void si_driver_ds_init_once(void)
   {
   #ifdef HAVE_PERFETTO
      util_perfetto_init();
   perfetto::DataSourceDescriptor dsd;
   dsd.set_name("gpu.renderstages.amd");
      #endif
   }
      static once_flag si_driver_ds_once_flag = ONCE_FLAG_INIT;
   static uint64_t iid = 1;
      static uint64_t get_iid()
   {
         }
      static uint32_t si_pps_clock_id(uint32_t gpu_id)
   {
      char buf[40];
               }
      void si_driver_ds_init(void)
   {
      call_once(&si_driver_ds_once_flag, si_driver_ds_init_once);
      }
      void si_ds_device_init(struct si_ds_device *device, const struct radeon_info *devinfo,
         {
      device->gpu_id = gpu_id;
   device->gpu_clock_id = si_pps_clock_id(gpu_id);
   device->info = devinfo;
   device->iid = get_iid();
   device->api = api;
      }
      void si_ds_device_fini(struct si_ds_device *device)
   {
         }
      struct si_ds_queue * si_ds_device_init_queue(struct si_ds_device *device, 
               {
      va_list ap;
            va_start(ap, fmt_name);
   vsnprintf(queue->name, sizeof(queue->name), fmt_name, ap);
            for (unsigned s = 0; s < SI_DS_QUEUE_STAGE_N_STAGES; s++) {
      queue->stages[s].queue_iid = get_iid();
                           }
      void si_ds_flush_data_init(struct si_ds_flush_data *data, struct si_ds_queue *queue, 
         {
               data->queue = queue;
               }
      void si_ds_flush_data_fini(struct si_ds_flush_data *data)
   {
         }
      #ifdef __cplusplus
   }
   #endif
