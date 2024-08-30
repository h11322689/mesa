   /**************************************************************************
   *
   * Copyright 2013 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <assert.h>
   #include <string.h>
   #include <limits.h>
      #include <tizplatform.h>
      #include "vl/vl_winsys.h"
      #include "h264eoutport.h"
   #include "h264eoutport_decls.h"
   #include "vid_enc_common.h"
      /*
   * h264eoutport class
   */
      static void * h264e_outport_ctor(void * ap_obj, va_list * app)
   {
         }
      static void * h264e_outport_dtor(void * ap_obj)
   {
         }
      /*
   * from tiz_api
   */
      static OMX_ERRORTYPE h264e_outport_AllocateBuffer(const void * ap_obj, OMX_HANDLETYPE ap_hdl,
               {
               r = super_UseBuffer(typeOf(ap_obj, "h264eoutport"), ap_obj, ap_hdl,
         if (r)
            (*buf)->pBuffer = NULL;
   (*buf)->pOutputPortPrivate = CALLOC(1, sizeof(struct output_buf_private));
   if (!(*buf)->pOutputPortPrivate) {
      super_FreeBuffer(typeOf(ap_obj, "h264eoutport"), ap_obj, ap_hdl, idx, *buf);
                  }
      static OMX_ERRORTYPE h264e_outport_FreeBuffer(const void * ap_obj, OMX_HANDLETYPE ap_hdl,
         {
      vid_enc_PrivateType *priv = tiz_get_prc(ap_hdl);
            if (outp) {
      if (outp->transfer)
         pipe_resource_reference(&outp->bitstream, NULL);
   FREE(outp);
      }
               }
      /*
   * h264e_outport_class
   */
      static void * h264e_outport_class_ctor(void * ap_obj, va_list * app)
   {
      /* NOTE: Class methods might be added in the future. None for now. */
      }
      /*
   * initialization
   */
      void * h264e_outport_class_init(void * ap_tos, void * ap_hdl)
   {
      void * tizavcport = tiz_get_type(ap_hdl, "tizavcport");
   void * h264eoutport_class
   = factory_new (classOf(tizavcport), "h264eoutport_class",
                  }
      void * h264e_outport_init(void * ap_tos, void * ap_hdl)
   {
      void * tizavcport = tiz_get_type(ap_hdl, "tizavcport");
   void * h264eoutport_class = tiz_get_type(ap_hdl, "h264eoutport_class");
   void * h264eoutport = factory_new
   /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
   (h264eoutport_class, "h264eoutport", tizavcport,
      sizeof(h264e_outport_t),
   /* TIZ_CLASS_COMMENT: class constructor */
   ap_tos, ap_hdl,
   /* TIZ_CLASS_COMMENT: class constructor */
   ctor, h264e_outport_ctor,
   /* TIZ_CLASS_COMMENT: class destructor */
   dtor, h264e_outport_dtor,
   /* TIZ_CLASS_COMMENT: */
   tiz_api_AllocateBuffer, h264e_outport_AllocateBuffer,
   /* TIZ_CLASS_COMMENT: */
   tiz_api_FreeBuffer, h264e_outport_FreeBuffer,
   /* TIZ_CLASS_COMMENT: stop value*/
            }
