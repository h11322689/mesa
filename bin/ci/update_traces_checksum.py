   #!/usr/bin/env python3
   # Copyright © 2022 Collabora Ltd.
   # Authors:
   #   David Heidelberg <david.heidelberg@collabora.com>
   #
   # For the dependencies, see the requirements.txt
   # SPDX-License-Identifier: MIT
      """
   Helper script to update traces checksums
   """
      import argparse
   import bz2
   import glob
   import re
   import json
   import sys
   from ruamel.yaml import YAML
      import gitlab
   from colorama import Fore, Style
   from gitlab_common import get_gitlab_project, read_token, wait_for_pipeline
         DESCRIPTION_FILE = "export PIGLIT_REPLAY_DESCRIPTION_FILE='.*/install/(.*)'$"
   DEVICE_NAME = "export PIGLIT_REPLAY_DEVICE_NAME='(.*)'$"
         def gather_results(
      project,
      ) -> None:
                        for job in pipeline.jobs.list(all=True, sort="desc"):
      if target_jobs_regex.match(job.name) and job.status == "failed":
         cur_job = project.jobs.get(job.id)
   # get variables
   print(f"👁  {job.name}...")
   log: list[str] = cur_job.trace().decode("unicode_escape").splitlines()
   filename: str = ''
   dev_name: str = ''
   for logline in log:
      desc_file = re.search(DESCRIPTION_FILE, logline)
   device_name = re.search(DEVICE_NAME, logline)
   if desc_file:
                     if not filename or not dev_name:
                           # find filename in Mesa source
   traces_file = glob.glob('./**/' + filename, recursive=True)
   # write into it
   with open(traces_file[0], 'r', encoding='utf-8') as target_file:
      yaml = YAML()
   yaml.compact(seq_seq=False, seq_map=False)
   yaml.version = 1,2
                        # parse artifact
                        for _, value in results["tests"].items():
      if (
         not value['images'] or
                                                                                                                                 def parse_args() -> None:
      """Parse args"""
   parser = argparse.ArgumentParser(
      description="Tool to generate patch from checksums ",
      )
   parser.add_argument(
         )
   parser.add_argument(
      "--token",
   metavar="token",
      )
            if __name__ == "__main__":
      try:
                                          print(f"Revision: {args.rev}")
   (pipe, cur_project) = wait_for_pipeline([cur_project], args.rev)
   print(f"Pipeline: {pipe.web_url}")
               except KeyboardInterrupt:
      sys.exit(1)
