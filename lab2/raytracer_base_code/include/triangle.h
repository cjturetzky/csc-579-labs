#pragma once
#include <cmath>
#include "hittable.h"
namespace rt {
struct Triangle: Hittable{
    Vec3 a,b,c; Vec3 n; int matId;
    Triangle(const Vec3&A,const Vec3&B,const Vec3&C,int m):a(A),b(B),c(C),matId(m){ n=normalize(cross(b-a,c-a)); }
    bool intersect(const Ray& r,double tmin,double tmax,Hit& rec) const override{
        const double EPS=1e-9; Vec3 ab=b-a, ac=c-a, p=cross(r.d, ac);
        double det=dot(ab,p); if(std::fabs(det)<EPS) return false; double invDet=1.0/det;
        Vec3 tvec=r.o - a; double u=dot(tvec,p)*invDet; if(u<0||u>1) return false;
        Vec3 q=cross(tvec,ab); double v=dot(r.d,q)*invDet; if(v<0||u+v>1) return false;
        double t=dot(ac,q)*invDet; if(t<tmin||t>tmax) return false; rec.t=t; rec.p=r.at(t); rec.n=n; rec.matId=matId; rec.hit=true; return true;
    }
}; } // namespace rt
