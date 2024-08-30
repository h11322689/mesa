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
      """Core data structures and routines for pick."""
      import asyncio
   import enum
   import json
   import pathlib
   import re
   import subprocess
   import typing
      import attr
      if typing.TYPE_CHECKING:
                                    sha: str
   description: str
   nominated: bool
   nomination_type: int
   resolution: typing.Optional[int]
   main_sha: typing.Optional[str]
   because_sha: typing.Optional[str]
      IS_FIX = re.compile(r'^\s*fixes:\s*([a-f0-9]{6,40})', flags=re.MULTILINE | re.IGNORECASE)
   # FIXME: I dislike the duplication in this regex, but I couldn't get it to work otherwise
   IS_CC = re.compile(r'^\s*cc:\s*["\']?([0-9]{2}\.[0-9])?["\']?\s*["\']?([0-9]{2}\.[0-9])?["\']?\s*\<?mesa-stable',
         IS_REVERT = re.compile(r'This reverts commit ([0-9a-f]{40})')
   IS_BACKPORT = re.compile(r'^\s*backport-to:\s*(\d{2}\.\d),?\s*(\d{2}\.\d)?',
            # XXX: hack
   SEM = asyncio.Semaphore(50)
      COMMIT_LOCK = asyncio.Lock()
      git_toplevel = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'],
         pick_status_json = pathlib.Path(git_toplevel) / '.pick_status.json'
         class PickUIException(Exception):
               @enum.unique
   class NominationType(enum.Enum):
         CC = 0
   FIXES = 1
   REVERT = 2
   NONE = 3
            @enum.unique
   class Resolution(enum.Enum):
         UNRESOLVED = 0
   MERGED = 1
   DENOMINATED = 2
   BACKPORTED = 3
            async def commit_state(*, amend: bool = False, message: str = 'Update') -> bool:
      """Commit the .pick_status.json file."""
   async with COMMIT_LOCK:
      p = await asyncio.create_subprocess_exec(
         'git', 'add', pick_status_json.as_posix(),
   stdout=asyncio.subprocess.DEVNULL,
   )
   v = await p.wait()
   if v != 0:
            if amend:
         else:
         p = await asyncio.create_subprocess_exec(
         'git', 'commit', *cmd,
   stdout=asyncio.subprocess.DEVNULL,
   )
   v = await p.wait()
   if v != 0:
               @attr.s(slots=True)
   class Commit:
         sha: str = attr.ib()
   description: str = attr.ib()
   nominated: bool = attr.ib(False)
   nomination_type: NominationType = attr.ib(NominationType.NONE)
   resolution: Resolution = attr.ib(Resolution.UNRESOLVED)
   main_sha: typing.Optional[str] = attr.ib(None)
   because_sha: typing.Optional[str] = attr.ib(None)
            def to_json(self) -> 'CommitDict':
      d: typing.Dict[str, typing.Any] = attr.asdict(self)
   d['nomination_type'] = self.nomination_type.value
   if self.resolution is not None:
               @classmethod
   def from_json(cls, data: 'CommitDict') -> 'Commit':
      c = cls(data['sha'], data['description'], data['nominated'], main_sha=data['main_sha'],
         c.nomination_type = NominationType(data['nomination_type'])
   if data['resolution'] is not None:
               def date(self) -> str:
      # Show commit date, ie. when the commit actually landed
   # (as opposed to when it was first written)
   return subprocess.check_output(
         ['git', 'show', '--no-patch', '--format=%cs', self.sha],
         async def apply(self, ui: 'UI') -> typing.Tuple[bool, str]:
      # FIXME: This isn't really enough if we fail to cherry-pick because the
   # git tree will still be dirty
   async with COMMIT_LOCK:
         p = await asyncio.create_subprocess_exec(
      'git', 'cherry-pick', '-x', self.sha,
   stdout=asyncio.subprocess.DEVNULL,
               if p.returncode != 0:
            self.resolution = Resolution.MERGED
            # Append the changes to the .pickstatus.json file
   ui.save()
   v = await commit_state(amend=True)
         async def abort_cherry(self, ui: 'UI', err: str) -> None:
      await ui.feedback(f'{self.sha} ({self.description}) failed to apply\n{err}')
   async with COMMIT_LOCK:
         p = await asyncio.create_subprocess_exec(
      'git', 'cherry-pick', '--abort',
   stdout=asyncio.subprocess.DEVNULL,
      )
         async def denominate(self, ui: 'UI') -> bool:
      self.resolution = Resolution.DENOMINATED
   ui.save()
   v = await commit_state(message=f'Mark {self.sha} as denominated')
   assert v
   await ui.feedback(f'{self.sha} ({self.description}) denominated successfully')
         async def backport(self, ui: 'UI') -> bool:
      self.resolution = Resolution.BACKPORTED
   ui.save()
   v = await commit_state(message=f'Mark {self.sha} as backported')
   assert v
   await ui.feedback(f'{self.sha} ({self.description}) backported successfully')
         async def resolve(self, ui: 'UI') -> None:
      self.resolution = Resolution.MERGED
   ui.save()
   v = await commit_state(amend=True)
   assert v
         async def update_notes(self, ui: 'UI', notes: typing.Optional[str]) -> None:
      self.notes = notes
   async with ui.git_lock:
         ui.save()
   assert v
         async def get_new_commits(sha: str) -> typing.List[typing.Tuple[str, str]]:
      # Try to get the authoritative upstream main
   p = await asyncio.create_subprocess_exec(
      'git', 'for-each-ref', '--format=%(upstream)', 'refs/heads/main',
   stdout=asyncio.subprocess.PIPE,
      out, _ = await p.communicate()
            p = await asyncio.create_subprocess_exec(
      'git', 'log', '--pretty=oneline', f'{sha}..{upstream}',
   stdout=asyncio.subprocess.PIPE,
      out, _ = await p.communicate()
   assert p.returncode == 0, f"git log didn't work: {sha}"
            def split_commit_list(commits: str) -> typing.Generator[typing.Tuple[str, str], None, None]:
      if not commits:
         for line in commits.split('\n'):
      v = tuple(line.split(' ', 1))
   assert len(v) == 2, 'this is really just for mypy'
         async def is_commit_in_branch(sha: str) -> bool:
      async with SEM:
      p = await asyncio.create_subprocess_exec(
         'git', 'merge-base', '--is-ancestor', sha, 'HEAD',
   stdout=asyncio.subprocess.DEVNULL,
   )
               async def full_sha(sha: str) -> str:
      async with SEM:
      p = await asyncio.create_subprocess_exec(
         'git', 'rev-parse', sha,
   stdout=asyncio.subprocess.PIPE,
   )
      if p.returncode:
                  async def resolve_nomination(commit: 'Commit', version: str) -> 'Commit':
      async with SEM:
      p = await asyncio.create_subprocess_exec(
         'git', 'log', '--format=%B', '-1', commit.sha,
   stdout=asyncio.subprocess.PIPE,
   )
   _out, _ = await p.communicate()
               # We give precedence to fixes and cc tags over revert tags.
   if fix_for_commit := IS_FIX.search(out):
      # We set the nomination_type and because_sha here so that we can later
   # check to see if this fixes another staged commit.
   try:
         except PickUIException:
         else:
         commit.nomination_type = NominationType.FIXES
   if await is_commit_in_branch(fixed):
         if backport_to := IS_BACKPORT.search(out):
      if version in backport_to.groups():
         commit.nominated = True
         if cc_to := IS_CC.search(out):
      if cc_to.groups() == (None, None) or version in cc_to.groups():
         commit.nominated = True
         if revert_of := IS_REVERT.search(out):
      # See comment for IS_FIX path
   try:
         except PickUIException:
         else:
         commit.nomination_type = NominationType.REVERT
   if await is_commit_in_branch(reverted):
                  async def resolve_fixes(commits: typing.List['Commit'], previous: typing.List['Commit']) -> None:
               The are still needed if they apply to a commit that is staged for
            This must be done in order, because a commit 3 might fix commit 2 which
   fixes commit 1.
   """
   shas: typing.Set[str] = set(c.sha for c in previous if c.nominated)
            for commit in reversed(commits):
      if not commit.nominated and commit.nomination_type is NominationType.FIXES:
            if commit.nominated:
         for commit in commits:
      if (commit.nomination_type is NominationType.REVERT and
               for oldc in reversed(commits):
      if oldc.sha == commit.because_sha:
      # In this case a commit that hasn't yet been applied is
   # reverted, we don't want to apply that commit at all
   oldc.nominated = False
   oldc.resolution = Resolution.DENOMINATED
   commit.nominated = False
         async def gather_commits(version: str, previous: typing.List['Commit'],
            # We create an array of the final size up front, then we pass that array
   # to the "inner" co-routine, which is turned into a list of tasks and
   # collected by asyncio.gather. We do this to allow the tasks to be
   # asynchronously gathered, but to also ensure that the commits list remains
   # in order.
   m_commits: typing.List[typing.Optional['Commit']] = [None] * len(new)
            async def inner(commit: 'Commit', version: str,
                  commits[index] = await resolve_nomination(commit, version)
         for i, (sha, desc) in enumerate(new):
      tasks.append(asyncio.ensure_future(
         await asyncio.gather(*tasks)
   assert None not in m_commits
                     for commit in commits:
      if commit.resolution is Resolution.UNRESOLVED and not commit.nominated:
                  def load() -> typing.List['Commit']:
      if not pick_status_json.exists():
         with pick_status_json.open('r') as f:
      raw = json.load(f)
         def save(commits: typing.Iterable['Commit']) -> None:
      commits = list(commits)
   with pick_status_json.open('wt') as f:
            asyncio.ensure_future(commit_state(message=f'Update to {commits[0].sha}'))
