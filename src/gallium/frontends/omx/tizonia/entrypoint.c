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
      #include <tizplatform.h>
   #include <tizkernel.h>
   #include <tizscheduler.h>
   #include <tizport.h>
   #include <tizport_decls.h>
   #include <tizvideoport.h>
   #include <tizvideoport_decls.h>
      #include "vid_dec_h264_common.h"
   #include "entrypoint.h"
   #include "h264d.h"
   #include "h264dprc.h"
   #include "h264dinport.h"
   #include "h264e.h"
   #include "h264eprc.h"
   #include "h264einport.h"
   #include "h264eoutport.h"
   #include "names.h"
      #include "util/u_debug.h"
      DEBUG_GET_ONCE_BOOL_OPTION(mesa_enable_omx_eglimage,
                  static OMX_BOOL egl_image_validation_hook(const OMX_HANDLETYPE ap_hdl,
               {
      const void * p_krn = NULL;
            assert(ap_hdl);
   assert(ap_eglimage);
            if (!debug_get_option_mesa_enable_omx_eglimage()) {
                  p_krn = tiz_get_krn(ap_hdl);
            const OMX_VIDEO_PORTDEFINITIONTYPE * p_video_portdef
            if (!p_video_portdef->pNativeWindow) {
                     }
      OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
   {
      tiz_role_factory_t h264d_role;
   tiz_role_factory_t h264e_role;
   const tiz_role_factory_t * rf_list[] = {&h264d_role, &h264e_role};
   tiz_type_factory_t h264dprc_type;
   tiz_type_factory_t h264d_inport_type;
   tiz_type_factory_t h264eprc_type;
   tiz_type_factory_t h264e_inport_type;
   tiz_type_factory_t h264e_outport_type;
   const tiz_type_factory_t * tf_list[] = {&h264dprc_type,
                           const tiz_eglimage_hook_t egl_validation_hook = {
      OMX_VID_DEC_AVC_OUTPUT_PORT_INDEX,
   egl_image_validation_hook,
               /* Settings for roles */
   strcpy ((OMX_STRING) h264d_role.role, OMX_VID_DEC_AVC_ROLE);
   h264d_role.pf_cport = instantiate_h264d_config_port;
   h264d_role.pf_port[0] = instantiate_h264d_input_port;
   h264d_role.pf_port[1] = instantiate_h264d_output_port;
   h264d_role.nports = 2;
            strcpy ((OMX_STRING) h264e_role.role, OMX_VID_ENC_AVC_ROLE);
   h264e_role.pf_cport = instantiate_h264e_config_port;
   h264e_role.pf_port[0] = instantiate_h264e_input_port;
   h264e_role.pf_port[1] = instantiate_h264e_output_port;
   h264e_role.nports = 2;
            /* Settings for classes */
   strcpy ((OMX_STRING) h264dprc_type.class_name, "h264dprc_class");
   h264dprc_type.pf_class_init = h264d_prc_class_init;
   strcpy ((OMX_STRING) h264dprc_type.object_name, "h264dprc");
            strcpy ((OMX_STRING) h264d_inport_type.class_name, "h264dinport_class");
   h264d_inport_type.pf_class_init = h264d_inport_class_init;
   strcpy ((OMX_STRING) h264d_inport_type.object_name, "h264dinport");
            strcpy ((OMX_STRING) h264eprc_type.class_name, "h264eprc_class");
   h264eprc_type.pf_class_init = h264e_prc_class_init;
   strcpy ((OMX_STRING) h264eprc_type.object_name, "h264eprc");
            strcpy ((OMX_STRING) h264e_inport_type.class_name, "h264einport_class");
   h264e_inport_type.pf_class_init = h264e_inport_class_init;
   strcpy ((OMX_STRING) h264e_inport_type.object_name, "h264einport");
            strcpy ((OMX_STRING) h264e_outport_type.class_name, "h264eoutport_class");
   h264e_outport_type.pf_class_init = h264e_outport_class_init;
   strcpy ((OMX_STRING) h264e_outport_type.object_name, "h264eoutport");
            /* Initialize the component infrastructure */
            /* Classes need to be registered first */
            /* Register the component roles */
            /* Register egl image validation hook for the decoder */
   tiz_check_omx (tiz_comp_register_role_eglimage_hook
                     }
