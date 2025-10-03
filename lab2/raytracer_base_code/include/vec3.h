#pragma once
#include <cmath>
namespace rt {
struct Vec3 {
    double x,y,z;
    Vec3(double v=0):x(v),y(v),z(v){} 
    Vec3(double x_,double y_,double z_):x(x_),y(y_),z(z_){} 
    Vec3 operator+(const Vec3& o) const { return {x+o.x,y+o.y,z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x,y-o.y,z-o.z}; }
    Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
    Vec3 operator/(double s) const { return {x/s,y/s,z/s}; }
    Vec3 operator-() const { return {-x,-y,-z}; }
    Vec3& operator+=(const Vec3& o){ x+=o.x; y+=o.y; z+=o.z; return *this; }
};
inline Vec3 operator*(double s, const Vec3& v){ return v*s; }
inline double dot(const Vec3& a,const Vec3& b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 cross(const Vec3& a,const Vec3& b){ return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline double length(const Vec3& v){ return std::sqrt(dot(v,v)); }
inline Vec3 normalize(const Vec3& v){ double L=length(v); return L>0? v/L : v; }
inline Vec3 hadamard(const Vec3& a,const Vec3& b){ return {a.x*b.x,a.y*b.y,a.z*b.z}; }
} // namespace rt
