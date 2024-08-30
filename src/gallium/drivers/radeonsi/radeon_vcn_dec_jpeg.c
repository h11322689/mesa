   /**************************************************************************
   *
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "pipe/p_video_codec.h"
   #include "radeon_vcn_dec.h"
   #include "radeon_video.h"
   #include "radeonsi/si_pipe.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
      #include <assert.h>
   #include <stdio.h>
      static struct pb_buffer *radeon_jpeg_get_decode_param(struct radeon_decoder *dec,
               {
      struct si_texture *luma = (struct si_texture *)((struct vl_video_buffer *)target)->resources[0];
            dec->jpg.bsd_size = align(dec->bs_size, 128);
   dec->jpg.dt_luma_top_offset = luma->surface.u.gfx9.surf_offset;
   dec->jpg.dt_chroma_top_offset = 0;
            switch (target->buffer_format) {
      case PIPE_FORMAT_IYUV:
   case PIPE_FORMAT_YV12:
   case PIPE_FORMAT_Y8_U8_V8_444_UNORM:
   case PIPE_FORMAT_R8_G8_B8_UNORM:
      chromav = (struct si_texture *)((struct vl_video_buffer *)target)->resources[2];
   dec->jpg.dt_chromav_top_offset = chromav->surface.u.gfx9.surf_offset;
   chroma = (struct si_texture *)((struct vl_video_buffer*)target)->resources[1];
   dec->jpg.dt_chroma_top_offset = chroma->surface.u.gfx9.surf_offset;
      case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_P010:
   case PIPE_FORMAT_P016:
      chroma = (struct si_texture *)((struct vl_video_buffer*)target)->resources[1];
   dec->jpg.dt_chroma_top_offset = chroma->surface.u.gfx9.surf_offset;
      default:
      }
   dec->jpg.dt_pitch = luma->surface.u.gfx9.surf_pitch * luma->surface.blk_w;
               }
      /* add a new set register command to the IB */
   static void set_reg_jpeg(struct radeon_decoder *dec, unsigned reg, unsigned cond, unsigned type,
         {
      radeon_emit(&dec->jcs[dec->cb_idx], RDECODE_PKTJ(reg, cond, type));
      }
      /* send a bitstream buffer command */
   static void send_cmd_bitstream(struct radeon_decoder *dec, struct pb_buffer *buf, uint32_t off,
         {
               // jpeg soft reset
            // ensuring the Reset is asserted in SCLK domain
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C2);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, 0x01400200);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C3);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, (1 << 9));
            // wait mem
            // ensuring the Reset is de-asserted in SCLK domain
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C3);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, (0 << 9));
            dec->ws->cs_add_buffer(&dec->jcs[dec->cb_idx], buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   addr = dec->ws->buffer_get_virtual_address(buf);
            // set UVD_LMI_JPEG_READ_64BIT_BAR_LOW/HIGH based on bitstream buffer address
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_LMI_JPEG_READ_64BIT_BAR_HIGH), COND0, TYPE0,
                  // set jpeg_rb_base
            // set jpeg_rb_base
            // set jpeg_rb_wptr
      }
      /* send a target buffer command */
   static void send_cmd_target(struct radeon_decoder *dec, struct pb_buffer *buf, uint32_t off,
         {
               set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_PITCH), COND0, TYPE0, (dec->jpg.dt_pitch >> 4));
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_UV_PITCH), COND0, TYPE0,
            set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_TILING_CTRL), COND0, TYPE0, 0);
            dec->ws->cs_add_buffer(&dec->jcs[dec->cb_idx], buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   addr = dec->ws->buffer_get_virtual_address(buf);
            // set UVD_LMI_JPEG_WRITE_64BIT_BAR_LOW/HIGH based on target buffer address
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_LMI_JPEG_WRITE_64BIT_BAR_HIGH), COND0, TYPE0,
                  // set output buffer data address
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_INDEX), COND0, TYPE0, 0);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_DATA), COND0, TYPE0, dec->jpg.dt_luma_top_offset);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_INDEX), COND0, TYPE0, 1);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_JPEG_DATA), COND0, TYPE0, dec->jpg.dt_chroma_top_offset);
            // set output buffer read pointer
            // enable error interrupts
            // start engine command
            // wait for job completion, wait for job JBSI fetch done
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C3);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, (dec->jpg.bsd_size >> 2));
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C2);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, 0x01400200);
            // wait for job jpeg outbuf idle
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C3);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, 0xFFFFFFFF);
            // stop engine
            // asserting jpeg lmi drop
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x0005);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, (1 << 23 | 1 << 0));
            // asserting jpeg reset
            // ensure reset is asserted in sclk domain
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C3);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, (1 << 9));
            // de-assert jpeg reset
            // ensure reset is de-asserted in sclk domain
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x01C3);
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_DATA), COND0, TYPE0, (0 << 9));
            // de-asserting jpeg lmi drop
   set_reg_jpeg(dec, SOC15_REG_ADDR(mmUVD_CTX_INDEX), COND0, TYPE0, 0x0005);
      }
      /* send a bitstream buffer command */
   static void send_cmd_bitstream_direct(struct radeon_decoder *dec, struct pb_buffer *buf,
               {
               // jpeg soft reset
            // ensuring the Reset is asserted in SCLK domain
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_cond_rd_timer, COND0, TYPE0, 0x01400200);
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_ref_data, COND0, TYPE0, (0x1 << 0x10));
            // wait mem
            // ensuring the Reset is de-asserted in SCLK domain
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_ref_data, COND0, TYPE0, (0 << 0x10));
            dec->ws->cs_add_buffer(&dec->jcs[dec->cb_idx], buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   addr = dec->ws->buffer_get_virtual_address(buf);
            // set UVD_LMI_JPEG_READ_64BIT_BAR_LOW/HIGH based on bitstream buffer address
   set_reg_jpeg(dec, dec->jpg_reg.lmi_jpeg_read_64bit_bar_high, COND0, TYPE0, (addr >> 32));
            // set jpeg_rb_base
            // set jpeg_rb_base
            // set jpeg_rb_wptr
      }
      /* send a target buffer command */
   static void send_cmd_target_direct(struct radeon_decoder *dec, struct pb_buffer *buf, uint32_t off,
               {
      uint64_t addr;
   uint32_t val;
   bool format_convert = false;
            switch (buffer_format) {
      case PIPE_FORMAT_R8G8B8A8_UNORM:
      format_convert = true;
   fc_sps_info_val = 1 | (1 << 4) | (0xff << 8);
      case PIPE_FORMAT_A8R8G8B8_UNORM:
      format_convert = true;
   fc_sps_info_val = 1 | (1 << 4) | (1 << 5) | (0xff << 8);
      case PIPE_FORMAT_R8_G8_B8_UNORM:
      format_convert = true;
   fc_sps_info_val = 1 | (1 << 5) | (0xff << 8);
      default:
               if (dec->jpg_reg.version == RDECODE_JPEG_REG_VER_V3 && format_convert) {
      set_reg_jpeg(dec, dec->jpg_reg.jpeg_pitch, COND0, TYPE0, dec->jpg.dt_pitch);
      } else {
      set_reg_jpeg(dec, dec->jpg_reg.jpeg_pitch, COND0, TYPE0, (dec->jpg.dt_pitch >> 4));
               set_reg_jpeg(dec, dec->jpg_reg.dec_addr_mode, COND0, TYPE0, 0);
   set_reg_jpeg(dec, dec->jpg_reg.dec_y_gfx10_tiling_surface, COND0, TYPE0, 0);
            dec->ws->cs_add_buffer(&dec->jcs[dec->cb_idx], buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   addr = dec->ws->buffer_get_virtual_address(buf);
            // set UVD_LMI_JPEG_WRITE_64BIT_BAR_LOW/HIGH based on target buffer address
   set_reg_jpeg(dec, dec->jpg_reg.lmi_jpeg_write_64bit_bar_high, COND0, TYPE0, (addr >> 32));
            // set output buffer data address
   if (dec->jpg_reg.version == RDECODE_JPEG_REG_VER_V2) {
      set_reg_jpeg(dec, dec->jpg_reg.jpeg_index, COND0, TYPE0, 0);
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_data, COND0, TYPE0, dec->jpg.dt_luma_top_offset);
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_index, COND0, TYPE0, 1);
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_data, COND0, TYPE0, dec->jpg.dt_chroma_top_offset);
   if (dec->jpg.dt_chromav_top_offset) {
      set_reg_jpeg(dec, dec->jpg_reg.jpeg_index, COND0, TYPE0, 2);
         } else {
      set_reg_jpeg(dec, dec->jpg_reg.jpeg_luma_base0_0, COND0, TYPE0, dec->jpg.dt_luma_top_offset);
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_chroma_base0_0, COND0, TYPE0, dec->jpg.dt_chroma_top_offset);
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_chromav_base0_0, COND0, TYPE0, dec->jpg.dt_chromav_top_offset);
   if (dec->jpg.crop_width && dec->jpg.crop_height) {
      set_reg_jpeg(dec, vcnipUVD_JPEG_ROI_CROP_POS_START, COND0, TYPE0,
         set_reg_jpeg(dec, vcnipUVD_JPEG_ROI_CROP_POS_STRIDE, COND0, TYPE0,
      } else {
      set_reg_jpeg(dec, vcnipUVD_JPEG_ROI_CROP_POS_START, COND0, TYPE0,
         set_reg_jpeg(dec, vcnipUVD_JPEG_ROI_CROP_POS_STRIDE, COND0, TYPE0,
      }
   if (format_convert) {
      /* set fc timeout control */
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_TMEOUT_CNT, COND0, TYPE0,(4244373504));
   /* set alpha position and packed format */
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_SPS_INFO, COND0, TYPE0, fc_sps_info_val);
   /* coefs */
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_R_COEF, COND0, TYPE0, 256 | (0 << 10) | (403 << 20));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_G_COEF, COND0, TYPE0, 256 | (976 << 10) | (904 << 20));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_B_COEF, COND0, TYPE0, 256 | (475 << 10) | (0 << 20));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_VUP_COEF_CNTL0, COND0, TYPE0, 128 | (384 << 16));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_VUP_COEF_CNTL1, COND0, TYPE0, 384 | (128 << 16));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_VUP_COEF_CNTL2, COND0, TYPE0, 128 | (384 << 16));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_VUP_COEF_CNTL3, COND0, TYPE0, 384 | (128 << 16));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_HUP_COEF_CNTL0, COND0, TYPE0, 128 | (384 << 16));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_HUP_COEF_CNTL1, COND0, TYPE0, 384 | (128 << 16));
   set_reg_jpeg(dec, vcnipUVD_JPEG_FC_HUP_COEF_CNTL2, COND0, TYPE0, 128 | (384 << 16));
      } else
      }
            // set output buffer read pointer
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_outbuf_rptr, COND0, TYPE0, 0);
   set_reg_jpeg(dec, dec->jpg_reg.jpeg_outbuf_cntl, COND0, TYPE0,
            // enable error interrupts
            // start engine command
   val = 0x6;
   if (dec->jpg_reg.version == RDECODE_JPEG_REG_VER_V3) {
      if (dec->jpg.crop_width && dec->jpg.crop_height)
         if (format_convert)
      }
            // wait for job completion, wait for job JBSI fetch done
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_ref_data, COND0, TYPE0, (dec->jpg.bsd_size >> 2));
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_cond_rd_timer, COND0, TYPE0, 0x01400200);
            // wait for job jpeg outbuf idle
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_ref_data, COND0, TYPE0, 0xFFFFFFFF);
            if (dec->jpg_reg.version == RDECODE_JPEG_REG_VER_V3 && format_convert) {
      val = val | (0x7 << 16);
   set_reg_jpeg(dec, dec->jpg_reg.jrbc_ib_ref_data, COND0, TYPE0, 0);
               // stop engine
      }
      /**
   * send cmd for vcn jpeg
   */
   void send_cmd_jpeg(struct radeon_decoder *dec, struct pipe_video_buffer *target,
         {
      struct pb_buffer *dt;
                     memset(dec->bs_ptr, 0, align(dec->bs_size, 128) - dec->bs_size);
   dec->ws->buffer_unmap(dec->ws, bs_buf->res->buf);
                     if (dec->jpg_reg.version == RDECODE_JPEG_REG_VER_V1) {
      send_cmd_bitstream(dec, bs_buf->res->buf, 0, RADEON_USAGE_READ, RADEON_DOMAIN_GTT);
      } else {
      send_cmd_bitstream_direct(dec, bs_buf->res->buf, 0, RADEON_USAGE_READ, RADEON_DOMAIN_GTT);
         }
