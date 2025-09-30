#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;
out VS_OUT { vec3 posOS; vec3 norOS; } vs_out;
void main(){
    vs_out.posOS = vertPos;
    vs_out.norOS = vertNor;
    gl_Position = vec4(vertPos,1.0);
}