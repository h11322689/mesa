   /*
   * Copyright © 2023 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "git_sha1.h"
   #include "pvr_dump_info.h"
   #include "pvr_dump.h"
      static inline void pvr_dump_field_bvnc(struct pvr_dump_ctx *ctx,
               {
      pvr_dump_field_computed(ctx,
                           name,
   "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16,
   "0x%08" PRIx64,
      }
      static inline void pvr_dump_field_drm_version(struct pvr_dump_ctx *ctx,
                                       {
      pvr_dump_field(ctx,
                  name,
   "%s %d.%d.%d (%s)",
   drm_name,
   drm_version_major,
   }
      static inline void pvr_dump_field_compatible_strings(struct pvr_dump_ctx *ctx,
         {
      char *const *temp_comp = comp;
   uint32_t count_log10;
            if (!*comp) {
      pvr_dump_println(ctx, "<empty>");
               while (*temp_comp++)
            count_log10 = u32_dec_digits(index);
            while (*comp)
      }
      void pvr_dump_physical_device_info(const struct pvr_device_dump_info *dump_info)
   {
      const struct pvr_device_runtime_info *run_info =
         const struct pvr_device_info *dev_info = dump_info->device_info;
                     pvr_dump_mark_section(&ctx, "General Info");
   pvr_dump_indent(&ctx);
   pvr_dump_field_string(&ctx, "Public Name", dev_info->ident.public_name);
   pvr_dump_field_string(&ctx, "Series Name", dev_info->ident.series_name);
   pvr_dump_field_bvnc(&ctx, "BVNC", dev_info);
   pvr_dump_field_drm_version(&ctx,
                              "DRM Display Driver Version",
      pvr_dump_field_drm_version(&ctx,
                              "DRM Render Driver Version",
      pvr_dump_field_string(&ctx, "MESA ", PACKAGE_VERSION MESA_GIT_SHA1);
            pvr_dump_mark_section(&ctx, "Display Platform Compatible Strings");
   pvr_dump_indent(&ctx);
   pvr_dump_field_compatible_strings(&ctx, dump_info->drm_display.comp);
            pvr_dump_mark_section(&ctx, "Render Platform Compatible Strings");
   pvr_dump_indent(&ctx);
   pvr_dump_field_compatible_strings(&ctx, dump_info->drm_render.comp);
   pvr_dump_dedent(&ctx);
            pvr_dump_mark_section(&ctx, "Runtime Info");
   pvr_dump_indent(&ctx);
   pvr_dump_field_member_u64(&ctx, run_info, cdm_max_local_mem_size_regs);
   pvr_dump_field_member_u64_units(&ctx, run_info, max_free_list_size, "bytes");
   pvr_dump_field_member_u64_units(&ctx, run_info, min_free_list_size, "bytes");
   pvr_dump_field_member_u64_units(&ctx,
                     pvr_dump_field_member_u64_units(&ctx,
                     pvr_dump_field_member_u32(&ctx, run_info, core_count);
   pvr_dump_field_member_u64(&ctx, run_info, max_coeffs);
   pvr_dump_field_member_u64(&ctx, run_info, num_phantoms);
   pvr_dump_dedent(&ctx);
               }
