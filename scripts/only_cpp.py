from __future__ import absolute_import, division, print_function
from only_common import *
import argparse
import distutils.spawn
import multiprocessing
import os
import subprocess
import sys

def run_build(build_args, mabu_args):
  print("Building for host")

  # Whereas debug builds are primarily I/O-bound,
  # release builds tax the system much more in terms of memory and CPU usage.
  #
  # So don't exhaust the entire system for these.
  #
  jobs = multiprocessing.cpu_count()
  if 'release' in build_args.host_spec:
    jobs = max(1, jobs // 2)

  opts = ['-j', str(jobs)]
  update_for_ccache(opts, build_args.ccache)
  opts += mabu_args

  mabu_projects = list(get_area_projects(build_args))

  target_mabu_projects = list(get_area_projects(build_args))

  if build_args.host and mabu_projects:
    # run all the host project builds
    print_step("Building for host [{}]...".format(build_args.host_spec))
    args = [build_args.mabu, '-t', build_args.host_spec] + opts + mabu_projects
    if launch(args, cwd=build_args.basedir) != 0:
      print_step("BUILD FAILED", finished=True)
      return 1
    else:
      print_step("Host build done", finished=True)

  if build_args.target and target_mabu_projects:
    print_step("Building for device [{}]...".format(build_args.target_spec))
    cert = []
    if not os.environ.get('MLCERT'):
      print_step("Cert not found for signing. Define MLCERT env var pointing to your cert from the creator portal.", finished=True)
      return 1

    args = [build_args.mabu, '-t', build_args.target_spec] + cert + \
           opts + target_mabu_projects
    if launch(args, cwd=build_args.basedir) != 0:
      print_step("BUILD FAILED", finished=True)
      return 1
    else:
      print_step("Device build done", finished=True)

  return 0

def run(build_args, mabu_args):
  # pass the clean flag down to the run_build
  if build_args.clean:
    if run_build(build_args, mabu_args) != 0:
      return 1
    else:
      return 0

  if build_args.build:
    # massage the build args
    if getattr(build_args, 'release_build', None) and not build_args.areas:
      build_args = copy.copy(build_args)
      build_args.areas = ['release']

    if run_build(build_args, mabu_args) != 0:
      return 1

  return 0

def register_args(parser, project):
  parser.add_argument('--ccache', dest='ccache', nargs='?', const='ccache',
                      help='''run compiler with 'ccache' (or the provided executable); without an argument, 
                      the script will check whether the executable exists (thus, it's safe to pass this even 
                      if you're not sure it's installed)''')

  areas = list(get_area_project_map(project).keys())
  areas.sort()
  parser.add_argument('-a', '--areas', dest='areas', action=AreaParserAction,
                      help='''specify areas to build, comma-separated (default: all); choices: {}'''
                        .format(', '.join(areas)))


def build(base_dir, project, dist_dir, package):
  register_common_args(parser)
  register_args(parser, project)

  build_args, mabu_args = parser.parse_known_args()
  canonicalize_args(build_args, mabu_args)

  build_args.basedir = base_dir or '.'
  build_args.project = project
  build_args.distdir = dist_dir
  build_args.package = package

  return_code = run(build_args, mabu_args)
  if return_code == 0:
    print("\nC++ Build: succeeded")

    if build_args.test and build_args.host:
      ok = run_tests(build_args, mabu_args)
      if not ok:
        return_code = -1

  else:
    print("\nC++ Build: failed")

  return return_code
