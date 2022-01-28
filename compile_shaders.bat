mkdir output
%VULKAN_SDK%\Bin\glslc.exe assets/shaders/diffuse.vert -o output/diffuse_vert.spv
%VULKAN_SDK%\Bin\glslc.exe assets/shaders/diffuse.frag -o output/diffuse_frag.spv
pause