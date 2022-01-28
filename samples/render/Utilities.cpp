#include "pch.h"
#include "Utilities.h"

namespace prm {
    uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, vk::PhysicalDeviceMemoryProperties gpuProperties,
        vk::MemoryPropertyFlagBits desiredProperties)
    {
        //Memory types on the gpu
        for (size_t i = 0; i < gpuProperties.memoryTypeCount; ++i)
        {
            if ((allowedTypes & (1 << i)) &&  //Index of memory type must match corresponding type in allowedTypes
                (gpuProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties) //Desired property bit flags are part of memory type's property flags
            {
                return static_cast<uint32_t>(i);
            }
        }

        throw std::runtime_error("Could not find memory type on device with desired properties");
    }
}
