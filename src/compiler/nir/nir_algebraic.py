   #
   # Copyright (C) 2014 Intel Corporation
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      import ast
   from collections import defaultdict
   import itertools
   import struct
   import sys
   import mako.template
   import re
   import traceback
      from nir_opcodes import opcodes, type_sizes
      # This should be the same as NIR_SEARCH_MAX_COMM_OPS in nir_search.c
   nir_search_max_comm_ops = 8
      # These opcodes are only employed by nir_search.  This provides a mapping from
   # opcode to destination type.
   conv_opcode_types = {
      'i2f' : 'float',
   'u2f' : 'float',
   'f2f' : 'float',
   'f2u' : 'uint',
   'f2i' : 'int',
   'u2u' : 'uint',
   'i2i' : 'int',
   'b2f' : 'float',
   'b2i' : 'int',
   'i2b' : 'bool',
      }
      def get_cond_index(conds, cond):
      if cond:
      if cond in conds:
         else:
         cond_index = len(conds)
      else:
         def get_c_opcode(op):
         if op in conv_opcode_types:
         else:
      _type_re = re.compile(r"(?P<type>int|uint|bool|float)?(?P<bits>\d+)?")
      def type_bits(type_str):
      m = _type_re.match(type_str)
            if m.group('bits') is None:
         else:
         # Represents a set of variables, each with a unique id
   class VarSet(object):
      def __init__(self):
      self.names = {}
   self.ids = itertools.count()
         def __getitem__(self, name):
      if name not in self.names:
                        def lock(self):
         class SearchExpression(object):
      def __init__(self, expr):
      self.opcode = expr[0]
   self.sources = expr[1:]
         @staticmethod
   def create(val):
      if isinstance(val, tuple):
         else:
               def __repr__(self):
      l = [self.opcode, *self.sources]
   if self.ignore_exact:
            class Value(object):
      @staticmethod
   def create(val, name_base, varset, algebraic_pass):
      if isinstance(val, bytes):
            if isinstance(val, tuple) or isinstance(val, SearchExpression):
         elif isinstance(val, Expression):
         elif isinstance(val, str):
         elif isinstance(val, (bool, float, int)):
         def __init__(self, val, name, type_str):
      self.in_val = str(val)
   self.name = name
         def __str__(self):
            def get_bit_size(self):
      """Get the physical bit-size that has been chosen for this value, or if
   there is none, the canonical value which currently represents this
   bit-size class. Variables will be preferred, i.e. if there are any
   variables in the equivalence class, the canonical value will be a
   variable. We do this since we'll need to know which variable each value
   is equivalent to when constructing the replacement expression. This is
   the "find" part of the union-find algorithm.
   """
            while isinstance(bit_size, Value):
      if bit_size._bit_size is None:
               if bit_size is not self:
               def set_bit_size(self, other):
      """Make self.get_bit_size() return what other.get_bit_size() return
   before calling this, or just "other" if it's a concrete bit-size. This is
   the "union" part of the union-find algorithm.
            self_bit_size = self.get_bit_size()
            if self_bit_size == other_bit_size:
                  @property
   def type_enum(self):
            @property
   def c_bit_size(self):
      bit_size = self.get_bit_size()
   if isinstance(bit_size, int):
         elif isinstance(bit_size, Variable):
         else:
      # If the bit-size class is neither a variable, nor an actual bit-size, then
   # - If it's in the search expression, we don't need to check anything
   # - If it's in the replace expression, either it's ambiguous (in which
   # case we'd reject it), or it equals the bit-size of the search value
            __template = mako.template.Template("""   { .${val.type_str} = {
      % if isinstance(val, Constant):
         % elif isinstance(val, Variable):
         ${val.index}, /* ${val.var_name} */
   ${'true' if val.is_constant else 'false'},
   ${val.type() or 'nir_type_invalid' },
   ${val.cond_index},
   % elif isinstance(val, Expression):
         ${'true' if val.inexact else 'false'},
   ${'true' if val.exact else 'false'},
   ${'true' if val.ignore_exact else 'false'},
   ${val.c_opcode()},
   ${val.comm_expr_idx}, ${val.comm_exprs},
   { ${', '.join(src.array_index for src in val.sources)} },
   % endif
         """)
         def render(self, cache):
      struct_init = self.__template.render(val=self,
                     if struct_init in cache:
      # If it's in the cache, register a name remap in the cache and render
   # only a comment saying it's been remapped
   self.array_index = cache[struct_init]
   return "   /* {} -> {} in the cache */\n".format(self.name,
      else:
      self.array_index = str(cache["next_index"])
   cache[struct_init] = self.array_index
         _constant_re = re.compile(r"(?P<value>[^@\(]+)(?:@(?P<bits>\d+))?")
      class Constant(Value):
      def __init__(self, val, name):
               if isinstance(val, (str)):
      m = _constant_re.match(val)
   self.value = ast.literal_eval(m.group('value'))
      else:
                  if isinstance(self.value, bool):
               def hex(self):
      if isinstance(self.value, (bool)):
         if isinstance(self.value, int):
         elif isinstance(self.value, float):
         else:
         def type(self):
      if isinstance(self.value, (bool)):
         elif isinstance(self.value, int):
         elif isinstance(self.value, float):
         def equivalent(self, other):
               This is check is much weaker than equality.  One generally cannot be
   used in place of the other.  Using this implementation for the __eq__
            """
   if not isinstance(other, type(self)):
               # The $ at the end forces there to be an error if any part of the string
   # doesn't match one of the field patterns.
   _var_name_re = re.compile(r"(?P<const>#)?(?P<name>\w+)"
                              class Variable(Value):
      def __init__(self, val, name, varset, algebraic_pass):
               m = _var_name_re.match(val)
   assert m and m.group('name') is not None, \
                     # Prevent common cases where someone puts quotes around a literal
   # constant.  If we want to support names that have numeric or
   # punctuation characters, we can me the first assertion more flexible.
   assert self.var_name.isalpha()
   assert self.var_name != 'True'
            self.is_constant = m.group('const') is not None
   self.cond_index = get_cond_index(algebraic_pass.variable_cond, m.group('cond'))
   self.required_type = m.group('type')
   self._bit_size = int(m.group('bits')) if m.group('bits') else None
            if self.required_type == 'bool':
      if self._bit_size is not None:
                     if self.required_type is not None:
                  def type(self):
      if self.required_type == 'bool':
         elif self.required_type in ('int', 'uint'):
         elif self.required_type == 'float':
         def equivalent(self, other):
               This is check is much weaker than equality.  One generally cannot be
   used in place of the other.  Using this implementation for the __eq__
            """
   if not isinstance(other, type(self)):
                  def swizzle(self):
      if self.swiz is not None:
      swizzles = {'x' : 0, 'y' : 1, 'z' : 2, 'w' : 3,
               'a' : 0, 'b' : 1, 'c' : 2, 'd' : 3,
   'e' : 4, 'f' : 5, 'g' : 6, 'h' : 7,
         _opcode_re = re.compile(r"(?P<inexact>~)?(?P<exact>!)?(?P<opcode>\w+)(?:@(?P<bits>\d+))?"
            class Expression(Value):
      def __init__(self, expr, name_base, varset, algebraic_pass):
                        m = _opcode_re.match(expr.opcode)
            self.opcode = m.group('opcode')
   self._bit_size = int(m.group('bits')) if m.group('bits') else None
   self.inexact = m.group('inexact') is not None
   self.exact = m.group('exact') is not None
   self.ignore_exact = expr.ignore_exact
            assert not self.inexact or not self.exact, \
            # "many-comm-expr" isn't really a condition.  It's notification to the
   # generator that this pattern is known to have too many commutative
   # expressions, and an error should not be generated for this case.
   self.many_commutative_expressions = False
   if self.cond and self.cond.find("many-comm-expr") >= 0:
      # Split the condition into a comma-separated list.  Remove
   # "many-comm-expr".  If there is anything left, put it back together.
   c = self.cond[1:-1].split(",")
                              # Deduplicate references to the condition functions for the expressions
   # and save the index for the order they were added.
            self.sources = [ Value.create(src, "{0}_{1}".format(name_base, i), varset, algebraic_pass)
            # nir_search_expression::srcs is hard-coded to 4
            if self.opcode in conv_opcode_types:
      assert self._bit_size is None, \
                     def equivalent(self, other):
               This is check is much weaker than equality.  One generally cannot be
   used in place of the other.  Using this implementation for the __eq__
            This implementation does not check for equivalence due to commutativity,
            """
   if not isinstance(other, type(self)):
            if len(self.sources) != len(other.sources):
            if self.opcode != other.opcode:
                  def __index_comm_exprs(self, base_idx):
      """Recursively count and index commutative expressions
   """
            # A note about the explicit "len(self.sources)" check. The list of
   # sources comes from user input, and that input might be bad.  Check
   # that the expected second source exists before accessing it. Without
   # this check, a unit test that does "('iadd', 'a')" will crash.
   if self.opcode not in conv_opcode_types and \
      "2src_commutative" in opcodes[self.opcode].algebraic_properties and \
   len(self.sources) >= 2 and \
   not self.sources[0].equivalent(self.sources[1]):
   self.comm_expr_idx = base_idx
      else:
            for s in self.sources:
      if isinstance(s, Expression):
                     def c_opcode(self):
            def render(self, cache):
      srcs = "".join(src.render(cache) for src in self.sources)
      class BitSizeValidator(object):
               NIR supports multiple bit-sizes on expressions in order to handle things
   such as fp64.  The source and destination of every ALU operation is
   assigned a type and that type may or may not specify a bit size.  Sources
   and destinations whose type does not specify a bit size are considered
   "unsized" and automatically take on the bit size of the corresponding
   register or SSA value.  NIR has two simple rules for bit sizes that are
            1) A given SSA def or register has a single bit size that is respected by
            2) The bit sizes of all unsized inputs/outputs on any given ALU
      instruction must match.  They need not match the sized inputs or
         In order to keep nir_algebraic relatively simple and easy-to-use,
   nir_search supports a type of bit-size inference based on the two rules
   above.  This is similar to type inference in many common programming
   languages.  If, for instance, you are constructing an add operation and you
   know the second source is 16-bit, then you know that the other source and
   the destination must also be 16-bit.  There are, however, cases where this
   inference can be ambiguous or contradictory.  Consider, for instance, the
                     This transformation can potentially cause a problem because usub_borrow is
   well-defined for any bit-size of integer.  However, b2i always generates a
   32-bit result so it could end up replacing a 64-bit expression with one
   that takes two 64-bit values and produces a 32-bit value.  As another
                     In this case, in the search expression a must be 32-bit but b can
   potentially have any bit size.  If we had a 64-bit b value, we would end up
            This class solves that problem by providing a validation layer that proves
   that a given search-and-replace operation is 100% well-defined before we
   generate any code.  This ensures that bugs are caught at compile time
            Each value maintains a "bit-size class", which is either an actual bit size
   or an equivalence class with other values that must have the same bit size.
   The validator works by combining bit-size classes with each other according
   to the NIR rules outlined above, checking that there are no inconsistencies.
   When doing this for the replacement expression, we make sure to never change
   the equivalence class of any of the search values. We could make the example
   transforms above work by doing some extra run-time checking of the search
   expression, but we make the user specify those constraints themselves, to
   avoid any surprises. Since the replacement bitsizes can only be connected to
   the source bitsize via variables (variables must have the same bitsize in
   the source and replacment expressions) or the roots of the expression (the
   replacement expression must produce the same bit size as the search
   expression), we prevent merging a variable with anything when processing the
   replacement expression, or specializing the search bitsize
                     from being allowed, since we'd have to merge the bitsizes for a and b due to
                     from being allowed, since the search expression has the bit size of a and b,
   which can't be specialized to 32 which is the bitsize of the replace
                     since the bitsize of 'b2i', which can be anything, can't be specialized to
            After doing all this, we check that every subexpression of the replacement
   was assigned a constant bitsize, the bitsize of a variable, or the bitsize
   of the search expresssion, since those are the things that are known when
   constructing the replacement expresssion. Finally, we record the bitsize
   needed in nir_search_value so that we know what to do when building the
   replacement expression.
            def __init__(self, varset):
            def compare_bitsizes(self, a, b):
      """Determines which bitsize class is a specialization of the other, or
   whether neither is. When we merge two different bitsizes, the
   less-specialized bitsize always points to the more-specialized one, so
   that calling get_bit_size() always gets you the most specialized bitsize.
   The specialization partial order is given by:
   - Physical bitsizes are always the most specialized, and a different
   bitsize can never specialize another.
   - In the search expression, variables can always be specialized to each
   other and to physical bitsizes. In the replace expression, we disallow
   this to avoid adding extra constraints to the search expression that
   the user didn't specify.
   - Expressions and constants without a bitsize can always be specialized to
            We return -1 if a <= b (b can be specialized to a), 0 if a = b, 1 if a >= b,
   and None if they are not comparable (neither a <= b nor b <= a).
   """
   if isinstance(a, int):
      if isinstance(b, int):
         elif isinstance(b, Variable):
         else:
      elif isinstance(a, Variable):
      if isinstance(b, int):
         elif isinstance(b, Variable):
         else:
      else:
      if isinstance(b, int):
         elif isinstance(b, Variable):
                  def unify_bit_size(self, a, b, error_msg):
      """Record that a must have the same bit-size as b. If both
   have been assigned conflicting physical bit-sizes, call "error_msg" with
   the bit-sizes of self and other to get a message and raise an error.
   In the replace expression, disallow merging variables with other
   variables and physical bit-sizes as well.
   """
   a_bit_size = a.get_bit_size()
                     assert cmp_result is not None, \
            if cmp_result < 0:
         elif not isinstance(a_bit_size, int):
         def merge_variables(self, val):
      """Perform the first part of type inference by merging all the different
   uses of the same variable. We always do this as if we're in the search
   expression, even if we're actually not, since otherwise we'd get errors
   if the search expression specified some constraint but the replace
   expression didn't, because we'd be merging a variable and a constant.
   """
   if isinstance(val, Variable):
      if self._var_classes[val.index] is None:
         else:
      other = self._var_classes[val.index]
   self.unify_bit_size(other, val,
         lambda other_bit_size, bit_size:
         elif isinstance(val, Expression):
               def validate_value(self, val):
      """Validate the an expression by performing classic Hindley-Milner
   type inference on bitsizes. This will detect if there are any conflicting
   requirements, and unify variables so that we know which variables must
   have the same bitsize. If we're operating on the replace expression, we
   will refuse to merge different variables together or merge a variable
   with a constant, in order to prevent surprises due to rules unexpectedly
   not matching at runtime.
   """
   if not isinstance(val, Expression):
            # Generic conversion ops are special in that they have a single unsized
   # source and an unsized destination and the two don't have to match.
   # This means there's no validation or unioning to do here besides the
   # len(val.sources) check.
   if val.opcode in conv_opcode_types:
      assert len(val.sources) == 1, \
      "Expression {} has {} sources, expected 1".format(
                  nir_op = opcodes[val.opcode]
   assert len(val.sources) == nir_op.num_inputs, \
                  for src in val.sources:
                     # First, unify all the sources. That way, an error coming up because two
   # sources have an incompatible bit-size won't produce an error message
   # involving the destination.
   first_unsized_src = None
   for src_type, src in zip(nir_op.input_types, val.sources):
      src_type_bits = type_bits(src_type)
   if src_type_bits == 0:
      if first_unsized_src is None:
                  if self.is_search:
      self.unify_bit_size(first_unsized_src, src,
      lambda first_unsized_src_bit_size, src_bit_size:
      'Source {} of {} must have bit size {}, while source {} ' \
   'must have incompatible bit size {}'.format(
      else:
      self.unify_bit_size(first_unsized_src, src,
      lambda first_unsized_src_bit_size, src_bit_size:
      'Sources {} (bit size of {}) and {} (bit size of {}) ' \
   'of {} may not have the same bit size when building the ' \
   'replacement expression.'.format(
   else:
      if self.is_search:
      self.unify_bit_size(src, src_type_bits,
      lambda src_bit_size, unused:
      '{} must have {} bits, but as a source of nir_op_{} '\
      else:
      self.unify_bit_size(src, src_type_bits,
      lambda src_bit_size, unused:
                  if dst_type_bits == 0:
      if first_unsized_src is not None:
      if self.is_search:
      self.unify_bit_size(val, first_unsized_src,
      lambda val_bit_size, src_bit_size:
      '{} must have the bit size of {}, while its source {} ' \
      else:
      self.unify_bit_size(val, first_unsized_src,
      lambda val_bit_size, src_bit_size:
      '{} must have {} bits, but its source {} ' \
      else:
      self.unify_bit_size(val, dst_type_bits,
      lambda dst_bit_size, unused:
               def validate_replace(self, val, search):
      bit_size = val.get_bit_size()
   assert isinstance(bit_size, int) or isinstance(bit_size, Variable) or \
         bit_size == search.get_bit_size(), \
   'Ambiguous bit size for replacement value {}: ' \
            if isinstance(val, Expression):
      for src in val.sources:
      elif isinstance(val, Variable):
      # These catch problems when someone copies and pastes the search
   # into the replacement.
                                          def validate(self, search, replace):
      self.is_search = True
   self.merge_variables(search)
   self.merge_variables(replace)
            self.is_search = False
            # Check that search is always more specialized than replace. Note that
   # we're doing this in replace mode, disallowing merging variables.
   search_bit_size = search.get_bit_size()
   replace_bit_size = replace.get_bit_size()
            assert cmp_result is not None and cmp_result <= 0, \
      'The search expression bit size {} and replace expression ' \
                           _optimization_ids = itertools.count()
      condition_list = ['true']
      class SearchAndReplace(object):
      def __init__(self, transform, algebraic_pass):
               search = transform[0]
   replace = transform[1]
   if len(transform) > 2:
         else:
            if self.condition not in condition_list:
                  varset = VarSet()
   if isinstance(search, Expression):
         else:
                     if isinstance(replace, Value):
         else:
               class TreeAutomaton(object):
      """This class calculates a bottom-up tree automaton to quickly search for
   the left-hand sides of tranforms. Tree automatons are a generalization of
   classical NFA's and DFA's, where the transition function determines the
   state of the parent node based on the state of its children. We construct a
   deterministic automaton to match patterns, using a similar algorithm to the
   classical NFA to DFA construction. At the moment, it only matches opcodes
   and constants (without checking the actual value), leaving more detailed
   checking to the search function which actually checks the leaves. The
   automaton acts as a quick filter for the search function, requiring only n
   + 1 table lookups for each n-source operation. The implementation is based
   on the theory described in "Tree Automatons: Two Taxonomies and a Toolkit."
   In the language of that reference, this is a frontier-to-root deterministic
   automaton using only symbol filtering. The filtering is crucial to reduce
   both the time taken to generate the tables and the size of the tables.
   """
   def __init__(self, transforms):
      self.patterns = [t.search for t in transforms]
   self._compute_items()
   self._build_table()
   #print('num items: {}'.format(len(set(self.items.values()))))
   #print('num states: {}'.format(len(self.states)))
   #for state, patterns in zip(self.states, self.patterns):
         class IndexMap(object):
      """An indexed list of objects, where one can either lookup an object by
   index or find the index associated to an object quickly using a hash
   table. Compared to a list, it has a constant time index(). Compared to a
   set, it provides a stable iteration order.
   """
   def __init__(self, iterable=()):
      self.objects = []
   self.map = {}
               def __getitem__(self, i):
            def __contains__(self, obj):
            def __len__(self):
            def __iter__(self):
            def clear(self):
                  def index(self, obj):
            def add(self, obj):
      if obj in self.map:
         else:
      index = len(self.objects)
   self.objects.append(obj)
            def __repr__(self):
         class Item(object):
      """This represents an "item" in the language of "Tree Automatons." This
   is just a subtree of some pattern, which represents a potential partial
   match at runtime. We deduplicate them, so that identical subtrees of
   different patterns share the same object, and store some extra
   information needed for the main algorithm as well.
   """
   def __init__(self, opcode, children):
      self.opcode = opcode
   self.children = children
   # These are the indices of patterns for which this item is the root node.
   self.patterns = []
   # This the set of opcodes for parents of this item. Used to speed up
               def __str__(self):
            def __repr__(self):
         def _compute_items(self):
      """Build a set of all possible items, deduplicating them."""
   # This is a map from (opcode, sources) to item.
            # The set of all opcodes used by the patterns. Used later to avoid
   # building and emitting all the tables for opcodes that aren't used.
            def get_item(opcode, children, pattern=None):
      commutative = len(children) >= 2 \
         item = self.items.setdefault((opcode, children),
         if commutative:
         if pattern is not None:
               self.wildcard = get_item("__wildcard", ())
            def process_subpattern(src, pattern=None):
      if isinstance(src, Constant):
      # Note: we throw away the actual constant value!
      elif isinstance(src, Variable):
      if src.is_constant:
         else:
      # Note: we throw away which variable it is here! This special
   # item is equivalent to nu in "Tree Automatons."
   else:
      assert isinstance(src, Expression)
   opcode = src.opcode
   stripped = opcode.rstrip('0123456789')
   if stripped in conv_opcode_types:
      # Matches that use conversion opcodes with a specific type,
   # like f2i1, are tricky.  Either we construct the automaton to
   # match specific NIR opcodes like nir_op_f2i1, in which case we
   # need to create separate items for each possible NIR opcode
   # for patterns that have a generic opcode like f2i, or we
   # construct it to match the search opcode, in which case we
   # need to map f2i1 to f2i when constructing the automaton. Here
   # we do the latter.
      self.opcodes.add(opcode)
   children = tuple(process_subpattern(c) for c in src.sources)
   item = get_item(opcode, children, pattern)
   for i, child in enumerate(children):
            for i, pattern in enumerate(self.patterns):
         def _build_table(self):
      """This is the core algorithm which builds up the transition table. It
   is based off of Algorithm 5.7.38 "Reachability-based tabulation of Cl .
   Comp_a and Filt_{a,i} using integers to identify match sets." It
   simultaneously builds up a list of all possible "match sets" or
   "states", where each match set represents the set of Item's that match a
   given instruction, and builds up the transition table between states.
   """
   # Map from opcode + filtered state indices to transitioned state.
   self.table = defaultdict(dict)
   # Bijection from state to index. q in the original algorithm is
   # len(self.states)
   self.states = self.IndexMap()
   # Lists of pattern matches separated by None
   self.state_patterns = [None]
   # Offset in the ->transforms table for each state index
   self.state_pattern_offsets = []
   # Map from state index to filtered state index for each opcode.
   self.filter = defaultdict(list)
   # Bijections from filtered state to filtered state index for each
   # opcode, called the "representor sets" in the original algorithm.
   # q_{a,j} in the original algorithm is len(self.rep[op]).
            # Everything in self.states with a index at least worklist_index is part
   # of the worklist of newly created states. There is also a worklist of
   # newly fitered states for each opcode, for which worklist_indices
   # serves a similar purpose. worklist_index corresponds to p in the
   # original algorithm, while worklist_indices is p_{a,j} (although since
   # we only filter by opcode/symbol, it's really just p_a).
   self.worklist_index = 0
            # This is the set of opcodes for which the filtered worklist is non-empty.
   # It's used to avoid scanning opcodes for which there is nothing to
   # process when building the transition table. It corresponds to new_a in
   # the original algorithm.
            # Process states on the global worklist, filtering them for each opcode,
   # updating the filter tables, and updating the filtered worklists if any
   # new filtered states are found. Similar to ComputeRepresenterSets() in
   # the original algorithm, although that only processes a single state.
   def process_new_states():
      while self.worklist_index < len(self.states):
      state = self.states[self.worklist_index]
   # Calculate pattern matches for this state. Each pattern is
   # assigned to a unique item, so we don't have to worry about
   # deduplicating them here. However, we do have to sort them so
   # that they're visited at runtime in the order they're specified
                  if patterns:
      # Add our patterns to the global table.
   self.state_pattern_offsets.append(len(self.state_patterns))
   self.state_patterns.extend(patterns)
      else:
                  # calculate filter table for this state, and update filtered
   # worklists.
   for op in self.opcodes:
      filt = self.filter[op]
   rep = self.rep[op]
   filtered = frozenset(item for item in state if \
         if filtered in rep:
         else:
      rep_index = rep.add(filtered)
                  # There are two start states: one which can only match as a wildcard,
   # and one which can match as a wildcard or constant. These will be the
   # states of intrinsics/other instructions and load_const instructions,
   # respectively. The indices of these must match the definitions of
   # WILDCARD_STATE and CONST_STATE below, so that the runtime C code can
   # initialize things correctly.
   self.states.add(frozenset((self.wildcard,)))
   self.states.add(frozenset((self.const,self.wildcard)))
            while len(new_opcodes) > 0:
      for op in new_opcodes:
      rep = self.rep[op]
   table = self.table[op]
   op_worklist_index = worklist_indices[op]
   if op in conv_opcode_types:
                        # Iterate over all possible source combinations where at least one
   # is on the worklist.
   for src_indices in itertools.product(range(len(rep)), repeat=num_srcs):
                              # Try all possible pairings of source items and add the
                                                         _algebraic_pass_template = mako.template.Template("""
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_search.h"
   #include "nir_search_helpers.h"
      /* What follows is NIR algebraic transform code for the following ${len(xforms)}
   * transforms:
   % for xform in xforms:
   *    ${xform.search} => ${xform.replace}
   % endfor
   */
      <% cache = {"next_index": 0} %>
   static const nir_search_value_union ${pass_name}_values[] = {
   % for xform in xforms:
         ${xform.search.render(cache)}
   ${xform.replace.render(cache)}
   % endfor
   };
      % if expression_cond:
   static const nir_search_expression_cond ${pass_name}_expression_cond[] = {
   % for cond in expression_cond:
         % endfor
   };
   % endif
      % if variable_cond:
   static const nir_search_variable_cond ${pass_name}_variable_cond[] = {
   % for cond in variable_cond:
         % endfor
   };
   % endif
      static const struct transform ${pass_name}_transforms[] = {
   % for i in automaton.state_patterns:
   % if i is not None:
         % else:
            % endif
   % endfor
   };
      static const struct per_op_table ${pass_name}_pass_op_table[nir_num_search_ops] = {
   % for op in automaton.opcodes:
         % if all(e == 0 for e in automaton.filter[op]):
         % else:
         .filter = (const uint16_t []) {
   % for e in automaton.filter[op]:
         % endfor
   % endif
         <%
   num_filtered = len(automaton.rep[op])
   %>
   .num_filtered_states = ${num_filtered},
   .table = (const uint16_t []) {
   <%
   num_srcs = len(next(iter(automaton.table[op])))
   %>
   % for indices in itertools.product(range(num_filtered), repeat=num_srcs):
         % endfor
         % endfor
   };
      /* Mapping from state index to offset in transforms (0 being no transforms) */
   static const uint16_t ${pass_name}_transform_offsets[] = {
   % for offset in automaton.state_pattern_offsets:
         % endfor
   };
      static const nir_algebraic_table ${pass_name}_table = {
      .transforms = ${pass_name}_transforms,
   .transform_offsets = ${pass_name}_transform_offsets,
   .pass_op_table = ${pass_name}_pass_op_table,
   .values = ${pass_name}_values,
   .expression_cond = ${ pass_name + "_expression_cond" if expression_cond else "NULL" },
      };
      bool
   ${pass_name}(nir_shader *shader)
   {
      bool progress = false;
   bool condition_flags[${len(condition_list)}];
   const nir_shader_compiler_options *options = shader->options;
   const shader_info *info = &shader->info;
   (void) options;
            STATIC_ASSERT(${str(cache["next_index"])} == ARRAY_SIZE(${pass_name}_values));
   % for index, condition in enumerate(condition_list):
   condition_flags[${index}] = ${condition};
            nir_foreach_function_impl(impl, shader) {
   progress |= nir_algebraic_impl(impl, condition_flags, &${pass_name}_table);
               }
   """)
         class AlgebraicPass(object):
      def __init__(self, pass_name, transforms):
      self.xforms = []
   self.opcode_xforms = defaultdict(lambda : [])
   self.pass_name = pass_name
   self.expression_cond = {}
                     for xform in transforms:
      if not isinstance(xform, SearchAndReplace):
      try:
         except:
      print("Failed to parse transformation:", file=sys.stderr)
   print("  " + str(xform), file=sys.stderr)
   traceback.print_exc(file=sys.stderr)
                  self.xforms.append(xform)
   if xform.search.opcode in conv_opcode_types:
      dst_type = conv_opcode_types[xform.search.opcode]
   for size in type_sizes(dst_type):
      sized_opcode = xform.search.opcode + str(size)
                  # Check to make sure the search pattern does not unexpectedly contain
   # more commutative expressions than match_expression (nir_search.c)
                  if xform.search.many_commutative_expressions:
      if comm_exprs <= nir_search_max_comm_ops:
      print("Transform expected to have too many commutative " \
         "expression but did not " \
   "({} <= {}).".format(comm_exprs, nir_search_max_comm_op),
   print("  " + str(xform), file=sys.stderr)
   traceback.print_exc(file=sys.stderr)
   print('', file=sys.stderr)
   else:
      if comm_exprs > nir_search_max_comm_ops:
      print("Transformation with too many commutative expressions " \
         "({} > {}).  Modify pattern or annotate with " \
   "\"many-comm-expr\".".format(comm_exprs,
                              if error:
            def render(self):
      return _algebraic_pass_template.render(pass_name=self.pass_name,
                                                # The replacement expression isn't necessarily exact if the search expression is exact.
   def ignore_exact(*expr):
      expr = SearchExpression.create(expr)
   expr.ignore_exact = True
   return expr
