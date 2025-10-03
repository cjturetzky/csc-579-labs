#pragma once
#include <vector>
#include <memory>
#include "hittable.h"
#include "material.h"
namespace rt {
struct PointLight{ Vec3 pos; Vec3 intensity; };
struct Scene{
    std::vector<std::unique_ptr<Hittable>> objects;
    std::vector<Material> materials;
    std::vector<PointLight> lights;
    int addMaterial(const Material& m){ materials.push_back(m); return (int)materials.size()-1; }
    void add(std::unique_ptr<Hittable> h){ objects.push_back(std::move(h)); }
    bool intersect(const Ray& r,double tmin,double tmax,Hit& best) const{
        Hit temp; bool hitAny=false; double closest=tmax;
        for(const auto& obj: objects){ if(obj->intersect(r,tmin,closest,temp)){ hitAny=true; closest=temp.t; best=temp; } }
        return hitAny;
    }
}; } // namespace rt
