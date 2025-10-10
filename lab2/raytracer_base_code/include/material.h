#pragma once
#include "vec3.h"
namespace rt { 
    // Describes the color of the material; No relation to light or reflection
    struct Material{ 
        Vec3 albedo; // RGB value of the material
        bool reflective; // Is this material reflective?
        Material(Vec3 a={0.8,0.8,0.8}, bool r = false):albedo(a),reflective(r){} 
    }; 
} 
