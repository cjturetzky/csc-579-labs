#pragma once
#include "ray.h"
namespace rt {
struct Hit{ double t; Vec3 p; Vec3 n; int matId; bool hit=false; };
struct Hittable{ virtual ~Hittable()=default; virtual bool intersect(const Ray&, double, double, Hit&) const = 0; };
} // namespace rt
