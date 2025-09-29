#version 330 core
layout(location=0) in vec3 vertPos;
// layout(location=1) in vec3 vertNor;

uniform mat4 M, V, P;
// uniform float outlineScale; // small scale around origin, e.g. 0.01–0.03

void main() {
    vec3 pos = vertPos * (1.01);
    gl_Position = P * V * (M * vec4(pos, 1.0));
}