   #include "pan_pps_perf.h"
      #include <lib/pan_device.h>
   #include <perf/pan_perf.h>
   #include <pps/pps.h>
   #include <util/ralloc.h>
      namespace pps {
   PanfrostDevice::PanfrostDevice(int fd)
      : ctx{ralloc_context(nullptr)},
      dev{reinterpret_cast<struct panfrost_device *>(
   {
      assert(fd >= 0);
      }
      PanfrostDevice::~PanfrostDevice()
   {
      if (ctx) {
         }
   if (dev) {
            }
      PanfrostDevice::PanfrostDevice(PanfrostDevice &&o): ctx{o.ctx}, dev{o.dev}
   {
      o.ctx = nullptr;
      }
      PanfrostDevice &
   PanfrostDevice::operator=(PanfrostDevice &&o)
   {
      std::swap(ctx, o.ctx);
   std::swap(dev, o.dev);
      }
      PanfrostPerf::PanfrostPerf(const PanfrostDevice &dev)
      : perf{reinterpret_cast<struct panfrost_perf *>(
      {
      assert(perf);
   assert(dev.dev);
      }
      PanfrostPerf::~PanfrostPerf()
   {
      if (perf) {
      panfrost_perf_disable(perf);
         }
      PanfrostPerf::PanfrostPerf(PanfrostPerf &&o): perf{o.perf}
   {
         }
      PanfrostPerf &
   PanfrostPerf::operator=(PanfrostPerf &&o)
   {
      std::swap(perf, o.perf);
      }
      int
   PanfrostPerf::enable() const
   {
      assert(perf);
      }
      void
   PanfrostPerf::disable() const
   {
      assert(perf);
      }
      int
   PanfrostPerf::dump() const
   {
      assert(perf);
      }
      } // namespace pps
