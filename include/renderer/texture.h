#pragma once
#include "camera.h"
#include <vector>

struct Texture {
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool loaded = false;

    bool load(const char* filename);
    Vec3 sample(float u, float v) const;
};