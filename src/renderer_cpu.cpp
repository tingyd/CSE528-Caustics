#include "renderer_cpu.h"
#include "scene.h"
#include <random>
#include <cmath>
#include <iostream>
#include <algorithm>
#include "renderer/camera.h"
extern bool texturesEnabled;
extern Texture floorTexture;
extern Texture backWallTexture;
extern Texture ceilingTexture;
float fresnelDielectric(float cosThetaI, float etaI, float etaT)
{
    cosThetaI = std::clamp(cosThetaI, -1.0f, 1.0f);

    if (cosThetaI < 0)
    {
        std::swap(etaI, etaT);
        cosThetaI = -cosThetaI;
    }

    float sinThetaI = std::sqrt(std::max(0.0f, 1.0f - cosThetaI * cosThetaI));
    float sinThetaT = etaI / etaT * sinThetaI;

    if (sinThetaT >= 1.0f)
        return 1.0f;

    float cosThetaT = std::sqrt(std::max(0.0f, 1.0f - sinThetaT * sinThetaT));

    float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) /
                  ((etaT * cosThetaI) + (etaI * cosThetaT));
    float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) /
                  ((etaI * cosThetaI) + (etaT * cosThetaT));

    return (Rparl * Rparl + Rperp * Rperp) / 2.0f;
}

Vec3 refractVec(Vec3 wi, Vec3 n, float eta)
{
    float cosThetaI = (-wi).dot(n);
    if (cosThetaI < 0)
    {
        n = -n;
        cosThetaI = -cosThetaI;
    }

    float sin2ThetaI = std::max(0.0f, 1.0f - cosThetaI * cosThetaI);
    float sin2ThetaT = eta * eta * sin2ThetaI;

    if (sin2ThetaT >= 1.0f)
        return Vec3(0, 0, 0);

    float cosThetaT = std::sqrt(1.0f - sin2ThetaT);
    return eta * wi + (eta * cosThetaI - cosThetaT) * n;
}

Vec3 reflectVec(Vec3 v, Vec3 n)
{
    return v - 2.0f * v.dot(n) * n;
}

float schlickBRDF(const Vec3 &n, const Vec3 &wo, const Vec3 &wi, float alpha)
{
    float v = std::max(n.dot(wo), 0.001f);
    float vp = std::max(n.dot(wi), 0.001f);

    if (v <= 0 || vp <= 0)
        return 0;

    auto G = [alpha](float x)
    { return x / (alpha - alpha * x + x); };

    float Gv = G(v);
    float Gvp = G(vp);
    float GProduct = Gv * Gvp;

    Vec3 h = (wo + wi).normalize();
    float nh = n.dot(h);
    float t = nh;

    float diffuse = (1.0f - GProduct) / PI;

    float denom = 1.0f + alpha * t * t - t * t;
    float specular = GProduct / (4.0f * PI * v * vp) * (alpha / (denom * denom));

    return diffuse + specular;
}
Vec3 cosineWeightedHemisphere(const Vec3 &normal, std::mt19937 &rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r1 = dist(rng);
    float r2 = dist(rng);

    float z = std::sqrt(1.0f - r2);
    float phi = 2.0f * PI * r1;
    float x = std::cos(phi) * std::sqrt(r2);
    float y = std::sin(phi) * std::sqrt(r2);

    Vec3 tangent = std::abs(normal.y) > 0.9f ? Vec3(1, 0, 0) : Vec3(0, 1, 0);
    Vec3 bitangent = normal.cross(tangent).normalize();
    tangent = bitangent.cross(normal);

    return (tangent * x + normal * z + bitangent * y).normalize();
}

