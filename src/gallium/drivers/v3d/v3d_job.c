   /*
   * Copyright © 2014-2017 Broadcom
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
      /** @file v3d_job.c
   *
   * Functions for submitting V3D render jobs to the kernel.
   */
      #include <xf86drm.h>
   #include "v3d_context.h"
   /* The OQ/semaphore packets are the same across V3D versions. */
   #define V3D_VERSION 33
   #include "broadcom/cle/v3dx_pack.h"
   #include "broadcom/common/v3d_macros.h"
   #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "util/set.h"
   #include "util/u_prim.h"
   #include "broadcom/clif/clif_dump.h"
      void
   v3d_job_free(struct v3d_context *v3d, struct v3d_job *job)
   {
         set_foreach(job->bos, entry) {
                                 if (job->write_prscs) {
                                       for (int i = 0; i < job->nr_cbufs; i++) {
            if (job->cbufs[i]) {
            _mesa_hash_table_remove_key(v3d->write_jobs,
   }
   if (job->zsbuf) {
            struct v3d_resource *rsc = v3d_resource(job->zsbuf->texture);
                        _mesa_hash_table_remove_key(v3d->write_jobs,
      }
   if (job->bbuf)
            if (v3d->job == job)
            v3d_destroy_cl(&job->bcl);
   v3d_destroy_cl(&job->rcl);
   v3d_destroy_cl(&job->indirect);
   v3d_bo_unreference(&job->tile_alloc);
            }
      struct v3d_job *
   v3d_job_create(struct v3d_context *v3d)
   {
                           v3d_init_cl(job, &job->bcl);
   v3d_init_cl(job, &job->rcl);
            job->draw_min_x = ~0;
   job->draw_min_y = ~0;
   job->draw_max_x = 0;
            job->bos = _mesa_set_create(job,
               }
      void
   v3d_job_add_bo(struct v3d_job *job, struct v3d_bo *bo)
   {
         if (!bo)
            if (_mesa_set_search(job->bos, bo))
            v3d_bo_reference(bo);
   _mesa_set_add(job->bos, bo);
                     if (job->submit.bo_handle_count >= job->bo_handles_size) {
            job->bo_handles_size = MAX2(4, job->bo_handles_size * 2);
   bo_handles = reralloc(job, bo_handles,
      }
   }
      void
   v3d_job_add_write_resource(struct v3d_job *job, struct pipe_resource *prsc)
   {
                  if (!job->write_prscs) {
            job->write_prscs = _mesa_set_create(job,
               _mesa_set_add(job->write_prscs, prsc);
   }
      void
   v3d_flush_jobs_using_bo(struct v3d_context *v3d, struct v3d_bo *bo)
   {
         hash_table_foreach(v3d->jobs, entry) {
                        }
      void
   v3d_job_add_tf_write_resource(struct v3d_job *job, struct pipe_resource *prsc)
   {
                  if (!job->tf_write_prscs)
            }
      static bool
   v3d_job_writes_resource_from_tf(struct v3d_job *job,
         {
         if (!job->tf_enabled)
            if (!job->tf_write_prscs)
            }
      void
   v3d_flush_jobs_writing_resource(struct v3d_context *v3d,
                     {
         struct hash_entry *entry = _mesa_hash_table_search(v3d->write_jobs,
                  /* We need to sync if graphics pipeline reads a resource written
      * by the compute pipeline. The same would be needed for the case of
   * graphics-compute dependency but nowadays all compute jobs
   * are serialized with the previous submitted job.
      if (!is_compute_pipeline && rsc->bo != NULL && rsc->compute_written) {
      v3d->sync_on_last_compute_job = true;
               if (!entry)
                     bool needs_flush;
   switch (flush_cond) {
   case V3D_FLUSH_ALWAYS:
               case V3D_FLUSH_NOT_CURRENT_JOB:
               case V3D_FLUSH_DEFAULT:
   default:
            /* For writes from TF in the same job we use the "Wait for TF"
   * feature provided by the hardware so we don't want to flush.
   * The exception to this is when the caller is about to map the
   * resource since in that case we don't have a 'Wait for TF'
   * command the in command stream. In this scenario the caller
   * is expected to set 'always_flush' to True.
               if (needs_flush)
   }
      void
   v3d_flush_jobs_reading_resource(struct v3d_context *v3d,
                     {
                  /* We only need to force the flush on TF writes, which is the only
      * case where we might skip the flush to use the 'Wait for TF'
   * command. Here we are flushing for a read, which means that the
   * caller intends to write to the resource, so we don't care if
   * there was a previous TF write to it.
      v3d_flush_jobs_writing_resource(v3d, prsc, flush_cond,
            hash_table_foreach(v3d->jobs, entry) {
                                    bool needs_flush;
   switch (flush_cond) {
   case V3D_FLUSH_NOT_CURRENT_JOB:
               case V3D_FLUSH_ALWAYS:
   case V3D_FLUSH_DEFAULT:
                                       /* Reminder: v3d->jobs is safe to keep iterating even
   * after deletion of an entry.
      }
      /**
   * Returns a v3d_job structure for tracking V3D rendering to a particular FBO.
   *
   * If we've already started rendering to this FBO, then return the same job,
   * otherwise make a new one.  If we're beginning rendering to an FBO, make
   * sure that any previous reads of the FBO (or writes to its color/Z surfaces)
   * have been flushed.
   */
   struct v3d_job *
   v3d_get_job(struct v3d_context *v3d,
               uint32_t nr_cbufs,
   struct pipe_surface **cbufs,
   {
         /* Return the existing job for this FBO if we have one */
   struct v3d_job_key local_key = {
            .cbufs = {
            cbufs[0],
   cbufs[1],
      },
      };
   struct hash_entry *entry = _mesa_hash_table_search(v3d->jobs,
         if (entry)
            /* Creating a new job.  Make sure that any previous jobs reading or
      * writing these buffers are flushed.
      struct v3d_job *job = v3d_job_create(v3d);
            for (int i = 0; i < job->nr_cbufs; i++) {
            if (cbufs[i]) {
                                       }
   if (zsbuf) {
            v3d_flush_jobs_reading_resource(v3d, zsbuf->texture,
               pipe_surface_reference(&job->zsbuf, zsbuf);
      }
   if (bbuf) {
            pipe_surface_reference(&job->bbuf, bbuf);
               for (int i = 0; i < job->nr_cbufs; i++) {
            if (cbufs[i])
      }
   if (zsbuf) {
                     struct v3d_resource *rsc = v3d_resource(zsbuf->texture);
   if (rsc->separate_stencil) {
            v3d_flush_jobs_reading_resource(v3d,
                     _mesa_hash_table_insert(v3d->write_jobs,
                     memcpy(&job->key, &local_key, sizeof(local_key));
            }
      struct v3d_job *
   v3d_get_job_for_fbo(struct v3d_context *v3d)
   {
         if (v3d->job)
            uint32_t nr_cbufs = v3d->framebuffer.nr_cbufs;
   struct pipe_surface **cbufs = v3d->framebuffer.cbufs;
   struct pipe_surface *zsbuf = v3d->framebuffer.zsbuf;
            if (v3d->framebuffer.samples >= 1) {
                        v3d_get_tile_buffer_size(&v3d->screen->devinfo,
                                    /* The dirty flags are tracking what's been updated while v3d->job has
      * been bound, so set them all to ~0 when switching between jobs.  We
   * also need to reset all state at the start of rendering.
               /* If we're binding to uninitialized buffers, no need to load their
      * contents before drawing.
      for (int i = 0; i < nr_cbufs; i++) {
            if (cbufs[i]) {
            struct v3d_resource *rsc = v3d_resource(cbufs[i]->texture);
            if (zsbuf) {
                                                            job->draw_tiles_x = DIV_ROUND_UP(v3d->framebuffer.width,
         job->draw_tiles_y = DIV_ROUND_UP(v3d->framebuffer.height,
                     }
      static void
   v3d_clif_dump(struct v3d_context *v3d, struct v3d_job *job)
   {
         if (!(V3D_DBG(CL) ||
         V3D_DBG(CL_NO_BIN) ||
            struct clif_dump *clif = clif_dump_init(&v3d->screen->devinfo,
                              set_foreach(job->bos, entry) {
                                                               }
      static void
   v3d_read_and_accumulate_primitive_counters(struct v3d_context *v3d)
   {
                  perf_debug("stalling on TF counts readback\n");
   struct v3d_resource *rsc = v3d_resource(v3d->prim_counts);
   if (v3d_bo_wait(rsc->bo, OS_TIMEOUT_INFINITE, "prim-counts")) {
            uint32_t *map = v3d_bo_map(rsc->bo) + v3d->prim_counts_offset;
   v3d->tf_prims_generated += map[V3D_PRIM_COUNTS_TF_WRITTEN];
   /* When we only have a vertex shader with no primitive
   * restart, we determine the primitive count in the CPU so
   * don't update it here again.
   */
   if (v3d->prog.gs || v3d->prim_restart) {
            v3d->prims_generated += map[V3D_PRIM_COUNTS_WRITTEN];
   uint8_t prim_mode =
         v3d->prog.gs ? v3d->prog.gs->prog_data.gs->out_prim_type
   uint32_t vertices_written =
         for (int i = 0; i < v3d->streamout.num_targets; i++) {
         }
      /**
   * Submits the job to the kernel and then reinitializes it.
   */
   void
   v3d_job_submit(struct v3d_context *v3d, struct v3d_job *job)
   {
         struct v3d_screen *screen = v3d->screen;
            if (!job->needs_flush)
            /* The GL_PRIMITIVES_GENERATED query is included with
      * OES_geometry_shader.
      job->needs_primitives_generated =
                  if (job->needs_primitives_generated)
                     if (cl_offset(&job->bcl) > 0)
            /* While the RCL will implicitly depend on the last RCL to have
      * finished, we also need to block on any previous TFU job we may have
   * dispatched.
               /* Update the sync object for the last rendering by our context. */
            job->submit.bcl_end = job->bcl.bo->offset + cl_offset(&job->bcl);
            if (v3d->active_perfmon) {
                        /* If we are submitting a job with a different perfmon, we need to
      * ensure the previous one fully finishes before starting this;
   * otherwise it would wrongly mix counter results.
      if (v3d->active_perfmon != v3d->last_perfmon) {
                        job->submit.flags = 0;
   if (job->tmu_dirty_rcl && screen->has_cache_flush)
            /* On V3D 4.1, the tile alloc/state setup moved to register writes
      * instead of binner packets.
      if (devinfo->ver >= 41) {
                                                      if (!V3D_DBG(NORAST)) {
                     ret = v3d_ioctl(v3d->fd, DRM_IOCTL_V3D_SUBMIT_CL, &job->submit);
   static bool warned = false;
   if (ret && !warned) {
            fprintf(stderr, "Draw call returned %s.  "
      } else if (!ret) {
                        /* If we are submitting a job in the middle of transform
   * feedback or there is a primitives generated query with a
   * geometry shader then we need to read the primitive counts
   * and accumulate them, otherwise they will be reset at the
   * start of the next draw when we emit the Tile Binning Mode
   * Configuration packet.
   *
   * If the job doesn't have any TF draw calls, then we know
   * the primitive count must be zero and we can skip stalling
   * for this. This also fixes a problem because it seems that
   * in this scenario the counters are not reset with the Tile
   * Binning Mode Configuration packet, which would translate
   * to us reading an obsolete (possibly non-zero) value from
   * the GPU counters.
   */
   if (job->needs_primitives_generated ||
      (v3d->streamout.num_targets &&
      done:
         }
      static bool
   v3d_job_compare(const void *a, const void *b)
   {
         }
      static uint32_t
   v3d_job_hash(const void *key)
   {
         }
      void
   v3d_job_init(struct v3d_context *v3d)
   {
         v3d->jobs = _mesa_hash_table_create(v3d,
               v3d->write_jobs = _mesa_hash_table_create(v3d,
         }
   