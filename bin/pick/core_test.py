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
      """Tests for pick's core data structures and routines."""
      from unittest import mock
   import textwrap
   import typing
      import attr
   import pytest
      from . import core
         class TestCommit:
         @pytest.fixture
   def unnominated_commit(self) -> 'core.Commit':
            @pytest.fixture
   def nominated_commit(self) -> 'core.Commit':
      return core.Commit('abc123', 'sub: A commit', True,
                     def test_not_nominated(self, unnominated_commit: 'core.Commit'):
         c = unnominated_commit
   v = c.to_json()
   assert v == {'sha': 'abc123', 'description': 'sub: A commit', 'nominated': False,
            def test_nominated(self, nominated_commit: 'core.Commit'):
         c = nominated_commit
   v = c.to_json()
   assert v == {'sha': 'abc123',
               'description': 'sub: A commit',
   'nominated': True,
                     def test_not_nominated(self, unnominated_commit: 'core.Commit'):
         c = unnominated_commit
   v = c.to_json()
            def test_nominated(self, nominated_commit: 'core.Commit'):
         c = nominated_commit
   v = c.to_json()
         class TestRE:
                              def test_simple(self):
                                          Fixes: 3d09bb390a39 (etnaviv: GC7000: State changes for HALTI3..5)
                     fix_for_commit = core.IS_FIX.search(message)
                     def test_single_branch(self):
         """Tests commit meant for a single branch, ie, 19.1"""
                                    Cc: 19.2 <mesa-stable@lists.freedesktop.org>
   Closes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/1923
                     cc_to = core.IS_CC.search(message)
            def test_multiple_branches(self):
         """Tests commit with more than one branch specified"""
                                                      cc_to = core.IS_CC.search(message)
   assert cc_to is not None
            def test_no_branch(self):
         """Tests commit with no branch specification"""
                                    Cc: <mesa-stable@lists.freedesktop.org>
                              def test_quotes(self):
         """Tests commit with quotes around the versions"""
                     Cc: "20.0" mesa-stable@lists.freedesktop.org
   Reviewed-by: Kenneth Graunke <kenneth@whitecape.org>
                     cc_to = core.IS_CC.search(message)
            def test_multiple_quotes(self):
         """Tests commit with quotes around the versions"""
                     Cc: "20.0" "20.1" mesa-stable@lists.freedesktop.org
   Reviewed-by: Kenneth Graunke <kenneth@whitecape.org>
                     cc_to = core.IS_CC.search(message)
   assert cc_to is not None
            def test_single_quotes(self):
         """Tests commit with quotes around the versions"""
                     Cc: '20.0' mesa-stable@lists.freedesktop.org
   Reviewed-by: Kenneth Graunke <kenneth@whitecape.org>
                     cc_to = core.IS_CC.search(message)
            def test_multiple_single_quotes(self):
         """Tests commit with quotes around the versions"""
                     Cc: '20.0' '20.1' mesa-stable@lists.freedesktop.org
   Reviewed-by: Kenneth Graunke <kenneth@whitecape.org>
                     cc_to = core.IS_CC.search(message)
   assert cc_to is not None
                     def test_simple(self):
                                                                  Cc: 19.2 <mesa-stable@lists.freedesktop.org>
                     revert_of = core.IS_REVERT.search(message)
                     def test_single_release(self):
         """Tests commit meant for a single branch, ie, 19.1"""
                                    Backport-to: 19.2
   Closes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/1923
                     backport_to = core.IS_BACKPORT.search(message)
            def test_multiple_release_space(self):
         """Tests commit with more than one branch specified"""
                                                      backport_to = core.IS_BACKPORT.search(message)
            def test_multiple_release_comma(self):
         """Tests commit with more than one branch specified"""
                                                      backport_to = core.IS_BACKPORT.search(message)
         class TestResolveNomination:
         @attr.s(slots=True)
                        out: typing.Optional[bytes] = attr.ib(None)
            async def mock(self, *_, **__):
                  async def communicate(self) -> typing.Tuple[bytes, bytes]:
                  async def wait(self) -> int:
         @staticmethod
   async def return_true(*_, **__) -> bool:
            @staticmethod
   async def return_false(*_, **__) -> bool:
            @pytest.mark.asyncio
   async def test_fix_is_nominated(self):
      s = self.FakeSubprocess(b'Fixes: 3d09bb390a39 (etnaviv: GC7000: State changes for HALTI3..5)')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert c.nominated
         @pytest.mark.asyncio
   async def test_fix_is_not_nominated(self):
      s = self.FakeSubprocess(b'Fixes: 3d09bb390a39 (etnaviv: GC7000: State changes for HALTI3..5)')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert not c.nominated
         @pytest.mark.asyncio
   async def test_cc_is_nominated(self):
      s = self.FakeSubprocess(b'Cc: 16.2 <mesa-stable@lists.freedesktop.org>')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
            assert c.nominated
         @pytest.mark.asyncio
   async def test_cc_is_nominated2(self):
      s = self.FakeSubprocess(b'Cc: mesa-stable@lists.freedesktop.org')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
            assert c.nominated
         @pytest.mark.asyncio
   async def test_cc_is_not_nominated(self):
      s = self.FakeSubprocess(b'Cc: 16.2 <mesa-stable@lists.freedesktop.org>')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
            assert not c.nominated
         @pytest.mark.asyncio
   async def test_backport_is_nominated(self):
      s = self.FakeSubprocess(b'Backport-to: 16.2')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
            assert c.nominated
         @pytest.mark.asyncio
   async def test_backport_is_not_nominated(self):
      s = self.FakeSubprocess(b'Backport-to: 16.2')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
            assert not c.nominated
         @pytest.mark.asyncio
   async def test_revert_is_nominated(self):
      s = self.FakeSubprocess(b'This reverts commit 1234567890123456789012345678901234567890.')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert c.nominated
         @pytest.mark.asyncio
   async def test_revert_is_not_nominated(self):
      s = self.FakeSubprocess(b'This reverts commit 1234567890123456789012345678901234567890.')
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert not c.nominated
         @pytest.mark.asyncio
   async def test_is_fix_and_backport(self):
      s = self.FakeSubprocess(
         b'Fixes: 3d09bb390a39 (etnaviv: GC7000: State changes for HALTI3..5)\n'
   )
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert c.nominated
         @pytest.mark.asyncio
   async def test_is_fix_and_cc(self):
      s = self.FakeSubprocess(
         b'Fixes: 3d09bb390a39 (etnaviv: GC7000: State changes for HALTI3..5)\n'
   )
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert c.nominated
         @pytest.mark.asyncio
   async def test_is_fix_and_revert(self):
      s = self.FakeSubprocess(
         b'Fixes: 3d09bb390a39 (etnaviv: GC7000: State changes for HALTI3..5)\n'
   )
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert c.nominated
         @pytest.mark.asyncio
   async def test_is_cc_and_revert(self):
      s = self.FakeSubprocess(
         b'This reverts commit 1234567890123456789012345678901234567890.\n'
   )
            with mock.patch('bin.pick.core.asyncio.create_subprocess_exec', s.mock):
                  assert c.nominated
         class TestResolveFixes:
         @pytest.mark.asyncio
   async def test_in_new(self):
      """Because commit abcd is nominated, so f123 should be as well."""
   c = [
         core.Commit('f123', 'desc', nomination_type=core.NominationType.FIXES, because_sha='abcd'),
   ]
   await core.resolve_fixes(c, [])
         @pytest.mark.asyncio
   async def test_not_in_new(self):
      """Because commit abcd is not nominated, commit f123 shouldn't be either."""
   c = [
         core.Commit('f123', 'desc', nomination_type=core.NominationType.FIXES, because_sha='abcd'),
   ]
   await core.resolve_fixes(c, [])
         @pytest.mark.asyncio
   async def test_in_previous(self):
      """Because commit abcd is nominated, so f123 should be as well."""
   p = [
         ]
   c = [
         ]
   await core.resolve_fixes(c, p)
         @pytest.mark.asyncio
   async def test_not_in_previous(self):
      """Because commit abcd is not nominated, commit f123 shouldn't be either."""
   p = [
         ]
   c = [
         ]
   await core.resolve_fixes(c, p)
         class TestIsCommitInBranch:
         @pytest.mark.asyncio
   async def test_no(self):
      # Hopefully this is never true?
   value = await core.is_commit_in_branch('ffffffffffffffffffffffffffffff')
         @pytest.mark.asyncio
   async def test_yes(self):
      # This commit is from 2000, it better always be in the branch
   value = await core.is_commit_in_branch('88f3b89a2cb77766d2009b9868c44e03abe2dbb2')
         class TestFullSha:
         @pytest.mark.asyncio
   async def test_basic(self):
      # This commit is from 2000, it better always be in the branch
   value = await core.full_sha('88f3b89a2cb777')
         @pytest.mark.asyncio
   async def test_invalid(self):
      # This commit is from 2000, it better always be in the branch
   with pytest.raises(core.PickUIException):
         await core.full_sha('fffffffffffffffffffffffffffffffffff')
