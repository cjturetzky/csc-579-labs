#pragma once
#include "hittable.h"
namespace rt {
struct Sphere: Hittable{
    Vec3 c; double R; int matId;
    Sphere(const Vec3& c_, double R_, int m):c(c_),R(R_),matId(m){}
    bool intersect(const Ray& r,double tmin,double tmax,Hit& rec) const override{
        Vec3 oc = r.o - c; double a=dot(r.d,r.d); double b=dot(oc,r.d); double c2=dot(oc,oc)-R*R;
        double disc=b*b - a*c2; if(disc<0) return false; double sdisc=std::sqrt(disc);
        double t=(-b - sdisc)/a; if(t<tmin||t>tmax){ t=(-b + sdisc)/a; if(t<tmin||t>tmax) return false; }
        rec.t=t; rec.p=r.at(t); rec.n=normalize(rec.p - c); rec.matId=matId; rec.hit=true; return true;
    }
}; } // namespace rt
