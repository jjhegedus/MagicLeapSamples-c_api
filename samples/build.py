####################################################################################
# Build and test native sample apps
#
# Run with -help for info.
####################################################################################
import sys

vi = sys.version_info
if vi.major < 3 or (vi.major == 3 and vi.minor < 5):
  print("Please run this script with Python 3.5 or newer")
  exit(1)

sys.path.insert(0, '../scripts')

from build_project import parse_args, build
import os
from only_common import launch

if __name__ == "__main__":
  base = os.path.dirname(os.path.realpath( __file__)) or '.'
  dist = os.path.join(base, "dist", "sdk_native_samples")
  packages = [os.path.join(base, "sdk_native_samples.package")]
  project = os.path.join(base, "project_areas.json")

  build_args, mabu_args = parse_args(project)

  if build_args.build_deps:
    args = [
      sys.executable,
      '../scripts/build_external_deps.py'
    ]
    launch(args, stdout=sys.stdout, stderr=sys.stderr)

  return_code = build(build_args, mabu_args, base_dir = base, project = project, dist_dir = dist, packages = packages)
  exit(return_code)
