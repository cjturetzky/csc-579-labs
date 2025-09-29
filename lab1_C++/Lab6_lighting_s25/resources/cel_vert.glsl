#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;

uniform mat4 M, V, P;
uniform vec3 lightPos; // world-space

out vec3 vN;  // normal in view space
out vec3 vL;  // light dir in view space
out vec3 vV;  // view dir in view space

void main() {
    vec4 wPos = M * vec4(vertPos, 1.0);
    gl_Position = P * V * wPos;

    mat3 N = mat3(transpose(inverse(M)));
    vec3 nWS = normalize(N * vertNor);
    vN = normalize((V * vec4(nWS, 0.0)).xyz);

    vec3 Lws = lightPos - wPos.xyz;
    vL = normalize((V * vec4(Lws, 0.0)).xyz);
    vV = normalize(-(V * wPos).xyz);
}