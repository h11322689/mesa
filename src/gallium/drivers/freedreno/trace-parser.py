   #!/usr/bin/env python3
      import re
   import sys
   import gzip
   import io
      # Captures per-frame state, including all the renderpasses, and
   # time spent in blits and compute jobs:
   class Frame:
      def __init__(self):
      self.frame_nr = None
   self.renderpasses = []
   # Times in ns:
   self.times_sysmem = []
   self.times_gmem = []
   self.times_compute = []
         def print(self):
      print("FRAME[{}]: {} blits ({:,} ns), {} SYSMEM ({:,} ns), {} GMEM ({:,} ns), {} COMPUTE ({:,} ns)".format(
            self.frame_nr,
   len(self.times_blit),    sum(self.times_blit),
   len(self.times_sysmem),  sum(self.times_sysmem),
               i = 0
   prologue_time = 0
   binning_time = 0
   restore_clear_time = 0
   draw_time = 0
   resolve_time = 0
   elapsed_time = 0
            for renderpass in self.renderpasses:
         renderpass.print(i)
   prologue_time += renderpass.prologue_time
   binning_time += renderpass.binning_time
   restore_clear_time += renderpass.restore_clear_time
   draw_time += renderpass.draw_time
   resolve_time += renderpass.resolve_time
            print("  TOTAL: prologue: {:,} ns ({}%), binning: {:,} ns ({}%), restore/clear: {:,} ns ({}%), draw: {:,} ns ({}%), resolve: {:,} ns ({}%), blit: {:,} ns ({}%), compute: {:,} ns ({}%), GMEM: {:,} ns ({}%), sysmem: {:,} ns ({}%), total: {:,} ns\n".format(
            prologue_time, 100.0 * prologue_time / total_time,
   binning_time, 100.0 * binning_time / total_time,
   restore_clear_time, 100.0 * restore_clear_time / total_time,
   draw_time, 100.0 * draw_time / total_time,
   resolve_time, 100.0 * resolve_time / total_time,
   sum(self.times_blit), 100.0 * sum(self.times_blit) / total_time,
   sum(self.times_compute), 100.0 * sum(self.times_compute) / total_time,
   sum(self.times_gmem), 100.0 * sum(self.times_gmem) / total_time,
         class FramebufferState:
      def __init__(self, width, height, layers, samples, nr_cbufs):
      self.width = width
   self.height = height
   self.layers = layers
   self.samples = samples
   self.nr_cbufs = nr_cbufs
         def get_formats(self):
      formats = []
   for surface in self.surfaces:
            class SurfaceState:
      def __init__(self, width, height, samples, format):
      self.width = width
   self.height = height
   self.samples = samples
      class BinningState:
      def __init__(self, nbins_x, nbins_y, bin_w, bin_h):
      self.nbins_x = nbins_x
   self.nbins_y = nbins_y
   self.bin_w = bin_w
      # Captures per-renderpass state, which can be either a binning or
   # sysmem pass.  Blits and compute jobs are not tracked separately
   # but have their time their times accounted for in the Frame state
   class RenderPass:
      def __init__(self, cleared, gmem_reason, num_draws):
      self.cleared = cleared
   self.gmem_reason = gmem_reason
            # The rest of the parameters aren't known until we see a later trace:
   self.binning_state = None   # None for sysmem passes, else BinningState
   self.fb = None
            self.elapsed_time = 0
   self.state_restore_time = 0
   self.prologue_time = 0
   self.draw_time = 0
            # Specific to GMEM passes:
   self.binning_time = 0
   self.vsc_overflow_test_time = 0
         def print_gmem_pass(self, nr):
      print("  GMEM[{}]: {}x{} ({}x{} tiles), {} draws, prologue: {:,} ns, binning: {:,} ns, restore/clear: {:,} ns, draw: {:,} ns, resolve: {:,} ns, total: {:,} ns, rt/zs: {}".format(
            nr, self.fb.width, self.fb.height,
   self.binning_state.nbins_x, self.binning_state.nbins_y,
   self.num_draws, self.prologue_time, self.binning_time,
   self.restore_clear_time, self.draw_time, self.resolve_time,
            def print_sysmem_pass(self, nr):
      print("  SYSMEM[{}]: {}x{}, {} draws, prologue: {:,} ns, clear: {:,} ns, draw: {:,} ns, total: {:,} ns, rt/zs: {}".format(
            nr, self.fb.width, self.fb.height,
   self.num_draws, self.prologue_time,
   self.restore_clear_time, self.draw_time,
            def print(self, nr):
      if self.binning_state:
         else:
      def main():
      filename = sys.argv[1]
   if filename.endswith(".gz"):
      file = gzip.open(filename, "r")
      else:
                  flush_batch_match = re.compile(r": flush_batch: (\S+): cleared=(\S+), gmem_reason=(\S+), num_draws=(\S+)")
   framebuffer_match = re.compile(r": framebuffer: (\S+)x(\S+)x(\S+)@(\S+), nr_cbufs: (\S+)")
            # draw/renderpass passes:
   gmem_match          = re.compile(r": render_gmem: (\S+)x(\S+) bins of (\S+)x(\S+)")
   sysmem_match        = re.compile(r": render_sysmem")
   state_restore_match = re.compile(r"\+(\S+): end_state_restore")
   prologue_match      = re.compile(r"\+(\S+): end_prologue")
   binning_ib_match    = re.compile(r"\+(\S+): end_binning_ib")
   vsc_overflow_match  = re.compile(r"\+(\S+): end_vsc_overflow_test")
   draw_ib_match       = re.compile(r"\+(\S+): end_draw_ib")
   resolve_match       = re.compile(r"\+(\S+): end_resolve")
      start_clear_restore_match = re.compile(r"start_clear_restore: fast_cleared: (\S+)")
            # Non-draw passes:
   compute_match = re.compile(r": start_compute")
            # End of pass/frame markers:
   elapsed_match = re.compile(r"ELAPSED: (\S+) ns")
            frame = Frame()      # current frame state
   renderpass = None    # current renderpass state
            # Helper to set the appropriate times table for the current pass,
   # which is expected to only happen once for a given render pass
   def set_times(t):
      nonlocal times
   if times  is not None:
               for line in lines:
      # Note, we only expect the flush_batch trace for !nondraw:
   match = re.search(flush_batch_match, line)
   if match is not None:
         assert(renderpass is None)
   renderpass = RenderPass(cleared=match.group(2),
                        match = re.search(framebuffer_match, line)
   if match is not None:
         assert(renderpass.fb is None)
   renderpass.fb = FramebufferState(width=match.group(1),
                              match = re.search(surface_match, line)
   if match is not None:
         surface = SurfaceState(width=match.group(1),
                              match = re.search(gmem_match, line)
   if match is not None:
         assert(renderpass.binning_state is None)
   renderpass.binning_state = BinningState(nbins_x=match.group(1),
                              match = re.search(sysmem_match, line)
   if match is not None:
         assert(renderpass.binning_state is None)
            match = re.search(state_restore_match, line)
   if match is not None:
                  match = re.search(prologue_match, line)
   if match is not None:
                  match = re.search(binning_ib_match, line)
   if match is not None:
         assert(renderpass.binning_state is not None)
            match = re.search(vsc_overflow_match, line)
   if match is not None:
         assert(renderpass.binning_state is not None)
            match = re.search(draw_ib_match, line)
   if match is not None:
                  match = re.search(resolve_match, line)
   if match is not None:
         assert(renderpass.binning_state is not None)
            match = re.search(start_clear_restore_match, line)
   if match is not None:
                  match = re.search(end_clear_restore_match, line)
   if match is not None:
                  match = re.search(compute_match, line)
   if match is not None:
                  match = re.search(blit_match, line)
   if match is not None:
                  match = re.search(eof_match, line)
   if match is not None:
         frame.frame_nr = int(match.group(1))
   frame.print()
   frame = Frame()
   times = None
            match = re.search(elapsed_match, line)
   if match is not None:
         time = int(match.group(1))
   #print("ELAPSED: " + str(time) + " ns")
   if renderpass is not None:
         times.append(time)
   times = None
         if __name__ == "__main__":
         