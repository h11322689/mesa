   #include "util/blob.h"
   #include "nv50_ir_driver.h"
   #include "nv50_ir.h"
   #include "nv50_ir_target.h"
   #include "nv50_ir_driver.h"
   #include "compiler/nir/nir_serialize.h"
      enum FixupApplyFunc {
      APPLY_NV50,
   APPLY_NVC0,
   APPLY_GK110,
   APPLY_GM107,
   APPLY_GV100,
   FLIP_NVC0,
   FLIP_GK110,
   FLIP_GM107,
      };
      extern bool
   nv50_ir_prog_info_serialize(struct blob *blob, struct nv50_ir_prog_info *info)
   {
      blob_write_uint32(blob, info->bin.smemSize);
   blob_write_uint16(blob, info->target);
   blob_write_uint8(blob, info->type);
   blob_write_uint8(blob, info->optLevel);
   blob_write_uint8(blob, info->dbgFlags);
                     if (info->type == PIPE_SHADER_COMPUTE)
                        }
      extern bool
   nv50_ir_prog_info_out_serialize(struct blob *blob,
         {
      blob_write_uint16(blob, info_out->target);
   blob_write_uint8(blob, info_out->type);
            blob_write_uint16(blob, info_out->bin.maxGPR);
   blob_write_uint32(blob, info_out->bin.tlsSpace);
   blob_write_uint32(blob, info_out->bin.smemSize);
   blob_write_uint32(blob, info_out->bin.codeSize);
   blob_write_bytes(blob, info_out->bin.code, info_out->bin.codeSize);
            if (!info_out->bin.relocData) {
         } else {
      nv50_ir::RelocInfo *reloc = (nv50_ir::RelocInfo *)info_out->bin.relocData;
   blob_write_uint32(blob, reloc->count);
   blob_write_uint32(blob, reloc->codePos);
   blob_write_uint32(blob, reloc->libPos);
   blob_write_uint32(blob, reloc->dataPos);
               if (!info_out->bin.fixupData) {
         } else {
      nv50_ir::FixupInfo *fixup = (nv50_ir::FixupInfo *)info_out->bin.fixupData;
            /* Going through each entry */
   for (uint32_t i = 0; i < fixup->count; i++) {
      blob_write_uint32(blob, fixup->entry[i].val);
   assert(fixup->entry[i].apply);
   /* Compare function pointers, for when at serializing
   * to know which function to apply */
   if (fixup->entry[i].apply == nv50_ir::nv50_interpApply)
         else if (fixup->entry[i].apply == nv50_ir::nvc0_interpApply)
         else if (fixup->entry[i].apply == nv50_ir::gk110_interpApply)
         else if (fixup->entry[i].apply == nv50_ir::gm107_interpApply)
         else if (fixup->entry[i].apply == nv50_ir::gv100_interpApply)
         else if (fixup->entry[i].apply == nv50_ir::nvc0_selpFlip)
         else if (fixup->entry[i].apply == nv50_ir::gk110_selpFlip)
         else if (fixup->entry[i].apply == nv50_ir::gm107_selpFlip)
         else if (fixup->entry[i].apply == nv50_ir::gv100_selpFlip)
         else {
      ERROR("unhandled fixup apply function pointer\n");
   assert(false);
                     blob_write_uint8(blob, info_out->numInputs);
   blob_write_uint8(blob, info_out->numOutputs);
   blob_write_uint8(blob, info_out->numSysVals);
   blob_write_bytes(blob, info_out->sv, info_out->numSysVals * sizeof(info_out->sv[0]));
   blob_write_bytes(blob, info_out->in, info_out->numInputs * sizeof(info_out->in[0]));
            switch(info_out->type) {
      case PIPE_SHADER_VERTEX:
      blob_write_bytes(blob, &info_out->prop.vp, sizeof(info_out->prop.vp));
      case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      blob_write_bytes(blob, &info_out->prop.tp, sizeof(info_out->prop.tp));
      case PIPE_SHADER_GEOMETRY:
      blob_write_bytes(blob, &info_out->prop.gp, sizeof(info_out->prop.gp));
      case PIPE_SHADER_FRAGMENT:
      blob_write_bytes(blob, &info_out->prop.fp, sizeof(info_out->prop.fp));
      case PIPE_SHADER_COMPUTE:
      blob_write_bytes(blob, &info_out->prop.cp, sizeof(info_out->prop.cp));
      default:
      }
   blob_write_bytes(blob, &info_out->io, sizeof(info_out->io));
               }
      extern bool
   nv50_ir_prog_info_out_deserialize(void *data, size_t size, size_t offset,
         {
      struct blob_reader reader;
   blob_reader_init(&reader, data, size);
            info_out->target = blob_read_uint16(&reader);
   info_out->type = blob_read_uint8(&reader);
            info_out->bin.maxGPR = blob_read_uint16(&reader);
   info_out->bin.tlsSpace = blob_read_uint32(&reader);
   info_out->bin.smemSize = blob_read_uint32(&reader);
   info_out->bin.codeSize = blob_read_uint32(&reader);
   info_out->bin.code = (uint32_t *)MALLOC(info_out->bin.codeSize);
   blob_copy_bytes(&reader, info_out->bin.code, info_out->bin.codeSize);
            info_out->bin.relocData = NULL;
   /*  Check if data contains RelocInfo */
   uint32_t count = blob_read_uint32(&reader);
   if (count) {
      nv50_ir::RelocInfo *reloc =
               reloc->codePos = blob_read_uint32(&reader);
   reloc->libPos = blob_read_uint32(&reader);
   reloc->dataPos = blob_read_uint32(&reader);
            blob_copy_bytes(&reader, reloc->entry, sizeof(*reloc->entry) * reloc->count);
               info_out->bin.fixupData = NULL;
   /* Check if data contains FixupInfo */
   count = blob_read_uint32(&reader);
   if (count) {
      nv50_ir::FixupInfo *fixup =
                        for (uint32_t i = 0; i < count; i++) {
               /* Assign back function pointer depending on stored enum */
   enum FixupApplyFunc apply = (enum FixupApplyFunc)blob_read_uint8(&reader);
   switch(apply) {
      case APPLY_NV50:
      fixup->entry[i].apply = nv50_ir::nv50_interpApply;
      case APPLY_NVC0:
      fixup->entry[i].apply = nv50_ir::nvc0_interpApply;
      case APPLY_GK110:
      fixup->entry[i].apply = nv50_ir::gk110_interpApply;
      case APPLY_GM107:
      fixup->entry[i].apply = nv50_ir::gm107_interpApply;
      case APPLY_GV100:
      fixup->entry[i].apply = nv50_ir::gv100_interpApply;
      case FLIP_NVC0:
      fixup->entry[i].apply = nv50_ir::nvc0_selpFlip;
      case FLIP_GK110:
      fixup->entry[i].apply = nv50_ir::gk110_selpFlip;
      case FLIP_GM107:
      fixup->entry[i].apply = nv50_ir::gm107_selpFlip;
      case FLIP_GV100:
      fixup->entry[i].apply = nv50_ir::gv100_selpFlip;
      default:
      ERROR("unhandled fixup apply function switch case");
   assert(false);
      }
               info_out->numInputs = blob_read_uint8(&reader);
   info_out->numOutputs = blob_read_uint8(&reader);
   info_out->numSysVals = blob_read_uint8(&reader);
   blob_copy_bytes(&reader, info_out->sv, info_out->numSysVals * sizeof(info_out->sv[0]));
   blob_copy_bytes(&reader, info_out->in, info_out->numInputs * sizeof(info_out->in[0]));
            switch(info_out->type) {
      case PIPE_SHADER_VERTEX:
      blob_copy_bytes(&reader, &info_out->prop.vp, sizeof(info_out->prop.vp));
      case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      blob_copy_bytes(&reader, &info_out->prop.tp, sizeof(info_out->prop.tp));
      case PIPE_SHADER_GEOMETRY:
      blob_copy_bytes(&reader, &info_out->prop.gp, sizeof(info_out->prop.gp));
      case PIPE_SHADER_FRAGMENT:
      blob_copy_bytes(&reader, &info_out->prop.fp, sizeof(info_out->prop.fp));
      case PIPE_SHADER_COMPUTE:
      blob_copy_bytes(&reader, &info_out->prop.cp, sizeof(info_out->prop.cp));
      default:
      }
   blob_copy_bytes(&reader, &(info_out->io), sizeof(info_out->io));
               }
