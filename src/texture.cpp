#include "renderer/texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <cmath>
#include <algorithm>

bool Texture::load(const char* filename) {
    unsigned char* imgData = stbi_load(filename, &width, &height, &channels, 3);
    if (!imgData) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return false;
    }
    data.assign(imgData, imgData + width * height * 3);
    stbi_image_free(imgData);
    channels = 3;
    loaded = true;
    std::cout << "Loaded texture: " << filename << " (" << width << "x" << height << ")" << std::endl;
    return true;
}

Vec3 Texture::sample(float u, float v) const {
    if (!loaded) return Vec3(1, 1, 1);
    
    u = u - std::floor(u);
    v = v - std::floor(v);
    
    float fx = u * (width - 1);
    float fy = v * (height - 1);
    int x0 = (int)fx;
    int y0 = (int)fy;
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);
    float dx = fx - x0;
    float dy = fy - y0;

    auto getPixel = [this](int x, int y) -> Vec3 {
        int idx = (y * width + x) * 3;
        return Vec3(
            data[idx] / 255.0f,
            data[idx + 1] / 255.0f,
            data[idx + 2] / 255.0f
        );
    };

    Vec3 c00 = getPixel(x0, y0);
    Vec3 c10 = getPixel(x1, y0);
    Vec3 c01 = getPixel(x0, y1);
    Vec3 c11 = getPixel(x1, y1);

    Vec3 c0 = c00 * (1 - dx) + c10 * dx;
    Vec3 c1 = c01 * (1 - dx) + c11 * dx;
    
    return c0 * (1 - dy) + c1 * dy;
}