#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
import platform
import shutil

sys.path.insert(0, '../scripts')
from build_project import register_common_args, canonicalize_args

IS_HOST_WINDOWS = platform.system() == "Windows"

MABU = 'mabu.cmd' if IS_HOST_WINDOWS else 'mabu'

ABSOLUTE_PATH = os.path.dirname(os.path.realpath(__file__))

MABU_PATH = shutil.which(MABU)
if MABU_PATH is not None:
  SDK_PATH = os.path.dirname(MABU_PATH)
else:
  print("Could not locate " + MABU + ", ensure you've ran 'source envsetup.sh' or 'envsetup.bat'")
  exit(1)

def register_args(parser):
    parser.add_argument('--clean', '-c', action='store_true',
                        help='''remove the build output of external dependencies''')
    parser.add_argument('--rebuild', '-r', action='store_true',
                        help='''clean then build the external dependencies''')
    parser.add_argument('--release-build', '--rb', action='store_true',
                        help='''build with a release configuration instead of a debug configuration''')
    parser.add_argument('--parallel-cmake', dest='parallel_cmake', action='store_true', default=None,
                        help='''build cmake dependencies in parallel''')

def create_toolchain_file(target):
    mabu_spec = subprocess.run(
      [MABU, '--target', target, '--create-cmake-toolchain', os.path.join('external', 'cmake', 'mlsdk.toolchain.cmake')], stdout=subprocess.PIPE, check=True)

def run_cmake(target, parallel=False):
    additional_defs = []
    if 'device' in target:
        additional_defs += ['-DCMAKE_TOOLCHAIN_FILE=cmake/mlsdk.toolchain.cmake',
                            '-G', 'Unix Makefiles']
        if IS_HOST_WINDOWS:
            make_program = os.path.join(
                SDK_PATH, 'tools', 'mabu', 'tools', 'MinGW', 'msys', '1.0', 'bin', 'make.exe')
            additional_defs += ['-DCMAKE_MAKE_PROGRAM=' + make_program]
    elif IS_HOST_WINDOWS:
        additional_defs += ['-DCMAKE_GENERATOR_PLATFORM=x64']

    additional_build_params = []
    if parallel:
        additional_build_params += ['--parallel']

    build_type = 'Release' if 'release' in target else 'Debug'

    mabu_spec = subprocess.run(
        [MABU, '--print-spec', '--target', target], stdout=subprocess.PIPE, check=True).stdout.decode("utf-8").strip()

    source_path = os.path.join(ABSOLUTE_PATH, 'external')
    build_path = os.path.join(ABSOLUTE_PATH, 'external', 'build', mabu_spec)
    install_path = os.path.join(ABSOLUTE_PATH, 'external', 'package', mabu_spec)

    os.makedirs(build_path, exist_ok=True)
    modified_env = os.environ.copy()

    if 'MLSDK' not in os.environ:
      modified_env['MLSDK'] = SDK_PATH
    else:
      print("Using MLSDK environment variable: '" + os.environ['MLSDK'] + "'")

    subprocess.run(['cmake',
                    source_path,
                    '-DCMAKE_BUILD_TYPE=' + build_type,
                    '-DCMAKE_INSTALL_PREFIX=' + install_path,
                    '-DCMAKE_SKIP_INSTALL_RPATH=TRUE'
                    ] + additional_defs, env=modified_env, cwd=build_path, check=True)
    subprocess.run(['cmake',
                    '--build', build_path,
                    '--config', build_type,
                    '--target', 'install',
                    ] + additional_build_params, cwd=build_path, check=True)


def build_external(build_args):
    build_config = 'release' if build_args.release_build else 'debug'
    parallel = build_args.parallel_cmake
    if build_args.host:
        run_cmake(build_config + '_host', parallel)
    if build_args.target:
        os.makedirs(os.path.join('external', 'cmake'), exist_ok=True)
        create_toolchain_file(build_config + '_device')
        run_cmake(build_config + '_device', parallel)


def clean():
    base_build_path = os.path.join(ABSOLUTE_PATH, 'external', 'build')
    base_install_path = os.path.join(ABSOLUTE_PATH, 'external', 'package')
    shutil.rmtree(base_build_path, ignore_errors=True)
    shutil.rmtree(base_install_path, ignore_errors=True)


def parse_args():
    parser = argparse.ArgumentParser("Build external dependencies that use build systems other than mabu")
    register_common_args(parser)
    register_args(parser)

    build_args = parser.parse_args()
    mabu_args = []
    if build_args.release_build:
        mabu_args += ['-t', 'release']

    canonicalize_args(build_args, mabu_args)
    return build_args


if __name__ == '__main__':
    build_args = parse_args()
    if build_args.clean or build_args.rebuild:
        clean()
    if build_args.build:
        build_external(build_args)
