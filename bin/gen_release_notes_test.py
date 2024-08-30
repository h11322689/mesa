   # Copyright © 2019,2021 Intel Corporation
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
      import sys
   import textwrap
   import typing
      import pytest
      # AsyncMock is new in 3.8, so if we're using an older version we need the
   # backported version of mock
   if sys.version_info >= (3, 8):
         else:
            from .gen_release_notes import *
         @pytest.mark.parametrize(
      'current, is_point, expected',
   [
      ('19.2.0', True, '19.2.1'),
   ('19.3.6', True, '19.3.7'),
         def test_next_version(current: str, is_point: bool, expected: str) -> None:
               @pytest.mark.parametrize(
      'current, is_point, expected',
   [
      ('19.3.6', True, '19.3.6'),
         def test_previous_version(current: str, is_point: bool, expected: str) -> None:
               @pytest.mark.asyncio
   async def test_get_shortlog():
      # Certainly not perfect, but it's something
   version = '19.2.0'
   out = await get_shortlog(version)
            @pytest.mark.asyncio
   async def test_gather_commits():
      # Certainly not perfect, but it's something
   version = '19.2.0'
   out = await gather_commits(version)
            @pytest.mark.asyncio
   @pytest.mark.parametrize(
      'content, bugs',
   [
      # It is important to have the title on a new line, as
            # Test the `Closes: #N` syntax
   (
                                 Closes: #1
   ''',
            # Test the Full url
   (
                        Closes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/3456
   ''',
            # Test projects that are not mesa
   (
                        Closes: https://gitlab.freedesktop.org/mesa/drm/-/3456
   ''',
   ),
   (
                        Closes: https://github.com/Organization/project/1234
   ''',
            # Test  multiple issues on one line
   (
                        Closes: #1, #2
   ''',
            # Test multiple closes
   (
                        Closes: #1
   Closes: #2
   ''',
   ),
   (
                        Closes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/3456
   Closes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/3457
   Closes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/3458
   ''',
   ),
   (
                        Closes: https://gitlab.freedesktop.org/mesa/mesa/issues/36
   ''',
   ),
   (
                        Closes: https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/20241
   ''',
   ),
   (
                        Fixes: https://gitlab.freedesktop.org/mesa/mesa/issues/36
   Fixes: 142565a3bc2
   Fixes: 142565a3bc2 ("docs: do something very useful")
   Fixes: 142565a3bc2 ("docs: fix #1234, have a comma")
   Fixes: https://gitlab.freedesktop.org/mesa/mesa/-/issues/37
   ''',
   ),
   (
                        fixes: https://gitlab.freedesktop.org/mesa/mesa/issues/36
   fiXES: https://gitlab.freedesktop.org/mesa/mesa/issues/37
   closes: https://gitlab.freedesktop.org/mesa/mesa/issues/38
   cloSES: https://gitlab.freedesktop.org/mesa/mesa/issues/39
   ''',
         async def test_parse_issues(content: str, bugs: typing.List[str]) -> None:
      mock_com = mock.AsyncMock(return_value=(textwrap.dedent(content).encode(), ''))
   mock_p = mock.Mock()
   mock_p.communicate = mock_com
            with mock.patch('bin.gen_release_notes.asyncio.create_subprocess_exec', mock_exec), \
            ids = await parse_issues('1234 not used')
      @pytest.mark.asyncio
   async def test_rst_escape():
      out = inliner.quoteInline('foo@bar')
   assert out == 'foo\@bar'
