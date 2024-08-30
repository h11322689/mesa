   #!/usr/bin/env python3
   # SPDX-License-Identifier: MIT
      # Copyright © 2021 Intel Corporation
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
      from __future__ import annotations
   from unittest import mock
   import argparse
   import csv
   import contextlib
   import datetime
   import tempfile
   import os
   import pathlib
   import typing
      import pytest
      from . import gen_calendar_entries
         @contextlib.contextmanager
   def mock_csv(data: typing.List[gen_calendar_entries.CalendarRowType]) -> typing.Iterator[None]:
      """Replace the actual CSV data with our test data."""
   with tempfile.TemporaryDirectory() as d:
      c = os.path.join(d, 'calendar.csv')
   with open(c, 'w') as f:
                  with mock.patch('bin.gen_calendar_entries.CALENDAR_CSV', pathlib.Path(c)):
         @pytest.fixture(autouse=True, scope='module')
   def disable_git_commits() -> None:
      """Mock out the commit function so no git commits are made during testing."""
   with mock.patch('bin.gen_calendar_entries.commit', mock.Mock()):
            class TestReleaseStart:
         def test_first_is_wednesday(self) -> None:
      d = gen_calendar_entries._calculate_release_start('20', '0')
   assert d.day == 15
   assert d.month == 1
         def test_first_is_before_wednesday(self) -> None:
      d = gen_calendar_entries._calculate_release_start('19', '0')
   assert d.day == 16
   assert d.month == 1
         def test_first_is_after_wednesday(self) -> None:
      d = gen_calendar_entries._calculate_release_start('21', '0')
   assert d.day == 13
   assert d.month == 1
         class TestNextReleaseDate:
         @contextlib.contextmanager
   def _patch_date(date: datetime.date) -> typing.Iterator[None]:
      mdate = mock.Mock()
   mdate.today = mock.Mock(return_value=date)
   with mock.patch('bin.gen_calendar_entries.datetime.date', mdate):
                     @pytest.fixture(scope='class', autouse=True)
   def data(self) -> None:
         date = datetime.date(2021, 1, 6)
            @pytest.mark.parametrize(
         'is_zero, expected',
   [
      (True, 13),
      )
   def test(self, is_zero: bool, expected: int) -> None:
                           @pytest.fixture(scope='class', autouse=True)
   def data(self) -> None:
         date = datetime.date(2021, 1, 5)
            @pytest.mark.parametrize(
         'is_zero, expected',
   [
      (True, 13),
      )
   def test(self, is_zero: bool, expected: int) -> None:
                           @pytest.fixture(scope='class', autouse=True)
   def data(self) -> None:
         date = datetime.date(2021, 1, 8)
            @pytest.mark.parametrize(
         'is_zero, expected',
   [
      (True, 13),
      )
   def test(self, is_zero: bool, expected: int) -> None:
               class TestRC:
         ORIGINAL_DATA = [
      ('20.3', '2021-01-13', '20.3.3', 'Dylan Baker', ''),
               @pytest.fixture(autouse=True, scope='class')
   def mock_version(self) -> None:
      """Keep the version set at a specific value."""
   with tempfile.TemporaryDirectory() as d:
         v = os.path.join(d, 'version')
                        @pytest.fixture(autouse=True)
   def csv(self) -> None:
      """inject our test data.."""
   with mock_csv(self.ORIGINAL_DATA):
         def test_basic(self) -> None:
      args: gen_calendar_entries.RCArguments = argparse.Namespace()
   args.manager = "Dylan Baker"
            expected = self.ORIGINAL_DATA.copy()
   expected.append(('21.0', '2021-01-13', f'21.0.0-rc1', 'Dylan Baker'))
   expected.append((    '', '2021-01-20', f'21.0.0-rc2', 'Dylan Baker'))
   expected.append((    '', '2021-01-27', f'21.0.0-rc3', 'Dylan Baker'))
                           class TestExtend:
         def test_one_release(self) -> None:
      data = [
         ('20.3', '2021-01-13', '20.3.3', 'Dylan Baker', ''),
            args: gen_calendar_entries.ExtendArguments = argparse.Namespace()
   args.series = '20.3'
            with mock_csv(data):
                  expected = [
         data[0],
   ('', '2021-01-27', '20.3.4', 'Dylan Baker', ''),
   ('', '2021-02-10', '20.3.5', 'Dylan Baker', ''),
               def test_one_release(self) -> None:
      data = [
         ('20.3', '2021-01-13', '20.3.3', 'Dylan Baker', ''),
   ('',     '2021-01-27', '20.3.4', 'Dylan Baker', 'This is the last planned release of the 20.3.x series.'),
   ('21.0', '2021-01-13', '21.0.1', 'Dylan Baker', ''),
   ('',     '2021-01-27', '21.0.2', 'Dylan Baker', ''),
   ('',     '2021-02-10', '21.0.3', 'Dylan Baker', ''),
            args: gen_calendar_entries.ExtendArguments = argparse.Namespace()
   args.series = '21.0'
            with mock_csv(data):
                  expected = data.copy()
   d = list(data[-1])
   d[-1] = ''
   expected[-1] = tuple(d)
   expected.extend([
                        def test_rc(self) -> None:
      data = [
         ('20.3', '2021-01-13', '20.3.3', 'Dylan Baker', ''),
   ('',     '2021-01-27', '20.3.4', 'Dylan Baker', 'This is the last planned release of the 20.3.x series.'),
   ('21.0', '2021-01-13', '21.0.0-rc1', 'Dylan Baker', ''),
            args: gen_calendar_entries.ExtendArguments = argparse.Namespace()
   args.series = '21.0'
            with mock_csv(data):
                  expected = data.copy()
   d = list(expected[-1])
   d[-1] = ''
   expected[-1] = tuple(d)
   expected.extend([
         ('', '2021-01-27', '21.0.0-rc3', 'Dylan Baker', ''),
                  class TestFinal:
         @pytest.fixture(autouse=True, scope='class')
   def _patch_date(self) -> typing.Iterator[None]:
      mdate = mock.Mock()
   mdate.today = mock.Mock(return_value=datetime.date(2021, 1, 6))
   with mock.patch('bin.gen_calendar_entries.datetime.date', mdate):
         ORIGINAL_DATA = [
      ('20.3', '2021-01-13', '20.3.3', 'Dylan Baker', ''),
               @pytest.fixture(autouse=True)
   def csv(self) -> None:
      """inject our test data.."""
   with mock_csv(self.ORIGINAL_DATA):
         def test_zero_released(self) -> None:
      args: gen_calendar_entries.FinalArguments = argparse.Namespace()
   args.manager = "Dylan Baker"
   args.zero_released = True
   args.series = '21.0'
            expected = self.ORIGINAL_DATA.copy()
   expected.append(('21.0', '2021-01-20', f'21.0.1', 'Dylan Baker'))
   expected.append((    '', '2021-02-03', f'21.0.2', 'Dylan Baker'))
                           def test_zero_not_released(self) -> None:
      args: gen_calendar_entries.FinalArguments = argparse.Namespace()
   args.manager = "Dylan Baker"
   args.zero_released = False
   args.series = '21.0'
            expected = self.ORIGINAL_DATA.copy()
   expected.append(('21.0', '2021-01-13', f'21.0.0', 'Dylan Baker'))
   expected.append((    '', '2021-01-27', f'21.0.1', 'Dylan Baker'))
   expected.append((    '', '2021-02-10', f'21.0.2', 'Dylan Baker'))
                     assert actual == expected
