#pragma once
#include <cmath>
#include "hittable.h"
namespace rt {
struct Plane: Hittable{
    Vec3 p0; Vec3 n; int matId;
    Plane(const Vec3& p0_,const Vec3& n_,int m):p0(p0_),n(normalize(n_)),matId(m){}
    bool intersect(const Ray& r,double tmin,double tmax,Hit& rec) const override{
        double denom=dot(n,r.d); if(std::fabs(denom)<1e-8) return false;
        double t=dot(p0 - r.o, n)/denom; if(t<tmin||t>tmax) return false;
        rec.t=t; rec.p=r.at(t); rec.n = denom<0? n : (n * -1.0); rec.matId=matId; rec.hit=true; return true;
    }
}; } // namespace rt
