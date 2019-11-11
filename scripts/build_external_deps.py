#!/usr/bin/python

from __future__ import print_function
from subprocess import Popen, PIPE
from only_common import find_mlsdk
from distutils.dir_util import copy_tree
import argparse
import os
import platform
import sys

win = platform.system() == 'Windows'
osx = platform.system() == 'Darwin'
linux = platform.system() == 'Linux'

plat = "undefined"
if win:
  plat = "win64"
elif osx:
  plat = "osx"
elif linux:
  plat = "linux64"

def main():
  if plat == "undefined":
    print("FATAL: Failed to detect platform. Exiting")
    exit(1)

  parser = argparse.ArgumentParser(description='Script to build external dependencies of tests.')
  args = parser.parse_args()

  mabuVersionResult = run([
    'mabu',
    '--version'
  ])

  if mabuVersionResult[0] is not 0:
    print ("! Error: mabu not available at command line. Run 'envsetup.sh' or make sure a LuminSDK install is available on your PATH")
    exit(1)

  filePath = os.path.dirname(os.path.realpath(__file__))
  appFrameworkPath = os.path.join(filePath, "..", "app_framework")

  baseCmd = [
    sys.executable,
    'get_deps.py',
  ]

  print("> Getting external dependencies. cwd={} cmd={}".format(appFrameworkPath, baseCmd))
  rawResults = run(baseCmd, appFrameworkPath)

  baseCmd = [
    sys.executable,
    'build_external.py',
  ]

  print("> Building external dependencies. cwd={} cmd={}".format(appFrameworkPath, baseCmd))
  rawResults = run(baseCmd, appFrameworkPath)
  exit(0)

def run(cmd, cwd='.'):
  p = Popen(' '.join(cmd), shell=True, stderr=sys.stderr, stdout=sys.stdout, universal_newlines=True, cwd=cwd)
  stdout, stderr = p.communicate()
  return (p.returncode, stdout, stderr)

if __name__ == "__main__":
  main()
