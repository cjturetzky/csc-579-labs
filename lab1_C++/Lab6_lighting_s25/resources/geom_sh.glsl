#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices=6) out;
in VS_OUT { vec3 posOS; vec3 norOS; } gs_in[];
uniform mat4 M, V, P;
void emit_line(vec3 a, vec3 b){
    gl_Position = P*V*M*vec4(a,1.0); EmitVertex();
    gl_Position = P*V*M*vec4(b,1.0); EmitVertex();
    EndPrimitive();
}
void main(){
    for(int i=0;i<3;++i){
        vec3 p = gs_in[i].posOS;
        vec3 n = normalize(gs_in[i].norOS);
        emit_line(p, p + n*0.005);
    }
}