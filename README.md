
# Vulkan Demo

Learn Vulkan and graphics programming

## Installation

#### Dependencies 
- [CMake](https://cmake.org/) 
- [VulkanSDK](https://vulkan.lunarg.com/sdk/home)

#### Setup
Clone repo alongside third-party repos
```bash
  git clone --recurse-submodules
```
Or initialize the submodules after cloning
```bash
  git submodule init
  git submodule update
```
Build project for visual studio 2022
```bash
  cmake -G "Visual Studio 17 2022" -A x64 -S . -Bbuild
```
To compile shaders use the compile_shaders.bat that uses the spir-v compiler provided by the SDK.
    
