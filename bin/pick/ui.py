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
      """Urwid UI for pick script."""
      import asyncio
   import itertools
   import textwrap
   import typing
      import attr
   import urwid
      from . import core
      if typing.TYPE_CHECKING:
            PALETTE = [
      ('a', 'black', 'light gray'),
   ('b', 'black', 'dark red'),
   ('bg', 'black', 'dark blue'),
      ]
         class RootWidget(urwid.Frame):
         def __init__(self, *args, ui: 'UI', **kwargs):
      super().__init__(*args, **kwargs)
         class CommitList(urwid.ListBox):
         def __init__(self, *args, ui: 'UI', **kwargs):
      super().__init__(*args, **kwargs)
         def keypress(self, size: int, key: str) -> typing.Optional[str]:
      if key == 'q':
         elif key == 'u':
         elif key == 'a':
         else:
               class CommitWidget(urwid.Text):
         # urwid.Text is normally not interactable, this is required to tell urwid
   # to use our keypress method
            def __init__(self, ui: 'UI', commit: 'core.Commit'):
      reason = commit.nomination_type.name.ljust(6)
   super().__init__(f'{commit.date()} {reason} {commit.sha[:10]} {commit.description}')
   self.ui = ui
         async def apply(self) -> None:
      async with self.ui.git_lock:
         result, err = await self.commit.apply(self.ui)
   if not result:
               async def denominate(self) -> None:
      async with self.ui.git_lock:
               async def backport(self) -> None:
      async with self.ui.git_lock:
               def keypress(self, size: int, key: str) -> typing.Optional[str]:
      if key == 'c':
         elif key == 'd':
         elif key == 'b':
         else:
               class FocusAwareEdit(urwid.Edit):
                           def __init__(self, *args, **kwargs):
      super().__init__(*args, **kwargs)
         def render(self, size: typing.Tuple[int], focus: bool = False) -> urwid.Canvas:
      if focus != self.__is_focus:
         self._emit("focus_changed", focus)
         @attr.s(slots=True)
   class UI:
                  :previous_commits: A list of commits to main since this branch was created
   :new_commits: Commits added to main since the last time this script was run
            commit_list: typing.List['urwid.Button'] = attr.ib(factory=lambda: urwid.SimpleFocusListWalker([]), init=False)
   feedback_box: typing.List['urwid.Text'] = attr.ib(factory=lambda: urwid.SimpleFocusListWalker([]), init=False)
   notes: 'FocusAwareEdit' = attr.ib(factory=lambda: FocusAwareEdit('', multiline=True), init=False)
   header: 'urwid.Text' = attr.ib(factory=lambda: urwid.Text('Mesa Stable Picker', align='center'), init=False)
   body: 'urwid.Columns' = attr.ib(attr.Factory(lambda s: s._make_body(), True), init=False)
   footer: 'urwid.Columns' = attr.ib(attr.Factory(lambda s: s._make_footer(), True), init=False)
   root: RootWidget = attr.ib(attr.Factory(lambda s: s._make_root(), True), init=False)
            previous_commits: typing.List['core.Commit'] = attr.ib(factory=list, init=False)
   new_commits: typing.List['core.Commit'] = attr.ib(factory=list, init=False)
            def _get_current_commit(self) -> typing.Optional['core.Commit']:
      entry = self.commit_list.get_focus()[0]
         def _change_notes_cb(self) -> None:
      commit = self._get_current_commit()
   if commit and commit.notes:
         else:
         def _change_notes_focus_cb(self, notes: 'FocusAwareEdit', focus: 'bool') -> 'None':
      # in the case of coming into focus we don't want to do anything
   if focus:
         commit = self._get_current_commit()
   if commit is None:
         text: str = notes.get_edit_text()
   if text != commit.notes:
         def _make_body(self) -> 'urwid.Columns':
      commits = CommitList(self.commit_list, ui=self)
   feedback = urwid.ListBox(self.feedback_box)
   urwid.connect_signal(self.commit_list, 'modified', self._change_notes_cb)
   notes = urwid.Filler(self.notes)
                  def _make_footer(self) -> 'urwid.Columns':
      body = [
         urwid.Text('[U]pdate'),
   urwid.Text('[Q]uit'),
   urwid.Text('[C]herry Pick'),
   urwid.Text('[D]enominate'),
   urwid.Text('[B]ackport'),
   ]
         def _make_root(self) -> 'RootWidget':
            def render(self) -> 'WidgetType':
      asyncio.ensure_future(self.update())
         def load(self) -> None:
            async def update(self) -> None:
      self.load()
   with open('VERSION', 'r') as f:
         if self.previous_commits:
         else:
                     if new_commits:
         pb = urwid.ProgressBar('a', 'b', done=len(new_commits))
   o = self.mainloop.widget
   self.mainloop.widget = urwid.Overlay(
         self.new_commits = await core.gather_commits(
                  for commit in reversed(list(itertools.chain(self.new_commits, self.previous_commits))):
         if commit.nominated and commit.resolution is core.Resolution.UNRESOLVED:
               async def feedback(self, text: str) -> None:
      self.feedback_box.append(urwid.AttrMap(urwid.Text(text), None))
   latest_item_index = len(self.feedback_box) - 1
         def remove_commit(self, commit: CommitWidget) -> None:
      for i, c in enumerate(self.commit_list):
         if c.base_widget is commit:
         def save(self):
            def add(self) -> None:
      """Add an additional commit which isn't nominated."""
            def reset_cb(_) -> None:
            async def apply_cb(edit: urwid.Edit) -> None:
                  # In case the text is empty
                  sha = await core.full_sha(text)
   for c in reversed(list(itertools.chain(self.new_commits, self.previous_commits))):
      if c.sha == sha:
      commit = c
                     q = urwid.Edit("Commit sha\n")
   ok_btn = urwid.Button('Ok')
   urwid.connect_signal(ok_btn, 'click', lambda _: asyncio.ensure_future(apply_cb(q)))
            can_btn = urwid.Button('Cancel')
            cols = urwid.Columns([ok_btn, can_btn])
   pile = urwid.Pile([q, cols])
            self.mainloop.widget = urwid.Overlay(
               def chp_failed(self, commit: 'CommitWidget', err: str) -> None:
               def reset_cb(_) -> None:
            t = urwid.Text(textwrap.dedent(f"""
                                    can_btn = urwid.Button('Cancel')
   urwid.connect_signal(can_btn, 'click', reset_cb)
   urwid.connect_signal(
            ok_btn = urwid.Button('Ok')
   urwid.connect_signal(ok_btn, 'click', reset_cb)
   urwid.connect_signal(
         urwid.connect_signal(
            cols = urwid.Columns([ok_btn, can_btn])
   pile = urwid.Pile([t, cols])
            self.mainloop.widget = urwid.Overlay(
         )
