   #!/usr/bin/env python3
      import argparse
   import subprocess
   import sys
         def print_(args: argparse.Namespace, success: bool, message: str) -> None:
      """
   Print function with extra coloring when supported and/or requested,
   and with a "quiet" switch
            COLOR_SUCCESS = '\033[32m'
   COLOR_FAILURE = '\033[31m'
            if args.quiet:
            if args.color == 'auto':
         else:
            s = ''
   if use_colors:
      if success:
         else:
                  if use_colors:
                     def is_commit_valid(commit: str) -> bool:
      ret = subprocess.call(['git', 'cat-file', '-e', commit],
                        def branch_has_commit(upstream_branch: str, commit: str) -> bool:
      """
   Returns True if the commit is actually present in the branch
   """
   ret = subprocess.call(['git', 'merge-base', '--is-ancestor',
                              def branch_has_backport_of_commit(upstream_branch: str, commit: str) -> str:
      """
   Returns the commit hash if the commit has been backported to the branch,
   or an empty string if is hasn't
   """
            out = subprocess.check_output(['git', 'log', '--format=%H',
                              def canonicalize_commit(commit: str) -> str:
      """
   Takes a commit-ish and returns a commit sha1 if the commit exists
            # Make sure input is valid first
   if not is_commit_valid(commit):
            out = subprocess.check_output(['git', 'rev-parse', commit],
                  def validate_branch(branch: str) -> str:
      if '/' not in branch:
            out = subprocess.check_output(['git', 'remote', '--verbose'],
         remotes = out.decode().splitlines()
   upstream, _ = branch.split('/', 1)
   valid_remote = False
   for line in remotes:
      if line.startswith(upstream + '\t'):
         if not valid_remote:
            if not is_commit_valid(branch):
                     if __name__ == "__main__":
      parser = argparse.ArgumentParser(description="""
   Returns 0 if the commit is present in the branch,
   1 if it's not,
   and 2 if it couldn't be determined (eg. invalid commit)
   """)
   parser.add_argument('commit',
               parser.add_argument('branch',
               parser.add_argument('--quiet',
               parser.add_argument('--color',
                              if branch_has_commit(args.branch, args.commit):
      print_(args, True, 'Commit ' + args.commit + ' is in branch ' + args.branch)
         backport = branch_has_backport_of_commit(args.branch, args.commit)
   if backport:
      print_(args, True,
               print_(args, False, 'Commit ' + args.commit + ' is NOT in branch ' + args.branch)
   exit(1)
