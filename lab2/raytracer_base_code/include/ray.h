#pragma once
#include "vec3.h"
namespace rt {
struct Ray{ 
    Vec3 o,d; // Origin, Direction
    Ray(const Vec3&o_,const Vec3&d_):o(o_),d(d_){} 
    Vec3 at(double t) const { return o + d*t; }
};
} // namespace rt
