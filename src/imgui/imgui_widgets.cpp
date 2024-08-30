   // dear imgui, v1.68 WIP
   // (widgets code)
      /*
      Index of this file:
      // [SECTION] Forward Declarations
   // [SECTION] Widgets: Text, etc.
   // [SECTION] Widgets: Main (Button, Image, Checkbox, RadioButton, ProgressBar, Bullet, etc.)
   // [SECTION] Widgets: Low-level Layout helpers (Spacing, Dummy, NewLine, Separator, etc.)
   // [SECTION] Widgets: ComboBox
   // [SECTION] Data Type and Data Formatting Helpers
   // [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
   // [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
   // [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
   // [SECTION] Widgets: InputText, InputTextMultiline
   // [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
   // [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
   // [SECTION] Widgets: Selectable
   // [SECTION] Widgets: ListBox
   // [SECTION] Widgets: PlotLines, PlotHistogram
   // [SECTION] Widgets: Value helpers
   // [SECTION] Widgets: MenuItem, BeginMenu, EndMenu, etc.
   // [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
   // [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
      */
      #if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
   #define _CRT_SECURE_NO_WARNINGS
   #endif
      #include "imgui.h"
   #ifndef IMGUI_DEFINE_MATH_OPERATORS
   #define IMGUI_DEFINE_MATH_OPERATORS
   #endif
   #include "imgui_internal.h"
      #include <ctype.h>      // toupper, isprint
   #if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
   #include <stddef.h>     // intptr_t
   #else
   #include <stdint.h>     // intptr_t
   #endif
      // Visual Studio warnings
   #ifdef _MSC_VER
   #pragma warning (disable: 4127) // condition expression is constant
   #pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
   #endif
      // Clang/GCC warnings with -Weverything
   #ifdef __clang__
   #pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
   #pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants (typically 0.0f) is ok.
   #pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
   #pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
   #if __has_warning("-Wzero-as-null-pointer-constant")
   #pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning : zero as null pointer constant              // some standard header variations use #define NULL 0
   #endif
   #if __has_warning("-Wdouble-promotion")
   #pragma clang diagnostic ignored "-Wdouble-promotion"       // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
   #endif
   #elif defined(__GNUC__)
   #pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
   #if __GNUC__ >= 8
   #pragma GCC diagnostic ignored "-Wclass-memaccess"          // warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
   #endif
   #endif
      //-------------------------------------------------------------------------
   // Data
   //-------------------------------------------------------------------------
      // Those MIN/MAX values are not define because we need to point to them
   static const ImS32  IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
   static const ImS32  IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
   static const ImU32  IM_U32_MIN = 0;
   static const ImU32  IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
   #ifdef LLONG_MIN
   static const ImS64  IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
   static const ImS64  IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
   #else
   static const ImS64  IM_S64_MIN = -9223372036854775807LL - 1;
   static const ImS64  IM_S64_MAX = 9223372036854775807LL;
   #endif
   static const ImU64  IM_U64_MIN = 0;
   #ifdef ULLONG_MAX
   static const ImU64  IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
   #else
   static const ImU64  IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
   #endif
      //-------------------------------------------------------------------------
   // [SECTION] Forward Declarations
   //-------------------------------------------------------------------------
      // Data Type helpers
   static inline int       DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* data_ptr, const char* format);
   static void             DataTypeApplyOp(ImGuiDataType data_type, int op, void* output, void* arg_1, const void* arg_2);
   static bool             DataTypeApplyOpFromText(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* format);
      // For InputTextEx()
   static bool             InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data);
   static int              InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end);
   static ImVec2           InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining = NULL, ImVec2* out_offset = NULL, bool stop_on_new_line = false);
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: Text, etc.
   //-------------------------------------------------------------------------
   // - TextUnformatted()
   // - Text()
   // - TextV()
   // - TextColored()
   // - TextColoredV()
   // - TextDisabled()
   // - TextDisabledV()
   // - TextWrapped()
   // - TextWrappedV()
   // - LabelText()
   // - LabelTextV()
   // - BulletText()
   // - BulletTextV()
   //-------------------------------------------------------------------------
      void ImGui::TextUnformatted(const char* text, const char* text_end)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   IM_ASSERT(text != NULL);
   const char* text_begin = text;
   if (text_end == NULL)
            const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrentLineTextBaseOffset);
   const float wrap_pos_x = window->DC.TextWrapPos;
   const bool wrap_enabled = wrap_pos_x >= 0.0f;
   if (text_end - text > 2000 && !wrap_enabled)
   {
      // Long text!
   // Perform manual coarse clipping to optimize for long multi-line text
   // - From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled.
   // - We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
   // - We use memchr(), pay attention that well optimized versions of those str/mem functions are much faster than a casually written loop.
   const char* line = text;
   const float line_height = GetTextLineHeight();
   const ImRect clip_rect = window->ClipRect;
            if (text_pos.y <= clip_rect.Max.y)
   {
                  // Lines to skip (can't skip when logging text)
   if (!g.LogEnabled)
   {
      int lines_skippable = (int)((clip_rect.Min.y - text_pos.y) / line_height);
   if (lines_skippable > 0)
   {
      int lines_skipped = 0;
   while (line < text_end && lines_skipped < lines_skippable)
   {
         const char* line_end = (const char*)memchr(line, '\n', text_end - line);
   if (!line_end)
         line = line_end + 1;
   }
                  // Lines to render
   if (line < text_end)
   {
      ImRect line_rect(pos, pos + ImVec2(FLT_MAX, line_height));
   while (line < text_end)
                           const char* line_end = (const char*)memchr(line, '\n', text_end - line);
   if (!line_end)
         const ImVec2 line_size = CalcTextSize(line, line_end, false);
   text_size.x = ImMax(text_size.x, line_size.x);
   RenderText(pos, line, line_end, false);
   line = line_end + 1;
   line_rect.Min.y += line_height;
                     // Count remaining lines
   int lines_skipped = 0;
   while (line < text_end)
   {
      const char* line_end = (const char*)memchr(line, '\n', text_end - line);
   if (!line_end)
         line = line_end + 1;
                                 ImRect bb(text_pos, text_pos + text_size);
   ItemSize(text_size);
      }
   else
   {
      const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
            // Account of baseline offset
   ImRect bb(text_pos, text_pos + text_size);
   ItemSize(text_size);
   if (!ItemAdd(bb, 0))
            // Render (we don't hide text after ## in this end-user function)
         }
      void ImGui::Text(const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   TextV(fmt, args);
      }
      void ImGui::TextV(const char* fmt, va_list args)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
      }
      void ImGui::TextColored(const ImVec4& col, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   TextColoredV(col, fmt, args);
      }
      void ImGui::TextColoredV(const ImVec4& col, const char* fmt, va_list args)
   {
      PushStyleColor(ImGuiCol_Text, col);
   TextV(fmt, args);
      }
      void ImGui::TextDisabled(const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   TextDisabledV(fmt, args);
      }
      void ImGui::TextDisabledV(const char* fmt, va_list args)
   {
      PushStyleColor(ImGuiCol_Text, GImGui->Style.Colors[ImGuiCol_TextDisabled]);
   TextV(fmt, args);
      }
      void ImGui::TextWrapped(const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   TextWrappedV(fmt, args);
      }
      void ImGui::TextWrappedV(const char* fmt, va_list args)
   {
      bool need_backup = (GImGui->CurrentWindow->DC.TextWrapPos < 0.0f);  // Keep existing wrap position if one is already set
   if (need_backup)
         TextV(fmt, args);
   if (need_backup)
      }
      void ImGui::LabelText(const char* label, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   LabelTextV(label, fmt, args);
      }
      // Add a label+text combo aligned to other label+value widgets
   void ImGui::LabelTextV(const char* label, const char* fmt, va_list args)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
            const ImVec2 label_size = CalcTextSize(label, NULL, true);
   const ImRect value_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2));
   const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + (label_size.x > 0.0f ? style.ItemInnerSpacing.x : 0.0f), style.FramePadding.y*2) + label_size);
   ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, 0))
            // Render
   const char* value_text_begin = &g.TempBuffer[0];
   const char* value_text_end = value_text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
   RenderTextClipped(value_bb.Min, value_bb.Max, value_text_begin, value_text_end, NULL, ImVec2(0.0f,0.5f));
   if (label_size.x > 0.0f)
      }
      void ImGui::BulletText(const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   BulletTextV(fmt, args);
      }
      // Text with a little bullet aligned to the typical tree node.
   void ImGui::BulletTextV(const char* fmt, va_list args)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
            const char* text_begin = g.TempBuffer;
   const char* text_end = text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
   const ImVec2 label_size = CalcTextSize(text_begin, text_end, false);
   const float text_base_offset_y = ImMax(0.0f, window->DC.CurrentLineTextBaseOffset); // Latch before ItemSize changes it
   const float line_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + g.Style.FramePadding.y*2), g.FontSize);
   const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize + (label_size.x > 0.0f ? (label_size.x + style.FramePadding.x*2) : 0.0f), ImMax(line_height, label_size.y)));  // Empty text doesn't add padding
   ItemSize(bb);
   if (!ItemAdd(bb, 0))
            // Render
   RenderBullet(bb.Min + ImVec2(style.FramePadding.x + g.FontSize*0.5f, line_height*0.5f));
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: Main
   //-------------------------------------------------------------------------
   // - ButtonBehavior() [Internal]
   // - Button()
   // - SmallButton()
   // - InvisibleButton()
   // - ArrowButton()
   // - CloseButton() [Internal]
   // - CollapseButton() [Internal]
   // - Scrollbar() [Internal]
   // - Image()
   // - ImageButton()
   // - Checkbox()
   // - CheckboxFlags()
   // - RadioButton()
   // - ProgressBar()
   // - Bullet()
   //-------------------------------------------------------------------------
      bool ImGui::ButtonBehavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags)
   {
      ImGuiContext& g = *GImGui;
            if (flags & ImGuiButtonFlags_Disabled)
   {
      if (out_hovered) *out_hovered = false;
   if (out_held) *out_held = false;
   if (g.ActiveId == id) ClearActiveID();
               // Default behavior requires click+release on same spot
   if ((flags & (ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnRelease | ImGuiButtonFlags_PressedOnDoubleClick)) == 0)
            ImGuiWindow* backup_hovered_window = g.HoveredWindow;
   if ((flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredRootWindow == window)
         #ifdef IMGUI_ENABLE_TEST_ENGINE
      if (id != 0 && window->DC.LastItemId != id)
      #endif
         bool pressed = false;
            // Drag source doesn't report as hovered
   if (hovered && g.DragDropActive && g.DragDropPayload.SourceId == id && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoDisableHover))
            // Special mode for Drag and Drop where holding button pressed for a long time while dragging another item triggers the button
   if (g.DragDropActive && (flags & ImGuiButtonFlags_PressedOnDragDropHold) && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoHoldToOpenOthers))
      if (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
   {
         hovered = true;
   SetHoveredID(id);
   if (CalcTypematicPressedRepeatAmount(g.HoveredIdTimer + 0.0001f, g.HoveredIdTimer + 0.0001f - g.IO.DeltaTime, 0.01f, 0.70f)) // FIXME: Our formula for CalcTypematicPressedRepeatAmount() is fishy
   {
      pressed = true;
            if ((flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredRootWindow == window)
            // AllowOverlap mode (rarely used) requires previous frame HoveredId to be null or to match. This allows using patterns where a later submitted widget overlaps a previous one.
   if (hovered && (flags & ImGuiButtonFlags_AllowItemOverlap) && (g.HoveredIdPreviousFrame != id && g.HoveredIdPreviousFrame != 0))
            // Mouse
   if (hovered)
   {
      if (!(flags & ImGuiButtonFlags_NoKeyModifiers) || (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt))
   {
         //                        | CLICKING        | HOLDING with ImGuiButtonFlags_Repeat
   // PressedOnClickRelease  |  <on release>*  |  <on repeat> <on repeat> .. (NOT on release)  <-- MOST COMMON! (*) only if both click/release were over bounds
   // PressedOnClick         |  <on click>     |  <on click> <on repeat> <on repeat> ..
   // PressedOnRelease       |  <on release>   |  <on repeat> <on repeat> .. (NOT on release)
   // PressedOnDoubleClick   |  <on dclick>    |  <on dclick> <on repeat> <on repeat> ..
   // FIXME-NAV: We don't honor those different behaviors.
   if ((flags & ImGuiButtonFlags_PressedOnClickRelease) && g.IO.MouseClicked[0])
   {
      SetActiveID(id, window);
   if (!(flags & ImGuiButtonFlags_NoNavFocus))
            }
   if (((flags & ImGuiButtonFlags_PressedOnClick) && g.IO.MouseClicked[0]) || ((flags & ImGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseDoubleClicked[0]))
   {
      pressed = true;
   if (flags & ImGuiButtonFlags_NoHoldingActiveID)
         else
            }
   if ((flags & ImGuiButtonFlags_PressedOnRelease) && g.IO.MouseReleased[0])
   {
      if (!((flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[0] >= g.IO.KeyRepeatDelay))  // Repeat mode trumps <on release>
                     // 'Repeat' mode acts when held regardless of _PressedOn flags (see table above).
   // Relies on repeat logic of IsMouseClicked() but we may as well do it ourselves if we end up exposing finer RepeatDelay/RepeatRate settings.
   if ((flags & ImGuiButtonFlags_Repeat) && g.ActiveId == id && g.IO.MouseDownDuration[0] > 0.0f && IsMouseClicked(0, true))
            if (pressed)
               // Gamepad/Keyboard navigation
   // We report navigated item as hovered but we don't set g.HoveredId to not interfere with mouse.
   if (g.NavId == id && !g.NavDisableHighlight && g.NavDisableMouseHover && (g.ActiveId == 0 || g.ActiveId == id || g.ActiveId == window->MoveId))
            if (g.NavActivateDownId == id)
   {
      bool nav_activated_by_code = (g.NavActivateId == id);
   bool nav_activated_by_inputs = IsNavInputPressed(ImGuiNavInput_Activate, (flags & ImGuiButtonFlags_Repeat) ? ImGuiInputReadMode_Repeat : ImGuiInputReadMode_Pressed);
   if (nav_activated_by_code || nav_activated_by_inputs)
         if (nav_activated_by_code || nav_activated_by_inputs || g.ActiveId == id)
   {
         // Set active id so it can be queried by user via IsItemActive(), equivalent of holding the mouse button.
   g.NavActivateId = id; // This is so SetActiveId assign a Nav source
   SetActiveID(id, window);
   if ((nav_activated_by_code || nav_activated_by_inputs) && !(flags & ImGuiButtonFlags_NoNavFocus))
                     bool held = false;
   if (g.ActiveId == id)
   {
      if (pressed)
         if (g.ActiveIdSource == ImGuiInputSource_Mouse)
   {
         if (g.ActiveIdIsJustActivated)
         if (g.IO.MouseDown[0])
   {
         }
   else
   {
      if (hovered && (flags & ImGuiButtonFlags_PressedOnClickRelease))
      if (!((flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[0] >= g.IO.KeyRepeatDelay))  // Repeat mode trumps <on release>
               }
   if (!(flags & ImGuiButtonFlags_NoNavFocus))
   }
   else if (g.ActiveIdSource == ImGuiInputSource_Nav)
   {
         if (g.NavActivateDownId != id)
               if (out_hovered) *out_hovered = hovered;
               }
      bool ImGui::ButtonEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const ImGuiID id = window->GetID(label);
            ImVec2 pos = window->DC.CursorPos;
   if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
                  const ImRect bb(pos, pos + size);
   ItemSize(size, style.FramePadding.y);
   if (!ItemAdd(bb, id))
            if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
         bool hovered, held;
   bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
   if (pressed)
            // Render
   const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
   RenderNavHighlight(bb, id);
   RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
            // Automatically close popups
   //if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
      }
      bool ImGui::Button(const char* label, const ImVec2& size_arg)
   {
         }
      // Small buttons fits within text without additional vertical spacing.
   bool ImGui::SmallButton(const char* label)
   {
      ImGuiContext& g = *GImGui;
   float backup_padding_y = g.Style.FramePadding.y;
   g.Style.FramePadding.y = 0.0f;
   bool pressed = ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine);
   g.Style.FramePadding.y = backup_padding_y;
      }
      // Tip: use ImGui::PushID()/PopID() to push indices or pointers in the ID stack.
   // Then you can keep 'str_id' empty or the same for all your buttons (instead of creating a string based on a non-string id)
   bool ImGui::InvisibleButton(const char* str_id, const ImVec2& size_arg)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            // Cannot use zero-size for InvisibleButton(). Unlike Button() there is not way to fallback using the label size.
            const ImGuiID id = window->GetID(str_id);
   ImVec2 size = CalcItemSize(size_arg, 0.0f, 0.0f);
   const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
   ItemSize(size);
   if (!ItemAdd(bb, id))
            bool hovered, held;
               }
      bool ImGui::ArrowButtonEx(const char* str_id, ImGuiDir dir, ImVec2 size, ImGuiButtonFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiID id = window->GetID(str_id);
   const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
   const float default_size = GetFrameHeight();
   ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
   if (!ItemAdd(bb, id))
            if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
            bool hovered, held;
            // Render
   const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
   RenderNavHighlight(bb, id);
   RenderFrame(bb.Min, bb.Max, col, true, g.Style.FrameRounding);
               }
      bool ImGui::ArrowButton(const char* str_id, ImGuiDir dir)
   {
      float sz = GetFrameHeight();
      }
      // Button to close a window
   bool ImGui::CloseButton(ImGuiID id, const ImVec2& pos, float radius)
   {
      ImGuiContext& g = *GImGui;
            // We intentionally allow interaction when clipped so that a mechanical Alt,Right,Validate sequence close a window.
   // (this isn't the regular behavior of buttons, but it doesn't affect the user much because navigation tends to keep items visible).
   const ImRect bb(pos - ImVec2(radius,radius), pos + ImVec2(radius,radius));
            bool hovered, held;
   bool pressed = ButtonBehavior(bb, id, &hovered, &held);
   if (is_clipped)
            // Render
   ImVec2 center = bb.GetCenter();
   if (hovered)
            float cross_extent = (radius * 0.7071f) - 1.0f;
   ImU32 cross_col = GetColorU32(ImGuiCol_Text);
   center -= ImVec2(0.5f, 0.5f);
   window->DrawList->AddLine(center + ImVec2(+cross_extent,+cross_extent), center + ImVec2(-cross_extent,-cross_extent), cross_col, 1.0f);
               }
      bool ImGui::CollapseButton(ImGuiID id, const ImVec2& pos)
   {
      ImGuiContext& g = *GImGui;
            ImRect bb(pos, pos + ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.0f);
   ItemAdd(bb, id);
   bool hovered, held;
            ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
   if (hovered || held)
                  // Switch to moving the window after mouse is moved beyond the initial drag threshold
   if (IsItemActive() && IsMouseDragging())
               }
      ImGuiID ImGui::GetScrollbarID(ImGuiLayoutType direction)
   {
      ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
      }
      // Vertical/Horizontal scrollbar
   // The entire piece of code below is rather confusing because:
   // - We handle absolute seeking (when first clicking outside the grab) and relative manipulation (afterward or when clicking inside the grab)
   // - We store values as normalized ratio and in a form that allows the window content to change while we are holding on a scrollbar
   // - We handle both horizontal and vertical scrollbars, which makes the terminology not ideal.
   void ImGui::Scrollbar(ImGuiLayoutType direction)
   {
      ImGuiContext& g = *GImGui;
            const bool horizontal = (direction == ImGuiLayoutType_Horizontal);
   const ImGuiStyle& style = g.Style;
            // Render background
   bool other_scrollbar = (horizontal ? window->ScrollbarY : window->ScrollbarX);
   float other_scrollbar_size_w = other_scrollbar ? style.ScrollbarSize : 0.0f;
   const ImRect window_rect = window->Rect();
   const float border_size = window->WindowBorderSize;
   ImRect bb = horizontal
      ? ImRect(window->Pos.x + border_size, window_rect.Max.y - style.ScrollbarSize, window_rect.Max.x - other_scrollbar_size_w - border_size, window_rect.Max.y - border_size)
      if (!horizontal)
            const float bb_height = bb.GetHeight();
   if (bb.GetWidth() <= 0.0f || bb_height <= 0.0f)
            // When we are too small, start hiding and disabling the grab (this reduce visual noise on very small window and facilitate using the resize grab)
   float alpha = 1.0f;
   if ((direction == ImGuiLayoutType_Vertical) && bb_height < g.FontSize + g.Style.FramePadding.y * 2.0f)
   {
      alpha = ImSaturate((bb_height - g.FontSize) / (g.Style.FramePadding.y * 2.0f));
   if (alpha <= 0.0f)
      }
            int window_rounding_corners;
   if (horizontal)
         else
         window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_ScrollbarBg), window->WindowRounding, window_rounding_corners);
            // V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
   float scrollbar_size_v = horizontal ? bb.GetWidth() : bb.GetHeight();
   float scroll_v = horizontal ? window->Scroll.x : window->Scroll.y;
   float win_size_avail_v = (horizontal ? window->SizeFull.x : window->SizeFull.y) - other_scrollbar_size_w;
            // Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
   // But we maintain a minimum size in pixel to allow for the user to still aim inside.
   IM_ASSERT(ImMax(win_size_contents_v, win_size_avail_v) > 0.0f); // Adding this assert to check if the ImMax(XXX,1.0f) is still needed. PLEASE CONTACT ME if this triggers.
   const float win_size_v = ImMax(ImMax(win_size_contents_v, win_size_avail_v), 1.0f);
   const float grab_h_pixels = ImClamp(scrollbar_size_v * (win_size_avail_v / win_size_v), style.GrabMinSize, scrollbar_size_v);
            // Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
   bool held = false;
   bool hovered = false;
   const bool previously_held = (g.ActiveId == id);
            float scroll_max = ImMax(1.0f, win_size_contents_v - win_size_avail_v);
   float scroll_ratio = ImSaturate(scroll_v / scroll_max);
   float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;
   if (held && allow_interaction && grab_h_norm < 1.0f)
   {
      float scrollbar_pos_v = horizontal ? bb.Min.x : bb.Min.y;
   float mouse_pos_v = horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
            // Click position in scrollbar normalized space (0.0f->1.0f)
   const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);
            bool seek_absolute = false;
   if (!previously_held)
   {
         // On initial click calculate the distance between mouse and the center of the grab
   if (clicked_v_norm >= grab_v_norm && clicked_v_norm <= grab_v_norm + grab_h_norm)
   {
         }
   else
   {
      seek_absolute = true;
               // Apply scroll
   // It is ok to modify Scroll here because we are being called in Begin() after the calculation of SizeContents and before setting up our starting position
   const float scroll_v_norm = ImSaturate((clicked_v_norm - *click_delta_to_grab_center_v - grab_h_norm*0.5f) / (1.0f - grab_h_norm));
   scroll_v = (float)(int)(0.5f + scroll_v_norm * scroll_max);//(win_size_contents_v - win_size_v));
   if (horizontal)
         else
            // Update values for rendering
   scroll_ratio = ImSaturate(scroll_v / scroll_max);
            // Update distance to grab now that we have seeked and saturated
   if (seek_absolute)
               // Render grab
   const ImU32 grab_col = GetColorU32(held ? ImGuiCol_ScrollbarGrabActive : hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab, alpha);
   ImRect grab_rect;
   if (horizontal)
         else
            }
      void ImGui::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
   if (border_col.w > 0.0f)
         ItemSize(bb);
   if (!ItemAdd(bb, 0))
            if (border_col.w > 0.0f)
   {
      window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
      }
   else
   {
            }
      // frame_padding < 0: uses FramePadding from style (default)
   // frame_padding = 0: no framing
   // frame_padding > 0: set framing size
   // The color used are the button colors.
   bool ImGui::ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
            // Default to using texture ID as ID. User can still push string/integer prefixes.
   // We could hash the size/uv to create a unique ID but that would prevent the user from animating UV.
   PushID((void*)(intptr_t)user_texture_id);
   const ImGuiID id = window->GetID("#image");
            const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
   const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
   const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
   ItemSize(bb);
   if (!ItemAdd(bb, id))
            bool hovered, held;
            // Render
   const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
   RenderNavHighlight(bb, id);
   RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
   if (bg_col.w > 0.0f)
                     }
      bool ImGui::Checkbox(const char* label, bool* v)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const ImGuiID id = window->GetID(label);
            const float square_sz = GetFrameHeight();
   const ImVec2 pos = window->DC.CursorPos;
   const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
   ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, id))
            bool hovered, held;
   bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
   if (pressed)
   {
      *v = !(*v);
               const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
   RenderNavHighlight(total_bb, id);
   RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
   if (*v)
   {
      const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
               if (g.LogEnabled)
         if (label_size.x > 0.0f)
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
      }
      bool ImGui::CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
   {
      bool v = ((*flags & flags_value) == flags_value);
   bool pressed = Checkbox(label, &v);
   if (pressed)
   {
      if (v)
         else
                  }
      bool ImGui::RadioButton(const char* label, bool active)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const ImGuiID id = window->GetID(label);
            const float square_sz = GetFrameHeight();
   const ImVec2 pos = window->DC.CursorPos;
   const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
   const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
   ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, id))
            ImVec2 center = check_bb.GetCenter();
   center.x = (float)(int)center.x + 0.5f;
   center.y = (float)(int)center.y + 0.5f;
            bool hovered, held;
   bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
   if (pressed)
            RenderNavHighlight(total_bb, id);
   window->DrawList->AddCircleFilled(center, radius, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
   if (active)
   {
      const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
               if (style.FrameBorderSize > 0.0f)
   {
      window->DrawList->AddCircle(center + ImVec2(1,1), radius, GetColorU32(ImGuiCol_BorderShadow), 16, style.FrameBorderSize);
               if (g.LogEnabled)
         if (label_size.x > 0.0f)
               }
      bool ImGui::RadioButton(const char* label, int* v, int v_button)
   {
      const bool pressed = RadioButton(label, *v == v_button);
   if (pressed)
            }
      // size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
   void ImGui::ProgressBar(float fraction, const ImVec2& size_arg, const char* overlay)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
            ImVec2 pos = window->DC.CursorPos;
   ImRect bb(pos, pos + CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y*2.0f));
   ItemSize(bb, style.FramePadding.y);
   if (!ItemAdd(bb, 0))
            // Render
   fraction = ImSaturate(fraction);
   RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
   bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
   const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
            // Default displaying the fraction as percentage string, but user can override it
   char overlay_buf[32];
   if (!overlay)
   {
      ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction*100+0.01f);
               ImVec2 overlay_size = CalcTextSize(overlay, NULL);
   if (overlay_size.x > 0.0f)
      }
      void ImGui::Bullet()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const float line_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + g.Style.FramePadding.y*2), g.FontSize);
   const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
   ItemSize(bb);
   if (!ItemAdd(bb, 0))
   {
      SameLine(0, style.FramePadding.x*2);
               // Render and stay on same line
   RenderBullet(bb.Min + ImVec2(style.FramePadding.x + g.FontSize*0.5f, line_height*0.5f));
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: Low-level Layout helpers
   //-------------------------------------------------------------------------
   // - Spacing()
   // - Dummy()
   // - NewLine()
   // - AlignTextToFramePadding()
   // - Separator()
   // - VerticalSeparator() [Internal]
   // - SplitterBehavior() [Internal]
   //-------------------------------------------------------------------------
      void ImGui::Spacing()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            }
      void ImGui::Dummy(const ImVec2& size)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
   ItemSize(bb);
      }
      void ImGui::NewLine()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiLayoutType backup_layout_type = window->DC.LayoutType;
   window->DC.LayoutType = ImGuiLayoutType_Vertical;
   if (window->DC.CurrentLineSize.y > 0.0f)     // In the event that we are on a line with items that is smaller that FontSize high, we will preserve its height.
         else
            }
      void ImGui::AlignTextToFramePadding()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   window->DC.CurrentLineSize.y = ImMax(window->DC.CurrentLineSize.y, g.FontSize + g.Style.FramePadding.y * 2);
      }
      // Horizontal/vertical separating line
   void ImGui::Separator()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
                  // Those flags should eventually be overridable by the user
   ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
   IM_ASSERT(ImIsPowerOfTwo((int)(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical))));   // Check that only 1 option is selected
   if (flags & ImGuiSeparatorFlags_Vertical)
   {
      VerticalSeparator();
               // Horizontal Separator
   if (window->DC.ColumnsSet)
            float x1 = window->Pos.x;
   float x2 = window->Pos.x + window->Size.x;
   if (!window->DC.GroupStack.empty())
            const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y+1.0f));
   ItemSize(ImVec2(0.0f, 0.0f)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
   if (!ItemAdd(bb, 0))
   {
      if (window->DC.ColumnsSet)
                              if (g.LogEnabled)
            if (window->DC.ColumnsSet)
   {
      PushColumnClipRect();
         }
      void ImGui::VerticalSeparator()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
                  float y1 = window->DC.CursorPos.y;
   float y2 = window->DC.CursorPos.y + window->DC.CurrentLineSize.y;
   const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + 1.0f, y2));
   ItemSize(ImVec2(bb.GetWidth(), 0.0f));
   if (!ItemAdd(bb, 0))
            window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator));
   if (g.LogEnabled)
      }
      // Using 'hover_visibility_delay' allows us to hide the highlight and mouse cursor for a short time, which can be convenient to reduce visual noise.
   bool ImGui::SplitterBehavior(const ImRect& bb, ImGuiID id, ImGuiAxis axis, float* size1, float* size2, float min_size1, float min_size2, float hover_extend, float hover_visibility_delay)
   {
      ImGuiContext& g = *GImGui;
            const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
   window->DC.ItemFlags |= ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus;
   bool item_add = ItemAdd(bb, id);
   window->DC.ItemFlags = item_flags_backup;
   if (!item_add)
            bool hovered, held;
   ImRect bb_interact = bb;
   bb_interact.Expand(axis == ImGuiAxis_Y ? ImVec2(0.0f, hover_extend) : ImVec2(hover_extend, 0.0f));
   ButtonBehavior(bb_interact, id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowItemOverlap);
   if (g.ActiveId != id)
            if (held || (g.HoveredId == id && g.HoveredIdPreviousFrame == id && g.HoveredIdTimer >= hover_visibility_delay))
            ImRect bb_render = bb;
   if (held)
   {
      ImVec2 mouse_delta_2d = g.IO.MousePos - g.ActiveIdClickOffset - bb_interact.Min;
            // Minimum pane size
   float size_1_maximum_delta = ImMax(0.0f, *size1 - min_size1);
   float size_2_maximum_delta = ImMax(0.0f, *size2 - min_size2);
   if (mouse_delta < -size_1_maximum_delta)
         if (mouse_delta > size_2_maximum_delta)
            // Apply resize
   if (mouse_delta != 0.0f)
   {
         if (mouse_delta < 0.0f)
         if (mouse_delta > 0.0f)
         *size1 += mouse_delta;
   *size2 -= mouse_delta;
   bb_render.Translate((axis == ImGuiAxis_X) ? ImVec2(mouse_delta, 0.0f) : ImVec2(0.0f, mouse_delta));
               // Render
   const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : (hovered && g.HoveredIdTimer >= hover_visibility_delay) ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
               }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: ComboBox
   //-------------------------------------------------------------------------
   // - BeginCombo()
   // - EndCombo()
   // - Combo()
   //-------------------------------------------------------------------------
      static float CalcMaxPopupHeightFromItemCount(int items_count)
   {
      ImGuiContext& g = *GImGui;
   if (items_count <= 0)
            }
      bool ImGui::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
   {
      // Always consume the SetNextWindowSizeConstraint() call in our early return paths
   ImGuiContext& g = *GImGui;
   ImGuiCond backup_next_window_size_constraint = g.NextWindowData.SizeConstraintCond;
            ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
                     const ImGuiStyle& style = g.Style;
            const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
   const ImVec2 label_size = CalcTextSize(label, NULL, true);
   const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : CalcItemWidth();
   const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
   const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
   ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, id, &frame_bb))
            bool hovered, held;
   bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
            const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
   const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
   RenderNavHighlight(frame_bb, id);
   if (!(flags & ImGuiComboFlags_NoPreview))
         if (!(flags & ImGuiComboFlags_NoArrowButton))
   {
      window->DrawList->AddRectFilled(ImVec2(frame_bb.Max.x - arrow_size, frame_bb.Min.y), frame_bb.Max, GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button), style.FrameRounding, (w <= arrow_size) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Right);
      }
   RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
   if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
         if (label_size.x > 0)
            if ((pressed || g.NavActivateId == id) && !popup_open)
   {
      if (window->DC.NavLayerCurrent == 0)
         OpenPopupEx(id);
               if (!popup_open)
            if (backup_next_window_size_constraint)
   {
      g.NextWindowData.SizeConstraintCond = backup_next_window_size_constraint;
      }
   else
   {
      if ((flags & ImGuiComboFlags_HeightMask_) == 0)
         IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));    // Only one
   int popup_max_height_in_items = -1;
   if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
   else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
   else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
               char name[16];
            // Peak into expected window size so we can position it
   if (ImGuiWindow* popup_window = FindWindowByName(name))
      if (popup_window->WasActive)
   {
         ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
   if (flags & ImGuiComboFlags_PopupAlignLeft)
         ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
   ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
         // Horizontally align ourselves with the framed text
   ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
   PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
   bool ret = Begin(name, NULL, window_flags);
   PopStyleVar();
   if (!ret)
   {
      EndPopup();
   IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
      }
      }
      void ImGui::EndCombo()
   {
         }
      // Getter for the old Combo() API: const char*[]
   static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
   {
      const char* const* items = (const char* const*)data;
   if (out_text)
            }
      // Getter for the old Combo() API: "item1\0item2\0item3\0"
   static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
   {
      // FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
   const char* items_separated_by_zeros = (const char*)data;
   int items_count = 0;
   const char* p = items_separated_by_zeros;
   while (*p)
   {
      if (idx == items_count)
         p += strlen(p) + 1;
      }
   if (!*p)
         if (out_text)
            }
      // Old API, prefer using BeginCombo() nowadays if you can.
   bool ImGui::Combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
   {
               // Call the getter to obtain the preview string which is a parameter to BeginCombo()
   const char* preview_value = NULL;
   if (*current_item >= 0 && *current_item < items_count)
            // The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
   if (popup_max_height_in_items != -1 && !g.NextWindowData.SizeConstraintCond)
            if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
            // Display items
   // FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
   bool value_changed = false;
   for (int i = 0; i < items_count; i++)
   {
      PushID((void*)(intptr_t)i);
   const bool item_selected = (i == *current_item);
   const char* item_text;
   if (!items_getter(data, i, &item_text))
         if (Selectable(item_text, item_selected))
   {
         value_changed = true;
   }
   if (item_selected)
                     EndCombo();
      }
      // Combo box helper allowing to pass an array of strings.
   bool ImGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
   {
      const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
      }
      // Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
   bool ImGui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
   {
      int items_count = 0;
   const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
   while (*p)
   {
      p += strlen(p) + 1;
      }
   bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
      }
      //-------------------------------------------------------------------------
   // [SECTION] Data Type and Data Formatting Helpers [Internal]
   //-------------------------------------------------------------------------
   // - PatchFormatStringFloatToInt()
   // - DataTypeFormatString()
   // - DataTypeApplyOp()
   // - DataTypeApplyOpFromText()
   // - GetMinimumStepAtDecimalPrecision
   // - RoundScalarWithFormat<>()
   //-------------------------------------------------------------------------
      struct ImGuiDataTypeInfo
   {
      size_t      Size;
   const char* PrintFmt;   // Unused
      };
      static const ImGuiDataTypeInfo GDataTypeInfo[] =
   {
      { sizeof(int),          "%d",   "%d"    },
      #ifdef _MSC_VER
      { sizeof(ImS64),        "%I64d","%I64d" },
      #else
      { sizeof(ImS64),        "%lld", "%lld"  },
      #endif
      { sizeof(float),        "%f",   "%f"    },  // float are promoted to double in va_arg
      };
   IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo) == ImGuiDataType_COUNT);
      // FIXME-LEGACY: Prior to 1.61 our DragInt() function internally used floats and because of this the compile-time default value for format was "%.0f".
   // Even though we changed the compile-time default, we expect users to have carried %f around, which would break the display of DragInt() calls.
   // To honor backward compatibility we are rewriting the format string, unless IMGUI_DISABLE_OBSOLETE_FUNCTIONS is enabled. What could possibly go wrong?!
   static const char* PatchFormatStringFloatToInt(const char* fmt)
   {
      if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
         const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
   const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
   if (fmt_end > fmt_start && fmt_end[-1] == 'f')
      #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
         if (fmt_start == fmt && fmt_end[0] == 0)
         ImGuiContext& g = *GImGui;
   ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
   #else
         #endif
      }
      }
      static inline int DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* data_ptr, const char* format)
   {
      if (data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_U32)   // Signedness doesn't matter when pushing the argument
         if (data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)   // Signedness doesn't matter when pushing the argument
         if (data_type == ImGuiDataType_Float)
         if (data_type == ImGuiDataType_Double)
         IM_ASSERT(0);
      }
      // FIXME: Adding support for clamping on boundaries of the data type would be nice.
   static void DataTypeApplyOp(ImGuiDataType data_type, int op, void* output, void* arg1, const void* arg2)
   {
      IM_ASSERT(op == '+' || op == '-');
   switch (data_type)
   {
      case ImGuiDataType_S32:
         if (op == '+')      *(int*)output = *(const int*)arg1 + *(const int*)arg2;
   else if (op == '-') *(int*)output = *(const int*)arg1 - *(const int*)arg2;
   case ImGuiDataType_U32:
         if (op == '+')      *(unsigned int*)output = *(const unsigned int*)arg1 + *(const ImU32*)arg2;
   else if (op == '-') *(unsigned int*)output = *(const unsigned int*)arg1 - *(const ImU32*)arg2;
   case ImGuiDataType_S64:
         if (op == '+')      *(ImS64*)output = *(const ImS64*)arg1 + *(const ImS64*)arg2;
   else if (op == '-') *(ImS64*)output = *(const ImS64*)arg1 - *(const ImS64*)arg2;
   case ImGuiDataType_U64:
         if (op == '+')      *(ImU64*)output = *(const ImU64*)arg1 + *(const ImU64*)arg2;
   else if (op == '-') *(ImU64*)output = *(const ImU64*)arg1 - *(const ImU64*)arg2;
   case ImGuiDataType_Float:
         if (op == '+')      *(float*)output = *(const float*)arg1 + *(const float*)arg2;
   else if (op == '-') *(float*)output = *(const float*)arg1 - *(const float*)arg2;
   case ImGuiDataType_Double:
         if (op == '+')      *(double*)output = *(const double*)arg1 + *(const double*)arg2;
   else if (op == '-') *(double*)output = *(const double*)arg1 - *(const double*)arg2;
      }
      }
      // User can input math operators (e.g. +100) to edit a numerical values.
   // NB: This is _not_ a full expression evaluator. We should probably add one and replace this dumb mess..
   static bool DataTypeApplyOpFromText(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* format)
   {
      while (ImCharIsBlankA(*buf))
            // We don't support '-' op because it would conflict with inputing negative value.
   // Instead you can use +-100 to subtract from an existing value
   char op = buf[0];
   if (op == '+' || op == '*' || op == '/')
   {
      buf++;
   while (ImCharIsBlankA(*buf))
      }
   else
   {
         }
   if (!buf[0])
            // Copy the value in an opaque buffer so we can compare at the end of the function if it changed at all.
   IM_ASSERT(data_type < ImGuiDataType_COUNT);
   int data_backup[2];
   IM_ASSERT(GDataTypeInfo[data_type].Size <= sizeof(data_backup));
            if (format == NULL)
            int arg1i = 0;
   if (data_type == ImGuiDataType_S32)
   {
      int* v = (int*)data_ptr;
   int arg0i = *v;
   float arg1f = 0.0f;
   if (op && sscanf(initial_value_buf, format, &arg0i) < 1)
         // Store operand in a float so we can use fractional value for multipliers (*1.1), but constant always parsed as integer so we can fit big integers (e.g. 2000000003) past float precision
   if (op == '+')      { if (sscanf(buf, "%d", &arg1i)) *v = (int)(arg0i + arg1i); }                   // Add (use "+-" to subtract)
   else if (op == '*') { if (sscanf(buf, "%f", &arg1f)) *v = (int)(arg0i * arg1f); }                   // Multiply
   else if (op == '/') { if (sscanf(buf, "%f", &arg1f) && arg1f != 0.0f) *v = (int)(arg0i / arg1f); }  // Divide
      }
   else if (data_type == ImGuiDataType_U32 || data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)
   {
      // Assign constant
   // FIXME: We don't bother handling support for legacy operators since they are a little too crappy. Instead we may implement a proper expression evaluator in the future.
      }
   else if (data_type == ImGuiDataType_Float)
   {
      // For floats we have to ignore format with precision (e.g. "%.2f") because sscanf doesn't take them in
   format = "%f";
   float* v = (float*)data_ptr;
   float arg0f = *v, arg1f = 0.0f;
   if (op && sscanf(initial_value_buf, format, &arg0f) < 1)
         if (sscanf(buf, format, &arg1f) < 1)
         if (op == '+')      { *v = arg0f + arg1f; }                    // Add (use "+-" to subtract)
   else if (op == '*') { *v = arg0f * arg1f; }                    // Multiply
   else if (op == '/') { if (arg1f != 0.0f) *v = arg0f / arg1f; } // Divide
      }
   else if (data_type == ImGuiDataType_Double)
   {
      format = "%lf"; // scanf differentiate float/double unlike printf which forces everything to double because of ellipsis
   double* v = (double*)data_ptr;
   double arg0f = *v, arg1f = 0.0;
   if (op && sscanf(initial_value_buf, format, &arg0f) < 1)
         if (sscanf(buf, format, &arg1f) < 1)
         if (op == '+')      { *v = arg0f + arg1f; }                    // Add (use "+-" to subtract)
   else if (op == '*') { *v = arg0f * arg1f; }                    // Multiply
   else if (op == '/') { if (arg1f != 0.0f) *v = arg0f / arg1f; } // Divide
      }
      }
      static float GetMinimumStepAtDecimalPrecision(int decimal_precision)
   {
      static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
   if (decimal_precision < 0)
            }
      template<typename TYPE>
   static const char* ImAtoi(const char* src, TYPE* output)
   {
      int negative = 0;
   if (*src == '-') { negative = 1; src++; }
   if (*src == '+') { src++; }
   TYPE v = 0;
   while (*src >= '0' && *src <= '9')
         *output = negative ? -v : v;
      }
      template<typename TYPE, typename SIGNEDTYPE>
   TYPE ImGui::RoundScalarWithFormatT(const char* format, ImGuiDataType data_type, TYPE v)
   {
      const char* fmt_start = ImParseFormatFindStart(format);
   if (fmt_start[0] != '%' || fmt_start[1] == '%') // Don't apply if the value is not visible in the format string
         char v_str[64];
   ImFormatString(v_str, IM_ARRAYSIZE(v_str), fmt_start, v);
   const char* p = v_str;
   while (*p == ' ')
         if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
         else
            }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
   //-------------------------------------------------------------------------
   // - DragBehaviorT<>() [Internal]
   // - DragBehavior() [Internal]
   // - DragScalar()
   // - DragScalarN()
   // - DragFloat()
   // - DragFloat2()
   // - DragFloat3()
   // - DragFloat4()
   // - DragFloatRange2()
   // - DragInt()
   // - DragInt2()
   // - DragInt3()
   // - DragInt4()
   // - DragIntRange2()
   //-------------------------------------------------------------------------
      // This is called by DragBehavior() when the widget is active (held by mouse or being manipulated with Nav controls)
   template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
   bool ImGui::DragBehaviorT(ImGuiDataType data_type, TYPE* v, float v_speed, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiDragFlags flags)
   {
      ImGuiContext& g = *GImGui;
   const ImGuiAxis axis = (flags & ImGuiDragFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
   const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
   const bool has_min_max = (v_min != v_max);
            // Default tweak speed
   if (v_speed == 0.0f && has_min_max && (v_max - v_min < FLT_MAX))
            // Inputs accumulates into g.DragCurrentAccum, which is flushed into the current value as soon as it makes a difference with our precision settings
   float adjust_delta = 0.0f;
   if (g.ActiveIdSource == ImGuiInputSource_Mouse && IsMousePosValid() && g.IO.MouseDragMaxDistanceSqr[0] > 1.0f*1.0f)
   {
      adjust_delta = g.IO.MouseDelta[axis];
   if (g.IO.KeyAlt)
         if (g.IO.KeyShift)
      }
   else if (g.ActiveIdSource == ImGuiInputSource_Nav)
   {
      int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
   adjust_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 1.0f / 10.0f, 10.0f)[axis];
      }
            // For vertical drag we currently assume that Up=higher value (like we do with vertical sliders). This may become a parameter.
   if (axis == ImGuiAxis_Y)
            // Clear current value on activation
   // Avoid altering values and clamping when we are _already_ past the limits and heading in the same direction, so e.g. if range is 0..255, current value is 300 and we are pushing to the right side, keep the 300.
   bool is_just_activated = g.ActiveIdIsJustActivated;
   bool is_already_past_limits_and_pushing_outward = has_min_max && ((*v >= v_max && adjust_delta > 0.0f) || (*v <= v_min && adjust_delta < 0.0f));
   bool is_drag_direction_change_with_power = is_power && ((adjust_delta < 0 && g.DragCurrentAccum > 0) || (adjust_delta > 0 && g.DragCurrentAccum < 0));
   if (is_just_activated || is_already_past_limits_and_pushing_outward || is_drag_direction_change_with_power)
   {
      g.DragCurrentAccum = 0.0f;
      }
   else if (adjust_delta != 0.0f)
   {
      g.DragCurrentAccum += adjust_delta;
               if (!g.DragCurrentAccumDirty)
            TYPE v_cur = *v;
            if (is_power)
   {
      // Offset + round to user desired precision, with a curve on the v_min..v_max range to get more precision on one side of the range
   FLOATTYPE v_old_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
   FLOATTYPE v_new_norm_curved = v_old_norm_curved + (g.DragCurrentAccum / (v_max - v_min));
   v_cur = v_min + (TYPE)ImPow(ImSaturate((float)v_new_norm_curved), power) * (v_max - v_min);
      }
   else
   {
                  // Round to user desired precision based on format string
            // Preserve remainder after rounding has been applied. This also allow slow tweaking of values.
   g.DragCurrentAccumDirty = false;
   if (is_power)
   {
      FLOATTYPE v_cur_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
      }
   else
   {
                  // Lose zero sign for float/double
   if (v_cur == (TYPE)-0)
            // Clamp values (+ handle overflow/wrap-around for integer types)
   if (*v != v_cur && has_min_max)
   {
      if (v_cur < v_min || (v_cur > *v && adjust_delta < 0.0f && !is_decimal))
         if (v_cur > v_max || (v_cur < *v && adjust_delta > 0.0f && !is_decimal))
               // Apply result
   if (*v == v_cur)
         *v = v_cur;
      }
      bool ImGui::DragBehavior(ImGuiID id, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power, ImGuiDragFlags flags)
   {
      ImGuiContext& g = *GImGui;
   if (g.ActiveId == id)
   {
      if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
         else if (g.ActiveIdSource == ImGuiInputSource_Nav && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
      }
   if (g.ActiveId != id)
            switch (data_type)
   {
   case ImGuiDataType_S32:    return DragBehaviorT<ImS32, ImS32, float >(data_type, (ImS32*)v,  v_speed, v_min ? *(const ImS32* )v_min : IM_S32_MIN, v_max ? *(const ImS32* )v_max : IM_S32_MAX, format, power, flags);
   case ImGuiDataType_U32:    return DragBehaviorT<ImU32, ImS32, float >(data_type, (ImU32*)v,  v_speed, v_min ? *(const ImU32* )v_min : IM_U32_MIN, v_max ? *(const ImU32* )v_max : IM_U32_MAX, format, power, flags);
   case ImGuiDataType_S64:    return DragBehaviorT<ImS64, ImS64, double>(data_type, (ImS64*)v,  v_speed, v_min ? *(const ImS64* )v_min : IM_S64_MIN, v_max ? *(const ImS64* )v_max : IM_S64_MAX, format, power, flags);
   case ImGuiDataType_U64:    return DragBehaviorT<ImU64, ImS64, double>(data_type, (ImU64*)v,  v_speed, v_min ? *(const ImU64* )v_min : IM_U64_MIN, v_max ? *(const ImU64* )v_max : IM_U64_MAX, format, power, flags);
   case ImGuiDataType_Float:  return DragBehaviorT<float, float, float >(data_type, (float*)v,  v_speed, v_min ? *(const float* )v_min : -FLT_MAX,   v_max ? *(const float* )v_max : FLT_MAX,    format, power, flags);
   case ImGuiDataType_Double: return DragBehaviorT<double,double,double>(data_type, (double*)v, v_speed, v_min ? *(const double*)v_min : -DBL_MAX,   v_max ? *(const double*)v_max : DBL_MAX,    format, power, flags);
   case ImGuiDataType_COUNT:  break;
   }
   IM_ASSERT(0);
      }
      bool ImGui::DragScalar(const char* label, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            if (power != 1.0f)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const ImGuiID id = window->GetID(label);
            const ImVec2 label_size = CalcTextSize(label, NULL, true);
   const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
            ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, id, &frame_bb))
                     // Default format string when passing NULL
   // Patch old "%.0f" format string to use "%d", read function comments for more details.
   IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
   if (format == NULL)
         else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
            // Tabbing or CTRL-clicking on Drag turns it into an input box
   bool start_text_input = false;
   const bool tab_focus_requested = FocusableItemRegister(window, id);
   if (tab_focus_requested || (hovered && (g.IO.MouseClicked[0] || g.IO.MouseDoubleClicked[0])) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
   {
      SetActiveID(id, window);
   SetFocusID(id, window);
   FocusWindow(window);
   g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
   if (tab_focus_requested || g.IO.KeyCtrl || g.IO.MouseDoubleClicked[0] || g.NavInputId == id)
   {
         start_text_input = true;
      }
   if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
   {
      window->DC.CursorPos = frame_bb.Min;
   FocusableItemUnregister(window);
               // Actual drag behavior
   const bool value_changed = DragBehavior(id, data_type, v, v_speed, v_min, v_max, format, power, ImGuiDragFlags_None);
   if (value_changed)
            // Draw frame
   const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
   RenderNavHighlight(frame_bb, id);
            // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
   char value_buf[64];
   const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
            if (label_size.x > 0.0f)
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
      }
      bool ImGui::DragScalarN(const char* label, ImGuiDataType data_type, void* v, int components, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   bool value_changed = false;
   BeginGroup();
   PushID(label);
   PushMultiItemsWidths(components);
   size_t type_size = GDataTypeInfo[data_type].Size;
   for (int i = 0; i < components; i++)
   {
      PushID(i);
   value_changed |= DragScalar("", data_type, v, v_speed, v_min, v_max, format, power);
   SameLine(0, g.Style.ItemInnerSpacing.x);
   PopID();
   PopItemWidth();
      }
            TextUnformatted(label, FindRenderedTextEnd(label));
   EndGroup();
      }
      bool ImGui::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, float power)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   PushID(label);
   BeginGroup();
            bool value_changed = DragFloat("##min", v_current_min, v_speed, (v_min >= v_max) ? -FLT_MAX : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format, power);
   PopItemWidth();
   SameLine(0, g.Style.ItemInnerSpacing.x);
   value_changed |= DragFloat("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? FLT_MAX : v_max, format_max ? format_max : format, power);
   PopItemWidth();
            TextUnformatted(label, FindRenderedTextEnd(label));
   EndGroup();
   PopID();
      }
      // NB: v_speed is float to allow adjusting the drag speed with more precision
   bool ImGui::DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   PushID(label);
   BeginGroup();
            bool value_changed = DragInt("##min", v_current_min, v_speed, (v_min >= v_max) ? INT_MIN : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format);
   PopItemWidth();
   SameLine(0, g.Style.ItemInnerSpacing.x);
   value_changed |= DragInt("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? INT_MAX : v_max, format_max ? format_max : format);
   PopItemWidth();
            TextUnformatted(label, FindRenderedTextEnd(label));
   EndGroup();
               }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
   //-------------------------------------------------------------------------
   // - SliderBehaviorT<>() [Internal]
   // - SliderBehavior() [Internal]
   // - SliderScalar()
   // - SliderScalarN()
   // - SliderFloat()
   // - SliderFloat2()
   // - SliderFloat3()
   // - SliderFloat4()
   // - SliderAngle()
   // - SliderInt()
   // - SliderInt2()
   // - SliderInt3()
   // - SliderInt4()
   // - VSliderScalar()
   // - VSliderFloat()
   // - VSliderInt()
   //-------------------------------------------------------------------------
      template<typename TYPE, typename FLOATTYPE>
   float ImGui::SliderCalcRatioFromValueT(ImGuiDataType data_type, TYPE v, TYPE v_min, TYPE v_max, float power, float linear_zero_pos)
   {
      if (v_min == v_max)
            const bool is_power = (power != 1.0f) && (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double);
   const TYPE v_clamped = (v_min < v_max) ? ImClamp(v, v_min, v_max) : ImClamp(v, v_max, v_min);
   if (is_power)
   {
      if (v_clamped < 0.0f)
   {
         const float f = 1.0f - (float)((v_clamped - v_min) / (ImMin((TYPE)0, v_max) - v_min));
   }
   else
   {
         const float f = (float)((v_clamped - ImMax((TYPE)0, v_min)) / (v_max - ImMax((TYPE)0, v_min)));
               // Linear slider
      }
      // FIXME: Move some of the code into SliderBehavior(). Current responsability is larger than what the equivalent DragBehaviorT<> does, we also do some rendering, etc.
   template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
   bool ImGui::SliderBehaviorT(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
   {
      ImGuiContext& g = *GImGui;
            const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
   const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
            const float grab_padding = 2.0f;
   const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
   float grab_sz = style.GrabMinSize;
   SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
   if (!is_decimal && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
         grab_sz = ImMin(grab_sz, slider_sz);
   const float slider_usable_sz = slider_sz - grab_sz;
   const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz*0.5f;
            // For power curve sliders that cross over sign boundary we want the curve to be symmetric around 0.0f
   float linear_zero_pos;   // 0.0->1.0f
   if (is_power && v_min * v_max < 0.0f)
   {
      // Different sign
   const FLOATTYPE linear_dist_min_to_0 = ImPow(v_min >= 0 ? (FLOATTYPE)v_min : -(FLOATTYPE)v_min, (FLOATTYPE)1.0f/power);
   const FLOATTYPE linear_dist_max_to_0 = ImPow(v_max >= 0 ? (FLOATTYPE)v_max : -(FLOATTYPE)v_max, (FLOATTYPE)1.0f/power);
      }
   else
   {
      // Same sign
               // Process interacting with the slider
   bool value_changed = false;
   if (g.ActiveId == id)
   {
      bool set_new_value = false;
   float clicked_t = 0.0f;
   if (g.ActiveIdSource == ImGuiInputSource_Mouse)
   {
         if (!g.IO.MouseDown[0])
   {
         }
   else
   {
      const float mouse_abs_pos = g.IO.MousePos[axis];
   clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
   if (axis == ImGuiAxis_Y)
            }
   else if (g.ActiveIdSource == ImGuiInputSource_Nav)
   {
         const ImVec2 delta2 = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
   float delta = (axis == ImGuiAxis_X) ? delta2.x : -delta2.y;
   if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
   {
         }
   else if (delta != 0.0f)
   {
      clicked_t = SliderCalcRatioFromValueT<TYPE,FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
   const int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
   if ((decimal_precision > 0) || is_power)
   {
      delta /= 100.0f;    // Gamepad/keyboard tweak speeds in % of slider bounds
   if (IsNavInputDown(ImGuiNavInput_TweakSlow))
      }
   else
   {
      if ((v_range >= -100.0f && v_range <= 100.0f) || IsNavInputDown(ImGuiNavInput_TweakSlow))
         else
      }
   if (IsNavInputDown(ImGuiNavInput_TweakFast))
         set_new_value = true;
   if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
         else
               if (set_new_value)
   {
         TYPE v_new;
   if (is_power)
   {
      // Account for power curve scale on both sides of the zero
   if (clicked_t < linear_zero_pos)
   {
      // Negative: rescale to the negative range before powering
   float a = 1.0f - (clicked_t / linear_zero_pos);
   a = ImPow(a, power);
      }
   else
   {
      // Positive: rescale to the positive range before powering
   float a;
   if (ImFabs(linear_zero_pos - 1.0f) > 1.e-6f)
         else
         a = ImPow(a, power);
         }
   else
   {
      // Linear slider
   if (is_decimal)
   {
         }
   else
   {
      // For integer values we want the clicking position to match the grab box so we round above
   // This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
   FLOATTYPE v_new_off_f = (v_max - v_min) * clicked_t;
   TYPE v_new_off_floor = (TYPE)(v_new_off_f);
   TYPE v_new_off_round = (TYPE)(v_new_off_f + (FLOATTYPE)0.5);
   if (!is_decimal && v_new_off_floor < v_new_off_round)
         else
                                 // Apply result
   if (*v != v_new)
   {
      *v = v_new;
                  // Output grab position so it can be displayed by the caller
   float grab_t = SliderCalcRatioFromValueT<TYPE,FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
   if (axis == ImGuiAxis_Y)
         const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
   if (axis == ImGuiAxis_X)
         else
               }
      // For 32-bits and larger types, slider bounds are limited to half the natural type range.
   // So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
   // It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
   bool ImGui::SliderBehavior(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
   {
      switch (data_type)
   {
   case ImGuiDataType_S32:
      IM_ASSERT(*(const ImS32*)v_min >= IM_S32_MIN/2 && *(const ImS32*)v_max <= IM_S32_MAX/2);
      case ImGuiDataType_U32:
      IM_ASSERT(*(const ImU32*)v_min <= IM_U32_MAX/2);
      case ImGuiDataType_S64:
      IM_ASSERT(*(const ImS64*)v_min >= IM_S64_MIN/2 && *(const ImS64*)v_max <= IM_S64_MAX/2);
      case ImGuiDataType_U64:
      IM_ASSERT(*(const ImU64*)v_min <= IM_U64_MAX/2);
      case ImGuiDataType_Float:
      IM_ASSERT(*(const float*)v_min >= -FLT_MAX/2.0f && *(const float*)v_max <= FLT_MAX/2.0f);
      case ImGuiDataType_Double:
      IM_ASSERT(*(const double*)v_min >= -DBL_MAX/2.0f && *(const double*)v_max <= DBL_MAX/2.0f);
      case ImGuiDataType_COUNT: break;
   }
   IM_ASSERT(0);
      }
      bool ImGui::SliderScalar(const char* label, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const ImGuiID id = window->GetID(label);
            const ImVec2 label_size = CalcTextSize(label, NULL, true);
   const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
            ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, id, &frame_bb))
            // Default format string when passing NULL
   // Patch old "%.0f" format string to use "%d", read function comments for more details.
   IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
   if (format == NULL)
         else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
            // Tabbing or CTRL-clicking on Slider turns it into an input box
   bool start_text_input = false;
   const bool tab_focus_requested = FocusableItemRegister(window, id);
   const bool hovered = ItemHoverable(frame_bb, id);
   if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
   {
      SetActiveID(id, window);
   SetFocusID(id, window);
   FocusWindow(window);
   g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
   if (tab_focus_requested || g.IO.KeyCtrl || g.NavInputId == id)
   {
         start_text_input = true;
      }
   if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
   {
      window->DC.CursorPos = frame_bb.Min;
   FocusableItemUnregister(window);
               // Draw frame
   const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
   RenderNavHighlight(frame_bb, id);
            // Slider behavior
   ImRect grab_bb;
   const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_None, &grab_bb);
   if (value_changed)
            // Render grab
            // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
   char value_buf[64];
   const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
            if (label_size.x > 0.0f)
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
      }
      // Add multiple sliders on 1 line for compact edition of multiple components
   bool ImGui::SliderScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, float power)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   bool value_changed = false;
   BeginGroup();
   PushID(label);
   PushMultiItemsWidths(components);
   size_t type_size = GDataTypeInfo[data_type].Size;
   for (int i = 0; i < components; i++)
   {
      PushID(i);
   value_changed |= SliderScalar("", data_type, v, v_min, v_max, format, power);
   SameLine(0, g.Style.ItemInnerSpacing.x);
   PopID();
   PopItemWidth();
      }
            TextUnformatted(label, FindRenderedTextEnd(label));
   EndGroup();
      }
      bool ImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format)
   {
      if (format == NULL)
         float v_deg = (*v_rad) * 360.0f / (2*IM_PI);
   bool value_changed = SliderFloat(label, &v_deg, v_degrees_min, v_degrees_max, format, 1.0f);
   *v_rad = v_deg * (2*IM_PI) / 360.0f;
      }
      bool ImGui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
   {
         }
      bool ImGui::VSliderScalar(const char* label, const ImVec2& size, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
            const ImVec2 label_size = CalcTextSize(label, NULL, true);
   const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
            ItemSize(bb, style.FramePadding.y);
   if (!ItemAdd(frame_bb, id))
            // Default format string when passing NULL
   // Patch old "%.0f" format string to use "%d", read function comments for more details.
   IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
   if (format == NULL)
         else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
            const bool hovered = ItemHoverable(frame_bb, id);
   if ((hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || g.NavInputId == id)
   {
      SetActiveID(id, window);
   SetFocusID(id, window);
   FocusWindow(window);
               // Draw frame
   const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
   RenderNavHighlight(frame_bb, id);
            // Slider behavior
   ImRect grab_bb;
   const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_Vertical, &grab_bb);
   if (value_changed)
            // Render grab
            // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
   // For the vertical slider we allow centered text to overlap the frame padding
   char value_buf[64];
   const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
   RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f,0.0f));
   if (label_size.x > 0.0f)
               }
      bool ImGui::VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, float power)
   {
         }
      bool ImGui::VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format)
   {
         }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
   //-------------------------------------------------------------------------
   // - ImParseFormatFindStart() [Internal]
   // - ImParseFormatFindEnd() [Internal]
   // - ImParseFormatTrimDecorations() [Internal]
   // - ImParseFormatPrecision() [Internal]
   // - InputScalarAsWidgetReplacement() [Internal]
   // - InputScalar()
   // - InputScalarN()
   // - InputFloat()
   // - InputFloat2()
   // - InputFloat3()
   // - InputFloat4()
   // - InputInt()
   // - InputInt2()
   // - InputInt3()
   // - InputInt4()
   // - InputDouble()
   //-------------------------------------------------------------------------
      // We don't use strchr() because our strings are usually very short and often start with '%'
   const char* ImParseFormatFindStart(const char* fmt)
   {
      while (char c = fmt[0])
   {
      if (c == '%' && fmt[1] != '%')
         else if (c == '%')
            }
      }
      const char* ImParseFormatFindEnd(const char* fmt)
   {
      // Printf/scanf types modifiers: I/L/h/j/l/t/w/z. Other uppercase letters qualify as types aka end of the format.
   if (fmt[0] != '%')
         const unsigned int ignored_uppercase_mask = (1 << ('I'-'A')) | (1 << ('L'-'A'));
   const unsigned int ignored_lowercase_mask = (1 << ('h'-'a')) | (1 << ('j'-'a')) | (1 << ('l'-'a')) | (1 << ('t'-'a')) | (1 << ('w'-'a')) | (1 << ('z'-'a'));
   for (char c; (c = *fmt) != 0; fmt++)
   {
      if (c >= 'A' && c <= 'Z' && ((1 << (c - 'A')) & ignored_uppercase_mask) == 0)
         if (c >= 'a' && c <= 'z' && ((1 << (c - 'a')) & ignored_lowercase_mask) == 0)
      }
      }
      // Extract the format out of a format string with leading or trailing decorations
   //  fmt = "blah blah"  -> return fmt
   //  fmt = "%.3f"       -> return fmt
   //  fmt = "hello %.3f" -> return fmt + 6
   //  fmt = "%.3f hello" -> return buf written with "%.3f"
   const char* ImParseFormatTrimDecorations(const char* fmt, char* buf, size_t buf_size)
   {
      const char* fmt_start = ImParseFormatFindStart(fmt);
   if (fmt_start[0] != '%')
         const char* fmt_end = ImParseFormatFindEnd(fmt_start);
   if (fmt_end[0] == 0) // If we only have leading decoration, we don't need to copy the data.
         ImStrncpy(buf, fmt_start, ImMin((size_t)(fmt_end - fmt_start) + 1, buf_size));
      }
      // Parse display precision back from the display format string
   // FIXME: This is still used by some navigation code path to infer a minimum tweak step, but we should aim to rework widgets so it isn't needed.
   int ImParseFormatPrecision(const char* fmt, int default_precision)
   {
      fmt = ImParseFormatFindStart(fmt);
   if (fmt[0] != '%')
         fmt++;
   while (*fmt >= '0' && *fmt <= '9')
         int precision = INT_MAX;
   if (*fmt == '.')
   {
      fmt = ImAtoi<int>(fmt + 1, &precision);
   if (precision < 0 || precision > 99)
      }
   if (*fmt == 'e' || *fmt == 'E') // Maximum precision with scientific notation
         if ((*fmt == 'g' || *fmt == 'G') && precision == INT_MAX)
            }
      // Create text input in place of an active drag/slider (used when doing a CTRL+Click on drag/slider widgets)
   // FIXME: Facilitate using this in variety of other situations.
   bool ImGui::InputScalarAsWidgetReplacement(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* data_ptr, const char* format)
   {
               // On the first frame, g.ScalarAsInputTextId == 0, then on subsequent frames it becomes == id.
   // We clear ActiveID on the first frame to allow the InputText() taking it back.
   if (g.ScalarAsInputTextId == 0)
            char fmt_buf[32];
   char data_buf[32];
   format = ImParseFormatTrimDecorations(format, fmt_buf, IM_ARRAYSIZE(fmt_buf));
   DataTypeFormatString(data_buf, IM_ARRAYSIZE(data_buf), data_type, data_ptr, format);
   ImStrTrimBlanks(data_buf);
   ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ((data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double) ? ImGuiInputTextFlags_CharsScientific : ImGuiInputTextFlags_CharsDecimal);
   bool value_changed = InputTextEx(label, data_buf, IM_ARRAYSIZE(data_buf), bb.GetSize(), flags);
   if (g.ScalarAsInputTextId == 0)
   {
      // First frame we started displaying the InputText widget, we expect it to take the active id.
   IM_ASSERT(g.ActiveId == id);
      }
   if (value_changed)
            }
      bool ImGui::InputScalar(const char* label, ImGuiDataType data_type, void* data_ptr, const void* step, const void* step_fast, const char* format, ImGuiInputTextFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
            IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
   if (format == NULL)
            char buf[64];
            bool value_changed = false;
   if ((flags & (ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsScientific)) == 0)
                  if (step != NULL)
   {
               BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
   PushID(label);
   PushItemWidth(ImMax(1.0f, CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
   if (InputText("", buf, IM_ARRAYSIZE(buf), flags)) // PushId(label) + "" gives us the expected ID from outside point of view
                  // Step buttons
   ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
   if (flags & ImGuiInputTextFlags_ReadOnly)
         SameLine(0, style.ItemInnerSpacing.x);
   if (ButtonEx("-", ImVec2(button_size, button_size), button_flags))
   {
         DataTypeApplyOp(data_type, '-', data_ptr, data_ptr, g.IO.KeyCtrl && step_fast ? step_fast : step);
   }
   SameLine(0, style.ItemInnerSpacing.x);
   if (ButtonEx("+", ImVec2(button_size, button_size), button_flags))
   {
         DataTypeApplyOp(data_type, '+', data_ptr, data_ptr, g.IO.KeyCtrl && step_fast ? step_fast : step);
   }
   SameLine(0, style.ItemInnerSpacing.x);
            PopID();
      }
   else
   {
      if (InputText(label, buf, IM_ARRAYSIZE(buf), flags))
                  }
      bool ImGui::InputScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* step, const void* step_fast, const char* format, ImGuiInputTextFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   bool value_changed = false;
   BeginGroup();
   PushID(label);
   PushMultiItemsWidths(components);
   size_t type_size = GDataTypeInfo[data_type].Size;
   for (int i = 0; i < components; i++)
   {
      PushID(i);
   value_changed |= InputScalar("", data_type, v, step, step_fast, format, flags);
   SameLine(0, g.Style.ItemInnerSpacing.x);
   PopID();
   PopItemWidth();
      }
            TextUnformatted(label, FindRenderedTextEnd(label));
   EndGroup();
      }
      bool ImGui::InputFloat(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags)
   {
      flags |= ImGuiInputTextFlags_CharsScientific;
      }
      bool ImGui::InputFloat2(const char* label, float v[2], const char* format, ImGuiInputTextFlags flags)
   {
         }
      bool ImGui::InputFloat3(const char* label, float v[3], const char* format, ImGuiInputTextFlags flags)
   {
         }
      bool ImGui::InputFloat4(const char* label, float v[4], const char* format, ImGuiInputTextFlags flags)
   {
         }
      // Prefer using "const char* format" directly, which is more flexible and consistent with other API.
   #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
   bool ImGui::InputFloat(const char* label, float* v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags flags)
   {
      char format[16] = "%f";
   if (decimal_precision >= 0)
            }
      bool ImGui::InputFloat2(const char* label, float v[2], int decimal_precision, ImGuiInputTextFlags flags)
   {
      char format[16] = "%f";
   if (decimal_precision >= 0)
            }
      bool ImGui::InputFloat3(const char* label, float v[3], int decimal_precision, ImGuiInputTextFlags flags)
   {
      char format[16] = "%f";
   if (decimal_precision >= 0)
            }
      bool ImGui::InputFloat4(const char* label, float v[4], int decimal_precision, ImGuiInputTextFlags flags)
   {
      char format[16] = "%f";
   if (decimal_precision >= 0)
            }
   #endif // IMGUI_DISABLE_OBSOLETE_FUNCTIONS
      bool ImGui::InputInt(const char* label, int* v, int step, int step_fast, ImGuiInputTextFlags flags)
   {
      // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
   const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
      }
      bool ImGui::InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags)
   {
         }
      bool ImGui::InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags)
   {
         }
      bool ImGui::InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags)
   {
         }
      bool ImGui::InputDouble(const char* label, double* v, double step, double step_fast, const char* format, ImGuiInputTextFlags flags)
   {
      flags |= ImGuiInputTextFlags_CharsScientific;
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: InputText, InputTextMultiline
   //-------------------------------------------------------------------------
   // - InputText()
   // - InputTextMultiline()
   // - InputTextEx() [Internal]
   //-------------------------------------------------------------------------
      bool ImGui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
   {
      IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
      }
      bool ImGui::InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
   {
         }
      static int InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end)
   {
      int line_count = 0;
   const char* s = text_begin;
   while (char c = *s++) // We are only matching for \n so we can ignore UTF-8 decoding
      if (c == '\n')
      s--;
   if (s[0] != '\n' && s[0] != '\r')
         *out_text_end = s;
      }
      static ImVec2 InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining, ImVec2* out_offset, bool stop_on_new_line)
   {
      ImGuiContext& g = *GImGui;
   ImFont* font = g.Font;
   const float line_height = g.FontSize;
            ImVec2 text_size = ImVec2(0,0);
            const ImWchar* s = text_begin;
   while (s < text_end)
   {
      unsigned int c = (unsigned int)(*s++);
   if (c == '\n')
   {
         text_size.x = ImMax(text_size.x, line_width);
   text_size.y += line_height;
   line_width = 0.0f;
   if (stop_on_new_line)
         }
   if (c == '\r')
            const float char_width = font->GetCharAdvance((ImWchar)c) * scale;
               if (text_size.x < line_width)
            if (out_offset)
            if (line_width > 0 || text_size.y == 0.0f)                        // whereas size.y will ignore the trailing \n
            if (remaining)
               }
      // Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
   namespace ImGuiStb
   {
      static int     STB_TEXTEDIT_STRINGLEN(const STB_TEXTEDIT_STRING* obj)                             { return obj->CurLenW; }
   static ImWchar STB_TEXTEDIT_GETCHAR(const STB_TEXTEDIT_STRING* obj, int idx)                      { return obj->TextW[idx]; }
   static float   STB_TEXTEDIT_GETWIDTH(STB_TEXTEDIT_STRING* obj, int line_start_idx, int char_idx)  { ImWchar c = obj->TextW[line_start_idx+char_idx]; if (c == '\n') return STB_TEXTEDIT_GETWIDTH_NEWLINE; return GImGui->Font->GetCharAdvance(c) * (GImGui->FontSize / GImGui->Font->FontSize); }
   static int     STB_TEXTEDIT_KEYTOTEXT(int key)                                                    { return key >= 0x10000 ? 0 : key; }
   static ImWchar STB_TEXTEDIT_NEWLINE = '\n';
   static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, STB_TEXTEDIT_STRING* obj, int line_start_idx)
   {
      const ImWchar* text = obj->TextW.Data;
   const ImWchar* text_remaining = NULL;
   const ImVec2 size = InputTextCalcTextSizeW(text + line_start_idx, text + obj->CurLenW, &text_remaining, NULL, true);
   r->x0 = 0.0f;
   r->x1 = size.x;
   r->baseline_y_delta = size.y;
   r->ymin = 0.0f;
   r->ymax = size.y;
      }
      static bool is_separator(unsigned int c)                                        { return ImCharIsBlankW(c) || c==',' || c==';' || c=='(' || c==')' || c=='{' || c=='}' || c=='[' || c==']' || c=='|'; }
   static int  is_word_boundary_from_right(STB_TEXTEDIT_STRING* obj, int idx)      { return idx > 0 ? (is_separator( obj->TextW[idx-1] ) && !is_separator( obj->TextW[idx] ) ) : 1; }
   static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(STB_TEXTEDIT_STRING* obj, int idx)   { idx--; while (idx >= 0 && !is_word_boundary_from_right(obj, idx)) idx--; return idx < 0 ? 0 : idx; }
   #ifdef __APPLE__    // FIXME: Move setting to IO structure
   static int  is_word_boundary_from_left(STB_TEXTEDIT_STRING* obj, int idx)       { return idx > 0 ? (!is_separator( obj->TextW[idx-1] ) && is_separator( obj->TextW[idx] ) ) : 1; }
   static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx)  { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_left(obj, idx)) idx++; return idx > len ? len : idx; }
   #else
   static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx)  { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_right(obj, idx)) idx++; return idx > len ? len : idx; }
   #endif
   #define STB_TEXTEDIT_MOVEWORDLEFT   STB_TEXTEDIT_MOVEWORDLEFT_IMPL    // They need to be #define for stb_textedit.h
   #define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL
      static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)
   {
               // We maintain our buffer length in both UTF-8 and wchar formats
   obj->CurLenA -= ImTextCountUtf8BytesFromStr(dst, dst + n);
            // Offset remaining text (FIXME-OPT: Use memmove)
   const ImWchar* src = obj->TextW.Data + pos + n;
   while (ImWchar c = *src++)
            }
      static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const ImWchar* new_text, int new_text_len)
   {
      const bool is_resizable = (obj->UserFlags & ImGuiInputTextFlags_CallbackResize) != 0;
   const int text_len = obj->CurLenW;
            const int new_text_len_utf8 = ImTextCountUtf8BytesFromStr(new_text, new_text + new_text_len);
   if (!is_resizable && (new_text_len_utf8 + obj->CurLenA + 1 > obj->BufCapacityA))
            // Grow internal buffer if needed
   if (new_text_len + text_len + 1 > obj->TextW.Size)
   {
      if (!is_resizable)
         IM_ASSERT(text_len < obj->TextW.Size);
               ImWchar* text = obj->TextW.Data;
   if (pos != text_len)
                  obj->CurLenW += new_text_len;
   obj->CurLenA += new_text_len_utf8;
               }
      // We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
   #define STB_TEXTEDIT_K_LEFT         0x10000 // keyboard input to move cursor left
   #define STB_TEXTEDIT_K_RIGHT        0x10001 // keyboard input to move cursor right
   #define STB_TEXTEDIT_K_UP           0x10002 // keyboard input to move cursor up
   #define STB_TEXTEDIT_K_DOWN         0x10003 // keyboard input to move cursor down
   #define STB_TEXTEDIT_K_LINESTART    0x10004 // keyboard input to move cursor to start of line
   #define STB_TEXTEDIT_K_LINEEND      0x10005 // keyboard input to move cursor to end of line
   #define STB_TEXTEDIT_K_TEXTSTART    0x10006 // keyboard input to move cursor to start of text
   #define STB_TEXTEDIT_K_TEXTEND      0x10007 // keyboard input to move cursor to end of text
   #define STB_TEXTEDIT_K_DELETE       0x10008 // keyboard input to delete selection or character under cursor
   #define STB_TEXTEDIT_K_BACKSPACE    0x10009 // keyboard input to delete selection or character left of cursor
   #define STB_TEXTEDIT_K_UNDO         0x1000A // keyboard input to perform undo
   #define STB_TEXTEDIT_K_REDO         0x1000B // keyboard input to perform redo
   #define STB_TEXTEDIT_K_WORDLEFT     0x1000C // keyboard input to move cursor left one word
   #define STB_TEXTEDIT_K_WORDRIGHT    0x1000D // keyboard input to move cursor right one word
   #define STB_TEXTEDIT_K_SHIFT        0x20000
      #define STB_TEXTEDIT_IMPLEMENTATION
   #include "imstb_textedit.h"
      }
      void ImGuiInputTextState::OnKeyPressed(int key)
   {
      stb_textedit_key(this, &StbState, key);
   CursorFollow = true;
      }
      ImGuiInputTextCallbackData::ImGuiInputTextCallbackData()
   {
         }
      // Public API to manipulate UTF-8 text
   // We expose UTF-8 to the user (unlike the STB_TEXTEDIT_* functions which are manipulating wchar)
   // FIXME: The existence of this rarely exercised code path is a bit of a nuisance.
   void ImGuiInputTextCallbackData::DeleteChars(int pos, int bytes_count)
   {
      IM_ASSERT(pos + bytes_count <= BufTextLen);
   char* dst = Buf + pos;
   const char* src = Buf + pos + bytes_count;
   while (char c = *src++)
                  if (CursorPos + bytes_count >= pos)
         else if (CursorPos >= pos)
         SelectionStart = SelectionEnd = CursorPos;
   BufDirty = true;
      }
      void ImGuiInputTextCallbackData::InsertChars(int pos, const char* new_text, const char* new_text_end)
   {
      const bool is_resizable = (Flags & ImGuiInputTextFlags_CallbackResize) != 0;
   const int new_text_len = new_text_end ? (int)(new_text_end - new_text) : (int)strlen(new_text);
   if (new_text_len + BufTextLen >= BufSize)
   {
      if (!is_resizable)
            // Contrary to STB_TEXTEDIT_INSERTCHARS() this is working in the UTF8 buffer, hence the midly similar code (until we remove the U16 buffer alltogether!)
   ImGuiContext& g = *GImGui;
   ImGuiInputTextState* edit_state = &g.InputTextState;
   IM_ASSERT(edit_state->ID != 0 && g.ActiveId == edit_state->ID);
   IM_ASSERT(Buf == edit_state->TempBuffer.Data);
   int new_buf_size = BufTextLen + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1;
   edit_state->TempBuffer.reserve(new_buf_size + 1);
   Buf = edit_state->TempBuffer.Data;
               if (BufTextLen != pos)
         memcpy(Buf + pos, new_text, (size_t)new_text_len * sizeof(char));
            if (CursorPos >= pos)
         SelectionStart = SelectionEnd = CursorPos;
   BufDirty = true;
      }
      // Return false to discard a character.
   static bool InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
   {
               if (c < 128 && c != ' ' && !isprint((int)(c & 0xFF)))
   {
      bool pass = false;
   pass |= (c == '\n' && (flags & ImGuiInputTextFlags_Multiline));
   pass |= (c == '\t' && (flags & ImGuiInputTextFlags_AllowTabInput));
   if (!pass)
               if (c >= 0xE000 && c <= 0xF8FF) // Filter private Unicode range. I don't imagine anybody would want to input them. GLFW on OSX seems to send private characters for special keys like arrow keys.
            if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsScientific))
   {
      if (flags & ImGuiInputTextFlags_CharsDecimal)
                  if (flags & ImGuiInputTextFlags_CharsScientific)
                  if (flags & ImGuiInputTextFlags_CharsHexadecimal)
                  if (flags & ImGuiInputTextFlags_CharsUppercase)
                  if (flags & ImGuiInputTextFlags_CharsNoBlank)
                     if (flags & ImGuiInputTextFlags_CallbackCharFilter)
   {
      ImGuiInputTextCallbackData callback_data;
   memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
   callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter;
   callback_data.EventChar = (ImWchar)c;
   callback_data.Flags = flags;
   callback_data.UserData = user_data;
   if (callback(&callback_data) != 0)
         *p_char = callback_data.EventChar;
   if (!callback_data.EventChar)
                  }
      // Edit a string of text
   // - buf_size account for the zero-terminator, so a buf_size of 6 can hold "Hello" but not "Hello!".
   //   This is so we can easily call InputText() on static arrays using ARRAYSIZE() and to match
   //   Note that in std::string world, capacity() would omit 1 byte used by the zero-terminator.
   // - When active, hold on a privately held copy of the text (and apply back to 'buf'). So changing 'buf' while the InputText is active has no effect.
   // - If you want to use ImGui::InputText() with std::string, see misc/cpp/imgui_stdlib.h
   // (FIXME: Rather messy function partly because we are doing UTF8 > u16 > UTF8 conversions on the go to more easily handle stb_textedit calls. Ideally we should stay in UTF-8 all the time. See https://github.com/nothings/stb/issues/188)
   bool ImGui::InputTextEx(const char* label, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
            ImGuiContext& g = *GImGui;
   ImGuiIO& io = g.IO;
            const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
   const bool is_editable = (flags & ImGuiInputTextFlags_ReadOnly) == 0;
   const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
   const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
   const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
   if (is_resizable)
            if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope,
         const ImGuiID id = window->GetID(label);
   const ImVec2 label_size = CalcTextSize(label, NULL, true);
   ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), (is_multiline ? GetTextLineHeight() * 8.0f : label_size.y) + style.FramePadding.y*2.0f); // Arbitrary default of 8 lines high for multi-line
   const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
            ImGuiWindow* draw_window = window;
   if (is_multiline)
   {
      if (!ItemAdd(total_bb, id, &frame_bb))
   {
         ItemSize(total_bb, style.FramePadding.y);
   EndGroup();
   }
   if (!BeginChildFrame(id, frame_bb.GetSize()))
   {
         EndChildFrame();
   EndGroup();
   }
   draw_window = GetCurrentWindow();
   draw_window->DC.NavLayerActiveMaskNext |= draw_window->DC.NavLayerCurrentMask; // This is to ensure that EndChild() will display a navigation highlight
      }
   else
   {
      ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, id, &frame_bb))
      }
   const bool hovered = ItemHoverable(frame_bb, id);
   if (hovered)
            // Password pushes a temporary font with only a fallback glyph
   if (is_password)
   {
      const ImFontGlyph* glyph = g.Font->FindGlyph('*');
   ImFont* password_font = &g.InputTextPasswordFont;
   password_font->FontSize = g.Font->FontSize;
   password_font->Scale = g.Font->Scale;
   password_font->DisplayOffset = g.Font->DisplayOffset;
   password_font->Ascent = g.Font->Ascent;
   password_font->Descent = g.Font->Descent;
   password_font->ContainerAtlas = g.Font->ContainerAtlas;
   password_font->FallbackGlyph = glyph;
   password_font->FallbackAdvanceX = glyph->AdvanceX;
   IM_ASSERT(password_font->Glyphs.empty() && password_font->IndexAdvanceX.empty() && password_font->IndexLookup.empty());
               // NB: we are only allowed to access 'edit_state' if we are the active widget.
            const bool focus_requested = FocusableItemRegister(window, id, (flags & (ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_AllowTabInput)) == 0);    // Using completion callback disable keyboard tabbing
   const bool focus_requested_by_code = focus_requested && (window->FocusIdxAllCounter == window->FocusIdxAllRequestCurrent);
            const bool user_clicked = hovered && io.MouseClicked[0];
   const bool user_scrolled = is_multiline && g.ActiveId == 0 && edit_state.ID == id && g.ActiveIdPreviousFrame == draw_window->GetIDNoKeepAlive("#SCROLLY");
                     bool select_all = (g.ActiveId != id) && ((flags & ImGuiInputTextFlags_AutoSelectAll) != 0 || user_nav_input_start) && (!is_multiline);
   if (focus_requested || user_clicked || user_scrolled || user_nav_input_start)
   {
      if (g.ActiveId != id)
   {
         // Start edition
   // Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
   // From the moment we focused we are ignoring the content of 'buf' (unless we are in read-only mode)
   const int prev_len_w = edit_state.CurLenW;
   const int init_buf_len = (int)strlen(buf);
   edit_state.TextW.resize(buf_size+1);             // wchar count <= UTF-8 count. we use +1 to make sure that .Data isn't NULL so it doesn't crash.
   edit_state.InitialText.resize(init_buf_len + 1); // UTF-8. we use +1 to make sure that .Data isn't NULL so it doesn't crash.
   memcpy(edit_state.InitialText.Data, buf, init_buf_len + 1);
   const char* buf_end = NULL;
   edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, buf_size, buf, NULL, &buf_end);
                  // Preserve cursor position and undo/redo stack if we come back to same widget
   // FIXME: We should probably compare the whole buffer to be on the safety side. Comparing buf (utf8) and edit_state.Text (wchar).
   const bool recycle_state = (edit_state.ID == id) && (prev_len_w == edit_state.CurLenW);
   if (recycle_state)
   {
      // Recycle existing cursor/selection/undo stack but clamp position
   // Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
      }
   else
   {
      edit_state.ID = id;
   edit_state.ScrollX = 0.0f;
   stb_textedit_initialize_state(&edit_state.StbState, !is_multiline);
   if (!is_multiline && focus_requested_by_code)
      }
   if (flags & ImGuiInputTextFlags_AlwaysInsertMode)
         if (!is_multiline && (focus_requested_by_tab || (user_clicked && io.KeyCtrl)))
   }
   SetActiveID(id, window);
   SetFocusID(id, window);
   FocusWindow(window);
   g.ActiveIdBlockNavInputFlags = (1 << ImGuiNavInput_Cancel);
   if (!is_multiline && !(flags & ImGuiInputTextFlags_CallbackHistory))
      }
   else if (io.MouseClicked[0])
   {
      // Release focus when we click outside
               bool value_changed = false;
   bool enter_pressed = false;
            if (g.ActiveId == id)
   {
      if (!is_editable && !g.ActiveIdIsJustActivated)
   {
         // When read-only we always use the live data passed to the function
   edit_state.TextW.resize(buf_size+1);
   const char* buf_end = NULL;
   edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, edit_state.TextW.Size, buf, NULL, &buf_end);
   edit_state.CurLenA = (int)(buf_end - buf);
            backup_current_text_length = edit_state.CurLenA;
   edit_state.BufCapacityA = buf_size;
   edit_state.UserFlags = flags;
   edit_state.UserCallback = callback;
            // Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
   // Down the line we should have a cleaner library-wide concept of Selected vs Active.
   g.ActiveIdAllowOverlap = !io.MouseDown[0];
            // Edit in progress
   const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + edit_state.ScrollX;
            const bool is_osx = io.ConfigMacOSXBehaviors;
   if (select_all || (hovered && !is_osx && io.MouseDoubleClicked[0]))
   {
         edit_state.SelectAll();
   }
   else if (hovered && is_osx && io.MouseDoubleClicked[0])
   {
         // Double-click select a word only, OS X style (by simulating keystrokes)
   edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
   }
   else if (io.MouseClicked[0] && !edit_state.SelectedAllMouseLock)
   {
         if (hovered)
   {
      stb_textedit_click(&edit_state, &edit_state.StbState, mouse_x, mouse_y);
      }
   else if (io.MouseDown[0] && !edit_state.SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
   {
         stb_textedit_drag(&edit_state, &edit_state.StbState, mouse_x, mouse_y);
   edit_state.CursorAnimReset();
   }
   if (edit_state.SelectedAllMouseLock && !io.MouseDown[0])
            if (io.InputQueueCharacters.Size > 0)
   {
         // Process text input (before we check for Return because using some IME will effectively send a Return?)
   // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
   bool ignore_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeySuper);
   if (!ignore_inputs && is_editable && !user_nav_input_start)
      for (int n = 0; n < io.InputQueueCharacters.Size; n++)
   {
      // Insert character if they pass filtering
   unsigned int c = (unsigned int)io.InputQueueCharacters[n];
                  // Consume characters
               bool cancel_edit = false;
   if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
   {
      // Handle key-presses
   const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
   const bool is_osx = io.ConfigMacOSXBehaviors;
   const bool is_shortcut_key = (is_osx ? (io.KeySuper && !io.KeyCtrl) : (io.KeyCtrl && !io.KeySuper)) && !io.KeyAlt && !io.KeyShift; // OS X style: Shortcuts using Cmd/Super instead of Ctrl
   const bool is_osx_shift_shortcut = is_osx && io.KeySuper && io.KeyShift && !io.KeyCtrl && !io.KeyAlt;
   const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
   const bool is_startend_key_down = is_osx && io.KeySuper && !io.KeyCtrl && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
   const bool is_ctrl_key_only = io.KeyCtrl && !io.KeyShift && !io.KeyAlt && !io.KeySuper;
            const bool is_cut   = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_X)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Delete))) && is_editable && !is_password && (!is_multiline || edit_state.HasSelection());
   const bool is_copy  = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_C)) || (is_ctrl_key_only  && IsKeyPressedMap(ImGuiKey_Insert))) && !is_password && (!is_multiline || edit_state.HasSelection());
   const bool is_paste = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_V)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Insert))) && is_editable;
   const bool is_undo  = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Z)) && is_editable && is_undoable);
            if (IsKeyPressedMap(ImGuiKey_LeftArrow))                        { edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_RightArrow))                  { edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_UpArrow) && is_multiline)     { if (io.KeyCtrl) SetWindowScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_DownArrow) && is_multiline)   { if (io.KeyCtrl) SetWindowScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_Home))                        { edit_state.OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_End))                         { edit_state.OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_Delete) && is_editable)       { edit_state.OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask); }
   else if (IsKeyPressedMap(ImGuiKey_Backspace) && is_editable)
   {
         if (!edit_state.HasSelection())
   {
      if (is_wordmove_key_down) edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT|STB_TEXTEDIT_K_SHIFT);
      }
   }
   else if (IsKeyPressedMap(ImGuiKey_Enter))
   {
         bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
   if (!is_multiline || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
   {
         }
   else if (is_editable)
   {
      unsigned int c = '\n'; // Insert new line
   if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
      }
   else if ((flags & ImGuiInputTextFlags_AllowTabInput) && IsKeyPressedMap(ImGuiKey_Tab) && !io.KeyCtrl && !io.KeyShift && !io.KeyAlt && is_editable)
   {
         unsigned int c = '\t'; // Insert TAB
   if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
   }
   else if (IsKeyPressedMap(ImGuiKey_Escape))
   {
         }
   else if (is_undo || is_redo)
   {
         edit_state.OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
   }
   else if (is_shortcut_key && IsKeyPressedMap(ImGuiKey_A))
   {
         edit_state.SelectAll();
   }
   else if (is_cut || is_copy)
   {
         // Cut, Copy
   if (io.SetClipboardTextFn)
   {
      const int ib = edit_state.HasSelection() ? ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end) : 0;
   const int ie = edit_state.HasSelection() ? ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end) : edit_state.CurLenW;
   edit_state.TempBuffer.resize((ie-ib) * 4 + 1);
   ImTextStrToUtf8(edit_state.TempBuffer.Data, edit_state.TempBuffer.Size, edit_state.TextW.Data+ib, edit_state.TextW.Data+ie);
      }
   if (is_cut)
   {
      if (!edit_state.HasSelection())
         edit_state.CursorFollow = true;
      }
   else if (is_paste)
   {
         if (const char* clipboard = GetClipboardText())
   {
      // Filter pasted buffer
   const int clipboard_len = (int)strlen(clipboard);
   ImWchar* clipboard_filtered = (ImWchar*)MemAlloc((clipboard_len+1) * sizeof(ImWchar));
   int clipboard_filtered_len = 0;
   for (const char* s = clipboard; *s; )
   {
      unsigned int c;
   s += ImTextCharFromUtf8(&c, s, NULL);
   if (c == 0)
         if (c >= 0x10000 || !InputTextFilterCharacter(&c, flags, callback, callback_user_data))
            }
   clipboard_filtered[clipboard_filtered_len] = 0;
   if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
   {
      stb_textedit_paste(&edit_state, &edit_state.StbState, clipboard_filtered, clipboard_filtered_len);
      }
                  if (g.ActiveId == id)
   {
      const char* apply_new_text = NULL;
   int apply_new_text_length = 0;
   if (cancel_edit)
   {
         // Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
   if (is_editable && strcmp(buf, edit_state.InitialText.Data) != 0)
   {
      apply_new_text = edit_state.InitialText.Data;
               // When using 'ImGuiInputTextFlags_EnterReturnsTrue' as a special case we reapply the live buffer back to the input buffer before clearing ActiveId, even though strictly speaking it wasn't modified on this frame.
   // If we didn't do that, code like InputInt() with ImGuiInputTextFlags_EnterReturnsTrue would fail. Also this allows the user to use InputText() with ImGuiInputTextFlags_EnterReturnsTrue without maintaining any user-side storage.
   bool apply_edit_back_to_user_buffer = !cancel_edit || (enter_pressed && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
   if (apply_edit_back_to_user_buffer)
   {
         // Apply new value immediately - copy modified buffer back
   // Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
   // FIXME: We actually always render 'buf' when calling DrawList->AddText, making the comment above incorrect.
   // FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks.
   if (is_editable)
   {
                        // User callback
   if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways)) != 0)
                     // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
   ImGuiInputTextFlags event_flag = 0;
   ImGuiKey event_key = ImGuiKey_COUNT;
   if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressedMap(ImGuiKey_Tab))
   {
      event_flag = ImGuiInputTextFlags_CallbackCompletion;
      }
   else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_UpArrow))
   {
      event_flag = ImGuiInputTextFlags_CallbackHistory;
      }
   else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_DownArrow))
   {
      event_flag = ImGuiInputTextFlags_CallbackHistory;
                           if (event_flag)
   {
      ImGuiInputTextCallbackData callback_data;
   memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
                        callback_data.EventKey = event_key;
   callback_data.Buf = edit_state.TempBuffer.Data;
                        // We have to convert from wchar-positions to UTF-8-positions, which can be pretty slow (an incentive to ditch the ImWchar buffer, see https://github.com/nothings/stb/issues/188)
   ImWchar* text = edit_state.TextW.Data;
                                       // Read back what user may have modified
   IM_ASSERT(callback_data.Buf == edit_state.TempBuffer.Data);  // Invalid to modify those fields
   IM_ASSERT(callback_data.BufSize == edit_state.BufCapacityA);
   IM_ASSERT(callback_data.Flags == flags);
   if (callback_data.CursorPos != utf8_cursor_pos)            { edit_state.StbState.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos); edit_state.CursorFollow = true; }
   if (callback_data.SelectionStart != utf8_selection_start)  { edit_state.StbState.select_start = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart); }
   if (callback_data.SelectionEnd != utf8_selection_end)      { edit_state.StbState.select_end = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd); }
   if (callback_data.BufDirty)
   {
         IM_ASSERT(callback_data.BufTextLen == (int)strlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
   if (callback_data.BufTextLen > backup_current_text_length && is_resizable)
         edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, edit_state.TextW.Size, callback_data.Buf, NULL);
   edit_state.CurLenA = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
                  // Will copy result string if modified
   if (is_editable && strcmp(edit_state.TempBuffer.Data, buf) != 0)
   {
      apply_new_text = edit_state.TempBuffer.Data;
               // Copy result to user buffer
   if (apply_new_text)
   {
         IM_ASSERT(apply_new_text_length >= 0);
   if (backup_current_text_length != apply_new_text_length && is_resizable)
   {
      ImGuiInputTextCallbackData callback_data;
   callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
   callback_data.Flags = flags;
   callback_data.Buf = buf;
   callback_data.BufTextLen = apply_new_text_length;
   callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
   callback_data.UserData = callback_user_data;
   callback(&callback_data);
   buf = callback_data.Buf;
   buf_size = callback_data.BufSize;
                     // If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
   ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
            // Clear temporary user storage
   edit_state.UserFlags = 0;
   edit_state.UserCallback = NULL;
               // Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
   if (clear_active_id && g.ActiveId == id)
            // Render
   // Select which buffer we are going to display. When ImGuiInputTextFlags_NoLiveEdit is set 'buf' might still be the old value. We set buf to NULL to prevent accidental usage from now on.
            // Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
   // without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
   // Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
            if (!is_multiline)
   {
      RenderNavHighlight(frame_bb, id);
               const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
   ImVec2 render_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
   ImVec2 text_size(0.f, 0.f);
   const bool is_currently_scrolling = (edit_state.ID == id && is_multiline && g.ActiveId == draw_window->GetIDNoKeepAlive("#SCROLLY"));
   if (g.ActiveId == id || is_currently_scrolling)
   {
               // This is going to be messy. We need to:
   // - Display the text (this alone can be more easily clipped)
   // - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
   // - Measure text height (for scrollbar)
   // We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
   // FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
   const ImWchar* text_begin = edit_state.TextW.Data;
            {
         // Count lines + find lines numbers straddling 'cursor' and 'select_start' position.
   const ImWchar* searches_input_ptr[2];
   searches_input_ptr[0] = text_begin + edit_state.StbState.cursor;
   searches_input_ptr[1] = NULL;
   int searches_remaining = 1;
   int searches_result_line_number[2] = { -1, -999 };
   if (edit_state.StbState.select_start != edit_state.StbState.select_end)
   {
      searches_input_ptr[1] = text_begin + ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end);
                     // Iterate all lines to find our line numbers
   // In multi-line mode, we never exit the loop until all lines are counted, so add one extra to the searches_remaining counter.
   searches_remaining += is_multiline ? 1 : 0;
   int line_count = 0;
   //for (const ImWchar* s = text_begin; (s = (const ImWchar*)wcschr((const wchar_t*)s, (wchar_t)'\n')) != NULL; s++)  // FIXME-OPT: Could use this when wchar_t are 16-bits
   for (const ImWchar* s = text_begin; *s != 0; s++)
      if (*s == '\n')
   {
      line_count++;
   if (searches_result_line_number[0] == -1 && s >= searches_input_ptr[0]) { searches_result_line_number[0] = line_count; if (--searches_remaining <= 0) break; }
         line_count++;
                  // Calculate 2d position by finding the beginning of the line and measuring distance
   cursor_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[0], text_begin), searches_input_ptr[0]).x;
   cursor_offset.y = searches_result_line_number[0] * g.FontSize;
   if (searches_result_line_number[1] >= 0)
   {
                        // Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
   if (is_multiline)
            // Scroll
   if (edit_state.CursorFollow)
   {
         // Horizontal scroll in chunks of quarter width
   if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
   {
      const float scroll_increment_x = size.x * 0.25f;
   if (cursor_offset.x < edit_state.ScrollX)
         else if (cursor_offset.x - size.x >= edit_state.ScrollX)
      }
   else
   {
                  // Vertical scroll
   if (is_multiline)
   {
      float scroll_y = draw_window->Scroll.y;
   if (cursor_offset.y - g.FontSize < scroll_y)
         else if (cursor_offset.y - size.y >= scroll_y)
         draw_window->DC.CursorPos.y += (draw_window->Scroll.y - scroll_y);   // To avoid a frame of lag
   draw_window->Scroll.y = scroll_y;
      }
   edit_state.CursorFollow = false;
            // Draw selection
   if (edit_state.StbState.select_start != edit_state.StbState.select_end)
   {
                        float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
   float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
   ImU32 bg_color = GetColorU32(ImGuiCol_TextSelectedBg);
   ImVec2 rect_pos = render_pos + select_start_offset - render_scroll;
   for (const ImWchar* p = text_selected_begin; p < text_selected_end; )
   {
      if (rect_pos.y > clip_rect.w + g.FontSize)
         if (rect_pos.y < clip_rect.y)
   {
      //p = (const ImWchar*)wmemchr((const wchar_t*)p, '\n', text_selected_end - p);  // FIXME-OPT: Could use this when wchar_t are 16-bits
   //p = p ? p + 1 : text_selected_end;
   while (p < text_selected_end)
            }
   else
   {
      ImVec2 rect_size = InputTextCalcTextSizeW(p, text_selected_end, &p, NULL, true);
   if (rect_size.x <= 0.0f) rect_size.x = (float)(int)(g.Font->GetCharAdvance((ImWchar)' ') * 0.50f); // So we can see selected empty lines
   ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos +ImVec2(rect_size.x, bg_offy_dn));
   rect.ClipWith(clip_rect);
   if (rect.Overlaps(clip_rect))
      }
   rect_pos.x = render_pos.x - render_scroll.x;
               const int buf_display_len = edit_state.CurLenA;
   if (is_multiline || buf_display_len < buf_display_max_length)
            // Draw blinking cursor
   bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (g.InputTextState.CursorAnim <= 0.0f) || ImFmod(g.InputTextState.CursorAnim, 1.20f) <= 0.80f;
   ImVec2 cursor_screen_pos = render_pos + cursor_offset - render_scroll;
   ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y-g.FontSize+0.5f, cursor_screen_pos.x+1.0f, cursor_screen_pos.y-1.5f);
   if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
            // Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
   if (is_editable)
      }
   else
   {
      // Render text only
   const char* buf_end = NULL;
   if (is_multiline)
         else
         if (is_multiline || (buf_end - buf_display) < buf_display_max_length)
               if (is_multiline)
   {
      Dummy(text_size + ImVec2(0.0f, g.FontSize)); // Always add room to scroll an extra line
   EndChildFrame();
               if (is_password)
            // Log as text
   if (g.LogEnabled && !is_password)
            if (label_size.x > 0)
            if (value_changed)
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
   if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
         else
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
   //-------------------------------------------------------------------------
   // - ColorEdit3()
   // - ColorEdit4()
   // - ColorPicker3()
   // - RenderColorRectWithAlphaCheckerboard() [Internal]
   // - ColorPicker4()
   // - ColorButton()
   // - SetColorEditOptions()
   // - ColorTooltip() [Internal]
   // - ColorEditOptionsPopup() [Internal]
   // - ColorPickerOptionsPopup() [Internal]
   //-------------------------------------------------------------------------
      bool ImGui::ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags)
   {
         }
      // Edit colors components (each component in 0.0f..1.0f range).
   // See enum ImGuiColorEditFlags_ for available options. e.g. Only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
   // With typical options: Left-click on colored square to open color picker. Right-click to open option menu. CTRL-Click over input fields to edit them and TAB to go to next item.
   bool ImGui::ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const float square_sz = GetFrameHeight();
   const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
   const float w_items_all = CalcItemWidth() - w_extra;
            BeginGroup();
            // If we're not showing any slider there's no point in doing any HSV conversions
   const ImGuiColorEditFlags flags_untouched = flags;
   if (flags & ImGuiColorEditFlags_NoInputs)
            // Context menu: display and modify options (before defaults are applied)
   if (!(flags & ImGuiColorEditFlags_NoOptions))
            // Read stored options
   if (!(flags & ImGuiColorEditFlags__InputsMask))
         if (!(flags & ImGuiColorEditFlags__DataTypeMask))
         if (!(flags & ImGuiColorEditFlags__PickerMask))
                  const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
   const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
            // Convert to the formats we need
   float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
   if (flags & ImGuiColorEditFlags_HSV)
                  bool value_changed = false;
            if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
   {
      // RGB/HSV 0..255 Sliders
   const float w_item_one  = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components-1)) / (float)components));
            const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
   const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
   const char* fmt_table_int[3][4] =
   {
         {   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
   { "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
   };
   const char* fmt_table_float[3][4] =
   {
         {   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
   { "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
   };
            PushItemWidth(w_item_one);
   for (int n = 0; n < components; n++)
   {
         if (n > 0)
         if (n + 1 == components)
         if (flags & ImGuiColorEditFlags_Float)
   {
      value_changed |= DragFloat(ids[n], &f[n], 1.0f/255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
      }
   else
   {
         }
   if (!(flags & ImGuiColorEditFlags_NoOptions))
   }
   PopItemWidth();
      }
   else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
   {
      // RGB Hexadecimal Input
   char buf[64];
   if (alpha)
         else
         PushItemWidth(w_items_all);
   if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
   {
         value_changed = true;
   char* p = buf;
   while (*p == '#' || ImCharIsBlankA(*p))
         i[0] = i[1] = i[2] = i[3] = 0;
   if (alpha)
         else
   }
   if (!(flags & ImGuiColorEditFlags_NoOptions))
                     ImGuiWindow* picker_active_window = NULL;
   if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
   {
      if (!(flags & ImGuiColorEditFlags_NoInputs))
            const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
   if (ColorButton("##ColorButton", col_v4, flags))
   {
         if (!(flags & ImGuiColorEditFlags_NoPicker))
   {
      // Store current color and open a picker
   g.ColorPickerRef = col_v4;
   OpenPopup("picker");
      }
   if (!(flags & ImGuiColorEditFlags_NoOptions))
            if (BeginPopup("picker"))
   {
         picker_active_window = g.CurrentWindow;
   if (label != label_display_end)
   {
      TextUnformatted(label, label_display_end);
      }
   ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
   ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
   PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
   value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
   PopItemWidth();
               if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
   {
      SameLine(0, style.ItemInnerSpacing.x);
               // Convert back
   if (picker_active_window == NULL)
   {
      if (!value_changed_as_float)
         for (int n = 0; n < 4; n++)
   if (flags & ImGuiColorEditFlags_HSV)
         if (value_changed)
   {
         col[0] = f[0];
   col[1] = f[1];
   col[2] = f[2];
   if (alpha)
               PopID();
            // Drag and Drop Target
   // NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
   if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
   {
      if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
   {
         memcpy((float*)col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
   }
   if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
   {
         memcpy((float*)col, payload->Data, sizeof(float) * components);
   }
               // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
   if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
            if (value_changed)
               }
      bool ImGui::ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags)
   {
      float col4[4] = { col[0], col[1], col[2], 1.0f };
   if (!ColorPicker4(label, col4, flags | ImGuiColorEditFlags_NoAlpha))
         col[0] = col4[0]; col[1] = col4[1]; col[2] = col4[2];
      }
      static inline ImU32 ImAlphaBlendColor(ImU32 col_a, ImU32 col_b)
   {
      float t = ((col_b >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
   int r = ImLerp((int)(col_a >> IM_COL32_R_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_R_SHIFT) & 0xFF, t);
   int g = ImLerp((int)(col_a >> IM_COL32_G_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_G_SHIFT) & 0xFF, t);
   int b = ImLerp((int)(col_a >> IM_COL32_B_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_B_SHIFT) & 0xFF, t);
      }
      // Helper for ColorPicker4()
   // NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
   // I spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding alltogether.
   void ImGui::RenderColorRectWithAlphaCheckerboard(ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, int rounding_corners_flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
   {
      ImU32 col_bg1 = GetColorU32(ImAlphaBlendColor(IM_COL32(204,204,204,255), col));
   ImU32 col_bg2 = GetColorU32(ImAlphaBlendColor(IM_COL32(128,128,128,255), col));
            int yi = 0;
   for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
   {
         float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
   if (y2 <= y1)
         for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
   {
      float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
   if (x2 <= x1)
         int rounding_corners_flags_cell = 0;
   if (y1 <= p_min.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopRight; }
   if (y2 >= p_max.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotRight; }
   rounding_corners_flags_cell &= rounding_corners_flags;
         }
   else
   {
            }
      // Helper for ColorPicker4()
   static void RenderArrowsForVerticalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w)
   {
      ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x + 1,         pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32_BLACK);
   ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x,             pos.y), half_sz,                              ImGuiDir_Right, IM_COL32_WHITE);
   ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x - 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left,  IM_COL32_BLACK);
      }
      // Note: ColorPicker4() only accesses 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
   // FIXME: we adjust the big color square height based on item width, which may cause a flickering feedback loop (if automatic height makes a vertical scrollbar appears, affecting automatic width..)
   bool ImGui::ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
   {
      ImGuiContext& g = *GImGui;
   ImGuiWindow* window = GetCurrentWindow();
            ImGuiStyle& style = g.Style;
            PushID(label);
            if (!(flags & ImGuiColorEditFlags_NoSidePreview))
            // Context menu: display and store options.
   if (!(flags & ImGuiColorEditFlags_NoOptions))
            // Read stored options
   if (!(flags & ImGuiColorEditFlags__PickerMask))
         IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__PickerMask))); // Check that only 1 is selected
   if (!(flags & ImGuiColorEditFlags_NoOptions))
            // Setup
   int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
   bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
   ImVec2 picker_pos = window->DC.CursorPos;
   float square_sz = GetFrameHeight();
   float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
   float sv_picker_size = ImMax(bars_width * 1, CalcItemWidth() - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
   float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
   float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
            float backup_initial_col[4];
            float wheel_thickness = sv_picker_size * 0.08f;
   float wheel_r_outer = sv_picker_size * 0.50f;
   float wheel_r_inner = wheel_r_outer - wheel_thickness;
            // Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
   float triangle_r = wheel_r_inner - (int)(sv_picker_size * 0.027f);
   ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f); // Hue point.
   ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
            float H,S,V;
                     PushItemFlag(ImGuiItemFlags_NoNav, true);
   if (flags & ImGuiColorEditFlags_PickerHueWheel)
   {
      // Hue wheel + SV triangle logic
   InvisibleButton("hsv", ImVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size));
   if (IsItemActive())
   {
         ImVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
   ImVec2 current_off = g.IO.MousePos - wheel_center;
   float initial_dist2 = ImLengthSqr(initial_off);
   if (initial_dist2 >= (wheel_r_inner-1)*(wheel_r_inner-1) && initial_dist2 <= (wheel_r_outer+1)*(wheel_r_outer+1))
   {
      // Interactive with Hue wheel
   H = ImAtan2(current_off.y, current_off.x) / IM_PI*0.5f;
   if (H < 0.0f)
            }
   float cos_hue_angle = ImCos(-H * 2.0f * IM_PI);
   float sin_hue_angle = ImSin(-H * 2.0f * IM_PI);
   if (ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, ImRotate(initial_off, cos_hue_angle, sin_hue_angle)))
   {
      // Interacting with SV triangle
   ImVec2 current_off_unrotated = ImRotate(current_off, cos_hue_angle, sin_hue_angle);
   if (!ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
         float uu, vv, ww;
   ImTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
   V = ImClamp(1.0f - vv, 0.0001f, 1.0f);
   S = ImClamp(uu / V, 0.0001f, 1.0f);
      }
   if (!(flags & ImGuiColorEditFlags_NoOptions))
      }
   else if (flags & ImGuiColorEditFlags_PickerHueBar)
   {
      // SV rectangle logic
   InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
   if (IsItemActive())
   {
         S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size-1));
   V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size-1));
   }
   if (!(flags & ImGuiColorEditFlags_NoOptions))
            // Hue bar logic
   SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
   InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
   if (IsItemActive())
   {
         H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size-1));
               // Alpha bar logic
   if (alpha_bar)
   {
      SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
   InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
   if (IsItemActive())
   {
         col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size-1));
      }
            if (!(flags & ImGuiColorEditFlags_NoSidePreview))
   {
      SameLine(0, style.ItemInnerSpacing.x);
               if (!(flags & ImGuiColorEditFlags_NoLabel))
   {
      const char* label_display_end = FindRenderedTextEnd(label);
   if (label != label_display_end)
   {
         if ((flags & ImGuiColorEditFlags_NoSidePreview))
                     if (!(flags & ImGuiColorEditFlags_NoSidePreview))
   {
      PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
   ImVec4 col_v4(col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
   if ((flags & ImGuiColorEditFlags_NoLabel))
         ColorButton("##current", col_v4, (flags & (ImGuiColorEditFlags_HDR|ImGuiColorEditFlags_AlphaPreview|ImGuiColorEditFlags_AlphaPreviewHalf|ImGuiColorEditFlags_NoTooltip)), ImVec2(square_sz * 3, square_sz * 2));
   if (ref_col != NULL)
   {
         Text("Original");
   ImVec4 ref_col_v4(ref_col[0], ref_col[1], ref_col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : ref_col[3]);
   if (ColorButton("##original", ref_col_v4, (flags & (ImGuiColorEditFlags_HDR|ImGuiColorEditFlags_AlphaPreview|ImGuiColorEditFlags_AlphaPreviewHalf|ImGuiColorEditFlags_NoTooltip)), ImVec2(square_sz * 3, square_sz * 2)))
   {
      memcpy(col, ref_col, components * sizeof(float));
      }
   PopItemFlag();
               // Convert back color to RGB
   if (value_changed_h || value_changed_sv)
            // R,G,B and H,S,V slider color editor
   bool value_changed_fix_hue_wrap = false;
   if ((flags & ImGuiColorEditFlags_NoInputs) == 0)
   {
      PushItemWidth((alpha_bar ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
   ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf;
   ImGuiColorEditFlags sub_flags = (flags & sub_flags_to_forward) | ImGuiColorEditFlags_NoPicker;
   if (flags & ImGuiColorEditFlags_RGB || (flags & ImGuiColorEditFlags__InputsMask) == 0)
         if (ColorEdit4("##rgb", col, sub_flags | ImGuiColorEditFlags_RGB))
   {
      // FIXME: Hackily differenciating using the DragInt (ActiveId != 0 && !ActiveIdAllowOverlap) vs. using the InputText or DropTarget.
   // For the later we don't want to run the hue-wrap canceling code. If you are well versed in HSV picker please provide your input! (See #2050)
   value_changed_fix_hue_wrap = (g.ActiveId != 0 && !g.ActiveIdAllowOverlap);
      if (flags & ImGuiColorEditFlags_HSV || (flags & ImGuiColorEditFlags__InputsMask) == 0)
         if (flags & ImGuiColorEditFlags_HEX || (flags & ImGuiColorEditFlags__InputsMask) == 0)
                     // Try to cancel hue wrap (after ColorEdit4 call), if any
   if (value_changed_fix_hue_wrap)
   {
      float new_H, new_S, new_V;
   ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
   if (new_H <= 0 && H > 0)
   {
         if (new_V <= 0 && V != new_V)
         else if (new_S <= 0)
               ImVec4 hue_color_f(1, 1, 1, 1); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
   ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
            const ImU32 hue_colors[6+1] = { IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255), IM_COL32(0,255,255,255), IM_COL32(0,0,255,255), IM_COL32(255,0,255,255), IM_COL32(255,0,0,255) };
            if (flags & ImGuiColorEditFlags_PickerHueWheel)
   {
      // Render Hue Wheel
   const float aeps = 1.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
   const int segment_per_arc = ImMax(4, (int)wheel_r_outer / 12);
   for (int n = 0; n < 6; n++)
   {
         const float a0 = (n)     /6.0f * 2.0f * IM_PI - aeps;
   const float a1 = (n+1.0f)/6.0f * 2.0f * IM_PI + aeps;
   const int vert_start_idx = draw_list->VtxBuffer.Size;
   draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer)*0.5f, a0, a1, segment_per_arc);
                  // Paint colors over existing vertices
   ImVec2 gradient_p0(wheel_center.x + ImCos(a0) * wheel_r_inner, wheel_center.y + ImSin(a0) * wheel_r_inner);
   ImVec2 gradient_p1(wheel_center.x + ImCos(a1) * wheel_r_inner, wheel_center.y + ImSin(a1) * wheel_r_inner);
            // Render Cursor + preview on Hue Wheel
   float cos_hue_angle = ImCos(H * 2.0f * IM_PI);
   float sin_hue_angle = ImSin(H * 2.0f * IM_PI);
   ImVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner+wheel_r_outer)*0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner+wheel_r_outer)*0.5f);
   float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
   int hue_cursor_segments = ImClamp((int)(hue_cursor_rad / 1.4f), 9, 32);
   draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
   draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad+1, IM_COL32(128,128,128,255), hue_cursor_segments);
            // Render SV triangle (rotated according to hue)
   ImVec2 tra = wheel_center + ImRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
   ImVec2 trb = wheel_center + ImRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
   ImVec2 trc = wheel_center + ImRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
   ImVec2 uv_white = GetFontTexUvWhitePixel();
   draw_list->PrimReserve(6, 6);
   draw_list->PrimVtx(tra, uv_white, hue_color32);
   draw_list->PrimVtx(trb, uv_white, hue_color32);
   draw_list->PrimVtx(trc, uv_white, IM_COL32_WHITE);
   draw_list->PrimVtx(tra, uv_white, IM_COL32_BLACK_TRANS);
   draw_list->PrimVtx(trb, uv_white, IM_COL32_BLACK);
   draw_list->PrimVtx(trc, uv_white, IM_COL32_BLACK_TRANS);
   draw_list->AddTriangle(tra, trb, trc, IM_COL32(128,128,128,255), 1.5f);
      }
   else if (flags & ImGuiColorEditFlags_PickerHueBar)
   {
      // Render SV Square
   draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size,sv_picker_size), IM_COL32_WHITE, hue_color32, hue_color32, IM_COL32_WHITE);
   draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size,sv_picker_size), IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, IM_COL32_BLACK, IM_COL32_BLACK);
   RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size,sv_picker_size), 0.0f);
   sv_cursor_pos.x = ImClamp((float)(int)(picker_pos.x + ImSaturate(S)     * sv_picker_size + 0.5f), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
            // Render Hue Bar
   for (int i = 0; i < 6; ++i)
         float bar0_line_y = (float)(int)(picker_pos.y + H * sv_picker_size + 0.5f);
   RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
               // Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
   float sv_cursor_rad = value_changed_sv ? 10.0f : 6.0f;
   draw_list->AddCircleFilled(sv_cursor_pos, sv_cursor_rad, col32_no_alpha, 12);
   draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad+1, IM_COL32(128,128,128,255), 12);
            // Render alpha bar
   if (alpha_bar)
   {
      float alpha = ImSaturate(col[3]);
   ImRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
   RenderColorRectWithAlphaCheckerboard(bar1_bb.Min, bar1_bb.Max, IM_COL32(0,0,0,0), bar1_bb.GetWidth() / 2.0f, ImVec2(0.0f, 0.0f));
   draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, col32_no_alpha, col32_no_alpha, col32_no_alpha & ~IM_COL32_A_MASK, col32_no_alpha & ~IM_COL32_A_MASK);
   float bar1_line_y = (float)(int)(picker_pos.y + (1.0f - alpha) * sv_picker_size + 0.5f);
   RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
                        if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
         if (value_changed)
                        }
      // A little colored square. Return true when clicked.
   // FIXME: May want to display/ignore the alpha component in the color display? Yet show it in the tooltip.
   // 'desc_id' is not called 'label' because we don't display it next to the button, but only in the tooltip.
   bool ImGui::ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags, ImVec2 size)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiID id = window->GetID(desc_id);
   float default_size = GetFrameHeight();
   if (size.x == 0.0f)
         if (size.y == 0.0f)
         const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
   ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
   if (!ItemAdd(bb, id))
            bool hovered, held;
            if (flags & ImGuiColorEditFlags_NoAlpha)
            ImVec4 col_without_alpha(col.x, col.y, col.z, 1.0f);
   float grid_step = ImMin(size.x, size.y) / 2.99f;
   float rounding = ImMin(g.Style.FrameRounding, grid_step * 0.5f);
   ImRect bb_inner = bb;
   float off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
   bb_inner.Expand(off);
   if ((flags & ImGuiColorEditFlags_AlphaPreviewHalf) && col.w < 1.0f)
   {
      float mid_x = (float)(int)((bb_inner.Min.x + bb_inner.Max.x) * 0.5f + 0.5f);
   RenderColorRectWithAlphaCheckerboard(ImVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col), grid_step, ImVec2(-grid_step + off, off), rounding, ImDrawCornerFlags_TopRight| ImDrawCornerFlags_BotRight);
      }
   else
   {
      // Because GetColorU32() multiplies by the global style Alpha and we don't want to display a checkerboard if the source code had no alpha
   ImVec4 col_source = (flags & ImGuiColorEditFlags_AlphaPreview) ? col : col_without_alpha;
   if (col_source.w < 1.0f)
         else
      }
   RenderNavHighlight(bb, id);
   if (g.Style.FrameBorderSize > 0.0f)
         else
            // Drag and Drop Source
   // NB: The ActiveId test is merely an optional micro-optimization, BeginDragDropSource() does the same test.
   if (g.ActiveId == id && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropSource())
   {
      if (flags & ImGuiColorEditFlags_NoAlpha)
         else
         ColorButton(desc_id, col, flags);
   SameLine();
   TextUnformatted("Color");
               // Tooltip
   if (!(flags & ImGuiColorEditFlags_NoTooltip) && hovered)
            if (pressed)
               }
      void ImGui::SetColorEditOptions(ImGuiColorEditFlags flags)
   {
      ImGuiContext& g = *GImGui;
   if ((flags & ImGuiColorEditFlags__InputsMask) == 0)
         if ((flags & ImGuiColorEditFlags__DataTypeMask) == 0)
         if ((flags & ImGuiColorEditFlags__PickerMask) == 0)
         IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__InputsMask)));   // Check only 1 option is selected
   IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__DataTypeMask))); // Check only 1 option is selected
   IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__PickerMask)));   // Check only 1 option is selected
      }
      // Note: only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
   void ImGui::ColorTooltip(const char* text, const float* col, ImGuiColorEditFlags flags)
   {
               int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = (flags & ImGuiColorEditFlags_NoAlpha) ? 255 : IM_F32_TO_INT8_SAT(col[3]);
            const char* text_end = text ? FindRenderedTextEnd(text, NULL) : text;
   if (text_end > text)
   {
      TextUnformatted(text, text_end);
               ImVec2 sz(g.FontSize * 3 + g.Style.FramePadding.y * 2, g.FontSize * 3 + g.Style.FramePadding.y * 2);
   ColorButton("##preview", ImVec4(col[0], col[1], col[2], col[3]), (flags & (ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf)) | ImGuiColorEditFlags_NoTooltip, sz);
   SameLine();
   if (flags & ImGuiColorEditFlags_NoAlpha)
         else
            }
      void ImGui::ColorEditOptionsPopup(const float* col, ImGuiColorEditFlags flags)
   {
      bool allow_opt_inputs = !(flags & ImGuiColorEditFlags__InputsMask);
   bool allow_opt_datatype = !(flags & ImGuiColorEditFlags__DataTypeMask);
   if ((!allow_opt_inputs && !allow_opt_datatype) || !BeginPopup("context"))
         ImGuiContext& g = *GImGui;
   ImGuiColorEditFlags opts = g.ColorEditOptions;
   if (allow_opt_inputs)
   {
      if (RadioButton("RGB", (opts & ImGuiColorEditFlags_RGB) != 0)) opts = (opts & ~ImGuiColorEditFlags__InputsMask) | ImGuiColorEditFlags_RGB;
   if (RadioButton("HSV", (opts & ImGuiColorEditFlags_HSV) != 0)) opts = (opts & ~ImGuiColorEditFlags__InputsMask) | ImGuiColorEditFlags_HSV;
      }
   if (allow_opt_datatype)
   {
      if (allow_opt_inputs) Separator();
   if (RadioButton("0..255",     (opts & ImGuiColorEditFlags_Uint8) != 0)) opts = (opts & ~ImGuiColorEditFlags__DataTypeMask) | ImGuiColorEditFlags_Uint8;
               if (allow_opt_inputs || allow_opt_datatype)
         if (Button("Copy as..", ImVec2(-1,0)))
         if (BeginPopup("Copy"))
   {
      int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = (flags & ImGuiColorEditFlags_NoAlpha) ? 255 : IM_F32_TO_INT8_SAT(col[3]);
   char buf[64];
   ImFormatString(buf, IM_ARRAYSIZE(buf), "(%.3ff, %.3ff, %.3ff, %.3ff)", col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
   if (Selectable(buf))
         ImFormatString(buf, IM_ARRAYSIZE(buf), "(%d,%d,%d,%d)", cr, cg, cb, ca);
   if (Selectable(buf))
         if (flags & ImGuiColorEditFlags_NoAlpha)
         else
         if (Selectable(buf))
                     g.ColorEditOptions = opts;
      }
      void ImGui::ColorPickerOptionsPopup(const float* ref_col, ImGuiColorEditFlags flags)
   {
      bool allow_opt_picker = !(flags & ImGuiColorEditFlags__PickerMask);
   bool allow_opt_alpha_bar = !(flags & ImGuiColorEditFlags_NoAlpha) && !(flags & ImGuiColorEditFlags_AlphaBar);
   if ((!allow_opt_picker && !allow_opt_alpha_bar) || !BeginPopup("context"))
         ImGuiContext& g = *GImGui;
   if (allow_opt_picker)
   {
      ImVec2 picker_size(g.FontSize * 8, ImMax(g.FontSize * 8 - (GetFrameHeight() + g.Style.ItemInnerSpacing.x), 1.0f)); // FIXME: Picker size copied from main picker function
   PushItemWidth(picker_size.x);
   for (int picker_type = 0; picker_type < 2; picker_type++)
   {
         // Draw small/thumbnail version of each picker type (over an invisible button for selection)
   if (picker_type > 0) Separator();
   PushID(picker_type);
   ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoOptions|ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_NoSidePreview|(flags & ImGuiColorEditFlags_NoAlpha);
   if (picker_type == 0) picker_flags |= ImGuiColorEditFlags_PickerHueBar;
   if (picker_type == 1) picker_flags |= ImGuiColorEditFlags_PickerHueWheel;
   ImVec2 backup_pos = GetCursorScreenPos();
   if (Selectable("##selectable", false, 0, picker_size)) // By default, Selectable() is closing popup
         SetCursorScreenPos(backup_pos);
   ImVec4 dummy_ref_col;
   memcpy(&dummy_ref_col, ref_col, sizeof(float) * ((picker_flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4));
   ColorPicker4("##dummypicker", &dummy_ref_col.x, picker_flags);
   }
      }
   if (allow_opt_alpha_bar)
   {
      if (allow_opt_picker) Separator();
      }
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
   //-------------------------------------------------------------------------
   // - TreeNode()
   // - TreeNodeV()
   // - TreeNodeEx()
   // - TreeNodeExV()
   // - TreeNodeBehavior() [Internal]
   // - TreePush()
   // - TreePop()
   // - TreeAdvanceToLabelPos()
   // - GetTreeNodeToLabelSpacing()
   // - SetNextTreeNodeOpen()
   // - CollapsingHeader()
   //-------------------------------------------------------------------------
      bool ImGui::TreeNode(const char* str_id, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   bool is_open = TreeNodeExV(str_id, 0, fmt, args);
   va_end(args);
      }
      bool ImGui::TreeNode(const void* ptr_id, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   bool is_open = TreeNodeExV(ptr_id, 0, fmt, args);
   va_end(args);
      }
      bool ImGui::TreeNode(const char* label)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            }
      bool ImGui::TreeNodeV(const char* str_id, const char* fmt, va_list args)
   {
         }
      bool ImGui::TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
   {
         }
      bool ImGui::TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
               }
      bool ImGui::TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   bool is_open = TreeNodeExV(str_id, flags, fmt, args);
   va_end(args);
      }
      bool ImGui::TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   bool is_open = TreeNodeExV(ptr_id, flags, fmt, args);
   va_end(args);
      }
      bool ImGui::TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
      }
      bool ImGui::TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
      }
      bool ImGui::TreeNodeBehaviorIsOpen(ImGuiID id, ImGuiTreeNodeFlags flags)
   {
      if (flags & ImGuiTreeNodeFlags_Leaf)
            // We only write to the tree storage if the user clicks (or explicitly use SetNextTreeNode*** functions)
   ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
            bool is_open;
   if (g.NextTreeNodeOpenCond != 0)
   {
      if (g.NextTreeNodeOpenCond & ImGuiCond_Always)
   {
         is_open = g.NextTreeNodeOpenVal;
   }
   else
   {
         // We treat ImGuiCond_Once and ImGuiCond_FirstUseEver the same because tree node state are not saved persistently.
   const int stored_value = storage->GetInt(id, -1);
   if (stored_value == -1)
   {
      is_open = g.NextTreeNodeOpenVal;
      }
   else
   {
         }
      }
   else
   {
                  // When logging is enabled, we automatically expand tree nodes (but *NOT* collapsing headers.. seems like sensible behavior).
   // NB- If we are above max depth we still allow manually opened nodes to be logged.
   if (g.LogEnabled && !(flags & ImGuiTreeNodeFlags_NoAutoOpenOnLog) && window->DC.TreeDepth < g.LogAutoExpandMaxDepth)
               }
      bool ImGui::TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
   const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
            if (!label_end)
                  // We vertically grow up to current line height up the typical widget height.
   const float text_base_offset_y = ImMax(padding.y, window->DC.CurrentLineTextBaseOffset); // Latch before ItemSize changes it
   const float frame_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + style.FramePadding.y*2), label_size.y + padding.y*2);
   ImRect frame_bb = ImRect(window->DC.CursorPos, ImVec2(window->Pos.x + GetContentRegionMax().x, window->DC.CursorPos.y + frame_height));
   if (display_frame)
   {
      // Framed header expand a little outside the default padding
   frame_bb.Min.x -= (float)(int)(window->WindowPadding.x*0.5f) - 1;
               const float text_offset_x = (g.FontSize + (display_frame ? padding.x*3 : padding.x*2));   // Collapser arrow width + Spacing
   const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x*2 : 0.0f);   // Include collapser
            // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
   // (Ideally we'd want to add a flag for the user to specify if we want the hit test to be done up to the right side of the content or not)
   const ImRect interact_bb = display_frame ? frame_bb : ImRect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + text_width + style.ItemSpacing.x*2, frame_bb.Max.y);
   bool is_open = TreeNodeBehaviorIsOpen(id, flags);
            // Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
   // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
   // This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
   if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            bool item_add = ItemAdd(interact_bb, id);
   window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
            if (!item_add)
   {
      if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
         IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
               // Flags that affects opening behavior:
   // - 0 (default) .................... single-click anywhere to open
   // - OpenOnDoubleClick .............. double-click anywhere to open
   // - OpenOnArrow .................... single-click on arrow to open
   // - OpenOnDoubleClick|OpenOnArrow .. single-click on arrow or double-click anywhere to open
   ImGuiButtonFlags button_flags = ImGuiButtonFlags_NoKeyModifiers;
   if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
         if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
         if (!is_leaf)
            bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
   bool hovered, held;
   bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
   bool toggled = false;
   if (!is_leaf)
   {
      if (pressed)
   {
         toggled = !(flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) || (g.NavActivateId == id);
   if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
         if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
         if (g.DragDropActive && is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
            if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
   {
         toggled = true;
   }
   if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
   {
         toggled = true;
            if (toggled)
   {
         is_open = !is_open;
      }
   if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
            // Render
   const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
   const ImVec2 text_pos = frame_bb.Min + ImVec2(text_offset_x, text_base_offset_y);
   ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
   if (display_frame)
   {
      // Framed type
   RenderFrame(frame_bb.Min, frame_bb.Max, col, true, style.FrameRounding);
   RenderNavHighlight(frame_bb, id, nav_highlight_flags);
   RenderArrow(frame_bb.Min + ImVec2(padding.x, text_base_offset_y), is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
   if (g.LogEnabled)
   {
         // NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
   const char log_prefix[] = "\n##";
   const char log_suffix[] = "##";
   LogRenderedText(&text_pos, log_prefix, log_prefix+3);
   RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
   }
   else
   {
            }
   else
   {
      // Unframed typed for tree nodes
   if (hovered || selected)
   {
         RenderFrame(frame_bb.Min, frame_bb.Max, col, false);
            if (flags & ImGuiTreeNodeFlags_Bullet)
         else if (!is_leaf)
         if (g.LogEnabled)
                     if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
         IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
      }
      void ImGui::TreePush(const char* str_id)
   {
      ImGuiWindow* window = GetCurrentWindow();
   Indent();
   window->DC.TreeDepth++;
      }
      void ImGui::TreePush(const void* ptr_id)
   {
      ImGuiWindow* window = GetCurrentWindow();
   Indent();
   window->DC.TreeDepth++;
      }
      void ImGui::TreePushRawID(ImGuiID id)
   {
      ImGuiWindow* window = GetCurrentWindow();
   Indent();
   window->DC.TreeDepth++;
      }
      void ImGui::TreePop()
   {
      ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
            window->DC.TreeDepth--;
   if (g.NavMoveDir == ImGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
      if (g.NavIdIsAlive && (window->DC.TreeDepthMayJumpToParentOnPop & (1 << window->DC.TreeDepth)))
   {
         SetNavID(window->IDStack.back(), g.NavLayer);
               IM_ASSERT(window->IDStack.Size > 1); // There should always be 1 element in the IDStack (pushed during window creation). If this triggers you called TreePop/PopID too much.
      }
      void ImGui::TreeAdvanceToLabelPos()
   {
      ImGuiContext& g = *GImGui;
      }
      // Horizontal distance preceding label when using TreeNode() or Bullet()
   float ImGui::GetTreeNodeToLabelSpacing()
   {
      ImGuiContext& g = *GImGui;
      }
      void ImGui::SetNextTreeNodeOpen(bool is_open, ImGuiCond cond)
   {
      ImGuiContext& g = *GImGui;
   if (g.CurrentWindow->SkipItems)
         g.NextTreeNodeOpenVal = is_open;
      }
      // CollapsingHeader returns true when opened but do not indent nor push into the ID stack (because of the ImGuiTreeNodeFlags_NoTreePushOnOpen flag).
   // This is basically the same as calling TreeNodeEx(label, ImGuiTreeNodeFlags_CollapsingHeader). You can remove the _NoTreePushOnOpen flag if you want behavior closer to normal TreeNode().
   bool ImGui::CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
               }
      bool ImGui::CollapsingHeader(const char* label, bool* p_open, ImGuiTreeNodeFlags flags)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            if (p_open && !*p_open)
            ImGuiID id = window->GetID(label);
   bool is_open = TreeNodeBehavior(id, flags | ImGuiTreeNodeFlags_CollapsingHeader | (p_open ? ImGuiTreeNodeFlags_AllowItemOverlap : 0), label);
   if (p_open)
   {
      // Create a small overlapping close button // FIXME: We can evolve this into user accessible helpers to add extra buttons on title bars, headers, etc.
   ImGuiContext& g = *GImGui;
   ImGuiItemHoveredDataBackup last_item_backup;
   float button_radius = g.FontSize * 0.5f;
   ImVec2 button_center = ImVec2(ImMin(window->DC.LastItemRect.Max.x, window->ClipRect.Max.x) - g.Style.FramePadding.x - button_radius, window->DC.LastItemRect.GetCenter().y);
   if (CloseButton(window->GetID((void*)((intptr_t)id+1)), button_center, button_radius))
                        }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: Selectable
   //-------------------------------------------------------------------------
   // - Selectable()
   //-------------------------------------------------------------------------
      // Tip: pass a non-visible label (e.g. "##dummy") then you can use the space to draw other text or image.
   // But you need to make sure the ID is unique, e.g. enclose calls in PushID/PopID or use ##unique_id.
   bool ImGui::Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
            if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) // FIXME-OPT: Avoid if vertically clipped.
            ImGuiID id = window->GetID(label);
   ImVec2 label_size = CalcTextSize(label, NULL, true);
   ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
   ImVec2 pos = window->DC.CursorPos;
   pos.y += window->DC.CurrentLineTextBaseOffset;
   ImRect bb_inner(pos, pos + size);
            // Fill horizontal space.
   ImVec2 window_padding = window->WindowPadding;
   float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? GetWindowContentRegionMax().x : GetContentRegionMax().x;
   float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
   ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
   ImRect bb(pos, pos + size_draw);
   if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
            // Selectables are tightly packed together, we extend the box to cover spacing between selectable.
   float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
   float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
   float spacing_R = style.ItemSpacing.x - spacing_L;
   float spacing_D = style.ItemSpacing.y - spacing_U;
   bb.Min.x -= spacing_L;
   bb.Min.y -= spacing_U;
   bb.Max.x += spacing_R;
   bb.Max.y += spacing_D;
   if (!ItemAdd(bb, id))
   {
      if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
                     // We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
   ImGuiButtonFlags button_flags = 0;
   if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
   if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
   if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
   if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
   if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
   if (flags & ImGuiSelectableFlags_Disabled)
            bool hovered, held;
   bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
   // Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
   if (pressed || hovered)
      if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
   {
         g.NavDisableHighlight = true;
      if (pressed)
            // Render
   if (hovered || selected)
   {
      const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
   RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
               if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
   {
      PushColumnClipRect();
               if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
   RenderTextClipped(bb_inner.Min, bb_inner.Max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
            // Automatically close popups
   if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
            }
      bool ImGui::Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
   {
      if (Selectable(label, *p_selected, flags, size_arg))
   {
      *p_selected = !*p_selected;
      }
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: ListBox
   //-------------------------------------------------------------------------
   // - ListBox()
   // - ListBoxHeader()
   // - ListBoxFooter()
   //-------------------------------------------------------------------------
      // FIXME: In principle this function should be called BeginListBox(). We should rename it after re-evaluating if we want to keep the same signature.
   // Helper to calculate the size of a listbox and display a label on the right.
   // Tip: To have a list filling the entire window width, PushItemWidth(-1) and pass an non-visible label e.g. "##empty"
   bool ImGui::ListBoxHeader(const char* label, const ImVec2& size_arg)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            const ImGuiStyle& style = GetStyle();
   const ImGuiID id = GetID(label);
            // Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
   ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
   ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
   ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
   ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
            if (!IsRectVisible(bb.Min, bb.Max))
   {
      ItemSize(bb.GetSize(), style.FramePadding.y);
   ItemAdd(bb, 0, &frame_bb);
               BeginGroup();
   if (label_size.x > 0)
            BeginChildFrame(id, frame_bb.GetSize());
      }
      // FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
   bool ImGui::ListBoxHeader(const char* label, int items_count, int height_in_items)
   {
      // Size default to hold ~7.25 items.
   // We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar.
   // We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
   // I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
   if (height_in_items < 0)
         const ImGuiStyle& style = GetStyle();
            // We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
   ImVec2 size;
   size.x = 0.0f;
   size.y = GetTextLineHeightWithSpacing() * height_in_items_f + style.FramePadding.y * 2.0f;
      }
      // FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
   void ImGui::ListBoxFooter()
   {
      ImGuiWindow* parent_window = GetCurrentWindow()->ParentWindow;
   const ImRect bb = parent_window->DC.LastItemRect;
                     // Redeclare item size so that it includes the label (we have stored the full size in LastItemRect)
   // We call SameLine() to restore DC.CurrentLine* data
   SameLine();
   parent_window->DC.CursorPos = bb.Min;
   ItemSize(bb, style.FramePadding.y);
      }
      bool ImGui::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items)
   {
      const bool value_changed = ListBox(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_items);
      }
      bool ImGui::ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
   {
      if (!ListBoxHeader(label, items_count, height_in_items))
            // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
   ImGuiContext& g = *GImGui;
   bool value_changed = false;
   ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
   while (clipper.Step())
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
   {
         const bool item_selected = (i == *current_item);
   const char* item_text;
                  PushID(i);
   if (Selectable(item_text, item_selected))
   {
      *current_item = i;
      }
   if (item_selected)
            ListBoxFooter();
   if (value_changed)
               }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: PlotLines, PlotHistogram
   //-------------------------------------------------------------------------
   // - PlotEx() [Internal]
   // - PlotLines()
   // - PlotHistogram()
   //-------------------------------------------------------------------------
      void ImGui::PlotEx(ImGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
            const ImVec2 label_size = CalcTextSize(label, NULL, true);
   if (frame_size.x == 0.0f)
         if (frame_size.y == 0.0f)
            const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
   const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
   const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
   ItemSize(total_bb, style.FramePadding.y);
   if (!ItemAdd(total_bb, 0, &frame_bb))
                  // Determine scale from values if not specified
   if (scale_min == FLT_MAX || scale_max == FLT_MAX)
   {
      float v_min = FLT_MAX;
   float v_max = -FLT_MAX;
   for (int i = 0; i < values_count; i++)
   {
         const float v = values_getter(data, i);
   v_min = ImMin(v_min, v);
   }
   if (scale_min == FLT_MAX)
         if (scale_max == FLT_MAX)
                        if (values_count > 0)
   {
      int res_w = ImMin((int)frame_size.x, values_count) + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);
            // Tooltip on hover
   int v_hovered = -1;
   if (hovered && inner_bb.Contains(g.IO.MousePos))
   {
         const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
                  const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
   const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
   if (plot_type == ImGuiPlotType_Lines)
         else if (plot_type == ImGuiPlotType_Histogram)
                  const float t_step = 1.0f / (float)res_w;
            float v0 = values_getter(data, (0 + values_offset) % values_count);
   float t0 = 0.0f;
   ImVec2 tp0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale) );                       // Point in the normalized space of our target rectangle
            const ImU32 col_base = GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLines : ImGuiCol_PlotHistogram);
            for (int n = 0; n < res_w; n++)
   {
         const float t1 = t0 + t_step;
   const int v1_idx = (int)(t0 * item_count + 0.5f);
   IM_ASSERT(v1_idx >= 0 && v1_idx < values_count);
                  // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
   ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
   ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, (plot_type == ImGuiPlotType_Lines) ? tp1 : ImVec2(tp1.x, histogram_zero_line_t));
   if (plot_type == ImGuiPlotType_Lines)
   {
         }
   else if (plot_type == ImGuiPlotType_Histogram)
   {
      if (pos1.x >= pos0.x + 2.0f)
                     t0 = t1;
               // Text overlay
   if (overlay_text)
            if (label_size.x > 0.0f)
      }
      struct ImGuiPlotArrayGetterData
   {
      const float* Values;
               };
      static float Plot_ArrayGetter(void* data, int idx)
   {
      ImGuiPlotArrayGetterData* plot_data = (ImGuiPlotArrayGetterData*)data;
   const float v = *(const float*)(const void*)((const unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
      }
      void ImGui::PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride)
   {
      ImGuiPlotArrayGetterData data(values, stride);
      }
      void ImGui::PlotLines(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
   {
         }
      void ImGui::PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride)
   {
      ImGuiPlotArrayGetterData data(values, stride);
      }
      void ImGui::PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
   {
         }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: Value helpers
   // Those is not very useful, legacy API.
   //-------------------------------------------------------------------------
   // - Value()
   //-------------------------------------------------------------------------
      void ImGui::Value(const char* prefix, bool b)
   {
         }
      void ImGui::Value(const char* prefix, int v)
   {
         }
      void ImGui::Value(const char* prefix, unsigned int v)
   {
         }
      void ImGui::Value(const char* prefix, float v, const char* float_format)
   {
      if (float_format)
   {
      char fmt[64];
   ImFormatString(fmt, IM_ARRAYSIZE(fmt), "%%s: %s", float_format);
      }
   else
   {
            }
      //-------------------------------------------------------------------------
   // [SECTION] MenuItem, BeginMenu, EndMenu, etc.
   //-------------------------------------------------------------------------
   // - ImGuiMenuColumns [Internal]
   // - BeginMainMenuBar()
   // - EndMainMenuBar()
   // - BeginMenuBar()
   // - EndMenuBar()
   // - BeginMenu()
   // - EndMenu()
   // - MenuItem()
   //-------------------------------------------------------------------------
      // Helpers for internal use
   ImGuiMenuColumns::ImGuiMenuColumns()
   {
      Count = 0;
   Spacing = Width = NextWidth = 0.0f;
   memset(Pos, 0, sizeof(Pos));
      }
      void ImGuiMenuColumns::Update(int count, float spacing, bool clear)
   {
      IM_ASSERT(Count <= IM_ARRAYSIZE(Pos));
   Count = count;
   Width = NextWidth = 0.0f;
   Spacing = spacing;
   if (clear) memset(NextWidths, 0, sizeof(NextWidths));
   for (int i = 0; i < Count; i++)
   {
      if (i > 0 && NextWidths[i] > 0.0f)
         Pos[i] = (float)(int)Width;
   Width += NextWidths[i];
         }
      float ImGuiMenuColumns::DeclColumns(float w0, float w1, float w2) // not using va_arg because they promote float to double
   {
      NextWidth = 0.0f;
   NextWidths[0] = ImMax(NextWidths[0], w0);
   NextWidths[1] = ImMax(NextWidths[1], w1);
   NextWidths[2] = ImMax(NextWidths[2], w2);
   for (int i = 0; i < 3; i++)
            }
      float ImGuiMenuColumns::CalcExtraSpace(float avail_w)
   {
         }
      // For the main menu bar, which cannot be moved, we honor g.Style.DisplaySafeAreaPadding to ensure text can be visible on a TV set.
   bool ImGui::BeginMainMenuBar()
   {
      ImGuiContext& g = *GImGui;
   g.NextWindowData.MenuBarOffsetMinVal = ImVec2(g.Style.DisplaySafeAreaPadding.x, ImMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
   SetNextWindowPos(ImVec2(0.0f, 0.0f));
   SetNextWindowSize(ImVec2(g.IO.DisplaySize.x, g.NextWindowData.MenuBarOffsetMinVal.y + g.FontBaseSize + g.Style.FramePadding.y));
   PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
   PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0,0));
   ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
   bool is_open = Begin("##MainMenuBar", NULL, window_flags) && BeginMenuBar();
   PopStyleVar(2);
   g.NextWindowData.MenuBarOffsetMinVal = ImVec2(0.0f, 0.0f);
   if (!is_open)
   {
      End();
      }
      }
      void ImGui::EndMainMenuBar()
   {
               // When the user has left the menu layer (typically: closed menus through activation of an item), we restore focus to the previous window
   ImGuiContext& g = *GImGui;
   if (g.CurrentWindow == g.NavWindow && g.NavLayer == 0)
               }
      bool ImGui::BeginMenuBar()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
         if (!(window->Flags & ImGuiWindowFlags_MenuBar))
            IM_ASSERT(!window->DC.MenuBarAppending);
   BeginGroup(); // Backup position on layer 0
            // We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
   // We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
   ImRect bar_rect = window->MenuBarRect();
   ImRect clip_rect(ImFloor(bar_rect.Min.x + 0.5f), ImFloor(bar_rect.Min.y + window->WindowBorderSize + 0.5f), ImFloor(ImMax(bar_rect.Min.x, bar_rect.Max.x - window->WindowRounding) + 0.5f), ImFloor(bar_rect.Max.y + 0.5f));
   clip_rect.ClipWith(window->OuterRectClipped);
            window->DC.CursorPos = ImVec2(bar_rect.Min.x + window->DC.MenuBarOffset.x, bar_rect.Min.y + window->DC.MenuBarOffset.y);
   window->DC.LayoutType = ImGuiLayoutType_Horizontal;
   window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
   window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);
   window->DC.MenuBarAppending = true;
   AlignTextToFramePadding();
      }
      void ImGui::EndMenuBar()
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
                  // Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
   if (NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
   {
      ImGuiWindow* nav_earliest_child = g.NavWindow;
   while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
         if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && g.NavMoveRequestForward == ImGuiNavForward_None)
   {
         // To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
   // This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth the hassle/cost)
   IM_ASSERT(window->DC.NavLayerActiveMaskNext & 0x02); // Sanity check
   FocusWindow(window);
   SetNavIDWithRectRel(window->NavLastIds[1], 1, window->NavRectRel[1]);
   g.NavLayer = ImGuiNavLayer_Menu;
   g.NavDisableHighlight = true; // Hide highlight for the current frame so we don't see the intermediary selection.
   g.NavMoveRequestForward = ImGuiNavForward_ForwardQueued;
               IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar);
   IM_ASSERT(window->DC.MenuBarAppending);
   PopClipRect();
   PopID();
   window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->MenuBarRect().Min.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
   window->DC.GroupStack.back().AdvanceCursor = false;
   EndGroup(); // Restore position on layer 0
   window->DC.LayoutType = ImGuiLayoutType_Vertical;
   window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
   window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
      }
      bool ImGui::BeginMenu(const char* label, bool enabled)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;
                     bool pressed;
   bool menu_is_open = IsPopupOpen(id);
   bool menuset_is_open = !(window->Flags & ImGuiWindowFlags_Popup) && (g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].OpenParentId == window->IDStack.back());
   ImGuiWindow* backed_nav_window = g.NavWindow;
   if (menuset_is_open)
            // The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
   // However the final position is going to be different! It is choosen by FindBestWindowPosForPopup().
   // e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
   ImVec2 popup_pos, pos = window->DC.CursorPos;
   if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
   {
      // Menu inside an horizontal menu bar
   // Selectable extend their highlight by half ItemSpacing in each direction.
   // For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
   popup_pos = ImVec2(pos.x - 1.0f - (float)(int)(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + window->MenuBarHeight());
   window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * 0.5f);
   PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 2.0f);
   float w = label_size.x;
   pressed = Selectable(label, menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_PressedOnClick | ImGuiSelectableFlags_DontClosePopups | (!enabled ? ImGuiSelectableFlags_Disabled : 0), ImVec2(w, 0.0f));
   PopStyleVar();
      }
   else
   {
      // Menu inside a menu
   popup_pos = ImVec2(pos.x, pos.y - style.WindowPadding.y);
   float w = window->MenuColumns.DeclColumns(label_size.x, 0.0f, (float)(int)(g.FontSize * 1.20f)); // Feedback to next frame
   float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);
   pressed = Selectable(label, menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_PressedOnClick | ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_DrawFillAvailWidth | (!enabled ? ImGuiSelectableFlags_Disabled : 0), ImVec2(w, 0.0f));
   if (!enabled) PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
   RenderArrow(pos + ImVec2(window->MenuColumns.Pos[2] + extra_w + g.FontSize * 0.30f, 0.0f), ImGuiDir_Right);
               const bool hovered = enabled && ItemHoverable(window->DC.LastItemRect, id);
   if (menuset_is_open)
            bool want_open = false, want_close = false;
   if (window->DC.LayoutType == ImGuiLayoutType_Vertical) // (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
   {
      // Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
   bool moving_within_opened_triangle = false;
   if (g.HoveredWindow == window && g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].ParentWindow == window && !(window->Flags & ImGuiWindowFlags_MenuBar))
   {
         if (ImGuiWindow* next_window = g.OpenPopupStack[g.BeginPopupStack.Size].Window)
   {
      // FIXME-DPI: Values should be derived from a master "scale" factor.
   ImRect next_window_rect = next_window->Rect();
   ImVec2 ta = g.IO.MousePos - g.IO.MouseDelta;
   ImVec2 tb = (window->Pos.x < next_window->Pos.x) ? next_window_rect.GetTL() : next_window_rect.GetTR();
   ImVec2 tc = (window->Pos.x < next_window->Pos.x) ? next_window_rect.GetBL() : next_window_rect.GetBR();
   float extra = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, 5.0f, 30.0f); // add a bit of extra slack.
   ta.x += (window->Pos.x < next_window->Pos.x) ? -0.5f : +0.5f;    // to avoid numerical issues
   tb.y = ta.y + ImMax((tb.y - extra) - ta.y, -100.0f);             // triangle is maximum 200 high to limit the slope and the bias toward large sub-menus // FIXME: Multiply by fb_scale?
   tc.y = ta.y + ImMin((tc.y + extra) - ta.y, +100.0f);
   moving_within_opened_triangle = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
               want_close = (menu_is_open && !hovered && g.HoveredWindow == window && g.HoveredIdPreviousFrame != 0 && g.HoveredIdPreviousFrame != id && !moving_within_opened_triangle);
            if (g.NavActivateId == id)
   {
         want_close = menu_is_open;
   }
   if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right) // Nav-Right to open
   {
         want_open = true;
      }
   else
   {
      // Menu bar
   if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
   {
         want_close = true;
   }
   else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
   {
         }
   else if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Down) // Nav-Down to open
   {
         want_open = true;
               if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
         if (want_close && IsPopupOpen(id))
                     if (!menu_is_open && want_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
   {
      // Don't recycle same menu level in the same frame, first close the other menu and yield for a frame.
   OpenPopup(label);
               menu_is_open |= want_open;
   if (want_open)
            if (menu_is_open)
   {
      // Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
   SetNextWindowPos(popup_pos, ImGuiCond_Always);
   ImGuiWindowFlags flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
   if (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
                        }
      void ImGui::EndMenu()
   {
      // Nav: When a left move request _within our child menu_ failed, close ourselves (the _parent_ menu).
   // A menu doesn't close itself because EndMenuBar() wants the catch the last Left<>Right inputs.
   // However, it means that with the current code, a BeginMenu() from outside another menu or a menu-bar won't be closable with the Left direction.
   ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
   if (g.NavWindow && g.NavWindow->ParentWindow == window && g.NavMoveDir == ImGuiDir_Left && NavMoveRequestButNoResultYet() && window->DC.LayoutType == ImGuiLayoutType_Vertical)
   {
      ClosePopupToLevel(g.BeginPopupStack.Size, true);
                  }
      bool ImGui::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)
   {
      ImGuiWindow* window = GetCurrentWindow();
   if (window->SkipItems)
            ImGuiContext& g = *GImGui;
   ImGuiStyle& style = g.Style;
   ImVec2 pos = window->DC.CursorPos;
            ImGuiSelectableFlags flags = ImGuiSelectableFlags_PressedOnRelease | (enabled ? 0 : ImGuiSelectableFlags_Disabled);
   bool pressed;
   if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
   {
      // Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
   // Note that in this situation we render neither the shortcut neither the selected tick mark
   float w = label_size.x;
   window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * 0.5f);
   PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 2.0f);
   pressed = Selectable(label, false, flags, ImVec2(w, 0.0f));
   PopStyleVar();
      }
   else
   {
      ImVec2 shortcut_size = shortcut ? CalcTextSize(shortcut, NULL) : ImVec2(0.0f, 0.0f);
   float w = window->MenuColumns.DeclColumns(label_size.x, shortcut_size.x, (float)(int)(g.FontSize * 1.20f)); // Feedback for next frame
   float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);
   pressed = Selectable(label, false, flags | ImGuiSelectableFlags_DrawFillAvailWidth, ImVec2(w, 0.0f));
   if (shortcut_size.x > 0.0f)
   {
         PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
   RenderText(pos + ImVec2(window->MenuColumns.Pos[1] + extra_w, 0.0f), shortcut, NULL, false);
   }
   if (selected)
               IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
      }
      bool ImGui::MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled)
   {
      if (MenuItem(label, shortcut, p_selected ? *p_selected : false, enabled))
   {
      if (p_selected)
            }
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
   //-------------------------------------------------------------------------
   // [BETA API] API may evolve! This code has been extracted out of the Docking branch,
   // and some of the construct which are not used in Master may be left here to facilitate merging.
   //-------------------------------------------------------------------------
   // - BeginTabBar()
   // - BeginTabBarEx() [Internal]
   // - EndTabBar()
   // - TabBarLayout() [Internal]
   // - TabBarCalcTabID() [Internal]
   // - TabBarCalcMaxTabWidth() [Internal]
   // - TabBarFindTabById() [Internal]
   // - TabBarRemoveTab() [Internal]
   // - TabBarCloseTab() [Internal]
   // - TabBarScrollClamp()v
   // - TabBarScrollToTab() [Internal]
   // - TabBarQueueChangeTabOrder() [Internal]
   // - TabBarScrollingButtons() [Internal]
   // - TabBarTabListPopupButton() [Internal]
   //-------------------------------------------------------------------------
      namespace ImGui
   {
      static void             TabBarLayout(ImGuiTabBar* tab_bar);
   static ImU32            TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label);
   static float            TabBarCalcMaxTabWidth();
   static float            TabBarScrollClamp(ImGuiTabBar* tab_bar, float scrolling);
   static void             TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab);
   static ImGuiTabItem*    TabBarScrollingButtons(ImGuiTabBar* tab_bar);
      }
      ImGuiTabBar::ImGuiTabBar()
   {
      ID = 0;
   SelectedTabId = NextSelectedTabId = VisibleTabId = 0;
   CurrFrameVisible = PrevFrameVisible = -1;
   ContentsHeight = 0.0f;
   OffsetMax = OffsetNextTab = 0.0f;
   ScrollingAnim = ScrollingTarget = 0.0f;
   Flags = ImGuiTabBarFlags_None;
   ReorderRequestTabId = 0;
   ReorderRequestDir = 0;
   WantLayout = VisibleTabWasSubmitted = false;
      }
      static int IMGUI_CDECL TabItemComparerByVisibleOffset(const void* lhs, const void* rhs)
   {
      const ImGuiTabItem* a = (const ImGuiTabItem*)lhs;
   const ImGuiTabItem* b = (const ImGuiTabItem*)rhs;
      }
      static int IMGUI_CDECL TabBarSortItemComparer(const void* lhs, const void* rhs)
   {
      const ImGuiTabBarSortItem* a = (const ImGuiTabBarSortItem*)lhs;
   const ImGuiTabBarSortItem* b = (const ImGuiTabBarSortItem*)rhs;
   if (int d = (int)(b->Width - a->Width))
            }
      bool    ImGui::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags)
   {
      ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
   if (window->SkipItems)
            ImGuiID id = window->GetID(str_id);
   ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
   ImRect tab_bar_bb = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->InnerClipRect.Max.x, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
   tab_bar->ID = id;
      }
      bool    ImGui::BeginTabBarEx(ImGuiTabBar* tab_bar, const ImRect& tab_bar_bb, ImGuiTabBarFlags flags)
   {
      ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
   if (window->SkipItems)
            if ((flags & ImGuiTabBarFlags_DockNode) == 0)
            g.CurrentTabBar.push_back(tab_bar);
   if (tab_bar->CurrFrameVisible == g.FrameCount)
   {
      //IMGUI_DEBUG_LOG("BeginTabBarEx already called this frame\n", g.FrameCount);
   IM_ASSERT(0);
               // When toggling back from ordered to manually-reorderable, shuffle tabs to enforce the last visible order.
   // Otherwise, the most recently inserted tabs would move at the end of visible list which can be a little too confusing or magic for the user.
   if ((flags & ImGuiTabBarFlags_Reorderable) && !(tab_bar->Flags & ImGuiTabBarFlags_Reorderable) && tab_bar->Tabs.Size > 1 && tab_bar->PrevFrameVisible != -1)
            // Flags
   if ((flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
            tab_bar->Flags = flags;
   tab_bar->BarRect = tab_bar_bb;
   tab_bar->WantLayout = true; // Layout will be done on the first call to ItemTab()
   tab_bar->PrevFrameVisible = tab_bar->CurrFrameVisible;
   tab_bar->CurrFrameVisible = g.FrameCount;
            // Layout
   ItemSize(ImVec2(tab_bar->OffsetMax, tab_bar->BarRect.GetHeight()));
            // Draw separator
   const ImU32 col = GetColorU32((flags & ImGuiTabBarFlags_IsFocused) ? ImGuiCol_TabActive : ImGuiCol_Tab);
   const float y = tab_bar->BarRect.Max.y - 1.0f;
   {
      const float separator_min_x = tab_bar->BarRect.Min.x - window->WindowPadding.x;
   const float separator_max_x = tab_bar->BarRect.Max.x + window->WindowPadding.x;
      }
      }
      void    ImGui::EndTabBar()
   {
      ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
   if (window->SkipItems)
            IM_ASSERT(!g.CurrentTabBar.empty());      // Mismatched BeginTabBar/EndTabBar
   ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
   if (tab_bar->WantLayout)
            // Restore the last visible height if no tab is visible, this reduce vertical flicker/movement when a tabs gets removed without calling SetTabItemClosed().
   const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
   if (tab_bar->VisibleTabWasSubmitted || tab_bar->VisibleTabId == 0 || tab_bar_appearing)
         else
            if ((tab_bar->Flags & ImGuiTabBarFlags_DockNode) == 0)
            }
      // This is called only once a frame before by the first call to ItemTab()
   // The reason we're not calling it in BeginTabBar() is to leave a chance to the user to call the SetTabItemClosed() functions.
   static void ImGui::TabBarLayout(ImGuiTabBar* tab_bar)
   {
      ImGuiContext& g = *GImGui;
            // Garbage collect
   int tab_dst_n = 0;
   for (int tab_src_n = 0; tab_src_n < tab_bar->Tabs.Size; tab_src_n++)
   {
      ImGuiTabItem* tab = &tab_bar->Tabs[tab_src_n];
   if (tab->LastFrameVisible < tab_bar->PrevFrameVisible)
   {
         if (tab->ID == tab_bar->SelectedTabId)
         }
   if (tab_dst_n != tab_src_n)
            }
   if (tab_bar->Tabs.Size != tab_dst_n)
            // Setup next selected tab
   ImGuiID scroll_track_selected_tab_id = 0;
   if (tab_bar->NextSelectedTabId)
   {
      tab_bar->SelectedTabId = tab_bar->NextSelectedTabId;
   tab_bar->NextSelectedTabId = 0;
               // Process order change request (we could probably process it when requested but it's just saner to do it in a single spot).
   if (tab_bar->ReorderRequestTabId != 0)
   {
      if (ImGuiTabItem* tab1 = TabBarFindTabByID(tab_bar, tab_bar->ReorderRequestTabId))
   {
         //IM_ASSERT(tab_bar->Flags & ImGuiTabBarFlags_Reorderable); // <- this may happen when using debug tools
   int tab2_order = tab_bar->GetTabOrder(tab1) + tab_bar->ReorderRequestDir;
   if (tab2_order >= 0 && tab2_order < tab_bar->Tabs.Size)
   {
      ImGuiTabItem* tab2 = &tab_bar->Tabs[tab2_order];
   ImGuiTabItem item_tmp = *tab1;
   *tab1 = *tab2;
   *tab2 = item_tmp;
   if (tab2->ID == tab_bar->SelectedTabId)
            }
   if (tab_bar->Flags & ImGuiTabBarFlags_SaveSettings)
   }
               // Tab List Popup (will alter tab_bar->BarRect and therefore the available width!)
   const bool tab_list_popup_button = (tab_bar->Flags & ImGuiTabBarFlags_TabListPopupButton) != 0;
   if (tab_list_popup_button)
      if (ImGuiTabItem* tab_to_select = TabBarTabListPopupButton(tab_bar)) // NB: Will alter BarRect.Max.x!
         ImVector<ImGuiTabBarSortItem>& width_sort_buffer = g.TabSortByWidthBuffer;
            // Compute ideal widths
   float width_total_contents = 0.0f;
   ImGuiTabItem* most_recently_selected_tab = NULL;
   bool found_selected_tab_id = false;
   for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
   {
      ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
            if (most_recently_selected_tab == NULL || most_recently_selected_tab->LastFrameSelected < tab->LastFrameSelected)
         if (tab->ID == tab_bar->SelectedTabId)
            // Refresh tab width immediately, otherwise changes of style e.g. style.FramePadding.x would noticeably lag in the tab bar.
   // Additionally, when using TabBarAddTab() to manipulate tab bar order we occasionally insert new tabs that don't have a width yet,
   // and we cannot wait for the next BeginTabItem() call. We cannot compute this width within TabBarAddTab() because font size depends on the active window.
   const char* tab_name = tab_bar->GetTabName(tab);
                     // Store data so we can build an array sorted by width if we need to shrink tabs down
   width_sort_buffer[tab_n].Index = tab_n;
               // Compute width
   const float width_avail = tab_bar->BarRect.GetWidth();
   float width_excess = (width_avail < width_total_contents) ? (width_total_contents - width_avail) : 0.0f;
   if (width_excess > 0.0f && (tab_bar->Flags & ImGuiTabBarFlags_FittingPolicyResizeDown))
   {
      // If we don't have enough room, resize down the largest tabs first
   if (tab_bar->Tabs.Size > 1)
         int tab_count_same_width = 1;
   while (width_excess > 0.0f && tab_count_same_width < tab_bar->Tabs.Size)
   {
         while (tab_count_same_width < tab_bar->Tabs.Size && width_sort_buffer[0].Width == width_sort_buffer[tab_count_same_width].Width)
         float width_to_remove_per_tab_max = (tab_count_same_width < tab_bar->Tabs.Size) ? (width_sort_buffer[0].Width - width_sort_buffer[tab_count_same_width].Width) : (width_sort_buffer[0].Width - 1.0f);
   float width_to_remove_per_tab = ImMin(width_excess / tab_count_same_width, width_to_remove_per_tab_max);
   for (int tab_n = 0; tab_n < tab_count_same_width; tab_n++)
         }
   for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
      }
   else
   {
      const float tab_max_width = TabBarCalcMaxTabWidth();
   for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
   {
         ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
               // Layout all active tabs
   float offset_x = 0.0f;
   for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
   {
      ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
   tab->Offset = offset_x;
   if (scroll_track_selected_tab_id == 0 && g.NavJustMovedToId == tab->ID)
            }
   tab_bar->OffsetMax = ImMax(offset_x - g.Style.ItemInnerSpacing.x, 0.0f);
            // Horizontal scrolling buttons
   const bool scrolling_buttons = (tab_bar->OffsetMax > tab_bar->BarRect.GetWidth() && tab_bar->Tabs.Size > 1) && !(tab_bar->Flags & ImGuiTabBarFlags_NoTabListScrollingButtons) && (tab_bar->Flags & ImGuiTabBarFlags_FittingPolicyScroll);
   if (scrolling_buttons)
      if (ImGuiTabItem* tab_to_select = TabBarScrollingButtons(tab_bar)) // NB: Will alter BarRect.Max.x!
         // If we have lost the selected tab, select the next most recently active one
   if (found_selected_tab_id == false)
         if (tab_bar->SelectedTabId == 0 && tab_bar->NextSelectedTabId == 0 && most_recently_selected_tab != NULL)
            // Lock in visible tab
   tab_bar->VisibleTabId = tab_bar->SelectedTabId;
            // Update scrolling
   if (scroll_track_selected_tab_id)
      if (ImGuiTabItem* scroll_track_selected_tab = TabBarFindTabByID(tab_bar, scroll_track_selected_tab_id))
      tab_bar->ScrollingAnim = TabBarScrollClamp(tab_bar, tab_bar->ScrollingAnim);
   tab_bar->ScrollingTarget = TabBarScrollClamp(tab_bar, tab_bar->ScrollingTarget);
   const float scrolling_speed = (tab_bar->PrevFrameVisible + 1 < g.FrameCount) ? FLT_MAX : (g.IO.DeltaTime * g.FontSize * 70.0f);
   if (tab_bar->ScrollingAnim != tab_bar->ScrollingTarget)
            // Clear name buffers
   if ((tab_bar->Flags & ImGuiTabBarFlags_DockNode) == 0)
      }
      // Dockables uses Name/ID in the global namespace. Non-dockable items use the ID stack.
   static ImU32   ImGui::TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label)
   {
      if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
   {
      ImGuiID id = ImHashStr(label, 0);
   KeepAliveID(id);
      }
   else
   {
      ImGuiWindow* window = GImGui->CurrentWindow;
         }
      static float ImGui::TabBarCalcMaxTabWidth()
   {
      ImGuiContext& g = *GImGui;
      }
      ImGuiTabItem* ImGui::TabBarFindTabByID(ImGuiTabBar* tab_bar, ImGuiID tab_id)
   {
      if (tab_id != 0)
      for (int n = 0; n < tab_bar->Tabs.Size; n++)
               }
      // The *TabId fields be already set by the docking system _before_ the actual TabItem was created, so we clear them regardless.
   void ImGui::TabBarRemoveTab(ImGuiTabBar* tab_bar, ImGuiID tab_id)
   {
      if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_id))
         if (tab_bar->VisibleTabId == tab_id)      { tab_bar->VisibleTabId = 0; }
   if (tab_bar->SelectedTabId == tab_id)     { tab_bar->SelectedTabId = 0; }
      }
      // Called on manual closure attempt
   void ImGui::TabBarCloseTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
   {
      if ((tab_bar->VisibleTabId == tab->ID) && !(tab->Flags & ImGuiTabItemFlags_UnsavedDocument))
   {
      // This will remove a frame of lag for selecting another tab on closure.
   // However we don't run it in the case where the 'Unsaved' flag is set, so user gets a chance to fully undo the closure
   tab->LastFrameVisible = -1;
      }
   else if ((tab_bar->VisibleTabId != tab->ID) && (tab->Flags & ImGuiTabItemFlags_UnsavedDocument))
   {
      // Actually select before expecting closure
         }
      static float ImGui::TabBarScrollClamp(ImGuiTabBar* tab_bar, float scrolling)
   {
      scrolling = ImMin(scrolling, tab_bar->OffsetMax - tab_bar->BarRect.GetWidth());
      }
      static void ImGui::TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
   {
      ImGuiContext& g = *GImGui;
   float margin = g.FontSize * 1.0f; // When to scroll to make Tab N+1 visible always make a bit of N visible to suggest more scrolling area (since we don't have a scrollbar)
   int order = tab_bar->GetTabOrder(tab);
   float tab_x1 = tab->Offset + (order > 0 ? -margin : 0.0f);
   float tab_x2 = tab->Offset + tab->Width + (order + 1 < tab_bar->Tabs.Size ? margin : 1.0f);
   if (tab_bar->ScrollingTarget > tab_x1)
         if (tab_bar->ScrollingTarget + tab_bar->BarRect.GetWidth() < tab_x2)
      }
      void ImGui::TabBarQueueChangeTabOrder(ImGuiTabBar* tab_bar, const ImGuiTabItem* tab, int dir)
   {
      IM_ASSERT(dir == -1 || dir == +1);
   IM_ASSERT(tab_bar->ReorderRequestTabId == 0);
   tab_bar->ReorderRequestTabId = tab->ID;
      }
      static ImGuiTabItem* ImGui::TabBarScrollingButtons(ImGuiTabBar* tab_bar)
   {
      ImGuiContext& g = *GImGui;
            const ImVec2 arrow_button_size(g.FontSize - 2.0f, g.FontSize + g.Style.FramePadding.y * 2.0f);
            const ImVec2 backup_cursor_pos = window->DC.CursorPos;
            const ImRect avail_bar_rect = tab_bar->BarRect;
   bool want_clip_rect = !avail_bar_rect.Contains(ImRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(scrolling_buttons_width, 0.0f)));
   if (want_clip_rect)
                     int select_dir = 0;
   ImVec4 arrow_col = g.Style.Colors[ImGuiCol_Text];
            PushStyleColor(ImGuiCol_Text, arrow_col);
   PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
   const float backup_repeat_delay = g.IO.KeyRepeatDelay;
   const float backup_repeat_rate = g.IO.KeyRepeatRate;
   g.IO.KeyRepeatDelay = 0.250f;
   g.IO.KeyRepeatRate = 0.200f;
   window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width, tab_bar->BarRect.Min.y);
   if (ArrowButtonEx("##<", ImGuiDir_Left, arrow_button_size, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
         window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width + arrow_button_size.x, tab_bar->BarRect.Min.y);
   if (ArrowButtonEx("##>", ImGuiDir_Right, arrow_button_size, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
         PopStyleColor(2);
   g.IO.KeyRepeatRate = backup_repeat_rate;
            if (want_clip_rect)
            if (select_dir != 0)
      if (ImGuiTabItem* tab_item = TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId))
   {
         int selected_order = tab_bar->GetTabOrder(tab_item);
   int target_order = selected_order + select_dir;
      window->DC.CursorPos = backup_cursor_pos;
               }
      static ImGuiTabItem* ImGui::TabBarTabListPopupButton(ImGuiTabBar* tab_bar)
   {
      ImGuiContext& g = *GImGui;
            // We use g.Style.FramePadding.y to match the square ArrowButton size
   const float tab_list_popup_button_width = g.FontSize + g.Style.FramePadding.y;
   const ImVec2 backup_cursor_pos = window->DC.CursorPos;
   window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x - g.Style.FramePadding.y, tab_bar->BarRect.Min.y);
            ImVec4 arrow_col = g.Style.Colors[ImGuiCol_Text];
   arrow_col.w *= 0.5f;
   PushStyleColor(ImGuiCol_Text, arrow_col);
   PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
   bool open = BeginCombo("##v", NULL, ImGuiComboFlags_NoPreview);
            ImGuiTabItem* tab_to_select = NULL;
   if (open)
   {
      for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
   {
         ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
   const char* tab_name = tab_bar->GetTabName(tab);
   if (Selectable(tab_name, tab_bar->SelectedTabId == tab->ID))
   }
               window->DC.CursorPos = backup_cursor_pos;
      }
      //-------------------------------------------------------------------------
   // [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
   //-------------------------------------------------------------------------
   // [BETA API] API may evolve! This code has been extracted out of the Docking branch,
   // and some of the construct which are not used in Master may be left here to facilitate merging.
   //-------------------------------------------------------------------------
   // - BeginTabItem()
   // - EndTabItem()
   // - TabItemEx() [Internal]
   // - SetTabItemClosed()
   // - TabItemCalcSize() [Internal]
   // - TabItemBackground() [Internal]
   // - TabItemLabelAndCloseButton() [Internal]
   //-------------------------------------------------------------------------
      bool    ImGui::BeginTabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags)
   {
      ImGuiContext& g = *GImGui;
   if (g.CurrentWindow->SkipItems)
            IM_ASSERT(g.CurrentTabBar.Size > 0 && "Needs to be called between BeginTabBar() and EndTabBar()!");
   ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
   bool ret = TabItemEx(tab_bar, label, p_open, flags);
   if (ret && !(flags & ImGuiTabItemFlags_NoPushId))
   {
      ImGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
      }
      }
      void    ImGui::EndTabItem()
   {
      ImGuiContext& g = *GImGui;
   if (g.CurrentWindow->SkipItems)
            IM_ASSERT(g.CurrentTabBar.Size > 0 && "Needs to be called between BeginTabBar() and EndTabBar()!");
   ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
   IM_ASSERT(tab_bar->LastTabItemIdx >= 0 && "Needs to be called between BeginTabItem() and EndTabItem()");
   ImGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
   if (!(tab->Flags & ImGuiTabItemFlags_NoPushId))
      }
      bool    ImGui::TabItemEx(ImGuiTabBar* tab_bar, const char* label, bool* p_open, ImGuiTabItemFlags flags)
   {
      // Layout whole tab bar if not already done
   if (tab_bar->WantLayout)
            ImGuiContext& g = *GImGui;
   ImGuiWindow* window = g.CurrentWindow;
   if (window->SkipItems)
            const ImGuiStyle& style = g.Style;
            // If the user called us with *p_open == false, we early out and don't render. We make a dummy call to ItemAdd() so that attempts to use a contextual popup menu with an implicit ID won't use an older ID.
   if (p_open && !*p_open)
   {
      PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
   ItemAdd(ImRect(), id);
   PopItemFlag();
               // Calculate tab contents size
            // Acquire tab data
   ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, id);
   bool tab_is_new = false;
   if (tab == NULL)
   {
      tab_bar->Tabs.push_back(ImGuiTabItem());
   tab = &tab_bar->Tabs.back();
   tab->ID = id;
   tab->Width = size.x;
      }
   tab_bar->LastTabItemIdx = (short)tab_bar->Tabs.index_from_ptr(tab);
            if (p_open == NULL)
            const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
   const bool tab_bar_focused = (tab_bar->Flags & ImGuiTabBarFlags_IsFocused) != 0;
   const bool tab_appearing = (tab->LastFrameVisible + 1 < g.FrameCount);
   tab->LastFrameVisible = g.FrameCount;
            // Append name with zero-terminator
   tab->NameOffset = tab_bar->TabsNames.size();
            // If we are not reorderable, always reset offset based on submission order.
   // (We already handled layout and sizing using the previous known order, but sizing is not affected by order!)
   if (!tab_appearing && !(tab_bar->Flags & ImGuiTabBarFlags_Reorderable))
   {
      tab->Offset = tab_bar->OffsetNextTab;
               // Update selected tab
   if (tab_appearing && (tab_bar->Flags & ImGuiTabBarFlags_AutoSelectNewTabs) && tab_bar->NextSelectedTabId == 0)
      if (!tab_bar_appearing || tab_bar->SelectedTabId == 0)
         // Lock visibility
   bool tab_contents_visible = (tab_bar->VisibleTabId == id);
   if (tab_contents_visible)
            // On the very first frame of a tab bar we let first tab contents be visible to minimize appearing glitches
   if (!tab_contents_visible && tab_bar->SelectedTabId == 0 && tab_bar_appearing)
      if (tab_bar->Tabs.Size == 1 && !(tab_bar->Flags & ImGuiTabBarFlags_AutoSelectNewTabs))
         if (tab_appearing && !(tab_bar_appearing && !tab_is_new))
   {
      PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
   ItemAdd(ImRect(), id);
   PopItemFlag();
               if (tab_bar->SelectedTabId == id)
            // Backup current layout position
            // Layout
   size.x = tab->Width;
   window->DC.CursorPos = tab_bar->BarRect.Min + ImVec2((float)(int)tab->Offset - tab_bar->ScrollingAnim, 0.0f);
   ImVec2 pos = window->DC.CursorPos;
            // We don't have CPU clipping primitives to clip the CloseButton (until it becomes a texture), so need to add an extra draw call (temporary in the case of vertical animation)
   bool want_clip_rect = (bb.Min.x < tab_bar->BarRect.Min.x) || (bb.Max.x >= tab_bar->BarRect.Max.x);
   if (want_clip_rect)
            ItemSize(bb, style.FramePadding.y);
   if (!ItemAdd(bb, id))
   {
      if (want_clip_rect)
         window->DC.CursorPos = backup_main_cursor_pos;
               // Click to Select a tab
   ImGuiButtonFlags button_flags = (ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_AllowItemOverlap);
   if (g.DragDropActive)
         bool hovered, held;
   bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
   hovered |= (g.HoveredId == id);
   if (pressed || ((flags & ImGuiTabItemFlags_SetSelected) && !tab_contents_visible)) // SetSelected can only be passed on explicit tab bar
            // Allow the close button to overlap unless we are dragging (in which case we don't want any overlapping tabs to be hovered)
   if (!held)
            // Drag and drop: re-order tabs
   if (held && !tab_appearing && IsMouseDragging(0))
   {
      if (!g.DragDropActive && (tab_bar->Flags & ImGuiTabBarFlags_Reorderable))
   {
         // While moving a tab it will jump on the other side of the mouse, so we also test for MouseDelta.x
   if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < bb.Min.x)
   {
      if (tab_bar->Flags & ImGuiTabBarFlags_Reorderable)
      }
   else if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > bb.Max.x)
   {
      if (tab_bar->Flags & ImGuiTabBarFlags_Reorderable)
               #if 0
      if (hovered && g.HoveredIdNotActiveTimer > 0.50f && bb.GetWidth() < tab->WidthContents)
   {
      // Enlarge tab display when hovering
   bb.Max.x = bb.Min.x + (float)(int)ImLerp(bb.GetWidth(), tab->WidthContents, ImSaturate((g.HoveredIdNotActiveTimer - 0.40f) * 6.0f));
   display_draw_list = GetOverlayDrawList(window);
         #endif
         // Render tab shape
   ImDrawList* display_draw_list = window->DrawList;
   const ImU32 tab_col = GetColorU32((held || hovered) ? ImGuiCol_TabHovered : tab_contents_visible ? (tab_bar_focused ? ImGuiCol_TabActive : ImGuiCol_TabUnfocusedActive) : (tab_bar_focused ? ImGuiCol_Tab : ImGuiCol_TabUnfocused));
   TabItemBackground(display_draw_list, bb, flags, tab_col);
            // Select with right mouse button. This is so the common idiom for context menu automatically highlight the current widget.
   const bool hovered_unblocked = IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
   if (hovered_unblocked && (IsMouseClicked(1) || IsMouseReleased(1)))
            if (tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)
            // Render tab label, process close button
   const ImGuiID close_button_id = p_open ? window->GetID((void*)((intptr_t)id + 1)) : 0;
   bool just_closed = TabItemLabelAndCloseButton(display_draw_list, bb, flags, tab_bar->FramePadding, label, id, close_button_id);
   if (just_closed && p_open != NULL)
   {
      *p_open = false;
               // Restore main window position so user can draw there
   if (want_clip_rect)
                  // Tooltip (FIXME: Won't work over the close button because ItemOverlap systems messes up with HoveredIdTimer)
   if (g.HoveredId == id && !held && g.HoveredIdNotActiveTimer > 0.50f)
      if (!(tab_bar->Flags & ImGuiTabBarFlags_NoTooltip))
            }
      // [Public] This is call is 100% optional but it allows to remove some one-frame glitches when a tab has been unexpectedly removed.
   // To use it to need to call the function SetTabItemClosed() after BeginTabBar() and before any call to BeginTabItem()
   void    ImGui::SetTabItemClosed(const char* label)
   {
      ImGuiContext& g = *GImGui;
   bool is_within_manual_tab_bar = (g.CurrentTabBar.Size > 0) && !(g.CurrentTabBar.back()->Flags & ImGuiTabBarFlags_DockNode);
   if (is_within_manual_tab_bar)
   {
      ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
   IM_ASSERT(tab_bar->WantLayout);         // Needs to be called AFTER BeginTabBar() and BEFORE the first call to BeginTabItem()
   ImGuiID tab_id = TabBarCalcTabID(tab_bar, label);
         }
      ImVec2 ImGui::TabItemCalcSize(const char* label, bool has_close_button)
   {
      ImGuiContext& g = *GImGui;
   ImVec2 label_size = CalcTextSize(label, NULL, true);
   ImVec2 size = ImVec2(label_size.x + g.Style.FramePadding.x, label_size.y + g.Style.FramePadding.y * 2.0f);
   if (has_close_button)
         else
            }
      void ImGui::TabItemBackground(ImDrawList* draw_list, const ImRect& bb, ImGuiTabItemFlags flags, ImU32 col)
   {
      // While rendering tabs, we trim 1 pixel off the top of our bounding box so they can fit within a regular frame height while looking "detached" from it.
   ImGuiContext& g = *GImGui;
   const float width = bb.GetWidth();
   IM_UNUSED(flags);
   IM_ASSERT(width > 0.0f);
   const float rounding = ImMax(0.0f, ImMin(g.Style.TabRounding, width * 0.5f - 1.0f));
   const float y1 = bb.Min.y + 1.0f;
   const float y2 = bb.Max.y - 1.0f;
   draw_list->PathLineTo(ImVec2(bb.Min.x, y2));
   draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding, y1 + rounding), rounding, 6, 9);
   draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding, y1 + rounding), rounding, 9, 12);
   draw_list->PathLineTo(ImVec2(bb.Max.x, y2));
   draw_list->PathFillConvex(col);
   if (g.Style.TabBorderSize > 0.0f)
   {
      draw_list->PathLineTo(ImVec2(bb.Min.x + 0.5f, y2));
   draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding + 0.5f, y1 + rounding + 0.5f), rounding, 6, 9);
   draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding - 0.5f, y1 + rounding + 0.5f), rounding, 9, 12);
   draw_list->PathLineTo(ImVec2(bb.Max.x - 0.5f, y2));
         }
      // Render text label (with custom clipping) + Unsaved Document marker + Close Button logic
   // We tend to lock style.FramePadding for a given tab-bar, hence the 'frame_padding' parameter.
   bool ImGui::TabItemLabelAndCloseButton(ImDrawList* draw_list, const ImRect& bb, ImGuiTabItemFlags flags, ImVec2 frame_padding, const char* label, ImGuiID tab_id, ImGuiID close_button_id)
   {
      ImGuiContext& g = *GImGui;
   ImVec2 label_size = CalcTextSize(label, NULL, true);
   if (bb.GetWidth() <= 1.0f)
            // Render text label (with clipping + alpha gradient) + unsaved marker
   const char* TAB_UNSAVED_MARKER = "*";
   ImRect text_pixel_clip_bb(bb.Min.x + frame_padding.x, bb.Min.y + frame_padding.y, bb.Max.x - frame_padding.x, bb.Max.y);
   if (flags & ImGuiTabItemFlags_UnsavedDocument)
   {
      text_pixel_clip_bb.Max.x -= CalcTextSize(TAB_UNSAVED_MARKER, NULL, false).x;
   ImVec2 unsaved_marker_pos(ImMin(bb.Min.x + frame_padding.x + label_size.x + 2, text_pixel_clip_bb.Max.x), bb.Min.y + frame_padding.y + (float)(int)(-g.FontSize * 0.25f));
      }
            // Close Button
   // We are relying on a subtle and confusing distinction between 'hovered' and 'g.HoveredId' which happens because we are using ImGuiButtonFlags_AllowOverlapMode + SetItemAllowOverlap()
   //  'hovered' will be true when hovering the Tab but NOT when hovering the close button
   //  'g.HoveredId==id' will be true when hovering the Tab including when hovering the close button
   //  'g.ActiveId==close_button_id' will be true when we are holding on the close button, in which case both hovered booleans are false
   bool close_button_pressed = false;
   bool close_button_visible = false;
   if (close_button_id != 0)
      if (g.HoveredId == tab_id || g.HoveredId == close_button_id || g.ActiveId == close_button_id)
      if (close_button_visible)
   {
      ImGuiItemHoveredDataBackup last_item_backup;
   const float close_button_sz = g.FontSize * 0.5f;
   if (CloseButton(close_button_id, ImVec2(bb.Max.x - frame_padding.x - close_button_sz, bb.Min.y + frame_padding.y + close_button_sz), close_button_sz))
                  // Close with middle mouse button
   if (!(flags & ImGuiTabItemFlags_NoCloseWithMiddleMouseButton) && IsMouseClicked(2))
                        // Label with ellipsis
   // FIXME: This should be extracted into a helper but the use of text_pixel_clip_bb and !close_button_visible makes it tricky to abstract at the moment
   const char* label_display_end = FindRenderedTextEnd(label);
   if (label_size.x > text_ellipsis_clip_bb.GetWidth())
   {
      const int ellipsis_dot_count = 3;
   const float ellipsis_width = (1.0f + 1.0f) * ellipsis_dot_count - 1.0f;
   const char* label_end = NULL;
   float label_size_clipped_x = g.Font->CalcTextSizeA(g.FontSize, text_ellipsis_clip_bb.GetWidth() - ellipsis_width + 1.0f, 0.0f, label, label_display_end, &label_end).x;
   if (label_end == label && label_end < label_display_end)    // Always display at least 1 character if there's no room for character + ellipsis
   {
         label_end = label + ImTextCountUtf8BytesFromChar(label, label_display_end);
   }
   while (label_end > label && ImCharIsBlankA(label_end[-1])) // Trim trailing space
   {
         label_end--;
   }
            const float ellipsis_x = text_pixel_clip_bb.Min.x + label_size_clipped_x + 1.0f;
   if (!close_button_visible && ellipsis_x + ellipsis_width <= bb.Max.x)
      }
   else
   {
                     }
