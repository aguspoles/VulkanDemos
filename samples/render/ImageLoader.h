#pragma once
#include "render/Texture.h"

namespace prm {

	class ImageLoader {
	public:
		ImageLoader(const ImageLoader&) = delete;
		ImageLoader(ImageLoader&&) = delete;

		ImageLoader& operator=(const ImageLoader&) = delete;
		ImageLoader& operator=(ImageLoader&&) = delete;

		static void LoadImageFromPath(const std::string& path, void*& outputData, Texture::Extent& imageSize);
		static void UnloadImage(void* imagaData);

	private:
		ImageLoader();
	};
}
