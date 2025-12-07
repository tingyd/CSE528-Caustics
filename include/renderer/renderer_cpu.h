#pragma once
#include <random>
#include "renderer/camera.h"
#include "renderer/scene.h"
#include "renderer/photon_map.h"
#include "renderer/utils.h"

const int CAUSTIC_PHOTON_COUNT = 30000;
const int GLOBAL_PHOTON_COUNT = 15000;
const int MAX_GATHER_PHOTONS = 50;
const float INITIAL_RADIUS = 50.0f;
void processInputCPU(GLFWwindow *window, float deltaTime, bool &cameraMoving,
                     bool &savePPMRequested);
Vec3 cosineWeightedHemisphere(const Vec3 &normal, std::mt19937 &rng);
void tracePhotons(PhotonMap &causticMap, PhotonMap &globalMap, std::mt19937 &rng);
Vec3 directLighting(const Vec3 &pos, const Vec3 &normal, std::mt19937 &rng);
Vec3 radianceEstimate(const PhotonMap &map, const Vec3 &pos, const Vec3 &normal,
                      const Vec3 &wo, int material, float u, float v, int textureId,
                      float initialRadius);
Vec3 trace(Vec3 ro, Vec3 rd, const PhotonMap &causticMap, const PhotonMap &globalMap,
           std::mt19937 &rng, int depth = 0);
Vec3 renderPixel(float px, float py, const CPUCamera &cam,
                 const PhotonMap &causticMap, const PhotonMap &globalMap,
                 std::mt19937 &rng);

extern bool texturesEnabled;
