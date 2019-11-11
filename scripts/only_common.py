from __future__ import print_function
import shutil
import sys, time

vi = sys.version_info
if vi.major < 3 or (vi.major == 3 and vi.minor < 5):
  print("Please run this script with Python 3.5 or newer")
  sys.exit(1)

import os
import shlex
import subprocess
import argparse
import platform

is_verbose = False


def find_and_remove(user_args, arg):
  """
  Look for a given flag in @user_args matching @arg and remove if found.
  :return: True if found and removed, else False
  """
  try:
    idx = user_args.index(arg)
    user_args[idx:idx+1] = []
    return True
  except ValueError:
    return False


def is_jenkins_build():
  return bool(os.getenv("JENKINS_HOME"))


def print_step(msg, finished=False):
  sys.stdout.flush()
  sys.stderr.flush()

  if finished:
    print('\n')

  print("[{}] >>>>>> {}\n".format(time.strftime("%H:%M:%S"), msg))

  sys.stdout.flush()
  sys.stderr.flush()


def test_symlink(base):
  """
  See if we can create soft/symlinks in `base`
  :param base:  path
  :return:  bool
  """
  testlink = os.path.join(base, ".testsymlink")
  testsrc = os.path.join(base, ".testsrc")
  try:

    # make a test file
    with open(testsrc, "w") as f:
      pass

    # remove link if existed
    try:
      os.unlink(testlink)
    except OSError:
      pass

    # try to make a link
    os.symlink(testsrc, testlink)

    return True

  except NotImplementedError:
    # os.symlink can raise this
    return False
  except OSError:
    # some other error
    return False

  finally:
    # clean up
    try:
      os.unlink(testlink)
    except OSError:
      pass
    try:
      os.unlink(testsrc)
    except OSError:
      pass


def test_link(base):
  """
  See if we can create (hard)links in `base`.
  :param base:  directory
  :return:  bool
  """
  testlink = os.path.join(base, ".testlink")
  testsrc = os.path.join(base, ".testsrc")
  try:

    # make a test file
    with open(testsrc, "w") as f:
      pass

    # remove link if existed
    try:
      os.unlink(testlink)
    except OSError:
      pass

    # try to make a link
    os.link(testsrc, testlink)

    return True

  except NotImplementedError:
    # os.symlink can raise this
    return False
  except OSError:
    # some other error
    return False

  finally:
    # clean up
    try:
      os.unlink(testlink)
    except OSError:
      pass
    try:
      os.unlink(testsrc)
    except OSError:
      pass


def launch(args, **kwargs):
  """ Launch program from @args and return exit code """
  comp = launch_with_completion(args, **kwargs)
  return comp.returncode if comp else -1


def launch_with_completion(args, **kwargs):
  """ Launch program from @args and return ProcessCompletion object """
  if is_verbose:
    cmd_str = shlex.quote(' '.join(args))
    env = kwargs.get('env')
    if env:
      orig_env = dict(os.environ)
      for k, v in env.items():
        if k not in orig_env or orig_env[k] != env[k]:
          cmd_str = k + '=' + v + ' ' + cmd_str
    print("[{}] Running: {}".format(time.strftime("%H:%M:%S"), cmd_str))
    sys.stdout.flush()
    sys.stderr.flush()

  # Workaround for bug affecting CI windows nodes. Sets mabu's PYTHON variable
  # to match the calling python executable. On CI, it seemed to switch to
  # the system default python under some conditions.
  if args[0] == "mabu" or args[0] == "mabu.cmd":
    args.append("PYTHON=" + sys.executable)

  try:
    comp = subprocess.run(args, **kwargs)
    return comp
  except subprocess.TimeoutExpired:
    print(args, "has taken more than", kwargs['timeout'], "seconds and has been terminated")
    return None
  finally:
    sys.stdout.flush()
    sys.stderr.flush()


