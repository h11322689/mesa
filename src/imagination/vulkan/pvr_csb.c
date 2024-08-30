   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * based in part on anv driver which is:
   * Copyright © 2015 Intel Corporation
   *
   * based in part on v3dv_cl.c which is:
   * Copyright © 2019 Raspberry Pi
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
      #include <assert.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_csb.h"
   #include "pvr_debug.h"
   #include "pvr_device_info.h"
   #include "pvr_private.h"
   #include "pvr_types.h"
   #include "util/list.h"
   #include "util/u_dynarray.h"
   #include "vk_log.h"
      /**
   * \file pvr_csb.c
   *
   * \brief Contains functions to manage Control Stream Builder (csb) object.
   *
   * A csb object can be used to create a primary/main control stream, referred
   * as control stream hereafter, or a secondary control stream, also referred as
   * a sub control stream. The main difference between these is that, the control
   * stream is the one directly submitted to the GPU and is terminated using
   * STREAM_TERMINATE. Whereas, the secondary control stream can be thought of as
   * an independent set of commands that can be referenced by a primary control
   * stream to avoid duplication and is instead terminated using STREAM_RETURN,
   * which means the control stream parser should return to the main stream it
   * came from.
   *
   * Note: Sub control stream is only supported for PVR_CMD_STREAM_TYPE_GRAPHICS
   * type control streams.
   */
      /**
   * \brief Initializes the csb object.
   *
   * \param[in] device Logical device pointer.
   * \param[in] csb    Control Stream Builder object to initialize.
   *
   * \sa #pvr_csb_finish()
   */
   void pvr_csb_init(struct pvr_device *device,
               {
      csb->start = NULL;
   csb->next = NULL;
   csb->pvr_bo = NULL;
   csb->end = NULL;
         #if defined(DEBUG)
         #endif
         csb->device = device;
   csb->stream_type = stream_type;
            if (stream_type == PVR_CMD_STREAM_TYPE_GRAPHICS_DEFERRED)
         else
      }
      /**
   * \brief Frees the resources associated with the csb object.
   *
   * \param[in] csb Control Stream Builder object to free.
   *
   * \sa #pvr_csb_init()
   */
   void pvr_csb_finish(struct pvr_csb *csb)
   {
   #if defined(DEBUG)
      assert(csb->relocation_mark_status ==
            #endif
         if (csb->stream_type == PVR_CMD_STREAM_TYPE_GRAPHICS_DEFERRED) {
         } else {
      list_for_each_entry_safe (struct pvr_bo, pvr_bo, &csb->pvr_bo_list, link) {
      list_del(&pvr_bo->link);
                  /* Leave the csb in a reset state to catch use after destroy instances */
      }
      /**
   * \brief Discard information only required while building and return the BOs.
   *
   * \param[in] csb Control Stream Builder object to bake.
   * \param[out] bo_list_out A list of \c pvr_bo containing the control stream.
   *
   * \return The last status value of \c csb.
   *
   * The value of \c bo_list_out is only defined iff this function returns
   * \c VK_SUCCESS. It is not allowed to call this function on a \c pvr_csb for
   * a deferred control stream type.
   *
   * The state of \c csb after calling this function (iff it returns
   * \c VK_SUCCESS) is identical to that after calling #pvr_csb_finish().
   * Unlike #pvr_csb_finish(), however, the caller must free every entry in
   * \c bo_list_out itself.
   */
   VkResult pvr_csb_bake(struct pvr_csb *const csb,
         {
               if (csb->status != VK_SUCCESS)
                     /* Same as pvr_csb_finish(). */
               }
      /**
   * \brief Adds VDMCTRL_STREAM_LINK/CDMCTRL_STREAM_LINK dwords into the control
   * stream pointed by csb object without setting a relocation mark.
   *
   * \warning This does not set the relocation mark.
   *
   * \param[in] csb  Control Stream Builder object to add LINK dwords to.
   * \param[in] addr Device virtual address of the sub control stream to link to.
   * \param[in] ret  Selects whether the sub control stream will return or
   *                 terminate.
   */
   static void
   pvr_csb_emit_link_unmarked(struct pvr_csb *csb, pvr_dev_addr_t addr, bool ret)
   {
      /* Not supported for deferred control stream. */
            /* Stream return is only supported for graphics control stream. */
            switch (csb->stream_type) {
   case PVR_CMD_STREAM_TYPE_GRAPHICS:
      pvr_csb_emit (csb, VDMCTRL_STREAM_LINK0, link) {
      link.link_addrmsb = addr;
               pvr_csb_emit (csb, VDMCTRL_STREAM_LINK1, link) {
                        case PVR_CMD_STREAM_TYPE_COMPUTE:
      pvr_csb_emit (csb, CDMCTRL_STREAM_LINK0, link) {
                  pvr_csb_emit (csb, CDMCTRL_STREAM_LINK1, link) {
                        default:
      unreachable("Unknown stream type");
         }
      /**
   * \brief Helper function to extend csb memory.
   *
   * Allocates a new buffer object and links it with the previous buffer object
   * using STREAM_LINK dwords and updates csb object to use the new buffer.
   *
   * To make sure that we have enough space to emit STREAM_LINK dwords in the
   * current buffer, a few bytes including guard padding size are reserved at the
   * end, every time a buffer is created. Every time we allocate a new buffer we
   * fix the current buffer in use to emit the stream link dwords. This makes sure
   * that when #pvr_csb_alloc_dwords() is called from #pvr_csb_emit() to add
   * STREAM_LINK0 and STREAM_LINK1, it succeeds without trying to allocate new
   * pages.
   *
   * \param[in] csb Control Stream Builder object to extend.
   * \return true on success and false otherwise.
   */
   static bool pvr_csb_buffer_extend(struct pvr_csb *csb)
   {
      const uint8_t stream_link_space =
      PVR_DW_TO_BYTES(pvr_cmd_length(VDMCTRL_STREAM_LINK0) +
      const uint8_t stream_reserved_space =
         const uint32_t cache_line_size =
         size_t current_state_update_size = 0;
   struct pvr_bo *pvr_bo;
            /* Make sure extra space allocated for stream links is sufficient for both
   * stream types.
   */
   STATIC_ASSERT((pvr_cmd_length(VDMCTRL_STREAM_LINK0) +
                        STATIC_ASSERT(PVRX(VDMCTRL_GUARD_SIZE_DEFAULT) ==
            result = pvr_bo_alloc(csb->device,
                        csb->device->heaps.general_heap,
      if (result != VK_SUCCESS) {
      vk_error(csb->device, result);
   csb->status = result;
               /* if this is not the first BO in csb */
   if (csb->pvr_bo) {
      bool zero_after_move = PVR_IS_DEBUG_SET(DUMP_CONTROL_STREAM);
            current_state_update_size =
            assert(csb->relocation_mark != NULL);
               #if defined(DEBUG)
         assert(csb->relocation_mark_status == PVR_CSB_RELOCATION_MARK_SET);
   csb->relocation_mark_status = PVR_CSB_RELOCATION_MARK_SET_AND_CONSUMED;
   #endif
            if (zero_after_move)
                     csb->end = (uint8_t *)csb->end + stream_link_space;
                        csb->pvr_bo = pvr_bo;
            /* Reserve space at the end, including the default guard padding, to make
   * sure we don't run out of space when a stream link is required.
   */
   csb->end = (uint8_t *)csb->start + pvr_bo->bo->size - stream_reserved_space;
                        }
      /**
   * \brief Provides a chunk of memory from the current csb buffer. In cases where
   * the buffer is not able to fulfill the required amount of memory,
   * #pvr_csb_buffer_extend() is called to allocate a new buffer. Maximum size
   * allocable in bytes is #PVR_CMD_BUFFER_CSB_BO_SIZE - size of STREAM_LINK0
   * and STREAM_LINK1 dwords.
   *
   * \param[in] csb        Control Stream Builder object to allocate from.
   * \param[in] num_dwords Number of dwords to allocate.
   * \return Valid host virtual address or NULL otherwise.
   */
   void *pvr_csb_alloc_dwords(struct pvr_csb *csb, uint32_t num_dwords)
   {
      const uint32_t required_space = PVR_DW_TO_BYTES(num_dwords);
            if (csb->status != VK_SUCCESS)
            if (csb->stream_type == PVR_CMD_STREAM_TYPE_GRAPHICS_DEFERRED) {
      p = util_dynarray_grow_bytes(&csb->deferred_cs_mem, 1, required_space);
   if (!p)
                     #if defined(DEBUG)
      if (csb->relocation_mark_status == PVR_CSB_RELOCATION_MARK_CLEARED)
      #endif
         if ((uint8_t *)csb->next + required_space > (uint8_t *)csb->end) {
      bool ret = pvr_csb_buffer_extend(csb);
   if (!ret)
                        csb->next = (uint8_t *)csb->next + required_space;
               }
      /**
   * \brief Copies control stream words from src csb into dst csb.
   *
   * The intended use is to copy PVR_CMD_STREAM_TYPE_GRAPHICS_DEFERRED type
   * control stream into PVR_CMD_STREAM_TYPE_GRAPHICS type device accessible
   * control stream for processing.
   *
   * This is mainly for secondary command buffers created with
   * VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag. In that case we need to
   * copy secondary control stream into the primary control stream for processing.
   * This is done as part of vkCmdExecuteCommands.
   *
   * We create deferred control stream which is basically the same control stream
   * but based in host side memory to avoid reserving device side resource.
   *
   * \param[in,out] csb_dst Destination control Stream Builder object.
   * \param[in]     csb_src Source Control Stream Builder object.
   */
   VkResult pvr_csb_copy(struct pvr_csb *csb_dst, struct pvr_csb *csb_src)
   {
      const uint8_t stream_reserved_space =
      PVR_DW_TO_BYTES(pvr_cmd_length(VDMCTRL_STREAM_LINK0) +
            const uint32_t size =
         const uint8_t *start = util_dynarray_begin(&csb_src->deferred_cs_mem);
            /* Only deferred control stream supported as src. */
            /* Only graphics control stream supported as dst. */
            if (size >= (PVR_CMD_BUFFER_CSB_BO_SIZE - stream_reserved_space)) {
      /* TODO: For now we don't support deferred streams bigger than one csb
   * buffer object size.
   *
   * While adding support for this make sure to not break the words/dwords
   * over two csb buffers.
   */
   pvr_finishme("Add support to copy streams bigger than one csb buffer");
               destination = pvr_csb_alloc_dwords(csb_dst, size);
   if (!destination) {
      assert(csb_dst->status != VK_SUCCESS);
                           }
      /**
   * \brief Adds VDMCTRL_STREAM_LINK/CDMCTRL_STREAM_LINK dwords into the control
   * stream pointed by csb object.
   *
   * \param[in] csb  Control Stream Builder object to add LINK dwords to.
   * \param[in] addr Device virtual address of the sub control stream to link to.
   * \param[in] ret  Selects whether the sub control stream will return or
   *                 terminate.
   */
   void pvr_csb_emit_link(struct pvr_csb *csb, pvr_dev_addr_t addr, bool ret)
   {
      pvr_csb_set_relocation_mark(csb);
   pvr_csb_emit_link_unmarked(csb, addr, ret);
      }
      /**
   * \brief Adds VDMCTRL_STREAM_RETURN dword into the control stream pointed by
   * csb object. Given a VDMCTRL_STREAM_RETURN marks the end of the sub control
   * stream, we return the status of the control stream as well.
   *
   * \param[in] csb Control Stream Builder object to add VDMCTRL_STREAM_RETURN to.
   * \return VK_SUCCESS on success, or error code otherwise.
   */
   VkResult pvr_csb_emit_return(struct pvr_csb *csb)
   {
      /* STREAM_RETURN is only supported by graphics control stream. */
   assert(csb->stream_type == PVR_CMD_STREAM_TYPE_GRAPHICS ||
            pvr_csb_set_relocation_mark(csb);
   /* clang-format off */
   pvr_csb_emit(csb, VDMCTRL_STREAM_RETURN, ret);
   /* clang-format on */
               }
      /**
   * \brief Adds STREAM_TERMINATE dword into the control stream pointed by csb
   * object. Given a STREAM_TERMINATE marks the end of the control stream, we
   * return the status of the control stream as well.
   *
   * \param[in] csb Control Stream Builder object to terminate.
   * \return VK_SUCCESS on success, or error code otherwise.
   */
   VkResult pvr_csb_emit_terminate(struct pvr_csb *csb)
   {
               switch (csb->stream_type) {
   case PVR_CMD_STREAM_TYPE_GRAPHICS:
      /* clang-format off */
   pvr_csb_emit(csb, VDMCTRL_STREAM_TERMINATE, terminate);
   /* clang-format on */
         case PVR_CMD_STREAM_TYPE_COMPUTE:
      /* clang-format off */
   pvr_csb_emit(csb, CDMCTRL_STREAM_TERMINATE, terminate);
   /* clang-format on */
         default:
      unreachable("Unknown stream type");
                           }
