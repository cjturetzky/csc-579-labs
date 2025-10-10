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

    // Adds a material to the vector storing them
    int addMaterial(const Material& m){ 
        materials.push_back(m); 
        return (int)materials.size()-1; 
    }

    // Adds a hittable object to the vector
    void add(std::unique_ptr<Hittable> h){ 
        objects.push_back(std::move(h)); 
    }

    // Detects any intersection between r and all objects in the scene
    // Returns true if an object is hit
    bool intersect(const Ray& r,double tmin,double tmax,Hit& best) const{
        Hit temp; 
        bool hitAny=false; 
        double closest=tmax;
        for(const auto& obj: objects){ // Iterate through all objects to determine if there is a hit 
            if(obj->intersect(r,tmin,closest,temp)){
                hitAny=true; 
                closest=temp.t; // 
                best=temp; // Save object information in best for later use
            } 
        }
        return hitAny;
    }
}; } // namespace rt
