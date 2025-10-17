#include "pch.h"
#include "ImageLoader.h"
#include "stb_image.h"
#include "render/Texture.h"

namespace prm {
    void ImageLoader::LoadImageFromPath(const std::string& path, void*& outputData, Texture::Extent& imageSize)
    {
        int texWidth, texHeight, texChannels;
        outputData = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize.width = texWidth;
        imageSize.height = texHeight;

        if (!outputData) {
            throw std::runtime_error("failed to load texture image!");
        }
    }

    void ImageLoader::UnloadImage(void* imagaData)
    {
        assert(imagaData);
        stbi_image_free(imagaData);
    }
}