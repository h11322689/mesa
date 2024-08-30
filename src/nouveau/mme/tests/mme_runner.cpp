   /*
   * Copyright © 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
   #include "mme_runner.h"
      #include <fcntl.h>
   #include <string.h>
   #include <xf86drm.h>
      #include "mme_fermi_sim.h"
   #include "mme_tu104_sim.h"
      #include "nvk_clc597.h"
      #include "nouveau_bo.h"
   #include "nouveau_context.h"
      /* nouveau_drm.h isn't C++-friendly */
   #define class cls
   #include "drm-uapi/nouveau_drm.h"
   #undef class
      mme_runner::mme_runner() :
   devinfo(NULL), data_addr(0), data(NULL)
   { }
      mme_runner::~mme_runner()
   { }
      mme_hw_runner::mme_hw_runner() :
   mme_runner(), p(NULL), dev(NULL), ctx(NULL),
   data_bo(NULL), push_bo(NULL),
   syncobj(0),
   push_map(NULL)
   {
         }
      void
   mme_runner::mme_store_data(mme_builder *b, uint32_t dw_idx,
         {
         }
      mme_hw_runner::~mme_hw_runner()
   {
      if (syncobj)
         if (push_bo) {
      nouveau_ws_bo_unmap(push_bo, push_map);
      }
   if (ctx)
         if (dev)
      }
      #define PUSH_SIZE 64 * 4096
      bool
   mme_hw_runner::set_up_hw(uint16_t min_cls, uint16_t max_cls)
   {
      drmDevicePtr devices[8];
            int i;
   for (i = 0; i < max_devices; i++) {
      if (devices[i]->available_nodes & 1 << DRM_NODE_RENDER &&
      devices[i]->bustype == DRM_BUS_PCI &&
   devices[i]->deviceinfo.pci->vendor_id == 0x10de) {
   dev = nouveau_ws_device_new(devices[i]);
                  if (dev->info.cls_eng3d < min_cls || dev->info.cls_eng3d > max_cls) {
      nouveau_ws_device_destroy(dev);
   dev = NULL;
               /* Found a Turning+ device */
                  if (dev == NULL)
                     int ret = nouveau_ws_context_create(dev, &ctx);
   if (ret)
            uint32_t data_bo_flags = NOUVEAU_WS_BO_GART | NOUVEAU_WS_BO_MAP;
   data_bo = nouveau_ws_bo_new_mapped(dev, DATA_BO_SIZE, 0,
               if (data_bo == NULL)
            memset(data, 139, DATA_BO_SIZE);
            uint32_t push_bo_flags = NOUVEAU_WS_BO_GART | NOUVEAU_WS_BO_MAP;
   push_bo = nouveau_ws_bo_new_mapped(dev, PUSH_SIZE, 0,
               if (push_bo == NULL)
            ret = drmSyncobjCreate(dev->fd, 0, &syncobj);
   if (ret < 0)
                        }
      void
   mme_hw_runner::reset_push()
   {
      nv_push_init(&push, (uint32_t *)push_map, PUSH_SIZE / 4);
            P_MTHD(p, NV9097, SET_OBJECT);
   P_NV9097_SET_OBJECT(p, {
      .class_id = dev->info.cls_eng3d,
         }
      void
   mme_hw_runner::submit_push()
   {
      struct drm_nouveau_exec_push push = {
      .va = push_bo->offset,
               struct drm_nouveau_sync sync = {
      .flags = DRM_NOUVEAU_SYNC_SYNCOBJ,
   .handle = syncobj,
               struct drm_nouveau_exec req = {
      .channel = (uint32_t)ctx->channel,
   .push_count = 1,
   .sig_count = 1,
   .sig_ptr = (uintptr_t)&sync,
               int ret = drmCommandWriteRead(dev->fd, DRM_NOUVEAU_EXEC,
                  ret = drmSyncobjWait(dev->fd, &syncobj, 1, INT64_MAX,
            }
      void
   mme_hw_runner::push_macro(uint32_t id, const std::vector<uint32_t> &macro)
   {
      P_MTHD(p, NV9097, LOAD_MME_START_ADDRESS_RAM_POINTER);
   P_NV9097_LOAD_MME_START_ADDRESS_RAM_POINTER(p, id);
   P_NV9097_LOAD_MME_START_ADDRESS_RAM(p, 0);
   P_1INC(p, NV9097, LOAD_MME_INSTRUCTION_RAM_POINTER);
   P_NV9097_LOAD_MME_INSTRUCTION_RAM_POINTER(p, 0);
      }
      void
   mme_hw_runner::run_macro(const std::vector<uint32_t>& macro,
         {
               P_1INC(p, NV9097, CALL_MME_MACRO(0));
   if (params.empty()) {
         } else {
                     }
      mme_fermi_sim_runner::mme_fermi_sim_runner(uint64_t data_addr)
   {
      memset(&info, 0, sizeof(info));
                     this->devinfo = &info;
   this->data_addr = data_addr,
      }
      mme_fermi_sim_runner::~mme_fermi_sim_runner()
   { }
      void
   mme_fermi_sim_runner::run_macro(const std::vector<uint32_t>& macro,
         {
      std::vector<mme_fermi_inst> insts(macro.size());
            /* First, make a copy of the data and simulate the macro */
   mme_fermi_sim_mem sim_mem = {
      .addr = data_addr,
   .data = data,
      };
   mme_fermi_sim(insts.size(), &insts[0],
            }
      mme_tu104_sim_runner::mme_tu104_sim_runner(uint64_t data_addr)
   {
      memset(&info, 0, sizeof(info));
                     this->devinfo = &info;
   this->data_addr = data_addr,
      }
      mme_tu104_sim_runner::~mme_tu104_sim_runner()
   { }
      void
   mme_tu104_sim_runner::run_macro(const std::vector<uint32_t>& macro,
         {
      std::vector<mme_tu104_inst> insts(macro.size());
            /* First, make a copy of the data and simulate the macro */
   mme_tu104_sim_mem sim_mem = {
      .addr = data_addr,
   .data = data,
      };
   mme_tu104_sim(insts.size(), &insts[0],
            }
