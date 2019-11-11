#! /usr/bin/env bash

# exit on failures
set -e

echo -e "\n[`date +%H:%M:%S`] >>>>>> ML Remote Shared Library Library Fixup\n"

BIN_FOLDER=$1
echo "bin_folder: ${BIN_FOLDER}"

if [ -z "$BIN_FOLDER" ] || [ ! -d "$BIN_FOLDER" ]; then
    echo "Expected a BIN_FOLDER argument pointing to a directory (e.g. dist/sdk_native_tests/prod/native_tests)"
    exit 1;
fi

# In our case we are dumping everything in the same directory so private_lib_folder is the same.
PRIVATE_LIB_FOLDER=$1

function get_max_filename_length() {
    # Do a prepass to get max filename length, so output is nicer
    local MAX_LENGTH=0
    for file in "$@"
    do
        local FILENAME=${file##*/}
        local FILENAME_LEN=${#FILENAME}
        if [ "$FILENAME_LEN" -gt "$MAX_LENGTH" ]; then
            MAX_LENGTH=$FILENAME_LEN
        fi
    done
    echo $MAX_LENGTH
}

function convert_library_lookup_osx() {
    # Go through each executable and shim lib and ensure it loads the local copies of libs
    INTERNAL_PATH_PREFIX=@loader_path
    INTERNAL_PATH_PREFIX_LEN=${#INTERNAL_PATH_PREFIX}

    EXECUTABLES=`find $BIN_FOLDER -type f -regex '.*/[^.]*'`
    PRIVATE_LIBS=`find $PRIVATE_LIB_FOLDER -type f -name \*.dylib`

    MAX_NAME_LENGTH=`get_max_filename_length $EXECUTABLES $PRIVATE_LIBS`

    echo "* Renaming references of private dylibs to [ $INTERNAL_PATH_PREFIX ]"
    for bin in $EXECUTABLES $PRIVATE_LIBS
    do
        FILENAME=${bin##*/}
        echo  "    * Checking binary $FILENAME"

        # Parse the otool -L output to identify dependent dylibs
        otool -L $bin | sed 1d | \
        while read dep_line
        do
            DEP=`echo $dep_line | awk '{print $1;}'`

            # Because of the way gflags is built it prefers adding an
            # @rpath on OSX. In the CMakeLists.txt it specifies CMake policy
            # CMP0042 as new which adds an @rpath. Single out this lib
            # and use @loader_path instead since we are dumping
            # everything into the same directory.
            regex='@rpath/(libgflags_nothreads.*\.dylib)'
            if [[ ${DEP} =~ $regex ]]; then
                install_name_tool -change $DEP $INTERNAL_PATH_PREFIX/${BASH_REMATCH[1]} $bin
            fi

            # If the dependency starts with /, it's likely a system lib,
            # if with @, it's already redirected
            if [[ ${DEP:0:1} == "/" ]] || [[ ${DEP:0:1} == "@" ]]; then
                continue
            fi

            # Prevent copying own file
            if [ "$DEP" == "$FILENAME" ]; then
                continue
            fi

            # If we've made it here, this is a legit dependency
            regex='lib/(.*)'
            if [[ $DEP =~ $regex ]]; then
                echo "        * Identified dependency ${BASH_REMATCH[1]}"
                install_name_tool -change $DEP $INTERNAL_PATH_PREFIX/${BASH_REMATCH[1]} $bin
            else
                echo "        * Identified dependency $DEP"
                install_name_tool -change $DEP $INTERNAL_PATH_PREFIX/$DEP $bin
            fi
        done
    done
}

OS=`uname`

if [ "$OS" = "Darwin" ] ; then
    convert_library_lookup_osx
fi

echo -e "Done.\n"
