   #!/usr/bin/env python3
   # Copyright © 2020 - 2023 Collabora Ltd.
   # Authors:
   #   David Heidelberg <david.heidelberg@collabora.com>
   #
   # SPDX-License-Identifier: MIT
      """
   Monitors Marge-bot and return number of assigned MRs.
   """
      import argparse
   import time
   import sys
   from datetime import datetime, timezone
   from dateutil import parser
      import gitlab
   from gitlab_common import read_token, pretty_duration
      REFRESH_WAIT = 30
   MARGE_BOT_USER_ID = 9716
         def parse_args() -> None:
      """Parse args"""
   parse = argparse.ArgumentParser(
         )
   parse.add_argument(
         )
   parse.add_argument(
      "--token",
   metavar="token",
      )
            if __name__ == "__main__":
      args = parse_args()
   token = read_token(args.token)
                     while True:
               jobs_num = len(mrs)
   for mr in mrs:
         updated = parser.parse(mr.updated_at)
   now = datetime.now(timezone.utc)
   diff = (now - updated).total_seconds()
   print(
                     if jobs_num == 0:
         if not args.wait:
            time.sleep(REFRESH_WAIT)