Vec3 radianceEstimate(const PhotonMap &map, const Vec3 &pos, const Vec3 &normal,
                      const Vec3 &wo, int material, float u, float v, int textureId,
                      float initialRadius)
{
    if (map.size() == 0)
        return Vec3(0, 0, 0);

    std::priority_queue<PhotonDistEntry> heap;
    float maxDistSq = initialRadius * initialRadius;

    map.locatePhotons(pos, MAX_GATHER_PHOTONS, maxDistSq, heap);

    if (heap.empty())
        return Vec3(0, 0, 0);

    Vec3 result(0, 0, 0);
    float alpha = getMaterialAlpha(material);
    Vec3 albedo = getMaterialColor(material, u, v, textureId);

    float finalRadiusSq = heap.top().distSq;

    while (!heap.empty())
    {
        const Photon *p = heap.top().photon;
        heap.pop();

        float nDotWi = normal.dot(p->incomingDir);
        if (nDotWi <= 0)
            continue;

        float brdf = schlickBRDF(normal, wo, p->incomingDir, alpha);
        result += p->power * brdf;
    }

    float area = PI * finalRadiusSq;
    if (area > 0)
    {
        result = result * (1.0f / area);
    }

    return result * albedo;
}

void tracePhotons(PhotonMap &causticMap, PhotonMap &globalMap, std::mt19937 &rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float lightCenterX = 278.0f;
    float lightCenterZ = 279.5f;
    float lightHalfW = 65.0f;
    float lightHalfD = 52.5f;

    Vec3 causticPhotonPower = Vec3(1.0f, 1.0f, 1.0f) * (2500000.0f / CAUSTIC_PHOTON_COUNT);
    Vec3 globalPhotonPower = Vec3(1.0f, 1.0f, 1.0f) * (1000000.0f / GLOBAL_PHOTON_COUNT);

    std::cout << "Tracing caustic photons..." << std::endl;

    for (int i = 0; i < CAUSTIC_PHOTON_COUNT; i++)
    {
        Vec3 ro(
            lightCenterX + (dist(rng) - 0.5f) * 2.0f * lightHalfW,
            548.7f,
            lightCenterZ + (dist(rng) - 0.5f) * 2.0f * lightHalfD);

        Vec3 target = Vec3(185, 80, 169) + Vec3(
                                               (dist(rng) - 0.5f) * 160.0f,
                                               (dist(rng) - 0.5f) * 160.0f,
                                               (dist(rng) - 0.5f) * 160.0f);
        Vec3 rd = (target - ro).normalize();

        Vec3 power = causticPhotonPower;
        bool hitSpecular = false;

        for (int bounce = 0; bounce < 20; bounce++)
        {
            Hit hit;
            if (!intersectScene(ro, rd, hit, false))
                break;

            if (hit.material == 1 || hit.material == 2)
            {
                hitSpecular = true;

                if (hit.material == 2)
                {
                    rd = reflectVec(rd, hit.normal);
                    ro = hit.point + hit.normal * 0.001f;
                    power = power * 0.95f;
                }
                else
                {
                    float ior = 1.5f;
                    bool entering = rd.dot(hit.normal) < 0;
                    Vec3 n = entering ? hit.normal : -hit.normal;
                    float eta = entering ? (1.0f / ior) : ior;

                    float cosTheta = (-rd).dot(n);
                    float Fr = fresnelDielectric(cosTheta, 1.0f, ior);

                    if (dist(rng) < Fr)
                    {
                        rd = reflectVec(rd, n);
                        ro = hit.point + n * 0.001f;
                    }
                    else
                    {
                        Vec3 refracted = refractVec(rd, n, eta);
                        if (refracted.lengthSq() < 0.001f)
                        {
                            rd = reflectVec(rd, n);
                            ro = hit.point + n * 0.001f;
                        }
                        else
                        {
                            rd = refracted.normalize();
                            ro = hit.point - n * 0.001f;
                        }
                    }
                    power = power * 0.99f;
                }
                continue;
            }

            if ((hit.material == 0 || hit.material == 3 || hit.material == 4) && hitSpecular)
            {
                causticMap.store(hit.point, power * getMaterialColor(hit.material, hit.u, hit.v, hit.textureId), (-rd).normalize());
                break;
            }

            break;
        }
    }

    std::cout << "Stored " << causticMap.size() << " caustic photons" << std::endl;
    std::cout << "Tracing global photons..." << std::endl;

    for (int i = 0; i < GLOBAL_PHOTON_COUNT; i++)
    {
        Vec3 ro(
            lightCenterX + (dist(rng) - 0.5f) * 2.0f * lightHalfW,
            548.7f,
            lightCenterZ + (dist(rng) - 0.5f) * 2.0f * lightHalfD);

        Vec3 rd = cosineWeightedHemisphere(Vec3(0, -1, 0), rng);

        Vec3 power = globalPhotonPower;
        bool storedFirst = false;

        for (int bounce = 0; bounce < 10; bounce++)
        {
            Hit hit;
            if (!intersectScene(ro, rd, hit, false))
                break;

            if (hit.material == 0 || hit.material == 3 || hit.material == 4)
            {
                if (storedFirst)
                {
                    globalMap.store(hit.point, power * getMaterialColor(hit.material, hit.u, hit.v, hit.textureId), (-rd).normalize());
                }
                storedFirst = true;

                float survivalProb = std::max(getMaterialColor(hit.material).x,
                                              std::max(getMaterialColor(hit.material).y,
                                                       getMaterialColor(hit.material).z));
                if (dist(rng) > survivalProb)
                    break;
                power = power * (1.0f / survivalProb);

                rd = cosineWeightedHemisphere(hit.normal, rng);
                ro = hit.point + hit.normal * 0.001f;
                power = power * getMaterialColor(hit.material, hit.u, hit.v, hit.textureId);
                continue;
            }

            if (hit.material == 2)
            {
                rd = reflectVec(rd, hit.normal);
                ro = hit.point + hit.normal * 0.001f;
                power = power * 0.95f;
                continue;
            }

            if (hit.material == 1)
            {
                float ior = 1.5f;
                bool entering = rd.dot(hit.normal) < 0;
                Vec3 n = entering ? hit.normal : -hit.normal;
                float eta = entering ? (1.0f / ior) : ior;

                float cosTheta = (-rd).dot(n);
                float Fr = fresnelDielectric(cosTheta, 1.0f, ior);

                if (dist(rng) < Fr)
                {
                    rd = reflectVec(rd, n);
                    ro = hit.point + n * 0.001f;
                }
                else
                {
                    Vec3 refracted = refractVec(rd, n, eta);
                    if (refracted.lengthSq() < 0.001f)
                    {
                        rd = reflectVec(rd, n);
                        ro = hit.point + n * 0.001f;
                    }
                    else
                    {
                        rd = refracted.normalize();
                        ro = hit.point - n * 0.001f;
                    }
                }
                power = power * 0.99f;
                continue;
            }
        }
    }

    std::cout << "Stored " << globalMap.size() << " global photons" << std::endl;
}

