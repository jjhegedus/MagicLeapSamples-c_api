####################################################################################
# Build and test native apps.
#
# Run with -help for info.
####################################################################################
import sys

vi = sys.version_info
if vi.major < 3 or (vi.major == 3 and vi.minor < 5):
  print("Please run this script with Python 3.5 or newer")
  exit(1)

from only_common import *
import argparse, os, copy, shutil
import only_cpp as only_cpp

win = platform.system() == 'Windows'
osx = platform.system() == 'Darwin'
linux = platform.system() == 'Linux'

failedTests = []

def update_rpaths(build_args):
  rel_install_dir = os.path.relpath(build_args.distdir, build_args.basedir)
  print("rel_install_dir: {0}".format(rel_install_dir))

  kwargs = {
    'cwd': build_args.basedir
  }
  if build_args.verbose == 0:
    kwargs['stdout'] = subprocess.PIPE

  if osx:
    print_step("Updating library lookups...")

    prep_path = os.path.join(build_args.basedir, '..', 'scripts', 'BinaryPrep.sh')
    res = launch([prep_path, rel_install_dir], **kwargs)
    if res != 0:
      print_step("Packaging FAILED - Binary Prep step", finished=True)
      return 1

  return 0

def layout_package(build_args, mabu_args):
  layout_args = [build_args.mabu] + mabu_args + ['-t', build_args.host_spec]

  for package in build_args.packages:
    layout_args.append(package)

  res = launch(layout_args, cwd=build_args.basedir)
  if res != 0:
    return False

  # make sure any executables copied are really +x'ed
  res = fix_permissions(build_args.distdir)
  if res != 0:
    return False

  if build_args.prepare_binaries:
    res = update_rpaths(build_args)
    if res != 0:
      return False

  return True

def run(build_args, mabu_args):
  # canonicalize args (unset = True)
  build_args.prepare_binaries = build_args.prepare_binaries != False

  if build_args.rebuild:
    build_args.clean = True
    build_args.build = True

  full_clean = build_args.release_build or build_args.areas == ['all']

  if build_args.clean and full_clean:
    print(build_args.areas)

    returncode = only_cpp.run(build_args, list(mabu_args) + ["-c"])
    if returncode != 0:
      print_step("C++ CLEAN FAILED", finished=True)
      return 1

    if build_args.clean_rm_dirs:
      for dir in build_args.clean_rm_dirs:
        shutil.rmtree(dir, ignore_errors=True)

  if build_args.build and build_args.cpp:
    print_step("Start building C++...")
    returncode = only_cpp.run(build_args, list(mabu_args))
    if returncode != 0:
      print_step("C++ BUILD FAILED", finished=True)
      return 1
    else:
      print_step("All C++ builds succeeded", finished=True)

  #
  # continue through all the steps even if some fail
  #
  # (note: the strings added here intentionally mirror the header
  # in print_step for easy searching)
  #
  failures = []

  if build_args.build or build_args.release_build:
    print_step("Generating base layout...")
    ok = layout_package(build_args, mabu_args)
    if not ok:
      print_step("LAYOUT STEPS FAILED", finished=True)
      failures.append("base layout")
    else:
      print_step("Layout updated [sourced from {}]".format(build_args.host_spec), finished=True)


  if failures:
    print_step("FAILED STEPS:\n\n\t" + "\n\t".join(failures), finished=True)
    return 1

  return 0


def register_args(parser):
  parser.add_argument('--clean', '-c', action='store_true',
                      help='''clean the build layout''')
  parser.add_argument('--rebuild', '-r', action='store_true',
                      help='''clean then rebuild the layout''')
  parser.add_argument('--release-build', '--rb', action='store_true',
                      help='''perform steps to generate the full release layout; implies '-a release' if no '-a' provided''')

  parser.add_argument('--prepare-binaries', '--pb', dest='prepare_binaries', action='store_true', default=None,
                      help='''prepare binaries by setting their library search paths (default)''')
  parser.add_argument('--no-prepare-binaries', '--no-pb', dest='prepare_binaries', action='store_false', default=None,
                      help='''do not prepare binaries''')
  parser.add_argument('--assemble-mlsdk', '--asmsdk', dest='asm_sdk', action='store_true', default=None,
                      help='''build stubs and overlay into mlsdk''')

def parse_args(project):
  parser = argparse.ArgumentParser()
  register_common_args(parser)
  register_args(parser)
  for mod in [only_cpp]:
    if hasattr(mod, 'register_args'):
      group = parser.add_argument_group(mod.__name__[4:])
      mod.register_args(group, project)

  argv = sys.argv[1:]
  build_args, mabu_args = parser.parse_known_args(argv)
  canonicalize_args(build_args, mabu_args)
  return build_args, mabu_args


def build(build_args, mabu_args, base_dir, project, dist_dir, packages, clean_rm_dirs=None):
  build_args.basedir = base_dir or '.'
  build_args.project = project
  build_args.distdir = dist_dir
  build_args.packages = packages
  build_args.clean_rm_dirs = clean_rm_dirs

  return run(build_args, mabu_args)
