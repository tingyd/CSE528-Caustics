#pragma once
#include "camera.h"
#include "texture.h"

const float PI = 3.14159265359f;

// Global textures
extern Texture floorTexture;
extern Texture backWallTexture;
extern Texture ceilingTexture;
extern bool texturesEnabled;

struct Hit {
    float t = 1e30f;
    Vec3 point, normal;
    float u = 0, v = 0;
    int material = -1;
    int textureId = -1;
};

Vec3 getMaterialColor(int mat, float u = 0, float v = 0, int textureId = -1);
float getMaterialAlpha(int mat);
bool intersectSphere(Vec3 ro, Vec3 rd, Vec3 center, float radius, Hit &hit, int mat);
bool intersectPlane(Vec3 ro, Vec3 rd, Vec3 p0, Vec3 normal,
                    float minA, float maxA, float minB, float maxB,
                    Hit &hit, int mat, int texId = -1);
bool intersectScene(Vec3 ro, Vec3 rd, Hit &hit, bool includeLight = true);