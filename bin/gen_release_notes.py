   #!/usr/bin/env python3
   # Copyright © 2019-2020 Intel Corporation
      # Permission is hereby granted, free of charge, to any person obtaining a copy
   # of this software and associated documentation files (the "Software"), to deal
   # in the Software without restriction, including without limitation the rights
   # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   # copies of the Software, and to permit persons to whom the Software is
   # furnished to do so, subject to the following conditions:
      # The above copyright notice and this permission notice shall be included in
   # all copies or substantial portions of the Software.
      # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   # SOFTWARE.
      """Generates release notes for a given version of mesa."""
      import asyncio
   import datetime
   import os
   import pathlib
   import re
   import subprocess
   import sys
   import textwrap
   import typing
   import urllib.parse
      import aiohttp
   from mako.template import Template
   from mako import exceptions
      import docutils.utils
   import docutils.parsers.rst.states as states
      CURRENT_GL_VERSION = '4.6'
   CURRENT_VK_VERSION = '1.3'
      TEMPLATE = Template(textwrap.dedent("""\
      ${header}
            %if not bugfix:
   Mesa ${this_version} is a new development release. People who are concerned
   with stability and reliability should stick with a previous release or
   wait for Mesa ${this_version[:-1]}1.
   %else:
   Mesa ${this_version} is a bug fix release which fixes bugs found since the ${previous_version} release.
            Mesa ${this_version} implements the OpenGL ${gl_version} API, but the version reported by
   glGetString(GL_VERSION) or glGetIntegerv(GL_MAJOR_VERSION) /
   glGetIntegerv(GL_MINOR_VERSION) depends on the particular driver being used.
   Some drivers don't support all the features required in OpenGL ${gl_version}. OpenGL
   ${gl_version} is **only** available if requested at context creation.
            Mesa ${this_version} implements the Vulkan ${vk_version} API, but the version reported by
   the apiVersion property of the VkPhysicalDeviceProperties struct
            SHA256 checksum
                                 New features
            %for f in features:
   - ${rst_escape(f)}
               Bug fixes
            %for b in bugs:
   - ${rst_escape(b)}
               Changes
   -------
   %for c, author_line in changes:
                           - ${rst_escape(c)}
         %endfor
            # copied from https://docutils.sourceforge.io/sandbox/xml2rst/xml2rstlib/markup.py
   class Inliner(states.Inliner):
      """
   Recognizer for inline markup. Derive this from the original inline
   markup parser for best results.
            # Copy static attributes from super class
            def quoteInline(self, text):
      """
   `text`: ``str``
         """
   # Method inspired by `states.Inliner.parse`
   self.document = docutils.utils.new_document("<string>")
   self.document.settings.trim_footnote_reference_space = False
   self.document.settings.character_level_inline_markup = False
   self.document.settings.pep_references = False
                     self.reporter = self.document.reporter
   self.reporter.stream = None
   self.language = None
   self.parent = self.document
   remaining = docutils.utils.escape2null(text)
   checked = ""
   processed = []
   unprocessed = []
   messages = []
   while remaining:
         original = remaining
   match = self.patterns.initial.search(remaining)
   if match:
      groups = match.groupdict()
   method = self.dispatch[groups['start'] or groups['backquote']
         before, inlines, remaining, sysmessages = method(self, match, 0)
   checked += before
   if inlines:
      assert len(inlines) == 1, "More than one inline found"
   inline = original[len(before)
         rolePfx = re.search("^:" + self.simplename + ":(?=`)",
         refSfx = re.search("_+$", inline)
   if rolePfx:
         # Prefixed roles need to be quoted in the middle
   checked += (inline[:rolePfx.end()] + "\\"
   elif refSfx and not re.search("^`", inline):
         # Pure reference markup needs to be quoted at the end
   checked += (inline[:refSfx.start()] + "\\"
   else:
         else:
         # Quote all original backslashes
   checked = re.sub('\x00', "\\\x00", checked)
   checked = re.sub('@', '\\@', checked)
      inliner = Inliner();
         async def gather_commits(version: str) -> str:
      p = await asyncio.create_subprocess_exec(
      'git', 'log', '--oneline', f'mesa-{version}..', '-i', '--grep', r'\(Closes\|Fixes\): \(https\|#\).*',
      out, _ = await p.communicate()
   assert p.returncode == 0, f"git log didn't work: {version}"
            async def parse_issues(commits: str) -> typing.List[str]:
      issues: typing.List[str] = []
   for commit in commits.split('\n'):
      sha, message = commit.split(maxsplit=1)
   p = await asyncio.create_subprocess_exec(
         'git', 'log', '--max-count', '1', r'--format=%b', sha,
   _out, _ = await p.communicate()
            for line in reversed(out):
         if not line.lower().startswith(('closes:', 'fixes:')):
         bug = line.split(':', 1)[1].strip()
   if (bug.startswith('https://gitlab.freedesktop.org/mesa/mesa')
      # Avoid parsing "merge_requests" URL. Note that a valid issue
   # URL may or may not contain the "/-/" text, so we check if
   # the word "issues" is contained in URL.
   and '/issues' in bug):
   # This means we have a bug in the form "Closes: https://..."
      elif ',' in bug:
      multiple_bugs = [b.strip().lstrip('#') for b in bug.split(',')]
   if not all(b.isdigit() for b in multiple_bugs):
      # this is likely a "Fixes" tag that refers to a commit name
                        async def gather_bugs(version: str) -> typing.List[str]:
      commits = await gather_commits(version)
   if commits:
         else:
            loop = asyncio.get_event_loop()
   async with aiohttp.ClientSession(loop=loop) as session:
         typing.cast(typing.Tuple[str, ...], results)
   bugs = list(results)
   if not bugs:
                  async def get_bug(session: aiohttp.ClientSession, bug_id: str) -> str:
      """Query gitlab to get the name of the issue that was closed."""
   # Mesa's gitlab id is 176,
   url = 'https://gitlab.freedesktop.org/api/v4/projects/176/issues'
   params = {'iids[]': bug_id}
   async with session.get(url, params=params) as response:
         if not content:
      # issues marked as "confidential" look like "404" page for
   # unauthorized users
      else:
            async def get_shortlog(version: str) -> str:
      """Call git shortlog."""
   p = await asyncio.create_subprocess_exec('git', 'shortlog', f'mesa-{version}..',
         out, _ = await p.communicate()
   assert p.returncode == 0, 'error getting shortlog'
   assert out is not None, 'just for mypy'
            def walk_shortlog(log: str) -> typing.Generator[typing.Tuple[str, bool], None, None]:
      for l in log.split('\n'):
      if l.startswith(' '): # this means we have a patch description
         elif l.strip():
         def calculate_next_version(version: str, is_point: bool) -> str:
      """Calculate the version about to be released."""
   if '-' in version:
         if is_point:
      base = version.split('.')
   base[2] = str(int(base[2]) + 1)
               def calculate_previous_version(version: str, is_point: bool) -> str:
               In the case of -rc to final that version is the previous .0 release,
   (19.3.0 in the case of 20.0.0, for example). for point releases that is
   the last point release. This value will be the same as the input value
   for a point release, but different for a major release.
   """
   if '-' in version:
         if is_point:
         base = version.split('.')
   if base[1] == '0':
      base[0] = str(int(base[0]) - 1)
      else:
                  def get_features(is_point_release: bool) -> typing.Generator[str, None, None]:
      p = pathlib.Path('docs') / 'relnotes' / 'new_features.txt'
   if p.exists() and p.stat().st_size > 0:
      if is_point_release:
         with p.open('rt') as f:
         for line in f:
      else:
            def update_release_notes_index(version: str) -> None:
               with relnotes_index_path.open('r') as f:
            new_relnotes = []
   first_list = True
   second_list = True
   for line in relnotes:
      if first_list and line.startswith('-'):
         first_list = False
   if (not first_list and second_list and
         re.match(r'   \d+.\d+(.\d+)? <relnotes/\d+.\d+(.\d+)?>', line)):
   second_list = False
         with relnotes_index_path.open('w') as f:
      for line in new_relnotes:
                  async def main() -> None:
      v = pathlib.Path('VERSION')
   with v.open('rt') as f:
         is_point_release = '-rc' not in raw_version
   assert '-devel' not in raw_version, 'Do not run this script on -devel'
   version = raw_version.split('-')[0]
   previous_version = calculate_previous_version(version, is_point_release)
   this_version = calculate_next_version(version, is_point_release)
   today = datetime.date.today()
   header = f'Mesa {this_version} Release Notes / {today}'
            shortlog, bugs = await asyncio.gather(
      get_shortlog(previous_version),
               final = pathlib.Path('docs') / 'relnotes' / f'{this_version}.rst'
   with final.open('wt') as f:
      try:
         f.write(TEMPLATE.render(
      bugfix=is_point_release,
   bugs=bugs,
   changes=walk_shortlog(shortlog),
   features=get_features(is_point_release),
   gl_version=CURRENT_GL_VERSION,
   this_version=this_version,
   header=header,
   header_underline=header_underline,
   previous_version=previous_version,
   vk_version=CURRENT_VK_VERSION,
      except:
                                 subprocess.run(['git', 'commit', '-m',
            if __name__ == "__main__":
      loop = asyncio.get_event_loop()
   loop.run_until_complete(main())
