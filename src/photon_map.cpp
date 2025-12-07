#include "renderer/photon_map.h"
#include <algorithm>

void PhotonMap::store(const Vec3 &pos, const Vec3 &power, const Vec3 &inDir)
{
    Photon p;
    p.position = pos;
    p.power = power;
    p.incomingDir = inDir;
    p.axis = 0;
    photons.push_back(p);
}

void PhotonMap::balance()
{
    if (photons.empty())
        return;
    std::vector<Photon> balanced(photons.size());
    balanceSegment(balanced, 0, 0, photons.size());
    photons = std::move(balanced);
}

void PhotonMap::balanceSegment(std::vector<Photon> &balanced, size_t index,
                               size_t start, size_t end)
{
    if (start >= end)
        return;

    Vec3 bboxMin(1e30f, 1e30f, 1e30f);
    Vec3 bboxMax(-1e30f, -1e30f, -1e30f);

    for (size_t i = start; i < end; i++)
    {
        bboxMin = Vec3(
            std::min(bboxMin.x, photons[i].position.x),
            std::min(bboxMin.y, photons[i].position.y),
            std::min(bboxMin.z, photons[i].position.z));
        bboxMax = Vec3(
            std::max(bboxMax.x, photons[i].position.x),
            std::max(bboxMax.y, photons[i].position.y),
            std::max(bboxMax.z, photons[i].position.z));
    }

    Vec3 extent = bboxMax - bboxMin;
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z)
        axis = 1;
    else if (extent.z > extent.x && extent.z > extent.y)
        axis = 2;

    size_t mid = (start + end) / 2;
    std::nth_element(photons.begin() + start, photons.begin() + mid,
                     photons.begin() + end,
                     [axis](const Photon &a, const Photon &b)
                     {
                         return a.position[axis] < b.position[axis];
                     });

    balanced[index] = photons[mid];
    balanced[index].axis = axis;

    size_t leftChild = 2 * index + 1;
    size_t rightChild = 2 * index + 2;

    if (mid > start && leftChild < balanced.size())
    {
        balanceSegment(balanced, leftChild, start, mid);
    }
    if (mid + 1 < end && rightChild < balanced.size())
    {
        balanceSegment(balanced, rightChild, mid + 1, end);
    }
}

void PhotonMap::locatePhotons(const Vec3 &pos, int maxPhotons, float &maxDistSq,
                              std::priority_queue<PhotonDistEntry> &heap) const
{
    if (photons.empty())
        return;
    locatePhotonsImpl(pos, 0, maxPhotons, maxDistSq, heap);
}

void PhotonMap::locatePhotonsImpl(const Vec3 &pos, size_t index, int maxPhotons,
                                  float &maxDistSq,
                                  std::priority_queue<PhotonDistEntry> &heap) const
{
    if (index >= photons.size())
        return;

    const Photon &p = photons[index];
    int axis = p.axis;

    float delta = pos[axis] - p.position[axis];

    size_t nearChild = (delta < 0) ? (2 * index + 1) : (2 * index + 2);
    size_t farChild = (delta < 0) ? (2 * index + 2) : (2 * index + 1);

    if (nearChild < photons.size())
    {
        locatePhotonsImpl(pos, nearChild, maxPhotons, maxDistSq, heap);
    }

    Vec3 diff = pos - p.position;
    float distSq = diff.lengthSq();

    if (distSq < maxDistSq)
    {
        PhotonDistEntry entry;
        entry.distSq = distSq;
        entry.photon = &p;
        heap.push(entry);

        if ((int)heap.size() > maxPhotons)
        {
            heap.pop();
            maxDistSq = heap.top().distSq;
        }
    }

    if (delta * delta < maxDistSq && farChild < photons.size())
    {
        locatePhotonsImpl(pos, farChild, maxPhotons, maxDistSq, heap);
    }
}