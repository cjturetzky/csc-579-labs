#pragma once
#include "ray.h"

// Interface describing an object that can be hit by a ray
// Must implement the Intersect function to determine if a given ray hits the object
namespace rt {
    struct Hit{ 
    double t; 
    Vec3 p; 
    Vec3 n; 
    int matId; 
    bool hit=false; 
};

struct Hittable{ 
    virtual ~Hittable()=default; 
    virtual bool intersect(const Ray&, double, double, Hit&) const = 0; 
};
} // namespace rt