Vec3 directLighting(const Vec3 &pos, const Vec3 &normal, std::mt19937 &rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float lightCenterX = 278.0f;
    float lightCenterZ = 279.5f;
    float lightHalfW = 65.0f;
    float lightHalfD = 52.5f;

    Vec3 lightPos(
        lightCenterX + (dist(rng) - 0.5f) * 2.0f * lightHalfW,
        548.7f,
        lightCenterZ + (dist(rng) - 0.5f) * 2.0f * lightHalfD);

    Vec3 toLight = lightPos - pos;
    float distToLight = toLight.length();
    Vec3 L = toLight.normalize();

    Hit shadowHit;
    if (intersectScene(pos + normal * 0.001f, L, shadowHit, true))
    {
        if (shadowHit.t < distToLight - 0.01f && shadowHit.material != 5)
        {
            return Vec3(0, 0, 0);
        }
    }

    float NdotL = std::max(0.0f, normal.dot(L));
    if (NdotL <= 0)
        return Vec3(0, 0, 0);

    float lightArea = 4.0f * lightHalfW * lightHalfD;
    Vec3 lightNormal(0, -1, 0);
    float LNdotL = std::max(0.0f, (-L).dot(lightNormal));

    Vec3 Le(15.0f, 15.0f, 15.0f);
    float G = NdotL * LNdotL / (distToLight * distToLight);

    return Le * G * lightArea / PI;
}