def killall(proc):
  # kill programs with the name @proc
  if sys.platform == 'win32':
    args = ["taskkill", "/f", "/im", proc + ".exe"]
  else:
    args = ["killall", "-9", proc]

  # run and ignore output
  launch(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def find_mabu():
  """ find the name for invoking mabu (a script) """
  return 'mabu.cmd' if sys.platform == 'win32' else 'mabu'


def find_mlsdk(user_args):
  """ find the appropriate MLSDK to use """
  mlsdk = os.getenv('MLSDK')
  if not mlsdk:
    for arg in user_args:
      if arg.startswith('MLSDK='):
        mlsdk = arg[6:]

  if not mlsdk:
    # search PATH to see if it has been set up
    for p in (os.getenv('Path') or os.getenv('PATH')).split(os.pathsep):
      if os.path.exists(os.path.join(p, 'include/ml_api.h')):
        mlsdk = p
        break

  return mlsdk


def get_user_target(user_args, edit_args=False):
  """ Find the last value passed to -t (ignore --target since that's a common arg) """
  last = -1
  target = None
  while True:
    try:
      # ... allowing for user overrides in '-t ...'
      next = user_args.index('-t', last + 1)
      if next + 1 < len(user_args):
        target = user_args[next + 1]
        if edit_args:
          user_args[next:next+2] = []
        else:
          last = next + 2
      else:
        break
    except ValueError:
      break

  return target


def update_for_ccache(cmdline, ccache):
  if ccache == 'ccache':
    # default value: make sure it exists
    ccache = shutil.which('ccache')
    if not ccache:
      print("**** Cannot locate ccache, ignoring --ccache")

  if ccache:
    cmdline.append('COMPILER_PREFIX=' + ccache)
    cmdline.append('LINKER_PREFIX=' + ccache)


def get_os_segment():
  return {
      'Windows': 'win64',
      'Darwin': 'osx',
      'Linux': 'linux64'
  } [platform.system()]


def get_host_spec(user_args, edit_args=False):
  """ Get the qualified host build spec, extracting allowing @args to override if needed """

  # first, use variable set up from vdsetup.{bat,sh} if set
  default_spec = []
  config = os.getenv('TESTS_CONFIG')
  if config:
    default_spec = ['-t', config]

  # fetch the full mabu target
  args = [find_mabu(), '--print-target'] + default_spec + ['-q']

  # ... allowing for user overrides in '-t ...'
  user_target = get_user_target(user_args, edit_args=edit_args)
  if user_target:
    args += ['-t', user_target]

  # ask mabu to resolve the build spec
  try:
    comp = subprocess.run(args, universal_newlines=True, stdout=subprocess.PIPE)
    if comp.returncode != 0:
      print("FAILED TO RUN MABU")
      return None
  except FileNotFoundError:
    # e.g. onlyUnity w/o mabu?
    return "debug_" + get_os_segment() + "_gcc_x64"

  host_spec = comp.stdout.strip()
  return host_spec


def _ensure_build_spec(user_args):
  """ convert "-t <target>" appropriately to return matching host and device specs """

  # this implicitly uses any -t/--target in user_args
  host_spec = get_host_spec(user_args, edit_args=True)
  if not host_spec:
    # failure here means we failed to run mabu -- no point continuing
    sys.exit(1)

  if 'debug' in host_spec:
    target_spec = 'device_debug'
  elif 'release' in host_spec:
    target_spec = 'device_release'
  else:
    target_spec = 'device'

  print("Host Build: {0}; Target Build: {1}".format(host_spec, target_spec))
  return host_spec, target_spec

# Map of base directory to raw area to list of .mabu or .package paths
_area_projects = {}
""" :type: dict[str, dict[str, list[str]]] """


# Map of areas to all the contained areas
_area_areas = {}
""" :type: dict[str, list[str]] """


def get_area_project_map(proj_json):
  """
  Get projects per "area"
  :param build_args
  :return: dictionary of "area" to list of *.package paths
  :rtype: dict[str, list[str]]
  """
  global _area_projects
  global _area_areas

  if proj_json not in _area_projects:
    import json

    with open(proj_json, 'rt') as f:
      content = f.read()

      content = '\n'.join(line for line in content.splitlines() if not line.strip().startswith('//'))
      area_json_orig = json.loads(content)

      area_json = {}
      for area, contents in area_json_orig.items():
        for subarea in area.split('|'):
          area_json[subarea] = list(contents)

      # resolve areas first (entries with no punctuation)
      # NOTE: these are not sorted, so we need to visit until
      # all areas are resolved

      area_map = {}
      while True:
        retry_areas = []

        for name, entries in area_json.items():
          areas = []
          for entry in entries:
            # gather only areas (not paths)
            if '/' not in entry and '.' not in entry:
              if entry in area_map:
                areas.append(entry)
              elif entry in area_json:
                retry_areas.append(entry)
              else:
                raise AssertionError("unknown areas referenced in {}: {}".format(area_map_file, entry))

          area_map[name] = areas

        if not retry_areas:
          break

      _area_areas = area_map

      # recurse and find .packages
      area_project_map = {}

      basedir = os.path.dirname(proj_json)
      for area, entries in area_json.items():
        area_projects = []
        for entry in entries:
          # gather only paths (not areas)
          if '.' in entry or '/' in entry:
            full = os.path.join(basedir, entry)
            if os.path.isfile(full):
              area_projects.append(full)
            else:
              for dirpath, dirnames, filenames in os.walk(full):
                area_projects += [os.path.normpath(os.path.join(dirpath, f)) for f in filenames
                                  if f.endswith(".package")]

        area_projects.sort()
        area_project_map[area] = area_projects

      _area_projects[proj_json] = area_project_map

  return _area_projects[proj_json]


def get_area_projects(build_args):
  """
  Get projects for the "areas" configured for the build
  :param build_args
  :return: list of *.package (or *.mabu) paths, filtered by area.
  :rtype: list[str]
  """
  proj_map = get_area_project_map(build_args.project)

  if build_args.areas:
    areas = build_args.areas
  else:
    areas = ['all']

  projects = set()

  all_areas = list(areas)

  for area in areas:
    if area not in proj_map:
      print("*** no such area: " + area, file=sys.stderr)
    else:
      all_areas += _area_areas[area]

  for area in all_areas:
    projects.update(set(proj_map.get(area, [])))

  return projects


def find_package_projects(build_args):
  """ Search the @basedir for all the .package projects applicable to host and target. """
  mabu_projects = list(get_area_projects(build_args))

  # split projects by (pure) host and target
  target_mabu_projects = [p for p in mabu_projects if "com.magicleap" in p]

  for p in target_mabu_projects:
    mabu_projects.remove(p)

  return mabu_projects, target_mabu_projects


def find_extension_projects(basedir):
  """ Find the .mabu projects that build dependent sources. """
  mabu_projects = []

  for dirpath, dirnames, filenames in os.walk(basedir):
    if 'External' not in dirpath:
      for f in filenames:
        if f.endswith(".mabu"):
          full = os.path.join(dirpath, f)
          with open(full, 'r') as f:
            text = f.read()
            if 'EXTENSIONS' in text:
              mabu_projects.append(full)

  return mabu_projects


def find_files(d, functor):
  bins = []
  for dirpath, dirnames, filenames in os.walk(d):
    if is_non_cli_binary_path(dirpath):
      # don't recurse
      dirnames[:] = []
      continue
    for file in filenames:
       full = os.path.join(dirpath, file)
       if functor(full):
         bins.append(full)
  return bins


def is_non_cli_binary_path(path):
  """ Detect if directory searches descend into non-CLI tools """
  return 'uifrontend' in path.lower() or 'unity' in path.lower()


def find_files_in_directory(d, functor):
  bins = []
  for dirpath, dirnames, filenames in os.walk(d):
    if is_non_cli_binary_path(dirpath):
      # don't recurse
      dirnames[:] = []
      continue

    if functor(dirpath):
      # this is a match: gobble up everything
      for dirpath, dirnames, filenames in os.walk(dirpath):
        for file in filenames:
          full = os.path.join(dirpath, file)
          bins.append(full)
      dirnames[:] = []
      continue

    # keep only directories matching `functor`
    dirnames[:] = [dirname for dirname in dirnames if functor(os.path.join(dirpath, dirname))]

  return bins


def is_elf(path):
  with open(path, 'rb') as f:
    sig = f.read(4)
    return sig == b'\x7fELF'


def is_macho(path):
  with open(path, 'rb') as f:
    sig = f.read(4)
    return sig == b'\xCF\xFA\xED\xFE'


if sys.platform == 'linux':
  def is_strippable(path):
    if ".sym" in path:
      return False

    return is_elf(path)

  def is_executable(path):
    ext = os.path.splitext(path)[1]
    return ext == "" and is_elf(path)

elif sys.platform == 'darwin':
  def is_strippable(path):
    if ".dSYM" in path:
      return False

    return is_macho(path)

  def is_executable(path):
    ext = os.path.splitext(path)[1]
    return ext == "" and is_macho(path)

else:
  def is_executable(path):
    ext = os.path.splitext(path)[1].lower()
    return ext == '.exe'


def fix_permissions(distdir):
  print_step("Correcting file permissions...")

  exes = find_files(distdir, is_executable)

  for exe in exes:
    if is_verbose:
      print("...", exe)
    os.chmod(exe, 0o777)

  print()
  return 0


class AreaParserAction(argparse.Action):
  def __init__(self, option_strings, dest, nargs=None, **kwargs):
    if nargs is not None:
      raise ValueError("nargs not allowed")
    super(AreaParserAction, self).__init__(option_strings, dest, **kwargs)

  def __call__(self, parser, namespace, area, option_string=None):
    areas = [x.strip() for x in area.strip("'\"").split(',')]
    setattr(namespace, self.dest, (getattr(namespace, self.dest) or []) + areas)


def register_common_args(parser):
  parser.add_argument('-v', '--verbose', action='count', default=0, dest='verbose',
                      help='''increase verbosity incrementally''')

  parser.add_argument('--build', action='store_true', default=None,
                      help='''build incrementally (default)''')
  parser.add_argument('--no-build', dest='build', action='store_false',
                      help='''don't build anything''')
  parser.add_argument('--test', action='store_true', default=None,
                      help='''run tests (default)''')
  parser.add_argument('--no-test', dest='test', action='store_false',
                      help='''don't run tests''')

  parser.add_argument('--host', '--native', dest='host',action='store_true', default=None,
                      help='''build host content (default)''')
  parser.add_argument('--no-host', '--no-native', dest='host', action='store_false',
                      help='''don't build host content''')
  parser.add_argument('--target', '--device', dest='target', action='store_true', default=None,
                      help='''build ML1 content (default)''')
  parser.add_argument('--no-target', '--no-device', dest='target', action='store_false',
                      help='''don't build ML1 content''')

  parser.add_argument('--c++', dest='cpp', action='store_true', default=True,
                      help='''process C/C++ components (default)''')
  parser.add_argument('--no-c++', dest='cpp', action='store_false',
                      help='''don't process C/C++ components''')
  parser.add_argument('--build-deps', dest='build_deps', action='store_true', default=None,
                      help='''build external dependencies''')


def canonicalize_args(build_args, mabu_args):
  global is_verbose

  # global use
  is_verbose = build_args.verbose > 0
  if build_args.verbose > 1:
    mabu_args.append("-v")

  # avoid repeated lookups
  build_args.mabu = find_mabu()
  build_args.mlsdk = find_mlsdk(mabu_args)
  build_args.host_spec, build_args.target_spec = _ensure_build_spec(mabu_args)

  # get the default areas
  if hasattr(build_args, 'areas') and not build_args.areas:
    if getattr(build_args, 'release_build', None):
      build_args.areas = ['release']
    else:
      build_args.areas = ['all']

  # standardize flags

  # unset values are None, convert them to True
  if getattr(build_args, 'clean', None):
    # when cleaning, assume no build after
    build_args.build = build_args.build == True
    build_args.test = build_args.test == False
  else:
    build_args.build = build_args.build != False
    build_args.test = build_args.test != False

  build_args.host = build_args.host != False
  build_args.target = build_args.target != False


if __name__ == "__main__":
  print("Silly human.  This script does nothing on its own.  Look elsewhere.")
