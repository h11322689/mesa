   #!/bin/env python
   COPYRIGHT = """\
   /*
   * Copyright 2021 Intel Corporation
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
   """
      import os
   import re
   import ply.lex as lex
   import ply.yacc as yacc
      # Libraries
      libraries = {}
      # LEXER
      keywords = {
      '__debugbreak': 'KW_DEBUGBREAK',
   'alignas': 'KW_ALIGNAS',
   'args': 'KW_ARGS',
   'atomic': 'KW_ATOMIC',
   'atomic_return': 'KW_ATOMIC_RETURN',
   'const': 'KW_CONST',
   'control': 'KW_CONTROL',
   'define': 'KW_DEFINE',
   'dispatch': 'KW_DISPATCH',
   'dispatch_indirect': 'KW_DISPATCH_INDIRECT',
   'goto': 'KW_GOTO',
   'if': 'KW_IF',
   'kernel': 'KW_KERNEL',
   'kernel_module': 'KW_KERNEL_MODULE',
   'import': 'KW_IMPORT',
   'library': 'KW_LIBRARY',
   'links': 'KW_LINKS',
   'load_dword': 'KW_LOAD_DWORD',
   'load_qword': 'KW_LOAD_QWORD',
   'metakernel': 'KW_METAKERNEL',
   'module': 'KW_MODULE',
   'not': 'KW_NOT',
   'offsetof': 'KW_OFFSETOF',
   'postsync': 'KW_POSTSYNC',
   'print': 'KW_PRINT',
   'semaphore_wait': 'KW_SEMAPHORE_WAIT',
   'shiftof': 'KW_SHIFTOF',
   'sizeof': 'KW_SIZEOF',
   'store_dword': 'KW_STORE_DWORD',
   'store_qword': 'KW_STORE_QWORD',
   'store_timestamp': 'KW_STORE_TIMESTAMP',
   'struct': 'KW_STRUCT',
   'unsigned': 'KW_UNSIGNED',
      }
      ops = {
      '&&': 'OP_LOGICAL_AND',
   '||': 'OP_LOGICAL_OR',
   '==': 'OP_EQUALEQUAL',
   '!=': 'OP_NOTEQUAL',
   '<=': 'OP_LESSEQUAL',
   '>=': 'OP_GREATEREQUAL',
   '<<': 'OP_LSHIFT',
      }
      tokens = [
      'INT_LITERAL',
   'STRING_LITERAL',
   'OP',
      ] + list(keywords.values()) + list(ops.values())
      def t_INT_LITERAL(t):
      r'(0x[a-fA-F0-9]+|\d+)'
   if t.value.startswith('0x'):
         else:
               def t_OP(t):
      r'(&&|\|\||==|!=|<=|>=|<<|>>)'
   t.type = ops.get(t.value)
         def t_IDENTIFIER(t):
      r'[a-zA-Z_][a-zA-Z_0-9]*'
   t.type = keywords.get(t.value, 'IDENTIFIER')
         def t_STRING_LITERAL(t):
      r'"(\\.|[^"\\])*"'
   t.value = t.value[1:-1]
         literals = "+*/(){};:,=&|!~^.%?-<>[]"
      t_ignore = ' \t'
      def t_newline(t):
      r'\n+'
         def t_error(t):
      print("WUT: {}".format(t.value))
         LEXER = lex.lex()
      # PARSER
      precedence = (
      ('right', '?', ':'),
   ('left', 'OP_LOGICAL_OR', 'OP_LOGICAL_AND'),
   ('left', '|'),
   ('left', '^'),
   ('left', '&'),
   ('left', 'OP_EQUALEQUAL', 'OP_NOTEQUAL'),
   ('left', '<', '>', 'OP_LESSEQUAL', 'OP_GREATEREQUAL'),
   ('left', 'OP_LSHIFT', 'OP_RSHIFT'),
   ('left', '+', '-'),
   ('left', '*', '/', '%'),
   ('right', '!', '~'),
      )
      def p_module(p):
      'module : element_list'
         def p_element_list(p):
      '''element_list : element_list element
         if len(p) == 2:
         else:
         def p_element(p):
      '''element : kernel_definition
               | kernel_module_definition
   | library_definition
   | metakernel_definition
   | module_name
   | struct_definition
         def p_module_name(p):
      'module_name : KW_MODULE IDENTIFIER ";"'
         def p_kernel_module_definition(p):
      'kernel_module_definition : KW_KERNEL_MODULE IDENTIFIER "(" STRING_LITERAL ")" "{" kernel_definition_list "}"'
         def p_kernel_definition(p):
      'kernel_definition : KW_KERNEL IDENTIFIER optional_annotation_list'
         def p_library_definition(p):
      'library_definition : KW_LIBRARY IDENTIFIER "{" library_definition_list "}"'
         def p_library_definition_list(p):
      '''library_definition_list :
         if len(p) < 3:
         else:
      p[0] = p[1]
      def p_import_definition(p):
      'import_definition : KW_IMPORT KW_STRUCT IDENTIFIER STRING_LITERAL ";"'
         def p_links_definition(p):
               # Process a library include like a preprocessor
            if not p[2] in libraries:
               def p_metakernel_definition(p):
      'metakernel_definition : KW_METAKERNEL IDENTIFIER "(" optional_parameter_list ")" optional_annotation_list scope'
         def p_kernel_definition_list(p):
      '''kernel_definition_list :
               if len(p) < 3:
         else:
      p[0] = p[1]
      def p_optional_annotation_list(p):
      '''optional_annotation_list :
               if len(p) < 4:
         else:
         def p_optional_parameter_list(p):
      '''optional_parameter_list :
               def p_annotation_list(p):
      '''annotation_list : annotation'''
         def p_annotation_list_append(p):
      '''annotation_list : annotation_list "," annotation'''
         def p_annotation(p):
      '''annotation : IDENTIFIER "=" INT_LITERAL
                     def p_parameter_list(p):
      '''parameter_list : parameter_definition'''
         def p_parameter_list_append(p):
      '''parameter_list : parameter_list "," parameter_definition'''
   p[0] = p[1]
         def p_parameter_definition(p):
      'parameter_definition : IDENTIFIER IDENTIFIER'
         def p_scope(p):
      '''scope : "{" optional_statement_list "}"'''
         def p_optional_statement_list(p):
      '''optional_statement_list :
               def p_statement_list(p):
      '''statement_list : statement'''
         def p_statement_list_append(p):
      '''statement_list : statement_list statement'''
   p[0] = p[1]
         def p_statement(p):
      '''statement : definition_statement ";"
               | assignment_statement ";"
   | load_store_statement ";"
   | dispatch_statement ";"
   | semaphore_statement ";"
   | label
   | goto_statement ";"
   | scope_statement
   | atomic_op_statement ";"
   | control_statement ";"
         def p_definition_statement(p):
      'definition_statement : KW_DEFINE IDENTIFIER value'
         def p_assignemt_statement(p):
      'assignment_statement : value "=" value'
         def p_load_store_statement_load_dword(p):
      '''load_store_statement : value "=" KW_LOAD_DWORD "(" value ")"'''
         def p_load_store_statement_load_qword(p):
      '''load_store_statement : value "=" KW_LOAD_QWORD "(" value ")"'''
         def p_load_store_statement_store_dword(p):
      '''load_store_statement : KW_STORE_DWORD "(" value "," value ")"'''
         def p_load_store_statement_store_qword(p):
      '''load_store_statement : KW_STORE_QWORD "(" value "," value ")"'''
         def p_dispatch_statement(p):
      '''dispatch_statement : direct_dispatch_statement
               def p_direct_dispatch_statement(p):
      '''direct_dispatch_statement : KW_DISPATCH IDENTIFIER "(" value "," value "," value ")" optional_kernel_arg_list optional_postsync'''
         def p_indirect_dispatch_statement(p):
      '''indirect_dispatch_statement : KW_DISPATCH_INDIRECT IDENTIFIER optional_kernel_arg_list optional_postsync'''
         def p_optional_kernel_arg_list(p):
      '''optional_kernel_arg_list :
               def p_value_list(p):
      '''value_list : value'''
         def p_value_list_append(p):
      '''value_list : value_list "," value'''
   p[0] = p[1]
         def p_optional_postsync(p):
      '''optional_postsync :
         if len(p) > 1:
         def p_postsync_operation(p):
      '''postsync_operation : postsync_write_dword
               def p_postsync_write_dword(p):
      '''postsync_write_dword : KW_POSTSYNC KW_STORE_DWORD "(" value "," value ")"'''
         def p_postsync_write_timestamp(p):
      '''postsync_write_timestamp : KW_POSTSYNC KW_STORE_TIMESTAMP "(" value ")"'''
         def p_semaphore_statement(p):
      '''semaphore_statement : KW_SEMAPHORE_WAIT KW_WHILE "(" "*" value "<" value ")"
                           | KW_SEMAPHORE_WAIT KW_WHILE "(" "*" value ">" value ")"
         def p_atomic_op_statement(p):
      '''atomic_op_statement : KW_ATOMIC IDENTIFIER IDENTIFIER "(" value_list ")"'''
         def p_atomic_op_statement_return(p):
      '''atomic_op_statement : KW_ATOMIC_RETURN IDENTIFIER IDENTIFIER "(" value_list ")"'''
         def p_label(p):
      '''label : IDENTIFIER ":"'''
         def p_goto_statement(p):
      '''goto_statement : KW_GOTO IDENTIFIER'''
         def p_goto_statement_if(p):
      '''goto_statement : KW_GOTO IDENTIFIER KW_IF "(" value ")"'''
         def p_goto_statement_if_not(p):
      '''goto_statement : KW_GOTO IDENTIFIER KW_IF KW_NOT "(" value ")"'''
         def p_scope_statement(p):
      '''scope_statement : scope'''
         def p_control_statement(p):
      '''control_statement : KW_CONTROL "(" id_list ")"'''
         def p_print_statement(p):
      '''print_statement : KW_PRINT "(" printable_list ")"'''
         def p_printable_list(p):
      '''printable_list : printable'''
         def p_printable_list_append(p):
      '''printable_list : printable_list "," printable'''
   p[0] = p[1]
         def p_printable_str_lit(p):
      '''printable : STRING_LITERAL'''
         def p_printable_value(p):
      '''printable : value'''
         def p_printable_str_lit_value(p):
      '''printable : STRING_LITERAL value'''
         def p_debug_break_statement(p):
      '''debug_break_statement : KW_DEBUGBREAK'''
         def p_id_list(p):
      '''id_list : IDENTIFIER'''
         def p_id_list_append(p):
      '''id_list : id_list "," IDENTIFIER'''
   p[0] = p[1]
         def p_value(p):
      '''value : IDENTIFIER
               def p_value_braces(p):
      '''value : "(" value ")"'''
         def p_value_member(p):
      '''value : value "." IDENTIFIER'''
         def p_value_idx(p):
      '''value : value "[" value "]"'''
         def p_value_binop(p):
      '''value : value "+" value
            | value "-" value
   | value "*" value
   | value "/" value
   | value "%" value
   | value "&" value
   | value "|" value
   | value "<" value
   | value ">" value
   | value "^" value
   | value OP_LESSEQUAL value
   | value OP_GREATEREQUAL value
   | value OP_EQUALEQUAL value
   | value OP_NOTEQUAL value
   | value OP_LOGICAL_AND value
   | value OP_LOGICAL_OR value
            def p_value_uniop(p):
      '''value : "!" value
               def p_value_cond(p):
      '''value : value "?" value ":" value'''
         def p_value_funcop(p):
      '''value : KW_OFFSETOF "(" offset_expression ")"
                     def p_offset_expression(p):
      '''offset_expression : IDENTIFIER'''
         def p_offset_expression_member(p):
      '''offset_expression : offset_expression "." IDENTIFIER'''
         def p_offset_expression_idx(p):
      '''offset_expression : offset_expression "[" INT_LITERAL "]"'''
         def p_struct_definition(p):
      '''struct_definition : KW_STRUCT optional_alignment_specifier IDENTIFIER "{" optional_struct_member_list "}" ";"'''
         def p_optional_alignment_specifier(p):
      '''optional_alignment_specifier :
         if len(p) == 1:
         else:
         def p_optional_struct_member_list(p):
      '''optional_struct_member_list :
         if len(p) == 1:
         else:
         def p_struct_member_list(p):
      '''struct_member_list : struct_member'''
         def p_struct_member_list_append(p):
      '''struct_member_list : struct_member_list struct_member'''
         def p_struct_member(p):
      '''struct_member : struct_member_typename IDENTIFIER ";"'''
         def p_struct_member_array(p):
      '''struct_member : struct_member_typename IDENTIFIER "[" INT_LITERAL "]" ";"'''
   '''struct_member : struct_member_typename IDENTIFIER "[" IDENTIFIER "]" ";"'''
         def p_struct_member_typename(p):
      '''struct_member_typename : IDENTIFIER'''
         def p_struct_member_typename_unsigned(p):
      '''struct_member_typename : KW_UNSIGNED IDENTIFIER'''
         def p_struct_member_typename_struct(p):
      '''struct_member_typename : KW_STRUCT IDENTIFIER'''
         def p_const_definition(p):
      '''const_definition : KW_CONST IDENTIFIER "=" INT_LITERAL ";"'''
         PARSER = yacc.yacc()
      # Shamelessly stolen from some StackOverflow answer
   def _remove_comments(text):
      def replacer(match):
      s = match.group(0)
   if s.startswith('/'):
         else:
      pattern = re.compile(
      r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
      )
         def parse_grl_file(grl_fname, libs):
               libraries = libs
   with open(grl_fname, 'r') as f:
      return PARSER.parse(_remove_comments(f.read()))
