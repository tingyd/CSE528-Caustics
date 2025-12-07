#include "renderer/utils.h"
#include <fstream>
#include <iostream>

const int WIDTH = 800;
const int HEIGHT = 800;

void savePPM(const std::vector<unsigned char>& frameData,
             int width, int height,
             const char* filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create " << filename << std::endl;
        return;
    }

    file << "P6\n" << width << " " << height << "\n255\n";

    // Write rows from top to bottom (y = height-1 → 0)
    for (int y = height - 1; y >= 0; --y)
    {
        const unsigned char* row = &frameData[y * width * 3];
        file.write(reinterpret_cast<const char*>(row), width * 3);
    }

    std::cout << "Saved frame to " << filename << std::endl;
}
