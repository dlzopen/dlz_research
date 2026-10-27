#!/bin/bash
set -e
cd /c/seesaw/source/dlz_research

VSMSVC="/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.37.32822"
SDK="/c/Program Files (x86)/Windows Kits/10"

export PATH="$VSMSVC/bin/Hostx64/x64:$SDK/bin/10.0.22621.0/x64:/c/Program Files/CMake/bin:/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja:$PATH"

export INCLUDE='C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.37.32822\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared'

export LIB='C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.37.32822\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64'

export LIBPATH="$LIB"

cmake --build dlzr/build --target dlzr_tests 2>&1 | tail -n 30
echo "BUILD_EXIT=${PIPESTATUS[0]}"
