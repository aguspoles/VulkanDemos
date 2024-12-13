Dependencies: CMake, Python 3
Setup:
Clone repo with (git clone --recurse-submodules) or initialize the submodules after cloning (git submodule init / git submodule update).
Build project for visual studio 2022: cmake -G "Visual Studio 17 2022" -A x64 -S . -Bbuild
