// main.cpp — Load an OBJ with tinyobjloader, render it, and show normals via geometry shader
// Build (Linux): g++ main.cpp -std=c++17 -lglfw -ldl -lGL -o obj_normals
// Run: ./obj_normals path/to/model.obj
// Windows/MSVC: link opengl32.lib + glfw3 + glad (or use vcpkg: `vcpkg install glfw3 glad`)

#include <iostream>
#include <glad/glad.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <GLFW/glfw3.h>


#include <tiny_obj_loader/tiny_obj_loader.h>

static void check(bool ok, const char* msg) {
    if (!ok) { std::cerr << "[FATAL] " << msg << "\n"; std::exit(EXIT_FAILURE); }
}

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> buf(len > 1 ? len : 1);
        GLsizei written = 0; glGetShaderInfoLog(s, (GLsizei)buf.size(), &written, buf.data());
        std::cerr << "[Shader] " << (written ? buf.data() : "compile failed (no log)") << "\n";
        std::exit(EXIT_FAILURE);
    }
    return s;
}

static GLuint linkProgram(std::initializer_list<GLuint> shaders) {
    GLuint p = glCreateProgram();
    for (auto s : shaders) glAttachShader(p, s);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> buf(len > 1 ? len : 1);
        GLsizei written = 0; glGetProgramInfoLog(p, (GLsizei)buf.size(), &written, buf.data());
        std::cerr << "[Link] " << (written ? buf.data() : "link failed (no log)") << "\n";
        std::exit(EXIT_FAILURE);
    }
    for (auto s : shaders) glDeleteShader(s);
    return p;
}

// ---------- Shaders ----------
static const char* meshVS = R"(#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;
uniform mat4 M, V, P;
out vec3 vNorVS;
void main() {
    mat3 N = mat3(transpose(inverse(M)));
    vec3 nWS = normalize(N * vertNor);
    vNorVS = normalize((mat3(V) * nWS));
    gl_Position = P * V * M * vec4(vertPos,1.0);
}
)";

static const char* meshFS = R"(#version 330 core
in vec3 vNorVS;
out vec4 FragColor;
void main() {
    vec3 L = normalize(vec3(0.6,0.7,0.4));
    float d = clamp(dot(normalize(vNorVS), L), 0.1, 1.0);
    FragColor = vec4(vec3(0.22,0.45,0.9)*d, 1.0);
}
)";

// Geometry-shader pipeline to draw per-vertex normal lines
static const char* normalsVS = R"(#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;
out VS_OUT { vec3 posOS; vec3 norOS; } vs_out;
void main(){
    vs_out.posOS = vertPos;
    vs_out.norOS = vertNor;
    gl_Position = vec4(vertPos,1.0);
}
)";

static const char* normalsGS = R"(#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices=6) out;
in VS_OUT { vec3 posOS; vec3 norOS; } gs_in[];
uniform mat4 M, V, P;
uniform float normalLength;
void emit_line(vec3 a, vec3 b){
    gl_Position = P*V*M*vec4(a,1.0); EmitVertex();
    gl_Position = P*V*M*vec4(b,1.0); EmitVertex();
    EndPrimitive();
}
void main(){
    for(int i=0;i<3;++i){
        vec3 p = gs_in[i].posOS;
        vec3 n = normalize(gs_in[i].norOS);
        emit_line(p, p + n*normalLength);
    }
}
)";

static const char* normalsFS = R"(#version 330 core
out vec4 FragColor;
void main(){ FragColor = vec4(0.1, 0.95, 0.2, 1.0); }
)";

// ---------- Math helpers ----------
static void makePerspective(float fovyDeg, float aspect, float znear, float zfar, float* m) {
    float f = 1.0f / std::tan(fovyDeg * 0.5f * 3.14159265f / 180.0f);
    m[0]=f/aspect; m[1]=0; m[2]=0; m[3]=0;
    m[4]=0; m[5]=f; m[6]=0; m[7]=0;
    m[8]=0; m[9]=0; m[10]=(zfar+znear)/(znear-zfar); m[11]=-1;
    m[12]=0; m[13]=0; m[14]=(2*zfar*znear)/(znear-zfar); m[15]=0;
}
static void makeIdentity(float* m){ for(int i=0;i<16;++i)m[i]=(i%5==0)?1.f:0.f; }
static void mult(const float* a,const float* b,float* r){
    for(int i=0;i<16;++i){ int c=i%4, r0=i/4*4; r[i]=a[r0+0]*b[c+0]+a[r0+1]*b[c+4]+a[r0+2]*b[c+8]+a[r0+3]*b[c+12]; }
}
static void makeTranslate(float x,float y,float z,float* m){ makeIdentity(m); m[12]=x;m[13]=y;m[14]=z; }
static void makeRotateY(float a,float* m){ makeIdentity(m); float c=cosf(a),s=sinf(a); m[0]=c; m[2]=s; m[8]=-s; m[10]=c; }
static void makeScale(float s,float* m){ makeIdentity(m); m[0]=m[5]=m[10]=s; }

