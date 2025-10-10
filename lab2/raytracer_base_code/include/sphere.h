#pragma once
#include "hittable.h"
namespace rt {
struct Sphere: Hittable{
    Vec3 c; // Center of the sphere
    double R; // Radius of the sphere
    int matId; // Material ID

    // Constructor
    Sphere(const Vec3& c_, double R_, int m):c(c_),R(R_),matId(m){}

    // Detects if r intersects the sphere at any point
    bool intersect(const Ray& r,double tmin,double tmax,Hit& rec) const override{
        Vec3 oc = r.o - c; // Vector between origin of r and center of sphere
        double a=dot(r.d,r.d); // Dot product between direction of r and direction of r
        double b=dot(oc,r.d); // Dot product between oc and direction of r
        double c2=dot(oc,oc)-R*R; // Dot product between oc and itself, subtract the square of the radius
        double disc=b*b - a*c2; // Determines if the ray r is hitting the sphere

        // A positive disc value means the ray has hit the sphere
        if(disc<0) return false; 

        double sdisc=std::sqrt(disc); // Square root of disc
        double t=(-b - sdisc)/a; 

        if(t<tmin||t>tmax){ 
            t=(-b + sdisc)/a; 
            if(t<tmin||t>tmax) return false;
        }

        rec.t=t; // Set values in passed Hit object; Allows caller to determine properties of the sphere
        rec.p=r.at(t); 
        rec.n=normalize(rec.p - c); 
        rec.matId=matId; 
        rec.hit=true; 

        return true;
    }
}; } // namespace rt
