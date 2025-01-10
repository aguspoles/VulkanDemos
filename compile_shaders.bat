mkdir output
%VULKAN_SDK%\Bin\glslc.exe assets/shaders/diffuse.vert -o output/diffuse_vert.spv
%VULKAN_SDK%\Bin\glslc.exe assets/shaders/diffuse.frag -o output/diffuse_frag.spv

%VULKAN_SDK%\Bin\glslc.exe assets/shaders/triangle.vert -o output/triangle_vert.spv
%VULKAN_SDK%\Bin\glslc.exe assets/shaders/triangle.frag -o output/triangle_frag.spv
pause