// ---------- OBJ loading ----------
struct Vertex { float px,py,pz, nx,ny,nz; };

// Legacy tinyobjloader (no attrib/index_t) version
static bool loadObjExpandTriangles(const std::string& path, std::vector<Vertex>& outVerts) {
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    // Old API signature: LoadObj(shapes, materials, err, filename)
    bool ok = tinyobj::LoadObj(shapes, materials, err, path.c_str());
    if (!err.empty()) std::cerr << "[TINYOBJ] " << err << "\n";
    if (!ok) return false;

    outVerts.clear();

    for (const auto& sh : shapes) {
        const auto& mesh = sh.mesh;

        // positions are packed as xyzxyz..., size = 3 * num_vertices
        const auto& P = mesh.positions;  // std::vector<float>
        const auto& N = mesh.normals;    // may be empty; size = 3 * num_vertices
        const auto& I = mesh.indices;    // flat indices into P/N/TC with a single index stream

        // Expect triangles (most OBJ meshes are triangulated in older loader output)
        if (I.size() % 3 != 0) {
            std::cerr << "[TINYOBJ] indices not divisible by 3; got " << I.size() << "\n";
        }

        for (size_t i = 0; i + 2 < I.size(); i += 3) {
            unsigned int ia = I[i+0];
            unsigned int ib = I[i+1];
            unsigned int ic = I[i+2];

            auto getP = [&](unsigned int idx, float out[3]) {
                out[0] = P[3*idx + 0];
                out[1] = P[3*idx + 1];
                out[2] = P[3*idx + 2];
            };
            auto getN = [&](unsigned int idx, float out[3]) -> bool {
                if (N.empty()) return false;
                out[0] = N[3*idx + 0];
                out[1] = N[3*idx + 1];
                out[2] = N[3*idx + 2];
                return true;
            };

            float pa[3], pb[3], pc[3]; getP(ia, pa); getP(ib, pb); getP(ic, pc);
            float na[3], nb[3], nc[3];
            bool hasNa = getN(ia, na), hasNb = getN(ib, nb), hasNc = getN(ic, nc);

            // If any normal is missing, compute a face normal
            if (!hasNa || !hasNb || !hasNc) {
                float ux = pb[0]-pa[0], uy = pb[1]-pa[1], uz = pb[2]-pa[2];
                float vx = pc[0]-pa[0], vy = pc[1]-pa[1], vz = pc[2]-pa[2];
                float nx = uy*vz - uz*vy;
                float ny = uz*vx - ux*vz;
                float nz = ux*vy - uy*vx;
                float len = std::sqrt(nx*nx + ny*ny + nz*nz); if (len < 1e-8f) len = 1.f;
                nx/=len; ny/=len; nz/=len;
                na[0]=nb[0]=nc[0]=nx; na[1]=nb[1]=nc[1]=ny; na[2]=nb[2]=nc[2]=nz;
            }

            outVerts.push_back({ pa[0], pa[1], pa[2], na[0], na[1], na[2] });
            outVerts.push_back({ pb[0], pb[1], pb[2], nb[0], nb[1], nb[2] });
            outVerts.push_back({ pc[0], pc[1], pc[2], nc[0], nc[1], nc[2] });
        }
    }

    // Normalize to unit-ish size for viewing
    float minx=1e30f,miny=1e30f,minz=1e30f, maxx=-1e30f,maxy=-1e30f,maxz=-1e30f;
    for (auto& v : outVerts) {
        minx = std::min(minx, v.px); miny = std::min(miny, v.py); minz = std::min(minz, v.pz);
        maxx = std::max(maxx, v.px); maxy = std::max(maxy, v.py); maxz = std::max(maxz, v.pz);
    }
    float cx = 0.5f*(minx+maxx), cy = 0.5f*(miny+maxy), cz = 0.5f*(minz+maxz);
    float sx = maxx-minx, sy = maxy-miny, sz = maxz-minz;
    float s = std::max(sx, std::max(sy, sz)); if (s < 1e-6f) s = 1.f;
    float inv = 1.8f / s;

    for (auto& v : outVerts) {
        v.px = (v.px - cx) * inv;
        v.py = (v.py - cy) * inv;
        v.pz = (v.pz - cz) * inv;
        // normals ok as-is (uniform scale)
    }

    return true;
}

