   #include "nouveau_context.h"
      #include "nouveau_device.h"
      #include "drm-uapi/nouveau_drm.h"
      #include <errno.h>
   #include <nouveau/nvif/ioctl.h>
   #include <xf86drm.h>
      static void
   nouveau_ws_subchan_dealloc(int fd, struct nouveau_ws_object *obj)
   {
      struct {
      struct nvif_ioctl_v0 ioctl;
      } args = {
      .ioctl = {
      .object = (uintptr_t)obj,
   .owner = NVIF_IOCTL_V0_OWNER_ANY,
   .route = 0x00,
   .type = NVIF_IOCTL_V0_DEL,
                  /* TODO returns -ENOENT for unknown reasons */
      }
      #define NOUVEAU_WS_CONTEXT_MAX_CLASSES 16
      static int
   nouveau_ws_context_query_classes(int fd, int channel, uint32_t classes[NOUVEAU_WS_CONTEXT_MAX_CLASSES])
   {
      struct {
      struct nvif_ioctl_v0 ioctl;
   struct nvif_ioctl_sclass_v0 sclass;
      } args = {
      .ioctl = {
      .route = 0xff,
   .token = channel,
   .type = NVIF_IOCTL_V0_SCLASS,
      },
   .sclass = {
      .count = NOUVEAU_WS_CONTEXT_MAX_CLASSES,
                  int ret = drmCommandWriteRead(fd, DRM_NOUVEAU_NVIF, &args, sizeof(args));
   if (ret)
            assert(args.sclass.count <= NOUVEAU_WS_CONTEXT_MAX_CLASSES);
   for (unsigned i = 0; i < NOUVEAU_WS_CONTEXT_MAX_CLASSES; i++)
               }
      static uint32_t
   nouveau_ws_context_find_class(uint32_t classes[NOUVEAU_WS_CONTEXT_MAX_CLASSES], uint8_t type)
   {
               /* find the highest matching one */
   for (unsigned i = 0; i < NOUVEAU_WS_CONTEXT_MAX_CLASSES; i++) {
      uint32_t val = classes[i];
   if ((val & 0xff) == type)
                  }
      static int
   nouveau_ws_subchan_alloc(int fd, int channel, uint32_t handle, uint16_t oclass, struct nouveau_ws_object *obj)
   {
      struct {
      struct nvif_ioctl_v0 ioctl;
      } args = {
      .ioctl = {
      .route = 0xff,
   .token = channel,
   .type = NVIF_IOCTL_V0_NEW,
      },
   .new = {
      .handle = handle,
   .object = (uintptr_t)obj,
   .oclass = oclass,
   .route = NVIF_IOCTL_V0_ROUTE_NVIF,
   .token = (uintptr_t)obj,
                  if (!oclass) {
      assert(!"called with invalid oclass");
                           }
      static void
   nouveau_ws_channel_dealloc(int fd, int channel)
   {
      struct drm_nouveau_channel_free req = {
                  int ret = drmCommandWrite(fd, DRM_NOUVEAU_CHANNEL_FREE, &req, sizeof(req));
      }
      int
   nouveau_ws_context_create(struct nouveau_ws_device *dev, struct nouveau_ws_context **out)
   {
      struct drm_nouveau_channel_alloc req = { };
   uint32_t classes[NOUVEAU_WS_CONTEXT_MAX_CLASSES];
            *out = CALLOC_STRUCT(nouveau_ws_context);
   if (!*out)
            int ret = drmCommandWriteRead(dev->fd, DRM_NOUVEAU_CHANNEL_ALLOC, &req, sizeof(req));
   if (ret)
            ret = nouveau_ws_context_query_classes(dev->fd, req.channel, classes);
   if (ret)
            base = (0xbeef + req.channel) << 16;
   uint32_t obj_class = nouveau_ws_context_find_class(classes, 0x2d);
   ret = nouveau_ws_subchan_alloc(dev->fd, req.channel, base | 0x902d, obj_class, &(*out)->eng2d);
   if (ret)
            obj_class = nouveau_ws_context_find_class(classes, 0x40);
   if (!obj_class)
         ret = nouveau_ws_subchan_alloc(dev->fd, req.channel, base | 0x323f, obj_class, &(*out)->m2mf);
   if (ret)
            obj_class = nouveau_ws_context_find_class(classes, 0xb5);
   ret = nouveau_ws_subchan_alloc(dev->fd, req.channel, 0, obj_class, &(*out)->copy);
   if (ret)
            obj_class = nouveau_ws_context_find_class(classes, 0x97);
   ret = nouveau_ws_subchan_alloc(dev->fd, req.channel, base | 0x003d, obj_class, &(*out)->eng3d);
   if (ret)
            obj_class = nouveau_ws_context_find_class(classes, 0xc0);
   ret = nouveau_ws_subchan_alloc(dev->fd, req.channel, base | 0x00c0, obj_class, &(*out)->compute);
   if (ret)
            (*out)->channel = req.channel;
   (*out)->dev = dev;
         fail_subchan:
      nouveau_ws_subchan_dealloc(dev->fd, &(*out)->compute);
   nouveau_ws_subchan_dealloc(dev->fd, &(*out)->eng3d);
   nouveau_ws_subchan_dealloc(dev->fd, &(*out)->copy);
   nouveau_ws_subchan_dealloc(dev->fd, &(*out)->m2mf);
      fail_2d:
         fail_chan:
      FREE(*out);
      }
      void
   nouveau_ws_context_destroy(struct nouveau_ws_context *context)
   {
      nouveau_ws_subchan_dealloc(context->dev->fd, &context->compute);
   nouveau_ws_subchan_dealloc(context->dev->fd, &context->eng3d);
   nouveau_ws_subchan_dealloc(context->dev->fd, &context->copy);
   nouveau_ws_subchan_dealloc(context->dev->fd, &context->m2mf);
   nouveau_ws_subchan_dealloc(context->dev->fd, &context->eng2d);
   nouveau_ws_channel_dealloc(context->dev->fd, context->channel);
      }
      bool
   nouveau_ws_context_killed(struct nouveau_ws_context *context)
   {
      /* we are using the normal pushbuf submission ioctl as this is how nouveau implemented this on
   * the kernel side.
   * And as long as we submit nothing (e.g. nr_push is 0) it's more or less a noop on the kernel
   * side.
   */
   struct drm_nouveau_gem_pushbuf req = {
         };
   int ret = drmCommandWriteRead(context->dev->fd, DRM_NOUVEAU_GEM_PUSHBUF, &req, sizeof(req));
   /* nouveau returns ENODEV once the channel was killed */
      }
