#include "pch.h"
#include "core/Helpers.h"

namespace prm
{
    std::vector<char> read_shader_file(const std::string& filename)
    {
        //Open file and go to end of file
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if(!file.is_open())
        {
            throw std::runtime_error("Failed to open shader file!");
        }

        const size_t fileSize = file.tellg();
        std::vector<char> fileBuffer(fileSize);

        file.seekg(0);//Return to position 0 of file

        //Read file data into buffer
        file.read(fileBuffer.data(), fileSize);

        file.close();

        return fileBuffer;
    }
}
