   /*
   * Copyright © 2016 Intel Corporation
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
      #include <stdio.h>
   #include <stdlib.h>
   #include <stdint.h>
   #include <getopt.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <string.h>
   #include <errno.h>
   #include <sys/stat.h>
   #include <sys/mman.h>
   #include <sys/types.h>
   #include <ctype.h>
      #include "util/macros.h"
      #include "aub_read.h"
   #include "aub_mem.h"
      #include "common/intel_disasm.h"
      #define xtzalloc(name) ((decltype(&name)) calloc(1, sizeof(name)))
   #define xtalloc(name) ((decltype(&name)) malloc(sizeof(name)))
      struct aub_file {
               uint16_t pci_id;
            /* List of batch buffers to process */
   struct {
      const uint8_t *start;
      } *execs;
   int n_execs;
                     /* Device state */
   struct intel_device_info devinfo;
   struct brw_isa_info isa;
      };
      static void
   store_exec_begin(struct aub_file *file)
   {
      if (unlikely(file->n_execs >= file->n_allocated_execs)) {
      file->n_allocated_execs = MAX2(2U * file->n_allocated_execs,
         file->execs = (decltype(file->execs))
      realloc(static_cast<void *>(file->execs),
               }
      static void
   store_exec_end(struct aub_file *file)
   {
      if (file->n_execs > 0 && file->execs[file->n_execs - 1].end == NULL)
      }
      static void
   handle_mem_write(void *user_data, uint64_t phys_addr,
         {
      struct aub_file *file = (struct aub_file *) user_data;
   file->idx_reg_write = 0;
      }
      static void
   handle_ring_write(void *user_data, enum intel_engine_class engine,
         {
      struct aub_file *file = (struct aub_file *) user_data;
   file->idx_reg_write = 0;
      }
      static void
   handle_reg_write(void *user_data, uint32_t reg_offset, uint32_t reg_value)
   {
               /* Only store the first register write of a series (execlist writes take
   * involve 2 dwords).
   */
   if (file->idx_reg_write++ == 0)
      }
      static void
   handle_info(void *user_data, int pci_id, const char *app_name)
   {
      struct aub_file *file = (struct aub_file *) user_data;
            file->pci_id = pci_id;
            if (!intel_get_device_info_from_pci_id(file->pci_id, &file->devinfo)) {
      fprintf(stderr, "can't find device information: pci_id=0x%x\n", file->pci_id);
      }
   brw_init_isa_info(&file->isa, &file->devinfo);
      }
      static void
   handle_error(void *user_data, const void *aub_data, const char *msg)
   {
         }
      static struct aub_file *
   aub_file_open(const char *filename)
   {
      struct aub_file *file;
   struct stat sb;
            file = xtzalloc(*file);
   fd = open(filename, O_RDWR);
   if (fd == -1) {
      fprintf(stderr, "open %s failed: %s\n", filename, strerror(errno));
               if (fstat(fd, &sb) == -1) {
      fprintf(stderr, "stat failed: %s\n", strerror(errno));
               file->map = (uint8_t *) mmap(NULL, sb.st_size,
         if (file->map == MAP_FAILED) {
      fprintf(stderr, "mmap failed: %s\n", strerror(errno));
                        file->cursor = file->map;
            struct aub_read aub_read = {};
   aub_read.user_data = file;
   aub_read.info = handle_info;
   aub_read.error = handle_error;
   aub_read.reg_write = handle_reg_write;
   aub_read.ring_write = handle_ring_write;
   aub_read.local_write = handle_mem_write;
   aub_read.phys_write = handle_mem_write;
   aub_read.ggtt_write = handle_mem_write;
            int consumed;
   while (file->cursor < file->end &&
         (consumed = aub_read_command(&aub_read, file->cursor,
                  /* Ensure we have an end on the last register write. */
   if (file->n_execs > 0 && file->execs[file->n_execs - 1].end == NULL)
               }
      /**/
      static void
   update_mem_for_exec(struct aub_mem *mem, struct aub_file *file, int exec_idx)
   {
      struct aub_read read = {};
   read.user_data = mem;
   read.local_write = aub_mem_local_write;
   read.phys_write = aub_mem_phys_write;
   read.ggtt_write = aub_mem_ggtt_write;
            /* Replay the aub file from the beginning up to just before the
   * commands we want to read. where the context setup happens.
   */
   const uint8_t *iter = file->map;
   while (iter < file->execs[exec_idx].start) {
            }
      /* UI */
      #include <epoxy/gl.h>
      #include "imgui/imgui.h"
   #include "imgui/imgui_memory_editor.h"
   #include "imgui_impl_gtk3.h"
   #include "imgui_impl_opengl3.h"
      #include "aubinator_viewer.h"
   #include "aubinator_viewer_urb.h"
      struct window {
      struct list_head link; /* link in the global list of windows */
                     char name[128];
            ImVec2 position;
            void (*display)(struct window*);
      };
      struct edit_window {
               struct aub_mem *mem;
   uint64_t address;
            struct intel_batch_decode_bo aub_bo;
            struct intel_batch_decode_bo gtt_bo;
               };
      struct pml4_window {
                  };
      struct shader_window {
               uint64_t address;
   char *shader;
      };
      struct urb_window {
               uint32_t end_urb_offset;
               };
      struct batch_window {
               struct aub_mem mem;
                     bool collapsed;
            struct aub_viewer_decode_cfg decode_cfg;
                        };
      static struct Context {
      struct aub_file *file;
   char *input_file;
                     /* UI state*/
   bool show_commands_window;
                              struct window file_window;
   struct window commands_window;
      } context;
      thread_local ImGuiContext* __MesaImGui;
      static int
   map_key(int k)
   {
         }
      static bool
   has_ctrl_key(int key)
   {
         }
      static bool
   window_has_ctrl_key(int key)
   {
         }
      static void
   destroy_window_noop(struct window *win)
   {
   }
      /* Shader windows */
      static void
   display_shader_window(struct window *win)
   {
               if (window->shader) {
      ImGui::InputTextMultiline("Assembly",
                  } else {
            }
      static void
   destroy_shader_window(struct window *win)
   {
               free(window->shader);
      }
      static struct shader_window *
   new_shader_window(struct aub_mem *mem, uint64_t address, const char *desc)
   {
               snprintf(window->base.name, sizeof(window->base.name),
            list_inithead(&window->base.parent_link);
   window->base.position = ImVec2(-1, -1);
   window->base.size = ImVec2(700, 300);
   window->base.opened = true;
   window->base.display = display_shader_window;
            struct intel_batch_decode_bo shader_bo =
         if (shader_bo.map) {
      FILE *f = open_memstream(&window->shader, &window->shader_size);
   if (f) {
      intel_disassemble(&context.file->isa,
                                          }
      /* URB windows */
      static void
   display_urb_window(struct window *win)
   {
      struct urb_window *window = (struct urb_window *) win;
   static const char *stages[] = {
      [AUB_DECODE_STAGE_VS] = "VS",
   [AUB_DECODE_STAGE_HS] = "HS",
   [AUB_DECODE_STAGE_DS] = "DS",
   [AUB_DECODE_STAGE_GS] = "GS",
   [AUB_DECODE_STAGE_PS] = "PS",
               ImGui::Text("URB allocation:");
   window->urb_view.DrawAllocation("##urb",
                        }
      static void
   destroy_urb_window(struct window *win)
   {
                  }
      static struct urb_window *
   new_urb_window(struct aub_viewer_decode_ctx *decode_ctx, uint64_t address)
   {
               snprintf(window->base.name, sizeof(window->base.name),
            list_inithead(&window->base.parent_link);
   window->base.position = ImVec2(-1, -1);
   window->base.size = ImVec2(700, 300);
   window->base.opened = true;
   window->base.display = display_urb_window;
            window->end_urb_offset = decode_ctx->end_urb_offset;
   memcpy(window->urb_stages, decode_ctx->urb_stages, sizeof(window->urb_stages));
                        }
      /* Memory editor windows */
      static uint8_t
   read_edit_window(const uint8_t *data, size_t off)
   {
                  }
      static void
   write_edit_window(uint8_t *data, size_t off, uint8_t d)
   {
      struct edit_window *window = (struct edit_window *) data;
   uint8_t *gtt = (uint8_t *) window->gtt_bo.map + window->gtt_offset + off;
               }
      static void
   display_edit_window(struct window *win)
   {
               if (window->aub_bo.map && window->gtt_bo.map) {
      ImGui::BeginChild(ImGui::GetID("##block"));
   window->editor.DrawContents((uint8_t *) window,
                              } else {
            }
      static void
   destroy_edit_window(struct window *win)
   {
               if (window->aub_bo.map)
            }
      static struct edit_window *
   new_edit_window(struct aub_mem *mem, uint64_t address, uint32_t len)
   {
               snprintf(window->base.name, sizeof(window->base.name),
            list_inithead(&window->base.parent_link);
   window->base.position = ImVec2(-1, -1);
   window->base.size = ImVec2(500, 600);
   window->base.opened = true;
   window->base.display = display_edit_window;
            window->mem = mem;
   window->address = address;
   window->aub_bo = aub_mem_get_ppgtt_addr_aub_data(mem, address);
   window->gtt_bo = aub_mem_get_ppgtt_addr_data(mem, address);
   window->len = len;
   window->editor = MemoryEditor();
   window->editor.OptShowDataPreview = true;
   window->editor.OptShowAscii = false;
   window->editor.ReadFn = read_edit_window;
            if (window->aub_bo.map) {
      uint64_t unaligned_map = (uint64_t) window->aub_bo.map;
   window->aub_bo.map = (const void *)(unaligned_map & ~0xffful);
            if (mprotect((void *) window->aub_bo.map, window->aub_bo.size, PROT_READ | PROT_WRITE) != 0) {
                                          }
      /* 4 level page table walk windows */
      static void
   display_pml4_level(struct aub_mem *mem, uint64_t table_addr, uint64_t table_virt_addr, int level)
   {
      if (level == 0)
            struct intel_batch_decode_bo table_bo =
         const uint64_t *table = (const uint64_t *) ((const uint8_t *) table_bo.map +
         if (!table) {
      ImGui::TextColored(context.cfg.missing_color, "Page not available");
                        if (level == 1) {
      for (int e = 0; e < 512; e++) {
      bool available = (table[e] & 1) != 0;
   uint64_t entry_virt_addr = table_virt_addr + e * addr_increment;
   if (!available)
         ImGui::Text("Entry%03i - phys_addr=0x%" PRIx64 " - virt_addr=0x%" PRIx64,
         } else {
      for (int e = 0; e < 512; e++) {
      bool available = (table[e] & 1) != 0;
   uint64_t entry_virt_addr = table_virt_addr + e * addr_increment;
   if (available &&
      ImGui::TreeNodeEx(&table[e],
                     display_pml4_level(mem, table[e] & ~0xffful, entry_virt_addr, level -1);
               }
      static void
   display_pml4_window(struct window *win)
   {
               ImGui::Text("pml4: %" PRIx64, window->mem->pml4);
   ImGui::BeginChild(ImGui::GetID("##block"));
   display_pml4_level(window->mem, window->mem->pml4, 0, 4);
      }
      static void
   show_pml4_window(struct pml4_window *window, struct aub_mem *mem)
   {
      if (window->base.opened) {
      window->base.opened = false;
               snprintf(window->base.name, sizeof(window->base.name),
            list_inithead(&window->base.parent_link);
   window->base.position = ImVec2(-1, -1);
   window->base.size = ImVec2(500, 600);
   window->base.opened = true;
   window->base.display = display_pml4_window;
                        }
      /* Batch decoding windows */
      static void
   display_decode_options(struct aub_viewer_decode_cfg *cfg)
   {
      char name[40];
   snprintf(name, sizeof(name), "command filter##%p", &cfg->command_filter);
   cfg->command_filter.Draw(name); ImGui::SameLine();
   snprintf(name, sizeof(name), "field filter##%p", &cfg->field_filter);
   cfg->field_filter.Draw(name); ImGui::SameLine();
      }
      static void
   batch_display_shader(void *user_data, const char *shader_desc, uint64_t address)
   {
      struct batch_window *window = (struct batch_window *) user_data;
   struct shader_window *shader_window =
               }
      static void
   batch_display_urb(void *user_data, const struct aub_decode_urb_stage_state *stages)
   {
      struct batch_window *window = (struct batch_window *) user_data;
               }
      static void
   batch_edit_address(void *user_data, uint64_t address, uint32_t len)
   {
      struct batch_window *window = (struct batch_window *) user_data;
   struct edit_window *edit_window =
               }
      static struct intel_batch_decode_bo
   batch_get_bo(void *user_data, bool ppgtt, uint64_t address)
   {
               if (window->uses_ppgtt && ppgtt)
         else
      }
      static void
   update_batch_window(struct batch_window *window, bool reset, int exec_idx)
   {
      if (reset)
                  window->exec_idx = MAX2(MIN2(context.file->n_execs - 1, exec_idx), 0);
      }
      static void
   display_batch_ring_write(void *user_data, enum intel_engine_class engine,
         {
                           }
      static void
   display_batch_execlist_write(void *user_data,
               {
               const uint32_t pphwsp_size = 4096;
   uint32_t pphwsp_addr = context_descriptor & 0xfffff000;
   struct intel_batch_decode_bo pphwsp_bo =
         uint32_t *context_img = (uint32_t *)((uint8_t *)pphwsp_bo.map +
                  uint32_t ring_buffer_head = context_img[5];
   uint32_t ring_buffer_tail = context_img[7];
   uint32_t ring_buffer_start = context_img[9];
                     struct intel_batch_decode_bo ring_bo =
         assert(ring_bo.size > 0);
                     window->decode_ctx.engine = engine;
   aub_viewer_render_batch(&window->decode_ctx, commands,
            }
      static void
   display_batch_window(struct window *win)
   {
               ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / (2 * 2));
   if (window_has_ctrl_key('f')) ImGui::SetKeyboardFocusHere();
   display_decode_options(&window->decode_cfg);
            if (ImGui::InputInt("Execbuf", &window->exec_idx))
            if (window_has_ctrl_key('p'))
         if (window_has_ctrl_key('n'))
            ImGui::Text("execbuf %i", window->exec_idx);
                     struct aub_read read = {};
   read.user_data = window;
   read.ring_write = display_batch_ring_write;
            const uint8_t *iter = context.file->execs[window->exec_idx].start;
   while (iter < context.file->execs[window->exec_idx].end) {
      iter += aub_read_command(&read, iter,
                  }
      static void
   destroy_batch_window(struct window *win)
   {
                        /* This works because children windows are inserted at the back of the
   * list, ensuring the deletion loop goes through the children after calling
   * this function.
   */
   list_for_each_entry(struct window, child_window,
                           }
      static void
   new_batch_window(int exec_idx)
   {
               snprintf(window->base.name, sizeof(window->base.name),
            list_inithead(&window->base.parent_link);
   list_inithead(&window->base.children_windows);
   window->base.position = ImVec2(-1, -1);
   window->base.size = ImVec2(600, 700);
   window->base.opened = true;
   window->base.display = display_batch_window;
            window->collapsed = true;
            aub_viewer_decode_ctx_init(&window->decode_ctx,
                              &context.cfg,
   &window->decode_cfg,
      window->decode_ctx.display_shader = batch_display_shader;
   window->decode_ctx.display_urb = batch_display_urb;
                        }
      /**/
      static void
   display_registers_window(struct window *win)
   {
      static struct ImGuiTextFilter filter;
   if (window_has_ctrl_key('f')) ImGui::SetKeyboardFocusHere();
            ImGui::BeginChild(ImGui::GetID("##block"));
   hash_table_foreach(context.file->spec->registers_by_name, entry) {
      struct intel_group *reg = (struct intel_group *) entry->data;
   if (filter.PassFilter(reg->name) &&
      ImGui::CollapsingHeader(reg->name)) {
   const struct intel_field *field = reg->fields;
   while (field) {
      ImGui::Text("%s : %i -> %i\n", field->name, field->start, field->end);
            }
      }
      static void
   show_register_window(void)
   {
               if (window->opened) {
      window->opened = false;
                        list_inithead(&window->parent_link);
   window->position = ImVec2(-1, -1);
   window->size = ImVec2(200, 400);
   window->opened = true;
   window->display = display_registers_window;
               }
      static void
   display_commands_window(struct window *win)
   {
      static struct ImGuiTextFilter cmd_filter;
   if (window_has_ctrl_key('f')) ImGui::SetKeyboardFocusHere();
   cmd_filter.Draw("name filter");
   static struct ImGuiTextFilter field_filter;
            static char opcode_str[9] = { 0, };
   ImGui::InputText("opcode filter", opcode_str, sizeof(opcode_str),
         size_t opcode_len = strlen(opcode_str);
            static bool show_dwords = true;
            ImGui::BeginChild(ImGui::GetID("##block"));
   hash_table_foreach(context.file->spec->commands, entry) {
      struct intel_group *cmd = (struct intel_group *) entry->data;
   if ((cmd_filter.PassFilter(cmd->name) &&
      (opcode_len == 0 || (opcode & cmd->opcode_mask) == cmd->opcode)) &&
   ImGui::CollapsingHeader(cmd->name)) {
   const struct intel_field *field = cmd->fields;
   int32_t last_dword = -1;
   while (field) {
      if (show_dwords && field->start / 32 != last_dword) {
      for (last_dword = MAX2(0, last_dword + 1);
      last_dword < field->start / 32; last_dword++) {
   ImGui::TextColored(context.cfg.dwords_color,
      }
      }
   if (field_filter.PassFilter(field->name))
                  }
   hash_table_foreach(context.file->spec->structs, entry) {
      struct intel_group *cmd = (struct intel_group *) entry->data;
   if (cmd_filter.PassFilter(cmd->name) && opcode_len == 0 &&
      ImGui::CollapsingHeader(cmd->name)) {
   const struct intel_field *field = cmd->fields;
   int32_t last_dword = -1;
   while (field) {
      if (show_dwords && field->start / 32 != last_dword) {
      last_dword = field->start / 32;
   ImGui::TextColored(context.cfg.dwords_color,
      }
   if (field_filter.PassFilter(field->name))
                  }
      }
      static void
   show_commands_window(void)
   {
               if (window->opened) {
      window->opened = false;
                        list_inithead(&window->parent_link);
   window->position = ImVec2(-1, -1);
   window->size = ImVec2(300, 400);
   window->opened = true;
   window->display = display_commands_window;
               }
      /* Main window */
      static const char *
   human_size(size_t size)
   {
      unsigned divisions = 0;
   double v = size;
   double divider = 1024;
   while (v >= divider) {
      v /= divider;
               static const char *units[] = { "Bytes", "Kilobytes", "Megabytes", "Gigabytes" };
   static char result[20];
   snprintf(result, sizeof(result), "%.2f %s",
            }
      static void
   display_aubfile_window(struct window *win)
   {
      ImGuiColorEditFlags cflags = (ImGuiColorEditFlags_NoAlpha |
                        ImGui::ColorEdit3("background", (float *)&cfg->clear_color, cflags); ImGui::SameLine();
   ImGui::ColorEdit3("missing", (float *)&cfg->missing_color, cflags); ImGui::SameLine();
   ImGui::ColorEdit3("error", (float *)&cfg->error_color, cflags); ImGui::SameLine();
   ImGui::ColorEdit3("highlight", (float *)&cfg->highlight_color, cflags); ImGui::SameLine();
   ImGui::ColorEdit3("dwords", (float *)&cfg->dwords_color, cflags); ImGui::SameLine();
            if (ImGui::Button("Commands list") || has_ctrl_key('c')) { show_commands_window(); } ImGui::SameLine();
   if (ImGui::Button("Registers list") || has_ctrl_key('r')) { show_register_window(); } ImGui::SameLine();
                     ImGui::Text("File name:        %s", context.input_file);
   ImGui::Text("File size:        %s", human_size(context.file->end - context.file->map));
   ImGui::Text("Execbufs          %u", context.file->n_execs);
   ImGui::Text("PCI ID:           0x%x", context.file->pci_id);
   ImGui::Text("Application name: %s", context.file->app_name);
            ImGui::SetNextWindowContentWidth(500);
   if (ImGui::BeginPopupModal("Help", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Some global keybindings:");
            static const char *texts[] = {
      "Ctrl-h",          "show this screen",
   "Ctrl-c",          "show commands list",
   "Ctrl-r",          "show registers list",
   "Ctrl-b",          "new batch window",
   "Ctrl-p/n",        "switch to previous/next batch buffer",
   "Ctrl-Tab",        "switch focus between window",
      };
   float align = 0.0f;
   for (uint32_t i = 0; i < ARRAY_SIZE(texts); i += 2)
                  for (uint32_t i = 0; i < ARRAY_SIZE(texts); i += 2) {
                  if (ImGui::Button("Done") || ImGui::IsKeyPressed(ImGuiKey_Escape))
               }
      static void
   show_aubfile_window(void)
   {
               if (window->opened)
            snprintf(window->name, sizeof(window->name),
            list_inithead(&window->parent_link);
   window->size = ImVec2(-1, 250);
   window->position = ImVec2(0, 0);
   window->opened = true;
   window->display = display_aubfile_window;
               }
      /* Main redrawing */
      static void
   display_windows(void)
   {
      /* Start by disposing closed windows, we don't want to destroy windows that
   * have already been scheduled to be painted. So destroy always happens on
   * the next draw cycle, prior to any drawing.
   */
   list_for_each_entry_safe(struct window, window, &context.windows, link) {
      if (window->opened)
            /* Can't close this one. */
   if (window == &context.file_window) {
      window->opened = true;
               list_del(&window->link);
   list_del(&window->parent_link);
   if (window->destroy)
               list_for_each_entry_safe(struct window, window, &context.windows, link) {
      ImGui::SetNextWindowPos(window->position, ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(window->size, ImGuiCond_FirstUseEver);
   if (ImGui::Begin(window->name, &window->opened)) {
      window->display(window);
   window->position = ImGui::GetWindowPos();
      }
   if (window_has_ctrl_key('w'))
               }
      static void
   repaint_area(GtkGLArea *area, GdkGLContext *gdk_gl_context)
   {
      ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGtk3_NewFrame();
                     ImGui::EndFrame();
            glClearColor(context.cfg.clear_color.Value.x,
               glClear(GL_COLOR_BUFFER_BIT);
      }
      static void
   realize_area(GtkGLArea *area)
   {
      ImGui::CreateContext();
   ImGui_ImplGtk3_Init(GTK_WIDGET(area), true);
                     ImGui::StyleColorsDark();
            ImGuiIO& io = ImGui::GetIO();
      }
      static void
   unrealize_area(GtkGLArea *area)
   {
               ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGtk3_Shutdown();
      }
      static void
   size_allocate_area(GtkGLArea *area,
               {
      if (!gtk_widget_get_realized(GTK_WIDGET(area)))
            /* We want to catch only initial size allocate. */
   g_signal_handlers_disconnect_by_func(area,
                  }
      static void
   print_help(const char *progname, FILE *file)
   {
      fprintf(file,
         "Usage: %s [OPTION]... FILE\n"
   "Decode aub file contents from FILE.\n\n"
   "      --help             display this help and exit\n"
      }
      int main(int argc, char *argv[])
   {
      int c, i;
   bool help = false;
   const struct option aubinator_opts[] = {
      { "help",          no_argument,       (int *) &help,                 true },
   { "xml",           required_argument, NULL,                          'x' },
                        i = 0;
   while ((c = getopt_long(argc, argv, "x:s:", aubinator_opts, &i)) != -1) {
      switch (c) {
   case 'x':
      context.xml_path = strdup(optarg);
      default:
                     if (optind < argc)
            if (help || !context.input_file) {
      print_help(argv[0], stderr);
                                 context.gtk_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(context.gtk_window), "Aubinator Viewer");
   g_signal_connect(context.gtk_window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
            GtkWidget* gl_area = gtk_gl_area_new();
   g_signal_connect(gl_area, "render", G_CALLBACK(repaint_area), NULL);
   g_signal_connect(gl_area, "realize", G_CALLBACK(realize_area), NULL);
   g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize_area), NULL);
   g_signal_connect(gl_area, "size_allocate", G_CALLBACK(size_allocate_area), NULL);
                                          }
