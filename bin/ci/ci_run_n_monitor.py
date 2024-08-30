   #!/usr/bin/env python3
   # Copyright © 2020 - 2022 Collabora Ltd.
   # Authors:
   #   Tomeu Vizoso <tomeu.vizoso@collabora.com>
   #   David Heidelberg <david.heidelberg@collabora.com>
   #
   # For the dependencies, see the requirements.txt
   # SPDX-License-Identifier: MIT
      """
   Helper script to restrict running only required CI jobs
   and show the job(s) logs.
   """
      import argparse
   import re
   from subprocess import check_output
   import sys
   import time
   from collections import defaultdict
   from concurrent.futures import ThreadPoolExecutor
   from functools import partial
   from itertools import chain
   from typing import Literal, Optional
      import gitlab
   from colorama import Fore, Style
   from gitlab_common import (
      get_gitlab_project,
   read_token,
   wait_for_pipeline,
      )
   from gitlab_gql import GitlabGQL, create_job_needs_dag, filter_dag, print_dag
      GITLAB_URL = "https://gitlab.freedesktop.org"
      REFRESH_WAIT_LOG = 10
   REFRESH_WAIT_JOBS = 6
      URL_START = "\033]8;;"
   URL_END = "\033]8;;\a"
      STATUS_COLORS = {
      "created": "",
   "running": Fore.BLUE,
   "success": Fore.GREEN,
   "failed": Fore.RED,
   "canceled": Fore.MAGENTA,
   "manual": "",
   "pending": "",
      }
      COMPLETED_STATUSES = ["success", "failed"]
         def print_job_status(job, new_status=False) -> None:
      """It prints a nice, colored job status with a link to the job."""
   if job.status == "canceled":
            if job.duration:
         elif job.started_at:
            print(
      STATUS_COLORS[job.status]
   + "🞋 job "
   + URL_START
   + f"{job.web_url}\a{job.name}"
   + URL_END
   + (f" has new status: {job.status}" if new_status else f" :: {job.status}")
   + (f" ({pretty_duration(duration)})" if job.started_at else "")
               def pretty_wait(sec: int) -> None:
      """shows progressbar in dots"""
   for val in range(sec, 0, -1):
      print(f"⏲  {val} seconds", end="\r")
         def monitor_pipeline(
      project,
   pipeline,
   target_job: str,
   dependencies,
   force_manual: bool,
      ) -> tuple[Optional[int], Optional[int]]:
      """Monitors pipeline and delegate canceling jobs"""
   statuses: dict[str, str] = defaultdict(str)
   target_statuses: dict[str, str] = defaultdict(str)
   stress_status_counter = defaultdict(lambda: defaultdict(int))
                     while True:
      to_cancel = []
   for job in pipeline.jobs.list(all=True, sort="desc"):
         # target jobs
                     if stress and job.status in ["success", "failed"]:
      if (
         stress < 0
   ):
                                             # all jobs
   if job.status != statuses[job.name]:
                  # run dependencies and cancel the rest
   if job.name in dependencies:
                           if stress:
         enough = True
   for job_name, status in stress_status_counter.items():
      print(
      f"{job_name}\tsucc: {status['success']}; "
   f"fail: {status['failed']}; "
   f"total: {sum(status.values())} of {stress}",
                        if not enough:
                     if len(target_statuses) == 1 and {"running"}.intersection(
         ):
            if (
         {"failed"}.intersection(target_statuses.values())
   ):
            if {"success", "manual"}.issuperset(target_statuses.values()):
                  def enable_job(
         ) -> None:
      """enable job"""
   if (
      (job.status in ["success", "failed"] and action_type != "retry")
   or (job.status == "manual" and not force_manual)
      ):
                     if job.status in ["success", "failed", "canceled"]:
         else:
            if action_type == "target":
         elif action_type == "retry":
         else:
                     def cancel_job(project, job) -> None:
      """Cancel GitLab job"""
   if job.status in [
      "canceled",
   "success",
   "failed",
      ]:
         pjob = project.jobs.get(job.id, lazy=True)
   pjob.cancel()
            def cancel_jobs(project, to_cancel) -> None:
      """Cancel unwanted GitLab jobs"""
   if not to_cancel:
            with ThreadPoolExecutor(max_workers=6) as exe:
      part = partial(cancel_job, project)
               def print_log(project, job_id) -> None:
      """Print job log into output"""
   printed_lines = 0
   while True:
               # GitLab's REST API doesn't offer pagination for logs, so we have to refetch it all
   lines = job.trace().decode("raw_unicode_escape").splitlines()
   for line in lines[printed_lines:]:
                  if job.status in COMPLETED_STATUSES:
         print(Fore.GREEN + f"Job finished: {job.web_url}" + Style.RESET_ALL)
         def parse_args() -> None:
      """Parse args"""
   parser = argparse.ArgumentParser(
      description="Tool to trigger a subset of container jobs "
   + "and monitor the progress of a test job",
   epilog="Example: mesa-monitor.py --rev $(git rev-parse HEAD) "
      )
   parser.add_argument(
      "--target",
   metavar="target-job",
   help="Target job regex. For multiple targets, separate with pipe | character",
      )
   parser.add_argument(
      "--token",
   metavar="token",
      )
   parser.add_argument(
         )
   parser.add_argument(
      "--stress",
   default=0,
   type=int,
      )
   parser.add_argument(
      "--project",
   default="mesa",
               mutex_group1 = parser.add_mutually_exclusive_group()
   mutex_group1.add_argument(
         )
   mutex_group1.add_argument(
      "--pipeline-url",
                        # argparse doesn't support groups inside add_mutually_exclusive_group(),
   # which means we can't just put `--project` and `--rev` in a group together,
   # we have to do this by heand instead.
   if args.pipeline_url and args.project != parser.get_default("project"):
      # weird phrasing but it's the error add_mutually_exclusive_group() gives
                  def find_dependencies(target_job: str, project_path: str, sha: str) -> set[str]:
      gql_instance = GitlabGQL()
   dag, _ = create_job_needs_dag(
                  target_dep_dag = filter_dag(dag, target_job)
   if not target_dep_dag:
      print(Fore.RED + "The job(s) were not found in the pipeline." + Fore.RESET)
      print(Fore.YELLOW)
   print("Detected job dependencies:")
   print()
   print_dag(target_dep_dag)
   print(Fore.RESET)
            if __name__ == "__main__":
      try:
                                 gl = gitlab.Gitlab(url=GITLAB_URL,
                           if args.pipeline_url:
         assert args.pipeline_url.startswith(GITLAB_URL)
   url_path = args.pipeline_url[len(GITLAB_URL):]
   url_path_components = url_path.split("/")
   project_name = "/".join(url_path_components[1:3])
   assert url_path_components[3] == "-"
   assert url_path_components[4] == "pipelines"
   pipeline_id = int(url_path_components[5])
   cur_project = gl.projects.get(project_name)
   pipe = cur_project.pipelines.get(pipeline_id)
   else:
                  mesa_project = gl.projects.get("mesa/mesa")
            print(f"Revision: {REV}")
            deps = set()
   if args.target:
         print("🞋 job: " + Fore.BLUE + args.target + Style.RESET_ALL)
   deps = find_dependencies(
         target_job_id, ret = monitor_pipeline(
                  if target_job_id:
            t_end = time.perf_counter()
   spend_minutes = (t_end - t_start) / 60
               except KeyboardInterrupt:
      sys.exit(1)
