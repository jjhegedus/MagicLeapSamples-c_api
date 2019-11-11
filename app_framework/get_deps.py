#!/usr/bin/env python3
import zipfile, urllib.request, shutil
import os
import sys
import platform

vi = sys.version_info
if platform.system() == 'Darwin':
  if vi.major < 3 or (vi.major == 3 and vi.minor < 7):
    print("Please run this script with Python 3.7 or newer")
    exit(1)
else:
  if vi.major < 3 or (vi.major == 3 and vi.minor < 5):
    print("Please run this script with Python 3.5 or newer")
    exit(1)

if __name__ == '__main__':

    deps = [('glm', 'external/glm-0.9.9.4.zip', 'https://github.com/g-truc/glm/archive/0.9.9.4.zip', 'external'),
            ('gflags', 'external/gflags-2.2.2.zip', 'https://github.com/gflags/gflags/archive/v2.2.2.zip', 'external'),
            ('glfw', 'external/glfw-3.2.1.zip', 'https://github.com/glfw/glfw/archive/3.2.1.zip', 'external'),
            ('gtest', 'external/gtest-1.7.0.zip', 'https://github.com/google/googletest/archive/release-1.7.0.zip', 'external'),
            ('stb', 'external/stb.zip', 'https://github.com/nothings/stb/archive/master.zip', 'external'),
            ('assimp', 'external/assimp-v.5.0.0.rc1.zip', 'https://github.com/assimp/assimp/archive/v.5.0.0.rc1.zip', 'external'),
            ('imgui', 'external/imgui-1.6.8.zip', 'https://github.com/ocornut/imgui/archive/v1.68.zip', 'external'),
            ('json', 'external/json-3.6.1.zip', 'https://github.com/nlohmann/json/archive/v3.6.1.zip', 'external')
    ]


    for dep in deps:
        print("Downloading {0} : {1}".format(dep[0], dep[2]))
        with urllib.request.urlopen(dep[2]) as response, open(dep[1], 'wb') as out_file:
            shutil.copyfileobj(response, out_file)

        if os.path.isfile(os.path.join(os.getcwd(), dep[1])):
            with zipfile.ZipFile(os.path.join(os.getcwd(), out_file.name), 'r') as zip_ref:
                zip_ref.extractall(dep[3])
