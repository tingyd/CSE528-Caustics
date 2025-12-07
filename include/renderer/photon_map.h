#pragma once
#include "camera.h"
#include <vector>
#include <queue>

struct Photon
{
    Vec3 position;
    Vec3 power;
    Vec3 incomingDir;
    short axis;
};

struct PhotonDistEntry
{
    float distSq;
    const Photon *photon;
    bool operator<(const PhotonDistEntry &other) const
    {
        return distSq < other.distSq;
    }
};
class PhotonMap {
public:
    std::vector<Photon> photons;

    void store(const Vec3 &pos, const Vec3 &power, const Vec3 &inDir);
    void balance();
    void locatePhotons(const Vec3 &pos, int maxPhotons, float &maxDistSq,
                       std::priority_queue<PhotonDistEntry> &heap) const;
    size_t size() const { return photons.size(); }

private:
    void balanceSegment(std::vector<Photon> &balanced, size_t index,
                        size_t start, size_t end);
    void locatePhotonsImpl(const Vec3 &pos, size_t index, int maxPhotons,
                           float &maxDistSq,
                           std::priority_queue<PhotonDistEntry> &heap) const;
};