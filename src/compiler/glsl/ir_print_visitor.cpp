   /*
   * Copyright © 2010 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <inttypes.h> /* for PRIx64 macro */
   #include "ir_print_visitor.h"
   #include "linker_util.h"
   #include "compiler/glsl_types.h"
   #include "glsl_parser_extras.h"
   #include "main/macros.h"
   #include "util/hash_table.h"
   #include "util/u_string.h"
   #include "util/half_float.h"
      void
   ir_instruction::print(void) const
   {
         }
      void
   ir_instruction::fprint(FILE *f) const
   {
               ir_print_visitor v(f);
      }
      static void
   glsl_print_type(FILE *f, const glsl_type *t)
   {
      if (t->is_array()) {
      fprintf(f, "(array ");
   glsl_print_type(f, t->fields.array);
      } else if (t->is_struct() && !is_gl_identifier(glsl_get_type_name(t))) {
         } else {
            }
      extern "C" {
   void
   _mesa_print_ir(FILE *f, exec_list *instructions,
         {
      if (state) {
      const glsl_type *const s = state->user_structures[i];
      fprintf(f, "(structure (%s) (%s@%p) (%u) (\n",
            for (unsigned j = 0; j < s->length; j++) {
      fprintf(f, "\t((");
   glsl_print_type(f, s->fields.structure[j].type);
      }
      fprintf(f, ")\n");
                     fprintf(f, "(\n");
   foreach_in_list(ir_instruction, ir, instructions) {
      ir->fprint(f);
   fprintf(f, "\n");
      }
      }
      void
   fprint_ir(FILE *f, const void *instruction)
   {
      const ir_instruction *ir = (const ir_instruction *)instruction;
      }
      } /* extern "C" */
      ir_print_visitor::ir_print_visitor(FILE *f)
         {
      indentation = 0;
   printable_names = _mesa_pointer_hash_table_create(NULL);
   symbols = _mesa_symbol_table_ctor();
      }
      ir_print_visitor::~ir_print_visitor()
   {
      _mesa_hash_table_destroy(printable_names, NULL);
   _mesa_symbol_table_dtor(symbols);
      }
      void ir_print_visitor::indent(void)
   {
      for (int i = 0; i < indentation; i++)
      }
      const char *
   ir_print_visitor::unique_name(ir_variable *var)
   {
      /* var->name can be NULL in function prototypes when a type is given for a
   * parameter but no name is given.  In that case, just return an empty
   * string.  Don't worry about tracking the generated name in the printable
   * names hash because this is the only scope where it can ever appear.
   */
   if (var->name == NULL) {
      static unsigned arg = 1;
               /* Do we already have a name for this variable? */
   struct hash_entry * entry =
            if (entry != NULL) {
                  /* If there's no conflict, just use the original name */
   const char* name = NULL;
   if (_mesa_symbol_table_find_symbol(this->symbols, var->name) == NULL) {
         } else {
      static unsigned i = 1;
      }
   _mesa_hash_table_insert(this->printable_names, var, (void *) name);
   _mesa_symbol_table_add_symbol(this->symbols, name, var);
      }
      void ir_print_visitor::visit(ir_rvalue *)
   {
         }
      void ir_print_visitor::visit(ir_variable *ir)
   {
               char binding[32] = {0};
   if (ir->data.binding)
            char loc[32] = {0};
   if (ir->data.location != -1)
            char component[32] = {0};
   if (ir->data.explicit_component || ir->data.location_frac != 0)
      snprintf(component, sizeof(component), "component=%i ",
         char stream[32] = {0};
   if (ir->data.stream & (1u << 31)) {
      if (ir->data.stream & ~(1u << 31)) {
      snprintf(stream, sizeof(stream), "stream(%u,%u,%u,%u) ",
               } else if (ir->data.stream) {
                  char image_format[32] = {0};
   if (ir->data.image_format) {
      snprintf(image_format, sizeof(image_format), "format=%x ",
               const char *const cent = (ir->data.centroid) ? "centroid " : "";
   const char *const samp = (ir->data.sample) ? "sample " : "";
   const char *const patc = (ir->data.patch) ? "patch " : "";
   const char *const inv = (ir->data.invariant) ? "invariant " : "";
   const char *const explicit_inv = (ir->data.explicit_invariant) ? "explicit_invariant " : "";
   const char *const prec = (ir->data.precise) ? "precise " : "";
   const char *const bindless = (ir->data.bindless) ? "bindless " : "";
   const char *const bound = (ir->data.bound) ? "bound " : "";
   const char *const memory_read_only = (ir->data.memory_read_only) ? "readonly " : "";
   const char *const memory_write_only = (ir->data.memory_write_only) ? "writeonly " : "";
   const char *const memory_coherent = (ir->data.memory_coherent) ? "coherent " : "";
   const char *const memory_volatile = (ir->data.memory_volatile) ? "volatile " : "";
   const char *const memory_restrict = (ir->data.memory_restrict) ? "restrict " : "";
   const char *const mode[] = { "", "uniform ", "shader_storage ",
                     STATIC_ASSERT(ARRAY_SIZE(mode) == ir_var_mode_count);
   const char *const interp[] = { "", "smooth", "flat", "noperspective", "explicit", "color" };
   STATIC_ASSERT(ARRAY_SIZE(interp) == INTERP_MODE_COUNT);
            fprintf(f, "(%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s) ",
         binding, loc, component, cent, bindless, bound,
   image_format, memory_read_only, memory_write_only,
   memory_coherent, memory_volatile, memory_restrict,
   samp, patc, inv, explicit_inv, prec, mode[ir->data.mode],
            glsl_print_type(f, ir->type);
            if (ir->constant_initializer) {
      fprintf(f, " ");
               if (ir->constant_value) {
      fprintf(f, " ");
         }
         void ir_print_visitor::visit(ir_function_signature *ir)
   {
      _mesa_symbol_table_push_scope(symbols);
   fprintf(f, "(signature ");
            glsl_print_type(f, ir->return_type);
   fprintf(f, "\n");
            fprintf(f, "(parameters\n");
            foreach_in_list(ir_variable, inst, &ir->parameters) {
      indent();
   inst->accept(this);
      }
            indent();
                     fprintf(f, "(\n");
            foreach_in_list(ir_instruction, inst, &ir->body) {
      indent();
   inst->accept(this);
      }
   indentation--;
   indent();
   fprintf(f, "))\n");
   indentation--;
      }
         void ir_print_visitor::visit(ir_function *ir)
   {
      fprintf(f, "(%s function %s\n", ir->is_subroutine ? "subroutine" : "", ir->name);
   indentation++;
   foreach_in_list(ir_function_signature, sig, &ir->signatures) {
      indent();
   sig->accept(this);
      }
   indentation--;
   indent();
      }
         void ir_print_visitor::visit(ir_expression *ir)
   {
                                 for (unsigned i = 0; i < ir->num_operands; i++) {
                     }
         void ir_print_visitor::visit(ir_texture *ir)
   {
               if (ir->op == ir_samples_identical) {
      ir->sampler->accept(this);
   fprintf(f, " ");
   ir->coordinate->accept(this);
   fprintf(f, ")");
               glsl_print_type(f, ir->type);
            ir->sampler->accept(this);
            if (ir->op != ir_txs && ir->op != ir_query_levels &&
      ir->op != ir_texture_samples) {
                     if (ir->op != ir_lod && ir->op != ir_samples_identical)
            ir->offset->accept(this);
         fprintf(f, "0");
                              if (ir->op != ir_txf && ir->op != ir_txf_ms &&
      ir->op != ir_txs && ir->op != ir_tg4 &&
   ir->op != ir_query_levels && ir->op != ir_texture_samples) {
   ir->projector->accept(this);
         fprintf(f, "1");
            fprintf(f, " ");
   ir->shadow_comparator->accept(this);
         fprintf(f, " ()");
                     if (ir->op == ir_tex || ir->op == ir_txb || ir->op == ir_txd) {
      if (ir->clamp) {
      fprintf(f, " ");
      } else {
                     fprintf(f, " ");
   switch (ir->op)
   {
   case ir_tex:
   case ir_lod:
   case ir_query_levels:
   case ir_texture_samples:
         case ir_txb:
      ir->lod_info.bias->accept(this);
      case ir_txl:
   case ir_txf:
   case ir_txs:
      ir->lod_info.lod->accept(this);
      case ir_txf_ms:
      ir->lod_info.sample_index->accept(this);
      case ir_txd:
      fprintf(f, "(");
   ir->lod_info.grad.dPdx->accept(this);
   fprintf(f, " ");
   ir->lod_info.grad.dPdy->accept(this);
   fprintf(f, ")");
      case ir_tg4:
      ir->lod_info.component->accept(this);
      case ir_samples_identical:
         };
      }
         void ir_print_visitor::visit(ir_swizzle *ir)
   {
      const unsigned swiz[4] = {
      ir->mask.x,
   ir->mask.y,
   ir->mask.z,
               fprintf(f, "(swiz ");
   for (unsigned i = 0; i < ir->mask.num_components; i++) {
         }
   fprintf(f, " ");
   ir->val->accept(this);
      }
         void ir_print_visitor::visit(ir_dereference_variable *ir)
   {
      ir_variable *var = ir->variable_referenced();
      }
         void ir_print_visitor::visit(ir_dereference_array *ir)
   {
      fprintf(f, "(array_ref ");
   ir->array->accept(this);
   ir->array_index->accept(this);
      }
         void ir_print_visitor::visit(ir_dereference_record *ir)
   {
      fprintf(f, "(record_ref ");
            const char *field_name =
            }
         void ir_print_visitor::visit(ir_assignment *ir)
   {
               char mask[5];
            for (unsigned i = 0; i < 4; i++) {
      mask[j] = "xyzw"[i];
   j++;
            }
                                       ir->rhs->accept(this);
      }
      static void
   print_float_constant(FILE *f, float val)
   {
      if (val == 0.0f)
      /* 0.0 == -0.0, so print with %f to get the proper sign. */
      else if (fabs(val) < 0.000001f)
         else if (fabs(val) > 1000000.0f)
         else
      }
      void ir_print_visitor::visit(ir_constant *ir)
   {
      fprintf(f, "(constant ");
   glsl_print_type(f, ir->type);
            if (ir->type->is_array()) {
      ir->get_array_element(i)->accept(this);
      } else if (ir->type->is_struct()) {
      fprintf(f, "(%s ", ir->type->fields.structure[i].name);
         fprintf(f, ")");
            } else {
      if (i != 0)
         switch (ir->type->base_type) {
         case GLSL_TYPE_INT16: fprintf(f, "%d", ir->value.i16[i]); break;
   case GLSL_TYPE_UINT:  fprintf(f, "%u", ir->value.u[i]); break;
   case GLSL_TYPE_INT:   fprintf(f, "%d", ir->value.i[i]); break;
   case GLSL_TYPE_FLOAT:
               case GLSL_TYPE_FLOAT16:
               case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_UINT64:
               case GLSL_TYPE_INT64: fprintf(f, "%" PRIi64, ir->value.i64[i]); break;
   case GLSL_TYPE_BOOL:  fprintf(f, "%d", ir->value.b[i]); break;
   case GLSL_TYPE_DOUBLE:
               if (ir->value.d[i] == 0.0)
      /* 0.0 == -0.0, so print with %f to get the proper sign. */
      else if (fabs(ir->value.d[i]) < 0.000001)
         else if (fabs(ir->value.d[i]) > 1000000.0)
         else
   default:
         }
            }
      }
         void
   ir_print_visitor::visit(ir_call *ir)
   {
      fprintf(f, "(call %s ", ir->callee_name());
   if (ir->return_deref)
         fprintf(f, " (");
   foreach_in_list(ir_rvalue, param, &ir->actual_parameters) {
         }
      }
         void
   ir_print_visitor::visit(ir_return *ir)
   {
               ir_rvalue *const value = ir->get_value();
   if (value) {
      fprintf(f, " ");
                  }
         void
   ir_print_visitor::visit(ir_discard *ir)
   {
               if (ir->condition != NULL) {
      fprintf(f, " ");
                  }
         void
   ir_print_visitor::visit(ir_demote *ir)
   {
         }
         void
   ir_print_visitor::visit(ir_if *ir)
   {
      fprintf(f, "(if ");
            fprintf(f, "(\n");
            foreach_in_list(ir_instruction, inst, &ir->then_instructions) {
      indent();
   inst->accept(this);
               indentation--;
   indent();
            indent();
   if (!ir->else_instructions.is_empty()) {
      fprintf(f, "(\n");
            indent();
   inst->accept(this);
   fprintf(f, "\n");
         }
   indentation--;
   indent();
      } else {
            }
         void
   ir_print_visitor::visit(ir_loop *ir)
   {
      fprintf(f, "(loop (\n");
            foreach_in_list(ir_instruction, inst, &ir->body_instructions) {
      indent();
   inst->accept(this);
      }
   indentation--;
   indent();
      }
         void
   ir_print_visitor::visit(ir_loop_jump *ir)
   {
         }
      void
   ir_print_visitor::visit(ir_emit_vertex *ir)
   {
      fprintf(f, "(emit-vertex ");
   ir->stream->accept(this);
      }
      void
   ir_print_visitor::visit(ir_end_primitive *ir)
   {
      fprintf(f, "(end-primitive ");
   ir->stream->accept(this);
      }
      void
   ir_print_visitor::visit(ir_barrier *)
   {
         }
