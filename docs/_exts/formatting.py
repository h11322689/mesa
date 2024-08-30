   # formatting.py
   # Sphinx extension providing formatting for Gallium-specific data
   # (c) Corbin Simpson 2010
   # Public domain to the extent permitted; contact author for special licensing
      import docutils.nodes
   import sphinx.addnodes
      from sphinx.util.nodes import split_explicit_title
   from docutils import nodes, utils
      def parse_opcode(env, sig, signode):
      opcode, desc = sig.split("-", 1)
   opcode = opcode.strip().upper()
   desc = " (%s)" % desc.strip()
   signode += sphinx.addnodes.desc_name(opcode, opcode)
   signode += sphinx.addnodes.desc_annotation(desc, desc)
            def ext_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
      text = utils.unescape(text)
            parts = ext.split('_', 2)
   if parts[0] == 'VK':
         elif parts[0] == 'GL':
         else:
            pnode = nodes.reference(title, title, internal=False, refuri=full_url)
         def vkfeat_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
      text = utils.unescape(text)
                     pnode = nodes.reference(title, title, internal=False, refuri=full_url)
         def setup(app):
      app.add_object_type("opcode", "opcode", "%s (TGSI opcode)",
         app.add_role('ext', ext_role)
   app.add_role('vk-feat', vkfeat_role)
