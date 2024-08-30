   #!/usr/bin/env python3
   # For the dependencies, see the requirements.txt
      import re
   from argparse import ArgumentDefaultsHelpFormatter, ArgumentParser, Namespace
   from collections import defaultdict
   from copy import deepcopy
   from dataclasses import dataclass, field
   from os import getenv
   from pathlib import Path
   from typing import Any, Iterable, Optional, Pattern, Union
      import yaml
   from filecache import DAY, filecache
   from gql import Client, gql
   from gql.transport.aiohttp import AIOHTTPTransport
   from graphql import DocumentNode
      Dag = dict[str, set[str]]
   TOKEN_DIR = Path(getenv("XDG_CONFIG_HOME") or Path.home() / ".config")
         def get_token_from_default_dir() -> str:
      try:
      token_file = TOKEN_DIR / "gitlab-token"
      except FileNotFoundError as ex:
      print(
         )
         def get_project_root_dir():
      root_path = Path(__file__).parent.parent.parent.resolve()
   gitlab_file = root_path / ".gitlab-ci.yml"
                     @dataclass
   class GitlabGQL:
      _transport: Any = field(init=False)
   client: Client = field(init=False)
   url: str = "https://gitlab.freedesktop.org/api/graphql"
            def __post_init__(self):
            def _setup_gitlab_gql_client(self) -> Client:
      # Select your transport with a defined url endpoint
   headers = {}
   if self.token:
         self._transport = AIOHTTPTransport(
            # Create a GraphQL client using the defined transport
   self.client = Client(
               @filecache(DAY)
   def query(
         ) -> dict[str, Any]:
      # Provide a GraphQL query
   source_path = Path(__file__).parent
            query: DocumentNode
   with open(pipeline_query_file, "r") as f:
                  # Execute the query on the transport
         def invalidate_query_cache(self):
            def create_job_needs_dag(
         ) -> tuple[Dag, dict[str, dict[str, Any]]]:
         result = gl_gql.query("pipeline_details.gql", params)
   incomplete_dag = defaultdict(set)
   jobs = {}
   pipeline = result["project"]["pipeline"]
   if not pipeline:
            for stage in pipeline["stages"]["nodes"]:
      for stage_job in stage["groups"]["nodes"]:
         for job in stage_job["jobs"]["nodes"]:
      needs = job.pop("needs")["nodes"]
   jobs[job["name"]] = job
            final_dag: Dag = {}
   for job, needs in incomplete_dag.items():
      final_needs: set = deepcopy(needs)
            while partial:
         next_depth = {n for dn in final_needs for n in incomplete_dag[dn]}
                           def filter_dag(dag: Dag, regex: Pattern) -> Dag:
               def print_dag(dag: Dag) -> None:
      for job, needs in dag.items():
      print(f"{job}:")
   print(f"\t{' '.join(needs)}")
         def fetch_merged_yaml(gl_gql: GitlabGQL, params) -> dict[Any]:
      gitlab_yml_file = get_project_root_dir() / ".gitlab-ci.yml"
   content = Path(gitlab_yml_file).read_text().strip()
   params["content"] = content
   raw_response = gl_gql.query("job_details.gql", params)
   if merged_yaml := raw_response["ciConfig"]["mergedYaml"]:
            gl_gql.invalidate_query_cache()
   raise ValueError(
         Could not fetch any content for merged YAML,
   please verify if the git SHA exists in remote.
   Maybe you forgot to `git push`?  """
            def recursive_fill(job, relationship_field, target_data, acc_data: dict, merged_yaml):
      if relatives := job.get(relationship_field):
      if isinstance(relatives, str):
            for relative in relatives:
                                 def get_variables(job, merged_yaml, project_path, sha) -> dict[str, str]:
      p = get_project_root_dir() / ".gitlab-ci" / "image-tags.yml"
            variables = image_tags["variables"]
   variables |= merged_yaml["variables"]
   variables |= job["variables"]
   variables["CI_PROJECT_PATH"] = project_path
   variables["CI_PROJECT_NAME"] = project_path.split("/")[1]
   variables["CI_REGISTRY_IMAGE"] = "registry.freedesktop.org/${CI_PROJECT_PATH}"
            while recurse_among_variables_space(variables):
                     # Based on: https://stackoverflow.com/a/2158532/1079223
   def flatten(xs):
      for x in xs:
      if isinstance(x, Iterable) and not isinstance(x, (str, bytes)):
         else:
         def get_full_script(job) -> list[str]:
      script = []
   for script_part in ("before_script", "script", "after_script"):
      script.append(f"# {script_part}")
   lines = flatten(job.get(script_part, []))
   script.extend(lines)
                  def recurse_among_variables_space(var_graph) -> bool:
      updated = False
   for var, value in var_graph.items():
      value = str(value)
   dep_vars = []
   if match := re.findall(r"(\$[{]?[\w\d_]*[}]?)", value):
         all_dep_vars = [v.lstrip("${").rstrip("}") for v in match]
            for dep_var in dep_vars:
         dep_value = str(var_graph[dep_var])
   new_value = var_graph[var]
   new_value = new_value.replace(f"${{{dep_var}}}", dep_value)
   new_value = new_value.replace(f"${dep_var}", dep_value)
                  def get_job_final_definition(job_name, merged_yaml, project_path, sha):
      job = merged_yaml[job_name]
            print("# --------- variables ---------------")
   for var, value in sorted(variables.items()):
            # TODO: Recurse into needs to get full script
   # TODO: maybe create a extra yaml file to avoid too much rework
   script = get_full_script(job)
   print()
   print()
   print("# --------- full script ---------------")
            if image := variables.get("MESA_IMAGE"):
      print()
   print()
   print("# --------- container image ---------------")
         def parse_args() -> Namespace:
      parser = ArgumentParser(
      formatter_class=ArgumentDefaultsHelpFormatter,
   description="CLI and library with utility functions to debug jobs via Gitlab GraphQL",
   epilog=f"""Example:
      )
   parser.add_argument("-pp", "--project-path", type=str, default="mesa/mesa")
   parser.add_argument("--sha", "--rev", type=str, required=True)
   parser.add_argument(
      "--regex",
   type=str,
   required=False,
      )
   parser.add_argument("--print-dag", action="store_true", help="Print job needs DAG")
   parser.add_argument(
      "--print-merged-yaml",
   action="store_true",
      )
   parser.add_argument(
         )
   parser.add_argument(
      "--gitlab-token-file",
   type=str,
   default=get_token_from_default_dir(),
               args = parser.parse_args()
   args.gitlab_token = Path(args.gitlab_token_file).read_text()
            def main():
      args = parse_args()
            if args.print_dag:
      dag, jobs = create_job_needs_dag(
                  if args.regex:
               if args.print_merged_yaml:
      print(
         fetch_merged_yaml(
               if args.print_job_manifest:
      merged_yaml = fetch_merged_yaml(
         )
   get_job_final_definition(
               if __name__ == "__main__":
      main()
