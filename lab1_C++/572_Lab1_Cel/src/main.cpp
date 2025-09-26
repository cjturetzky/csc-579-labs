// main.cpp — OBJ loader + Cel (Toon) Shading with Outline (A/D rotate, W/S zoom)
// Build (Linux): g++ main.cpp -std=c++17 -lglfw -ldl -lGL -o obj_cel
// Run: ./obj_cel path/to/model.obj
// Windows/MSVC: link opengl32.lib + glfw3 + glad (or use vcpkg: `vcpkg install glfw3 glad`)

#include <iostream>
#include <glad/glad.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
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

// ---------- Shaders (embedded) ----------
// Cel shading: quantized diffuse + simple thresholded specular + optional rim
static const char* celVS = R"(#version 330 core
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
)";

static const char* celFS = R"(#version 330 core
in vec3 vN;
in vec3 vL;
in vec3 vV;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

out vec4 FragColor;

float stepBand(float x, float bands) { return floor(x * bands) / bands; }

void main() {
    vec3 N = normalize(vN);
    vec3 L = normalize(vL);
    vec3 V = normalize(vV);
    vec3 H = normalize(L + V);

    // Quantize diffuse into a few bands
    float ndl = max(dot(N, L), 0.0);
    float bands = 3.0;                 // try 2–4
    float dQ = stepBand(ndl, bands);

    // Thresholded specular highlight
    float nsh = pow(max(dot(N, H), 0.0), MatShine);
    float sQ = step(0.5, nsh);         // binary highlight

    // Optional rim (ink near silhouette)
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    float rimQ = step(0.6, rim) * 0.25;

    vec3 color = MatAmb + dQ * MatDif + sQ * MatSpec + rimQ * vec3(1.0);
    FragColor = vec4(color, 1.0);
}
)";

// Outline pass: draw backfaces slightly “inflated” to create a silhouette
static const char* outlineVS = R"(#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;

uniform mat4 M, V, P;
uniform float outlineScale; // small scale around origin, e.g. 0.01–0.03

void main() {
    vec3 pos = vertPos * (1.0 + outlineScale);
    gl_Position = P * V * (M * vec4(pos, 1.0));
}
)";

static const char* outlineFS = R"(#version 330 core
out vec4 FragColor;
void main(){ FragColor = vec4(0.0,0.0,0.0,1.0); } // black outline
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

// ---------- OBJ loading (legacy tinyobj API) ----------
struct Vertex { float px,py,pz, nx,ny,nz; };

static bool loadObjExpandTriangles(const std::string& path, std::vector<Vertex>& outVerts) {
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    bool ok = tinyobj::LoadObj(shapes, materials, err, path.c_str());
    if (!err.empty()) std::cerr << "[TINYOBJ] " << err << "\n";
    if (!ok) return false;

    outVerts.clear();

    for (const auto& sh : shapes) {
        const auto& mesh = sh.mesh;

        const auto& P = mesh.positions;  // xyz packed
        const auto& N = mesh.normals;    // may be empty
        const auto& I = mesh.indices;    // triangle indices

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

    // Normalize to unit-ish size
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
    }
    return true;
}

// ----------- Input state -----------
static float gYaw = 0.0f;     // radians, Y rotation (A/D)
static float gDist = -3.2f;   // camera distance along -Z (W/S moves this toward/away)
static double gPrevTime = 0.0;

