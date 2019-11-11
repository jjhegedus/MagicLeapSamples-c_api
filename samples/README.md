# Fetching the Lumin SDK #

To be able to build any of the sample applications requires you to have
an Lumin SDK, which contains the toolchain and the build system 'mabu'
(Magic Builder). To download the Lumin SDK, go to the [Creator
Portal](https://creator.magicleap.com/downloads/lumin-sdk/overview).

# Getting a Developer Certificate #

Go to the [Creator
Portal](https://creator.magicleap.com/downloads/lumin-sdk/overview)
and navigate to Publish => Certificates and Click "Add New".  A
privatekey.zip will be downloaded but you also have to wait for the
Status to be "Active" and a Download link will appear on the same row.
Unzip the private key and put that and the .cert file from the
Download link in the same directory.  Set the environment variable
MLCERT to point to the .cert file.

# Building the Sample Apps #

1. If you installed these samples via Package Manager you can use the "Open Shell" button to get a shell that's good to go otherwise follow steps 2-4.
2. `cd /path/to/mlsdk`
3. If on linux / OSX: `source envsetup.sh`. If on Windows: `envsetup.bat`
4. If on linux / OSX: `export MLSDK=/path/to/mlsdk`. If on Windows: `set MLSDK=/path/to/mlsdk`
5. If on linux / OSX: `export MLCERT=/path/to/cert_from_creator_portal`. If on Windows: `set MLCERT=/path/to/cert_from_creator_portal`
5. `cd /mlsdk/<version>/samples/c_api/app_framework`
6. `python3.5 get_deps.py`  Note: On OSX you may need python3.7 if you run into issues related to SSL/TLS.
7. `python3.5 build_external.py`
8. `cd /mlsdk/<version>/samples/c_api/samples`
9. `python3.5 build.py`

# Cross-Building Notes #

The theory of development for Lumin SDK is that you can build and debug on host as well as on device.  It provides enough headers
and libraries for Lumin API development on all supported platforms (under `include` and `lib`).  It also bundles a toolchain
and C / C++ libraries for native development for the Magic Leap device (under `lumin/`).

Although host development is well-supported, Lumin SDK does not bundle host toolchains -- you are expected to install
e.g. Visual Studio 2017 or 2019 for Windows and the Windows 8+ SDK or Xcode for macOS yourself.
`mabu` will detect such tools in their expected installation locations.

Similarly, you should be able to use any reasonably cross-platform third-party code with `mabu` (the native build system)
but we do not provide ready-made builds or ports of such code.

# Running under ML Remote #

Follow the instructions at https://creator.magicleap.com/learn/guides/tools-magic-leap-remote

Start ML Remote and then Start Simulator.

Then launch your application:

   * Set up the ML Remote environment (`mlsdk\VirtualDevice\mlvdsetup.bat` or `source mlsdk/VirtualDevice/mlvdsetup.sh`).
   * Invoke `samples_envsetup.bat` or `samples_envsetup.sh`.
   * cd `BUILD/<spec>`
   * Run the binary located here `BUILD/<spec>/<binary>`.

(The canonical way to launch a mabu app is `mabu <someprogram> --invoke`, but this has some limitations.  Also, we need to ensure
GLFW is available on the library path.  This script makes use of `samples_envsetup.bat` or `samples_envsetup.sh` to do that.)
