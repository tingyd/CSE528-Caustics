#include "renderer/scene.h"
#include <cmath>
#include <algorithm>

Texture floorTexture;
Texture backWallTexture;
Texture ceilingTexture;
bool texturesEnabled = true;

Vec3 getMaterialColor(int mat, float u, float v, int textureId) {
    if (texturesEnabled) {
        if (textureId == 0 && floorTexture.loaded) {
            return floorTexture.sample(u * 2.0f, v * 2.0f);
        }
        if (textureId == 1 && backWallTexture.loaded) {
            return backWallTexture.sample(u, v);
        }
        if (textureId == 2 && ceilingTexture.loaded) {
            return ceilingTexture.sample(u * 2.0f, v * 2.0f);
        }
    }
    
    if (mat == 0) return Vec3(0.73f, 0.73f, 0.73f);
    if (mat == 3) return Vec3(0.65f, 0.05f, 0.05f);
    if (mat == 4) return Vec3(0.12f, 0.45f, 0.15f);
    if (mat == 5) return Vec3(15.0f, 15.0f, 15.0f);
    return Vec3(1, 1, 1);
}

float getMaterialAlpha(int mat) {
    if (mat == 0) return 1.0f;
    if (mat == 3) return 0.8f;
    if (mat == 4) return 0.8f;
    return 1.0f;
}

bool intersectSphere(Vec3 ro, Vec3 rd, Vec3 center, float radius, Hit &hit, int mat) {
    Vec3 oc = ro - center;
    float a = rd.dot(rd);
    float b = oc.dot(rd);
    float c = oc.dot(oc) - radius * radius;
    float disc = b * b - a * c;
    if (disc < 0) return false;

    float t = (-b - std::sqrt(disc)) / a;
    if (t < 0.001f) t = (-b + std::sqrt(disc)) / a;
    if (t < 0.001f || t >= hit.t) return false;

    hit.t = t;
    hit.point = ro + rd * t;
    hit.normal = (hit.point - center).normalize();
    hit.material = mat;
    hit.textureId = -1;
    
    Vec3 localP = (hit.point - center).normalize();
    hit.u = 0.5f + atan2(localP.z, localP.x) / (2.0f * PI);
    hit.v = 0.5f - asin(localP.y) / PI;
    
    return true;
}

bool intersectPlane(Vec3 ro, Vec3 rd, Vec3 p0, Vec3 normal,
                    float minA, float maxA, float minB, float maxB,
                    Hit &hit, int mat, int texId) {
    float denom = normal.dot(rd);
    if (std::abs(denom) < 0.0001f) return false;

    float t = (p0 - ro).dot(normal) / denom;
    if (t < 0.001f || t >= hit.t) return false;

    Vec3 p = ro + rd * t;

    bool inBounds = false;
    float u = 0, v = 0;

    if (std::abs(normal.y) > 0.5f) {
        inBounds = (p.x >= minA && p.x <= maxA && p.z >= minB && p.z <= maxB);
        u = (p.x - minA) / (maxA - minA);
        v = (p.z - minB) / (maxB - minB);
    } else if (std::abs(normal.x) > 0.5f) {
        inBounds = (p.z >= minA && p.z <= maxA && p.y >= minB && p.y <= maxB);
        u = (p.z - minA) / (maxA - minA);
        v = (p.y - minB) / (maxB - minB);
    } else if (std::abs(normal.z) > 0.5f) {
        inBounds = (p.x >= minA && p.x <= maxA && p.y >= minB && p.y <= maxB);
        u = (p.x - minA) / (maxA - minA);
        v = (p.y - minB) / (maxB - minB);
    }

    if (!inBounds) return false;

    hit.t = t;
    hit.point = p;
    hit.normal = normal;
    hit.material = mat;
    hit.u = u;
    hit.v = v;
    hit.textureId = texId;
    return true;
}

bool intersectScene(Vec3 ro, Vec3 rd, Hit &hit, bool includeLight) {
    bool hitAny = false;

    if (intersectPlane(ro, rd, Vec3(0, 0, 0), Vec3(0, 1, 0), 0, 552.8f, 0, 559.2f, hit, 0, 0))
        hitAny = true;
    if (intersectPlane(ro, rd, Vec3(0, 548.8f, 0), Vec3(0, -1, 0), 0, 552.8f, 0, 559.2f, hit, 0, 2))
        hitAny = true;
    if (intersectPlane(ro, rd, Vec3(0, 0, 559.2f), Vec3(0, 0, -1), 0, 552.8f, 0, 548.8f, hit, 0, 1))
        hitAny = true;
    if (intersectPlane(ro, rd, Vec3(552.8f, 0, 0), Vec3(-1, 0, 0), 0, 559.2f, 0, 548.8f, hit, 3, -1))
        hitAny = true;
    if (intersectPlane(ro, rd, Vec3(0, 0, 0), Vec3(1, 0, 0), 0, 559.2f, 0, 548.8f, hit, 4, -1))
        hitAny = true;

    if (includeLight) {
        if (intersectPlane(ro, rd, Vec3(278, 548.7f, 279.5f), Vec3(0, -1, 0), 213, 343, 227, 332, hit, 5, -1))
            hitAny = true;
    }

    if (intersectSphere(ro, rd, Vec3(185, 80, 169), 80.0f, hit, 1))
        hitAny = true;
    if (intersectSphere(ro, rd, Vec3(368, 80, 351), 80.0f, hit, 2))
        hitAny = true;

    return hitAny;
}