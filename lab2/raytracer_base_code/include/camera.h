#pragma once
#include <cmath>
#include "vec3.h"
#include "ray.h"
namespace rt {
constexpr double PI = 3.14159265358979323846;
struct Camera {
    Vec3 eye,look,up; 
    double vfov; 
    int W,H;
    Vec3 u,v,w; 
    double aspect, halfH, halfW;

    // Constructor
    Camera(const Vec3& eye_,const Vec3& look_,const Vec3& up_, double vfov_deg,int W_,int H_)
    : eye(eye_),look(look_),up(up_),vfov(vfov_deg*(PI/180.0)),W(W_),H(H_){
        aspect = double(W)/H; halfH = std::tan(vfov/2.0); halfW = aspect*halfH;
        w = normalize(eye - look); u = normalize(cross(up, w)); v = cross(w, u);
    }

    // Returns the normalized ray pointing toward the specified pixel
    Ray primary(double sx,double sy) const{ 
        Vec3 dir = normalize(-w + sx*halfW*u + sy*halfH*v); 
        return Ray(eye, dir); }
};
} // namespace rt