static void handleInput(GLFWwindow* win) {
    double now = glfwGetTime();
    double dt = std::max(0.0, now - gPrevTime);
    gPrevTime = now;

    const float rotSpeed = 1.6f;   // radians per second
    const float zoomSpeed = 2.0f;  // world units per second

    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) gYaw -= rotSpeed * (float)dt;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) gYaw += rotSpeed * (float)dt;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) gDist += zoomSpeed * (float)dt; // move closer (less negative)
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) gDist -= zoomSpeed * (float)dt; // move away (more negative)

    // Clamp distance to a sensible range
    if (gDist > -0.8f)  gDist = -0.8f;   // don't cross the near plane
    if (gDist < -10.0f) gDist = -10.0f;  // not too far
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
    GLFWwindow* win = glfwCreateWindow(900, 700, "OBJ + Cel Shading (A/D rotate, W/S zoom)", nullptr, nullptr);
    check(win, "glfwCreateWindow failed");
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    check(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "gladLoadGLLoader failed");
    glEnable(GL_DEPTH_TEST);

    // Programs: outline and cel
    GLuint pOutline = linkProgram({ compile(GL_VERTEX_SHADER,   outlineVS),
                                    compile(GL_FRAGMENT_SHADER, outlineFS) });
    GLuint pCel     = linkProgram({ compile(GL_VERTEX_SHADER,   celVS),
                                    compile(GL_FRAGMENT_SHADER, celFS) });

    // Uniform locations for cel
    GLint uM_cel = glGetUniformLocation(pCel, "M");
    GLint uV_cel = glGetUniformLocation(pCel, "V");
    GLint uP_cel = glGetUniformLocation(pCel, "P");
    GLint uLight = glGetUniformLocation(pCel, "lightPos");
    GLint uAmb   = glGetUniformLocation(pCel, "MatAmb");
    GLint uDif   = glGetUniformLocation(pCel, "MatDif");
    GLint uSpec  = glGetUniformLocation(pCel, "MatSpec");
    GLint uShine = glGetUniformLocation(pCel, "MatShine");

    // Uniform locations for outline
    GLint uM_out = glGetUniformLocation(pOutline, "M");
    GLint uV_out = glGetUniformLocation(pOutline, "V");
    GLint uP_out = glGetUniformLocation(pOutline, "P");
    GLint uS_out = glGetUniformLocation(pOutline, "outlineScale");

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

    // Material defaults
    const float amb[3]  = {0.15f, 0.15f, 0.15f};
    const float dif[3]  = {0.80f, 0.65f, 0.20f};
    const float spec[3] = {0.25f, 0.25f, 0.25f};
    const float shine   = 96.0f;
    const float light[3]= {2.5f, 2.0f, 2.5f}; // world-space point light

    gPrevTime = glfwGetTime(); // init dt timer

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        handleInput(win); // <-- A/D/W/S

        glViewport(0,0,w,h);
        glClearColor(0.08f,0.1f,0.14f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Model/View: rotate by yaw and position camera at gDist on -Z
        float T[16], R[16], S[16], M[16], V[16], TR[16];
        makeTranslate(0.f, 0.f, gDist, T);    // zoom in/out with W/S
        makeRotateY(gYaw, R);                 // rotate with A/D
        makeScale(1.0f, S);
        mult(T,R,TR); mult(TR,S,M);
        makeIdentity(V);

        // --- 1) Outline pass: backfaces, slightly inflated
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glUseProgram(pOutline);
        glUniformMatrix4fv(uM_out,1,GL_FALSE,M);
        glUniformMatrix4fv(uV_out,1,GL_FALSE,V);
        glUniformMatrix4fv(uP_out,1,GL_FALSE,P);
        glUniform1f(uS_out, 0.02f); // outline thickness (0.01–0.03 typical)

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindVertexArray(0);

        glCullFace(GL_BACK); // restore

        // --- 2) Cel pass: frontfaces, quantized lighting
        glUseProgram(pCel);
        glUniformMatrix4fv(uM_cel,1,GL_FALSE,M);
        glUniformMatrix4fv(uV_cel,1,GL_FALSE,V);
        glUniformMatrix4fv(uP_cel,1,GL_FALSE,P);
        glUniform3fv(uLight,1,light);
        glUniform3fv(uAmb,1,amb);
        glUniform3fv(uDif,1,dif);
        glUniform3fv(uSpec,1,spec);
        glUniform1f(uShine, shine);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindVertexArray(0);

        glDisable(GL_CULL_FACE);

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1,&vao);
    glDeleteBuffers(1,&vbo);
    glDeleteProgram(pOutline);
    glDeleteProgram(pCel);
    glfwTerminate();
    return 0;
}
