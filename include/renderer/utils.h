#pragma once
#include <vector>

void savePPM(const std::vector<unsigned char>& frameData, int width, int height, const char* filename);

extern const int WIDTH;
extern const int HEIGHT;
