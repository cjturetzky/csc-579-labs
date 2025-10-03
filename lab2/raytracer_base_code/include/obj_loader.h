#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include "triangle.h"
#include "scene.h"
namespace rt {
inline bool load_obj_positions_indices(const std::string& path, std::vector<Vec3>& outV, std::vector<uint32_t>& outI){
    std::ifstream in(path); if(!in.is_open()) return false; std::string line; outV.clear(); outI.clear();
    auto parse_index=[](const std::string& tok)->int{ size_t s=tok.find('/'); if(s==std::string::npos) return std::stoi(tok); return std::stoi(tok.substr(0,s)); };
    while(std::getline(in,line)){ if(line.empty()||line[0]=='#') continue; std::istringstream iss(line); std::string h; iss>>h;
        if(h=="v"){ double x,y,z; iss>>x>>y>>z; outV.push_back({x,y,z}); }
        else if(h=="f"){ std::vector<int> idx; std::string tok; while(iss>>tok){ idx.push_back(parse_index(tok)); } if(idx.size()<3) continue;
            for(size_t i=1;i+1<idx.size();++i){ outI.push_back((uint32_t)(idx[0]-1)); outI.push_back((uint32_t)(idx[i]-1)); outI.push_back((uint32_t)(idx[i+1]-1)); } }
    } return true;
}
inline void add_mesh(Scene& sc,const std::vector<Vec3>& V,const std::vector<uint32_t>& I,int matId,const Vec3& scale={1,1,1},const Vec3& translate={0,0,0}){
    auto apply=[&](const Vec3&p)->Vec3{ return {p.x*scale.x+translate.x, p.y*scale.y+translate.y, p.z*scale.z+translate.z}; };
    for(size_t i=0;i+2<I.size();i+=3){ Vec3 a=apply(V[I[i]]), b=apply(V[I[i+1]]), c=apply(V[I[i+2]]); sc.add(std::make_unique<Triangle>(a,b,c,matId)); }
}
} // namespace rt
