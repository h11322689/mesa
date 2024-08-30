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
      """Helper script for manipulating the release calendar."""
      from __future__ import annotations
   import argparse
   import csv
   import contextlib
   import datetime
   import pathlib
   import subprocess
   import typing
      if typing.TYPE_CHECKING:
      import _csv
            class RCArguments(Protocol):
                     class FinalArguments(Protocol):
               series: str
   manager: str
         class ExtendArguments(Protocol):
               series: str
                     _ROOT = pathlib.Path(__file__).parent.parent
   CALENDAR_CSV = _ROOT / 'docs' / 'release-calendar.csv'
   VERSION = _ROOT / 'VERSION'
   LAST_RELEASE = 'This is the last planned release of the {}.x series.'
   OR_FINAL = 'Or {}.0 final.'
         def read_calendar() -> typing.List[CalendarRowType]:
      """Read the calendar and return a list of it's rows."""
   with CALENDAR_CSV.open('r') as f:
            def commit(message: str) -> None:
      """Commit the changes the the release-calendar.csv file."""
               def _calculate_release_start(major: str, minor: str) -> datetime.date:
               This is quarterly, on the second wednesday, in January, April, July, and October.
   """
            # Wednesday is 3
   day = quarter.isoweekday()
   if day > 3:
      # this will walk back into the previous month, it's much simpler to
   # duplicate the 14 than handle the calculations for the month and year
   # changing.
      elif day < 3:
                  def release_candidate(args: RCArguments) -> None:
      """Add release candidate entries."""
   with VERSION.open('r') as f:
         major, minor, _ = version.split('.')
                     with CALENDAR_CSV.open('w', newline='') as f:
      writer = csv.writer(f)
            writer.writerow([f'{major}.{minor}', date.isoformat(), f'{major}.{minor}.0-rc1', args.manager])
   for row in range(2, 4):
         date = date + datetime.timedelta(days=7)
   date = date + datetime.timedelta(days=7)
                  def _calculate_next_release_date(next_is_zero: bool) -> datetime.date:
               If the next is .0, we have the release in seven days, if the next is .1,
   then it's in 14
   """
   date = datetime.date.today()
   day = date.isoweekday()
   if day < 3:
         elif day > 3:
      # this will walk back into the previous month, it's much simpler to
   # duplicate the 14 than handle the calculations for the month and year
   # changing.
      else:
         delta += 7
   if not next_is_zero:
                  def final_release(args: FinalArguments) -> None:
      """Add final release entries."""
   data = read_calendar()
            with CALENDAR_CSV.open('w', newline='') as f:
      writer = csv.writer(f)
                     writer.writerow([args.series, date.isoformat(), f'{args.series}.{base}', args.manager])
   for row in range(base + 1, 3):
         date = date + datetime.timedelta(days=14)
   date = date + datetime.timedelta(days=14)
                  def extend(args: ExtendArguments) -> None:
      """Extend a release."""
   @contextlib.contextmanager
   def write_existing(writer: _csv._writer, current: typing.List[CalendarRowType]) -> typing.Iterator[CalendarRowType]:
               This is a bit clever, basically what happens it writes out the
   original csv file until it reaches the start of the release after the
   one we're appending, then it yields the last row. When control is
   returned it writes out the rest of the original calendar data.
   """
   last_row: typing.Optional[CalendarRowType] = None
   in_wanted = False
   for row in current:
         if in_wanted and row[0]:
      in_wanted = False
   assert last_row is not None
      if row[0] == args.series:
         if in_wanted and len(row) >= 5 and row[4] in {LAST_RELEASE.format(args.series), OR_FINAL.format(args.series)}:
      # If this was the last planned release and we're adding more,
   # then we need to remove that message and add it elsewhere
   r = list(row)
   r[4] = None
   # Mypy can't figure this out…
      last_row = row
   # If this is the only entry we can hit a case where the contextmanager
   # hasn't yielded
   if in_wanted:
                  with CALENDAR_CSV.open('w', newline='') as f:
      writer = csv.writer(f)
   with write_existing(writer, current) as row:
         # Get rid of -rcX as well
   if '-rc' in row[2]:
      first_point = int(row[2].split('rc')[-1]) + 1
   template = '{}.0-rc{}'
      else:
                        date = datetime.date.fromisoformat(row[1])
   for i in range(first_point, first_point + args.count):
      date = date + datetime.timedelta(days=days)
   r = [None, date.isoformat(), template.format(args.series, i), row[3], None]
   if i == first_point + args.count - 1:
      if days == 14:
                        def main() -> None:
      parser = argparse.ArgumentParser()
            rc = sub.add_parser('release-candidate', aliases=['rc'], help='Generate calendar entries for a release candidate.')
   rc.add_argument('manager', help="the name of the person managing the release.")
            fr = sub.add_parser('release', help='Generate calendar entries for a final release.')
   fr.add_argument('manager', help="the name of the person managing the release.")
   fr.add_argument('series', help='The series to extend, such as "29.3" or "30.0".')
   fr.add_argument('--zero-released', action='store_true', help='The .0 release was today, the next release is .1')
            ex = sub.add_parser('extend', help='Generate additional entries for a release.')
   ex.add_argument('series', help='The series to extend, such as "29.3" or "30.0".')
   ex.add_argument('count', type=int, help='The number of new entries to add.')
            args = parser.parse_args()
            if __name__ == "__main__":
      main()