Vec3 trace(Vec3 ro, Vec3 rd, const PhotonMap &causticMap, const PhotonMap &globalMap,
           std::mt19937 &rng, int depth)
{
    if (depth > 10)
        return Vec3(0, 0, 0);

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    Hit hit;

    if (!intersectScene(ro, rd, hit))
    {
        return Vec3(0.01f, 0.01f, 0.02f);
    }

    if (hit.material == 5)
    {
        return getMaterialColor(5);
    }

    // Diffuse surfaces with texture support
    if (hit.material == 0 || hit.material == 3 || hit.material == 4)
    {
        Vec3 wo = (-rd).normalize();

        Vec3 direct = directLighting(hit.point, hit.normal, rng) *
                      getMaterialColor(hit.material, hit.u, hit.v, hit.textureId) / PI;

        Vec3 caustic = radianceEstimate(causticMap, hit.point, hit.normal, wo,
                                        hit.material, hit.u, hit.v, hit.textureId, 30.0f);

        Vec3 indirect = radianceEstimate(globalMap, hit.point, hit.normal, wo,
                                         hit.material, hit.u, hit.v, hit.textureId, INITIAL_RADIUS);

        return direct + caustic + indirect;
    }

    if (hit.material == 2)
    {
        Vec3 reflectDir = reflectVec(rd, hit.normal);
        return trace(hit.point + hit.normal * 0.001f, reflectDir,
                     causticMap, globalMap, rng, depth + 1) *
               0.98f;
    }

    if (hit.material == 1)
    {
        float ior = 1.5f;
        bool entering = rd.dot(hit.normal) < 0;
        Vec3 n = entering ? hit.normal : -hit.normal;
        float eta = entering ? (1.0f / ior) : ior;

        float cosTheta = (-rd).dot(n);
        float Fr = fresnelDielectric(cosTheta, 1.0f, ior);

        Vec3 result(0, 0, 0);

        if (dist(rng) < Fr)
        {
            Vec3 reflectDir = reflectVec(rd, n);
            result = trace(hit.point + n * 0.001f, reflectDir,
                           causticMap, globalMap, rng, depth + 1);
        }
        else
        {
            Vec3 refracted = refractVec(rd, n, eta);
            if (refracted.lengthSq() < 0.001f)
            {
                Vec3 reflectDir = reflectVec(rd, n);
                result = trace(hit.point + n * 0.001f, reflectDir,
                               causticMap, globalMap, rng, depth + 1);
            }
            else
            {
                result = trace(hit.point - n * 0.001f, refracted.normalize(),
                               causticMap, globalMap, rng, depth + 1);
            }
        }

        return result * 0.99f;
    }

    return Vec3(0, 0, 0);
}

Vec3 renderPixel(float px, float py, const CPUCamera &cam,
                 const PhotonMap &causticMap, const PhotonMap &globalMap,
                 std::mt19937 &rng)
{
    float fov = 40.0f;
    float scale = std::tan(fov * 0.5f * PI / 180.0f);

    Vec3 forward = cam.getForward();
    Vec3 right = cam.getRight();
    Vec3 up = cam.getUp();

    Vec3 rd = (right * px + up * py + forward).normalize();

    return trace(cam.position, rd, causticMap, globalMap, rng);
}
void processInputCPU(GLFWwindow *window, float deltaTime,
                     bool &cameraMoving, bool &savePPMRequested)
{
    cameraMoving = false;

    float moveSpeed = 400.0f * deltaTime;

    Vec3 forward = CPUCameraControl::camera.getForward();
    Vec3 right   = CPUCameraControl::camera.getRight();
    Vec3 up(0, 1, 0);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        CPUCameraControl::camera.position += forward * moveSpeed;
        cameraMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        CPUCameraControl::camera.position -= forward * moveSpeed;
        cameraMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        CPUCameraControl::camera.position -= right * moveSpeed;
        cameraMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        CPUCameraControl::camera.position += right * moveSpeed;
        cameraMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        CPUCameraControl::camera.position += up * moveSpeed;
        cameraMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        CPUCameraControl::camera.position -= up * moveSpeed;
        cameraMoving = true;
    }

    // ------------ SAVE PPM ------------
    static bool pPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
        savePPMRequested = true;
        pPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        pPressed = false;
    }

    // ------------ TEXTURES ON/OFF ------------
    static bool tPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed) {
        texturesEnabled = !texturesEnabled;
        cameraMoving = true;
        tPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tPressed = false;
    }
}
