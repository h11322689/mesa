   /* -*- c++ -*- */
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
      #include "glsl_symbol_table.h"
   #include "ast.h"
      class symbol_table_entry {
   public:
               bool add_interface(const glsl_type *i, enum ir_variable_mode mode)
   {
               switch (mode) {
   case ir_var_uniform:
      dest = &ibu;
      case ir_var_shader_storage:
      dest = &iss;
      case ir_var_shader_in:
      dest = &ibi;
      case ir_var_shader_out:
      dest = &ibo;
      default:
      assert(!"Unsupported interface variable mode!");
               if (*dest != NULL) {
         } else {
      *dest = i;
                  const glsl_type *get_interface(enum ir_variable_mode mode)
   {
      switch (mode) {
   case ir_var_uniform:
         case ir_var_shader_storage:
         case ir_var_shader_in:
         case ir_var_shader_out:
         default:
      assert(!"Unsupported interface variable mode!");
                  symbol_table_entry(ir_variable *v)               :
         symbol_table_entry(ir_function *f)               :
         symbol_table_entry(const glsl_type *t)           :
         symbol_table_entry(const glsl_type *t, enum ir_variable_mode mode) :
         {
      assert(t->is_interface());
      }
   symbol_table_entry(const class ast_type_specifier *a):
            ir_variable *v;
   ir_function *f;
   const glsl_type *t;
   const glsl_type *ibu;
   const glsl_type *iss;
   const glsl_type *ibi;
   const glsl_type *ibo;
      };
      glsl_symbol_table::glsl_symbol_table()
   {
      this->separate_function_namespace = false;
   this->table = _mesa_symbol_table_ctor();
   this->mem_ctx = ralloc_context(NULL);
      }
      glsl_symbol_table::~glsl_symbol_table()
   {
      _mesa_symbol_table_dtor(table);
      }
      void glsl_symbol_table::push_scope()
   {
         }
      void glsl_symbol_table::pop_scope()
   {
         }
      bool glsl_symbol_table::name_declared_this_scope(const char *name)
   {
         }
      bool glsl_symbol_table::add_variable(ir_variable *v)
   {
               if (this->separate_function_namespace) {
      /* In 1.10, functions and variables have separate namespaces. */
   symbol_table_entry *existing = get_entry(v->name);
   /* If there's already an existing function (not a constructor!) in
      * the current scope, just update the existing entry to include 'v'.
      if (existing->v == NULL && existing->t == NULL) {
      existing->v = v;
      }
         /* If not declared at this scope, add a new entry.  But if an existing
      * entry includes a function, propagate that to this block - otherwise
   * the new variable declaration would shadow the function.
      symbol_table_entry *entry = new(linalloc) symbol_table_entry(v);
   if (existing != NULL)
         int added = _mesa_symbol_table_add_symbol(table, v->name, entry);
   assert(added == 0);
   (void)added;
   return true;
         }
               /* 1.20+ rules: */
   symbol_table_entry *entry = new(linalloc) symbol_table_entry(v);
      }
      bool glsl_symbol_table::add_type(const char *name, const glsl_type *t)
   {
      symbol_table_entry *entry = new(linalloc) symbol_table_entry(t);
      }
      bool glsl_symbol_table::add_interface(const char *name, const glsl_type *i,
         {
      assert(i->is_interface());
   symbol_table_entry *entry = get_entry(name);
   if (entry == NULL) {
      symbol_table_entry *entry =
         bool add_interface_symbol_result =
         assert(add_interface_symbol_result);
      } else {
            }
      bool glsl_symbol_table::add_function(ir_function *f)
   {
      if (this->separate_function_namespace && name_declared_this_scope(f->name)) {
      /* In 1.10, functions and variables have separate namespaces. */
   symbol_table_entry *existing = get_entry(f->name);
   existing->f = f;
   return true;
            }
   symbol_table_entry *entry = new(linalloc) symbol_table_entry(f);
      }
      bool glsl_symbol_table::add_default_precision_qualifier(const char *type_name,
         {
               ast_type_specifier *default_specifier = new(linalloc) ast_type_specifier(name);
            symbol_table_entry *entry =
            if (!get_entry(name))
               }
      void glsl_symbol_table::add_global_function(ir_function *f)
   {
      symbol_table_entry *entry = new(linalloc) symbol_table_entry(f);
   int added = _mesa_symbol_table_add_global_symbol(table, f->name, entry);
   assert(added == 0);
      }
      ir_variable *glsl_symbol_table::get_variable(const char *name)
   {
      symbol_table_entry *entry = get_entry(name);
      }
      const glsl_type *glsl_symbol_table::get_type(const char *name)
   {
      symbol_table_entry *entry = get_entry(name);
      }
      const glsl_type *glsl_symbol_table::get_interface(const char *name,
         {
      symbol_table_entry *entry = get_entry(name);
      }
      ir_function *glsl_symbol_table::get_function(const char *name)
   {
      symbol_table_entry *entry = get_entry(name);
      }
      int glsl_symbol_table::get_default_precision_qualifier(const char *type_name)
   {
      char *name = ralloc_asprintf(mem_ctx, "#default_precision_%s", type_name);
   symbol_table_entry *entry = get_entry(name);
   if (!entry)
            }
      symbol_table_entry *glsl_symbol_table::get_entry(const char *name)
   {
      return (symbol_table_entry *)
      }
      void
   glsl_symbol_table::disable_variable(const char *name)
   {
      /* Ideally we would remove the variable's entry from the symbol table, but
   * that would be difficult.  Fortunately, since this is only used for
   * built-in variables, it won't be possible for the shader to re-introduce
   * the variable later, so all we really need to do is to make sure that
   * further attempts to access it using get_variable() will return NULL.
   */
   symbol_table_entry *entry = get_entry(name);
   if (entry != NULL) {
            }
      void
   glsl_symbol_table::replace_variable(const char *name,
         {
      symbol_table_entry *entry = get_entry(name);
   if (entry != NULL) {
            }