int main(int argc, char** argv) {
    const char* objPath = (argc >= 2) ? argv[1] : nullptr;
    if (!objPath) {
        std::cerr << "Usage: " << argv[0] << " path/to/model.obj\n";
        return EXIT_FAILURE;
    }

    check(glfwInit(), "glfwInit failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* win = glfwCreateWindow(900, 700, "OBJ + Geometry Shader (Normals)", nullptr, nullptr);
    check(win, "glfwCreateWindow failed");
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    check(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "gladLoadGLLoader failed");
    glEnable(GL_DEPTH_TEST);

    // Programs
    GLuint pMesh = linkProgram({ compile(GL_VERTEX_SHADER, meshVS),
                                 compile(GL_FRAGMENT_SHADER, meshFS) });
    GLuint pNormals = linkProgram({ compile(GL_VERTEX_SHADER, normalsVS),
                                    compile(GL_GEOMETRY_SHADER, normalsGS),
                                    compile(GL_FRAGMENT_SHADER, normalsFS) });

    GLint uM_mesh = glGetUniformLocation(pMesh, "M");
    GLint uV_mesh = glGetUniformLocation(pMesh, "V");
    GLint uP_mesh = glGetUniformLocation(pMesh, "P");

    GLint uM_norm = glGetUniformLocation(pNormals, "M");
    GLint uV_norm = glGetUniformLocation(pNormals, "V");
    GLint uP_norm = glGetUniformLocation(pNormals, "P");
    GLint uLen    = glGetUniformLocation(pNormals, "normalLength");

    // Load OBJ
    std::vector<Vertex> verts;
    if (!loadObjExpandTriangles(objPath, verts)) {
        std::cerr << "Failed to load OBJ: " << objPath << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "Loaded triangles: " << (verts.size()/3) << " (verts: " << verts.size() << ")\n";

    // Create VAO/VBO
    GLuint vao=0,vbo=0; glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(3*sizeof(float)));
    glBindVertexArray(0);

    // Camera/projection
    int w=900,h=700;
    float P[16]; makePerspective(60.0f, float(w)/float(h), 0.05f, 100.0f, P);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        float t = (float)glfwGetTime();

        glViewport(0,0,w,h);
        glClearColor(0.08f,0.1f,0.14f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Model/View: spin a little and move back
        float T[16], R[16], S[16], M[16], V[16], TR[16];
        makeTranslate(0.f, 0.f, -3.2f, T);
        makeRotateY(0.0f, R);
        // makeRotateY(t*0.5f, R);
        makeScale(1.0f, S);
        mult(T,R,TR); mult(TR,S,M);
        makeIdentity(V);

        // 1) Draw the mesh solid
        glUseProgram(pMesh);
        glUniformMatrix4fv(uM_mesh,1,GL_FALSE,M);
        glUniformMatrix4fv(uV_mesh,1,GL_FALSE,V);
        glUniformMatrix4fv(uP_mesh,1,GL_FALSE,P);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindVertexArray(0);

        // 2) Draw normals via geometry shader
        glUseProgram(pNormals);
        glUniformMatrix4fv(uM_norm,1,GL_FALSE,M);
        glUniformMatrix4fv(uV_norm,1,GL_FALSE,V);
        glUniformMatrix4fv(uP_norm,1,GL_FALSE,P);
        glUniform1f(uLen, 0.08f);
        glLineWidth(2.0f);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindVertexArray(0);
        glLineWidth(1.0f);

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1,&vao);
    glDeleteBuffers(1,&vbo);
    glDeleteProgram(pMesh);
    glDeleteProgram(pNormals);
    glfwTerminate();
    return 0;
}
